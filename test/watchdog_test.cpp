#include <chrono>
#include <memory>
#include <utility>

#include "watchdog_test.hpp"

using namespace phosphor::watchdog;

/** @brief Make sure that watchdog is started and not enabled */
TEST_F(WdogTest, createWdogAndDontEnable)
{
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled */
TEST_F(WdogTest, createWdogAndEnable)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_TRUE(wdog->timerEnabled());

    // Get the configured interval
    auto remaining = milliseconds(wdog->timeRemaining());

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= defaultInterval - defaultDrift) &&
                (remaining <= defaultInterval));

    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Later, disable watchdog
 */
TEST_F(WdogTest, createWdogAndEnableThenDisable)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Disable and then verify
    EXPECT_FALSE(wdog->enabled(false));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait for 5 seconds and make sure that the remaining
 *         time shows 25 seconds.
 */
TEST_F(WdogTest, enableWdogAndWait5Seconds)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Sleep for 5 seconds
    auto sleepTime = seconds(5s);
    std::this_thread::sleep_for(sleepTime);

    // Get the remaining time again and expectation is that we get 25s
    auto remaining = milliseconds(wdog->timeRemaining());
    auto expected = defaultInterval -
                    duration_cast<milliseconds>(sleepTime);

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= expected - defaultDrift) &&
                (remaining <= expected));
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait 1 second and then reset the timer to 5 seconds
 *         and then expect the watchdog to expire in 5 seconds
 */
TEST_F(WdogTest, enableWdogAndResetTo5Seconds)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Sleep for 1 second
    std::this_thread::sleep_for(1s);

    // Next timer will expire in 5 seconds from now.
    auto expireTime = seconds(5s);
    auto newTime = duration_cast<milliseconds>(expireTime);
    wdog->timeRemaining(newTime.count());

    // Waiting for expiration
    int count = 0;
    while(count < expireTime.count() && wdog->enabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1s));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_FALSE(wdog->timerEnabled());
    EXPECT_EQ(expireTime.count() - 1, count);

    // Make sure secondary callback was not called.
    EXPECT_FALSE(expired);
}

/** @brief Make sure the Interval can be updated directly.
 */
TEST_F(WdogTest, verifyIntervalUpdateReceived)
{
    auto expireTime = seconds(5s);
    auto newTime = duration_cast<milliseconds>(expireTime);
    wdog->interval(newTime.count());

    // Expect an update in the Interval
    EXPECT_EQ(newTime.count(), wdog->interval());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait default interval seconds and make sure that wdog has died
 */
TEST_F(WdogTest, enableWdogAndWaitTillEnd)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    auto expireTime = duration_cast<seconds>(
                        milliseconds(defaultInterval));

    // Waiting default expiration
    int count = 0;
    while(count < expireTime.count() && wdog->enabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1s));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }

    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerEnabled());
    EXPECT_EQ(expireTime.count() - 1, count);
}

/** @brief Make sure the watchdog is started and enabled with a fallback
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that fallback runs to completion and ensure the watchdog
 *         is disabled
 */
TEST_F(WdogTest, enableWdogWithFallbackTillEnd)
{
    auto primaryInterval = 5s;
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback{
        .action = Watchdog::Action::PowerOff,
        .interval = static_cast<uint64_t>(fallbackIntervalMs),
    };
    std::map<Watchdog::Action, Watchdog::TargetName> emptyActionTargets;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, eventP,
                    std::move(emptyActionTargets), std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    auto waited = 0s;
    while(waited < primaryInterval && wdog->enabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            waited += sleepTime;
        }
    }
    EXPECT_EQ(primaryInterval - 1s, waited);

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_TRUE(wdog->timerEnabled());

    // We should still be ticking in fallback when setting action or interval
    auto newInterval = primaryInterval - 1s;
    auto newIntervalMs = milliseconds(newInterval).count();
    EXPECT_EQ(newInterval, milliseconds(wdog->interval(newIntervalMs)));
    EXPECT_EQ(Watchdog::Action::None,
            wdog->expireAction(Watchdog::Action::None));

    EXPECT_FALSE(wdog->enabled());
    EXPECT_GE(remaining, milliseconds(wdog->timeRemaining()));
    EXPECT_LT(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_TRUE(wdog->timerEnabled());

    // Test that setting the timeRemaining always resets the timer to the
    // fallback interval
    EXPECT_EQ(fallback.interval, wdog->timeRemaining(primaryInterval.count()));
    EXPECT_FALSE(wdog->enabled());

    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LE(fallbackInterval - defaultDrift, remaining);
    EXPECT_TRUE(wdog->timerEnabled());

    // Waiting fallback expiration
    waited = 0s;
    while(waited < fallbackInterval && wdog->timerEnabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            waited += sleepTime;
        }
    }
    EXPECT_EQ(fallbackInterval - 1s, waited);

    // We should now have disabled the watchdog after the fallback expires
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerEnabled());

    // Make sure enabling the watchdog again works
    EXPECT_TRUE(wdog->enabled(true));

    // We should have re-entered the primary
    EXPECT_TRUE(wdog->enabled());
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and enabled with a fallback
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that we can re-enable the watchdog during fallback
 */
TEST_F(WdogTest, enableWdogWithFallbackReEnable)
{
    auto primaryInterval = 5s;
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback{
        .action = Watchdog::Action::PowerOff,
        .interval = static_cast<uint64_t>(fallbackIntervalMs),
        .always_enabled = false,
    };
    std::map<Watchdog::Action, Watchdog::TargetName> emptyActionTargets;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, eventP,
                    std::move(emptyActionTargets), std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    auto waited = 0s;
    while(waited <= primaryInterval && wdog->enabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            waited += sleepTime;
        }
    }
    EXPECT_EQ(primaryInterval - 1s, waited);

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_TRUE(wdog->timerEnabled());

    EXPECT_TRUE(wdog->enabled(true));

    // We should have re-entered the primary
    EXPECT_TRUE(wdog->enabled());
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and with a fallback without
 *         sending an enable
 *         Then enable the watchdog
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that fallback runs to completion and ensure the watchdog
 *         is in the fallback state again
 */
TEST_F(WdogTest, enableWdogWithFallbackAlways)
{
    auto primaryInterval = 5s;
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback{
        .action = Watchdog::Action::PowerOff,
        .interval = static_cast<uint64_t>(fallbackIntervalMs),
        .always_enabled = true,
    };
    std::map<Watchdog::Action, Watchdog::TargetName> emptyActionTargets;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, eventP,
                    std::move(emptyActionTargets), std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));

    // Waiting default expiration
    auto waited = 0s;
    while(waited < primaryInterval && wdog->enabled())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            waited += sleepTime;
        }
    }
    EXPECT_EQ(primaryInterval - 1s, waited);

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_TRUE(wdog->timerEnabled());

    // Waiting fallback expiration
    waited = 0s;
    while(waited < fallbackInterval &&
          remaining >= milliseconds(wdog->timeRemaining()))
    {
        remaining = milliseconds(wdog->timeRemaining());

        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            waited += sleepTime;
        }
    }
    EXPECT_EQ(fallbackInterval - 1s, waited);

    // We should now enter the fallback again
    EXPECT_FALSE(wdog->enabled());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_TRUE(wdog->timerEnabled());
}
