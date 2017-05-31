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
#include <xyz/openbmc_project/Watchdog/Timer/error.hpp>
#include "argument.hpp"
#include "watchdog.hpp"
#include "elog-errors.hpp"

static void exitWithError(const char* err, char** argv)
{
    phosphor::watchdog::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    using namespace phosphor::logging;
    using InternalError = sdbusplus::xyz::openbmc_project::Watchdog::
                                Timer::Error::InternalError;
    // Read arguments.
    auto options = phosphor::watchdog::ArgumentParser(argc, argv);

    // Parse out path argument.
    auto path = (options)["path"];
    if (path == phosphor::watchdog::ArgumentParser::emptyString)
    {
        exitWithError("Path not specified.", argv);
    }

    // Parse out service name argument
    auto service = (options)["service"];
    if (service == phosphor::watchdog::ArgumentParser::emptyString)
    {
        exitWithError("Service not specified.", argv);
    }

    // Parse out target argument. It is fine if the caller does not
    // pass this if they are not interested in calling into any target
    // on meeting a condition.
    auto target = (options)["target"];

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

        // Wait until the timer has expired
        while(!watchdog.timerExpired())
        {
            // -1 denotes wait for ever
            r = sd_event_run(eventP.get(), (uint64_t)-1);
            if (r < 0)
            {
                elog<InternalError>(
                    phosphor::logging::xyz::openbmc_project::
                        Watchdog::Timer::InternalError::ERROR_DESCRIPTION(
                            "Error waiting for events"));
            }
        }
    }
    catch(InternalError& e)
    {
        phosphor::logging::commit<InternalError>();

        // Need a coredump in the error cases.
        std::terminate();
    }
    return 0;
}
