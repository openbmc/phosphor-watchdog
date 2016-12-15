#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace State
{
namespace server
{

class Watchdog
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
        Watchdog() = delete;
        Watchdog(const Watchdog&) = delete;
        Watchdog& operator=(const Watchdog&) = delete;
        Watchdog(Watchdog&&) = delete;
        Watchdog& operator=(Watchdog&&) = delete;
        virtual ~Watchdog() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Watchdog(bus::bus& bus, const char* path);



        /** @brief Implementation for Reset
         *  Re-arm the watchdog timer.
         */
        virtual void reset(
            ) = 0;


        /** @brief Send signal 'Timeout'
         *
         *  Signal indicating that watchdog timed out.
         */
        void timeout(
            );

        /** Get value of Enabled */
        virtual bool enabled() const;
        /** Set value of Enabled */
        virtual bool enabled(bool value);
        /** Get value of Interval */
        virtual uint64_t interval() const;
        /** Set value of Interval */
        virtual uint64_t interval(uint64_t value);
        /** Get value of TimeRemaining */
        virtual uint64_t timeRemaining() const;
        /** Set value of TimeRemaining */
        virtual uint64_t timeRemaining(uint64_t value);

    private:

        /** @brief sd-bus callback for Reset
         */
        static int _callback_Reset(
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'Enabled' */
        static int _callback_get_Enabled(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'Enabled' */
        static int _callback_set_Enabled(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'Interval' */
        static int _callback_get_Interval(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'Interval' */
        static int _callback_set_Interval(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'TimeRemaining' */
        static int _callback_get_TimeRemaining(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'TimeRemaining' */
        static int _callback_set_TimeRemaining(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);


        static constexpr auto _interface = "xyz.openbmc_project.State.Watchdog";
        static const vtable::vtable_t _vtable[];
        sdbusplus::server::interface::interface
                _xyz_openbmc_project_State_Watchdog_interface;

        bool _enabled{};
        uint64_t _interval = 30000;
        uint64_t _timeRemaining{};

};


} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

