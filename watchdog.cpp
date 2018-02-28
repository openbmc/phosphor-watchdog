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
    if (!value)
    {
        // Attempt to disable our timer if needed
        tryDisable();

        return false;
    }
    else if (!this->enabled())
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

    return WatchdogInherits::enabled(value);
}

// Get the remaining time before timer expires.
// If the timer is disabled, returns 0
uint64_t Watchdog::timeRemaining() const
{
    uint64_t timeRemain = 0;

    // timer may have already expired and disabled
    if (timerEnabled())
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
    if (!timerEnabled())
    {
        // We don't need to update the timer because it is off
        return 0;
    }

    // Update new expiration
    auto usec = duration_cast<microseconds>(milliseconds(value));
    timer.start(usec);

    // Update Base class data.
    return WatchdogInherits::timeRemaining(value);
}

// Optional callback function on timer expiration
void Watchdog::timeOutHandler()
{
    auto action = expireAction();
    auto target = actionTargets.find(action);

    if (target == actionTargets.end())
    {
        log<level::INFO>("watchdog: Timed out with no target",
                entry("ACTION=%s", convertForMessage(action).c_str()));
    }
    else
    {
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

    tryDisable();
}

void Watchdog::tryDisable()
{
    if (timerEnabled() || timer.expired())
    {
        timer.setEnabled<std::false_type>();
        timer.clearExpired();

        log<level::INFO>("watchdog: disabled");
    }

    // Make sure we accurately reflect our enabled state to the
    // dbus interface.
    WatchdogInherits::enabled(false);
}

} // namespace watchdog
} // namepsace phosphor
