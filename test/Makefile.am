AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# Build/add utest to test suite
check_PROGRAMS = timer_test \
                 watchdog_test

utestCPPFLAGS = $(GTEST_MAIN_CFLAGS) \
                $(AM_CPPFLAGS) \
                $(SDBUSPLUS_CFLAGS) \
                $(PHOSPHOR_LOGGING_CFLAGS) \
                $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

utestCXXFLAGS = $(PTHREAD_CFLAGS)

utestLDFLAGS = $(GTEST_MAIN_LIBS) \
               $(PTHREAD_LIBS) \
               $(OESDK_TESTCASE_FLAGS) \
               $(SDBUSPLUS_LIBS) \
               $(PHOSPHOR_LOGGING_LIBS) \
               $(PHOSPHOR_DBUS_INTERFACES_LIBS)

timer_test_CPPFLAGS = ${utestCPPFLAGS}
timer_test_CXXFLAGS = ${utestCXXFLAGS}
timer_test_LDFLAGS = ${utestLDFLAGS}

watchdog_test_CPPFLAGS = ${utestCPPFLAGS}
watchdog_test_CXXFLAGS = ${utestCXXFLAGS}
watchdog_test_LDFLAGS = ${utestLDFLAGS}

timer_test_SOURCES = timer_test.cpp
watchdog_test_SOURCES = watchdog_test.cpp

timer_test_LDADD =  $(top_builddir)/timer.o

watchdog_test_LDADD = $(top_builddir)/watchdog.o \
                      $(top_builddir)/timer.o
