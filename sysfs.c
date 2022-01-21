// SPDX-License-Identifier: GPL-2.0-only
/*
 * usbparse.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000 by Greg Kroah-Hartman, <greg@kroah.com>
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "usbtree.h"
#include "sysfs.h"		/* all of the usb structure definitions */
#include "ccan/list/list.h"

#define MAX_LINE_SIZE	1000


Device          *rootDevice = NULL;
static  Device          *lastDevice = NULL;
static  DeviceBandwidth *currentBandwidth = NULL;


/* taken from all-io.h from util-linux repo */
static inline ssize_t read_all(int fd, char *buf, size_t count)
{
	ssize_t ret;
	ssize_t c = 0;
	int tries = 0;

	while (count > 0) {
		ret = read(fd, buf, count);
		if (ret <= 0) {
			if (ret < 0 && (errno == EAGAIN || errno == EINTR) &&
			    (tries++ < 5)) {
				usleep(250000);
				continue;
			}
			return c ? c : -1;
		}
		tries = 0;
		count -= ret;
		buf += ret;
		c += ret;
	}
	return c;
}

static int openreadclose(const char *filename, char *buffer, size_t bufsize)
{
	ssize_t count;
	int fd;

	fd = openat(0, filename, O_RDONLY);
	if (fd < 0) {
		printf("error opening %s\n", filename);
		return fd;
	}

	count = read_all(fd, buffer, bufsize);
	if (count < 0) {
		printf("Error %ld reading from %s\n", count, filename);
	}

	close(fd);
	return count;
}

static int check_file_present(const char *filename)
{
	struct stat sb;
	int retval;

	retval = stat(filename, &sb);
	if (retval == -1) {
		//fprintf(stderr,
		//	"filename %s is not present\n", filename);
		return retval;
	}

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr,
			"filename %s must be a real file, not anything else.\n",
			filename);
		return -1;
	}
	return 0;
}

static int check_dir_present(const char *filename)
{
	struct stat sb;
	int retval;

	retval = stat(filename, &sb);
	if (retval == -1) {
//		fprintf(stderr,
//			"filename %s is not present\n", filename);
		return retval;
	}

	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr,
			"filename %s must be a directory, not anything else.\n",
			filename);
		return -1;
	}
	return 0;
}

static char *sysfs_string(const char *directory, const char *filename)
{
	char full_filename[PATH_MAX];
	char buffer[256];
	char *temp;
	int retval;

	snprintf(full_filename, PATH_MAX, "%s/%s", directory, filename);

	if (check_file_present(full_filename))
		return NULL;

	retval = openreadclose(full_filename, buffer, sizeof(buffer));
	if (retval <= 0)
		return NULL;

	/* strip trailing \n off */
	buffer[retval-1] = 0x00;
	temp = g_strdup(buffer);
	return temp;
}

static int sysfs_int(const char *dir, const char *filename, int base)
{
	char *string = sysfs_string(dir, filename);

	if (!string)
		return 0;

	int value = strtol(string, NULL, base);

	g_free(string);
	return value;
}

#if 0
static signed int GetInt (char *string, char *pattern, int base)
{
	char    *pointer;

	pointer = strstr (string, pattern);
	if (pointer == NULL)
		return(0);

	pointer += strlen(pattern);
	return((int)(strtol (pointer, NULL, base)));
}
#endif

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

#if 0
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
#endif

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

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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

	interface->class        = (gchar *)g_malloc0 ((INTERFACE_CLASS_SIZE + 1) * sizeof(gchar));
	interface->name         = (gchar *)g_malloc0 ((INTERFACE_DRIVERNAME_STRING_MAXLENGTH + 1) * sizeof(gchar));

	interface->interfaceNumber      = GetInt (data, INTERFACE_NUMBER_STRING, 10);
	interface->alternateNumber      = GetInt (data, INTERFACE_ALTERNATESETTING_STRING, 10);
	interface->numEndpoints         = GetInt (data, INTERFACE_NUMENDPOINTS_STRING, 10);
	interface->subClass             = GetInt (data, INTERFACE_SUBCLASS_STRING, 16);
	interface->protocol             = GetInt (data, INTERFACE_PROTOCOL_STRING, 16);

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
#endif

