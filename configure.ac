#											-*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([lbs-server], [0.1])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([lbs-server/lbs_server.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

###### Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_RANLIB

# PLATFORM configuration
PLATFORM_INIT

# Add default build options to CFLAGS, LDFLAGS
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -Werror"
	LDFLAGS="$LDFLAGS -Wl,-z,defs -Wl,--as-needed -Wl,--hash-style=both"
fi

# Add -g option to CFLAGS
AC_ARG_ENABLE([debug],
	[AC_HELP_STRING([--enable-debug],[turn on debugging [default=no]])],
	[case "${enableval}" in
	 yes) enable_dbg=yes ;;
	 no) enable_dbg=no ;;
	 *) AC_MSG_ERROR([Bad value ${enableval} for --enable-debug]) ;;
	 esac],[enable_dbg=no])
if ([test "x$enable_dbg" = xyes]); then
	CFLAGS="$CFLAGS -g"
fi

# Check GCC ELF visibility
AC_MSG_CHECKING(for ELF visibility)
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[
		__attribute__((visibility("default")))
		int var=10;
	]])],
	[has_visibility=yes
	AC_DEFINE([EXPORT_API], [__attribute__((visibility("default")))], [Symbol visibility prefix])
	CFLAGS="$CFLAGS -fvisibility=hidden"],
	[has_visibility=no
	AC_DEFINE([EXPORT_API], [], [Symbol visibility prefix])	]
)
AC_MSG_RESULT($has_visibility)

# Check required libraries
#PKG_CHECK_MODULES(PROVIDERS, [glib-2.0 network tapi vconf vconf-internal-keys msg-service gthread-2.0 dlog gio-2.0 gio-unix-2.0 lbs-dbus capi-network-wifi journal libresourced capi-appfw-app-manager deviced])
PKG_CHECK_MODULES(PROVIDERS, [glib-2.0 network tapi vconf vconf-internal-keys gthread-2.0 dlog gio-2.0 gio-unix-2.0 lbs-dbus capi-network-wifi])
AC_SUBST(PROVIDERS_CFLAGS)
AC_SUBST(PROVIDERS_LIBS)

PKG_CHECK_MODULES(MODULE, [glib-2.0 gmodule-2.0 location dlog gio-2.0 lbs-dbus])
AC_SUBST(MODULE_CFLAGS)
AC_SUBST(MODULE_LIBS)

AC_HAVE_LIBRARY(pthread, [PTHREAD_LIBS=-lpthread])

# GPS configuration
AC_ARG_ENABLE([gps],
		[AC_HELP_STRING([--enable-gps],[enable GPS [default=no]])],
		[case "${enableval}" in
		 yes) gps=yes ;;
		 no) gps=no ;;
		 *) AC_MSG_ERROR([Bad value ${enableval} for --enable-gps]) ;;
		 esac],[gps=no])
AM_CONDITIONAL([GPS], [test "x$gps" = xyes])
AC_MSG_CHECKING([whether host GPS])
if ([test "x$gps" = xyes])
then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL([HAVE_GPS], [test "x$gps" = xyes])

# WPS configuration
AC_ARG_ENABLE([wps],
		[AC_HELP_STRING([--enable-wps],[enable WPS [default=no]])],
		[case "${enableval}" in
		 yes) wps=yes ;;
		 no) wps=no ;;
		 *) AC_MSG_ERROR([Bad value ${enableval} for --enable-wps]) ;;
		 esac],[wps=no])
AM_CONDITIONAL([WPS], [test "x$wps" = xyes])
AC_MSG_CHECKING([whether host WPS])
if ([test "x$wps" = xyes])
then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL([HAVE_WPS], [test "x$wps" = xyes])

# Check DBus configuration path
DBUS_CONF_DIR="${sysconfdir}/dbus-1/system.d"
AC_SUBST(DBUS_CONF_DIR)

# Check DBus service path
DBUS_SERVICES_DIR="/usr/share/dbus-1/services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBus is])


# Config files
AC_CONFIG_FILES([
Makefile
lbs-server/Makefile
lbs-server/lbs-server-plugin.pc
module/Makefile
])

AC_OUTPUT
