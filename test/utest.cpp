#include <gtest/gtest.h>
#include "watchdog.hpp"

using namespace phosphor::state::watchdog;

constexpr auto testObjpath = "/watchdog/test";
constexpr auto testService = "watchdog.test";

class WatchdogTest : public testing::Test
{
    public:
        std::string objpath;
        std::string service;
        sdbusplus::bus::bus bus;
        Watchdog watchdog;

        WatchdogTest()
            : objpath(testObjpath),
              service(testService),
              bus(sdbusplus::bus::new_default()),
              watchdog(bus, objpath.c_str())
        {
        }

        ~WatchdogTest()
        {
        }

        virtual void SetUp()
        {
            sdbusplus::server::manager::manager objManager(bus,
                                                    objpath.c_str());
            bus.request_name(service.c_str());
            // intialize watchdog correct?
            ASSERT_GE(watchdog.initTimer(), 0);
        }
};

/** @brief Get watchdog default interval */
TEST_F(WatchdogTest, getInterval)
{
    // default interval is 30000 msec
    EXPECT_EQ(30000, watchdog.interval());
}

/** @brief Set watchdog interval */
TEST_F(WatchdogTest, setInterval)
{
    watchdog.interval(10000);
    EXPECT_EQ(10000, watchdog.interval());
}

/** @brief Get remaining time */
TEST_F(WatchdogTest, getRemainTime)
{
    // if not enabled, expect 0
    EXPECT_EQ(0, watchdog.timeRemaining());
    watchdog.enabled(true);
    // if enabled, expect a positive value
    EXPECT_GT(watchdog.timeRemaining(), 0);
    watchdog.enabled(false);
    EXPECT_EQ(0, watchdog.timeRemaining());
}

/** @brief Set remaining time */
TEST_F(WatchdogTest, setRemainTime)
{
    watchdog.timeRemaining(10000);
    // if not enabled, set timeRemaining has not effect
    EXPECT_EQ(0, watchdog.timeRemaining());
    watchdog.enabled(true);
    watchdog.timeRemaining(10000);
    // expect the timeRemaining between 0 and 10000 msec
    EXPECT_GT(watchdog.timeRemaining(), 0);
    EXPECT_LE(watchdog.timeRemaining(), 10000);
}

// Todo, and test to check Timeout signal