#if 0
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
#endif

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

struct entity {
	struct list_node list;
	char *name;
};

static struct entity *entity_create(char *name)
{
	struct entity *e;

	e = malloc(sizeof(*e));
	memset(e, 0x00, sizeof(*e));
	e->name = g_strdup(name);
	return e;
}

static void entity_destroy(struct entity *e)
{
	list_del(&e->list);
	g_free(e->name);
	g_free(e);
}

static void entity_add(struct list_head *list, struct entity *e)
{
	struct entity *iter;

	list_for_each(list, iter, list) {
		if (strcmp(iter->name, e->name) > 0) {
			list_add_before(list, &iter->list, &e->list);
			goto exit;
		}
	}
	list_add_tail(list, &e->list);

exit:
#if 0
	printf("list = ");
	list_for_each(list, iter, list) {
		printf("%s ", iter->name);
	}
	printf("\n");
#endif
	return;
}

static void endpoint_parse(struct DeviceInterface *interface, const char *dir)
{
//	DeviceConfig    *config;
	DeviceEndpoint  *endpoint;
	int             i;
//	int             configNum;
//	int             interfaceNum;

	/* find a place in this interface to place the endpoint */
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

	endpoint->attribute	= sysfs_int(dir, "bmAttributes", 16);
	endpoint->maxPacketSize	= sysfs_int(dir, "wMaxPacketSize", 16);

	char *epdir	= sysfs_string(dir, "direction");
	if (!strcmp(epdir, "in"))
		endpoint->in = TRUE;
	else
		endpoint->in = FALSE;
	g_free(epdir);

	endpoint->type		= sysfs_string(dir, "type");
	endpoint->interval	= sysfs_string(dir, "interval");
	endpoint->address	= sysfs_int(dir, "bEndpointAddress", 16);

	/* point the interface to the endpoint */
	interface->endpoint[i] = endpoint;
#if 0
	char *direction;
	char *type;

	char *addr	= sysfs_read(dir, "bEndpointAddress");
	char *eptype	= sysfs_read(dir, "type");
	char *interval	= sysfs_read(dir, "interval");

	type = eptype;
	if (!strcmp(eptype, "Control"))
		type = "Ctrl";
	else if (!strcmp(eptype, "Interrupt"))
		type = "Int.";

	char *maxps_hex = sysfs_read(dir, "wMaxPacketSize");
	int maxps_num = strtol(maxps_hex, NULL, 16);

	/* max packet size is bits 0-10 with multiplicity values in bits 11 and 12 */
	maxps_num = (maxps_num & 0x7ff) * (1 + ((maxps_num >> 11) & 0x03));

	printf("E:  Ad=%s(%s) Atr=%s(%s) MxPS=%4d Ivl=%s\n",
	       addr, direction, attr, type, maxps_num, interval);
#endif
}

static void endpoints_parse(struct DeviceInterface *interface, const char *dir)
{
	DIR *d;
	struct dirent *de;
	LIST_HEAD(endpoint_list);
	struct entity *temp;
	struct entity *e;

	d = opendir(dir);
	if (!d) {
		fprintf(stderr, "Can not open %s directory\n", dir);
		return;
	}

	while ((de = readdir(d))) {
		if (de->d_type == DT_DIR) {
			/* See if this directory is an interface or not */
			char endpoint[PATH_MAX];

			snprintf(endpoint, PATH_MAX, "%s/%s", dir, de->d_name);
			char *endpointaddr = sysfs_string(endpoint, "bEndpointAddress");
			if (endpointaddr) {
				e = entity_create(endpoint);
				entity_add(&endpoint_list, e);
				g_free(endpointaddr);
			}

		}
	}

	closedir(d);

	list_for_each_safe(&endpoint_list, e, temp, list) {
		endpoint_parse(interface, e->name);
		entity_destroy(e);
	}
}

