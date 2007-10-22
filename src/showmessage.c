/*
 * Showmessage.c
 * 
 * This file taken from the book "Developing Linux Applications with GTK+ and GDK"
 * by Eric Harlow
 *
 */

#include <gtk/gtk.h>
#include "showmessage.h"

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
void ShowMessage (gchar *title, gchar *message)
{
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *dialog_window;

    /* --- Create a dialog window --- */
    dialog_window = gtk_dialog_new ();

    gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy", GTK_SIGNAL_FUNC (ClearShowMessage), NULL);

    /* --- Set the title and add a border --- */
    gtk_window_set_title (GTK_WINDOW (dialog_window), title);
    gtk_container_border_width (GTK_CONTAINER (dialog_window), 0);

    /* --- Create an "Ok" button with the focus --- */
    button = gtk_button_new_with_label ("OK");

    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (CloseShowMessage), dialog_window);

	/* --- Default the "Ok" button --- */
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

    /* --- Create a descriptive label --- */
    label = gtk_label_new (message);

    /* --- Put some room around the label text --- */
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);

    /* --- Add label to designated area on dialog --- */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
              label, TRUE, TRUE, 0);

    /* --- Show the label --- */
    gtk_widget_show (label);

    /* --- Show the dialog --- */
    gtk_widget_show (dialog_window);

    /* --- Only this window can have actions done. --- */
    gtk_grab_add (dialog_window);
}

