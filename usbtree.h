/*************************************************************************
** usbtree.h for USBView - a USB device viewer
** Copyright (c) 1999, 2000 by Greg Kroah-Hartman, greg@kroah.com
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** (See the included file COPYING)
*************************************************************************/

#ifndef __USB_TREE_H
#define __USB_TREE_H

extern gchar		devicesFile[1000];
extern GtkWidget	*treeUSB;
extern GtkWidget	*textDescription;

extern void	LoadUSBTree		(int refresh);
extern void	initialize_stuff	(void);
extern GtkWidget * create_windowMain	(void);
extern void	configure_dialog	(void);
extern void	about_dialog		(void);

extern void	on_buttonClose_clicked		(GtkButton *button, gpointer user_data);
extern gboolean	on_window1_delete_event		(GtkWidget *widget, GdkEvent *event, gpointer user_data);
extern void	on_buttonRefresh_clicked	(GtkButton *button, gpointer user_data);
extern void	on_buttonConfigure_clicked	(GtkButton *button, gpointer user_data);
extern void	on_buttonAbout_clicked		(GtkButton *button, gpointer user_data);
extern gint	on_timer_timeout		(gpointer user_data);

extern void	ShowMessage	(gchar *title, gchar *message, gboolean centered);
extern gboolean	MessageShown	(void);


#endif	/* __USB_TREE_H */