static void interface_parse(struct Device *device, const char *dir)
{
	DeviceConfig    *config;
	DeviceInterface *interface;
	int             i;
	int             configNum;

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

	interface->interfaceNumber	= sysfs_int(dir, "bInterfaceNumber", 10);
	interface->alternateNumber	= sysfs_int(dir, "bAlternateSetting", 10);
	interface->numEndpoints		= sysfs_int(dir, "bNumEndpoints", 10);
	interface->subClass		= sysfs_int(dir, "bInterfaceSubClass", 16);
	interface->protocol		= sysfs_int(dir, "bInterfaceProtocol", 16);

	interface->class		= sysfs_string(dir, "bInterfaceClass");

	char drivername[PATH_MAX];
	char link[PATH_MAX];
	struct stat sb;
	int retval;
	char *driver = "(none)";

	/*
	 * find the driver name by looking at the basename of the driver
	 * symbolic link.
	 * In bash this would just be:
	 *	driver=`readlink $ifpath/driver`
	 *	driver=`basename "$driver"`
	 */
	snprintf(drivername, PATH_MAX, "%s/driver", dir);
	retval = stat(drivername, &sb);
	memset(link, 0x00, sizeof(link));
	if (!retval) {
		retval = readlink(drivername, link, sizeof(link));
		if (retval > 0) {
			/*
			 * crude 'basename', go to the end of the string and
			 * walk back to find a '/' and then go forward one.
			 */
			driver = &link[strlen(link)];
			while (driver[0] != '/')
				driver--;
			driver++;
		}
	}

	interface->name = g_strdup(driver);

	/* if this interface does not have a driver attached to it, save that info for later */
	if (strncmp(interface->name, INTERFACE_DRIVERNAME_NODRIVER_STRING, INTERFACE_DRIVERNAME_STRING_MAXLENGTH) == 0) {
		interface->driverAttached = FALSE;
	} else {
		interface->driverAttached = TRUE;
	}

	/* now point the config to this interface */
	config->interface[i] = interface;

	endpoints_parse(interface, dir);
}

static void interfaces_parse(struct Device *device, const char *dir)
{
	DIR *d;
	struct dirent *de;
	LIST_HEAD(interface_list);
	struct entity *temp;
	struct entity *e;

	d = opendir(dir);
	if (!d) {
		fprintf(stderr, "Can not open %s directory\n", dir);
		return;
	}

	while ((de = readdir(d))) {
		if (de->d_type == DT_DIR) {
			/* See if this directory is an interface or not */
			char interface[PATH_MAX];

			snprintf(interface, PATH_MAX, "%s/%s", dir, de->d_name);
			char *interfacenum = sysfs_string(interface, "bInterfaceNumber");
			if (interfacenum) {
				e = entity_create(interface);
				entity_add(&interface_list, e);
				g_free(interfacenum);
			}
		}
	}

	closedir(d);

	list_for_each_safe(&interface_list, e, temp, list) {
		interface_parse(device, e->name);
		entity_destroy(e);
	}
}

static void device_parse(struct Device *parent, const char *directory);

static void children_parse(struct Device *parent, const char *dir)
{
	int devcount = 0;
	DIR *d;
	struct dirent *de;

	d = opendir(dir);
	if (!d) {
		fprintf(stderr, "Can not open %s directory\n", dir);
		return;
	}

	while ((de = readdir(d))) {
		if (de->d_type == DT_DIR) {
			if (de->d_name[0] == '.')
				continue;

			/* See if this directory is an interface or not */
			char device[PATH_MAX];

			snprintf(device, PATH_MAX, "%s/%s", dir, de->d_name);
			char *device_class = sysfs_string(device, "bDeviceClass");
			if (device_class) {
				++devcount;
				device_parse(parent, device);
			}

			g_free(device_class);
		}
	}

	closedir(d);
}


