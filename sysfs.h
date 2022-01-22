// SPDX-License-Identifier: GPL-2.0-only
/*
 * sysfs.h for USBView - a USB device viewer
 * Copyright (c) 1999, 2000, 2021-2022 by Greg Kroah-Hartman, greg@kroah.com
 */
#ifndef __SYSFS_H
#define __SYSFS_H

/* should make these dynamic someday... */
#define MAX_ENDPOINTS				32
#define MAX_INTERFACES				128
#define MAX_CONFIGS				32
#define MAX_CHILDREN				32

#define DEVICE_REVISION_NUMBER_SIZE		6
#define DEVICE_STRING_MAXSIZE			255

#define INTERFACE_DRIVERNAME_NODRIVER_STRING	"(none)"
#define INTERFACE_DRIVERNAME_STRING_MAXLENGTH	50

struct DeviceEndpoint {
	gint		address;
	gboolean	in;		/* TRUE if in, FALSE if out */
	gint		attribute;
	gchar		*type;
	gint		maxPacketSize;
	gchar		*interval;
};

struct DeviceInterface {
	gchar		*name;
	gint		interfaceNumber;
	gint		alternateNumber;
	gint		numEndpoints;
	gint		subClass;
	gint		protocol;
	gchar		*class;
	struct DeviceEndpoint *endpoint[MAX_ENDPOINTS];
	gboolean	driverAttached;		/* TRUE if driver is attached to this interface currently */
};

struct DeviceConfig {
	gint		configNumber;
	gint		numInterfaces;
	gint		attributes;
	gchar		*maxPower;
	struct DeviceInterface *interface[MAX_INTERFACES];
};

struct DeviceBandwidth {
	gint		allocated;
	gint		total;
	gint		percent;
	gint		numInterruptRequests;
	gint		numIsocRequests;
};

struct Device {
	gchar		*name;
	gint		busNumber;
	gint		level;
	gint		portNumber;
	gint		connectorNumber;
	gint		deviceNumber;
	gint		speed;
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
	gchar		*manufacturer;
	gchar		*product;
	gchar		*serialNumber;
	struct DeviceConfig *config[MAX_CONFIGS];
	struct Device	*parent;
	struct Device	*child[MAX_CHILDREN];
	struct DeviceBandwidth	*bandwidth;
	GtkWidget	*tree;
	GtkTreeIter	leaf;
};

extern struct Device *rootDevice;

struct Device *usb_find_device(int deviceNumber, int busNumber);
void usb_initialize_list(void);
void sysfs_parse(void);
void usb_name_devices(void);

#endif	/* __USB_PARSE_H */

