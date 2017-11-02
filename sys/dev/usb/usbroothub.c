/* $NetBSD: usbroothub.c,v 1.2.14.1 2017/11/02 21:29:52 snj Exp $ */

/*-
 * Copyright (c) 1998, 2004, 2011, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, Jared D. McNeill (jmcneill@invisible.ca),
 * Matthew R. Green (mrg@eterna.com.au) and Nick Hudson.
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

/*
 * Copyright (c) 2008
 *	Matthias Drochner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbroothub.h>
#include <dev/usb/usbhist.h>

extern int usbdebug;

/* helper functions for USB root hub emulation */

static usbd_status	roothub_ctrl_transfer(struct usbd_xfer *);
static usbd_status	roothub_ctrl_start(struct usbd_xfer *);
static void		roothub_ctrl_abort(struct usbd_xfer *);
static void		roothub_ctrl_close(struct usbd_pipe *);
static void		roothub_ctrl_done(struct usbd_xfer *);
static void		roothub_noop(struct usbd_pipe *pipe);

const struct usbd_pipe_methods roothub_ctrl_methods = {
	.upm_transfer =	roothub_ctrl_transfer,
	.upm_start =	roothub_ctrl_start,
	.upm_abort =	roothub_ctrl_abort,
	.upm_close =	roothub_ctrl_close,
	.upm_cleartoggle =	roothub_noop,
	.upm_done =	roothub_ctrl_done,
};

int
usb_makestrdesc(usb_string_descriptor_t *p, int l, const char *s)
{
	int i;

	if (l == 0)
		return 0;
	p->bLength = 2 * strlen(s) + 2;
	if (l == 1)
		return 1;
	p->bDescriptorType = UDESC_STRING;
	l -= 2;
	/* poor man's utf-16le conversion */
	for (i = 0; s[i] && l > 1; i++, l -= 2)
		USETW2(p->bString[i], 0, s[i]);
	return 2 * i + 2;
}

int
usb_makelangtbl(usb_string_descriptor_t *p, int l)
{

	if (l == 0)
		return 0;
	p->bLength = 4;
	if (l == 1)
		return 1;
	p->bDescriptorType = UDESC_STRING;
	if (l < 4)
		return 2;
	USETW(p->bString[0], 0x0409); /* english/US */
	return 4;
}

/*
 * Data structures and routines to emulate the root hub.
 */
static const usb_device_descriptor_t usbroothub_devd1 = {
	.bLength = sizeof(usb_device_descriptor_t),
	.bDescriptorType = UDESC_DEVICE,
	.bcdUSB = {0x00, 0x01},
	.bDeviceClass = UDCLASS_HUB,
	.bDeviceSubClass = UDSUBCLASS_HUB,
	.bDeviceProtocol = UDPROTO_FSHUB,
	.bMaxPacketSize = 64,
	.idVendor = {0},
	.idProduct = {0},
	.bcdDevice = {0x00, 0x01},
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 0,
	.bNumConfigurations = 1
};

static const struct usb_roothub_descriptors usbroothub_confd1 = {
	.urh_confd = {
		.bLength = USB_CONFIG_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_CONFIG,
		.wTotalLength = USETWD(sizeof(usbroothub_confd1)),
		.bNumInterface = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = UC_ATTR_MBO | UC_SELF_POWERED,
		.bMaxPower = 0,
	},
	.urh_ifcd = {
		.bLength = USB_INTERFACE_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = UICLASS_HUB,
		.bInterfaceSubClass = UISUBCLASS_HUB,
		.bInterfaceProtocol = UIPROTO_FSHUB,
		.iInterface = 0
	},
	.urh_endpd = {
		.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_ENDPOINT,
		.bEndpointAddress = UE_DIR_IN | USBROOTHUB_INTR_ENDPT,
		.bmAttributes = UE_INTERRUPT,
		.wMaxPacketSize = USETWD(8),			/* max packet */
		.bInterval = 255,
	},
};

static const usb_device_descriptor_t usbroothub_devd2 = {
	.bLength = sizeof(usb_device_descriptor_t),
	.bDescriptorType = UDESC_DEVICE,
	.bcdUSB = {0x00, 0x02},
	.bDeviceClass = UDCLASS_HUB,
	.bDeviceSubClass = UDSUBCLASS_HUB,
	.bDeviceProtocol = UDPROTO_HSHUBSTT,
	.bMaxPacketSize = 64,
	.idVendor = {0},
	.idProduct = {0},
	.bcdDevice = {0x00, 0x01},
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 0,
	.bNumConfigurations = 1
};

