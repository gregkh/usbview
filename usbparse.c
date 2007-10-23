/*************************************************************************
** usbparse.c for USBView - a USB device viewer
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

#include "usbtree.h"
#include "usbparse.h"		/* all of the usb structure definitions */

#define MAX_LINE_SIZE	1000


Device          *rootDevice = NULL;
static  Device          *lastDevice = NULL;
static  DeviceBandwidth *currentBandwidth = NULL;


static signed int GetInt (char *string, char *pattern, int base)
{
	char    *pointer;

	pointer = strstr (string, pattern);
	if (pointer == NULL)
		return(0);

	pointer += strlen(pattern);
	return((int)(strtol (pointer, NULL, base)));
}

/*  uncomment this if we ever need to read a float
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
	char    *pointer;

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
	int     i;

	if (interface == NULL)
		return;

	for (i = 0; i < MAX_ENDPOINTS; ++i)
		DestroyEndpoint (interface->endpoint[i]);

	g_free (interface->name);
	g_free (interface->class);

	g_free (interface);

	return;
}


static void DestroyConfig (DeviceConfig *config)
{
	int     i;

	if (config == NULL)
		return;

	for (i = 0; i < MAX_INTERFACES; ++i)
		DestroyInterface (config->interface[i]);

	g_free (config->maxPower);

	g_free (config);

	return;
}


static void DestroyBandwidth (DeviceBandwidth *bandwidth)
{
	/* nothing dynamic in the bandwidth structure yet. */
	return;
}


static void DestroyDevice (Device *device)
{
	int     i;

	if (device == NULL)
		return;

	for (i = 0; i < MAX_CHILDREN; ++i)
		DestroyDevice (device->child[i]);

	for (i = 0; i < MAX_CONFIGS; ++i)
		DestroyConfig (device->config[i]);

	if (device->bandwidth != NULL) {
		DestroyBandwidth (device->bandwidth);
		g_free (device->bandwidth);
	}

	g_free (device->name);
	g_free (device->version);
	g_free (device->class);
	g_free (device->subClass);
	g_free (device->protocol);
	g_free (device->revisionNumber);
	g_free (device->manufacturer);
	g_free (device->product);
	g_free (device->serialNumber);

	g_free (device);

	return;
}




static Device *FindDeviceNode (Device *device, int deviceNumber, int busNumber)
{
	int     i;
	Device  *result;

	if (device == NULL)
		return(NULL);

	if ((device->deviceNumber == deviceNumber) &&
	    (device->busNumber == busNumber))
		return(device);

	for (i = 0; i < MAX_CHILDREN; ++i) {
		result = FindDeviceNode (device->child[i], deviceNumber, busNumber);
		if (result != NULL)
			return(result);
	}

	return(NULL);
}


Device *usb_find_device (int deviceNumber, int busNumber)
{
	int     i;
	Device  *result;

	/* search through the tree to try to find a device */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		result = FindDeviceNode (rootDevice->child[i], deviceNumber, busNumber);
		if (result != NULL)
			return(result);
	}
	return(NULL);
}


static Device *AddDevice (char *line)
{
	Device  *device;

	/* create a new device */
	device = (Device *)(g_malloc0 (sizeof(Device)));

	/* parse the line */
	device->busNumber       = GetInt (line, TOPOLOGY_BUS_STRING, 10);
	device->level           = GetInt (line, TOPOLOGY_LEVEL_STRING, 10);
	device->parentNumber    = GetInt (line, TOPOLOGY_PARENT_STRING, 10);
	device->portNumber      = GetInt (line, TOPOLOGY_PORT_STRING, 10);
	device->count           = GetInt (line, TOPOLOGY_COUNT_STRING, 10);
	device->deviceNumber    = GetInt (line, TOPOLOGY_DEVICENUMBER_STRING, 10);
	device->speed           = GetInt (line, TOPOLOGY_SPEED_STRING, 10);
	device->maxChildren     = GetInt (line, TOPOLOGY_MAXCHILDREN_STRING, 10);

	if (device->deviceNumber == -1)
		device->deviceNumber = 0;

	/* Set up the parent / child relationship */
	if (device->level == 0) {
		/* this is the root, don't go looking for a parent */
		device->parent = rootDevice;
		++rootDevice->maxChildren;
		rootDevice->child[rootDevice->maxChildren-1] = device;
	} else {
		/* need to find this device's parent */
		device->parent = usb_find_device (device->parentNumber, device->busNumber);
		if (device->parent == NULL) {
			printf ("can't find parent...not good.\n");
		}
		device->parent->child[device->portNumber] = device;
	}

	return(device);
}





