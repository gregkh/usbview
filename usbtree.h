// SPDX-License-Identifier: GPL-2.0-only
/*
 * usbtree.h for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, greg@kroah.com
 */
#ifndef __USB_TREE_H
#define __USB_TREE_H

enum {
	NAME_COLUMN,
	DEVICE_ADDR_COLUMN,
	COLOR_COLUMN,
	N_COLUMNS
};

extern GtkTreeStore	*treeStore;
extern GtkWidget	*treeUSB;
extern GtkWidget	*textDescriptionView;
extern GtkTextBuffer	*textDescriptionBuffer;
extern GtkWidget	*windowMain;

void LoadUSBTree(int refresh);
void initialize_stuff(void);
GtkWidget *create_windowMain(void);

void on_buttonClose_clicked(GtkButton *button, gpointer user_data);
gboolean on_window1_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_buttonRefresh_clicked(GtkButton *button, gpointer user_data);
void on_buttonAbout_clicked(GtkButton *button, gpointer user_data);
gint on_timer_timeout(gpointer user_data);

#endif	/* __USB_TREE_H */
