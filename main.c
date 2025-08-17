// SPDX-License-Identifier: GPL-2.0-only
/*
 * main.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>

#include "usbtree.h"


static int netlink_fd;
static struct timespec last_refresh_time = {0, 0};

static int should_refresh(void)
{
	struct timespec now;
	
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 1;
	
	if (last_refresh_time.tv_sec == 0) {
		last_refresh_time = now;
		return 1;
	}
	
	double elapsed = (now.tv_sec - last_refresh_time.tv_sec) + 
	                (now.tv_nsec - last_refresh_time.tv_nsec) / 1e9;
	
	if (elapsed < 0.2)
		return 0;
	
	last_refresh_time = now;
	return 1;
}

static gboolean netlink_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	char buf[4096];
	ssize_t len;
	int need_refresh = 0;
	
	len = recv(netlink_fd, buf, sizeof(buf) - 1, 0);
	if (len > 0) {
		buf[len] = '\0';
		
		if (strstr(buf, "usb") || strstr(buf, "USB")) {
			char *action = NULL;
			char *subsystem = NULL;
			char *devpath = NULL;
			
			char *line = buf;
			while (*line) {
				if (strncmp(line, "ACTION=", 7) == 0) {
					action = line + 7;
				} else if (strncmp(line, "SUBSYSTEM=", 10) == 0) {
					subsystem = line + 10;
				} else if (strncmp(line, "DEVPATH=", 8) == 0) {
					devpath = line + 8;
				}
				
				while (*line && *line != '\0') line++;
				if (*line == '\0') line++;
			}
			
			if (action && (strcmp(action, "add") == 0 || strcmp(action, "remove") == 0)) {
				// printf("USB device %s: %s\n", action, devpath ? devpath : "unknown");
				need_refresh = 1;
			}
		}

		if (need_refresh) {
			if (should_refresh()) {
				LoadUSBTree(666);
			}
		}
	}
	
	return TRUE;
}

static void init_netlink(void)
{
	struct sockaddr_nl addr;
	
	printf("init netlink USB monitoring...\n");
	
	netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
	if (netlink_fd < 0) {
		perror("netlink socket");
		return;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = 1;
	
	if (bind(netlink_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("netlink bind");
		close(netlink_fd);
		netlink_fd = -1;
		return;
	}
		
	GIOChannel *ch = g_io_channel_unix_new(netlink_fd);
	g_io_add_watch(ch, G_IO_IN, netlink_cb, NULL);
	g_io_channel_unref(ch);
}


int main (int argc, char *argv[])
{
	GtkWidget *window1;

	gtk_init (&argc, &argv);

	initialize_stuff();

	init_netlink();

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

