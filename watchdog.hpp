#pragma once

#include <systemd/sd-event.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/State/Watchdog/server.hpp>
#include "timer.hpp"
namespace phosphor
{
namespace watchdog
{
using WatchdogInherits = sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::State::server::Watchdog>;

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

        /** @brief Constructs the Watchdog object
         *
         *  @param[in] bus     - DBus bus to attach to.
         *  @param[in] objPath - Object path to attach to.
         *  @param[in] event   - reference to sd_event unique pointer
         *  @param[in] target  - systemd target to be called into on timeout
         */
        Watchdog(sdbusplus::bus::bus& bus,
                const char* objPath,
                EventPtr& event,
                std::string target = "") :
            WatchdogInherits(bus, objPath),
            bus(bus),
            target(target),
            timer(event, std::bind(&Watchdog::timeOutHandler, this))
        {
            // Nothing
        }

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
         *  @return 0 if watchdog is disabled or expired.
         *          Remaining time in milliseconds otherwise.
         */
        uint64_t timeRemaining() const override;

        /** @brief Reset timer to expire after new timeout in milliseconds.
         *
         *  @param[in] value - the time in miliseconds after which
         *                     the watchdog will expire
         *
         *  @return: updated timeout value if watchdog is enabled.
         *           0 otherwise.
         */
        uint64_t timeRemaining(uint64_t value) override;

        /** @brief Tells if the referenced timer is expired or not */
        inline auto timerExpired() const
        {
            return timer.expired();
        }

    private:
        /** @brief sdbusplus handle */
        sdbusplus::bus::bus& bus;

        /** @brief Systemd unit to be started when the timer expires */
        std::string target;

        /** @brief Contained timer object */
        Timer timer;

        /** @brief Optional Callback handler on timer expirartion */
        void timeOutHandler();
};

} // namespace watchdog
} // namespace phosphor
