AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# Build/add utest to test suite
check_PROGRAMS = argument_test \
                 watchdog_test

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

argument_test_CPPFLAGS = ${utestCPPFLAGS}
argument_test_CXXFLAGS = ${utestCXXFLAGS}
argument_test_LDFLAGS = ${utestLDFLAGS}

watchdog_test_CPPFLAGS = ${utestCPPFLAGS}
watchdog_test_CXXFLAGS = ${utestCXXFLAGS}
watchdog_test_LDFLAGS = ${utestLDFLAGS}

argument_test_SOURCES = ../argument.cpp argument_test.cpp
watchdog_test_SOURCES = ../watchdog.cpp watchdog_test.cpp
