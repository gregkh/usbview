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

extern gchar		devicesFile[1000];
extern GtkTreeStore	*treeStore;
extern GtkWidget	*treeUSB;
extern GtkWidget	*textDescriptionView;
extern GtkTextBuffer	*textDescriptionBuffer;
extern GtkWidget	*windowMain;

extern void	LoadUSBTree		(int refresh);
extern void	initialize_stuff	(void);
extern GtkWidget * create_windowMain	(void);
extern void	configure_dialog	(void);

extern void	on_buttonClose_clicked		(GtkButton *button, gpointer user_data);
extern gboolean	on_window1_delete_event		(GtkWidget *widget, GdkEvent *event, gpointer user_data);
extern void	on_buttonRefresh_clicked	(GtkButton *button, gpointer user_data);
extern void	on_buttonConfigure_clicked	(GtkButton *button, gpointer user_data);
extern void	on_buttonAbout_clicked		(GtkButton *button, gpointer user_data);
extern gint	on_timer_timeout		(gpointer user_data);

#endif	/* __USB_TREE_H */
