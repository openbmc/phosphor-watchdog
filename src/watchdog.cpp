#include "watchdog.hpp"

#include <algorithm>
#include <chrono>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <string_view>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace watchdog
{
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace phosphor::logging;

using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

// systemd service to kick start a target.
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

void Watchdog::initService()
{
    sdbusplus::asio::object_server watchdogServer =
        sdbusplus::asio::object_server(conn);
    watchdog = watchdogServer.add_interface(
        objPath.data(), "xyz.openbmc_project.State.Watchdog");

    watchdog->register_method(
        "ResetTimeRemaining",
        [this](bool enableWatchdog) { resetTimeRemaining(enableWatchdog); });
    watchdog->register_property(
        "CurrentTimerUse", convertForMessage(watchdogCurrentTimerUse),
        [this](const std::string& timer, std::string& resp) -> int {
            auto nextTimer = WatchdogBase::convertStringToTimerUse(timer);
            if (!nextTimer || !watchdog->set_property("CurrentTimerUse", timer))
            {
                return 1;
            }
            watchdogCurrentTimerUse = *nextTimer;
            resp = timer;

            return 0;
        });
    watchdog->register_property("Enabled", watchdogEnabled,
                                [this](bool value, bool& resp) {
                                    auto out = enabled(value);
                                    if (!out)
                                    {
                                        return 1;
                                    }
                                    resp = *out;
                                    return 0;
                                });
    watchdog->register_property(
        "ExpireAction", convertForMessage(watchdogExpireAction),
        [this](const std::string& action, std::string& resp) {
            auto nextAction = WatchdogBase::convertStringToAction(action);
            if (!nextAction || !watchdog->set_property("ExpireAction", action))
            {
                return 1;
            }
            watchdogExpireAction = *nextAction;
            resp = action;
            return 0;
        });

    watchdog->register_property(
        "ExpiredTimerUse", convertForMessage(watchdogExpiredTimerUse),
        [this](const std::string& timer, std::string& resp) -> int {
            auto nextTimer = WatchdogBase::convertStringToTimerUse(timer);
            if (!nextTimer || !watchdog->set_property("ExpiredTimerUse", timer))
            {
                return 1;
            }

            watchdogExpiredTimerUse = *nextTimer;
            resp = timer;
            return 0;
        });
    watchdog->register_property(
        "Initialized", watchdogInitialized,
        [this](bool initialized, bool& resp) -> int {
            if (!watchdog->set_property("Initialized", initialized))
            {
                return 1;
            }
            watchdogInitialized = initialized;
            resp = watchdogInitialized;
            return 0;
        });

    watchdog->register_property("Interval", watchdogInterval,
                                [this](uint64_t value, uint64_t& resp) -> int {
                                    auto out = interval(value);
                                    if (!out)
                                    {
                                        return 1;
                                    }
                                    resp = *out;
                                    return 0;
                                });
    watchdog->register_property(
        "TimeRemaining", watchdogTimeRemaining,
        [this](uint64_t value, uint64_t& resp) -> int {
            auto out = timeRemaining(value);
            if (!out)
            {
                return 1;
            }
            resp = *out;
            return 0;
        },
        [this](uint64_t) -> uint64_t { return timeRemaining(); });
    watchdog->register_signal<std::string>(std::string("Timeout"));

    watchdog->initialize();
}

void Watchdog::resetTimeRemaining(bool enableWatchdog)
{
    timeRemaining(watchdogInterval);
    if (enableWatchdog)
    {
        enabled(true);
    }
}

// Enable or disable watchdog
std::optional<bool> Watchdog::enabled(bool value)
{
    if (!value)
    {
        // Make sure we accurately reflect our enabled state to the
        // tryFallbackOrDisable() call
        if (watchdog->set_property("Enabled", watchdogEnabled))
        {
            watchdogEnabled = value;
        }

        // Attempt to fallback or disable our timer if needed
        tryFallbackOrDisable();

        return false;
    }
    else if (!watchdogEnabled)
    {
        auto interval_ms = watchdogInterval;
        asyncTimer.expires_after(std::chrono::milliseconds(interval_ms));
        watchdogTimerEnabled = true;
        log<level::INFO>("watchdog: enabled and started",
                         entry("INTERVAL=%llu", interval_ms));
    }
    if (watchdog->set_property("Enabled", watchdogEnabled))
    {
        watchdogEnabled = value;
        return watchdogEnabled;
    }

    return std::nullopt;
}

// Get the remaining time before timer expires.
// If the timer is disabled, returns 0
uint64_t Watchdog::timeRemaining() const
{
    // timer may have already expired and disabled
    if (!timerEnabled())
    {
        return 0;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(
               asyncTimer.expiry() - std::chrono::steady_clock::now())
        .count();
}

// Reset the timer to a new expiration value
std::optional<uint64_t> Watchdog::timeRemaining(uint64_t value)
{
    if (!timerEnabled())
    {
        // We don't need to update the timer because it is off
        return std::nullopt;
    }

    if (watchdogEnabled)
    {
        // Update interval to minInterval if applicable
        value = std::max(value, minInterval);
    }
    else
    {
        // Having a timer but not displaying an enabled value means we
        // are inside of the fallback
        value = fallback->interval;
    }

    // Update new expiration
    asyncTimer.expires_after(std::chrono::milliseconds(value));
    watchdogTimerEnabled = true;

    // Update Base class data.
    if (watchdog->set_property("TimeRemaining", value))
    {
        watchdogTimeRemaining = value;
        return watchdogTimeRemaining;
    }
    return std::nullopt;
}

// Set value of Interval
std::optional<uint64_t> Watchdog::interval(uint64_t value)
{
    auto nextValue = std::max(value, minInterval);
    if (watchdog->set_property("Interval", nextValue))
    {
        watchdogInterval = nextValue;
        return watchdogInterval;
    }
    return std::nullopt;
}

// Optional callback function on timer expiration
void Watchdog::timeOutHandler(const boost::system::error_code& e)
{
    if (e == boost::asio::error::operation_aborted)
    {
        // Timer was cancelled
        return;
    }

    Action action = watchdogExpireAction;
    if (!watchdogEnabled)
    {
        action = fallback->action;
    }

    if (watchdog->set_property(
            "ExpiredTimerUse",
            WatchdogBase::convertTimerUseToString(watchdogCurrentTimerUse)))
    {
        watchdogExpiredTimerUse = watchdogCurrentTimerUse;
    }

    auto target = actionTargetMap.find(action);
    if (target == actionTargetMap.end())
    {
        log<level::INFO>(
            "watchdog: Timed out with no target",
            entry("ACTION=%s", convertForMessage(action).c_str()),
            entry("TIMER_USE=%s",
                  convertForMessage(watchdogCurrentTimerUse).c_str()));
    }
    else
    {
        log<level::INFO>(
            "watchdog: Timed out",
            entry("ACTION=%s", convertForMessage(action).c_str()),
            entry("TIMER_USE=%s",
                  convertForMessage(watchdogExpiredTimerUse).c_str()),
            entry("TARGET=%s", target->second.c_str()));

        try
        {
            auto signal = conn->new_signal(
                objPath.data(), "xyz.openbmc_project.Watchdog", "Timeout");
            signal.append(convertForMessage(action).c_str());
            signal.signal_send();
        }
        catch (const sdbusplus::exception::exception& e)
        {
            log<level::ERR>("watchdog: failed to send timeout signal",
                            entry("ERROR=%s", e.what()));
        }

        try
        {
            auto method = conn->new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                                SYSTEMD_INTERFACE, "StartUnit");
            method.append(target->second);
            method.append("replace");

            conn->call_noreply(method);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            log<level::ERR>("watchdog: Failed to start unit",
                            entry("TARGET=%s", target->second.c_str()),
                            entry("ERROR=%s", e.what()));
            commit<InternalFailure>();
        }
    }

    tryFallbackOrDisable();
}

void Watchdog::tryFallbackOrDisable()
{
    // We only re-arm the watchdog if we were already enabled and have
    // a possible fallback
    if (fallback && (fallback->always || watchdogEnabled))
    {
        auto interval_ms = fallback->interval;
        asyncTimer.expires_after(std::chrono::milliseconds(interval_ms));
        watchdogTimerEnabled = true;
        log<level::INFO>("watchdog: falling back",
                         entry("INTERVAL=%llu", interval_ms));
    }
    else if (timerEnabled())
    {
        watchdogTimerEnabled = false;
        log<level::INFO>("watchdog: disabled");
    }

    // Make sure we accurately reflect our enabled state to the
    // dbus interface.
    if (watchdog->set_property("Enabled", false))
    {
        watchdogEnabled = false;
    }
}

} // namespace watchdog
} // namespace phosphor
