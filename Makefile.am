sbin_PROGRAMS = phosphor-watchdog
phosphor_watchdog_SOURCES = \
	xyz.openbmc_project.State.Watchdog.cpp \
	watchdog.cpp \
	watchdog_main.cpp

# systemd required for journal interfaces
phosphor_watchdog_LDFLAGS = $(SYSTEMD_LIBS)

SUBDIRS = test
