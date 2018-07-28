/*	$NetBSD: umodeswitch.c,v 1.3.6.1 2018/07/28 04:37:58 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umodeswitch.c,v 1.3.6.1 2018/07/28 04:37:58 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>

#include "usbdevs.h"

/*
 * This device driver handles devices that have two personalities.
 * The first uses the 'usbdevif'
 * interface attribute so that a match will claim the entire USB device
 * for itself. This is used for when a device needs to be mode-switched
 * and ensures any other interfaces present cannot be claimed by other
 * drivers while the mode-switch is in progress.
 */
static int umodeswitch_match(device_t, cfdata_t, void *);
static void umodeswitch_attach(device_t, device_t, void *);
static int umodeswitch_detach(device_t, int);

CFATTACH_DECL2_NEW(umodeswitch, 0, umodeswitch_match,
    umodeswitch_attach, umodeswitch_detach, NULL, NULL, NULL);

static int
send_bulkmsg(struct usbd_device *dev, void *cmd, size_t cmdlen)
{
	struct usbd_interface *iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct usbd_pipe *pipe;
	struct usbd_xfer *xfer;
	int err, i;

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, 0, 0);
	if (err) {
		aprint_error("%s: failed to set config index\n", __func__);
		return UMATCH_NONE;
	}

	err = usbd_device2interface_handle(dev, 0, &iface);
	if (err != 0) {
		aprint_error("%s: failed to get interface\n", __func__);
		return UMATCH_NONE;
	}

	id = usbd_get_interface_descriptor(iface);
	ed = NULL;
	for (i = 0 ; i < id->bNumEndpoints ; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL)
			continue;
		if (UE_GET_DIR(ed->bEndpointAddress) != UE_DIR_OUT)
			continue;
		if ((ed->bmAttributes & UE_XFERTYPE) == UE_BULK)
			break;
	}

	if (i == id->bNumEndpoints)
		return UMATCH_NONE;

	err = usbd_open_pipe(iface, ed->bEndpointAddress,
	    USBD_EXCLUSIVE_USE, &pipe);
	if (err != 0) {
		aprint_error("%s: failed to open bulk transfer pipe %d\n",
		    __func__, ed->bEndpointAddress);
		return UMATCH_NONE;
	}

	int error = usbd_create_xfer(pipe, cmdlen, 0, 0, &xfer);
	if (!error) {

		usbd_setup_xfer(xfer, NULL, cmd, cmdlen,
		    USBD_SYNCHRONOUS, USBD_DEFAULT_TIMEOUT, NULL);

		err = usbd_transfer(xfer);

#if 0 /* XXXpooka: at least my huawei "fails" this always, but still detaches */
		if (err)
			aprint_error("%s: transfer failed\n", __func__);
#else
		err = 0;
#endif
		usbd_destroy_xfer(xfer);
	} else {
		aprint_error("%s: failed to allocate xfer\n", __func__);
		err = USBD_NOMEM;
	}

	usbd_abort_pipe(pipe);
	usbd_close_pipe(pipe);

	return err == USBD_NORMAL_COMPLETION ? UMATCH_HIGHEST : UMATCH_NONE;
}

/* Byte 0..3: Command Block Wrapper (CBW) signature */
static void
set_cbw(unsigned char *cmd)
{
	cmd[0] = 0x55;
	cmd[1] = 0x53;
	cmd[2] = 0x42;
	cmd[3] = 0x43;
}

static int
u3g_bulk_scsi_eject(struct usbd_device *dev)
{
	unsigned char cmd[31];

	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);
	/* 4..7: CBW Tag, has to unique, but only a single transfer used. */
	cmd[4] = 0x01;
	/* 8..11: CBW Transfer Length, no data here */
	/* 12: CBW Flag: output, so 0 */
	/* 13: CBW Lun: 0 */
	/* 14: CBW Length */
	cmd[14] = 0x06;

	/* Rest is the SCSI payload */

	/* 0: SCSI START/STOP opcode */
	cmd[15] = 0x1b;
	/* 1..3 unused */
	/* 4 Load/Eject command */
	cmd[19] = 0x02;
	/* 5: unused */

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}

