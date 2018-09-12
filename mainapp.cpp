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

#include "argument.hpp"
#include "watchdog.hpp"

#include <experimental/optional>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>

using phosphor::watchdog::ArgumentParser;
using phosphor::watchdog::Watchdog;
using sdbusplus::xyz::openbmc_project::State::server::convertForMessage;

static void exitWithError(const char* err, char** argv)
{
    ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

void printActionTargets(
    const std::map<Watchdog::Action, std::string>& actionTargets)
{
    std::cerr << "Action Targets:\n";
    for (const auto& actionTarget : actionTargets)
    {
        std::cerr << "  " << convertForMessage(actionTarget.first) << " -> "
                  << actionTarget.second << "\n";
    }
    std::cerr << std::flush;
}

int main(int argc, char** argv)
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
    // Read arguments.
    auto options = ArgumentParser(argc, argv);

    // Parse out continue argument.
    auto continueParam = (options)["continue"];
    // Default it to exit on watchdog timeout
    auto continueAfterTimeout = false;
    if (!continueParam.empty())
    {
        continueAfterTimeout = true;
    }

    // Parse out path argument.
    auto pathParam = (options)["path"];
    if (pathParam.empty())
    {
        exitWithError("Path not specified.", argv);
    }
    if (pathParam.size() > 1)
    {
        exitWithError("Multiple paths specified.", argv);
    }
    auto path = pathParam.back();

    // Parse out service name argument
    auto serviceParam = (options)["service"];
    if (serviceParam.empty())
    {
        exitWithError("Service not specified.", argv);
    }
    if (serviceParam.size() > 1)
    {
        exitWithError("Multiple services specified.", argv);
    }
    auto service = serviceParam.back();

    // Parse out target argument. It is fine if the caller does not
    // pass this if they are not interested in calling into any target
    // on meeting a condition.
    auto targetParam = (options)["target"];
    if (targetParam.size() > 1)
    {
        exitWithError("Multiple targets specified.", argv);
    }
    std::map<Watchdog::Action, Watchdog::TargetName> actionTargets;
    if (!targetParam.empty())
    {
        auto target = targetParam.back();
        actionTargets[Watchdog::Action::HardReset] = target;
        actionTargets[Watchdog::Action::PowerOff] = target;
        actionTargets[Watchdog::Action::PowerCycle] = target;
    }

    // Parse out the action_target arguments. We allow one target to map
    // to an action. These targets can replace the target specified above.
    for (const auto& actionTarget : (options)["action_target"])
    {
        size_t keyValueSplit = actionTarget.find("=");
        if (keyValueSplit == std::string::npos)
        {
            exitWithError(
                "Invalid action_target format, expect <action>=<target>.",
                argv);
        }

        std::string key = actionTarget.substr(0, keyValueSplit);
        std::string value = actionTarget.substr(keyValueSplit + 1);

        // Convert an action from a fully namespaced value
        Watchdog::Action action;
        try
        {
            action = Watchdog::convertActionFromString(key);
        }
        catch (const sdbusplus::exception::InvalidEnumString&)
        {
            exitWithError("Bad action specified.", argv);
        }

        actionTargets[action] = std::move(value);
    }
    printActionTargets(actionTargets);

    // Parse out the fallback settings for the watchdog. Note that we require
    // both of the fallback arguments to do anything here, but having a fallback
    // is entirely optional.
    auto fallbackActionParam = (options)["fallback_action"];
    auto fallbackIntervalParam = (options)["fallback_interval"];
    if (fallbackActionParam.empty() ^ fallbackIntervalParam.empty())
    {
        exitWithError("Only one of the fallback options was specified.", argv);
    }
    if (fallbackActionParam.size() > 1 || fallbackIntervalParam.size() > 1)
    {
        exitWithError("Multiple fallbacks specified.", argv);
    }
    std::experimental::optional<Watchdog::Fallback> fallback;
    if (!fallbackActionParam.empty())
    {
        Watchdog::Action action;
        try
        {
            action =
                Watchdog::convertActionFromString(fallbackActionParam.back());
        }
        catch (const sdbusplus::exception::InvalidEnumString&)
        {
            exitWithError("Bad action specified.", argv);
        }
        uint64_t interval;
        try
        {
            interval = std::stoull(fallbackIntervalParam.back());
        }
        catch (const std::logic_error&)
        {
            exitWithError("Failed to convert fallback interval to integer.",
                          argv);
        }
        fallback = Watchdog::Fallback{
            .action = action,
            .interval = interval,
            .always = false,
        };
    }

    auto fallbackAlwaysParam = (options)["fallback_always"];
    if (!fallbackAlwaysParam.empty())
    {
        if (!fallback)
        {
            exitWithError("Specified the fallback should always be enabled but "
                          "no fallback provided.",
                          argv);
        }
        fallback->always = true;
    }

    sd_event* event = nullptr;
    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return r;
    }
    phosphor::watchdog::EventPtr eventP{event};
    event = nullptr;

    // Get a handle to system dbus.
    auto bus = sdbusplus::bus::new_default();

    // Add systemd object manager.
    sdbusplus::server::manager::manager(bus, path.c_str());

    // Attach the bus to sd_event to service user requests
    bus.attach_event(eventP.get(), SD_EVENT_PRIORITY_NORMAL);

    try
    {
        // Create a watchdog object
        Watchdog watchdog(bus, path.c_str(), eventP, std::move(actionTargets),
                          std::move(fallback));

        // Claim the bus
        bus.request_name(service.c_str());

        // Loop until our timer expires and we don't want to continue
        while (continueAfterTimeout || !watchdog.timerExpired())
        {
            // -1 denotes wait for ever
            r = sd_event_run(eventP.get(), (uint64_t)-1);
            if (r < 0)
            {
                log<level::ERR>("Error waiting for events");
                elog<InternalFailure>();
            }
        }
    }
    catch (InternalFailure& e)
    {
        phosphor::logging::commit<InternalFailure>();

        // Need a coredump in the error cases.
        std::terminate();
    }
    return 0;
}
