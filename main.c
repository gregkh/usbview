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

#include <libudev.h>
#include <glib-unix.h>

static struct udev *udev = NULL;
static struct udev_monitor *mon = NULL;
static guint udev_source_id = 0;

static gboolean udev_mon_cb(gint fd, GIOCondition condition, gpointer user_data)
{
    struct udev_device *dev;
    struct udev_device *parent_dev;
    const char *action;
    const char *dev_id_str;

    dev = udev_monitor_receive_device(mon);
    if (!dev)
    {
        g_warning("failed to receive device from udev monitor");
        return TRUE;
    }

    action = udev_device_get_action(dev);
    if (action)
    {
        parent_dev = udev_device_get_parent(dev);
        dev_id_str = udev_device_get_property_value(dev, "ID_MODEL");

        const char *devtype = udev_device_get_devtype(dev);
        if (devtype && strcmp(devtype, "usb_device") != 0)
        {
            udev_device_unref(dev);
            return TRUE;
        }

        if (strncmp(action, "remove", 7) == 0)
        {
            g_message("a device was removed (from %s)", udev_device_get_property_value(parent_dev, "ID_MODEL"));
        }
        else if (strncmp(action, "add", 3) == 0)
        {
            g_message("add a device: > %s < (from %s)",
                      dev_id_str ? dev_id_str : "unknown",
                      udev_device_get_property_value(parent_dev, "ID_MODEL"));
        }
        else
        {
            g_debug("udev action: %s for device: %s (%s)",
                      action,
                      udev_device_get_property_value(dev, "ID_MODEL"),
                      parent_dev ? udev_device_get_property_value(parent_dev, "ID_MODEL") : "");
        }

        LoadUSBTree(666);
    }

    udev_device_unref(dev);
    return TRUE;
}

static void init_udev_mon(void)
{
    udev = udev_new();
    if (!udev)
    {
        g_warning("failed to create udev context");
        return;
    }

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon)
    {
        g_warning("failed to create udev monitor");
        udev_unref(udev);
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
    udev_monitor_enable_receiving(mon);

    udev_source_id = g_unix_fd_add(udev_monitor_get_fd(mon), G_IO_IN | G_IO_ERR | G_IO_HUP,
                                   udev_mon_cb, NULL);
}

int main(int argc, char *argv[])
{
    GtkWidget *window1;

    gtk_init(&argc, &argv);
    // Initialize udev monitor
    init_udev_mon();

    initialize_stuff();

    /*
     * The following code was added by Glade to create one of each component
     * (except popup menus), just so that you see something after building
     * the project. Delete any components that you don't want shown initially.
     */
    window1 = create_windowMain();
    gtk_widget_show(window1);

    LoadUSBTree(0);
    gtk_main();
    return 0;
}
