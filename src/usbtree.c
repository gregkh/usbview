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


#define TOPOLOGY_LEVEL_STRING			"Lev="
#define TOPOLOGY_PARENT_STRING			"Prnt="
#define TOPOLOGY_PORT_STRING			"Port="
#define TOPOLOGY_COUNT_STRING			"Cnt="
#define TOPOLOGY_DEVICENUMBER_STRING		"Dev#="
#define TOPOLOGY_SPEED_STRING			"Spd="
#define TOPOLOGY_INTERFACENUMBER_STRING		"If#="
#define TOPOLOGY_MAXCHILDREN_STRING		"MxCh="
#define TOPOLOGY_DRIVERNAME_STRING		"Driver="
#define TOPOLOGY_DRIVERNAME_STRING_MAXLENGTH	50

#define DEVICE_VERSION_STRING			"Ver="
#define DEVICE_CLASS_STRING			"Cls="
#define DEVICE_SUBCLASS_STRING			"Sub="
#define DEVICE_PROTOCOL_STRING			"Prot="
#define DEVICE_MAXPACKETSIZE_STRING		"MxPS="
#define DEVICE_NUMCONFIGS_STRING		"#Cfgs="
#define DEVICE_VENDOR_STRING			"Vendor="
#define DEVICE_PRODUCTID_STRING			"ProdID="
#define DEVICE_REVISION_STRING			"Rev="

#define CONFIG_NUMINTERFACES_STRING		"#Ifs="
#define CONFIG_CONFIGNUMBER_STRING		"Cfg#="
#define CONFIG_ATTRIBUTES_STRING		"Atr="
#define CONFIG_MAXPOWER_STRING			"MxPwr="

#define INTERFACE_NUMBER_STRING			"If#="
#define INTERFACE_ALTERNATESETTING_STRING	"Alt="
#define INTERFACE_NUMENDPOINTS_STRING		"#EPs="
#define INTERFACE_CLASS_STRING			"Cls="
#define INTERFACE_SUBCLASS_STRING		"Sub="
#define INTERFACE_PROTOCOL_STRING		"Prot="

#define ENDPOINT_ADDRESS_STRING			"Ad="
#define ENDPOINT_ATTRIBUTES_STRING		"Atr="
#define ENDPOINT_MAXPACKETSIZE_STRING		"MxPS="
#define ENDPOINT_INTERVAL_STRING		"Ivl="










typedef struct DeviceEndpoint {
	gint		address;
	gboolean	in;		/* TRUE if in, FALSE if out */
	gint		attribute;
	gchar		*type;
	gint		maxPacketSize;
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
	gint		attributes;
	gchar		*maxPower;
	DeviceInterface	*interface[MAX_INTERFACES];
} DeviceConfig;


typedef struct Device {
	gchar		*name;
	gint		level;
	gint		parentNumber;
	gint		portNumber;
	gint		connectorNumber;
	gint		count;
	gint		deviceNumber;
	gint		speed;
	gint		interfaceNumber;
	gint		maxChildren;
	gchar		*version;
	gchar		*class;
	gchar		*subClass;
	gchar		*protocol;
	gint		maxPacketSize;
	gint		numConfigs;
	gint		vendorId;
	gint		productId;
	gchar		*revisionNumber;
	DeviceConfig	*config[MAX_CONFIGS];
	struct Device	*parent;
	struct Device	*child[MAX_CHILDREN];
	GtkWidget	*tree;
	GtkWidget	*leaf;
} Device;

	


static Device	*rootDevice = NULL;
static Device	*lastDevice;


static signed int GetInt (char *string, char *pattern, int base)
{
	char	*pointer;

	pointer = strstr (string, pattern);
	if (pointer == NULL)
		return (0);

	pointer += strlen(pattern);
	return ((int)(strtol (pointer, NULL, base)));
}

/*
static double GetFloat (char *string, char *pattern)
{
	char	*pointer;

	pointer = strstr (string, pattern);
	if (pointer == NULL)
		return (0.0);

	pointer += strlen(pattern);
	return (strtod (pointer, NULL));
}
*/


static void GetString (char *dest, char *source, char *pattern, int maxSize)
{
	char	*pointer;

	pointer = strstr (source, pattern);
	if (pointer == NULL)
		return;

	pointer += strlen(pattern);
	strncpy (dest, pointer, maxSize);
	return;
}


