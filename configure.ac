dnl Process this file with autoconf to produce a configure script.

# Prologue.

AC_PREREQ([2.69])
AC_INIT([USBView],[2.0],[Greg Kroah-Hartman <greg@kroah.com>],[usbview],[http://www.kroah.com/linux-usb/])
AC_CONFIG_SRCDIR([usbtree.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

# Checks for programs.

AC_PROG_CC

# Checks for libraries.

AC_SEARCH_LIBS([strerror],[cposix])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])
AC_SUBST([GTK_FLAGS])
AC_SUBST([GTK_LIBS])

# Checks for header files.

AC_CHECK_HEADERS([ctype.h errno.h stdio.h],,
 AC_MSG_ERROR(required system header file unavailable))

# Checks for typedefs, structures, and compiler characteristics.

AX_CFLAGS_WARN_ALL

# Checks for library functions.

AC_CHECK_FUNCS([memset strstr strtol],,
 AC_MSG_ERROR(required library function unavailable))

# Epilogue.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
