/*************************************************************************
** interface.c for USBView - a USB device viewer
** Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
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


#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include "usbtree.h"

GtkWidget *treeUSB;
GtkWidget *textDescription;

int timer;

GtkWidget*
create_windowMain ()
{
	GtkWidget *windowMain;
	GtkWidget *vbox1;
	GtkWidget *hpaned1;
	GtkWidget *scrolledwindow1;
	GtkWidget *hbuttonbox1;
	GtkWidget *buttonRefresh;
	GtkWidget *buttonConfigure;
	GtkWidget *buttonClose;
	GtkWidget *buttonAbout;

	windowMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (windowMain, "windowMain");
	gtk_object_set_data (GTK_OBJECT (windowMain), "windowMain", windowMain);
	gtk_window_set_title (GTK_WINDOW (windowMain), "USB Viewer");
	gtk_window_set_default_size (GTK_WINDOW (windowMain), 500, 300);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (vbox1, "vbox1");
	gtk_widget_ref (vbox1);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "vbox1", vbox1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (windowMain), vbox1);

	hpaned1 = gtk_hpaned_new ();
	gtk_widget_set_name (hpaned1, "hpaned1");
	gtk_widget_ref (hpaned1);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "hpaned1", hpaned1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (hpaned1);
	gtk_box_pack_start (GTK_BOX (vbox1), hpaned1, TRUE, TRUE, 0);
	gtk_paned_set_position (GTK_PANED (hpaned1), 200);

	treeUSB = gtk_ctree_new_with_titles (1, 0, NULL);
	gtk_widget_set_name (treeUSB, "treeUSB");
	gtk_widget_ref (treeUSB);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "treeUSB", treeUSB,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (treeUSB);
	gtk_container_add (GTK_CONTAINER (hpaned1), treeUSB);
	gtk_widget_set_usize (treeUSB, 200, -2);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
	gtk_widget_ref (scrolledwindow1);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "scrolledwindow1", scrolledwindow1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow1);
	gtk_container_add (GTK_CONTAINER (hpaned1), scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	textDescription = gtk_text_new (NULL, NULL);
	gtk_widget_set_name (textDescription, "textDescription");
	gtk_widget_ref (textDescription);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "textDescription", textDescription,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (textDescription);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), textDescription);

	hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
	gtk_widget_ref (hbuttonbox1);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "hbuttonbox1", hbuttonbox1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 5);
	//gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 10);
	//gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 50, 25);
	//gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 25, 10);

	buttonRefresh = gtk_button_new_with_label ("Refresh");
	gtk_widget_set_name (buttonRefresh, "buttonRefresh");
	gtk_widget_ref (buttonRefresh);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "buttonRefresh", buttonRefresh,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (buttonRefresh);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonRefresh);
	gtk_container_set_border_width (GTK_CONTAINER (buttonRefresh), 4);
	GTK_WIDGET_SET_FLAGS (buttonRefresh, GTK_CAN_DEFAULT);

	buttonConfigure = gtk_button_new_with_label ("Configure");
	gtk_widget_set_name (buttonConfigure, "buttonConfigure");
	gtk_widget_ref (buttonConfigure);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "buttonConfigure", buttonConfigure,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (buttonConfigure);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonConfigure);
	gtk_container_set_border_width (GTK_CONTAINER (buttonConfigure), 4);
	GTK_WIDGET_SET_FLAGS (buttonConfigure, GTK_CAN_DEFAULT);

	buttonAbout = gtk_button_new_with_label ("About");
	gtk_widget_set_name (buttonAbout, "buttonAbout");
	gtk_widget_ref (buttonAbout);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "buttonAbout", buttonAbout,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (buttonAbout);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonAbout);
	gtk_container_set_border_width (GTK_CONTAINER (buttonAbout), 4);
	GTK_WIDGET_SET_FLAGS (buttonAbout, GTK_CAN_DEFAULT);

	buttonClose = gtk_button_new_with_label ("Close");
	gtk_widget_set_name (buttonClose, "buttonClose");
	gtk_widget_ref (buttonClose);
	gtk_object_set_data_full (GTK_OBJECT (windowMain), "buttonClose", buttonClose,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (buttonClose);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonClose);
	gtk_container_set_border_width (GTK_CONTAINER (buttonClose), 4);
	GTK_WIDGET_SET_FLAGS (buttonClose, GTK_CAN_DEFAULT);

	gtk_signal_connect (GTK_OBJECT (windowMain), "delete_event",
			    GTK_SIGNAL_FUNC (on_window1_delete_event),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (buttonRefresh), "clicked",
			    GTK_SIGNAL_FUNC (on_buttonRefresh_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (buttonConfigure), "clicked",
			    GTK_SIGNAL_FUNC (on_buttonConfigure_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (buttonAbout), "clicked",
			    GTK_SIGNAL_FUNC (on_buttonAbout_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (buttonClose), "clicked",
			    GTK_SIGNAL_FUNC (on_buttonClose_clicked),
			    NULL);

	/* create our timer */
	timer = gtk_timeout_add (2000, on_timer_timeout, 0);
	
	return windowMain;
}