static void GetDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;

	g_free (device->version);
	g_free (device->class);
	g_free (device->subClass);
	g_free (device->protocol);

	device->version         = (char *)g_malloc0 ((DEVICE_VERSION_SIZE) * sizeof(char));
	device->class           = (char *)g_malloc0 ((DEVICE_CLASS_SIZE) * sizeof(char));
	device->subClass        = (char *)g_malloc0 ((DEVICE_SUBCLASS_SIZE) * sizeof(char));
	device->protocol        = (char *)g_malloc0 ((DEVICE_PROTOCOL_SIZE) * sizeof(char));

	GetString (device->version, data, DEVICE_VERSION_STRING, DEVICE_VERSION_SIZE-1);
	GetString (device->class, data, DEVICE_CLASS_STRING, DEVICE_CLASS_SIZE-1);
	GetString (device->subClass, data, DEVICE_SUBCLASS_STRING, DEVICE_SUBCLASS_SIZE-1);
	GetString (device->protocol, data, DEVICE_PROTOCOL_STRING, DEVICE_PROTOCOL_SIZE-1);

	device->maxPacketSize   = GetInt (data, DEVICE_MAXPACKETSIZE_STRING, 10);
	device->numConfigs      = GetInt (data, DEVICE_NUMCONFIGS_STRING, 10);

	return;
}



static void GetMoreDeviceInformation (Device *device, char *data)
{
	if (device == NULL)
		return;

	g_free (device->revisionNumber);

	device->vendorId        = GetInt (data, DEVICE_VENDOR_STRING, 16);
	device->productId       = GetInt (data, DEVICE_PRODUCTID_STRING, 16);

	device->revisionNumber  = (char *)g_malloc0 ((DEVICE_REVISION_NUMBER_SIZE) * sizeof(char));
	GetString (device->revisionNumber, data, DEVICE_REVISION_STRING, DEVICE_REVISION_NUMBER_SIZE);

	return;
}


static void GetDeviceString (Device *device, char *data)
{
	if (device == NULL)
		return;

	if (strstr (data, DEVICE_MANUFACTURER_STRING) != NULL) {
		g_free (device->manufacturer);
		device->manufacturer = (gchar *)g_malloc0 (DEVICE_STRING_MAXSIZE);
		GetString (device->manufacturer, data, DEVICE_MANUFACTURER_STRING, DEVICE_STRING_MAXSIZE-1);
		return;
	}

	if (strstr (data, DEVICE_PRODUCT_STRING) != NULL) {
		g_free (device->product);
		device->product = (gchar *)g_malloc0 (DEVICE_STRING_MAXSIZE);
		GetString (device->product, data, DEVICE_PRODUCT_STRING, DEVICE_STRING_MAXSIZE-1);
		return;
	}

	if (strstr (data, DEVICE_SERIALNUMBER_STRING) != NULL) {
		g_free (device->serialNumber);
		device->serialNumber = (gchar *)g_malloc0 (DEVICE_STRING_MAXSIZE);
		GetString (device->serialNumber, data, DEVICE_SERIALNUMBER_STRING, DEVICE_STRING_MAXSIZE-1);
		return;
	}

	return;
}


static void GetBandwidth (Device *device, char *data)
{
	DeviceBandwidth *bandwidth;

	if (device == NULL)
		return;

	bandwidth = (DeviceBandwidth *)g_malloc0 (sizeof(DeviceBandwidth));

	bandwidth->allocated            = GetInt (data, BANDWIDTH_ALOCATED, 10);
	bandwidth->total                = GetInt (data, BANDWIDTH_TOTAL, 10);
	bandwidth->percent              = GetInt (data, BANDWIDTH_PERCENT, 10);
	bandwidth->numInterruptRequests = GetInt (data, BANDWIDTH_INTERRUPT_TOTAL, 10);
	bandwidth->numIsocRequests      = GetInt (data, BANDWIDTH_ISOC_TOTAL, 10);

	device->bandwidth = bandwidth;

	return;
}


static void AddConfig (Device *device, char *data)
{
	DeviceConfig    *config;
	int             i;

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
	config->maxPower        = (gchar *)g_malloc0 ((CONFIG_MAXPOWER_SIZE) * sizeof(gchar));

	config->numInterfaces   = GetInt (data, CONFIG_NUMINTERFACES_STRING, 10);
	config->configNumber    = GetInt (data, CONFIG_CONFIGNUMBER_STRING, 10);
	config->attributes      = GetInt (data, CONFIG_ATTRIBUTES_STRING, 16);

	GetString (config->maxPower, data, CONFIG_MAXPOWER_STRING, CONFIG_MAXPOWER_SIZE);

	/* have the device now point to this config */
	device->config[i] = config;

	return;
}


static void AddInterface (Device *device, char *data)
{
	DeviceConfig    *config;
	DeviceInterface *interface;
	int             i;
	int             configNum;

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

	interface->class        = (gchar *)g_malloc0 ((INTERFACE_CLASS_SIZE) * sizeof(gchar));
	interface->name         = (gchar *)g_malloc0 ((INTERFACE_DRIVERNAME_STRING_MAXLENGTH) * sizeof(gchar));

	interface->interfaceNumber      = GetInt (data, INTERFACE_NUMBER_STRING, 10);
	interface->alternateNumber      = GetInt (data, INTERFACE_ALTERNATESETTING_STRING, 10);
	interface->numEndpoints         = GetInt (data, INTERFACE_NUMENDPOINTS_STRING, 10);
	interface->subClass             = GetInt (data, INTERFACE_SUBCLASS_STRING, 10);
	interface->protocol             = GetInt (data, INTERFACE_PROTOCOL_STRING, 10);

	GetString (interface->class, data, INTERFACE_CLASS_STRING, INTERFACE_CLASS_SIZE);
	GetString (interface->name, data, INTERFACE_DRIVERNAME_STRING, INTERFACE_DRIVERNAME_STRING_MAXLENGTH);

	/* if this interface does not have a driver attached to it, save that info for later */
	if (strncmp(interface->name, INTERFACE_DRIVERNAME_NODRIVER_STRING, INTERFACE_DRIVERNAME_STRING_MAXLENGTH) == 0) {
		interface->driverAttached = FALSE;
	} else {
		interface->driverAttached = TRUE;
	}

	/* now point the config to this interface */
	config->interface[i] = interface;

	return;
}


