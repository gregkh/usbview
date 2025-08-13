// SPDX-License-Identifier: GPL-2.0-only
/*
 * main.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdlib.h>

#include <gtk/gtk.h>

#include "usbtree.h"

int main (int argc, char *argv[])
{
	GtkWidget *window1;

	gtk_init ();

	initialize_stuff();

	/*
	 * The following code was added by Glade to create one of each component
	 * (except popup menus), just so that you see something after building
	 * the project. Delete any components that you don't want shown initially.
	 */
	window1 = create_windowMain ();
	gtk_widget_set_visible (window1, TRUE);

	LoadUSBTree(0);
	
	/* Create and run main loop */
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);
	g_main_loop_unref (mainloop);
	
	return 0;
}

