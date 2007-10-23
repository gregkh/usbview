/*************************************************************************
** usbtree.c for USBView - a USB device viewer
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "usbtree.h"
#include "usbparse.h"

#define MAX_LINE_SIZE	1000


static void Init (void)
{
	GtkTextIter begin;
	GtkTextIter end;

	/* blow away the tree if there is one */
	if (rootDevice != NULL) {
		gtk_ctree_remove_node (GTK_CTREE(treeUSB), GTK_CTREE_NODE(rootDevice->leaf));
	}

	/* clean out the text box */
	gtk_text_buffer_get_start_iter(textDescriptionBuffer,&begin);
	gtk_text_buffer_get_end_iter(textDescriptionBuffer,&end);
	gtk_text_buffer_delete (textDescriptionBuffer, &begin, &end);

	return;
}


static void PopulateListBox (int deviceId)
{
	Device  *device;
	//gint    position = 0;
	char    *string;
	char    *tempString;
	int     configNum;
	int     interfaceNum;
	int     endpointNum;
	int     deviceNumber = (deviceId >> 8);
	int     busNumber = (deviceId & 0x00ff);
	GtkTextIter begin;
	GtkTextIter end;
	GtkTextIter position;

	device = usb_find_device (deviceNumber, busNumber);
	if (device == NULL) {
		printf ("Can't seem to find device info to display\n");
		return;
	}

	/* clear the textbox */
	gtk_text_buffer_get_start_iter(textDescriptionBuffer,&begin);
	gtk_text_buffer_get_end_iter(textDescriptionBuffer,&end);
	gtk_text_buffer_delete (textDescriptionBuffer, &begin, &end);

	/* freeze the display */
	/* this keeps the annoying scroll from happening */
	gtk_widget_freeze_child_notify(textDescriptionView);

	string = (char *)g_malloc (1000);

	/* add the name to the textbox if we have one*/
	if (device->name != NULL) {
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, device->name,strlen(device->name));
	}

	/* add the manufacturer if we have one */
	if (device->manufacturer != NULL) {
		sprintf (string, "\nManufacturer: %s", device->manufacturer);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* add the serial number if we have one */
	if (device->serialNumber != NULL) {
		sprintf (string, "\nSerial Number: %s", device->serialNumber);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* add speed */
	switch (device->speed) {
		case 1 :        tempString = "1.5Mb/s (low)";   break;
		case 12 :       tempString = "12Mb/s (full)";   break;
		case 480 :      tempString = "480Mb/s (high)";  break;		/* planning ahead... */
		default :       tempString = "unknown";         break;
	}
	sprintf (string, "\nSpeed: %s", tempString);
	gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

	/* add ports if available */
	if (device->maxChildren) {
		sprintf (string, "\nNumber of Ports: %i", device->maxChildren);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* add the bandwidth info if available */
	if (device->bandwidth != NULL) {
		sprintf (string, "\nBandwidth allocated: %i / %i (%i%%)", device->bandwidth->allocated, device->bandwidth->total, device->bandwidth->percent);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

		sprintf (string, "\nTotal number of interrupt requests: %i", device->bandwidth->numInterruptRequests);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

		sprintf (string, "\nTotal number of isochronous requests: %i", device->bandwidth->numIsocRequests);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* add the USB version, device class, subclass, protocol, max packet size, and the number of configurations (if it is there) */
	if (device->version) {
		sprintf (string, "\nUSB Version: %s\nDevice Class: %s\nDevice Subclass: %s\nDevice Protocol: %s\n"
			 "Maximum Default Endpoint Size: %i\nNumber of Configurations: %i",
			 device->version, device->class, device->subClass, device->protocol,
			 device->maxPacketSize, device->numConfigs);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* add the vendor id, product id, and revision number (if it is there) */
	if (device->vendorId) {
		sprintf (string, "\nVendor Id: %.4x\nProduct Id: %.4x\nRevision Number: %s",
			 device->vendorId, device->productId, device->revisionNumber);
		gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
	}

	/* display all the info for the configs */
	for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
		if (device->config[configNum]) {
			DeviceConfig    *config = device->config[configNum];

			/* show this config */
			sprintf (string, "\n\nConfig Number: %i\n\tNumber of Interfaces: %i\n\t"
				 "Attributes: %.2x\n\tMaxPower Needed: %s",
				 config->configNumber, config->numInterfaces, 
				 config->attributes, config->maxPower);
			gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

			/* show all of the interfaces for this config */
			for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
				if (config->interface[interfaceNum]) {
					DeviceInterface *interface = config->interface[interfaceNum];

					sprintf (string, "\n\n\tInterface Number: %i", interface->interfaceNumber);
					gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string, strlen(string));

					if (interface->name != NULL) {
						sprintf (string, "\n\t\tName: %s", interface->name);
						gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string, strlen(string));
					}

					sprintf (string, "\n\t\tAlternate Number: %i\n\t\tClass: %s\n\t\t"
						 "Sub Class: %.2x\n\t\tProtocol: %.2x\n\t\tNumber of Endpoints: %i",
						 interface->alternateNumber, interface->class, 
						 interface->subClass, interface->protocol, interface->numEndpoints);
					gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string, strlen(string));

					/* show all of the endpoints for this interface */
					for (endpointNum = 0; endpointNum < MAX_ENDPOINTS; ++endpointNum) {
						if (interface->endpoint[endpointNum]) {
							DeviceEndpoint  *endpoint = interface->endpoint[endpointNum];

							sprintf (string, "\n\n\t\t\tEndpoint Address: %.2x\n\t\t\t"
								 "Direction: %s\n\t\t\tAttribute: %i\n\t\t\t"
								 "Type: %s\n\t\t\tMax Packet Size: %i\n\t\t\tInterval: %s",
								 endpoint->address, 
								 endpoint->in ? "in" : "out", endpoint->attribute,
								 endpoint->type, endpoint->maxPacketSize, endpoint->interval);
							gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));
						}
					}
				}
			}
		}
	}

	/* thaw the display */
	gtk_widget_thaw_child_notify(textDescriptionView);

	/* clean up our string */
	g_free (string);

	return;
}


