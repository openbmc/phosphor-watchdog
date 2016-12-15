#pragma once

#include <time.h>
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
template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using WatchdogIface =
    sdbusplus::xyz::openbmc_project::State::server::Watchdog;

} // namespace detail

/** @class Watchdog
 *  @brief OpenBMC watchdog implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.State.Watchdog DBus API.
 */
class Watchdog final :
    public detail::ServerObject<detail::WatchdogIface>
{
    public:
        Watchdog() = delete;
        Watchdog(const Watchdog&) = delete;
        Watchdog& operator=(const Watchdog&) = delete;
        Watchdog(Watchdog&&) = default;
        Watchdog& operator=(Watchdog&&) = default;
        ~Watchdog() = default;

        /** @brief Constructor for the Watchdog object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] busname - Name of DBus bus to own.
         *  @param[in] obj - Object path to attach to.
         */
        Watchdog(sdbusplus::bus::bus&& bus,
                    const char* busname,
                    const char* obj);

        /** @brief Start processing DBus messages. */
        void run() noexcept;

        /*
         * @fn reset()
         * @brief sd_bus Reset method implementation callback.
         * @details Re-arm the watchdog.
         */
        void reset() override;
        bool enabled(bool value) override;
        uint64_t timeRemaining() const override;
    private:
        /** @brief Signal handler for sigaction(), it needs to be static */
        static void timeoutHandler(int signum, siginfo_t *si, void *uc);
        /** @brief Arm the one-shot time */
        int armTimer();
        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus _bus;

        /** @brief sdbusplus org.freedesktop.DBus.ObjectManager reference. */
        sdbusplus::server::manager::manager _manager;
        /** @brief a posix timer */
        timer_t timerid;
        /** @brief object path of this watchdog */
        std::string objpath;
};

} // namespace watchdog
} // namespace state
} // namespace phosphor
