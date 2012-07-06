/*************************************************************************
** configure-dialog.c for USBView - a USB device viewer
** Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; version 2 of the License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
** (See the included file COPYING)
*************************************************************************/


#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include "usbtree.h"
#include "usbparse.h"

static	GtkWidget	*fileEntry;

static void fileSelectButtonClick (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new (
				"locate usbdevfs devices file",
				GTK_WINDOW (windowMain),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text (GTK_ENTRY (fileEntry), filename);
		g_free (filename);
	}
			  
	gtk_widget_destroy (dialog);
}

void configure_dialog (void)
{
	GtkWidget *dialog, *content_area;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *fileSelectButton;
	gchar *editString;
	gint result;

	dialog = gtk_dialog_new_with_buttons (
				"USB View Configuration",
				GTK_WINDOW (windowMain),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add (GTK_CONTAINER (content_area), hbox1);

	label1 = gtk_label_new ("Location of usbdevfs devices file");
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 5);

	fileEntry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (fileEntry), devicesFile);
	gtk_box_pack_start (GTK_BOX (hbox1), fileEntry, TRUE, TRUE, 0);

	fileSelectButton = gtk_button_new_with_label ("...");
	g_signal_connect (G_OBJECT (fileSelectButton), "clicked",
			  G_CALLBACK (fileSelectButtonClick), NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), fileSelectButton, TRUE, FALSE, 1);

	gtk_widget_show_all (dialog);
	result = gtk_dialog_run(GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		editString = gtk_editable_get_chars (
					GTK_EDITABLE (fileEntry), 0, -1);
		strcpy (devicesFile, editString);
		g_free (editString);
		LoadUSBTree (0);
	}
	gtk_widget_destroy (dialog);
}
