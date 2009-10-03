/*	$NetBSD: rumpusbhc.c,v 1.2 2009/10/03 19:07:33 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 * This rump driver attaches ugen as a kernel usb host controller.
 * It's still somewhat under the hammer ....
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rumpusbhc.c,v 1.2 2009/10/03 19:07:33 pooka Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbroothub_subr.h>

#include <rump/rumpuser.h>

#include "rump_dev_private.h"

#define UGEN_NEPTS 16
#define UGEN_EPT_CTRL 0 /* ugenx.00 is the control endpoint */

struct rumpusbhc_softc {
	struct usbd_bus sc_bus;
	int sc_devnum;

	int sc_ugenfd[UGEN_NEPTS];
	int sc_fdmodes[UGEN_NEPTS];

	int sc_port_status;
	int sc_port_change;
	int sc_addr;
	int sc_conf;
};

static const struct cfiattrdata usb_iattrdata = {
        "usbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata *const rumpusbhc_attrs[] = {
	&usb_iattrdata,
	NULL,
};

static int	rumpusbhc_probe(struct device *, struct cfdata *, void *);
static void	rumpusbhc_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(rumpusbhc, sizeof(struct rumpusbhc_softc),
	rumpusbhc_probe, rumpusbhc_attach, NULL, NULL);
CFDRIVER_DECL(rumpusbhc, DV_DULL, rumpusbhc_attrs);

struct cfparent rumpusbhcpar = {
	"mainbus",
	"mainbus",
	DVUNIT_ANY
};

/* probe ugen0 through ugen3 */
struct cfdata rumpusbhc_cfdata[] = {
	{ "rumpusbhc", "rumpusbhc", 0, FSTATE_NOTFOUND, NULL, 0, &rumpusbhcpar},
	{ "rumpusbhc", "rumpusbhc", 1, FSTATE_NOTFOUND, NULL, 0, &rumpusbhcpar},
	{ "rumpusbhc", "rumpusbhc", 2, FSTATE_NOTFOUND, NULL, 0, &rumpusbhcpar},
	{ "rumpusbhc", "rumpusbhc", 3, FSTATE_NOTFOUND, NULL, 0, &rumpusbhcpar},
};

#define UGENDEV_BASESTR "/dev/ugen"
#define UGENDEV_BUFSIZE 32
static void
makeugendevstr(int devnum, int endpoint, char *buf)
{

	CTASSERT(UGENDEV_BUFSIZE > sizeof(UGENDEV_BASESTR)+sizeof("0.00")+1);
	sprintf(buf, "%s%d.%02d", UGENDEV_BASESTR, devnum, endpoint);
}

/*
 * Our fictional hubbie.
 */

static const usb_device_descriptor_t rumphub_udd = {
	.bLength		= USB_DEVICE_DESCRIPTOR_SIZE,
	.bDescriptorType	= UDESC_DEVICE,
	.bDeviceClass		= UDCLASS_HUB,
	.bDeviceSubClass	= UDSUBCLASS_HUB,
	.bDeviceProtocol	= UDPROTO_FSHUB,
	.bMaxPacketSize		= 64,
	.bNumConfigurations	= 1,
};

static const usb_config_descriptor_t rumphub_ucd = {
	.bLength		= USB_CONFIG_DESCRIPTOR_SIZE,
	.bDescriptorType	= UDESC_CONFIG,
	.wTotalLength		= { USB_CONFIG_DESCRIPTOR_SIZE
				  + USB_INTERFACE_DESCRIPTOR_SIZE
				  + USB_ENDPOINT_DESCRIPTOR_SIZE },
	.bNumInterface		= 1,
};

static const usb_interface_descriptor_t rumphub_uid = {
	.bLength		= USB_INTERFACE_DESCRIPTOR_SIZE,
	.bDescriptorType	= UDESC_INTERFACE,
	.bInterfaceNumber	= 0,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= UICLASS_HUB,
	.bInterfaceSubClass	= UISUBCLASS_HUB,
	.bInterfaceProtocol	= UIPROTO_FSHUB,
};

static const usb_endpoint_descriptor_t rumphub_epd = {
	.bLength		= USB_ENDPOINT_DESCRIPTOR_SIZE,
	.bDescriptorType	= UDESC_ENDPOINT,
	.bmAttributes		= UE_INTERRUPT,
	.wMaxPacketSize		= {64, 0},
};

