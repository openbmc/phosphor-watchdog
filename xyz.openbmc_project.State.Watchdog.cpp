#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/State/Watchdog/server.hpp>

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

Watchdog::Watchdog(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_State_Watchdog_interface(
                bus, path, _interface, _vtable, this)
{
}



void Watchdog::timeout(
            )
{
    using sdbusplus::server::binding::details::convertForMessage;

    auto& i = _xyz_openbmc_project_State_Watchdog_interface;
    auto m = i.new_signal("Timeout");

    m.append();
    m.signal_send();
}

namespace details
{
namespace Watchdog
{
static const auto _signal_Timeout =
        utility::tuple_to_array(std::make_tuple('\0'));
}
}

auto Watchdog::enabled() const ->
        bool
{
    return _enabled;
}

int Watchdog::_callback_get_Enabled(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Watchdog*>(context);
        m.append(convertForMessage(o->enabled()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Watchdog::enabled(bool value) ->
        bool
{
    if (_enabled != value)
    {
        _enabled = value;
        _xyz_openbmc_project_State_Watchdog_interface.property_changed("Enabled");
    }

    return _enabled;
}

int Watchdog::_callback_set_Enabled(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Watchdog*>(context);

        bool v{};
        m.read(v);
        o->enabled(v);
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Watchdog
{
static const auto _property_Enabled =
    utility::tuple_to_array(message::types::type_id<
            bool>());
}
}
auto Watchdog::interval() const ->
        uint64_t
{
    return _interval;
}

int Watchdog::_callback_get_Interval(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Watchdog*>(context);
        m.append(convertForMessage(o->interval()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Watchdog::interval(uint64_t value) ->
        uint64_t
{
    if (_interval != value)
    {
        _interval = value;
        _xyz_openbmc_project_State_Watchdog_interface.property_changed("Interval");
    }

    return _interval;
}

int Watchdog::_callback_set_Interval(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Watchdog*>(context);

        uint64_t v{};
        m.read(v);
        o->interval(v);
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Watchdog
{
static const auto _property_Interval =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}
auto Watchdog::timeRemaining() const ->
        uint64_t
{
    return _timeRemaining;
}

int Watchdog::_callback_get_TimeRemaining(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Watchdog*>(context);
        m.append(convertForMessage(o->timeRemaining()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Watchdog::timeRemaining(uint64_t value) ->
        uint64_t
{
    if (_timeRemaining != value)
    {
        _timeRemaining = value;
        _xyz_openbmc_project_State_Watchdog_interface.property_changed("TimeRemaining");
    }

    return _timeRemaining;
}

int Watchdog::_callback_set_TimeRemaining(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Watchdog*>(context);

        uint64_t v{};
        m.read(v);
        o->timeRemaining(v);
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Watchdog
{
static const auto _property_TimeRemaining =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}


const vtable::vtable_t Watchdog::_vtable[] = {
    vtable::start(),

    vtable::signal("Timeout",
                   details::Watchdog::_signal_Timeout
                        .data()),
    vtable::property("Enabled",
                     details::Watchdog::_property_Enabled
                        .data(),
                     _callback_get_Enabled,
                     _callback_set_Enabled,
                     vtable::property_::emits_change),
    vtable::property("Interval",
                     details::Watchdog::_property_Interval
                        .data(),
                     _callback_get_Interval,
                     _callback_set_Interval,
                     vtable::property_::emits_change),
    vtable::property("TimeRemaining",
                     details::Watchdog::_property_TimeRemaining
                        .data(),
                     _callback_get_TimeRemaining,
                     _callback_set_TimeRemaining,
                     vtable::property_::emits_change),
    vtable::end()
};

} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

