dnl Process this file with autoconf to produce a configure script.

AC_INIT([USBView],[2.0],[Greg Kroah-Hartman <greg@kroah.com>],[usbview],[http://www.kroah.com/linux-usb/])
AC_CONFIG_SRCDIR([usbtree.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

AC_CHECK_PROG([have_convert],[convert],[yes],[no])

AC_MSG_CHECKING([whether to try to install icons])
AC_ARG_ENABLE(icons,
	[AS_HELP_STRING([--enable-icons],[try to install icons (default=yes)])],
	[icons=$enableval],
	[icons=yes])
AC_MSG_RESULT([$icons])

AC_MSG_CHECKING([whether to try to install desktop file])
AC_ARG_ENABLE(desktop,
	[AS_HELP_STRING([--enable-desktop],[try to install desktop file (default=yes)])],
	[desktop=$enableval],
	[desktop=yes])
AC_MSG_RESULT([$desktop])

AS_IF([test "$have_convert" = "no" && "$icons" = "yes"],
	[AC_MSG_WARN([convert not available, disabling icon installation])
	 icons=no])

AM_CONDITIONAL(HAVE_CONVERT,[test "$have_convert" = "yes"])
AM_CONDITIONAL(DESKTOP,[test x${desktop} = xyes])
AM_CONDITIONAL(ICONS,[test x${icons} = xyes])

AC_PROG_CC

AC_HEADER_STDC

AC_SEARCH_LIBS([strerror],[cposix])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])
AC_SUBST([GTK_FLAGS])
AC_SUBST([GTK_LIBS])

AX_CFLAGS_WARN_ALL

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT

