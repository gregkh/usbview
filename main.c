// SPDX-License-Identifier: GPL-2.0-only
/*
 * main.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <gtk/gtk.h>

#include "usbtree.h"

int main (int argc, char *argv[])
{
	GtkWidget *window1;

	gtk_init (&argc, &argv);

	initialize_stuff();

	/*
	 * The following code was added by Glade to create one of each component
	 * (except popup menus), just so that you see something after building
	 * the project. Delete any components that you don't want shown initially.
	 */
	window1 = create_windowMain ();
	gtk_widget_show (window1);

	LoadUSBTree(0);
	gtk_main ();
	return 0;
}

