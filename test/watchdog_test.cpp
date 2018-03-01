#include <chrono>

#include "watchdog_test.hpp"

using namespace phosphor::watchdog;

seconds WdogTest::waitForWatchdog(seconds timeLimit)
{
    auto previousTimeRemaining = wdog->timeRemaining();
    auto ret = 0s;
    while (ret < timeLimit &&
           previousTimeRemaining >= wdog->timeRemaining() &&
           wdog->timerEnabled())
    {
        previousTimeRemaining = wdog->timeRemaining();

        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = 1s;
        if(!sd_event_run(eventP.get(), microseconds(sleepTime).count()))
        {
            ret += sleepTime;
        }
    }

    return ret;
}

/** @brief Make sure that watchdog is started and not enabled */
TEST_F(WdogTest, createWdogAndDontEnable)
{
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

    // We should be able to configure persistent properties
    // while disabled
    auto newAction = Watchdog::Action::Poweroff;
    EXPECT_EQ(newAction, wdog->expireAction(newAction));
    auto newIntervalMs = milliseconds(defaultInterval * 2).count();
    EXPECT_EQ(newIntervalMs, wdog->interval(newIntervalMs));

    EXPECT_EQ(newAction, wdog->expireAction());
    EXPECT_EQ(newIntervalMs, wdog->interval());

    // We won't be able to configure timeRemaining
    EXPECT_EQ(0, wdog->timeRemaining(1000));
    EXPECT_EQ(0, wdog->timeRemaining());

    // Timer should not have become enabled
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled */
TEST_F(WdogTest, createWdogAndEnable)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Get the configured interval
    auto remaining = milliseconds(wdog->timeRemaining());

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= defaultInterval - defaultDrift) &&
                (remaining <= defaultInterval));

    EXPECT_FALSE(wdog->timerExpired());
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
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
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
    EXPECT_FALSE(wdog->timerExpired());
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
    EXPECT_EQ(expireTime - 1s, waitForWatchdog(expireTime));
    EXPECT_TRUE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

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
    EXPECT_EQ(expireTime - 1s, waitForWatchdog(expireTime));

    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_TRUE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}
