#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/server/object.hpp>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <xyz/openbmc_project/State/Watchdog/server.hpp>

namespace phosphor
{
namespace watchdog
{

constexpr auto DEFAULT_MIN_INTERVAL_MS = 0;
using WatchdogBase = sdbusplus::xyz::openbmc_project::State::server::Watchdog;

/** @class Watchdog
 *  @brief OpenBMC watchdog implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.State.Watchdog DBus API.
 */
class Watchdog
{
  public:
    Watchdog() = delete;
    ~Watchdog() = default;
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /** @brief Type used to hold the name of a Watchdog Action.
     */
    using Action = WatchdogBase::Action;
    /** @brief Type used to hold the name of a Watchdog Timer.
     */
    using TimerUse = WatchdogBase::TimerUse;

    /** @brief Type used to hold the name of a systemd target.
     */
    using TargetName = std::string;

    /** @brief Type used to store the mapping of a Watchdog timeout
     *         action to a systemd target.
     */
    using ActionTargetMap = std::unordered_map<Action, TargetName>;

    /** @brief Type used to specify the parameters of a fallback watchdog
     */
    struct Fallback
    {
        Action action;
        uint64_t interval;
        bool always;
    };

    /** @brief Constructs the Watchdog object
     *
     *  @param[in] io              - Async IO Service
     *  @param[in] conn            - Dbus connection for Async IO.
     *  @param[in] objPath         - Object path to attach to.
     *  @param[in] event           - reference to sdeventplus::Event loop
     *  @param[in] actionTargets   - map of systemd targets called on timeout
     *  @param[in] fallback        - fallback watchdog
     *  @param[in] minInterval     - minimum intervale value allowed
     *  @param[in] defaultInterval - default interval to start with
     */
    Watchdog(boost::asio::io_service& io,
             const std::shared_ptr<sdbusplus::asio::connection>& conn,
             const char* objPath,
             Watchdog::ActionTargetMap&& actionTargetMap = {},
             std::optional<Fallback>&& fallback = std::nullopt,
             uint64_t minInterval = DEFAULT_MIN_INTERVAL_MS,
             uint64_t defaultInterval = 0) :
        conn(conn),
        actionTargetMap(std::move(actionTargetMap)), fallback(fallback),
        minInterval(minInterval), asyncTimer(io), objPath(objPath)
    {
        // Setup all Watchdog State properties.
        initService();

        // Use default if passed in otherwise just use default that comes
        // with object
        if (defaultInterval)
        {
            interval(defaultInterval);
        }
        else
        {
            interval(watchdogInterval);
        }

        // Start Watchdog Timer
        asyncTimer.expires_after(std::chrono::seconds(watchdogInterval));
        asyncTimer.async_wait([this](const auto& e) { timeOutHandler(e); });

        // We need to poke the enable mechanism to make sure that the timer
        // enters the fallback state if the fallback is always enabled.
        tryFallbackOrDisable();
    }

    /** @brief Resets the TimeRemaining to the configured Interval
     *         Optionally enables the watchdog.
     *
     *  @param[in] enableWatchdog - Should the call enable the watchdog
     */
    void resetTimeRemaining(bool enableWatchdog);

    /** @brief Enable or disable watchdog
     *         If a watchdog state is changed from disable to enable,
     *         the watchdog timer is set with the default expiration
     *         interval and it starts counting down.
     *         If a watchdog is already enabled, setting @value to true
     *         has no effect.
     *
     *  @param[in] value - 'true' to enable. 'false' to disable
     *
     *  @return : applied value if success, std::nullopt otherwise
     */
    std::optional<bool> enabled(bool value);

    /** @brief Gets the remaining time before watchdog expires.
     *
     *  @return 0 if watchdog is expired.
     *          Remaining time in milliseconds otherwise.
     */
    uint64_t timeRemaining() const;

    /** @brief Reset timer to expire after new timeout in milliseconds.
     *
     *  @param[in] value - the time in milliseconds after which
     *                     the watchdog will expire
     *
     *  @return: updated timeout value if watchdog is enabled.
     *           std::nullopt otherwise.
     */
    std::optional<uint64_t> timeRemaining(uint64_t value);

    /** @brief Set value of Interval
     *
     *  @param[in] value - interval time to set
     *
     *  @return: interval that was set if success, std::nullopt otherwise.
     *
     */
    std::optional<uint64_t> interval(uint64_t value);

    /** @brief Tells if the referenced timer is expired or not */
    inline auto timerExpired() const
    {
        return watchdogTimerEnabled;
    }

    /** @brief Tells if the timer is running or not */
    inline bool timerEnabled() const
    {
        auto expireTime = asyncTimer.expiry();
        return expireTime < std::chrono::steady_clock::now();
    }

  private:
    void initService();

    /** @brief Dbus connection for Async IO */
    std::shared_ptr<sdbusplus::asio::connection> conn;

    /** @brief Map of systemd units to be started when the timer expires */
    ActionTargetMap actionTargetMap;

    /** @brief Fallback timer options */
    std::optional<Fallback> fallback;

    /** @brief Minimum watchdog interval value */
    uint64_t minInterval;

    /** @brief Contained timer object */
    boost::asio::steady_timer asyncTimer;

    /** @brief Optional Callback handler on timer expiration */
    void timeOutHandler(const boost::system::error_code& e);

    /** @brief Attempt to enter the fallback watchdog or disables it */
    void tryFallbackOrDisable();

    /** @brief Object path of the watchdog */

    std::string_view objPath;

    /** @brief Watchdog State Properties */
    std::shared_ptr<sdbusplus::asio::dbus_interface> watchdog;

    /** @brief Watchdog Reset Interval */
    uint64_t watchdogInterval;

    /** @brief Watchdog Time Remaining */
    uint64_t watchdogTimeRemaining;

    /** @brief Watchdog Initialization State */
    bool watchdogInitialized;

    /** @brief Watchdog Enabled State */
    bool watchdogEnabled;

    /** @brief Watchdog Timer Enabled State */
    bool watchdogTimerEnabled;

    /** @brief Watchdog Expire Action */
    Action watchdogExpireAction;

    /** @brief Watchdog Current Timer */
    TimerUse watchdogCurrentTimerUse;

    /** @brief Watchdog Expire Timer */
    TimerUse watchdogExpiredTimerUse;
};

} // namespace watchdog
} // namespace phosphor
