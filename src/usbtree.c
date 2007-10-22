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

#define MAX_ENDPOINTS	32
#define MAX_INTERFACES	8
#define MAX_CONFIGS	8
#define MAX_CHILDREN	8

#define DEVICE_VERSION_SIZE		6
#define DEVICE_CLASS_SIZE		10
#define DEVICE_SUBCLASS_SIZE		3
#define DEVICE_PROTOCOL_SIZE		3
#define DEVICE_VENDOR_ID_SIZE		5
#define DEVICE_PRODUCT_ID_SIZE		5
#define DEVICE_REVISION_NUMBER_SIZE	6

#define CONFIG_ATTRIBUTES_SIZE		3
#define CONFIG_MAXPOWER_SIZE		10

#define INTERFACE_CLASS_SIZE		10
#define INTERFACE_SUBCLASS_SIZE		3
#define INTERFACE_PROTOCOL_SIZE		3

#define ENDPOINT_TYPE_SIZE		5
#define ENDPOINT_MAXPACKETSIZE_SIZE	5
#define ENDPOINT_INTERVAL_SIZE		10

typedef struct DeviceEndpoint {
	gint		address;
	gboolean	in;		/* TRUE if in, FALSE if out */
	gint		attribute;
	gchar		*type;
	gchar		*maxPacketSize;
	gchar		*interval;
} DeviceEndpoint;


typedef struct DeviceInterface {
	gint		interfaceNumber;
	gint		alternateNumber;
	gint		numEndpoints;
	gchar		*class;
	gchar		*subClass;
	gchar		*protocol;
	DeviceEndpoint	*endpoint[MAX_ENDPOINTS];
} DeviceInterface;
	


typedef struct DeviceConfig {
	gint		configNumber;
	gint		numInterfaces;
	gchar		*attributes;
	gchar		*maxPower;
	DeviceInterface	*interface[MAX_INTERFACES];
} DeviceConfig;


