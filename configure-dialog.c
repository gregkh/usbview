/*************************************************************************
** configure-dialog.c for USBView - a USB device viewer
** Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License.
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

#include "usbtree.h"
#include "usbparse.h"




static	GtkWidget	*fileEntry;
static	GtkWidget	*filew;
static	char		*sFilename;

/* 
 * Get the selected filename and print it to the console 
 */
static void file_ok_sel (GtkWidget *w, GtkFileSelection *fs)
{
	const char *sTempFile;

	/* --- Get the name --- */
	sTempFile = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

	/* --- Allocate space and save it. --- */
	sFilename = malloc (sizeof (char) * (strlen (sTempFile) + 1));
	strcpy (sFilename, sTempFile);

	/* --- Destroy the file selection --- */
	gtk_widget_destroy (filew);
}


static void file_cancel_sel (GtkWidget *w, GtkFileSelection *fs)
{
	/* --- Destroy the file selection --- */
	gtk_widget_destroy (filew);
}


/*
 * DestroyDialog
 *
 * Destroy the dialog (obvious, eh?) but also remove the
 * grab and close the modal. 
 */
static int DestroyDialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove (widget);
	gtk_main_quit ();
	return(FALSE);
}


/*
 * GetFilename 
 */
static char *GetFilename (char *sTitle, char *initialFilename)
{
	sFilename = NULL;

	/* --- Create a new file selection widget --- */
	filew = gtk_file_selection_new (sTitle);

	/* --- If it's destroyed --- */
	gtk_signal_connect (GTK_OBJECT (filew), "destroy", (GtkSignalFunc) DestroyDialog, &filew);

	/* --- Connect the ok_button to file_ok_sel function  --- */
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", (GtkSignalFunc) file_ok_sel, filew );

	/* --- Connect the cancel_button to destroy the widget  --- */
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", (GtkSignalFunc) file_cancel_sel, filew);

	/* --- Lets set the filename --- */
	gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew), initialFilename);

	/* --- Turn off the file operation buttons --- */
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION(filew));

	/* --- Of course, we show it --- */
	gtk_widget_show (filew);

	/* --- Make sure we keep the focus --- */
	gtk_grab_add (filew);

	gtk_main ();

	return(sFilename);
}


static void ClearShowMessage (GtkWidget *widget, gpointer data)
{
	gtk_grab_remove (widget);
}

static void CancelConfigureDialog (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialogWidget = (GtkWidget *) data;

	gtk_grab_remove (dialogWidget);

	/* --- Close the widget --- */
	gtk_widget_destroy (dialogWidget);
}

static void OkConfigureDialog (GtkWidget *widget, gpointer data)
{
	GtkWidget       *dialogWidget = (GtkWidget *) data;
	gchar           *editString;

	editString = gtk_editable_get_chars (GTK_EDITABLE (fileEntry), 0, -1);

	gtk_grab_remove (dialogWidget);

	/* --- Close the widget --- */
	gtk_widget_destroy (dialogWidget);

	strcpy (devicesFile, editString);
	g_free (editString);
	LoadUSBTree(0);
}


static void fileSelectButtonClick (GtkWidget *widget, gpointer data)
{
	gchar   *newFilename;

	newFilename = GetFilename ("locate usbdevfs devices file", devicesFile);

	if (newFilename != NULL) {
		gtk_entry_set_text (GTK_ENTRY (fileEntry), newFilename);
		gtk_widget_show (fileEntry);
		g_free (newFilename);
	}
}


