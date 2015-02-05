dnl Process this file with autoconf to produce a configure script.

AC_INIT([USBView],[2.0],[Greg Kroah-Hartman <greg@kroah.com>],[usbview],[http://www.kroah.com/linux-usb/])
AC_CONFIG_SRCDIR([usbtree.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

# required versions of other packages
#m4_define([gtk_required_version], [2.2.0])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_HEADER_STDC

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.0)
AC_SUBST(GTK_FLAGS)
AC_SUBST(GTK_LIBS)

AX_CFLAGS_WARN_ALL

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT

