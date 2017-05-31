sbin_PROGRAMS = phosphor-watchdog

noinst_HEADERS = timer.hpp \
                 watchdog.hpp

phosphor_watchdog_SOURCES =  \
                argument.cpp \
                timer.cpp    \
                watchdog.cpp \
                mainapp.cpp  \
                xyz/openbmc_project/Watchdog/Timer/error.cpp

BUILT_SOURCES =  xyz/openbmc_project/Watchdog/Timer/error.hpp \
                 xyz/openbmc_project/Watchdog/Timer/error.cpp

CLEANFILES = ${BUILT_SOURCES}

phosphor_watchdog_LDFLAGS = $(SYSTEMD_LIBS) \
                            ${PHOSPHOR_LOGGING_LIBS} \
                            ${SDBUSPLUS_LIBS} \
                            ${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_watchdog_CXXFLAGS = $(SYSTEMD_CFLAGS)\
                             ${PHOSPHOR_LOGGING_CFLAGS} \
                             ${SDBUSPLUS_CFLAGS} \
                             ${PHOSPHOR_DBUS_INTERFACES_CFLAGS}

xyz/openbmc_project/Watchdog/Timer/error.hpp: ${top_srcdir}/xyz/openbmc_project/Watchdog/Timer.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-header xyz.openbmc_project.Watchdog.Timer > $@

xyz/openbmc_project/Watchdog/Timer/error.cpp: ${top_srcdir}/xyz/openbmc_project/Watchdog/Timer.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-cpp xyz.openbmc_project.Watchdog.Timer > $@

SUBDIRS = test
