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

    if(argc <= 1)
    {
        std::cerr << "usage: " << argv[0]
            << " <watchdog object path> "
            << "e.g /xyz/openbmc_project/State/Watchdog/Host" << std::endl;
        return -1;
    }

    objpath = argv[1];

    // generate the service name from objpath
    service = objpath;
    std::replace(service.begin(), service.end(), '/', '.');
    // remove the first '.'
    service.erase(0, 1);

    /** @brief Dbus constructs */
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();

    /** @brief sd_bus object manager */
    sdbusplus::server::manager::manager objManager(bus, objpath.c_str());

    /** @brief Claim the bus */
    bus.request_name(service.c_str());

    phosphor::state::watchdog::Watchdog watchdog(bus, objpath.c_str());

    watchdog.run();

    return 0;
}