void configure_dialog (void)
{
	GtkWidget *configDialog;
	GtkWidget *dialog_vbox2;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *label41;
	GtkWidget *label42;
	GtkWidget *label43;
	GtkWidget *vbox1;
	GtkWidget *dialog_action_area2;
	GtkWidget *hbuttonbox2;
	GtkWidget *okButton;
	GtkWidget *cancelButton;
	GtkWidget *fileSelectButton;

	configDialog = gtk_dialog_new ();
	gtk_object_set_data (GTK_OBJECT (configDialog), "configDialog", configDialog);
	gtk_window_set_title (GTK_WINDOW (configDialog), "USB View Configuration");
	gtk_window_set_policy (GTK_WINDOW (configDialog), TRUE, TRUE, FALSE);

	dialog_vbox2 = GTK_DIALOG (configDialog)->vbox;
	gtk_object_set_data (GTK_OBJECT (configDialog), "dialog_vbox2", dialog_vbox2);
	gtk_widget_show (dialog_vbox2);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_ref (vbox1);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "vbox1", vbox1, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), vbox1, TRUE, TRUE, 0);

	label41 = gtk_label_new ("");
	gtk_widget_ref (label41);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "label41", label41, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label41);
	gtk_box_pack_start (GTK_BOX (vbox1), label41, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_ref (hbox1);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "hbox1", hbox1, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (hbox1);
//	gtk_box_pack_start (GTK_BOX (dialog_vbox2), hbox1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	label42 = gtk_label_new ("");
	gtk_widget_ref (label42);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "label42", label42, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label42);
	gtk_box_pack_start (GTK_BOX (vbox1), label42, FALSE, FALSE, 0);


	label1 = gtk_label_new ("  Location of usbdevfs devices file");
	gtk_widget_ref (label1);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "label1", label1, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 5);

	fileEntry = gtk_entry_new ();
	gtk_widget_ref (fileEntry);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "fileEntry", fileEntry, (GtkDestroyNotify) gtk_widget_unref);
	gtk_entry_set_text (GTK_ENTRY (fileEntry), devicesFile);
	gtk_widget_show (fileEntry);
	gtk_box_pack_start (GTK_BOX (hbox1), fileEntry, TRUE, TRUE, 0);

	dialog_action_area2 = GTK_DIALOG (configDialog)->action_area;
	gtk_object_set_data (GTK_OBJECT (configDialog), "dialog_action_area2", dialog_action_area2);
	gtk_widget_show (dialog_action_area2);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area2), 1);

	hbuttonbox2 = gtk_hbutton_box_new ();
	gtk_widget_ref (hbuttonbox2);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "hbuttonbox2", hbuttonbox2, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (hbuttonbox2);
	gtk_box_pack_start (GTK_BOX (dialog_action_area2), hbuttonbox2, TRUE, TRUE, 11);

	okButton = gtk_button_new_with_label ("  OK  ");
	gtk_widget_ref (okButton);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "okButton", okButton, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (okButton);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), okButton);
	GTK_WIDGET_SET_FLAGS (okButton, GTK_CAN_DEFAULT);

	cancelButton = gtk_button_new_with_label ("  Cancel  ");
	gtk_widget_ref (cancelButton);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "cancelButton", cancelButton, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (cancelButton);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), cancelButton);
	GTK_WIDGET_SET_FLAGS (cancelButton, GTK_CAN_DEFAULT);

	fileSelectButton = gtk_button_new_with_label ("...");
	gtk_widget_ref (fileSelectButton);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "fileSelectButton", fileSelectButton, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (fileSelectButton);

	gtk_box_pack_start (GTK_BOX (hbox1), fileSelectButton, TRUE, FALSE, 1);

	label43 = gtk_label_new ("    ");
	gtk_widget_ref (label43);
	gtk_object_set_data_full (GTK_OBJECT (configDialog), "label43", label43, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label43);
	gtk_box_pack_start (GTK_BOX (hbox1), label43, FALSE, FALSE, 0);
	
	gtk_signal_connect (GTK_OBJECT (okButton), "clicked", GTK_SIGNAL_FUNC (OkConfigureDialog), configDialog);
	gtk_signal_connect (GTK_OBJECT (cancelButton), "clicked", GTK_SIGNAL_FUNC (CancelConfigureDialog), configDialog);
	gtk_signal_connect (GTK_OBJECT (fileSelectButton), "clicked", GTK_SIGNAL_FUNC (fileSelectButtonClick), configDialog);

	/* --- Default the "Ok" button --- */
	GTK_WIDGET_SET_FLAGS (okButton, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (okButton);

	gtk_signal_connect (GTK_OBJECT (configDialog), "destroy", GTK_SIGNAL_FUNC (ClearShowMessage), NULL);

	/* --- Show the dialog --- */
	gtk_widget_show (configDialog);

	/* --- Only this window can have actions done. --- */
	gtk_grab_add (configDialog);

	return;
}