static void device_parse(struct Device *parent, const char *dir)
{
	Device  *device;

	if (check_dir_present(dir))
		return;

	device = (Device *)(g_malloc0 (sizeof(Device)));

	if (parent == rootDevice)
		device->level = 0;
	else
		device->level = 1;

	int portnum = 0;

	if (parent != rootDevice) {
		/*
		 * The port number is the last number of the file (if present)
		 * before the '.' or '-' - 1
		 */
		const char *temp = &dir[strlen(dir)];

		while ((*temp != '.') && (*temp != '-'))
			--temp;
		temp++;
		portnum = strtol(temp, NULL, 10) - 1;
		if (portnum < 0)
			portnum = 0;
	}
	device->portNumber	= portnum;
	device->busNumber	= sysfs_int(dir, "busnum", 10);
	device->deviceNumber	= sysfs_int(dir, "devnum", 10);
	device->speed		= sysfs_int(dir, "speed", 10);
	device->maxChildren	= sysfs_int(dir, "maxchild", 10);

	device->parent = parent;

	if (parent == rootDevice) {
		++rootDevice->maxChildren;
		rootDevice->child[rootDevice->maxChildren-1] = device;
	} else {
		parent->child[portnum] = device;
	}

	device->vendorId	= sysfs_int(dir, "idVendor", 16);
	device->productId	= sysfs_int(dir, "idProduct", 16);

	device->version		= sysfs_string(dir, "version");
	device->class		= sysfs_string(dir, "bDeviceClass");
	device->subClass	= sysfs_string(dir, "bDeviceSubClass");
	device->protocol	= sysfs_string(dir, "bDeviceProtocol");
	device->maxPacketSize	= sysfs_int(dir, "bMaxPacketSize0", 10);
	device->numConfigs	= sysfs_int(dir, "bNumConfigurations", 10);

	device->manufacturer	= sysfs_string(dir, "manufacturer");
	device->product		= sysfs_string(dir, "product");
	device->serialNumber	= sysfs_string(dir, "serial");
	char *bcddevice = sysfs_string(dir, "bcdDevice");

	device->revisionNumber  = (char *)g_malloc0 ((DEVICE_REVISION_NUMBER_SIZE) * sizeof(char));
	device->revisionNumber[0] = bcddevice[0];
	device->revisionNumber[1] = bcddevice[1];
	device->revisionNumber[2] = '.';
	device->revisionNumber[3] = bcddevice[2];
	device->revisionNumber[4] = bcddevice[3];
	g_free(bcddevice);

	DeviceConfig    *config;

	config = (DeviceConfig *)g_malloc0 (sizeof(DeviceConfig));
	config->maxPower        = sysfs_string(dir, "bMaxPower");
	config->numInterfaces   = sysfs_int(dir, "bNumInterfaces", 10);
	config->configNumber	= sysfs_int(dir, "bConfigurationValue", 10);
	config->attributes	= sysfs_int(dir, "bmAttributes", 16);

	/* have the device now point to this config */
	device->config[0] = config;

#if 0
	const char *classname = class_decode(devclass);
#endif

#if 0
	char *bcddevice = sysfs_read(directory, "bcdDevice");

	device->revisionNumber  = (char *)g_malloc0 ((DEVICE_REVISION_NUMBER_SIZE) * sizeof(char));
	GetString (device->revisionNumber, data, DEVICE_REVISION_STRING, DEVICE_REVISION_NUMBER_SIZE);

#endif

	interfaces_parse(device, dir);

	children_parse(device, dir);
#if 0

	print_children(directory, devnum, level);
#endif
}


void sysfs_parse(void)
{
	char filename[PATH_MAX];
	int i;

	snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/");

	if (check_dir_present(filename)) {
		fprintf(stderr, "%s must be present, exiting...\n", filename);
		return;
	}

	for (i = 1; i < 100; ++i) {
		snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/usb%d/", i);
		device_parse(rootDevice, filename);
	}
}

void usb_parse_line (char * line)
{
	/* chop off the trailing \n */
	line[strlen(line)-1] = 0x00;

	/* look at the first character to see what kind of line this is */
	switch (line[0]) {
		case 'T': /* topology */
			//lastDevice = AddDevice (line);
			break;

		case 'B': /* bandwidth */
			//GetBandwidth (lastDevice, line);  // FIXME, not in sysfs?
			break;

		case 'D': /* device information */
			//GetDeviceInformation (lastDevice, line);
			break;

		case 'P': /* more device information */
			//GetMoreDeviceInformation (lastDevice, line);
			break;

		case 'S': /* device string information */
			//GetDeviceString (lastDevice, line);
			break;

		case 'C': /* config descriptor info */
			//AddConfig (lastDevice, line);
			break;

		case 'I': /* interface descriptor info */
			//AddInterface (lastDevice, line);
			break;

		case 'E': /* endpoint descriptor info */
			//AddEndpoint (lastDevice, line);
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