typedef struct Device {
	char		*name;
	int		level;
	int		parentNumber;
	int		portNumber;
	int		connectorNumber;
	int		count;
	int		deviceNumber;
	int		speed;
	int		interfaceNumber;
	int		maxChildren;
	char		*version;
	char		*class;
	char		*subClass;
	char		*protocol;
	int		maxPacketSize;
	int		numConfigs;
	char		*vendorId;
	char		*productId;
	char		*revisionNumber;
	DeviceConfig	*config[MAX_CONFIGS];
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


static void DestroyEndpoint (DeviceEndpoint *endpoint)
{
	if (endpoint == NULL)
		return;
		
	g_free (endpoint->type);
	g_free (endpoint->maxPacketSize);
	g_free (endpoint->interval);
	
	g_free (endpoint);
	
	return;
}


static void DestroyInterface (DeviceInterface *interface)
{
	int	i;
	
	if (interface == NULL)
		return;
		
	for (i = 0; i < MAX_ENDPOINTS; ++i)
		DestroyEndpoint (interface->endpoint[i]);

	g_free (interface->class);
	g_free (interface->subClass);
	g_free (interface->protocol);
	
	g_free (interface);
	
	return;
}


static void DestroyConfig (DeviceConfig *config)
{
	int	i;
	
	if (config == NULL)
		return;
		
	for (i = 0; i < MAX_INTERFACES; ++i)
		DestroyInterface (config->interface[i]);

	g_free (config->attributes);
	g_free (config->maxPower);
	
	g_free (config);
	
	return;
}


static void DestroyDevice (Device *device)
{
	int	i;
	
	if (device == NULL)
		return;
		
	for (i = 0; i < MAX_CHILDREN; ++i)
		DestroyDevice (device->child[i]);

	for (i = 0; i < MAX_CONFIGS; ++i)
		DestroyConfig (device->config[i]);

	g_free (device->name);
	g_free (device->version);
	g_free (device->class);
	g_free (device->subClass);
	g_free (device->protocol);
	g_free (device->vendorId);
	g_free (device->productId);
	g_free (device->revisionNumber);

	g_free (device);

	return;
}




static void Init (void)
{
	if (rootDevice != NULL) {
		DestroyDevice (rootDevice);
		rootDevice = NULL;
		}

	rootDevice = (Device *)g_malloc0 (sizeof(Device));

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
	Device	*device;
	int	nameLength;
	
	/* create a new device */
	device = (Device *)(g_malloc0 (sizeof(Device)));
	
	/* parse the line */
	/* parsing in C sucks, I could do this a zillion different ways, but this
	   is good enough for now. */
	device->level		= GetValue (' ', line[8], line[9]);
	device->parentNumber	= GetValue (' ', line[16], line[17]);
	device->portNumber	= GetValue (' ', line[24], line[25]);
	device->count		= GetValue (' ', line[31], line[32]);
	device->deviceNumber	= GetValue (line[39], line[40], line[41]);
	device->speed		= GetValue (line[47], line[48], line[49]);
	device->interfaceNumber	= GetValue (line[55], line[56], line[57]);
	device->maxChildren	= GetValue (' ', line[64], line[65]);
	
	nameLength = strlen (&line[74])+1;
	device->name = (char *)g_malloc0 (nameLength);
	strncpy (device->name, &line[74], nameLength);
	device->name[nameLength-2] = 0x00;
	
	if (device->deviceNumber == -1)
		device->deviceNumber = 0;

	/* printf ("%i %i %i %i %i %i %i %i %s\n", level, parentNumber, portNumber, count, deviceNumber, speed, interfaceNumber, maxChildren, name); */

	/* Set up the parent / child relationship */
	if (device->level == 0) {
		/* this is the root, don't go looking for a parent */
		device->parent = rootDevice;
		rootDevice->maxChildren = 1;
		rootDevice->child[0] = device;
		}
	else {
		/* need to find this device's parent */
		device->parent = FindDevice (device->parentNumber);
		if (device->parent == NULL) {
			printf ("can't find parent...not good.\n");
			}
		device->parent->child[device->portNumber] = device;
		}

	return (device);
}





static void GetDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;
		
	/* ok, this is a hack, I "should" turn the id's into the raw number, but for now, 
	   let's just stick with the string representation */
	g_free (device->version);
	g_free (device->class);
	g_free (device->subClass);
	g_free (device->protocol);

	device->version		= (char *)g_malloc0 ((DEVICE_VERSION_SIZE) * sizeof(char));
	device->class		= (char *)g_malloc0 ((DEVICE_CLASS_SIZE) * sizeof(char));
	device->subClass	= (char *)g_malloc0 ((DEVICE_SUBCLASS_SIZE) * sizeof(char));
	device->protocol	= (char *)g_malloc0 ((DEVICE_PROTOCOL_SIZE) * sizeof(char));

	memcpy (device->version, &data[8], DEVICE_VERSION_SIZE-1);
	memcpy (device->class, &data[18], DEVICE_CLASS_SIZE-1);
	memcpy (device->subClass, &data[32], DEVICE_SUBCLASS_SIZE-1);
	memcpy (device->protocol, &data[40], DEVICE_PROTOCOL_SIZE-1);
	
	device->maxPacketSize	= GetValue (' ', data[48], data[49]);
	device->numConfigs	= GetValue (data[57], data[58], data[59]);

	return;
}



static void GetMoreDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;
		
	/* ok, this is a hack, I "should" turn the id's into the raw number, but for now, 
	   let's just stick with the string representation */
	g_free (device->vendorId);
	g_free (device->productId);
	g_free (device->revisionNumber);

	device->vendorId	= (char *)g_malloc0 ((DEVICE_VENDOR_ID_SIZE) * sizeof(char));
	device->productId	= (char *)g_malloc0 ((DEVICE_PRODUCT_ID_SIZE) * sizeof(char));
	device->revisionNumber	= (char *)g_malloc0 ((DEVICE_REVISION_NUMBER_SIZE) * sizeof(char));

	memcpy (device->vendorId, &data[11], DEVICE_VENDOR_ID_SIZE-1);
	memcpy (device->productId, &data[23], DEVICE_PRODUCT_ID_SIZE-1);
	memcpy (device->revisionNumber, &data[32], DEVICE_REVISION_NUMBER_SIZE-1);
	
	/* printf ("%s\t%s\t%s\n", device->vendorId, device->productId, device->revisionNumber); */

	return;
}


static void AddConfig (Device *device, char *data)
{
	DeviceConfig	*config;
	int		i;

	if (device == NULL)
		return;

	/* Find the next available config in this device */
	for (i = 0; i < MAX_CONFIGS; ++i) {
		if (device->config[i] == NULL) {
			break;
			}
		}
	if (i >= MAX_CONFIGS) {
		/* ran out of room to hold this config */
		g_warning ("Too many configs for this device.\n");
		return;
		}
	
	config = (DeviceConfig *)g_malloc0 (sizeof(DeviceConfig));
	config->attributes	= (gchar *)g_malloc0 ((CONFIG_ATTRIBUTES_SIZE) * sizeof(gchar));
	config->maxPower	= (gchar *)g_malloc0 ((CONFIG_MAXPOWER_SIZE) * sizeof(gchar));

	config->numInterfaces	= GetValue (' ', data[9], data[10]);
	config->configNumber	= GetValue (' ', data[17], data[18]);

	memcpy (config->attributes, &data[24], CONFIG_ATTRIBUTES_SIZE-1);
	data[strlen(data)-1] = 0x00;
	strncpy (config->maxPower, &data[33], CONFIG_MAXPOWER_SIZE-1);
	
	/* have the device now point to this config */
	device->config[i] = config;
	
	return;
}


