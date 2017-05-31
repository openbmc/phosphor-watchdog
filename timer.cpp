#include <chrono>
#include <systemd/sd-event.h>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Watchdog/Timer/error.hpp>
#include "timer.hpp"
#include "elog-errors.hpp"
#include <iostream>
namespace phosphor
{
namespace watchdog
{

// For throwing exception
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Watchdog::Timer::Error;

// Initializes the timer object
void Timer::initialize()
{
    // This can not be called more than once.
    if (eventSource.get())
    {
        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION("Timer already initialized"));
    }

    // Add infinite expiration time
    decltype(eventSource.get()) sourcePtr = nullptr;
    auto r = sd_event_add_time(event.get(),
                               &sourcePtr,
                               CLOCK_MONOTONIC, // Time base
                               UINT64_MAX,      // Expire time - way long time
                               0,               // Use default event accuracy
                               timeoutHandler,  // Callback handler on timeout
                               this);           // User data
    eventSource.reset(sourcePtr);

    if (r < 0)
    {
        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION(
                    "Timer initialization failed"));
    }

    // Disable the timer for now
    setTimer<std::false_type>();
}

// callback handler on timeout
int Timer::timeoutHandler(sd_event_source* eventSource,
                          uint64_t usec, void* userData)
{
    using namespace phosphor::logging;

    log<level::INFO>("Timer Expired");

    auto timer = static_cast<Timer*>(userData);
    timer->expire = true;

    // Call an optional callback function
    if(timer->userCallBack)
    {
        timer->userCallBack();
    }
    return 0;
}

// Gets the time from steady_clock
std::chrono::microseconds Timer::getCurrentTime()
{
    using namespace std::chrono;
    auto usec = steady_clock::now().time_since_epoch();
    return duration_cast<microseconds>(usec);
}

// Sets the expiration time and arms the timer
void Timer::startTimer(std::chrono::microseconds usec)
{
    using namespace std::chrono;

    // Get the current MONOTONIC time and add the delta
    auto expireTime = getCurrentTime() + usec;

    // Set the time
    auto r = sd_event_source_set_time(eventSource.get(),
                                      expireTime.count());
    if (r < 0)
    {
        std::string error = "Error setting the expiration time to: " +
            std::to_string(duration_cast<milliseconds>(usec).count()) +
            " milliseconds";

        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION(error.c_str()));
    }
}

// Returns current timer enablement type
int Timer::getTimer() const
{
    int enabled{};
    auto r = sd_event_source_get_enabled(eventSource.get(), &enabled);
    if (r < 0)
    {
        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION(
                    "Error geting current timer type enablement state"));
    }
    return enabled;
}

// Enables / disables the timer
void Timer::setTimer(int type)
{
    auto r = sd_event_source_set_enabled(eventSource.get(), type);
    if (r < 0)
    {
        std::string error = "Error setting the timer type to: " +
                                std::to_string(type);
        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION(error.c_str()));
    }
}

// Returns time remaining before expiration
std::chrono::microseconds Timer::getRemainingTime() const
{
    uint64_t next = 0;
    auto r = sd_event_source_get_time(eventSource.get(), &next);
    if (r < 0)
    {
        elog<InternalError>(
            phosphor::logging::xyz::openbmc_project::Watchdog::Timer::
                InternalError::ERROR_DESCRIPTION(
                    "Error fetching remaining time to expire"));
    }
    return std::chrono::microseconds(next);
}

} // namespace watchdog
} // namespace phosphor
