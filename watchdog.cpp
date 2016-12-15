#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <sdbusplus/vtable.hpp>
#include <systemd/sd-bus.h>
#include <signal.h>
#include <time.h>
#include "watchdog.hpp"

namespace phosphor
{
namespace state
{
namespace watchdog
{

void Watchdog::timeoutHandler(int signum, siginfo_t *si, void *uc)
{
    // get the object address
    auto *obj = static_cast<Watchdog *>(si->si_value.sival_ptr);
    // invoke dbus signal
    std::cerr << obj->objpath << ": time out" << std::endl;
    obj->timeout();
}

int Watchdog::setTimer(uint64_t value)
{
    struct itimerspec its;
    uint64_t it_val = value;
    int ret;

    its.it_value.tv_sec = it_val / 1000;
    its.it_value.tv_nsec = it_val % 1000 * 1000000;
    // one-shot timer
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    ret = timer_settime(timerid, 0, &its, NULL);
    if(ret == -1)
        std::cerr << objpath << ": timer_settime() error"<< std::endl;

    std::cout << objpath << ": timeout after " << value << " ms." << std::endl;
    return ret;
}

bool Watchdog::enabled(bool value)
{
    if(detail::ServerObject<detail::WatchdogIface>::enabled() != value)
    {
        if(value == true)
        {
            // Start the one-shot timer
            setTimer(interval());
        }
        else if(value == false)
        {
            // Disarm the timer
            setTimer(0);
        }
    }
    return detail::ServerObject<detail::WatchdogIface>::enabled(value);
}

uint64_t Watchdog::timeRemaining() const
{
    struct itimerspec its;
    uint64_t time_remain;
    int ret = 0;

    ret = timer_gettime(timerid, &its);
    if(ret == -1)
        std::cerr << objpath << ": timer_gettime() error"<< std::endl;

    time_remain = its.it_value.tv_sec * 1000 + its.it_value.tv_nsec / 1000000;

    // FIXME: need to update _timeRemaining in base class? But invoking timeRemaining(time_remain)
    // will recursively call Watchdog::timeRemaining()
    //return detail::ServerObject<detail::WatchdogIface>::timeRemaining(time_remain);
    return time_remain;
}

uint64_t Watchdog::timeRemaining(uint64_t value)
{
    int ret = 0;

    if(detail::ServerObject<detail::WatchdogIface>::enabled() == true)
        ret = setTimer(value);
    if (ret != -1)
    {
        // update _timeRemaining in base clase. Invoke property_changed("timeRemaining").
        return detail::ServerObject<detail::WatchdogIface>::timeRemaining(value);
    }

    return 0;
}

Watchdog::Watchdog(sdbusplus::bus::bus &&bus,
                 const char* busname,
                 const char* obj) :
    detail::ServerObject<detail::WatchdogIface>(bus, obj),
    _bus(std::move(bus)),
    _manager(sdbusplus::server::manager::manager(_bus, obj))
{
    _bus.request_name(busname);
    objpath = obj;
}

void Watchdog::run() noexcept
{
    struct sigevent sev;
    struct sigaction sa;
    int ret;

    // Establish handler for timer signal
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = timeoutHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    ret = sigaction(SIGRTMIN, &sa, NULL);
    if(ret == -1)
    {
        std::cerr << objpath << ": sigaction() error" << std::endl;
        return;
    }

    // Create the timer
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    // Pass address of the object to signal handle
    sev.sigev_value.sival_ptr = this;
    ret = timer_create(CLOCK_REALTIME, &sev, &timerid);
    if(ret == -1)
    {
        std::cerr << objpath << ": timer_create() error" << std::endl;
        return;
    }

    while(true)
    {
        try
        {
            _bus.process_discard();
            _bus.wait();
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

} // namespace watchdog
} // namespace state
} // namepsace phosphor
