/*************************************************************************
** showmessage.c for USBView - a USB device viewer
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

/*
 * This file taken from the book "Developing Linux Applications with GTK+ and GDK"
 * by Eric Harlow
 *
 */

#include <gtk/gtk.h>
#include "usbtree.h"

static gboolean message_shown = FALSE;

gboolean MessageShown (void)
{
	return message_shown;
}


/*
 * CloseShowMessage
 *
 * Routine to close the about dialog window.
 */
static void CloseShowMessage (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialogWidget = (GtkWidget *) data;

	gtk_grab_remove (dialogWidget);

	/* --- Close the widget --- */
	gtk_widget_destroy (dialogWidget);
	
	message_shown = FALSE;
}



/*
 * ClearShowMessage
 *
 * Release the window "grab" 
 * Clear out the global dialog_window since that
 * is checked when the dialog is brought up.
 */
static void ClearShowMessage (GtkWidget *widget, gpointer data)
{
	gtk_grab_remove (widget);
}


/*
 * ShowMessage
 *
 * Show a popup message to the user.
 */
void ShowMessage (gchar *title, gchar *message, gboolean centered)
{
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *dialog_window;

	message_shown = TRUE;
	
	/* --- Create a dialog window --- */
	dialog_window = gtk_dialog_new ();

	gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy", GTK_SIGNAL_FUNC (ClearShowMessage), NULL);

	/* --- Set the title and add a border --- */
	gtk_window_set_title (GTK_WINDOW (dialog_window), title);
	gtk_container_border_width (GTK_CONTAINER (dialog_window), 0);

	/* --- Create an "Ok" button with the focus --- */
	button = gtk_button_new_with_label ("  OK  ");

	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (CloseShowMessage), dialog_window);

	/* --- Default the "Ok" button --- */
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), button, TRUE, FALSE, 10);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	/* --- Create a descriptive label --- */
	label = gtk_label_new (message);

	/* --- Put some room around the label text --- */
	gtk_misc_set_padding (GTK_MISC (label), 10, 10);

	/* --- Add label to designated area on dialog --- */
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
			    label, TRUE, TRUE, 0);

	if (centered == FALSE) {
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	}
	/* --- Show the label --- */
	gtk_widget_show (label);

	/* --- Show the dialog --- */
	gtk_widget_show (dialog_window);

	/* --- Only this window can have actions done. --- */
	gtk_grab_add (dialog_window);
}

