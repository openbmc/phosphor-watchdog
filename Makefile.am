sbin_PROGRAMS = phosphor-watchdog

phosphor_watchdog_SOURCES = \
                argument.cpp \
                timer.cpp \
                mainapp.cpp

phosphor_watchdog_LDFLAGS = $(SYSTEMD_LIBS) \
                            ${PHOSPHOR_LOGGING_LIBS}
phosphor_watchdog_CXXFLAGS = $(SYSTEMD_CFLAGS)\
                             ${PHOSPHOR_LOGGING_CFLAGS}