static const usb_hub_descriptor_t rumphub_hdd = {
	.bDescLength		= USB_HUB_DESCRIPTOR_SIZE,
	.bDescriptorType	= UDESC_HUB,
	.bNbrPorts		= 1,
};

static usbd_status
rumpusb_root_ctrl_start(usbd_xfer_handle xfer)
{
	usb_device_request_t *req = &xfer->request;
	struct rumpusbhc_softc *sc = xfer->pipe->device->bus->hci_private;
	int len, totlen, value, curlen, err;
	uint8_t *buf;

	len = totlen = UGETW(req->wLength);
	if (len)
		buf = KERNADDR(&xfer->dmabuf, 0);
	value = UGETW(req->wValue);

#define C(x,y) ((x) | ((y) << 8))
	switch(C(req->bRequest, req->bmRequestType)) {

	case C(UR_GET_CONFIG, UT_READ_DEVICE):
		if (len > 0) {
			*buf = sc->sc_conf;
			totlen = 1;
		}
		break;

	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		switch (value >> 8) {
		case UDESC_DEVICE:
			totlen = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			memcpy(buf, &rumphub_udd, totlen);
			break;

		case UDESC_CONFIG:
			totlen = 0;
			curlen = min(len, USB_CONFIG_DESCRIPTOR_SIZE);
			memcpy(buf, &rumphub_ucd, curlen);
			len -= curlen;
			buf += curlen;
			totlen += curlen;

			curlen = min(len, USB_INTERFACE_DESCRIPTOR_SIZE);
			memcpy(buf, &rumphub_uid, curlen);
			len -= curlen;
			buf += curlen;
			totlen += curlen;

			curlen = min(len, USB_ENDPOINT_DESCRIPTOR_SIZE);
			memcpy(buf, &rumphub_epd, curlen);
			len -= curlen;
			buf += curlen;
			totlen += curlen;
			break;

		case UDESC_STRING:
#define sd ((usb_string_descriptor_t *)buf)
			switch (value & 0xff) {
			case 0: /* Language table */
				totlen = usb_makelangtbl(sd, len);
				break;
			case 1: /* Vendor */
				totlen = usb_makestrdesc(sd, len, "rod nevada");
				break;
			case 2: /* Product */
				totlen = usb_makestrdesc(sd, len,
				    "RUMPUSBHC root hub");
				break;
			}
#undef sd
			break;

		default:
			panic("unhandled read device request");
			break;
		}
		break;

	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
		if (value >= USB_MAX_DEVICES) {
			err = USBD_IOERROR;
			goto ret;
		}
		sc->sc_addr = value;
		break;

	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		if (value != 0 && value != 1) {
			err = USBD_IOERROR;
			goto ret;
		}
		sc->sc_conf = value;
		break;

	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		switch (value) {
		case UHF_PORT_RESET:
			sc->sc_port_change |= UPS_C_PORT_RESET;
			break;
		case UHF_PORT_POWER:
			break;
		default:
			panic("unhandled");
		}
		break;

	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		sc->sc_port_change &= ~value;
		break;

	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		totlen = min(len, USB_HUB_DESCRIPTOR_SIZE);
		memcpy(buf, &rumphub_hdd, totlen);
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		/* huh?  other hc's do this */
		memset(buf, 0, len);
		totlen = len;
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		{
		usb_port_status_t ps;

		USETW(ps.wPortStatus, sc->sc_port_status);
		USETW(ps.wPortChange, sc->sc_port_change);
		totlen = min(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
		}

	default:
		panic("unhandled request");
		break;
	}
	err = USBD_NORMAL_COMPLETION;
	xfer->actlen = totlen;

ret:
	xfer->status = err;
	usb_transfer_complete(xfer);
	return (USBD_IN_PROGRESS);
}

static usbd_status
rumpusb_root_ctrl_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	return (rumpusb_root_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static void
rumpusb_root_ctrl_abort(usbd_xfer_handle xfer)
{

}

static void
rumpusb_root_ctrl_close(usbd_pipe_handle pipe)
{

}

static void
rumpusb_root_ctrl_cleartoggle(usbd_pipe_handle pipe)
{

}

static void
rumpusb_root_ctrl_done(usbd_xfer_handle xfer)
{

}

static const struct usbd_pipe_methods rumpusb_root_ctrl_methods = {
	.transfer =	rumpusb_root_ctrl_transfer,
	.start =	rumpusb_root_ctrl_start,
	.abort =	rumpusb_root_ctrl_abort,
	.close =	rumpusb_root_ctrl_close,
	.cleartoggle =	rumpusb_root_ctrl_cleartoggle,
	.done =		rumpusb_root_ctrl_done,
};

static usbd_status
rumpusb_device_ctrl_start(usbd_xfer_handle xfer)
{
	usb_device_request_t *req = &xfer->request;
	struct rumpusbhc_softc *sc = xfer->pipe->device->bus->hci_private;
	uint8_t *buf;
	int len, totlen;
	int value;
	int err = 0;
	int ru_error;

	len = totlen = UGETW(req->wLength);
	if (len)
		buf = KERNADDR(&xfer->dmabuf, 0);
	value = UGETW(req->wValue);

#define C(x,y) ((x) | ((y) << 8))
	switch(C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		switch (value>>8) {
		case UDESC_DEVICE:
			{
			usb_device_descriptor_t uddesc;
			totlen = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			memset(buf, 0, totlen);
			if (rumpuser_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_DEVICE_DESC, &uddesc, &ru_error) == -1)
				panic("%d", ru_error);
			memcpy(buf, &uddesc, totlen);
			}

			break;
		case UDESC_CONFIG:
			{
			struct usb_full_desc ufdesc;
			ufdesc.ufd_config_index = 0;
			ufdesc.ufd_size = len;
			ufdesc.ufd_data = buf;
			memset(buf, 0, totlen);
			if (rumpuser_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_FULL_DESC, &ufdesc, &ru_error) == -1)
				panic("%d", ru_error);
			totlen = len;
			}
			break;

		case UDESC_STRING:
			{
			struct usb_device_info udi;

			if (rumpuser_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_DEVICEINFO, &udi, &ru_error) == -1) {
				printf("rumpusbhc: get dev info failed: %d\n",
				    ru_error);
				err = USBD_IOERROR;
				goto ret;
			}

			switch (value & 0xff) {
#define sd ((usb_string_descriptor_t *)buf)
			case 0: /* language table */
				break;
			case 1: /* vendor */
				totlen = usb_makestrdesc(sd, len,
				    udi.udi_vendor);
				break;
			case 2: /* product */
				totlen = usb_makestrdesc(sd, len,
				    udi.udi_product);
				break;
			}
#undef sd
			}
			break;

		default:
			panic("not handled");
		}
		break;

	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
		/* ignored, ugen won't let us */
		break;

	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		/* ignored, ugen won't let us */
		break;

	case C(UR_SET_INTERFACE, UT_WRITE_DEVICE):
		/* ignored, ugen won't let us */
		break;

	case C(UR_CLEAR_FEATURE, UT_WRITE_ENDPOINT):
		printf("clear feature UNIMPL\n");
		break;

	case C(0xfe, UT_READ_CLASS_INTERFACE):
		/* XXX: what is this? */
		totlen = 0;
		break;

	default:
		panic("unhandled request");
		break;
	}
	xfer->actlen = totlen;
	err = USBD_NORMAL_COMPLETION;

 ret:
	xfer->status = err;
	usb_transfer_complete(xfer);
	return (USBD_IN_PROGRESS);
}

