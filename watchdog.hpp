#pragma once

#include <systemd/sd-event.h>
#include <sdbusplus/server.hpp>
#include "xyz/openbmc_project/State/Watchdog/server.hpp"

namespace phosphor
{
namespace state
{
namespace watchdog
{
namespace detail
{
template <typename... T>
using ServerObject = sdbusplus::server::object::object<T...>;

using WatchdogIface =
    sdbusplus::xyz::openbmc_project::State::server::Watchdog;

} // namespace detail

/** @class Watchdog
 *  @brief OpenBMC watchdog implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.State.Watchdog DBus API.
 */
class Watchdog :
    public detail::ServerObject<detail::WatchdogIface>
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
            bus.detach_event();
            sd_event_unref(timerEvent);
        }

        /** @brief Constructor for the Watchdog object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] obj - Object path to attach to.
         */
        Watchdog(sdbusplus::bus::bus& bus,
                 const char* objpath) :
                detail::ServerObject<detail::WatchdogIface>(bus, objpath),
                bus(bus),
                timerEventSource(nullptr),
                timerEvent(nullptr)
        {
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

        /** @brief Initialize the sd-event event object and timer event source.
         *  Attach the bus with the event object.
         */
        int initTimer();

    private:
        // Callback for sd_event_add_time(), it needs to be static
        static int timeoutHandler(sd_event_source* es,
                                  uint64_t usec, void* userdata);

        // Set the expire time in miliseconds
        int setTimer(uint64_t value);

        // Enable timer to start counting down
        int enableTimer();

        // Stop the timer
        int disableTimer();

        // Return current time in us for @clock_id
        uint64_t now(clockid_t clock_id) const;

        // Bus to which this watchdog attaches
        sdbusplus::bus::bus& bus;

        // The sd-event event source for one-shot timer
        sd_event_source* timerEventSource;

        // The sd-event event loop object used to dispatch sd-bus event
        // and timer event
        sd_event* timerEvent;
};

} // namespace watchdog
} // namespace state
} // namespace phosphor
