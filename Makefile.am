# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022 Greg Kroah-Hartman <greg@kroah.com>
#
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = $(GTK_CFLAGS)
usbview_LDADD = $(GTK_LIBS)

bin_PROGRAMS = usbview

man_MANS = usbview.8

usbview_SOURCES =		\
	main.c			\
	interface.c		\
	callbacks.c		\
	usbtree.c usbtree.h	\
	sysfs.c sysfs.h		\
	ccan/check_type/check_type.h	\
	ccan/str/str.h			\
	ccan/str/str_debug.h		\
	ccan/config.h			\
	ccan/container_of/container_of.h\
	ccan/list/list.h		\
	usbview_logo.xpm

interface.o: $(icon_bitmaps_xpm)

EXTRA_DIST = $(man_MANS) usbview_icon.svg usbview.desktop	\
	usbview_logo.xcf				\
	com.kroah.usbview.metainfo.xml			\
	LICENSES/GPL-2.0-only.txt

desktopdir = $(datadir)/applications
metainfodir = $(datadir)/metainfo
if DESKTOP
desktop_DATA = usbview.desktop
metainfo_DATA = com.kroah.usbview.metainfo.xml
endif

icondir = $(datadir)/icons

icon_bitmaps_png = \
       hicolor/16x16/apps/usbview.png \
       hicolor/22x22/apps/usbview.png \
       hicolor/32x32/apps/usbview.png \
       hicolor/48x48/apps/usbview.png \
       hicolor/64x64/apps/usbview.png \
       hicolor/256x256/apps/usbview.png

icon_bitmaps_xpm = hicolor/64x64/apps/usbview_icon.xpm

if ICONS
nobase_icon_DATA = $(icon_scalable) $(icon_bitmaps_png)
endif

# Rasterize the SVG with rsvg-convert when available.  ImageMagick may
# delegate SVG rendering to inkscape, which crashes (Gio::DBus::Error)
# when several instances are started at once by a parallel make.
$(icon_bitmaps_png): usbview_icon.svg
	mkdir -p $$(dirname $@)
if HAVE_RSVG_CONVERT
	size=$$(basename $$(dirname $$(dirname $@))); \
	$(RSVG_CONVERT) --width=$${size%x*} --height=$${size#*x} --keep-aspect-ratio --output=$@ $<
else
if HAVE_CONVERT
# Work around the inkscape crash above, in case ImageMagick hands the SVG
# to inkscape: SELF_CALL makes inkscape register as a non-unique
# GApplication, so concurrent instances do not race on D-Bus.  See
# https://gitlab.com/inkscape/inkscape/-/work_items/4716
	SELF_CALL=1 $(CONVERT) -background none -density 300x300 -geometry $$(basename $$(dirname $$(dirname $@))) -density 96x96 $< $@
else
	echo "error: unable to generate $@ from $<"
	exit 1
endif
endif

# The XPM is converted from the PNG of the same size, so ImageMagick
# never has to render SVG itself.
$(icon_bitmaps_xpm): hicolor/64x64/apps/usbview.png
	mkdir -p $$(dirname $@)
if HAVE_CONVERT
	$(CONVERT) $< $@
else
	echo "error: unable to generate $@ from $<"
	exit 1
endif

icon_scalable = hicolor/scalable/apps/usbview.svg

$(icon_scalable): usbview_icon.svg
	mkdir -p $$(dirname $@)
	cp $< $@

CLEANFILES = $(icon_scalable) $(icon_bitmaps_png) $(icon_bitmaps_xpm)

# gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor; gtk-update-icon-cache -f -t $(datadir)/icons/HighContrast
#
# install-data-hook: update-icon-cache
# uninstall-hook: update-icon-cache
# update-icon-cache:
#	@-if test -z "$(DESTDIR)"; then \
#		echo "Updating Gtk icon cache."; \
#		$(gtk_update_icon_cache); \
#	else \
#		echo "*** Icon cache not updated.  After (un)install, run this:"; \
#		echo "***   $(gtk_update_icon_cache)"; \
#	fi