static const usb_device_qualifier_t usbroothub_odevd2 = {
	.bLength = USB_DEVICE_QUALIFIER_SIZE,
	.bDescriptorType = UDESC_DEVICE_QUALIFIER,
	.bcdUSB = {0x00, 0x02},
	.bDeviceClass = UDCLASS_HUB,
	.bDeviceSubClass = UDSUBCLASS_HUB,
	.bDeviceProtocol = UDPROTO_FSHUB,
	.bMaxPacketSize0 = 64,
	.bNumConfigurations = 1,
};

static const struct usb_roothub_descriptors usbroothub_confd2 = {
	.urh_confd = {
		.bLength = USB_CONFIG_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_CONFIG,
		.wTotalLength = USETWD(sizeof(usbroothub_confd2)),
		.bNumInterface = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = UC_ATTR_MBO | UC_SELF_POWERED,
		.bMaxPower = 0,
	},
	.urh_ifcd = {
		.bLength = USB_INTERFACE_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = UICLASS_HUB,
		.bInterfaceSubClass = UISUBCLASS_HUB,
		.bInterfaceProtocol = UIPROTO_HSHUBSTT,
		.iInterface = 0
	},
	.urh_endpd = {
		.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_ENDPOINT,
		.bEndpointAddress = UE_DIR_IN | USBROOTHUB_INTR_ENDPT,
		.bmAttributes = UE_INTERRUPT,
		.wMaxPacketSize = USETWD(8),			/* max packet */
		.bInterval = 12,
	},
};

static const usb_hub_descriptor_t usbroothub_hubd = {
	.bDescLength = USB_HUB_DESCRIPTOR_SIZE,
	.bDescriptorType = UDESC_HUB,
	.bNbrPorts = 1,
	.wHubCharacteristics = USETWD(UHD_PWR_NO_SWITCH | UHD_OC_INDIVIDUAL),
	.bPwrOn2PwrGood = 50,
	.bHubContrCurrent = 0,
	.DeviceRemovable = {0},		/* port is removable */
};

/*
 * Simulate a hardware hub by handling all the necessary requests.
 */
