#pragma once

#include <phosphor-logging/log.hpp>
#include <systemd/sd-event.h>
#include <sdbusplus/server.hpp>
#include "xyz/openbmc_project/State/Watchdog/server.hpp"

namespace phosphor
{
namespace state
{
namespace watchdog
{
using namespace phosphor::logging;
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
        ~Watchdog()
        {
            // free event source and event
            sd_event_source_unref(timerEventSource);
        }

        /** @brief Constructor for the Watchdog object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] obj - Object path to attach to.
         */
        Watchdog(sdbusplus::bus::bus& bus,
                 const char* objPath) :
                WatchdogInherits(bus, objPath),
                bus(bus),
                timerEventSource(nullptr)
        {
            sd_event_add_time(
                bus.get_event(),
                &timerEventSource,
                CLOCK_MONOTONIC,
                UINT64_MAX, /* expire time - never */
                0, /* use default event accuracy that the event may be delayed */
                timeoutHandler, this);

            // Timer is enabled by default. Disable for now.
            auto r = setTimerEnabled<std::false_type>();
            if (r >= 0)
            {
                log<level::INFO>("watchdog ready");
            }
        }

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
        auto now() const;

        // Set the expire time in miliseconds
        int setTimer(uint64_t value);

        // Enable(start) or diable(stop) timer
        template <typename T> int setTimerEnabled()
        {
            // One-shot timer: the event source will be enabled but
            // automatically reset to SD_EVENT_OFF after the event source was
            // dispatched once
            auto state = T::value ? SD_EVENT_ONESHOT : SD_EVENT_OFF;

            auto r = sd_event_source_set_enabled(timerEventSource, state);
            if (r < 0)
            {
                log<level::ERR>
                    ("watchdog setTimerEnabled: \
                        sd_event_source_set_enabled() error",
                    entry("ERROR=%s", strerror(-r)));
            }

            return r;
        }


        // Bus to which this watchdog attaches
        sdbusplus::bus::bus& bus;

        // The sd-event event source for one-shot timer
        sd_event_source* timerEventSource;
};

} // namespace watchdog
} // namespace state
} // namespace phosphor