void SelectItem (GtkWidget *widget, GtkCTreeNode *node, gint column, gpointer userData)
{
	int     data;
	data = (int) gtk_ctree_node_get_row_data (GTK_CTREE (widget), node);

	PopulateListBox ((int)data);

	return;
}


static void DisplayDevice (Device *parent, Device *device)
{
	int		i;
	gchar		*text[1];
	int		configNum;
	int		interfaceNum;
	gboolean	driverAttached = TRUE;

	if (device == NULL)
		return;

	/* build this node */
	text[0] = device->name;
	device->leaf = gtk_ctree_insert_node (GTK_CTREE(treeUSB), parent->leaf, NULL, text, 1, NULL, NULL, NULL, NULL, FALSE, FALSE);
	gtk_ctree_node_set_row_data (GTK_CTREE(treeUSB), device->leaf, (gpointer)((device->deviceNumber<<8) | (device->busNumber)));

	/* determine if this device has drivers attached to all interfaces */
	for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
		if (device->config[configNum]) {
			DeviceConfig    *config = device->config[configNum];
			for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
				if (config->interface[interfaceNum]) {
					DeviceInterface *interface = config->interface[interfaceNum];
					if (interface->driverAttached == FALSE) {
						driverAttached = FALSE;
						break;
					}
				}
			}
		}
	}

	/* change the color of this leaf if there are no drivers attached to it */
	if (driverAttached == FALSE) {
		GdkColor        red;
		 
		red.red = 56000;
		red.green = 0;
		red.blue = 0;
		red.pixel = 0;
		gtk_ctree_node_set_foreground (GTK_CTREE(treeUSB), device->leaf, &red);
	}

	/* create all of the children's leafs */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		DisplayDevice (device, device->child[i]);
	}

	return;
}


#define FILENAME_SIZE	1000;

gchar devicesFile[1000];
static gchar previousDevicesFile[1000];
static time_t	previousChange;

const char *verifyMessage =     " Verify that you have USB compiled into your kernel, \n"
				" have the USB core modules loaded, and have the \n"
				" usbdevfs filesystem mounted. ";

static void FileError (void)
{
	gchar *tempString = g_malloc0(strlen (verifyMessage) + strlen (devicesFile) + 50);
	sprintf (tempString, " Can not open the file %s \n\n%s", devicesFile, verifyMessage);
	ShowMessage ("USBView Error", tempString, FALSE);
	g_free (tempString);
	return;
}


static int FileHasChanged (void)
{
	struct stat	file_info;
	int		result;

	if (strcmp (previousDevicesFile, devicesFile) == 0) {
		/* we've looked at this filename before, so check the file time of the file */
		result = stat (devicesFile, &file_info);
		if (result) {
			/* something wrong in looking for this file */
			return 0;
		}
		
		if (file_info.st_ctime == previousChange) {
			/* no change */
			return 0;
		} else {
			/* something changed */
			previousChange = file_info.st_ctime;
			return 1;
		}
	} else {
		/* filenames are different, so save the name for the next time */
		strcpy (previousDevicesFile, devicesFile);
		return 1;
	}
}


void LoadUSBTree (int refresh)
{
	static gboolean signal_connected = FALSE;
	FILE            *usbFile;
	char            *dataLine;
	int             finished;
	int             i;

	if (MessageShown() == TRUE) {
		return;
	}

	/* if refresh is selected, then always do a refresh, otherwise look at the file first */
	if (!refresh) {
		if (!FileHasChanged()) {
			return;
		}
	}

	usbFile = fopen (devicesFile, "r");
	if (usbFile == NULL) {
		FileError();
		return;
	}
	finished = 0;

	Init();

	usb_initialize_list ();

	dataLine = (char *)g_malloc (MAX_LINE_SIZE);
	while (!finished) {
		/* read the line in from the file */
		fgets (dataLine, MAX_LINE_SIZE-1, usbFile);

		if (dataLine[strlen(dataLine)-1] == '\n')
			usb_parse_line (dataLine);

		if (feof (usbFile))
			finished = 1;
	}

	fclose (usbFile);
	g_free (dataLine);

	usb_name_devices ();

	/* set up our tree */
	gtk_ctree_set_line_style (GTK_CTREE(treeUSB), GTK_CTREE_LINES_DOTTED);
	gtk_ctree_set_expander_style (GTK_CTREE(treeUSB), GTK_CTREE_EXPANDER_SQUARE);
	gtk_ctree_set_indent (GTK_CTREE(treeUSB),10);
	gtk_clist_column_titles_passive (GTK_CLIST(treeUSB));

	/* build our tree */
	for (i = 0; i < rootDevice->maxChildren; ++i) {
		DisplayDevice (rootDevice, rootDevice->child[i]);
	}

	gtk_widget_show (treeUSB);

	gtk_ctree_expand_recursive (GTK_CTREE(treeUSB), NULL);

	/* hook up our callback function to this tree if we haven't yet */
	if (!signal_connected) {
		gtk_signal_connect (GTK_OBJECT (treeUSB), "tree-select-row", GTK_SIGNAL_FUNC (SelectItem), NULL);
		signal_connected = TRUE;
	}

	return;
}



void initialize_stuff (void)
{
	strcpy (devicesFile, "/proc/bus/usb/devices");
	memset (&previousDevicesFile[0], 0x00, sizeof(previousDevicesFile));
	previousChange = 0;

	return;
}

