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

#include <sys/inotify.h>

#include "usbtree.h"


static int inotify_fd;

static gboolean inotify_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	char buf[4096];
	ssize_t len;
	struct inotify_event *event;

	while ((len = read(inotify_fd, buf, sizeof(buf))) > 0) {
	}
	
	LoadUSBTree(666);

	return TRUE;
}

static void init_inotify(void) 
{
	const char *path = "/sys/bus/usb/devices";
	inotify_fd = inotify_init1(IN_NONBLOCK);
	if (inotify_fd < 0) {
		perror("inotify_init1");
		return;
	}

	if (inotify_add_watch(inotify_fd, path, IN_ALL_EVENTS) < 0) {
		perror("inotify_add_watch");
		close(inotify_fd);
		inotify_fd = -1;
		return;
	}

	GIOChannel *ch = g_io_channel_unix_new(inotify_fd);
	g_io_add_watch(ch, G_IO_IN, inotify_cb, NULL);
	g_io_channel_unref(ch);
}


int main (int argc, char *argv[])
{
	GtkWidget *window1;

	gtk_init (&argc, &argv);

	initialize_stuff();

	init_inotify();

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

