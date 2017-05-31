// This file was autogenerated.  Do not edit!
// See elog-gen.py for more details
#pragma once

#include <string>
#include <tuple>
#include <type_traits>
#include <sdbusplus/exception.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Watchdog
{
namespace Timer
{
namespace Error
{
    struct InternalError;
} // namespace Error
} // namespace Timer
} // namespace Watchdog
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

namespace phosphor
{

namespace logging
{

namespace xyz
{
namespace openbmc_project
{
namespace Watchdog
{
namespace Timer
{
namespace _InternalError
{

struct ERROR_DESCRIPTION
{
    static constexpr auto str = "ERROR_DESCRIPTION=%s";
    static constexpr auto str_short = "ERROR_DESCRIPTION";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr ERROR_DESCRIPTION(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _InternalError

struct InternalError : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.Watchdog.Timer.InternalError";
    static constexpr auto errDesc = "An Internal error occured with timer";
    static constexpr auto L = level::INFO;
    using ERROR_DESCRIPTION = _InternalError::ERROR_DESCRIPTION;
    using metadata_types = std::tuple<ERROR_DESCRIPTION>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Timer
} // namespace Watchdog
} // namespace openbmc_project
} // namespace xyz


namespace details
{

template <>
struct map_exception_type<sdbusplus::xyz::openbmc_project::Watchdog::Timer::Error::InternalError>
{
    using type = xyz::openbmc_project::Watchdog::Timer::InternalError;
};

}

} // namespace logging

} // namespace phosphor