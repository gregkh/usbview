## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

VERSION = 1.0

bin_PROGRAMS = usbview

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
	usbview.spec

usbview_LDADD = @GTK_LIBS@ 

