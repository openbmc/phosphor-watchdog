#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/source/time.hpp>
#include <unordered_map>
#include <utility>
#include <xyz/openbmc_project/State/Watchdog/server.hpp>

namespace phosphor
{
namespace watchdog
{

namespace Base = sdbusplus::xyz::openbmc_project::State::server;
using WatchdogInherits = sdbusplus::server::object::object<Base::Watchdog>;

/** @class Watchdog
 *  @brief OpenBMC watchdog implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.State.Watchdog DBus API.
 */
class Watchdog : public WatchdogInherits
{
  public:
    Watchdog() = delete;
    ~Watchdog() = default;
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

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
     *  @param[in] bus            - DBus bus to attach to.
     *  @param[in] objPath        - Object path to attach to.
     *  @param[in] event          - reference to sdeventplus::Event loop
     *  @param[in] actionTargets  - map of systemd targets called on timeout
     *  @param[in] fallback
     */
    Watchdog(sdbusplus::bus::bus& bus, const char* objPath,
             const sdeventplus::Event& event,
             ActionTargetMap&& actionTargetMap = {},
             std::optional<Fallback>&& fallback = std::nullopt) :
        WatchdogInherits(bus, objPath),
        bus(bus), actionTargetMap(std::move(actionTargetMap)),
        fallback(std::move(fallback)), expired(false), clock(event),
        timer(event, clock.now(), std::chrono::milliseconds(1),
              std::bind(&Watchdog::timeOutHandler, this, std::placeholders::_1,
                        std::placeholders::_2))
    {
        // We need to poke the enable mechanism to make sure that the timer
        // enters the fallback state if the fallback is always enabled.
        tryFallbackOrDisable();
    }

    /** @brief Resets the TimeRemaining to the configured Interval
     *         Optionally enables the watchdog.
     *
     *  @param[in] enableWatchdog - Should the call enable the watchdog
     */
    void resetTimeRemaining(bool enableWatchdog) override;

    /** @brief Since we are overriding the setter-enabled but not the
     *         getter-enabled, we need to have this using in order to
     *         allow passthrough usage of the getter-enabled.
     */
    using Base::Watchdog::enabled;

    /** @brief Enable or disable watchdog
     *         If a watchdog state is changed from disable to enable,
     *         the watchdog timer is set with the default expiration
     *         interval and it starts counting down.
     *         If a watchdog is already enabled, setting @value to true
     *         has no effect.
     *
     *  @param[in] value - 'true' to enable. 'false' to disable
     *
     *  @return : applied value if success, previous value otherwise
     */
    bool enabled(bool value) override;

    /** @brief Gets the remaining time before watchdog expires.
     *
     *  @return 0 if watchdog is expired.
     *          Remaining time in milliseconds otherwise.
     */
    uint64_t timeRemaining() const override;

    /** @brief Reset timer to expire after new timeout in milliseconds.
     *
     *  @param[in] value - the time in milliseconds after which
     *                     the watchdog will expire
     *
     *  @return: updated timeout value if watchdog is enabled.
     *           0 otherwise.
     */
    uint64_t timeRemaining(uint64_t value) override;

    /** @brief Tells if the referenced timer is expired or not */
    inline auto timerExpired() const
    {
        return expired;
    }

    /** @brief Tells if the timer is running or not */
    inline bool timerEnabled() const
    {
        return timer.get_enabled() != sdeventplus::source::Enabled::Off;
    }

  private:
    /** @brief Types used for the timer object
     */
    static constexpr sdeventplus::ClockId clockId =
        sdeventplus::ClockId::Monotonic;
    using Clock = sdeventplus::Clock<clockId>;
    using Time = sdeventplus::source::Time<clockId>;

    /** @brief sdbusplus handle */
    sdbusplus::bus::bus& bus;

    /** @brief Map of systemd units to be started when the timer expires */
    ActionTargetMap actionTargetMap;

    /** @brief Fallback timer options */
    std::optional<Fallback> fallback;

    /** @brief Has our watchdog expired since being enabled? */
    bool expired;

    /** @brief Time source for timer configuration */
    Clock clock;

    /** @brief Contained timer object */
    Time timer;

    /** @brief Optional Callback handler on timer expirartion */
    void timeOutHandler(Time& source, Time::TimePoint time);

    /** @brief Attempt to enter the fallback watchdog or disables it */
    void tryFallbackOrDisable();
};

} // namespace watchdog
} // namespace phosphor
