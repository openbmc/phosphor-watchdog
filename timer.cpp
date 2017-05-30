#include <chrono>
#include <systemd/sd-event.h>
#include <phosphor-logging/log.hpp>
#include "timer.hpp"
namespace phosphor
{
namespace watchdog
{

// Initializes the timer object
void Timer::initialize()
{
    // This can not be called more than once.
    if (eventSource.get())
    {
        // TODO: Need to throw elog exception stating its already added.
        throw std::runtime_error("Timer already initialized");
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
        // TODO: throw elog exception
        throw std::runtime_error("Timer initialization failed");
    }

    // Disable the timer for now
    setEnabled<std::false_type>();
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
void Timer::start(std::chrono::microseconds usec)
{
    // Get the current MONOTONIC time and add the delta
    auto expireTime = getCurrentTime() + usec;

    // Set the time
    auto r = sd_event_source_set_time(eventSource.get(),
                                      expireTime.count());
    if (r < 0)
    {
        // TODO throw elog exception
        throw std::runtime_error("Error setting the expiration time");
    }
}

// Returns current timer enablement type
int Timer::getEnabled() const
{
    int enabled{};
    auto r = sd_event_source_get_enabled(eventSource.get(), &enabled);
    if (r < 0)
    {
        // TODO: Need to throw elog exception
        throw std::runtime_error("Error geting current time enablement state");
    }
    return enabled;
}

// Enables / disables the timer
void Timer::setEnabled(int type)
{
    auto r = sd_event_source_set_enabled(eventSource.get(), type);
    if (r < 0)
    {
        // TODO: Need to throw elog exception
        throw std::runtime_error("Error altering enabled property");
    }
}

// Returns time remaining before expiration
std::chrono::microseconds Timer::getRemaining() const
{
    uint64_t next = 0;
    auto r = sd_event_source_get_time(eventSource.get(), &next);
    if (r < 0)
    {
        // TODO: Need to throw elog exception
        throw std::runtime_error("Error altering enabled property");
    }
    return std::chrono::microseconds(next);
}

} // namespace watchdog
} // namespace phosphor
