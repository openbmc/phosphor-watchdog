#include <timer_test.hpp>
#include <chrono>
#include <memory>
#include <watchdog.hpp>

using namespace std::chrono;
using namespace std::chrono_literals;

// Test Watchdog functionality
class WdogTest : public TimerTest
{
    public:
        // Gets called as part of each TEST_F construction
        WdogTest()
            : bus(sdbusplus::bus::new_default()),
              wdog(std::make_unique<phosphor::watchdog::Watchdog>(
                      bus, TEST_PATH, eventP)),
              defaultInterval(milliseconds(wdog.interval())),
              defaultDrift(30)
        {
            // Check for successful creation of
            // event handler and bus handler
            EXPECT_GE(rc, 0);

            // Initially the watchdog would be disabled
            EXPECT_FALSE(wdog->enabled());
        }

        //sdbusplus handle
        sdbusplus::bus::bus bus;

        // Watchdog object
        std::unique_ptr<phosphor::watchdog::Watchdog> wdog;

        // This is the default interval as given in Interface definition
        milliseconds defaultInterval;

        // Acceptable drift when we compare the interval to timeRemaining.
        // This is needed since it depends on when do we get scheduled and it
        // has happened that remaining time was off by few msecs.
        milliseconds defaultDrift;

    private:
        // Dummy name for object path
        // This is just to satisfy the constructor. Does not have
        // a need to check if the objects paths have been created.
        static constexpr auto TEST_PATH = "/test/path";
};
