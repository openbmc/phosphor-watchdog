#include <chrono>
#include <phosphor-logging/log.hpp>
#include "watchdog.hpp"
namespace phosphor
{
namespace watchdog
{
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace phosphor::logging;

// systemd service to kick start a target.
constexpr auto SYSTEMD_SERVICE    = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT       = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE  = "org.freedesktop.systemd1.Manager";

// Enable or disable watchdog
bool Watchdog::enabled(bool value)
{
    if (!value && fallback)
    {
        auto interval_ms = std::get<uint64_t>(fallback.value());
        auto interval_us = duration_cast<microseconds>(milliseconds(interval_ms));

        timer.clearExpired();
        timer.start(interval_us);
        timer.setEnabled<std::true_type>();
        // NOTE: We don't actually want to report value = true here
        // just in case the host tries repeatedly to disable this fallback
        // timer.

        log<level::INFO>("watchdog: falling back",
                entry("INTERVAL=%llu", interval_ms));
    }
    else if (this->enabled() != value)
    {
        if (value)
        {
            // Start ONESHOT timer. Timer handles all in usec
            auto usec = duration_cast<microseconds>(
                    milliseconds(this->interval()));
            // Update new expiration
            timer.start(usec);

            // Enable timer
            timer.setEnabled<std::true_type>();

            log<level::INFO>("watchdog: enabled and started",
                    entry("INTERVAL=%llu", this->interval()));
        }
        else
        {
            timer.setEnabled<std::false_type>();
            timer.clearExpired();
            log<level::INFO>("watchdog: disabled");
        }
    }
    return WatchdogInherits::enabled(value);
}

// Get the remaining time before timer expires.
// If the timer is disabled, returns 0
uint64_t Watchdog::timeRemaining() const
{
    uint64_t timeRemain = 0;

    // timer may have already expired and disabled
    if (timer.getEnabled() != SD_EVENT_OFF)
    {
        // the one-shot timer does not expire yet
        auto expiry = duration_cast<milliseconds>(
                timer.getRemaining());

        // convert to msec per interface expectation.
        auto timeNow = duration_cast<milliseconds>(
                Timer::getCurrentTime());

        // Its possible that timer may have expired by now.
        // So need to cross verify.
        timeRemain = (expiry > timeNow) ?
            (expiry - timeNow).count() : 0;
    }
    return timeRemain;
}

// Reset the timer to a new expiration value
uint64_t Watchdog::timeRemaining(uint64_t value)
{
    if (this->enabled())
    {
        // Disable the timer
        timer.setEnabled<std::false_type>();

        // Timer handles all in microseconds and hence converting
        auto usec = duration_cast<microseconds>(
                                  milliseconds(value));
        // Update new expiration
        timer.start(usec);

        // Enable the timer.
        timer.setEnabled<std::true_type>();

        // Update Base class data.
        return WatchdogInherits::timeRemaining(value);
    }
    return 0;
}

// Optional callback function on timer expiration
void Watchdog::timeOutHandler()
{
    Action action;
    if (this->enabled())
    {
        action = expireAction();
    }
    else if (fallback)
    {
        action = std::get<Action>(fallback.value());
    }
    else
    {
        return;
    }

    auto target = actionTargets.find(action);
    if (target == actionTargets.end())
    {
        log<level::INFO>("watchdog: Timed out with no target",
                entry("ACTION=%s", convertForMessage(action).c_str()));
        return;
    }

    auto method = bus.new_method_call(SYSTEMD_SERVICE,
            SYSTEMD_ROOT,
            SYSTEMD_INTERFACE,
            "StartUnit");
    method.append(target->second);
    method.append("replace");

    log<level::INFO>("watchdog: Timed out",
            entry("ACTION=%s", convertForMessage(action).c_str()),
            entry("TARGET=%s", target->second.c_str()));
    bus.call_noreply(method);
}

} // namespace watchdog
} // namepsace phosphor