static void AddInterface (Device *device, char *data)
{
	DeviceConfig	*config;
	DeviceInterface	*interface;
	int		i;
	int		configNum;
	
	if (device == NULL)
		return;

	/* find the LAST config in the device */
	configNum = -1;
	for (i = 0; i < MAX_CONFIGS; ++i)
		if (device->config[i])
			configNum = i;
	if (configNum == -1) {
		/* no config to put this interface at, not good */
		g_warning ("No config to put an interface at for this device.\n");
		return;
		}
		
	config = device->config[configNum];

	/* now find a place in this config to place the interface */
	for (i = 0; i < MAX_INTERFACES; ++i)
		if (config->interface[i] == NULL)
			break;
	if (i >= MAX_INTERFACES) {
		/* ran out of room to hold this interface */
		g_warning ("Too many interfaces for this device.\n");
		return;
		}
	
	interface = (DeviceInterface *)g_malloc0 (sizeof(DeviceInterface));

	interface->class	= (gchar *)g_malloc0 ((INTERFACE_CLASS_SIZE) * sizeof(gchar));
	interface->subClass	= (gchar *)g_malloc0 ((INTERFACE_SUBCLASS_SIZE) * sizeof(gchar));
	interface->protocol	= (gchar *)g_malloc0 ((INTERFACE_PROTOCOL_SIZE) * sizeof(gchar));

	interface->interfaceNumber	= GetValue (' ', data[8], data[9]);
	interface->alternateNumber	= GetValue (' ', data[15], data[16]);
	interface->numEndpoints		= GetValue (' ', data[23], data[24]);

	memcpy (interface->class, &data[30], INTERFACE_CLASS_SIZE-1);
	memcpy (interface->subClass, &data[44], INTERFACE_SUBCLASS_SIZE-1);
	memcpy (interface->protocol, &data[52], INTERFACE_PROTOCOL_SIZE-1);

	/* now point the config to this interface */
	config->interface[i] = interface;

	return;
}


static void AddEndpoint (Device *device, char *data)
{
	DeviceConfig	*config;
	DeviceInterface	*interface;
	DeviceEndpoint	*endpoint;
	int		i;
	int		configNum;
	int		interfaceNum;
	
	if (device == NULL)
		return;

	/* find the LAST config in the device */
	configNum = -1;
	for (i = 0; i < MAX_CONFIGS; ++i)
		if (device->config[i])
			configNum = i;
	if (configNum == -1) {
		/* no config to put this interface at, not good */
		g_warning ("No config to put an interface at for this device.\n");
		return;
		}
		
	config = device->config[configNum];

	/* find the LAST interface in the config */
	interfaceNum = -1;
	for (i = 0; i < MAX_INTERFACES; ++i)
		if (config->interface[i])
			interfaceNum = i;
	if (interfaceNum == -1) {
		/* no interface to put this endpoint at, not good */
		g_warning ("No interface to put an endpoint at for this device.\n");
		return;
		}

	interface = config->interface[interfaceNum];
		
	/* now find a place in this interface to place the endpoint */
	for (i = 0; i < MAX_ENDPOINTS; ++i) {
		if (interface->endpoint[i] == NULL) {
			break;
			}
		}
	if (i >= MAX_ENDPOINTS) {
		/* ran out of room to hold this endpoint */
		g_warning ("Too many endpoints for this device.\n");
		return;
		}
	
	endpoint = (DeviceEndpoint *)g_malloc0 (sizeof(DeviceEndpoint));

	endpoint->type		= (gchar *)g_malloc0 ((ENDPOINT_TYPE_SIZE) * sizeof(gchar));
	endpoint->maxPacketSize	= (gchar *)g_malloc0 ((ENDPOINT_MAXPACKETSIZE_SIZE) * sizeof(gchar));
	endpoint->interval	= (gchar *)g_malloc0 ((ENDPOINT_INTERVAL_SIZE) * sizeof(gchar));

	endpoint->address	= GetValue (' ', data[7], data[8]);	/* not too good, this is a hex number...as most are...hmm...*/
	if (data[10] == 'I')
		endpoint->in = TRUE;
	else
		endpoint->in = FALSE;
	endpoint->attribute	= GetValue (' ', data[17], data[18]);
	
	memcpy (endpoint->type, &data[20], ENDPOINT_TYPE_SIZE-1);
	memcpy (endpoint->maxPacketSize, &data[31], ENDPOINT_MAXPACKETSIZE_SIZE-1);
	data[strlen(data)-1] = 0x00;
	strncpy (endpoint->interval, &data[40], ENDPOINT_INTERVAL_SIZE-1);

	/* point the interface to the endpoint */
	interface->endpoint[i] = endpoint;
}


