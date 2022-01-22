// SPDX-License-Identifier: GPL-2.0-only
/*
 * sysfs.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000, 2022 by Greg Kroah-Hartman, <greg@kroah.com>
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

#include "usbtree.h"
#include "sysfs.h"
#include "ccan/list/list.h"

struct Device *rootDevice = NULL;
static struct DeviceBandwidth *currentBandwidth = NULL;


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

static void DestroyEndpoint (struct DeviceEndpoint *endpoint)
{
	if (endpoint == NULL)
		return;

	g_free (endpoint->type);
	g_free (endpoint->interval);

	g_free (endpoint);

	return;
}


static void DestroyInterface (struct DeviceInterface *interface)
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


static void DestroyConfig (struct DeviceConfig *config)
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


static void DestroyBandwidth (struct DeviceBandwidth *bandwidth)
{
	/* nothing dynamic in the bandwidth structure yet. */
	return;
}

static void DestroyDevice(struct Device *device)
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

static struct Device *FindDeviceNode(struct Device *device,
				     int deviceNumber, int busNumber)
{
	int     i;
	struct Device *result;

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

struct Device *usb_find_device (int deviceNumber, int busNumber)
{
	int     i;
	struct Device *result;

	/* search through the tree to try to find a device */
	for (i = 0; i < MAX_CHILDREN; ++i) {
		result = FindDeviceNode (rootDevice->child[i], deviceNumber, busNumber);
		if (result != NULL)
			return(result);
	}
	return(NULL);
}

/* Build all of the names of the devices */
static void NameDevice (struct Device *device)
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
				struct DeviceConfig *config = device->config[configNum];
				for (interfaceNum = 0; interfaceNum < MAX_INTERFACES; ++interfaceNum) {
					if (config->interface[interfaceNum]) {
						struct DeviceInterface *interface = config->interface[interfaceNum];
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

	rootDevice = (struct Device *)g_malloc0 (sizeof(struct Device));

	/* clean up any bandwidth devices */
	if (currentBandwidth != NULL) {
		DestroyBandwidth (currentBandwidth);
		g_free (currentBandwidth);
	}

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
#ifdef DEBUG_LIST
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
	struct DeviceEndpoint *endpoint;
	int             i;

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

	endpoint = g_malloc0(sizeof(struct DeviceEndpoint));

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

	// FIXME check max packet size
#if 0
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
	struct DeviceConfig *config;
	struct DeviceInterface *interface;
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

	interface = g_malloc0(sizeof(struct DeviceInterface));

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
	if (strncmp(interface->name, INTERFACE_DRIVERNAME_NODRIVER_STRING,
		    INTERFACE_DRIVERNAME_STRING_MAXLENGTH) == 0) {
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
	struct Device *device;

	if (check_dir_present(dir))
		return;

	device = (struct Device *)(g_malloc0 (sizeof(struct Device)));

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

	struct DeviceConfig *config;

	config = g_malloc0(sizeof(struct DeviceConfig));
	config->maxPower        = sysfs_string(dir, "bMaxPower");
	config->numInterfaces   = sysfs_int(dir, "bNumInterfaces", 10);
	config->configNumber	= sysfs_int(dir, "bConfigurationValue", 10);
	config->attributes	= sysfs_int(dir, "bmAttributes", 16);

	/* have the device now point to this config */
	device->config[0] = config;

	interfaces_parse(device, dir);

	children_parse(device, dir);
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

void usb_name_devices (void)
{
	NameDevice (rootDevice);
}
