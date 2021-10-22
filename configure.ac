dnl Process this file with autoconf to produce a configure script.

# Prologue.

AC_PREREQ([2.69])
AC_INIT([USBView],[2.1],[Greg Kroah-Hartman <greg@kroah.com>],[usbview],[http://www.kroah.com/linux-usb/])
AC_CONFIG_SRCDIR([usbtree.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Process options.

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

AC_MSG_CHECKING([whether to try to install policykit file])
AC_ARG_ENABLE(polkit,
	[AS_HELP_STRING([--enable-policykit],[try to install policykit file (default=yes)])],
	[polkit=$enableval],
	[polkit=yes])
AC_MSG_RESULT([$polkit])

# Checks for programs.

AC_PROG_CC
AC_CHECK_PROG([have_convert],[convert],[yes],[no])

# Set automake conditionals.

AC_SUBST(CONVERT,[convert])
AM_CONDITIONAL(HAVE_CONVERT, [test "$have_convert" = "yes"])

AS_IF([test "$have_convert" = "no"],
      [AC_MSG_WARN([no bitmap conversion utility, disabling icon installation])
       icons=no])

AM_CONDITIONAL(DESKTOP,[test x${desktop} = xyes])
AM_CONDITIONAL(POLKIT,[test x${polkit} = xyes])
AM_CONDITIONAL(ICONS,[test x${icons} = xyes])

# Checks for libraries.

AC_SEARCH_LIBS([strerror],[cposix])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])
AC_SUBST([GTK_FLAGS])
AC_SUBST([GTK_LIBS])

# Checks for header files.

AC_CHECK_HEADERS([ctype.h errno.h stdio.h],,
 AC_MSG_ERROR(required system header file unavailable))

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CHECK_FUNCS([memset strstr strtol],,
 AC_MSG_ERROR(required library function unavailable))

# Epilogue.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_MSG_RESULT([
	usbview $VERSION
	===========

	prefix:                 ${prefix}
	datarootdir:            ${datarootdir}
	datadir:                ${datadir}
	mandir:                 ${mandir}

	compiler:               ${CC}
	cflags:                 ${CFLAGS}
	ldflags:                ${LDFLAGS}
])
