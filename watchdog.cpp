#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <phosphor-logging/log.hpp>
#include <systemd/sd-bus.h>
#include <sdbusplus/vtable.hpp>
#include "watchdog.hpp"

namespace phosphor
{
namespace state
{
namespace watchdog
{
using namespace std::chrono;
using namespace phosphor::logging;

/* Return current time in usec, using CLOCK_MONOTONIC.
 * Watchdog uses CLOCK_MONOTONIC for sd-event timer. */
uint64_t Watchdog::now() const
{
    struct timespec ts{};
    auto r = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (r == -1)
    {
        log<level::ERR>("watchdog: clock_gettime() error",
                entry("ERROR=%s", strerror(errno)));
        return 0;
    }

    // convert to microsecond
    auto currTime = duration_cast<microseconds>
                    (seconds(ts.tv_sec) + nanoseconds(ts.tv_nsec));

    return currTime.count();
}

bool Watchdog::enabled(bool value)
{
    int r = 0;

    if (WatchdogInherits::enabled() != value)
    {
        if (value)
        {
            // start the one-shot timer
            r = setTimer(interval());
            if (r < 0)
            {
                return false;
            }

            r = setTimerEnabled<std::true_type>();
            if (r < 0)
            {
                return false;
            }

            log<level::INFO>("watchdog: enabled and started",
                             entry("INTERVAL=%llu", interval()));
        }
        else
        {
            r = setTimerEnabled<std::false_type>();
            if (r < 0)
            {
                return true;
            }

            log<level::INFO>("watchdog: disabled");
        }
    }
    return WatchdogInherits::enabled(value);
}

/* get the remaining time before timer expires.
 * if the timer is disabled, returns 0 */
uint64_t Watchdog::timeRemaining() const
{
    uint64_t timeRemain = 0;
    int enabled = 0;

    if (WatchdogInherits::enabled())
    {
        // timer may already expire and disabled
        auto r = sd_event_source_get_enabled(timerEventSource, &enabled);
        if (r < 0)
        {
            log<level::ERR>(
                "watchdog timeRemaining: sd_event_source_get_enabled() error",
                entry("ERROR=%s", strerror(-r)));
            return 0;
        }

        if (enabled != SD_EVENT_OFF)
        {
            // the one-shot timer does not expire yet
            uint64_t next = 0;

            /* get the configured expire time, in usec */
            r = sd_event_source_get_time(timerEventSource, &next);
            if (r < 0)
            {
                log<level::ERR>(
                    "watchdog timeRemaining: sd_event_source_get_time() error",
                    entry("ERROR=%s", strerror(-r)));
                return 0;
            }
            // convert to msec
            auto nextMsec = duration_cast<milliseconds>(microseconds(next));
            auto nowMsec = duration_cast<milliseconds>(microseconds(now()));
            timeRemain = (nextMsec > nowMsec) ?
                            (nextMsec - nowMsec).count() : 0;
        }
    }

    return timeRemain;
}

/* reset the timer to @value if the timer is enabled */
uint64_t Watchdog::timeRemaining(uint64_t value)
{
    if (WatchdogInherits::enabled())
    {
        auto r = setTimerEnabled<std::false_type>();
        if (r < 0)
        {
            return 0;
        }

        r = setTimer(value);
        if (r < 0)
        {
            return 0;
        }

        r = setTimerEnabled<std::true_type>();
        if (r < 0)
        {
            return 0;
        }

        log<level::INFO>("watchdog: reset timer",
                         entry("VALUE=%llu", value));

        // Invoke property_changed("timeRemaining")
        return WatchdogInherits::timeRemaining(value);
    }

    return 0;
}

/* the static watchdog timout handler */
int Watchdog::timeoutHandler(sd_event_source* es, uint64_t usec, void* userData)
{
    auto wdg = static_cast<Watchdog*>(userData);

    // invoke dbus signal
    log<level::ALERT>("watchdog: time out");
    wdg->timeout();

    return 0;
}

// @value in msec
int Watchdog::setTimer(uint64_t value)
{
    // convert to usec
    auto nextUsec = duration_cast<microseconds>(milliseconds(value));
    auto next = now() + nextUsec.count();

    /* set next expire time */
    auto r = sd_event_source_set_time(timerEventSource, next);
    if (r < 0)
    {
        log<level::ERR>
            ("watchdog setTimer: sd_event_source_set_set_time() error",
            entry("ERROR=%s", strerror(-r)));
    }

    return r;
}

} // namespace watchdog
} // namespace state
} // namepsace phosphor
