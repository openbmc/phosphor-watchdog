A implementation of interface `phosphor-dbus-interface/xyz/openbmc_project/State/Watchdog.interface.yaml`.
Currently support *host* watchdog monitoring Host booting.

* To invoke:
`phosphor-watchdog host`

* To start the timer (if a timer has already been enabled, need to call "Reset()" to re-start it)
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host org.freedesktop.DBus.Properties Set ssv xyz.openbmc_project.State.Watchdog Enabled b 1
```

* To stop the timer:
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host org.freedesktop.DBus.Properties Set ssv xyz.openbmc_project.State.Watchdog Enabled b 0
```

* To reset (re-arm) the timer to default timeout interval  (so as to prevent it from firing)
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host  xyz.openbmc_project.State.Watchdog Reset
```

* To set the time out interval (to 3000 milisecond)
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host org.freedesktop.DBus.Properties Set ssv xyz.openbmc_project.State.Watchdog Interval 30000
```

* To get the interval
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host org.freedesktop.DBus.Properties Get ss xyz.openbmc_project.State.Watchdog Interval
```

* To get the time remaining for next time out
```
busctl call xyz.openbmc_project.State.Watchdog.Host  /xyz/openbmc_project/State/Watchdog/Host org.freedesktop.DBus.Properties Get ss xyz.openbmc_project.State.Watchdog TimeRemaining
```

* When time out, a dbus signal named *Timeout* will be triggered
