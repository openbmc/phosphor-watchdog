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
    if (WatchdogInherits::enabled() != value)
    {
        if (value)
        {
            // Start ONESHOT timer. Timer handles all in usec
            auto usec = duration_cast<microseconds>(
                                      milliseconds(this->interval()));
            // Update new expiration
            timer.startTimer(usec);

            // Enable timer
            timer.setTimer<std::true_type>();

            log<level::INFO>("watchdog: enabled and started",
                             entry("INTERVAL=%llu", this->interval()));
        }
        else
        {
            timer.setTimer<std::false_type>();
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

    if (WatchdogInherits::enabled())
    {
        // timer may have already expired and disabled
        if (timer.getTimer() != SD_EVENT_OFF)
        {
            // the one-shot timer does not expire yet
            auto expiry = duration_cast<milliseconds>(
                                   timer.getRemainingTime());

            // convert to msec per interface expectation.
            auto timeNow = duration_cast<milliseconds>(
                                    Timer::getCurrentTime());

            // Its possible that timer may have expired by now.
            // So need to cross verify.
            timeRemain = (expiry > timeNow) ?
                            (expiry - timeNow).count() : 0;
        }
    }
    return timeRemain;
}

// Reset the timer to a new expiration value
uint64_t Watchdog::timeRemaining(uint64_t value)
{
    if (WatchdogInherits::enabled())
    {
        // Disable the timer
        timer.setTimer<std::false_type>();

        // Timer handles all in microseconds and hence converting
        auto usec = duration_cast<microseconds>(
                                  milliseconds(value));
        // Update new expiration
        timer.startTimer(usec);

        // Enable the timer.
        timer.setTimer<std::true_type>();

        log<level::INFO>("watchdog: reset timer",
                         entry("VALUE=%llu", value));

        // Update Base class data.
        return WatchdogInherits::timeRemaining(value);
    }
    return 0;
}

// Secondary callback function on timer expiration
void Watchdog::timeOutHandler()
{
    if (!target.empty())
    {
        auto method = bus.new_method_call(SYSTEMD_SERVICE,
                                          SYSTEMD_ROOT,
                                          SYSTEMD_INTERFACE,
                                          "StartUnit");
        method.append(target);
        method.append("replace");

        bus.call_noreply(method);
    }
    return;
}

} // namespace watchdog
} // namepsace phosphor
