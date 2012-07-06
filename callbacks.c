/*************************************************************************
** callbacks.c for USBView - a USB device viewer
** Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; version 2 of the License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
** (See the included file COPYING)
*************************************************************************/

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <gtk/gtk.h>
#include "usbtree.h"
#include "usbview_logo.xpm"	/* logo */


void on_buttonClose_clicked (GtkButton *button, gpointer user_data)
{
	gtk_main_quit();
}


gboolean on_window1_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_main_quit();

	return FALSE;
}


void on_buttonRefresh_clicked (GtkButton *button, gpointer user_data)
{
	LoadUSBTree(1);
}


void on_buttonConfigure_clicked (GtkButton *button, gpointer user_data)
{
	configure_dialog ();
}


void on_buttonAbout_clicked (GtkButton *button, gpointer user_data)
{
	GdkPixbuf *logo;
	gchar *authors[] = { "Greg Kroah-Hartman <greg@kroah.com>", NULL };

	logo = gdk_pixbuf_new_from_xpm_data ((const char **)usbview_logo_xpm);
	gtk_show_about_dialog (GTK_WINDOW (windowMain),
		"logo", logo,
		"program-name", "usbview",
		"version", VERSION,
		"comments", "Display information on USB devices",
		"website-label", "http://www.kroah.com/linux-usb/",
		"website", "http://www.kroah.com/linux-usb/",
		"copyright", "Copyright © 1999-2012",
		"authors", authors,
		NULL);
	g_object_unref (logo);
}


gint on_timer_timeout (gpointer user_data)
{
	LoadUSBTree(0);
	return 1;
}

