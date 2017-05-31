#include <watchdog_test.hpp>

using namespace phosphor::watchdog;

/** @brief Make sure that watchdog is started and not enabled */
TEST_F(WdogTest, createWdogAndDontEnable)
{
    EXPECT_EQ(false, wdog.enabled());
    EXPECT_EQ(0, wdog.timeRemaining());
    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled */
TEST_F(WdogTest, createWdogAndEnable)
{
    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));
    EXPECT_EQ(false, wdog.timerExpired());

    // Get the configured interval
    auto remaining = milliseconds(wdog.timeRemaining());

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= defaultInterval - defaultDrift) &&
                (remaining <= defaultInterval));

    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Later, disable watchdog
 */
TEST_F(WdogTest, createWdogAndEnableThenDisable)
{
    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Disable and then verify
    EXPECT_EQ(false, wdog.enabled(false));
    EXPECT_EQ(false, wdog.enabled());
    EXPECT_EQ(0, wdog.timeRemaining());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait for 5 seconds and make sure that the remaining
 *         time shows 25 seconds.
 */
TEST_F(WdogTest, enableWdogAndWait5Seconds)
{
    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Sleep for 5 seconds
    auto sleepTime = seconds(5s);
    std::this_thread::sleep_for(sleepTime);

    // Get the remaining time again and expectation is that we get 25s
    auto remaining = milliseconds(wdog.timeRemaining());
    auto expected = defaultInterval -
                    duration_cast<milliseconds>(sleepTime);

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= expected - defaultDrift) &&
                (remaining <= expected));
    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait 1 second and then reset the timer to 5 seconds
 *         and then expect the watchdog to expire in 5 seconds
 */
TEST_F(WdogTest, enableWdogAndResetTo5Seconds)
{
    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Sleep for 1 second
    std::this_thread::sleep_for(1s);

    // Next timer will expire in 5 seconds from now.
    auto expireTime = seconds(5s);
    auto newTime = duration_cast<milliseconds>(expireTime);
    wdog.timeRemaining(newTime.count());

    // Waiting for expiration
    int count = 0;
    while(count < expireTime.count() && !wdog.timerExpired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1s));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, wdog.timerExpired());
    EXPECT_EQ(expireTime.count() - 1 , count);

    // Make sure secondary callback was not called.
    EXPECT_EQ(false, expired);
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait default interval seconds and make sure that wdog has died
 */
TEST_F(WdogTest, enableWdogAndWaitTillEnd)
{
    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));
    auto expireTime = duration_cast<seconds>(
                        milliseconds(defaultInterval));

    // Waiting default expiration
    int count = 0;
    while(count < expireTime.count() && !wdog.timerExpired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1s));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, wdog.enabled());
    EXPECT_EQ(0, wdog.timeRemaining());
    EXPECT_EQ(true, wdog.timerExpired());
    EXPECT_EQ(expireTime.count() - 1, count);
}