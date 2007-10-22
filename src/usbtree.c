/*************************************************************************
** usbtree.c for USBView - a USB device viewer
** Copyright (c) 1999 by Greg Kroah-Hartman, greg@kroah.com
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
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "usbtree.h"

#define MAX_LINE_SIZE	1000
#define MAX_CHILDREN	8


typedef struct Device {
	char		*name;
	int		level;
	int		connectorNumber;
	int		count;
	int		deviceNumber;
	int		speed;
	int		interfaceNumber;
	int		maxChildren;
	char		*vendorId;
	char		*productId;
	char		*revisionNumber;
	struct Device	*parent;
	struct Device	*child[MAX_CHILDREN];
	GtkWidget	*tree;
	GtkWidget	*leaf;
} Device;

static Device	*rootDevice = NULL;
static Device	*lastDevice;


static signed int GetValue (char a, char b, char c)
{
	char		hundreds = 0;
	char		tens = 0;
	char		ones = 0;
	char		positive = 1;
	signed int	value;
	
	if (isdigit(a))
		hundreds = a - '0';
	else if (a == '-')
		positive = 0;
		
	if (isdigit(b))
		tens = b - '0';
	else if (b == '-')
		positive = 0;

	if (isdigit(c))
		ones = c - '0';
	else if (c == '-')
		positive = 0;

	value = (hundreds * 100) + (tens * 10) + ones;
	if (!positive)
		value = -value;
	
	return (value);
}


static void DestroyDevice (Device *device)
{
	int	i;
	
	if (device == NULL)
		return;
		
	for (i = 0; i < MAX_CHILDREN; ++i) {
		DestroyDevice (device->child[i]);
		}

	if (device->name)
		free (device->name);
	if (device->vendorId)
		free (device->vendorId);
	if (device->productId)
		free (device->productId);
	if (device->revisionNumber)
		free (device->revisionNumber);

	free (device);

	return;
}




static void Init (void)
{
	if (rootDevice != NULL) {
		DestroyDevice (rootDevice);
		rootDevice = NULL;
		}

	rootDevice = (Device *)malloc (sizeof(Device));
	memset (rootDevice, 0x00, sizeof(Device));

	/* blow away the tree */
	gtk_tree_clear_items (GTK_TREE(treeUSB), 0, -1);
	
	/* clean out the text box */
	gtk_editable_delete_text (GTK_EDITABLE(textDescription), 0, -1);

	lastDevice = NULL;

	return;
}


static Device *FindDeviceNode (Device *device, int deviceNumber)
{
	int	i;
	Device	*result;
	
	if (device == NULL)
		return (NULL);

	if (device->deviceNumber == deviceNumber)
		return (device);
		
	for (i = 0; i < MAX_CHILDREN; ++i) {
		result = FindDeviceNode (device->child[i], deviceNumber);
		if (result != NULL)
			return (result);
		}

	return (NULL);
}


static Device *FindDevice (int deviceNumber)
{
	int	i;
	Device	*result;

	/* search through the tree to try to find a device */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		result = FindDeviceNode (rootDevice->child[i], deviceNumber);
		if (result != NULL)
			return (result);
		}
	return (NULL);
}


static Device *AddDevice (char *line)
{
	Device	*tempDevice;
	int	level;
	int	parentNumber;
	int	portNumber;
	int	count;
	int	deviceNumber;
	int	speed;
	int	interfaceNumber;
	int	maxChildren;
	int	nameLength;
	char	*name;
	
	/* parse the line */
	level		= GetValue (' ', line[8], line[9]);
	parentNumber	= GetValue (' ', line[16], line[17]);
	portNumber	= GetValue (' ', line[24], line[25]);
	count		= GetValue (' ', line[31], line[32]);
	deviceNumber	= GetValue (line[39], line[40], line[41]);
	speed		= GetValue (line[47], line[48], line[49]);
	interfaceNumber	= GetValue (line[55], line[56], line[57]);
	maxChildren	= GetValue (' ', line[64], line[65]);
	
	nameLength = strlen (&line[74])+1;
	name = (char *)malloc(nameLength);
	strncpy (name, &line[74], nameLength);
	name[nameLength-2] = 0x00;
	
	/* printf ("%i %i %i %i %i %i %i %i %s\n", level, parentNumber, portNumber, count, deviceNumber, speed, interfaceNumber, maxChildren, name); */

	tempDevice = (Device *)(malloc (sizeof(Device)));
	memset (tempDevice, 0x00, sizeof(Device));
	
	/* Set up the parent / child relationship */
	if (level == 0) {
		/* this is the root, don't go looking for a parent */
		tempDevice->parent = rootDevice;
		rootDevice->maxChildren = 1;
		rootDevice->child[0] = tempDevice;
		}
	else {
		/* need to find this device's parent */
		tempDevice->parent = FindDevice (parentNumber);
		if (tempDevice->parent == NULL) {
			printf ("can't find parent...not good.\n");
			}
		tempDevice->parent->child[portNumber] = tempDevice;
		}
	/* fill up the driver's fields */
	if (deviceNumber == -1)
		deviceNumber = 0;
	tempDevice->deviceNumber = deviceNumber;
	tempDevice->name = name;
	tempDevice->speed = speed;
	tempDevice->maxChildren = maxChildren;

	return (tempDevice);
}