static void DestroyEndpoint (DeviceEndpoint *endpoint)
{
	if (endpoint == NULL)
		return;
		
	g_free (endpoint->type);
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
	
	/* create a new device */
	device = (Device *)(g_malloc0 (sizeof(Device)));
	
	/* parse the line */
	device->level		= GetInt (line, TOPOLOGY_LEVEL_STRING, 10);
	device->parentNumber	= GetInt (line, TOPOLOGY_PARENT_STRING, 10);
	device->portNumber	= GetInt (line, TOPOLOGY_PORT_STRING, 10);
	device->count		= GetInt (line, TOPOLOGY_COUNT_STRING, 10);
	device->deviceNumber	= GetInt (line, TOPOLOGY_DEVICENUMBER_STRING, 10);
	device->speed		= GetInt (line, TOPOLOGY_SPEED_STRING, 10);
	device->interfaceNumber	= GetInt (line, TOPOLOGY_INTERFACENUMBER_STRING, 10);
	device->maxChildren	= GetInt (line, TOPOLOGY_MAXCHILDREN_STRING, 10);
	
	device->name = (char *)g_malloc0 (TOPOLOGY_DRIVERNAME_STRING_MAXLENGTH);
	GetString (device->name, line, TOPOLOGY_DRIVERNAME_STRING, TOPOLOGY_DRIVERNAME_STRING_MAXLENGTH);
	
	if (device->deviceNumber == -1)
		device->deviceNumber = 0;

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
		
	g_free (device->version);
	g_free (device->class);
	g_free (device->subClass);
	g_free (device->protocol);

	device->version		= (char *)g_malloc0 ((DEVICE_VERSION_SIZE) * sizeof(char));
	device->class		= (char *)g_malloc0 ((DEVICE_CLASS_SIZE) * sizeof(char));
	device->subClass	= (char *)g_malloc0 ((DEVICE_SUBCLASS_SIZE) * sizeof(char));
	device->protocol	= (char *)g_malloc0 ((DEVICE_PROTOCOL_SIZE) * sizeof(char));

	GetString (device->version, data, DEVICE_VERSION_STRING, DEVICE_VERSION_SIZE-1);
	GetString (device->class, data, DEVICE_CLASS_STRING, DEVICE_CLASS_SIZE-1);
	GetString (device->subClass, data, DEVICE_SUBCLASS_STRING, DEVICE_SUBCLASS_SIZE-1);
	GetString (device->protocol, data, DEVICE_PROTOCOL_STRING, DEVICE_PROTOCOL_SIZE-1);
	
	device->maxPacketSize	= GetInt (data, DEVICE_MAXPACKETSIZE_STRING, 10);
	device->numConfigs	= GetInt (data, DEVICE_NUMCONFIGS_STRING, 10);

	return;
}



static void GetMoreDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;
		
	g_free (device->revisionNumber);
	
	device->vendorId	= GetInt (data, DEVICE_VENDOR_STRING, 16);
	device->productId	= GetInt (data, DEVICE_PRODUCTID_STRING, 16);

	device->revisionNumber	= (char *)g_malloc0 ((DEVICE_REVISION_NUMBER_SIZE) * sizeof(char));
	GetString (device->revisionNumber, data, DEVICE_REVISION_STRING, DEVICE_REVISION_NUMBER_SIZE);

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
	config->maxPower	= (gchar *)g_malloc0 ((CONFIG_MAXPOWER_SIZE) * sizeof(gchar));

	config->numInterfaces	= GetInt (data, CONFIG_NUMINTERFACES_STRING, 10);
	config->configNumber	= GetInt (data, CONFIG_CONFIGNUMBER_STRING, 10);
	config->attributes	= GetInt (data, CONFIG_ATTRIBUTES_STRING, 16);

	GetString (config->maxPower, data, CONFIG_MAXPOWER_STRING, CONFIG_MAXPOWER_SIZE);

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

	interface->interfaceNumber	= GetInt (data, INTERFACE_NUMBER_STRING, 10);
	interface->alternateNumber	= GetInt (data, INTERFACE_ALTERNATESETTING_STRING, 10);
	interface->numEndpoints		= GetInt (data, INTERFACE_NUMENDPOINTS_STRING, 10);

	GetString (interface->class, data, INTERFACE_CLASS_STRING, INTERFACE_CLASS_SIZE);
	GetString (interface->subClass, data, INTERFACE_SUBCLASS_STRING, INTERFACE_SUBCLASS_SIZE);
	GetString (interface->protocol, data, INTERFACE_PROTOCOL_STRING, INTERFACE_PROTOCOL_SIZE);

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
	endpoint->interval	= (gchar *)g_malloc0 ((ENDPOINT_INTERVAL_SIZE) * sizeof(gchar));

	endpoint->address	= GetInt (data, ENDPOINT_ADDRESS_STRING, 16);
	if (data[10] == 'I')
		endpoint->in = TRUE;
	else
		endpoint->in = FALSE;
	endpoint->attribute	= GetInt (data, ENDPOINT_ATTRIBUTES_STRING, 16);
	endpoint->maxPacketSize	= GetInt (data, ENDPOINT_MAXPACKETSIZE_STRING, 10);
	
	GetString (interface->class, data, INTERFACE_CLASS_STRING, INTERFACE_CLASS_SIZE);

	memcpy (endpoint->type, &data[20], ENDPOINT_TYPE_SIZE-1);
	GetString (endpoint->interval, data, ENDPOINT_INTERVAL_STRING, ENDPOINT_INTERVAL_SIZE);

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

	/* add speed */
	sprintf (string, "\nSpeed: %iMb/s", device->speed);
	gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);

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
		sprintf (string, "\nVendor Id: %.4x\nProduct Id: %.4x\nRevision Number: %s",
			 device->vendorId, device->productId, device->revisionNumber);
		gtk_editable_insert_text (GTK_EDITABLE(textDescription), string, strlen(string), &position);
		}

	/* display all the info for the configs */
	for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
		if (device->config[configNum]) {
			DeviceConfig	*config = device->config[configNum];
			
			/* show this config */
			sprintf (string, "\n\nConfig Number: %i\n\tNumber of Interfaces: %i\n\t"
				"Attributes: %.2x\n\tMaxPower Needed: %s",
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
							
							sprintf (string, "\n\n\t\t\tEndpoint Address: %.2x\n\t\t\t"
								"Direction: %s\n\t\t\tAttribute: %i\n\t\t\t"
								"Type: %s\n\t\t\tMax Packet Size: %i\n\t\t\tInterval: %s",
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

	/* throw the cursor back to the top so the user sees the top first */
	gtk_editable_set_position (GTK_EDITABLE(textDescription), 0);

	/* clean up our string */
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
		gtk_tree_item_expand (GTK_TREE_ITEM(device->leaf));	/* make the tree expanded to start with */
		}
	
	/* create all of the children's leafs */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		DisplayDevice (device, device->child[i]);
		}

	return;
}
	

static void ParseLine (char * line)
{
	/* chop off the trailing \n */
	line[strlen(line)-1] = 0x00;

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