static void AddEndpoint (Device *device, char *data)
{
	DeviceConfig    *config;
	DeviceInterface *interface;
	DeviceEndpoint  *endpoint;
	int             i;
	int             configNum;
	int             interfaceNum;

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

	endpoint->type          = (gchar *)g_malloc0 ((ENDPOINT_TYPE_SIZE) * sizeof(gchar));
	endpoint->interval      = (gchar *)g_malloc0 ((ENDPOINT_INTERVAL_SIZE) * sizeof(gchar));

	endpoint->address       = GetInt (data, ENDPOINT_ADDRESS_STRING, 16);
	if (data[10] == 'I')
		endpoint->in = TRUE;
	else
		endpoint->in = FALSE;
	endpoint->attribute     = GetInt (data, ENDPOINT_ATTRIBUTES_STRING, 16);
	endpoint->maxPacketSize = GetInt (data, ENDPOINT_MAXPACKETSIZE_STRING, 10);

	GetString (interface->class, data, INTERFACE_CLASS_STRING, INTERFACE_CLASS_SIZE);

	memcpy (endpoint->type, &data[20], ENDPOINT_TYPE_SIZE-1);
	GetString (endpoint->interval, data, ENDPOINT_INTERVAL_STRING, ENDPOINT_INTERVAL_SIZE);

	/* point the interface to the endpoint */
	interface->endpoint[i] = endpoint;
}


/* Build all of the names of the devices */
static void NameDevice (Device *device)
{
	int     configNum;
	int     interfaceNum;
	int     i;

	if (device == NULL)
		return;

	/* build the name for this device */
	if (device != rootDevice) {
		if (device->name)
			g_free (device->name);
		device->name = (gchar *)g_malloc0 (DEVICE_STRING_MAXSIZE*sizeof (gchar));

		/* see if this device has a product name */
		if (device->product != NULL) {
			strcpy (device->name, device->product);
			goto create_children_names;
		}

		/* see if this device is a root hub */
		if (device->level == 0) {
			strcpy (device->name, "root hub");
			goto create_children_names;
		}

		/* look through all of the interfaces of this device, adding them all up to form a name */
		for (configNum = 0; configNum < MAX_CONFIGS; ++configNum) {
			if (device->config[configNum]) {
				DeviceConfig    *config = device->config[configNum];
				for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
					if (config->interface[interfaceNum]) {
						DeviceInterface *interface = config->interface[interfaceNum];
						if (interface->name != NULL) {
							if (strstr (interface->name, "none") == NULL) {
								if (strcmp (interface->name, "hid") == 0) {
									if (interface->subClass == 1) {
										switch (interface->protocol) {
											case 1 :
												strcat (device->name, "keyboard");
												continue;
											case 2 :
												strcat (device->name, "mouse");
												continue;
										}
									}
								}
								if (strstr (device->name, interface->name) == NULL) {
									if (strlen (device->name) > 0) {
										strcat (device->name, " / ");
									}
									strcat (device->name, interface->name);
								}
							}
						}
					}
				}
			}
		}

		if (strlen (device->name) == 0) {
			strcat (device->name, "Unknown Device");
		}

	}

	create_children_names:

	/* create all of the children's names */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		NameDevice (device->child[i]);
	}

	return;
}


void usb_initialize_list (void)
{
	if (rootDevice != NULL) {
		DestroyDevice (rootDevice);
		rootDevice = NULL;
	}

	rootDevice = (Device *)g_malloc0 (sizeof(Device));

	/* clean up any bandwidth devices */
	if (currentBandwidth != NULL) {
		DestroyBandwidth (currentBandwidth);
		g_free (currentBandwidth);
	}

	lastDevice = NULL;

	return;
}


void usb_parse_line (char * line)
{
	/* chop off the trailing \n */
	line[strlen(line)-1] = 0x00;

	/* look at the first character to see what kind of line this is */
	switch (line[0]) {
		case 'T': /* topology */
			lastDevice = AddDevice (line);
			break;

		case 'B': /* bandwidth */
			GetBandwidth (lastDevice, line);
			break;

		case 'D': /* device information */
			GetDeviceInformation (lastDevice, line);
			break;

		case 'P': /* more device information */
			GetMoreDeviceInformation (lastDevice, line);
			break;

		case 'S': /* device string information */
			GetDeviceString (lastDevice, line);
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


void usb_name_devices (void)
{
	NameDevice (rootDevice);
}
