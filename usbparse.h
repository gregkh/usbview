// SPDX-License-Identifier: GPL-2.0-only
/*
 * usbparse.h for USBView - a USB device viewer
 * Copyright (c) 1999, 2000, 2021 by Greg Kroah-Hartman, greg@kroah.com
 */
#ifndef __USB_PARSE_H
#define __USB_PARSE_H

/* should make these dynamic someday... */
#define MAX_ENDPOINTS				32
#define MAX_INTERFACES				128
#define MAX_CONFIGS				32
#define MAX_CHILDREN				32

#define DEVICE_VERSION_SIZE			6
#define DEVICE_CLASS_SIZE			10
#define DEVICE_SUBCLASS_SIZE			3
#define DEVICE_PROTOCOL_SIZE			3
#define DEVICE_VENDOR_ID_SIZE			5
#define DEVICE_PRODUCT_ID_SIZE			5
#define DEVICE_REVISION_NUMBER_SIZE		6

#define CONFIG_ATTRIBUTES_SIZE			3
#define CONFIG_MAXPOWER_SIZE			10

#define INTERFACE_CLASS_SIZE			10

#define ENDPOINT_TYPE_SIZE			5
#define ENDPOINT_MAXPACKETSIZE_SIZE		5
#define ENDPOINT_INTERVAL_SIZE			10


#define TOPOLOGY_BUS_STRING			"Bus="
#define TOPOLOGY_LEVEL_STRING			"Lev="
#define TOPOLOGY_PARENT_STRING			"Prnt="
#define TOPOLOGY_PORT_STRING			"Port="
#define TOPOLOGY_COUNT_STRING			"Cnt="
#define TOPOLOGY_DEVICENUMBER_STRING		"Dev#="
#define TOPOLOGY_SPEED_STRING			"Spd="
#define TOPOLOGY_MAXCHILDREN_STRING		"MxCh="

#define BANDWIDTH_ALOCATED			"Alloc="
#define BANDWIDTH_TOTAL				"/"
#define BANDWIDTH_PERCENT			"us ("
#define BANDWIDTH_INTERRUPT_TOTAL		"#Int="
#define BANDWIDTH_ISOC_TOTAL			"#Iso="

#define DEVICE_VERSION_STRING			"Ver="
#define DEVICE_CLASS_STRING			"Cls="
#define DEVICE_SUBCLASS_STRING			"Sub="
#define DEVICE_PROTOCOL_STRING			"Prot="
#define DEVICE_MAXPACKETSIZE_STRING		"MxPS="
#define DEVICE_NUMCONFIGS_STRING		"#Cfgs="
#define DEVICE_VENDOR_STRING			"Vendor="
#define DEVICE_PRODUCTID_STRING			"ProdID="
#define DEVICE_REVISION_STRING			"Rev="
#define DEVICE_MANUFACTURER_STRING		"Manufacturer="
#define DEVICE_PRODUCT_STRING			"Product="
#define DEVICE_SERIALNUMBER_STRING		"SerialNumber="
#define DEVICE_STRING_MAXSIZE			255

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
#define INTERFACE_DRIVERNAME_STRING		"Driver="
#define INTERFACE_DRIVERNAME_NODRIVER_STRING	"(none)"
#define INTERFACE_DRIVERNAME_STRING_MAXLENGTH	50

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
	gchar		*name;
	gint		interfaceNumber;
	gint		alternateNumber;
	gint		numEndpoints;
	gint		subClass;
	gint		protocol;
	gchar		*class;
	DeviceEndpoint	*endpoint[MAX_ENDPOINTS];
	gboolean	driverAttached;		/* TRUE if driver is attached to this interface currently */
} DeviceInterface;
	


typedef struct DeviceConfig {
	gint		configNumber;
	gint		numInterfaces;
	gint		attributes;
	gchar		*maxPower;
	DeviceInterface	*interface[MAX_INTERFACES];
} DeviceConfig;


typedef struct DeviceBandwidth {
	gint		allocated;
	gint		total;
	gint		percent;
	gint		numInterruptRequests;
	gint		numIsocRequests;
} DeviceBandwidth;


typedef struct Device {
	gchar		*name;
	gint		busNumber;
	gint		level;
	gint		parentNumber;
	gint		portNumber;
	gint		connectorNumber;
	gint		count;
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
	DeviceConfig	*config[MAX_CONFIGS];
	struct Device	*parent;
	struct Device	*child[MAX_CHILDREN];
	DeviceBandwidth	*bandwidth;
	GtkWidget	*tree;
	GtkTreeIter	leaf;
} Device;


extern Device	*rootDevice;


extern Device *usb_find_device (int deviceNumber, int busNumber);
extern void usb_initialize_list	(void);
extern void usb_parse_line	(char *line);
extern void usb_name_devices	(void);


#endif	/* __USB_PARSE_H */