static usbd_status
rumpusb_device_ctrl_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	return (rumpusb_device_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static void
rumpusb_device_ctrl_abort(usbd_xfer_handle xfer)
{

}

static void
rumpusb_device_ctrl_close(usbd_pipe_handle pipe)
{

}

static void
rumpusb_device_ctrl_cleartoggle(usbd_pipe_handle pipe)
{

}

static void
rumpusb_device_ctrl_done(usbd_xfer_handle xfer)
{

}

static const struct usbd_pipe_methods rumpusb_device_ctrl_methods = {
	.transfer =	rumpusb_device_ctrl_transfer,
	.start =	rumpusb_device_ctrl_start,
	.abort =	rumpusb_device_ctrl_abort,
	.close =	rumpusb_device_ctrl_close,
	.cleartoggle =	rumpusb_device_ctrl_cleartoggle,
	.done =		rumpusb_device_ctrl_done,
};

static usbd_xfer_handle daintr;
static void
rhscintr(void)
{

	daintr->actlen = daintr->length;
	daintr->status = USBD_NORMAL_COMPLETION;
	usb_transfer_complete(daintr);
}

static usbd_status
rumpusb_root_intr_start(usbd_xfer_handle xfer)
{

	daintr = xfer;
	return (USBD_IN_PROGRESS);
}

static usbd_status
rumpusb_root_intr_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	return (rumpusb_root_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static void
rumpusb_root_intr_abort(usbd_xfer_handle xfer)
{

}

static void
rumpusb_root_intr_close(usbd_pipe_handle pipe)
{

}

static void
rumpusb_root_intr_cleartoggle(usbd_pipe_handle pipe)
{

}

static void
rumpusb_root_intr_done(usbd_xfer_handle xfer)
{

}

static const struct usbd_pipe_methods rumpusb_root_intr_methods = {
	.transfer =	rumpusb_root_intr_transfer,
	.start =	rumpusb_root_intr_start,
	.abort =	rumpusb_root_intr_abort,
	.close =	rumpusb_root_intr_close,
	.cleartoggle =	rumpusb_root_intr_cleartoggle,
	.done =		rumpusb_root_intr_done,
};

static usbd_status
rumpusb_device_bulk_start(usbd_xfer_handle xfer)
{
	struct rumpusbhc_softc *sc = xfer->pipe->device->bus->hci_private;
	ssize_t n;
	bool isread;
	int len, error, endpt;
	uint8_t *buf;

	endpt = xfer->pipe->endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	endpt = UE_GET_ADDR(endpt);
	KASSERT(endpt < UGEN_NEPTS);

	KASSERT(xfer->length);
	len = xfer->length;
	buf = KERNADDR(&xfer->dmabuf, 0);

	if (isread) {
		n = rumpuser_read(sc->sc_ugenfd[endpt], buf, len, &error);
		if (n != len)
			n = 0;
	} else {
		n = rumpuser_write(sc->sc_ugenfd[endpt], buf, len, &error);
		if (n != len)
			panic("short write");
	}

	xfer->actlen = n;
	xfer->status = USBD_NORMAL_COMPLETION;
	usb_transfer_complete(xfer);
	return (USBD_IN_PROGRESS);
}

static usbd_status
rumpusb_device_bulk_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	return (rumpusb_device_bulk_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static void
rumpusb_device_bulk_abort(usbd_xfer_handle xfer)
{

}

static void
rumpusb_device_bulk_close(usbd_pipe_handle pipe)
{

}

static void
rumpusb_device_bulk_cleartoggle(usbd_pipe_handle pipe)
{

}

static void
rumpusb_device_bulk_done(usbd_xfer_handle xfer)
{

}

static const struct usbd_pipe_methods rumpusb_device_bulk_methods = {
	.transfer =	rumpusb_device_bulk_transfer,
	.start =	rumpusb_device_bulk_start,
	.abort =	rumpusb_device_bulk_abort,
	.close =	rumpusb_device_bulk_close,
	.cleartoggle =	rumpusb_device_bulk_cleartoggle,
	.done =		rumpusb_device_bulk_done,
};

static usbd_status
rumpusbhc_open(struct usbd_pipe *pipe)
{
	usbd_device_handle dev = pipe->device;
	struct rumpusbhc_softc *sc = dev->bus->hci_private;
	usb_endpoint_descriptor_t *ed = pipe->endpoint->edesc;
	u_int8_t addr = dev->address;
	u_int8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	char buf[UGENDEV_BUFSIZE];
	int endpt, oflags, error;
	int fd;

	sc->sc_port_status = UPS_CURRENT_CONNECT_STATUS
	    | UPS_PORT_ENABLED | UPS_PORT_POWER | UPS_HIGH_SPEED;
	sc->sc_port_change = UPS_C_CONNECT_STATUS;

	if (addr == sc->sc_addr) {
		switch (xfertype) {
		case UE_CONTROL:
			pipe->methods = &rumpusb_root_ctrl_methods;
			break;
		case UE_INTERRUPT:
			pipe->methods = &rumpusb_root_intr_methods;
			break;
		default:
			panic("%d not supported", xfertype);
			break;
		}
	} else {
		switch (xfertype) {
		case UE_CONTROL:
			pipe->methods = &rumpusb_device_ctrl_methods;
			break;
		case UE_BULK:
			endpt = pipe->endpoint->edesc->bEndpointAddress;
			if (UE_GET_DIR(endpt) == UE_DIR_IN) {
				oflags = O_RDONLY;
			} else {
				oflags = O_WRONLY;
			}
			endpt = UE_GET_ADDR(endpt);
			pipe->methods = &rumpusb_device_bulk_methods;

			if (sc->sc_fdmodes[endpt] == oflags
			    || sc->sc_fdmodes[endpt] == O_RDWR)
				break;

			if (sc->sc_fdmodes[endpt] != -1) {
				/* XXX: closing from under someone? */
				rumpuser_close(sc->sc_ugenfd[endpt], &error);
				oflags = O_RDWR;
			}

			makeugendevstr(sc->sc_devnum, endpt, buf);
			fd = rumpuser_open(buf, oflags, &error);
			if (fd == -1)
				return USBD_INVAL; /* XXX: no mapping */
			sc->sc_ugenfd[endpt] = fd;
			sc->sc_fdmodes[endpt] = oflags;
			break;
		default:
			panic("%d not supported", xfertype);
			break;

		}
	}
	return 0;
}

static void
rumpusbhc_softint(void *arg)
{

}

static void
rumpusbhc_poll(struct usbd_bus *ubus)
{

}

static usbd_status
rumpusbhc_allocm(struct usbd_bus *bus, usb_dma_t *dma, uint32_t size)
{
	struct rumpusbhc_softc *sc = bus->hci_private;

	return usb_allocmem(&sc->sc_bus, size, 0, dma);
}

static void
rumpusbhc_freem(struct usbd_bus *ubus, usb_dma_t *udma)
{

}

static struct usbd_xfer *
rumpusbhc_allocx(struct usbd_bus *bus)
{
	usbd_xfer_handle xfer;

	xfer = kmem_zalloc(sizeof(struct usbd_xfer), KM_SLEEP);
	xfer->busy_free = XFER_BUSY;

	return xfer;
}

static void
rumpusbhc_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{

	kmem_free(xfer, sizeof(struct usbd_xfer));
}

struct rumpusbhc_pipe {
	struct usbd_pipe pipe;
};

static const struct usbd_bus_methods rumpusbhc_bus_methods = {
	.open_pipe =	rumpusbhc_open,
	.soft_intr =	rumpusbhc_softint,
	.do_poll =	rumpusbhc_poll,
	.allocm = 	rumpusbhc_allocm,
	.freem = 	rumpusbhc_freem,
	.allocx = 	rumpusbhc_allocx,
	.freex =	rumpusbhc_freex,
};

void
rump_dev_rumpusbhc_init()
{
	int error;

	error = config_cfdriver_attach(&rumpusbhc_cd);
	if (error)
		panic("rumpusbhc cfdriver attach failed: %d", error);
	error = config_cfattach_attach("rumpusbhc", &rumpusbhc_ca);
	if (error)
		panic("rumpusbhc cfattach attach failed: %d", error);
	error = config_cfdata_attach(rumpusbhc_cfdata, 0);
	if (error)
		panic("rumpusbhc cfdata attach failed: %d", error);
}

static int
rumpusbhc_probe(struct device *parent, struct cfdata *match, void *aux)
{
	char buf[UGENDEV_BUFSIZE];
	int fd, error;

	makeugendevstr(match->cf_unit, 0, buf);
	fd = rumpuser_open(buf, O_RDWR, &error);
	if (fd == -1)
		return 0;

	rumpuser_close(fd, &error);
	return 1;
}

static void
rumpusbhc_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	struct rumpusbhc_softc *sc = device_private(self);
	char buf[UGENDEV_BUFSIZE];
	int error;

	aprint_normal("\n");

	memset(sc, 0, sizeof(*sc));
	memset(&sc->sc_ugenfd, -1, sizeof(sc->sc_ugenfd));
	memset(&sc->sc_fdmodes, -1, sizeof(sc->sc_fdmodes));

	sc->sc_bus.usbrev = USBREV_2_0;
	sc->sc_bus.methods = &rumpusbhc_bus_methods;
	sc->sc_bus.hci_private = sc;
	sc->sc_bus.pipe_size = sizeof(struct rumpusbhc_pipe);
	sc->sc_devnum = maa->maa_unit;

	makeugendevstr(sc->sc_devnum, 0, buf);
	sc->sc_ugenfd[UGEN_EPT_CTRL] = rumpuser_open(buf, O_RDWR, &error);
	if (error)
		panic("rumpusbhc_attach: failed to open ctrl ept %s\n", buf);

	config_found(self, &sc->sc_bus, usbctlprint);

	/* whoah, like extreme XXX, bro */
	rhscintr();
}
