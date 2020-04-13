/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <sys/types.h>

#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>

#include "fido.h"

#define REPORT_LEN	65

struct dev {
	IOHIDDeviceRef	ref;
	CFStringRef	loop_id;
};

static int
get_int32(IOHIDDeviceRef dev, CFStringRef key, int32_t *v)
{
	CFTypeRef ref;

	if ((ref = IOHIDDeviceGetProperty(dev, key)) == NULL ||
	    CFGetTypeID(ref) != CFNumberGetTypeID()) {
		fido_log_debug("%s: IOHIDDeviceGetProperty", __func__);
		return (-1);
	}

	if (CFNumberGetType(ref) != kCFNumberSInt32Type &&
	    CFNumberGetType(ref) != kCFNumberSInt64Type) {
		fido_log_debug("%s: CFNumberGetType", __func__);
		return (-1);
	}

	if (CFNumberGetValue(ref, kCFNumberSInt32Type, v) == false) {
		fido_log_debug("%s: CFNumberGetValue", __func__);
		return (-1);
	}

	return (0);
}

static int
get_utf8(IOHIDDeviceRef dev, CFStringRef key, void *buf, size_t len)
{
	CFTypeRef ref;

	memset(buf, 0, len);

	if ((ref = IOHIDDeviceGetProperty(dev, key)) == NULL ||
	    CFGetTypeID(ref) != CFStringGetTypeID()) {
		fido_log_debug("%s: IOHIDDeviceGetProperty", __func__);
		return (-1);
	}

	if (CFStringGetCString(ref, buf, len, kCFStringEncodingUTF8) == false) {
		fido_log_debug("%s: CFStringGetCString", __func__);
		return (-1);
	}

	return (0);
}

static bool
is_fido(IOHIDDeviceRef dev)
{
	uint32_t	usage_page;
	int32_t		report_len;

	if (get_int32(dev, CFSTR(kIOHIDPrimaryUsagePageKey),
	    (int32_t *)&usage_page) != 0 || usage_page != 0xf1d0)
		return (false);

	if (get_int32(dev, CFSTR(kIOHIDMaxInputReportSizeKey),
	    &report_len) < 0 || report_len != REPORT_LEN - 1) {
		fido_log_debug("%s: unsupported report len", __func__);
		return (false);
	}

	return (true);
}

static int
get_id(IOHIDDeviceRef dev, int16_t *vendor_id, int16_t *product_id)
{
	int32_t	vendor;
	int32_t	product;

	if (get_int32(dev, CFSTR(kIOHIDVendorIDKey), &vendor) < 0 ||
	    vendor > UINT16_MAX) {
		fido_log_debug("%s: get_int32 vendor", __func__);
		return (-1);
	}

	if (get_int32(dev, CFSTR(kIOHIDProductIDKey), &product) < 0 ||
	    product > UINT16_MAX) {
		fido_log_debug("%s: get_int32 product", __func__);
		return (-1);
	}

	*vendor_id = (int16_t)vendor;
	*product_id = (int16_t)product;

	return (0);
}

static int
get_str(IOHIDDeviceRef dev, char **manufacturer, char **product)
{
	char	buf[512];
	int	ok = -1;

	*manufacturer = NULL;
	*product = NULL;

	if (get_utf8(dev, CFSTR(kIOHIDManufacturerKey), buf, sizeof(buf)) < 0) {
		fido_log_debug("%s: get_utf8 manufacturer", __func__);
		goto fail;
	}

	if ((*manufacturer = strdup(buf)) == NULL) {
		fido_log_debug("%s: strdup manufacturer", __func__);
		goto fail;
	}

	if (get_utf8(dev, CFSTR(kIOHIDProductKey), buf, sizeof(buf)) < 0) {
		fido_log_debug("%s: get_utf8 product", __func__);
		goto fail;
	}

	if ((*product = strdup(buf)) == NULL) {
		fido_log_debug("%s: strdup product", __func__);
		goto fail;
	}

	ok = 0;
fail:
	if (ok < 0) {
		free(*manufacturer);
		free(*product);
		*manufacturer = NULL;
		*product = NULL;
	}

	return (ok);
}

