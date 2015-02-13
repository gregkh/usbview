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
	usbparse.c usbparse.h	\
	configure-dialog.c      \
	usbview_logo.xpm

interface.o: $(src_icon_bitmap)

EXTRA_DIST = $(man_MANS) usbview_icon.svg usbview.spec usbview_logo.xcf

desktopdir = $(datadir)/applications
if DESKTOP
desktop_DATA = usbview.desktop
endif

icondir = $(datadir)/icons

icon_bitmaps = \
       hicolor/16x16/apps/usbview.png \
       hicolor/22x22/apps/usbview.png \
       hicolor/32x32/apps/usbview.png \
       hicolor/48x48/apps/usbview.png \
       hicolor/64x64/apps/usbview.png \
       hicolor/256x256/apps/usbview.png

src_icon_bitmap = hicolor/64x64/apps/usbview_icon.xpm

if ICONS
nobase_icon_DATA = $(icon_scalable) $(icon_bitmaps)
endif

$(icon_bitmaps) $(src_icon_bitmap): usbview_icon.svg
	mkdir -p $$(dirname $@)
	convert -geometry $$(basename $$(dirname $$(dirname $@))) $< $@

icon_scalable = hicolor/scalable/apps/usbview.svg

$(icon_scalable): usbview_icon.svg
	mkdir -p $$(dirname $@)
	cp $< $@

CLEANFILES = $(icon_scalable) $(icon_bitmaps) $(src_icon_bitmap)

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
