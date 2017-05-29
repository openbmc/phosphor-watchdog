#pragma once

#include <memory>
#include <chrono>
#include <systemd/sd-event.h>
namespace phosphor
{
namespace watchdog
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        eventSource = sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/** @class Timer
 *  @brief Manages starting timers and handling timeouts
 */
class Timer
{
    public:
        /** @brief Only need the default Timer */
        Timer() = delete;
        ~Timer() = default;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;

        /** @brief Constructs timer object
         *
         *  @param[in] event   - sd_event unique pointer
         */
        Timer(EventPtr& event)
            : event(event)
        {
            // Initialize the timer
            initialize();
        }

        /** @brief Tells whether the timer is expired or not */
        inline auto expired() const
        {
            return expire;
        }

        /** @brief Returns the current Timer enablement type */
        int getTimer() const;

        /** @brief Enables / disables the timer */
        template <typename T> void setTimer()
        {
            constexpr auto type = T::value ? SD_EVENT_ONESHOT : SD_EVENT_OFF;
            return setTimer(type);
        }

        /** @brief Returns time remaining in usec before expiration
         *         which is an offset to current steady clock
         */
        std::chrono::microseconds getRemainingTime() const;

        /** @brief Starts the timer with specified expiration value.
         *         std::steady_clock is used for base time.
         *
         *  @param[in] usec - Microseconds from the current time
         *                    before expiration.
         *
         *  @return None.
         *
         *  @error Throws exception
         */
        void startTimer(std::chrono::microseconds usec);

        /** @brief Gets the current time from steady clock */
        static std::chrono::microseconds getCurrentTime();

    private:
        /** @brief pointer to sd_event */
        EventPtr& event;

        /** @brief event source */
        EventSourcePtr eventSource;

        /** @brief Set to true when the timeoutHandler is called into */
        bool expire = false;

        /** @brief Initializes the timer object with infinite
         *         expiration time and sets up the callback handler
         *
         *  @return None.
         *
         *  @error Throws exception
         */
        void initialize();

        /** @brief Callback function when timer goes off
         *
         *  @param[in] eventSource - Source of the event
         *  @param[in] usec        - time in microseconds
         *  @param[in] userData    - User data pointer
         *
         */
        static int timeoutHandler(sd_event_source* eventSource,
                                  uint64_t usec, void* userData);

        /** @brief Enables / disables the timer
         *
         *  @param[in] type - Timer type.
         *                    This implementation uses only SD_EVENT_OFF
         *                    and SD_EVENT_ONESHOT
         */
        void setTimer(int type);
};

} // namespace watchdog
} // namespace phosphor
