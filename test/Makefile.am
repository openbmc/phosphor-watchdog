AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# Build/add utest to test suite
check_PROGRAMS = watchdog

utestCPPFLAGS = $(GTEST_MAIN_CFLAGS) \
                $(AM_CPPFLAGS) \
                $(SDBUSPLUS_CFLAGS) \
                $(SDEVENTPLUS_CFLAGS) \
                $(PHOSPHOR_LOGGING_CFLAGS) \
                $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

utestCXXFLAGS = $(PTHREAD_CFLAGS)

utestLDFLAGS = $(GTEST_MAIN_LIBS) \
               $(PTHREAD_LIBS) \
               $(OESDK_TESTCASE_FLAGS) \
               $(SDBUSPLUS_LIBS) \
               $(SDEVENTPLUS_LIBS) \
               $(PHOSPHOR_LOGGING_LIBS) \
               $(PHOSPHOR_DBUS_INTERFACES_LIBS)

watchdog_CPPFLAGS = ${utestCPPFLAGS}
watchdog_CXXFLAGS = ${utestCXXFLAGS}
watchdog_LDFLAGS = ${utestLDFLAGS}

watchdog_SOURCES = ../watchdog.cpp watchdog.cpp
