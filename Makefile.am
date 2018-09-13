sbin_PROGRAMS = phosphor-watchdog

noinst_HEADERS = watchdog.hpp

phosphor_watchdog_SOURCES =  \
                watchdog.cpp \
                mainapp.cpp

phosphor_watchdog_LDFLAGS = ${PHOSPHOR_LOGGING_LIBS} \
                            ${SDBUSPLUS_LIBS} \
                            ${SDEVENTPLUS_LIBS} \
                            ${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_watchdog_CXXFLAGS = ${PHOSPHOR_LOGGING_CFLAGS} \
                             ${SDBUSPLUS_CFLAGS} \
                             ${SDEVENTPLUS_CFLAGS} \
                             ${PHOSPHOR_DBUS_INTERFACES_CFLAGS}
SUBDIRS = test