static char *
get_path(IOHIDDeviceRef dev)
{
	io_service_t	s;
	io_string_t	path;

	if ((s = IOHIDDeviceGetService(dev)) == MACH_PORT_NULL) {
		fido_log_debug("%s: IOHIDDeviceGetService", __func__);
		return (NULL);
	}

	if (IORegistryEntryGetPath(s, kIOServicePlane, path) != KERN_SUCCESS) {
		fido_log_debug("%s: IORegistryEntryGetPath", __func__);
		return (NULL);
	}

	return (strdup(path));
}

static int
copy_info(fido_dev_info_t *di, IOHIDDeviceRef dev)
{
	memset(di, 0, sizeof(*di));

	if (is_fido(dev) == false)
		return (-1);

	if (get_id(dev, &di->vendor_id, &di->product_id) < 0 ||
	    get_str(dev, &di->manufacturer, &di->product) < 0 ||
	    (di->path = get_path(dev)) == NULL) {
		free(di->path);
		free(di->manufacturer);
		free(di->product);
		explicit_bzero(di, sizeof(*di));
		return (-1);
	}

	return (0);
}

int
fido_hid_manifest(fido_dev_info_t *devlist, size_t ilen, size_t *olen)
{
	IOHIDManagerRef	manager = NULL;
	CFSetRef	devset = NULL;
	CFIndex		devcnt;
	IOHIDDeviceRef *devs = NULL;
	int		r = FIDO_ERR_INTERNAL;

	*olen = 0;

	if (ilen == 0)
		return (FIDO_OK); /* nothing to do */

	if (devlist == NULL)
		return (FIDO_ERR_INVALID_ARGUMENT);

	if ((manager = IOHIDManagerCreate(kCFAllocatorDefault,
	    kIOHIDManagerOptionNone)) == NULL) {
		fido_log_debug("%s: IOHIDManagerCreate", __func__);
		goto fail;
	}

	IOHIDManagerSetDeviceMatching(manager, NULL);

	if ((devset = IOHIDManagerCopyDevices(manager)) == NULL) {
		fido_log_debug("%s: IOHIDManagerCopyDevices", __func__);
		goto fail;
	}

	if ((devcnt = CFSetGetCount(devset)) < 0) {
		fido_log_debug("%s: CFSetGetCount", __func__);
		goto fail;
	}

	if ((devs = calloc(devcnt, sizeof(*devs))) == NULL) {
		fido_log_debug("%s: calloc", __func__);
		goto fail;
	}

	CFSetGetValues(devset, (void *)devs);

	for (CFIndex i = 0; i < devcnt; i++) {
		if (copy_info(&devlist[*olen], devs[i]) == 0) {
			devlist[*olen].io = (fido_dev_io_t) {
				fido_hid_open,
				fido_hid_close,
				fido_hid_read,
				fido_hid_write,
			};
			if (++(*olen) == ilen)
				break;
		}
	}

	r = FIDO_OK;
fail:
	if (manager != NULL)
		CFRelease(manager);
	if (devset != NULL)
		CFRelease(devset);

	free(devs);

	return (r);
}

