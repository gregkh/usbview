/*************************************************************************
** about-dialog.c for USBView - a USB device viewer
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

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "usbtree.h"
#include "usbparse.h"

#include "usbview_logo.xpm"	/* logo */


static void OkAboutDialog (GtkWidget *widget, gpointer data)
{
	GtkWidget       *dialogWidget = (GtkWidget *) data;
	
	gtk_grab_remove (dialogWidget);

	/* --- Close the widget --- */
	gtk_widget_destroy (dialogWidget);
}


void about_dialog (void)
{
	GtkWidget *aboutDialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *label1;
	GtkWidget *dialog_action_area1;
	GtkWidget *okButton;
	static GdkPixmap *logo;
        GdkBitmap *logoMask;
	GtkWidget *logoWidget;
	aboutDialog = gtk_dialog_new ();
	gtk_object_set_data (GTK_OBJECT (aboutDialog), "aboutDialog", aboutDialog);
	gtk_window_set_title (GTK_WINDOW (aboutDialog), "About usbview");
	gtk_window_set_policy (GTK_WINDOW (aboutDialog), TRUE, TRUE, FALSE);
	
	dialog_vbox1 = GTK_DIALOG (aboutDialog)->vbox;
	gtk_object_set_data (GTK_OBJECT (aboutDialog), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	gtk_widget_realize(aboutDialog);
	logo = gdk_pixmap_create_from_xpm_d (aboutDialog->window, &logoMask, NULL, usbview_logo_xpm);
	logoWidget = gtk_pixmap_new (logo, logoMask);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), logoWidget, FALSE, FALSE, 0);
	gtk_widget_show (logoWidget);
	
	label1 = gtk_label_new (//"\n"
				//"  USBView   Version " VERSION "\n\n"
				//"  Copyright (C) 1999, 2000\n"
				//"  Greg Kroah-Hartman <greg@kroah.com>  \n\n"
				"http://usbview.sourceforge.net/");
	gtk_widget_ref (label1);
	gtk_object_set_data_full (GTK_OBJECT (aboutDialog), "label1", label1,
			    (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), label1, FALSE, FALSE, 0);
//	gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
	
	dialog_action_area1 = GTK_DIALOG (aboutDialog)->action_area;
	gtk_object_set_data (GTK_OBJECT (aboutDialog), "dialog_action_area1", dialog_action_area1);
	gtk_widget_show (dialog_action_area1);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);
	
	okButton = gtk_button_new_with_label ("  Ok  ");
	gtk_widget_ref (okButton);
	gtk_object_set_data_full (GTK_OBJECT (aboutDialog), "okButton", okButton,
			    (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (okButton);
	gtk_box_pack_start (GTK_BOX (dialog_action_area1), okButton, FALSE, FALSE, 10);
	
	gtk_signal_connect (GTK_OBJECT (okButton), "clicked", GTK_SIGNAL_FUNC (OkAboutDialog), aboutDialog);
	
	/* --- Default the "Ok" button --- */
	GTK_WIDGET_SET_FLAGS (okButton, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (okButton);

	/* --- Show the dialog --- */
	gtk_widget_show (aboutDialog);

	/* --- Only this window can have actions done. --- */
	gtk_grab_add (aboutDialog);

	return;
}

