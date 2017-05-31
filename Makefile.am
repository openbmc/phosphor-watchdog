sbin_PROGRAMS = phosphor-watchdog

phosphor_watchdog_SOURCES = \
                argument.cpp \
                timer.cpp \
				watchdog.cpp \
                mainapp.cpp

phosphor_watchdog_LDFLAGS = $(SYSTEMD_LIBS) \
                            ${PHOSPHOR_LOGGING_LIBS} \
                            ${SDBUSPLUS_LIBS} \
                            ${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_watchdog_CXXFLAGS = $(SYSTEMD_CFLAGS)\
                             ${PHOSPHOR_LOGGING_CFLAGS} \
                             ${SDBUSPLUS_CFLAGS} \
                             ${PHOSPHOR_DBUS_INTERFACES_CFLAGS}

SUBDIRS = test
