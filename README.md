An implementation of interface
`xyz.openbmc_project.state.watchdog.interface.yaml`.

* To invoke: `phosphor-watchdog <watchdog object path> <watchdog service name>`.
For example: `phosphor-watchdog /xyz/openbmc_project/state/watchdog/host0
xyz.openbmc_project.State.Watchdog.Host`.
Bellow examples use `/xyz/openbmc_project/state/watchdog/host0` as object path.

* To start the timer (use the default timeout interval):
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Set ssv xyz.openbmc_project.State.Watchdog Enabled b 1
```

* To stop the timer:
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Set ssv xyz.openbmc_project.State.Watchdog Enabled b 0
```

* To set the default time out interval (to 3000 milisecond):
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Set ssv xyz.openbmc_project.State.Watchdog Interval t 30000
```

* To get the interval:
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Get ss xyz.openbmc_project.State.Watchdog Interval
```

* To get the time remaining for next time out:
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Get ss xyz.openbmc_project.State.Watchdog TimeRemaining
```
If watchdog timer is disabled, will return 0.

* To re-arm the timer with a new time out interval
(also to prevent watchdog from firing):
```
busctl call xyz.openbmc_project.State.Watchdog.Host \
/xyz/openbmc_project/state/watchdog/host0 \
org.freedesktop.DBus.Properties \
Set ssv xyz.openbmc_project.State.Watchdog TimeRemaining t 10000
```

* When time out, a dbus signal named *Timeout* will be triggered.
