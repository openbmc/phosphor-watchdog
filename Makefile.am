AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = phosphor-watchdog

phosphor_watchdog_SOURCES = \
	watchdog.cpp \
	watchdog_main.cpp


generic_cxxflags = $(SYSTEMD_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) $(SDBUSPLUS_CFLAGS)
generic_ldflags = $(SYSTEMD_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS) $(SDBUSPLUS_LIBS)

phosphor_watchdog_CXXFLAGS = $(generic_cxxflags)
phosphor_watchdog_LDFLAGS = $(generic_ldflags)
