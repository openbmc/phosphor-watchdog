#pragma once

#include <systemd/sd-event.h>
#include <experimental/optional>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/State/Watchdog/server.hpp>
#include "timer.hpp"
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
        Watchdog& operator=(Watchdog &&) = delete;

        /** @brief Type used to hold the name of a systemd target.
         */
        using TargetName = std::string;

        /** @brief Type used to specify the parameters of a fallback watchdog
         */
        struct Fallback {
            Action action;
            uint64_t interval;
        };

        /** @brief Constructs the Watchdog object
         *
         *  @param[in] bus            - DBus bus to attach to.
         *  @param[in] objPath        - Object path to attach to.
         *  @param[in] event          - reference to sd_event unique pointer
         *  @param[in] actionTargets  - map of systemd targets called on timeout
         *  @param[in] fallback
         */
        Watchdog(sdbusplus::bus::bus& bus,
                const char* objPath,
                EventPtr& event,
                std::map<Action, TargetName>&& actionTargets =
                    std::map<Action, TargetName>(),
                std::experimental::optional<Fallback>&&
                    fallback = std::experimental::nullopt) :
            WatchdogInherits(bus, objPath),
            bus(bus),
            actionTargets(std::move(actionTargets)),
            fallback(std::move(fallback)),
            timer(event, std::bind(&Watchdog::timeOutHandler, this))
        {
            // Nothing
        }

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

        inline bool timerEnabled()
        {
            return timer.getEnabled() != SD_EVENT_OFF;
        }

    private:
        /** @brief sdbusplus handle */
        sdbusplus::bus::bus& bus;

        /** @brief Map of systemd units to be started when the timer expires */
        std::map<Action, TargetName> actionTargets;

        /** @brief Fallback timer options */
        std::experimental::optional<Fallback> fallback;

        /** @brief Contained timer object */
        Timer timer;

        /** @brief Optional Callback handler on timer expirartion */
        void timeOutHandler();

        /** @brief Attempt to enter the fallback watchdog or disables it */
        void tryFallbackOrDisable();
};

} // namespace watchdog
} // namespace phosphor
