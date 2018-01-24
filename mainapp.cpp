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

#include <iostream>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include "argument.hpp"
#include "watchdog.hpp"

static void exitWithError(const char* err, char** argv)
{
    phosphor::watchdog::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    using namespace phosphor::logging;
    using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                                Error::InternalFailure;
    // Read arguments.
    auto options = phosphor::watchdog::ArgumentParser(argc, argv);

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
    auto target = targetParam.back();

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
        phosphor::watchdog::Watchdog watchdog(bus, path.c_str(),
                                              eventP, std::move(target));
        // Claim the bus
        bus.request_name(service.c_str());

        // Loop forever processing events
        while (true)
        {
            // -1 denotes wait for ever
            r = sd_event_run(eventP.get(), (uint64_t)-1);
            if (r < 0)
            {
                log<level::ERR>("Error waiting for events");
                elog<InternalFailure>();
            }

            // The timer expiring is an event that breaks from the above.
            if (watchdog.timerExpired())
            {
                // Either disable the timer or exit.
                if (continueAfterTimeout)
                {
                    // The watchdog will be disabled but left running to be
                    // re-enabled.
                    watchdog.enabled(false);
                }
                else
                {
                    // The watchdog daemon will now exit.
                    break;
                }
            }
        }
    }
    catch(InternalFailure& e)
    {
        phosphor::logging::commit<InternalFailure>();

        // Need a coredump in the error cases.
        std::terminate();
    }
    return 0;
}
