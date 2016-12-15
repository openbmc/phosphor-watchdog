#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <log.hpp>
#include <sdbusplus/vtable.hpp>
#include "watchdog.hpp"
#include <systemd/sd-bus.h>

namespace phosphor
{
namespace state
{
namespace watchdog
{

using namespace phosphor::logging;

/* return current time in us, of clock @clock_id */
uint64_t Watchdog::now(clockid_t clock_id) const
{
    int r = 0;
    struct timespec ts;

    r = clock_gettime(clock_id, &ts);
    if(r == -1)
    {
        log<level::ERR>("watchdog: clock_gettime() error",
                entry("ERROR=%s", strerror(errno)));
        return 0;
    }

    // convert to microsecond
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

bool Watchdog::enabled(bool value)
{
    int r = 0;

    if(detail::ServerObject<detail::WatchdogIface>::enabled() != value)
    {
        if(value)
        {
            // start the one-shot timer
            r = setTimer(interval() * 1000);
            if(r < 0)
                return false;

            r = enableTimer();
            if(r < 0)
                return false;

            log<level::INFO>("watchdog: enabled and started",
                    entry("INTERVAL=%llu", interval()));
        }
        else
        {
            r = disableTimer();
            if(r < 0)
                return true;

            log<level::INFO>("watchdog: disabled");
        }
    }
    return detail::ServerObject<detail::WatchdogIface>::enabled(value);
}

/* get the remaining time before timer expire.
 * if the timer is disabled, returns 0 */
uint64_t Watchdog::timeRemaining() const
{
    uint64_t time_remain = 0;
    int enabled = 0;
    int r = 0;

    if(detail::ServerObject<detail::WatchdogIface>::enabled())
    {
        // timer may already expire and be disabled
        r = sd_event_source_get_enabled(timer_event_source, &enabled);
        if(r < 0)
        {
            log<level::ERR>(
                "watchdog timeRemaining: sd_event_source_get_enabled() error",
                entry("ERROR=%s", strerror(-r)));
            return 0;
        }

        if(enabled == SD_EVENT_OFF)
        {
            time_remain = 0;
        }
        else
        {
            uint64_t next = 0;

            /* get the configured expire time */
            sd_event_source_get_time(timer_event_source, &next);
            if(r < 0)
            {
                log<level::ERR>(
                    "watchdog timeRemaining: sd_event_source_get_time() error",
                    entry("ERROR=%s", strerror(-r)));
                return 0;
            }
            // convert to ms
            time_remain = (next - now(CLOCK_MONOTONIC)) / 1000;
        }
    }

    return time_remain;
}

/* reset the timer to @value if the timer is enabled */
uint64_t Watchdog::timeRemaining(uint64_t value)
{
    int r = 0;

    if(detail::ServerObject<detail::WatchdogIface>::enabled())
    {
        r = disableTimer();
        if(r < 0)
            return 0;

        r = setTimer(value * 1000);
        if(r < 0)
            return 0;

        r = enableTimer();
        if(r < 0)
            return 0;

        log<level::INFO>("watchdog: reset timer",
                entry("VALUE=%llu", value));

        // Invoke property_changed("timeRemaining").
        return detail::ServerObject<detail::WatchdogIface>::timeRemaining(value);
    }

    return 0;
}

/* the static watchdog timout handler */
int Watchdog::timeoutHandler(sd_event_source *es, uint64_t usec, void *userdata)
{
    auto wdg = static_cast<Watchdog*>(userdata);

    // invoke dbus signal
    log<level::ALERT>("watchdog: time out");
    wdg->timeout();

    return 0;
}

int Watchdog::disableTimer()
{
    int r = 0;

    r = sd_event_source_set_enabled(timer_event_source, SD_EVENT_OFF);
    if(r < 0)
        log<level::ERR>("watchdog disableTimer: sd_event_source_set_enabled() error",
                entry("ERROR=%s", strerror(-r)));
    return r;
}

int Watchdog::enableTimer()
{
    int r = 0;

    r = sd_event_source_set_enabled(timer_event_source, SD_EVENT_ONESHOT);
    if(r < 0)
        log<level::ERR>("watchdog enableTimer: sd_event_source_set_enabled() error",
                entry("ERROR=%s", strerror(-r)));
    return r;
}

int Watchdog::setTimer(uint64_t value)
{
    int r = 0;
    uint64_t next = 0;

    next = now(CLOCK_MONOTONIC) + value;

    /* set next expire time */
    r = sd_event_source_set_time(timer_event_source, next);
    if(r < 0)
        log<level::ERR>("watchdog setTimer: sd_event_source_set_set_time() error",
                entry("ERROR=%s", strerror(-r)));

    return r;
}

int Watchdog::initTimer()
{
    int r = 0;

    // acquire a referece to the default event loop
    r = sd_event_default(&timer_event);
    if(r < 0)
    {
        log<level::ERR>("watchdog initTimer: sd_event_default() error",
                entry("ERROR=%s", strerror(-r)));
        return r;
    }

    // attach bus to this event loop
    r = bus.attach_event(timer_event, SD_EVENT_PRIORITY_NORMAL);
    if(r < 0)
    {
        log<level::ERR>("watchdog initTimer: bus.attach_event() error",
                entry("ERROR=%s", strerror(-r)));
        return r;
    }

    r = sd_event_add_time(timer_event, &timer_event_source,
            CLOCK_MONOTONIC,
            UINT_MAX, /* expire time - never */
            0, /* use default event accuracy that the event may be delayed */
            timeoutHandler, this);
    if(r < 0)
    {
        log<level::ERR>("watchdog initTimer: sd_event_add_time() error",
                entry("ERROR=%s", strerror(-r)));
        return r;
    }

    // disable for now
    r = disableTimer();
    if(r >= 0)
        log<level::INFO>("watchdog ready");

    return r;
}

int Watchdog::run()
{
    int r = 0;

    r = initTimer();
    if(r < 0)
       return r;

    r = sd_event_loop(timer_event);
    return r;
}

} // namespace watchdog
} // namespace state
} // namepsace phosphor