void *
fido_hid_open(const char *path)
{
	io_registry_entry_t	 entry = MACH_PORT_NULL;
	struct dev		*dev = NULL;
	int			 ok = -1;
	int			 r;
	char			 loop_id[32];

	if ((dev = calloc(1, sizeof(*dev))) == NULL) {
		fido_log_debug("%s: calloc", __func__);
		goto fail;
	}

	if ((entry = IORegistryEntryFromPath(kIOMasterPortDefault,
	    path)) == MACH_PORT_NULL) {
		fido_log_debug("%s: IORegistryEntryFromPath", __func__);
		goto fail;
	}

	if ((dev->ref = IOHIDDeviceCreate(kCFAllocatorDefault,
	    entry)) == NULL) {
		fido_log_debug("%s: IOHIDDeviceCreate", __func__);
		goto fail;
	}

	if (IOHIDDeviceOpen(dev->ref,
	    kIOHIDOptionsTypeSeizeDevice) != kIOReturnSuccess) {
		fido_log_debug("%s: IOHIDDeviceOpen", __func__);
		goto fail;
	}

	if ((r = snprintf(loop_id, sizeof(loop_id), "fido2-%p",
	    (void *)dev->ref)) < 0 || (size_t)r >= sizeof(loop_id)) {
		fido_log_debug("%s: snprintf", __func__);
		goto fail;
	}

	if ((dev->loop_id = CFStringCreateWithCString(NULL, loop_id,
	    kCFStringEncodingASCII)) == NULL) {
		fido_log_debug("%s: CFStringCreateWithCString", __func__);
		goto fail;
	}

	ok = 0;
fail:
	if (entry != MACH_PORT_NULL)
		IOObjectRelease(entry);

	if (ok < 0 && dev != NULL) {
		if (dev->ref != NULL)
			CFRelease(dev->ref);
		if (dev->loop_id != NULL)
			CFRelease(dev->loop_id);
		free(dev);
		dev = NULL;
	}

	return (dev);
}

void
fido_hid_close(void *handle)
{
	struct dev *dev = handle;

	if (IOHIDDeviceClose(dev->ref,
	    kIOHIDOptionsTypeSeizeDevice) != kIOReturnSuccess)
		fido_log_debug("%s: IOHIDDeviceClose", __func__);

	CFRelease(dev->ref);
	CFRelease(dev->loop_id);

	free(dev);
}

static void
read_callback(void *context, IOReturn result, void *dev, IOHIDReportType type,
    uint32_t report_id, uint8_t *report, CFIndex report_len)
{
	(void)context;
	(void)dev;
	(void)report;

	if (result != kIOReturnSuccess || type != kIOHIDReportTypeInput ||
	    report_id != 0 || report_len != REPORT_LEN - 1) {
		fido_log_debug("%s: io error", __func__);
	}
}

static void
removal_callback(void *context, IOReturn result, void *sender)
{
	(void)context;
	(void)result;
	(void)sender;

	CFRunLoopStop(CFRunLoopGetCurrent());
}

int
fido_hid_read(void *handle, unsigned char *buf, size_t len, int ms)
{
	struct dev		*dev = handle;
	CFRunLoopRunResult	 r;

	(void)ms; /* XXX */

	if (len != REPORT_LEN - 1) {
		fido_log_debug("%s: invalid len", __func__);
		return (-1);
	}

	explicit_bzero(buf, len);

	IOHIDDeviceRegisterInputReportCallback(dev->ref, buf, len,
	    &read_callback, NULL);
	IOHIDDeviceRegisterRemovalCallback(dev->ref, &removal_callback, dev);
	IOHIDDeviceScheduleWithRunLoop(dev->ref, CFRunLoopGetCurrent(),
	    dev->loop_id);

	do
		r = CFRunLoopRunInMode(dev->loop_id, 0.003, true);
	while (r != kCFRunLoopRunHandledSource);

	IOHIDDeviceRegisterInputReportCallback(dev->ref, buf, len, NULL, NULL);
	IOHIDDeviceRegisterRemovalCallback(dev->ref, NULL, NULL);
	IOHIDDeviceUnscheduleFromRunLoop(dev->ref, CFRunLoopGetCurrent(),
	    dev->loop_id);

	return (REPORT_LEN - 1);
}

int
fido_hid_write(void *handle, const unsigned char *buf, size_t len)
{
	struct dev *dev = handle;

	if (len != REPORT_LEN) {
		fido_log_debug("%s: invalid len", __func__);
		return (-1);
	}

	if (IOHIDDeviceSetReport(dev->ref, kIOHIDReportTypeOutput, 0, buf + 1,
	    len - 1) != kIOReturnSuccess) {
		fido_log_debug("%s: IOHIDDeviceSetReport", __func__);
		return (-1);
	}

	return (REPORT_LEN);
}
