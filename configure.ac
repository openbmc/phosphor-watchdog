# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-watchdog], [1.0],
        [https://github.com/openbmc/phosphor-watchdog/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# For linking
LT_INIT

# Check for needed modules
PKG_CHECK_MODULES(
    [PHOSPHOR_LOGGING],
    [phosphor-logging],
    [],
    [AC_MSG_ERROR([Could not find required openbmc/phosphor-logging])]
)
PKG_CHECK_MODULES(
    [SDBUSPLUS],
    [sdbusplus],
    [],
    [AC_MSG_ERROR([Could not find required openbmc/sdbusplus])]
)
PKG_CHECK_MODULES(
    [SDEVENTPLUS],
    [sdeventplus],
    [],
    [AC_MSG_ERROR([Could not find required openbmc/sdeventplus])]
)
PKG_CHECK_MODULES(
    [PHOSPHOR_DBUS_INTERFACES],
    [phosphor-dbus-interfaces],
    [],
    [AC_MSG_ERROR([Could not find required openbmc/phosphor-dbus-interfaces])]
)
PKG_CHECK_MODULES([GTEST_MAIN], [gtest_main], [],
    [
        AX_PTHREAD(
            [GTEST_MAIN_CFLAGS="-DGTEST_HAS_PTHREAD=1"],
            [GTEST_MAIN_CFLAGS="-DGTEST_HAS_PTHREAD=0"]
        )
        [GTEST_MAIN_LIBS="-lgtest_main -lgtest"]
        AC_SUBST(GTEST_MAIN_CFLAGS)
        AC_SUBST(GTEST_MAIN_LIBS)
    ])

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING(
        [--enable-oe-sdk],
        [Link testcases absolutely against OE SDK so they can be ran within it.]
    )
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT]
)
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
