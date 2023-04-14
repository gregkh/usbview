// SPDX-License-Identifier: GPL-2.0-only
/*
 * usbtree.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000, 2021-2022 by Greg Kroah-Hartman, <greg@kroah.com>
 */
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
#include "sysfs.h"

#define MAX_LINE_SIZE	1000


static void Init (void)
{
	GtkTextIter begin;
	GtkTextIter end;

	/* blow away the tree if there is one */
	if (rootDevice != NULL) {
		gtk_tree_store_clear (treeStore);
	}

	/* clean out the text box */
	gtk_text_buffer_get_start_iter(textDescriptionBuffer,&begin);
	gtk_text_buffer_get_end_iter(textDescriptionBuffer,&end);
	gtk_text_buffer_delete (textDescriptionBuffer, &begin, &end);

	return;
}


static void PopulateListBox (int deviceId)
{
	struct Device *device;
	char    *string;
	char    *tempString;
	int     configNum;
	int     interfaceNum;
	int     endpointNum;
	int     deviceNumber = (deviceId >> 8);
	int     busNumber = (deviceId & 0x00ff);
	GtkTextIter begin;
	GtkTextIter end;

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
		case 480 :      tempString = "480Mb/s (high)";  break;
		case 5000 :     tempString = "5Gb/s (super)";   break;
		case 10000 :    tempString = "10Gb/s (super+)"; break;
		case 20000 :    tempString = "20Gb/s (super+)"; break;
		default :       tempString = "unknown";         break;
	}
	sprintf (string, "\nSpeed: %s", tempString);
	gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

	/* Add Bus number */
	sprintf (string, "\nBus:%4d", busNumber);
	gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

	/* Add device address */
	sprintf (string, "\nAddress:%4d", deviceNumber);
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
			struct DeviceConfig *config = device->config[configNum];

			/* show this config */
			sprintf (string, "\n\nConfig Number: %i\n\tNumber of Interfaces: %i\n\t"
				 "Attributes: %.2x\n\tMaxPower Needed: %s",
				 config->configNumber, config->numInterfaces,
				 config->attributes, config->maxPower);
			gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, string,strlen(string));

			/* show all of the interfaces for this config */
			for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
				if (config->interface[interfaceNum]) {
					struct DeviceInterface *interface = config->interface[interfaceNum];

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
							struct DeviceEndpoint *endpoint = interface->endpoint[endpointNum];

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


static void SelectItem (GtkTreeSelection *selection, gpointer userData)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint deviceAddr;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				DEVICE_ADDR_COLUMN, &deviceAddr,
				-1);
		PopulateListBox (deviceAddr);
	}
}


static void DisplayDevice (struct Device *parent, struct Device *device)
{
	int		i;
	int		configNum;
	int		interfaceNum;
	gboolean	driverAttached = TRUE;
	gint		deviceAddr;
	const gchar	*tooltip = NULL;
	const gchar	*color = NULL;

	if (device == NULL)
		return;

	/* build this node */
	deviceAddr = (device->deviceNumber << 8) | device->busNumber;
	gtk_tree_store_append (treeStore, &device->leaf,
			       (device->level != 0) ? &parent->leaf : NULL);

	/* determine if this device has drivers attached to all interfaces */
	for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
		if (device->config[configNum]) {
			struct DeviceConfig *config = device->config[configNum];
			for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
				if (config->interface[interfaceNum]) {
					struct DeviceInterface *interface = config->interface[interfaceNum];
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
		color = "red";
		tooltip = "This device has no attached driver";
	}

	gtk_tree_store_set (treeStore, &device->leaf,
			    NAME_COLUMN, device->name,
			    DEVICE_ADDR_COLUMN, deviceAddr,
			    COLOR_COLUMN, color,
			    TOOLTIP_COLUMN, tooltip,
			    -1);

	/* create all of the children's leafs */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		DisplayDevice (device, device->child[i]);
	}

	return;
}


void LoadUSBTree (int refresh)
{
	static gboolean signal_connected = FALSE;
	int             i;

	Init();

	usb_initialize_list ();

	sysfs_parse();
	usb_name_devices ();

	/* build our tree */
	for (i = 0; i < rootDevice->maxChildren; ++i) {
		DisplayDevice (rootDevice, rootDevice->child[i]);
	}

	gtk_widget_show (treeUSB);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (treeUSB));

	/* hook up our callback function to this tree if we haven't yet */
	if (!signal_connected) {
		GtkTreeSelection *select;
		select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeUSB));
		g_signal_connect (G_OBJECT (select), "changed",
				  G_CALLBACK (SelectItem), NULL);
		signal_connected = TRUE;
	}

	return;
}

void initialize_stuff(void)
{
	return;
}
