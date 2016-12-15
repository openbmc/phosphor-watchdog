#include <cstdlib>
#include <iostream>
#include <exception>
#include <string>
#include "config.h"
#include <sdbusplus/bus.hpp>
#include "watchdog.hpp"

int main(int argc, char *argv[])
{
    std::string busname, objpath;

    if(argc <= 1)
    {
        std::cerr << "usage: " << argv[0] <<
            " $watchdog_type (e.g. host)" << std::endl;
        return -1;
    }

    if(std::string(argv[1]) ==  std::string("host"))
    {
        // Watchdog to monitor host booting
        busname = BUSNAME_HOST_WATCHDOG;
        objpath = OBJ_HOST_WATCHDOG;
    }
    else
    {
        std::cerr << argv[0] <<
            " supported watchdog type: host " << std::endl;
        return -1;
    }

    try{
        phosphor::state::watchdog::Watchdog watchdog(
                sdbusplus::bus::new_system(),
                busname.c_str(),
                objpath.c_str());
	    watchdog.run();
        exit(EXIT_SUCCESS);
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    exit(EXIT_FAILURE);
}