usbd_status
roothub_ctrl_transfer(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct usbd_bus *bus = pipe->up_dev->ud_bus;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(bus->ub_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(bus->ub_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return roothub_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
roothub_ctrl_start(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct usbd_bus *bus = pipe->up_dev->ud_bus;
	usb_device_request_t *req;
	usbd_status err = USBD_IOERROR;		/* XXX STALL? */
	uint16_t len, value;
	int buflen, actlen;
	void *buf;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(xfer->ux_rqflags & URQ_REQUEST);
	req = &xfer->ux_request;

	USBHIST_LOG(usbdebug, "type=%#2jx request=%#2jx", req->bmRequestType,
	    req->bRequest, 0, 0);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);

	buf = len ? usbd_get_buffer(xfer) : NULL;
	buflen = 0;

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_CLEAR_FEATURE, UT_WRITE_DEVICE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_INTERFACE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_ENDPOINT):
		/*
		 * DEVICE_REMOTE_WAKEUP and ENDPOINT_HALT are no-ops
		 * for the integrated root hub.
		 */
		break;
	case C(UR_GET_CONFIG, UT_READ_DEVICE):
		if (len > 0) {
			uint8_t *out = buf;

			*out = bus->ub_rhconf;
			buflen = sizeof(*out);
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		USBHIST_LOG(usbdebug, "wValue=%#4jx", value, 0, 0, 0);

		if (len == 0)
			break;
		switch (value) {
		case C(0, UDESC_DEVICE):
			if (bus->ub_revision == USBREV_2_0) {
				buflen = min(len, sizeof(usbroothub_devd2));
				memcpy(buf, &usbroothub_devd2, buflen);
			} else {
				buflen = min(len, sizeof(usbroothub_devd1));
				memcpy(buf, &usbroothub_devd1, buflen);
			}
			break;
		case C(0, UDESC_CONFIG):
			if (bus->ub_revision == USBREV_2_0) {
				buflen = min(len, sizeof(usbroothub_confd2));
				memcpy(buf, &usbroothub_confd2, buflen);
			} else {
				buflen = min(len, sizeof(usbroothub_confd1));
				memcpy(buf, &usbroothub_confd1, buflen);
			}
			break;
		case C(0, UDESC_DEVICE_QUALIFIER):
			if (bus->ub_revision == USBREV_2_0) {
				/*
				 * We can't really operate at another speed,
				 * but the spec says we need this descriptor.
				 */
				buflen = min(len, sizeof(usbroothub_odevd2));
				memcpy(buf, &usbroothub_odevd2, buflen);
			} else
				goto fail;
			break;
		case C(0, UDESC_OTHER_SPEED_CONFIGURATION):
			if (bus->ub_revision == USBREV_2_0) {
				struct usb_roothub_descriptors confd;

				/*
				 * We can't really operate at another speed,
				 * but the spec says we need this descriptor.
				 */
				buflen = min(len, sizeof(usbroothub_confd2));
				memcpy(&confd, &usbroothub_confd2, buflen);
				confd.urh_confd.bDescriptorType =
				    UDESC_OTHER_SPEED_CONFIGURATION;
				memcpy(buf, &confd, buflen);
			} else
				goto fail;
			break;
#define sd ((usb_string_descriptor_t *)buf)
		case C(0, UDESC_STRING):
			/* Language table */
			buflen = usb_makelangtbl(sd, len);
			break;
		case C(1, UDESC_STRING):
			/* Vendor */
			buflen = usb_makestrdesc(sd, len, "NetBSD");
			break;
		case C(2, UDESC_STRING):
			/* Product */
			buflen = usb_makestrdesc(sd, len, "Root hub");
			break;
#undef sd
		default:
			/* Default to error */
			buflen = -1;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		buflen = min(len, sizeof(usbroothub_hubd));
		memcpy(buf, &usbroothub_hubd, buflen);
		break;
	case C(UR_GET_INTERFACE, UT_READ_INTERFACE):
		/* Get Interface, 9.4.4 */
		if (len > 0) {
			uint8_t *out = buf;

			*out = 0;
			buflen = sizeof(*out);
		}
		break;
	case C(UR_GET_STATUS, UT_READ_DEVICE):
		/* Get Status from device, 9.4.5 */
		if (len > 1) {
			usb_status_t *out = buf;

			USETW(out->wStatus, UDS_SELF_POWERED);
			buflen = sizeof(*out);
		}
		break;
	case C(UR_GET_STATUS, UT_READ_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_ENDPOINT):
		/* Get Status from interface, endpoint, 9.4.5 */
		if (len > 1) {
			usb_status_t *out = buf;

			USETW(out->wStatus, 0);
			buflen = sizeof(*out);
		}
		break;
	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
		/* Set Address, 9.4.6 */
		USBHIST_LOG(usbdebug, "UR_SET_ADDRESS, UT_WRITE_DEVICE: "
		    "addr %jd", value, 0, 0, 0);
		if (value >= USB_MAX_DEVICES) {
			goto fail;
		}
		bus->ub_rhaddr = value;
		break;
	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		/* Set Configuration, 9.4.7 */
		if (value != 0 && value != 1) {
			goto fail;
		}
		bus->ub_rhconf = value;
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_DEVICE):
		/* Set Descriptor, 9.4.8, not supported */
		break;
	case C(UR_SET_FEATURE, UT_WRITE_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_INTERFACE):
	case C(UR_SET_FEATURE, UT_WRITE_ENDPOINT):
		/* Set Feature, 9.4.9, not supported */
		goto fail;
	case C(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
		/* Set Interface, 9.4.10, not supported */
		break;
	case C(UR_SYNCH_FRAME, UT_WRITE_ENDPOINT):
		/* Synch Frame, 9.4.11, not supported */
		break;
	default:
		/* Default to error */
		buflen = -1;
		break;
	}

	actlen = bus->ub_methods->ubm_rhctrl(bus, req, buf, buflen);
	USBHIST_LOG(usbdebug, "xfer %#jx buflen %jd actlen %jd",
	    (uintptr_t)xfer, buflen, actlen, 0);
	if (actlen < 0)
		goto fail;

	xfer->ux_actlen = actlen;
	err = USBD_NORMAL_COMPLETION;

 fail:
	USBHIST_LOG(usbdebug, "xfer %#jx err %jd", (uintptr_t)xfer, err, 0, 0);

	xfer->ux_status = err;
	mutex_enter(bus->ub_lock);
	usb_transfer_complete(xfer);
	mutex_exit(bus->ub_lock);

	return USBD_NORMAL_COMPLETION;
}

/* Abort a root control request. */
Static void
roothub_ctrl_abort(struct usbd_xfer *xfer)
{

	/* Nothing to do, all transfers are synchronous. */
}

/* Close the root pipe. */
Static void
roothub_ctrl_close(struct usbd_pipe *pipe)
{

	/* Nothing to do. */
}

Static void
roothub_ctrl_done(struct usbd_xfer *xfer)
{

	/* Nothing to do. */
}

static void
roothub_noop(struct usbd_pipe *pipe)
{

}
