/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "watchdog.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <optional>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>

using phosphor::watchdog::Watchdog;
using phosphor::watchdog::WatchdogBase;
using sdbusplus::xyz::openbmc_project::State::server::convertForMessage;

void printActionTargetMap(const Watchdog::ActionTargetMap& actionTargetMap)
{
    std::cerr << "Action Targets:\n";
    for (const auto& [action, target] : actionTargetMap)
    {
        std::cerr << "  " << convertForMessage(action) << " -> " << target
                  << "\n";
    }
    std::cerr << std::flush;
}

void printFallback(const Watchdog::Fallback& fallback)
{
    std::cerr << "Fallback Options:\n";
    std::cerr << "  Action: " << convertForMessage(fallback.action) << "\n";
    std::cerr << "  Interval(ms): " << std::dec << fallback.interval << "\n";
    std::cerr << "  Always re-execute: " << std::boolalpha << fallback.always
              << "\n";
    std::cerr << std::flush;
}

int main(int argc, char* argv[])
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    CLI::App app{"Canonical openbmc host watchdog daemon"};

    // Service related options
    const std::string serviceGroup = "Service Options";
    std::string path;
    app.add_option("-p,--path", path,
                   "DBus Object Path. "
                   "Ex: /xyz/openbmc_project/state/watchdog/host0")
        ->required()
        ->group(serviceGroup);
    std::string service;
    app.add_option("-s,--service", service,
                   "DBus Service Name. "
                   "Ex: xyz.openbmc_project.State.Watchdog.Host")
        ->required()
        ->group(serviceGroup);
    bool continueAfterTimeout{false};
    app.add_flag("-c,--continue", continueAfterTimeout,
                 "Continue daemon after watchdog timeout")
        ->group(serviceGroup);

    // Target related options
    const std::string targetGroup = "Target Options";
    std::optional<std::string> target;
    app.add_option("-t,--target", target,
                   "Systemd unit to be called on "
                   "timeout for all actions but NONE. "
                   "Deprecated, use --action_target instead.")
        ->group(targetGroup);
    std::vector<std::string> actionTargets;
    app.add_option("-a,--action_target", actionTargets,
                   "Map of action to "
                   "systemd unit to be called on timeout if that action is "
                   "set for ExpireAction when the timer expires.")
        ->group(targetGroup);

    // Fallback related options
    const std::string fallbackGroup = "Fallback Options";
    std::optional<std::string> fallbackAction;
    auto fallbackActionOpt =
        app.add_option("-f,--fallback_action", fallbackAction,
                       "Enables the "
                       "watchdog even when disabled via the dbus interface. "
                       "Perform this action when the fallback expires.")
            ->group(fallbackGroup);
    std::optional<unsigned> fallbackIntervalMs;
    auto fallbackIntervalOpt =
        app.add_option("-i,--fallback_interval", fallbackIntervalMs,
                       "Enables the "
                       "watchdog even when disabled via the dbus interface. "
                       "Waits for this interval before performing the fallback "
                       "action.")
            ->group(fallbackGroup);
    fallbackIntervalOpt->needs(fallbackActionOpt);
    fallbackActionOpt->needs(fallbackIntervalOpt);
    bool fallbackAlways{false};
    app.add_flag("-e,--fallback_always", fallbackAlways,
                 "Enables the "
                 "watchdog even when disabled by the dbus interface. "
                 "This option is only valid with a fallback specified")
        ->group(fallbackGroup)
        ->needs(fallbackActionOpt)
        ->needs(fallbackIntervalOpt);

    // Should we watch for postcodes
    bool watchPostcodes{false};
    app.add_flag("-w,--watch_postcodes", watchPostcodes,
                 "Should we reset the time remaining any time a postcode "
                 "is signaled.");

    // Interval related options
    uint64_t minInterval = phosphor::watchdog::DEFAULT_MIN_INTERVAL_MS;
    app.add_option("-m,--min_interval", minInterval,
                   "Set minimum interval for watchdog in milliseconds");

    // 0 to indicate to use default from PDI if not passed in
    uint64_t defaultInterval = 0;
    app.add_option("-d,--default_interval", defaultInterval,
                   "Set default interval for watchdog in milliseconds");

    CLI11_PARSE(app, argc, argv);

    // Put together a list of actions and associated systemd targets
    // The new --action_target options take precedence over the legacy
    // --target
    Watchdog::ActionTargetMap actionTargetMap;
    if (target)
    {
        actionTargetMap[Watchdog::Action::HardReset] = *target;
        actionTargetMap[Watchdog::Action::PowerOff] = *target;
        actionTargetMap[Watchdog::Action::PowerCycle] = *target;
    }
    for (const auto& actionTarget : actionTargets)
    {
        size_t keyValueSplit = actionTarget.find("=");
        if (keyValueSplit == std::string::npos)
        {
            std::cerr << "Invalid action_target format, "
                         "expect <action>=<target>."
                      << std::endl;
            return 1;
        }

        std::string key = actionTarget.substr(0, keyValueSplit);
        std::string value = actionTarget.substr(keyValueSplit + 1);

        // Convert an action from a fully namespaced value
        Watchdog::Action action;
        try
        {
            action = WatchdogBase::convertActionFromString(key);
        }
        catch (const sdbusplus::exception::InvalidEnumString&)
        {
            std::cerr << "Bad action specified: " << key << std::endl;
            return 1;
        }

        // Detect duplicate action target arguments
        if (actionTargetMap.find(action) != actionTargetMap.end())
        {
            std::cerr << "Got duplicate action: " << key << std::endl;
            return 1;
        }

        actionTargetMap[action] = std::move(value);
    }
    printActionTargetMap(actionTargetMap);

    // Build the fallback option used for the Watchdog
    std::optional<Watchdog::Fallback> maybeFallback;
    if (fallbackAction)
    {
        Watchdog::Fallback fallback;
        try
        {
            fallback.action =
                WatchdogBase::convertActionFromString(*fallbackAction);
        }
        catch (const sdbusplus::exception::InvalidEnumString&)
        {
            std::cerr << "Bad fallback action specified: " << *fallbackAction
                      << std::endl;
            return 1;
        }
        fallback.interval = *fallbackIntervalMs;
        fallback.always = fallbackAlways;

        printFallback(fallback);
        maybeFallback = fallback;
    }

    try
    {
        boost::asio::io_context io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);

        // Add systemd object manager.
        sdbusplus::server::manager::manager watchdogManager(*conn,
                                                            path.c_str());

        bool done = false;

        // Create a watchdog object
        Watchdog watchdog(io, conn, path.c_str(), std::move(actionTargetMap),
                          std::move(maybeFallback), minInterval,
                          defaultInterval,
                          [continueAfterTimeout, &io, &done]() {
                              if (done || !continueAfterTimeout)
                              {
                                  io.stop();
                              }
                          });

        std::optional<sdbusplus::bus::match::match> watchPostcodeMatch;
        if (watchPostcodes)
        {
            watchPostcodeMatch.emplace(
                *conn,
                sdbusplus::bus::match::rules::propertiesChanged(
                    "/xyz/openbmc_project/state/boot/raw0",
                    "xyz.openbmc_project.State.Boot.Raw"),
                std::bind(&Watchdog::resetTimeRemaining, std::ref(watchdog),
                          false));
        }

        // Claim the bus
        conn->request_name(service.c_str());

        auto intCb = [&done](const boost::system::error_code& error, int) {
            if (!error)
            {
                done = true;
            }
        };
        boost::asio::signal_set sigintSet(io, SIGINT);
        sigintSet.async_wait(intCb);
        boost::asio::signal_set sigtermSet(io, SIGTERM);
        sigtermSet.async_wait(std::move(intCb));

        // Loop until our timer expires and we don't want to continue
        io.run();
    }
    catch (const InternalFailure& e)
    {
        phosphor::logging::commit<InternalFailure>();

        // Need a coredump in the error cases.
        std::terminate();
    }
    return 0;
}