#define VENDOR_ID_SIZE		5
#define PRODUCT_ID_SIZE		5
#define REVISION_NUMBER_SIZE	6

void GetMoreDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;
		
	/* ok, this is a hack, I "should" turn the id's into the raw number, but for now, 
	   let's just stick with the string representation */
	if (device->vendorId)
		free (device->vendorId);
	if (device->productId)
		free (device->productId);
	if (device->revisionNumber)
		free (device->revisionNumber);

	device->vendorId	= (char *)malloc ((VENDOR_ID_SIZE) * sizeof(char));
	device->productId	= (char *)malloc ((PRODUCT_ID_SIZE) * sizeof(char));
	device->revisionNumber	= (char *)malloc ((REVISION_NUMBER_SIZE) * sizeof(char));

	memset (device->vendorId, 0x00, VENDOR_ID_SIZE);
	memset (device->productId, 0x00, PRODUCT_ID_SIZE);
	memset (device->revisionNumber, 0x00, REVISION_NUMBER_SIZE);
	
	memcpy (device->vendorId, &data[11], VENDOR_ID_SIZE-1);
	memcpy (device->productId, &data[23], PRODUCT_ID_SIZE-1);
	memcpy (device->revisionNumber, &data[32], REVISION_NUMBER_SIZE-1);
	
	/* printf ("%s\t%s\t%s\n", device->vendorId, device->productId, device->revisionNumber); */

	return;
}



void PopulateListBox (int deviceNumber)
{
	Device	*device;
	gint	position = 0;
	char	*string;
	
	device = FindDevice (deviceNumber);
	if (device == NULL) {
		printf ("Can't seem to find device info to display\n");
		return;
		}

	/* clear the textbox */
	gtk_editable_delete_text (GTK_EDITABLE(textDescription), 0, -1);

	string = (char *)malloc (1000);

	/* add the name to the textbox */
	gtk_editable_insert_text (GTK_EDITABLE(textDescription), device->name, strlen(device->name), &position);

	/* add speed (commented out cauz we aren't currently grabbing this correctly
	sprintf (string, "\nSpeed: %i", device->speed);
	gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
	*/

	/* add ports if available */
	if (device->maxChildren) {
		sprintf (string, "\nNumber of Ports: %i", device->maxChildren);
		gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
		}

	/* add the vendor id, product id, and revision number (if it is there) */
	if (device->vendorId) {
		sprintf (string, "\nVendor Id: %s\tProduct Id: %s\tRevision Number: %s",
			 device->vendorId, device->productId, device->revisionNumber);
		
		gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
		}

	free (string);

	return;
}



void SelectItem (GtkWidget *widget, gpointer data)
{
	PopulateListBox ((int)data);
	
	return;
}


static void DisplayDevice (Device *parent, Device *device)
{
	int	i;
	char	hasChildren;
	
	if (device == NULL)
		return;
		
	/* create the leaf for this device */
	device->leaf = gtk_tree_item_new_with_label (device->name);
	if (parent->tree == NULL)
		gtk_tree_append (GTK_TREE (treeUSB), device->leaf);
	else
		gtk_tree_append (GTK_TREE (parent->tree), device->leaf);
	gtk_widget_show (device->leaf);

	/* hook up our callback function to this node */
	gtk_signal_connect (GTK_OBJECT (device->leaf), "select", GTK_SIGNAL_FUNC (SelectItem), (gpointer)(device->deviceNumber));

	/* if we have children, then make a subtree */
	hasChildren = 0;
	for (i = 0; i < MAX_CHILDREN; ++i) {
		if (device->child[i]) {
			hasChildren = 1;
			break;
			}
		}
	if (hasChildren) {
		device->tree = gtk_tree_new();
		gtk_tree_item_set_subtree (GTK_TREE_ITEM(device->leaf), device->tree);
		}
	
	/* create all of the children's leafs */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		DisplayDevice (device, device->child[i]);
		}

	return;
}
	

static void ParseLine (char * line)
{
	/* look at the first character to see what kind of line this is */
	switch (line[0]) {
		case 'T': /* topology */
			lastDevice = AddDevice (line);
			break;
			
		case 'D': /* device information */
			break;

		case 'P': /* more device information */
			GetMoreDeviceInformation (lastDevice, line);
			break;

		case 'C': /* config descriptor info */
		case 'I': /* interface descriptor info */
		case 'E': /* endpoint descriptor info */
		default:
			break;
		}

	return;
}


void LoadUSBTree (void)
{
	FILE		*usbFile;
	char		*dataLine;
	int		finished;

	usbFile = fopen ("/proc/bus/usb/devices", "r");
	if (usbFile == NULL) {
		fprintf (stderr, "Can not open /proc/bus/usb/devices\n"
				"Verify that you have this option compiled in to your kernel, and that you\n"
				"have the correct permissions to access it.\n");
		return;
		}
	finished = 0;
	Init();

	dataLine = (char *)malloc (MAX_LINE_SIZE);
	while (!finished) {
		/* read the line in from the file */
		fgets (dataLine, MAX_LINE_SIZE, usbFile);

		ParseLine (dataLine);
		
		if (feof (usbFile))
			finished = 1;
		}


	DisplayDevice (rootDevice, rootDevice->child[0]);
	
	
	return;
}