static void PopulateListBox (int deviceNumber)
{
	Device	*device;
	gint	position = 0;
	char	*string;
	int	configNum;
	int	interfaceNum;
	int	endpointNum;
	
	device = FindDevice (deviceNumber);
	if (device == NULL) {
		printf ("Can't seem to find device info to display\n");
		return;
		}

	/* clear the textbox */
	gtk_editable_delete_text (GTK_EDITABLE(textDescription), 0, -1);

	string = (char *)g_malloc (1000);

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

	/* add the USB version, device class, subclass, protocol, max packet size, and the number of configurations (if it is there) */
	if (device->version) {
		sprintf (string, "\nUSB Version: %s\nDevice Class: %s\nDevice Subclass: %s\nDevice Protocol: %s\n"
			 "Maximum Default Endpoint Size: %i\nNumber of Configurations: %i",
			 device->version, device->class, device->subClass, device->protocol,
			 device->maxPacketSize, device->numConfigs);
		gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
		}

	/* add the vendor id, product id, and revision number (if it is there) */
	if (device->vendorId) {
		sprintf (string, "\nVendor Id: %s\nProduct Id: %s\nRevision Number: %s",
			 device->vendorId, device->productId, device->revisionNumber);
		gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
		}

	/* display all the info for the configs */
	for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
		if (device->config[configNum]) {
			DeviceConfig	*config = device->config[configNum];
			
			/* show this config */
			sprintf (string, "\n\nConfig Number: %i\n\tNumber of Interfaces: %i\n\t"
				"Attributes: %s\n\tMaxPower Needed: %s",
				config->configNumber, config->numInterfaces, 
				config->attributes, config->maxPower);
			gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);

			/* show all of the interfaces for this config */
			for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
				if (config->interface[interfaceNum]) {
					DeviceInterface	*interface = config->interface[interfaceNum];
					
					sprintf (string, "\n\n\tInterface Number: %i\n\t\tAlternate Number: %i\n\t\t"
						"Class: %s\n\t\tSub Class: %s\n\t\tProtocol: %s\n\t\tNumber of Endpoints: %i",
						interface->interfaceNumber, interface->alternateNumber, 
						interface->class, interface->subClass, interface->protocol, interface->numEndpoints);
					gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);

					/* show all of the endpoints for this interface */
					for (endpointNum = 0; endpointNum < MAX_ENDPOINTS; ++endpointNum) {
						if (interface->endpoint[endpointNum]) {
							DeviceEndpoint	*endpoint = interface->endpoint[endpointNum];
							
							sprintf (string, "\n\n\t\t\tEndpoint Address: %i\n\t\t\t"
								"Direction: %s\n\t\t\tAttribute: %i\n\t\t\t"
								"Type: %s\n\t\t\tMax Packet Size: %s\n\t\t\tInterval: %s",
								endpoint->address, 
								endpoint->in ? "in" : "out", endpoint->attribute,
								endpoint->type, endpoint->maxPacketSize, endpoint->interval);
							gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
							}
						}
					}
				}
			}
		}

	g_free (string);

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
			GetDeviceInformation (lastDevice, line);
			break;

		case 'P': /* more device information */
			GetMoreDeviceInformation (lastDevice, line);
			break;

		case 'C': /* config descriptor info */
			AddConfig (lastDevice, line);
			break;
			
		case 'I': /* interface descriptor info */
			AddInterface (lastDevice, line);
			break;
			
		case 'E': /* endpoint descriptor info */
			AddEndpoint (lastDevice, line);
			break;
			
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

	dataLine = (char *)g_malloc (MAX_LINE_SIZE);
	while (!finished) {
		/* read the line in from the file */
		fgets (dataLine, MAX_LINE_SIZE, usbFile);

		if (dataLine[strlen(dataLine)-1] == '\n')
			ParseLine (dataLine);
		
		if (feof (usbFile))
			finished = 1;
		}

	g_free (dataLine);

	DisplayDevice (rootDevice, rootDevice->child[0]);
	
	
	return;
}
