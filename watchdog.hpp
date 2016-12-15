#pragma once

#include "xyz/openbmc_project/State/Watchdog/server.hpp"

#include <systemd/sd-event.h>
#include <sdbusplus/server.hpp>

#include <chrono>

namespace phosphor
{
namespace state
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
        Watchdog(const Watchdog&) = delete;
        Watchdog& operator=(const Watchdog&) = delete;
        Watchdog(Watchdog&&) = delete;
        Watchdog& operator=(Watchdog &&) = delete;

        /** @brief Constructor for the Watchdog object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] obj - Object path to attach to.
         */
        Watchdog(sdbusplus::bus::bus& bus, const char* objPath);
        ~Watchdog();

        /** @brief Enable or disable watchdog
         *  If a watchdog state is changed from disable to enable,
         *  the watchdog timer is set with the default expiration interval,
         *  and it starts counting down.
         *  If a watchdog is already enabled, setting @value to true
         *  has no effect.
         *  @param[in] value - 'true' to enable. 'false' to disable
         */
        bool enabled(bool value) override;

        /** @brief Get the remaining time before watchdog expires.
         *  @retval If watchdog timer is disabled, returns 0,
         *  otherwise, returns remaining time in miliseconds.
         */
        uint64_t timeRemaining() const override;

        /** @brief Reset timer to expire after @value miliseconds.
         *  @param[in] value - the time in miliseconds after which
         *  the watchdog will expire */
        uint64_t timeRemaining(uint64_t value) override;

    private:
        // Callback for sd_event_add_time(), it needs to be static
        static int timeoutHandler(sd_event_source* es,
                                  uint64_t usec, void* userdata);

        // Return current time in usec
        std::chrono::microseconds now() const;

        // Set the expire time in miliseconds
        int setTimer(uint64_t value);

        // Enable(start) or diable(stop) timer
        int setTimerEnabled(bool enable);

        // Bus to which this watchdog attaches
        sdbusplus::bus::bus& bus;

        // The sd-event event source for one-shot timer
        sd_event_source* timerEventSource;
};

} // namespace watchdog
} // namespace state
} // namespace phosphor
