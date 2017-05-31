#include <iostream>
#include <chrono>
#include <gtest/gtest.h>
#include <watchdog.hpp>

using namespace phosphor::watchdog;

// Tells if the watchdog timer expired.
bool expired{};

// Dummy names for object path
constexpr auto TEST_PATH = "/test/path";

// 30 Seconds in milliseconds
constexpr auto DEFAULT_INTERVAL = 30000;

// Depending on how we get scheduled, we may get 29999 when checked
// So need this to take care of that condition
constexpr auto DEFAULT_DRIFT = 29999;

// 25 Seconds in milliseconds
constexpr auto _25_SECONDS_INTERVAL = 25000;

class WdogTest : public ::testing::Test
{
    public:
        // systemd event handler
        sd_event* events;

        // Need this so that events can be initialized.
        int rc;

        // sdbusplus bus handler
        sdbusplus::bus::bus bus;

        // Gets called as part of each TEST_F construction
        WdogTest()
            : rc(sd_event_default(&events)),
              bus(sdbusplus::bus::new_default())
        {
            // Check for successful creation of
            // event handler and bus handler
            EXPECT_GE(rc, 0);
        }

        // Handler called by timer expiration
        void timeOutHandler()
        {
            std::cout << "Time out handler called" << std::endl;
            expired = true;
        }
};

/** @brief Make sure that watchdog is started and not enabled */
TEST_F(WdogTest, createWdogAndDontEnable)
{
    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);

    EXPECT_EQ(false, wdog.enabled());
    EXPECT_EQ(0, wdog.timeRemaining());
    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled */
TEST_F(WdogTest, createWdogAndEnable)
{
    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);
    EXPECT_EQ(false, wdog.enabled());

    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Get how long have spent here.
    auto remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= DEFAULT_DRIFT) &&
                (remaining <= DEFAULT_INTERVAL));

    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Later, disable watchdog
 */
TEST_F(WdogTest, createWdogAndEnableThenDisable)
{
    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);
    EXPECT_EQ(false, wdog.enabled());

    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Get how long have spent here.
    auto remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= DEFAULT_DRIFT) &&
                (remaining <= DEFAULT_INTERVAL));

    EXPECT_EQ(false, wdog.timerExpired());

    // Disable and then verify
    EXPECT_EQ(false, wdog.enabled(false));
    EXPECT_EQ(false, wdog.enabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait for 5 seconds and make sure that the remaining
 *         time shows 25 seconds.
 */
TEST_F(WdogTest, enableWdogAndWait5Seconds)
{
    using namespace std::chrono;

    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);
    EXPECT_EQ(false, wdog.enabled());

    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Get the remaining time again and expectation is that we get 30s
    auto remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= DEFAULT_DRIFT) &&
                (remaining <= DEFAULT_INTERVAL));

    sleep(5);

    // Get the remaining time again and expectation is that we get 25s
    remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= (_25_SECONDS_INTERVAL - 1)) &&
                (remaining <=  _25_SECONDS_INTERVAL));

    EXPECT_EQ(false, wdog.timerExpired());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait 1 second and then reset the timer to 5 seconds
 *         and then expect the watchdog to expire in 5 seconds
 */
TEST_F(WdogTest, enableWdogAndResetTo5Seconds)
{
    using namespace std::chrono;

    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);
    EXPECT_EQ(false, wdog.enabled());

    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Get the remaining time again and expectation is that we get 30s
    auto remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= DEFAULT_DRIFT) &&
                (remaining <= DEFAULT_INTERVAL));

    sleep(1);

    // Next timer will expire in 5 seconds from now.
    auto newTime = duration_cast<milliseconds>(seconds(5));
    wdog.timeRemaining(newTime.count());

    // Waiting for expiration
    int count = 0;
    while(count < 5 && !wdog.timerExpired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, wdog.timerExpired());
    EXPECT_EQ(4, count);

    // Make sure secondary callback was not called.
    EXPECT_EQ(false, expired);

}

/** @brief Starts the timer and expects it to
 *         expire in configured time and expects the
 *         deault callback handler to kick-in
 */
TEST_F(WdogTest, testTimerForExpirationDefaultTimeoutHandler)
{
    using namespace std::chrono;

    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Expect timer to expire in 2 seconds
    auto expireTime = duration_cast<microseconds>(seconds(2));

    phosphor::watchdog::Timer timer(eventP);

    // Set the expiration and enable the timer
    timer.startTimer(expireTime);
    timer.setTimer<std::true_type>();

    // Waiting 2 seconds to expect expiration
    int count = 0;
    while(count < 2 && !timer.expired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, timer.expired());
    EXPECT_EQ(1, count);

    // Make sure secondary callback was not called.
    EXPECT_EQ(false, expired);
}

/** @brief Starts the timer and expects it to expire
 *         in configured time and expects the secondary
 *         callback to be called into along with default.
 */
TEST_F(WdogTest, testTimerForExpirationSecondCallBack)
{
    using namespace std::chrono;

    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Expect timer to expire in 2 seconds
    auto expireTime = duration_cast<microseconds>(seconds(2));

    phosphor::watchdog::Timer timer(eventP,
                    std::bind(&WdogTest::timeOutHandler, this));

    // Set the expiration and enable the timer
    timer.startTimer(expireTime);
    timer.setTimer<std::true_type>();

    // Waiting 2 seconds to expect expiration
    int count = 0;
    while(count < 2 && !timer.expired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, timer.expired());
    EXPECT_EQ(1, count);

    // This gets set as part of secondary callback
    EXPECT_EQ(true, expired);
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait 30 seconds and make sure that wdog has died
 */
TEST_F(WdogTest, enableWdogAndWaitTillEnd)
{
    using namespace std::chrono;

    phosphor::watchdog::EventPtr eventP{events};
    events = nullptr;

    // Create a watchdog object
    phosphor::watchdog::Watchdog wdog(bus, TEST_PATH, eventP);
    EXPECT_EQ(false, wdog.enabled());

    // Enable and then verify
    EXPECT_EQ(true, wdog.enabled(true));

    // Get the remaining time again and expectation is that we get 30s
    auto remaining = wdog.timeRemaining();

    // Its possible that we are off by ~1 msec depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= DEFAULT_DRIFT) &&
                (remaining <= DEFAULT_INTERVAL));

    // Waiting 30 seconds to expect expiration
    int count = 0;
    while(count < 30 && !wdog.timerExpired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if(!sd_event_run(eventP.get(), sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, wdog.enabled());
    EXPECT_EQ(0, wdog.timeRemaining());
    EXPECT_EQ(true, wdog.timerExpired());
    EXPECT_EQ(29, count);
}