static int
u3g_bulk_ata_eject(struct usbd_device *dev)
{
	unsigned char cmd[31];

	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);
	/* 4..7: CBW Tag, has to unique, but only a single transfer used. */
	cmd[4] = 0x01;
	/* 8..11: CBW Transfer Length, no data here */
	/* 12: CBW Flag: output, so 0 */
	/* 13: CBW Lun: 0 */
	/* 14: CBW Length */
	cmd[14] = 0x06;

	/* Rest is the SCSI payload */

	/* 0: ATA pass-through */
	cmd[15] = 0x85;
	/* 1..3 unused */
	/* 4 XXX What is this command? */
	cmd[19] = 0x24;
	/* 5: unused */

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}

static int
u3g_huawei_reinit(struct usbd_device *dev)
{
	/*
	 * The Huawei device presents itself as a umass device with Windows
	 * drivers on it. After installation of the driver, it reinits into a
	 * 3G serial device.
	 */
	usb_device_request_t req;
	usb_config_descriptor_t *cdesc;

	/* Get the config descriptor */
	cdesc = usbd_get_config_descriptor(dev);
	if (cdesc == NULL) {
		usb_device_descriptor_t dd;

		if (usbd_get_device_desc(dev, &dd) != 0)
			return UMATCH_NONE;

		if (dd.bNumConfigurations != 1)
			return UMATCH_NONE;

		if (usbd_set_config_index(dev, 0, 1) != 0)
			return UMATCH_NONE;

		cdesc = usbd_get_config_descriptor(dev);

		if (cdesc == NULL)
			return UMATCH_NONE;
	}

	/*
	 * One iface means umass mode, more than 1 (4 usually) means 3G mode.
	 *
	 * XXX: We should check the first interface's device class just to be
	 * sure. If it's a mass storage device, then we can be fairly certain
	 * it needs a mode-switch.
	 */
	if (cdesc->bNumInterface > 1)
		return UMATCH_NONE;

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, UHF_PORT_SUSPEND);
	USETW(req.wLength, 0);

	(void) usbd_do_request(dev, &req, 0);

	return UMATCH_HIGHEST; /* Prevent umass from attaching */
}

static int
u3g_huawei_k3765_reinit(struct usbd_device *dev)
{
	unsigned char cmd[31];

	/* magic string adapted from some webpage */
	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);

	cmd[15]= 0x11;
	cmd[16]= 0x06;

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}
static int
u3g_huawei_e171_reinit(struct usbd_device *dev)
{
	unsigned char cmd[31];

	/* magic string adapted from some webpage */
	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);

	cmd[15]= 0x11;
	cmd[16]= 0x06;
	cmd[17]= 0x20;
	cmd[20]= 0x01;

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}

static int
u3g_huawei_e353_reinit(struct usbd_device *dev)
{
	unsigned char cmd[31];

	/* magic string adapted from some webpage */
	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);

	cmd[4] = 0x7f;
	cmd[9] = 0x02;
	cmd[12] = 0x80;
	cmd[14] = 0x0a;
	cmd[15] = 0x11;
	cmd[16] = 0x06;
	cmd[17] = 0x20;
	cmd[23] = 0x01;

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}

static int
u3g_sierra_reinit(struct usbd_device *dev)
{
	/* Some Sierra devices presents themselves as a umass device with
	 * Windows drivers on it. After installation of the driver, it
	 * reinits into a * 3G serial device.
	 */
	usb_device_request_t req;

	req.bmRequestType = UT_VENDOR;
	req.bRequest = UR_SET_INTERFACE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, UHF_PORT_CONNECTION);
	USETW(req.wLength, 0);

	(void) usbd_do_request(dev, &req, 0);

	return UMATCH_HIGHEST; /* Match to prevent umass from attaching */
}

