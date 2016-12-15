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
        Watchdog& operator=(Watchdog&&) = delete;
        ~Watchdog()
        {
            // free event source and event
            sd_event_source_unref(timer_event_source);
            bus.detach_event();
            sd_event_unref(timer_event);
        }

        /** @brief Constructor for the Watchdog object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] obj - Object path to attach to.
         */
        Watchdog(sdbusplus::bus::bus& bus,
                    const char* objpath) :
                    detail::ServerObject<detail::WatchdogIface>(
                            bus, objpath),
                    bus(bus)
        {
        }

        /** @brief Start the watchdog event loop
         *  @retval On success, these functions return 0 or a positive integer */
        int run();

        /** @brief Enable or disable watchdog
         *  @param[in] value - 'true' to enable. 'false' to disable
         */
        bool enabled(bool value) override;

        /** @brief Get the remaining time (in miliseconds)  before
         *  watchdog expires.
         *  @retval If watchdog timer is disabled, returns 0.
         */
        uint64_t timeRemaining() const override;

        /** @brief Reset timer to expire after @value miliseconds. */
        uint64_t timeRemaining(uint64_t value) override;

    private:
        /** @brief Callback for sd_event, it needs to be static */
        static int timeoutHandler(sd_event_source *es,
                uint64_t usec, void *userdata);

        /** @brief Intialize the sd-event event object and timer event source */
        int initTimer();

        /** @brief Arm the one-shot time to expire after @value miliseconds */
        int setTimer(uint64_t value);

        int enableTimer();

        int disableTimer();

        /** @brief Return current time in us from @clock_id  */
        uint64_t now(clockid_t clock_id) const;

        /** @brief Bus to which this watchdog attaches */
        sdbusplus::bus::bus &bus;

        sd_event_source* timer_event_source;

        sd_event* timer_event;
};

} // namespace watchdog
} // namespace state
} // namespace phosphor
