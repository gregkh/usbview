%define 	name	usbview
%define 	version	0.8.0
%define 	release	1
%define 	serial	1
%define 	prefix	/usr

Summary: 	USB topology and device viewer
Name:		%{name}
Version:	%{version}
Release:	%{release}
Serial:		%{serial}
Copyright:	GPL
Group:		Applications/System
Url:		http://www.kroah.com/linux-usb/
Vendor:		Greg Kroah-Hartman <greg@kroah.com>
Source:		http://www.kroah.com/linux-usb/%{name}-%{version}.tar.gz
BuildRoot:	/var/tmp/%{name}-%{version}
Requires:	gtk+ >= 1.2.3

%description
USBView is a GTK program that displays the topography of the devices 
that are plugged into the USB bus on a Linux machine. It also displays 
information on each of the devices. This can be useful to determine if 
a device is working properly or not.

%prep
%setup -q

%build
%configure --prefix=%{prefix}
make

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi

%files
%defattr(-,root,root)
%doc ChangeLog COPYING* README TODO
%{prefix}/bin/usbview

%changelog
* Thu Jun 14 2000 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.8.0]
- Added ability to select where the devices file is located at.
  This allows you to view a usbdevfs devices file that comes
  from another user, for instance. It also accomidates those who
  do not mount usbdevfs at /proc/bus/usb.
- Fixed bug with devices that have a lot of interfaces.
- Changed the tree widget to a different style.
- Restructured the internal code a bit nicer (a lot of work to
  go with this...)
- Added a TODO file to the archive listing some potential changes
  that could be done.

* Tue Jan 4 2000 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.7.0]
- The logic for determining the name of the device changed to
  properly display the name of a keyboard or mouse when the
  HID driver is used. This is needed for kernel versions 2.3.36
  and up.

* Tue Dec 21 1999 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.6.0]
- now can handle multiple root hubs.
- added display of bus bandwidth information for root hubs.
- added support for device strings to describe the device.
- made logic for device name to be smarter due to the availability
  of the string descriptors.

* Tue Dec 7 1999 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.5.0]
- updated the parser to handle the fact that the interface now
  dictates what driver is loaded.
- Tested on kernel version 2.3.29

* Tue Oct 26 1999 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.4.0]
- redid the user interface slightly, making the tree always expanded,
  showing the top of the text field, and better balancing the splitter
  bar.
- changed the parsing to make it easier to handle any future changes
  in the way the format of /proc/bus/usb/devices

* Sun Oct 3 1999 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.2.0]
- Configuration, interface, and endpoint data is now displayed
  for each device.
- Fixed problem with processing the last line in the /proc/... file
  twice.

* Sat Oct 2 1999 Greg Kroah-Hartman <greg@kroah.com>
[usbview-0.1.0]
- devices are read from /proc/bus/usb/devices and put into a tree view
- very basic information is displayed about each device when it is
  selected

