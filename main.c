// SPDX-License-Identifier: GPL-2.0-only
/*
 * main.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>

#include "usbtree.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

/*
 * Refresh the tree automatically when USB devices come and go, by
 * listening for kernel uevents on a netlink socket.  (sysfs does not
 * support inotify, so watching /sys does not work.)
 *
 * Plugging in one device produces a burst of uevents (the device, its
 * interfaces, driver binding), so rather than refreshing on the first
 * one, wait until the bus has been quiet for a moment.
 */
#define HOTPLUG_SETTLE_MS	250

static guint hotplug_refresh_source;

static gboolean hotplug_refresh(gpointer data)
{
	hotplug_refresh_source = 0;
	LoadUSBTree(1);
	return G_SOURCE_REMOVE;
}

static void hotplug_schedule_refresh(void)
{
	if (hotplug_refresh_source)
		g_source_remove(hotplug_refresh_source);
	hotplug_refresh_source = g_timeout_add(HOTPLUG_SETTLE_MS,
					       hotplug_refresh, NULL);
}

static gboolean hotplug_event(GIOChannel *source, GIOCondition condition,
			      gpointer data)
{
	int fd = g_io_channel_unix_get_fd(source);
	char buf[8192];
	ssize_t len;

	if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return G_SOURCE_REMOVE;

	while ((len = recv(fd, buf, sizeof(buf) - 1, MSG_DONTWAIT)) != 0) {
		if (len < 0) {
			if (errno == EINTR)
				continue;
			/* receive buffer overflowed: events were lost */
			if (errno == ENOBUFS)
				hotplug_schedule_refresh();
			break;
		}
		buf[len] = '\0';

		/* "action@devpath\0KEY=value\0KEY=value\0..." */
		for (char *p = buf; p < buf + len; p += strlen(p) + 1) {
			if (strcmp(p, "SUBSYSTEM=usb") == 0) {
				hotplug_schedule_refresh();
				break;
			}
		}
	}

	return G_SOURCE_CONTINUE;
}

static void hotplug_init(void)
{
	struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK,
		.nl_pid = 0,		/* let the kernel assign it */
		.nl_groups = 1,		/* kernel uevents */
	};
	GIOChannel *channel;
	int fd;

	fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
		    NETLINK_KOBJECT_UEVENT);
	if (fd < 0)
		return;		/* no automatic refresh; not fatal */

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		return;
	}

	channel = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_add_watch(channel, G_IO_IN | G_IO_ERR | G_IO_HUP, hotplug_event,
		       NULL);
	g_io_channel_unref(channel);
}
#else
static void hotplug_init(void)
{
}
#endif

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
	hotplug_init();
	gtk_main ();
	return 0;
}

