#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <sdbusplus/bus.hpp>
#include "watchdog.hpp"

int main(int argc, char *argv[])
{
    std::string service, objpath;

    if(argc <= 2)
    {
        std::cout << "usage: " << argv[0]
            << " <watchdog object path> <watchdog service name>\n"
            << "e.g: " << argv[0]
            << " /xyz/openbmc_project/state/watchdog/host0"
            << " xyz.openbmc_project.State.Watchdog.Host" << std::endl;
        return -1;
    }
    objpath = argv[1];
    service = argv[2];

    // generate the service name from objpath
    //service = objpath;
    //std::replace(service.begin(), service.end(), '/', '.');
    // remove the first '.'
    //service.erase(0, 1);

    // Dbus constructs
    auto bus = sdbusplus::bus::new_default();

    // sd_bus object manager
    sdbusplus::server::manager::manager objManager(bus, objpath.c_str());

    sd_event* timerEvent = nullptr;

    // acquire a referece to the default event loop
    sd_event_default(&timerEvent);
    // attach bus to this event loop
    bus.attach_event(timerEvent, SD_EVENT_PRIORITY_NORMAL);

    // Claim the bus
    bus.request_name(service.c_str());

    phosphor::state::watchdog::Watchdog watchdog(bus, objpath.c_str());

    // Start event loop for all sd-bus events and timer event
    sd_event_loop(bus.get_event());

    bus.detach_event();
    sd_event_unref(timerEvent);
    return 0;
}
