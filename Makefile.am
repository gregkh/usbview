## Process this file with automake to produce Makefile.in

INCLUDES = \
	$(GTK_CFLAGS)	\
	$(GLIB_CFLAGS)	\
	@CPPFLAGS@

VERSION = 1.1

bin_PROGRAMS = usbview

man_MANS = usbview.8

usbview_SOURCES =		\
	main.c			\
	interface.c		\
	callbacks.c		\
	usbtree.c usbtree.h	\
	showmessage.c		\
	usbparse.c usbparse.h	\
	configure-dialog.c      \
	about-dialog.c		\
	usbview_logo.xpm	\
	usbview_logo.xcf	\
	usb_icon.xpm		\
	usbview.spec

usbview_LDADD = \
	$(GTK_LIBS)	\
	$(GLIB_LIBS)

EXTRA_DIST = $(man_MANS)