static int
u3g_4gsystems_reinit(struct usbd_device *dev)
{
	/* magic string adapted from usb_modeswitch database */
	unsigned char cmd[31];

	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	set_cbw(cmd);

	cmd[4] = 0x12;
	cmd[5] = 0x34;
	cmd[6] = 0x56;
	cmd[7] = 0x78;
	cmd[8] = 0x80;
	cmd[12] = 0x80;
	cmd[14] = 0x06;
	cmd[15] = 0x06;
	cmd[16] = 0xf5;
	cmd[17] = 0x04;
	cmd[18] = 0x02;
	cmd[19] = 0x52;
	cmd[20] = 0x70;

	return send_bulkmsg(dev, cmd, sizeof(cmd));
}

/*
 * First personality:
 *
 * Claim the entire device if a mode-switch is required.
 */

static int
umodeswitch_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	/*
	 * Huawei changes product when it is configured as a modem.
	 */
	switch (uaa->uaa_vendor) {
	case USB_VENDOR_HUAWEI:
		if (uaa->uaa_product == USB_PRODUCT_HUAWEI_K3765)
			return UMATCH_NONE;

		switch (uaa->uaa_product) {
		case USB_PRODUCT_HUAWEI_E1750INIT:
		case USB_PRODUCT_HUAWEI_K3765INIT:
			return u3g_huawei_k3765_reinit(uaa->uaa_device);
			break;
		case USB_PRODUCT_HUAWEI_E171INIT:
			return u3g_huawei_e171_reinit(uaa->uaa_device);
			break;
		case USB_PRODUCT_HUAWEI_E353INIT:
			return u3g_huawei_e353_reinit(uaa->uaa_device);
			break;
		default:
			return u3g_huawei_reinit(uaa->uaa_device);
			break;
		}
		break;

	case USB_VENDOR_NOVATEL2:
		switch (uaa->uaa_product){
		case USB_PRODUCT_NOVATEL2_MC950D_DRIVER:
		case USB_PRODUCT_NOVATEL2_U760_DRIVER:
			return u3g_bulk_scsi_eject(uaa->uaa_device);
			break;
		default:
			break;
		}
		break;

	case USB_VENDOR_LG:
		if (uaa->uaa_product == USB_PRODUCT_LG_NTT_DOCOMO_L02C_STORAGE)
			return u3g_bulk_scsi_eject(uaa->uaa_device);
		break;

	case USB_VENDOR_RALINK:
		switch (uaa->uaa_product){
		case USB_PRODUCT_RALINK_RT73:
			return u3g_bulk_scsi_eject(uaa->uaa_device);
			break;
		}
		break;

	case USB_VENDOR_SIERRA:
		if (uaa->uaa_product == USB_PRODUCT_SIERRA_INSTALLER)
			return u3g_sierra_reinit(uaa->uaa_device);
		break;

	case USB_VENDOR_ZTE:
		switch (uaa->uaa_product){
		case USB_PRODUCT_ZTE_INSTALLER:
		case USB_PRODUCT_ZTE_MF820D_INSTALLER:
			(void)u3g_bulk_ata_eject(uaa->uaa_device);
			(void)u3g_bulk_scsi_eject(uaa->uaa_device);
			return UMATCH_HIGHEST;
		default:
			break;
		}
		break;

	case USB_VENDOR_LONGCHEER:
		if (uaa->uaa_product == USB_PRODUCT_LONGCHEER_XSSTICK_P14_INSTALLER)
			return u3g_4gsystems_reinit(uaa->uaa_device);
		break;

	default:
		break;
	}

	return UMATCH_NONE;
}

static void
umodeswitch_attach(device_t parent, device_t self, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	aprint_naive("\n");
	aprint_normal(": Switching off umass mode\n");

	if (uaa->uaa_vendor == USB_VENDOR_NOVATEL2) {
		switch (uaa->uaa_product) {
	    	case USB_PRODUCT_NOVATEL2_MC950D_DRIVER:
	    	case USB_PRODUCT_NOVATEL2_U760_DRIVER:
			/* About to disappear... */
			return;
			break;
		default:
			break;
		}
	}

	/* Move the device into the configured state. */
	(void) usbd_set_config_index(uaa->uaa_device, 0, 1);
}

static int
umodeswitch_detach(device_t self, int flags)
{

	return 0;
}
