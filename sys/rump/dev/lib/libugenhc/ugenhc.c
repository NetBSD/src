/*	$NetBSD: ugenhc.c,v 1.22.4.12 2015/10/22 11:15:42 skrll Exp $	*/

/*
 * Copyright (c) 2009, 2010 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: ugenhc.c,v 1.22.4.12 2015/10/22 11:15:42 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/mutex.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbhid.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbroothub.h>

#include <rump/rumpuser.h>

#include "ugenhc_user.h"

#include "rump_private.h"
#include "rump_dev_private.h"

#define UGEN_NEPTS 16
#define UGEN_EPT_CTRL 0 /* ugenx.00 is the control endpoint */

struct ugenhc_softc {
	struct usbd_bus sc_bus;
	int sc_devnum;

	int sc_ugenfd[UGEN_NEPTS];
	int sc_fdmodes[UGEN_NEPTS];

	int sc_port_status;
	int sc_port_change;

	struct lwp *sc_rhintr;
	struct usbd_xfer *sc_intrxfer;

	kmutex_t sc_lock;
};

static int	ugenhc_probe(device_t, cfdata_t, void *);
static void	ugenhc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ugenhc, sizeof(struct ugenhc_softc),
	ugenhc_probe, ugenhc_attach, NULL, NULL);

struct rusb_xfer {
	struct usbd_xfer rusb_xfer;
	int rusb_status; /* now this is a cheap trick */
};
#define RUSB(x) ((struct rusb_xfer *)x)

#define UGENHC_BUS2SC(bus)	((bus)->ub_hcpriv)
#define UGENHC_PIPE2SC(pipe)	UGENHC_BUS2SC((pipe)->up_dev->ud_bus)
#define UGENHC_XFER2SC(pipe)	UGENHC_BUS2SC((xfer)->ux_bus)

#define UGENDEV_BASESTR "/dev/ugen"
#define UGENDEV_BUFSIZE 32
static void
makeugendevstr(int devnum, int endpoint, char *buf, size_t len)
{

	CTASSERT(UGENDEV_BUFSIZE > sizeof(UGENDEV_BASESTR)+sizeof("0.00")+1);
	snprintf(buf, len, "%s%d.%02d", UGENDEV_BASESTR, devnum, endpoint);
}

static int
ugenhc_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct ugenhc_softc *sc = UGENHC_BUS2SC(bus);
	int totlen = 0;
	uint16_t len, value;

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		switch (value) {
		case C(0, UDESC_DEVICE): {
			usb_device_descriptor_t devd;

			totlen = min(buflen, sizeof(devd));
			memcpy(&devd, buf, totlen);
			USETW(devd.idVendor, 0x7275);
			USETW(devd.idProduct, 0x6d72);
			memcpy(buf, &devd, totlen);
			break;
		}
#define sd ((usb_string_descriptor_t *)buf)
		case C(1, UDESC_STRING):
			/* Vendor */
			totlen = usb_makestrdesc(sd, len, "rod nevada");
			break;
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "RUMPUSBHC root hub");
			break;
#undef sd
		default:
			/* default from usbroothub */
			return buflen;
		}
		break;

	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		switch (value) {
		case UHF_PORT_RESET:
			sc->sc_port_change |= UPS_C_PORT_RESET;
			break;
		case UHF_PORT_POWER:
			break;
		default:
			return -1;
		}
		break;

	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		sc->sc_port_change &= ~value;
		break;

	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		totlen = buflen;
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		/* huh?  other hc's do this */
		memset(buf, 0, len);
		totlen = len;
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER): {
		usb_port_status_t ps;

		USETW(ps.wPortStatus, sc->sc_port_status);
		USETW(ps.wPortChange, sc->sc_port_change);
		totlen = min(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	}
	default:
		/* default from usbroothub */
		return buflen;
	}

	return totlen;
}

static usbd_status
rumpusb_device_ctrl_start(struct usbd_xfer *xfer)
{
	usb_device_request_t *req = &xfer->ux_request;
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	uint8_t *buf = NULL;
	int len, totlen;
	int value;
	int err = 0;
	int ru_error, mightfail = 0;

	len = totlen = UGETW(req->wLength);
	if (len)
		buf = xfer->ux_buf;
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
			if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_DEVICE_DESC, &uddesc, &ru_error) == -1) {
				err = EIO;
				goto ret;
			}
			memcpy(buf, &uddesc, totlen);
			}

			break;
		case UDESC_CONFIG:
			{
			struct usb_full_desc ufdesc;
			ufdesc.ufd_config_index = value & 0xff;
			ufdesc.ufd_size = len;
			ufdesc.ufd_data = buf;
			memset(buf, 0, len);
			if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_FULL_DESC, &ufdesc, &ru_error) == -1) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = ufdesc.ufd_size;
			}
			break;

		case UDESC_STRING:
			{
			struct usb_device_info udi;

			if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
			    USB_GET_DEVICEINFO, &udi, &ru_error) == -1) {
				printf("ugenhc: get dev info failed: %d\n",
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
		if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
		    USB_SET_CONFIG, &value, &ru_error) == -1) {
			printf("ugenhc: set config failed: %d\n",
			    ru_error);
			err = USBD_IOERROR;
			goto ret;
		}
		break;

	case C(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
		{
		struct usb_alt_interface uai;

		totlen = 0;
		uai.uai_interface_index = UGETW(req->wIndex);
		uai.uai_alt_no = value;
		if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
		    USB_SET_ALTINTERFACE, &uai, &ru_error) == -1) {
			printf("ugenhc: set alt interface failed: %d\n",
			    ru_error);
			err = USBD_IOERROR;
			goto ret;
		}
		break;
		}

	/*
	 * This request might fail unknown reasons.  "EIO" doesn't
	 * give much help, and debugging the host ugen would be
	 * necessary.  However, since it doesn't seem to really
	 * affect anything, just let it fail for now.
	 */
	case C(0x00, UT_WRITE_CLASS_INTERFACE):
		mightfail = 1;
		/*FALLTHROUGH*/

	/*
	 * XXX: don't wildcard these yet.  I want to better figure
	 * out what to trap here.  This is kinda silly, though ...
	 */

	case C(0x01, UT_WRITE_VENDOR_DEVICE):
	case C(0x06, UT_WRITE_VENDOR_DEVICE):
	case C(0x07, UT_READ_VENDOR_DEVICE):
	case C(0x09, UT_READ_VENDOR_DEVICE):
	case C(0xfe, UT_READ_CLASS_INTERFACE):
	case C(0x01, UT_READ_CLASS_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
	case C(UR_GET_DESCRIPTOR, UT_READ_INTERFACE):
	case C(0xff, UT_WRITE_CLASS_INTERFACE):
	case C(0x20, UT_WRITE_CLASS_INTERFACE):
	case C(0x22, UT_WRITE_CLASS_INTERFACE):
	case C(0x0a, UT_WRITE_CLASS_INTERFACE):
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
	case C(0x00, UT_WRITE_CLASS_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_DEVICE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
	case C(UR_SET_REPORT, UT_WRITE_CLASS_INTERFACE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_ENDPOINT):
		{
		struct usb_ctl_request ucr;

		memcpy(&ucr.ucr_request, req, sizeof(ucr.ucr_request));
		ucr.ucr_data = buf;
		if (rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[UGEN_EPT_CTRL],
		    USB_DO_REQUEST, &ucr, &ru_error) == -1) {
			if (!mightfail) {
				panic("request failed: %d", ru_error);
			} else {
				err = ru_error;
			}
		}
		}
		break;

	default:
		panic("unhandled request");
		break;
	}
	xfer->ux_actlen = totlen;
	err = USBD_NORMAL_COMPLETION;

 ret:
	xfer->ux_status = err;
	mutex_enter(&sc->sc_lock);
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

static usbd_status
rumpusb_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	usbd_status err;

	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	return rumpusb_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static void
rumpusb_device_ctrl_abort(struct usbd_xfer *xfer)
{

}

static void
rumpusb_device_ctrl_close(struct usbd_pipe *pipe)
{

}

static void
rumpusb_device_ctrl_cleartoggle(struct usbd_pipe *pipe)
{

}

static void
rumpusb_device_ctrl_done(struct usbd_xfer *xfer)
{

}

static const struct usbd_pipe_methods rumpusb_device_ctrl_methods = {
	.upm_transfer =	rumpusb_device_ctrl_transfer,
	.upm_start =	rumpusb_device_ctrl_start,
	.upm_abort =	rumpusb_device_ctrl_abort,
	.upm_close =	rumpusb_device_ctrl_close,
	.upm_cleartoggle =	rumpusb_device_ctrl_cleartoggle,
	.upm_done =	rumpusb_device_ctrl_done,
};

static void
rhscintr(void *arg)
{
	char buf[UGENDEV_BUFSIZE];
	struct ugenhc_softc *sc = arg;
	struct usbd_xfer *xfer;
	int fd, error;

	makeugendevstr(sc->sc_devnum, 0, buf, sizeof(buf));

	for (;;) {
		/*
		 * Detect device attach.
		 */

		for (;;) {
			error = rumpuser_open(buf, RUMPUSER_OPEN_RDWR, &fd);
			if (error == 0)
				break;
			kpause("ugwait", false, hz/4, NULL);
		}

		sc->sc_ugenfd[UGEN_EPT_CTRL] = fd;
		sc->sc_port_status = UPS_CURRENT_CONNECT_STATUS
		    | UPS_PORT_ENABLED | UPS_PORT_POWER;
		sc->sc_port_change = UPS_C_CONNECT_STATUS | UPS_C_PORT_RESET;

		xfer = sc->sc_intrxfer;
		memset(xfer->ux_buffer, 0xff, xfer->ux_length);
		xfer->ux_actlen = xfer->ux_length;
		xfer->ux_status = USBD_NORMAL_COMPLETION;

		mutex_enter(&sc->sc_lock);
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);

		kpause("ugwait2", false, hz, NULL);

		/*
		 * Detect device detach.
		 */

		for (;;) {
			fd = rumpuser_open(buf, RUMPUSER_OPEN_RDWR, &error);
			if (fd == -1)
				break;

			error = rumpuser_close(fd);
			kpause("ugwait2", false, hz/4, NULL);
		}

		sc->sc_port_status = ~(UPS_CURRENT_CONNECT_STATUS
		    | UPS_PORT_ENABLED | UPS_PORT_POWER);
		sc->sc_port_change = UPS_C_CONNECT_STATUS | UPS_C_PORT_RESET;

		error = rumpuser_close(sc->sc_ugenfd[UGEN_EPT_CTRL]);
		sc->sc_ugenfd[UGEN_EPT_CTRL] = -1;

		xfer = sc->sc_intrxfer;
		memset(xfer->ux_buffer, 0xff, xfer->ux_length);
		xfer->ux_actlen = xfer->ux_length;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		mutex_enter(&sc->sc_lock);
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);

		kpause("ugwait3", false, hz, NULL);
	}

	kthread_exit(0);
}

static usbd_status
rumpusb_root_intr_start(struct usbd_xfer *xfer)
{
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	int error;

	mutex_enter(&sc->sc_lock);
	sc->sc_intrxfer = xfer;
	if (!sc->sc_rhintr) {
		error = kthread_create(PRI_NONE, 0, NULL,
		    rhscintr, sc, &sc->sc_rhintr, "ugenrhi");
		if (error)
			xfer->ux_status = USBD_IOERROR;
	}
	mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

static usbd_status
rumpusb_root_intr_transfer(struct usbd_xfer *xfer)
{
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	usbd_status err;

	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	return rumpusb_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static void
rumpusb_root_intr_abort(struct usbd_xfer *xfer)
{

}

static void
rumpusb_root_intr_close(struct usbd_pipe *pipe)
{

}

static void
rumpusb_root_intr_cleartoggle(struct usbd_pipe *pipe)
{

}

static void
rumpusb_root_intr_done(struct usbd_xfer *xfer)
{

}

static const struct usbd_pipe_methods rumpusb_root_intr_methods = {
	.upm_transfer =	rumpusb_root_intr_transfer,
	.upm_start =	rumpusb_root_intr_start,
	.upm_abort =	rumpusb_root_intr_abort,
	.upm_close =	rumpusb_root_intr_close,
	.upm_cleartoggle =	rumpusb_root_intr_cleartoggle,
	.upm_done =	rumpusb_root_intr_done,
};

static usbd_status
rumpusb_device_bulk_start(struct usbd_xfer *xfer)
{
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	size_t n, done;
	bool isread;
	int len, error, endpt;
	uint8_t *buf;
	int xfererr = USBD_NORMAL_COMPLETION;
	int shortval, i;

	ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	endpt = ed->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	endpt = UE_GET_ADDR(endpt);
	KASSERT(endpt < UGEN_NEPTS);

	buf = KERNADDR(&xfer->ux_dmabuf, 0);
	done = 0;
	if ((ed->bmAttributes & UE_XFERTYPE) == UE_ISOCHRONOUS) {
		for (i = 0, len = 0; i < xfer->ux_nframes; i++)
			len += xfer->ux_frlengths[i];
	} else {
		KASSERT(xfer->ux_length);
		len = xfer->ux_length;
	}
	shortval = (xfer->ux_flags & USBD_SHORT_XFER_OK) != 0;

	while (RUSB(xfer)->rusb_status == 0) {
		if (isread) {
			struct rumpuser_iovec iov;

			rumpcomp_ugenhc_ioctl(sc->sc_ugenfd[endpt],
			    USB_SET_SHORT_XFER, &shortval, &error);
			iov.iov_base = buf+done;
			iov.iov_len = len-done;
			error = rumpuser_iovread(sc->sc_ugenfd[endpt], &iov, 1,
			    RUMPUSER_IOV_NOSEEK, &n);
			if (error) {
				n = 0;
				if (done == 0) {
					if (error == ETIMEDOUT)
						continue;
					xfererr = USBD_IOERROR;
					goto out;
				}
			}
			done += n;
			if (done == len)
				break;
		} else {
			struct rumpuser_iovec iov;

			iov.iov_base = buf;
			iov.iov_len = len;
			error = rumpuser_iovwrite(sc->sc_ugenfd[endpt], &iov, 1,
			    RUMPUSER_IOV_NOSEEK, &n);
			done = n;
			if (done == len)
				break;
			else if (!error)
				panic("short write");

			xfererr = USBD_IOERROR;
			goto out;
		}

		if (shortval) {
			/*
			 * Holy XXX, bitman.  I get >16byte interrupt
			 * transfers from ugen in 16 byte chunks.
			 * Don't know how to better fix this for now.
			 * Of course this hack will fail e.g. if someone
			 * sports other magic values or if the transfer
			 * happens to be an integral multiple of 16
			 * in size ....
			 */
			if ((ed->bmAttributes & UE_XFERTYPE) == UE_INTERRUPT
			    && n == 16) {
				continue;
			} else {
				break;
			}
		}
	}

	if (RUSB(xfer)->rusb_status == 0) {
		xfer->ux_actlen = done;
	} else {
		xfererr = USBD_CANCELLED;
		RUSB(xfer)->rusb_status = 2;
	}
 out:
	if ((ed->bmAttributes & UE_XFERTYPE) == UE_ISOCHRONOUS)
		if (done != len)
			panic("lazy bum");
	xfer->ux_status = xfererr;
	mutex_enter(&sc->sc_lock);
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
	return USBD_IN_PROGRESS;
}

static void
doxfer_kth(void *arg)
{
	struct usbd_pipe *pipe = arg;
	struct ugenhc_softc *sc = UGENHC_PIPE2SC(pipe);

	mutex_enter(&sc->sc_lock);
	do {
		struct usbd_xfer *xfer = SIMPLEQ_FIRST(&pipe->up_queue);
		mutex_exit(&sc->sc_lock);
		rumpusb_device_bulk_start(xfer);
		mutex_enter(&sc->sc_lock);
	} while (!SIMPLEQ_EMPTY(&pipe->up_queue));
	mutex_exit(&sc->sc_lock);
	kthread_exit(0);
}

static usbd_status
rumpusb_device_bulk_transfer(struct usbd_xfer *xfer)
{
	struct ugenhc_softc *sc = UGENHC_XFER2SC(xfer);
	usbd_status err;

	if (!rump_threads) {
		/* XXX: lie about supporting async transfers */
		if ((xfer->ux_flags & USBD_SYNCHRONOUS) == 0) {
			printf("non-threaded rump does not support "
			    "async transfers.\n");
			return USBD_IN_PROGRESS;
		}

		mutex_enter(&sc->sc_lock);
		err = usb_insert_transfer(xfer);
		mutex_exit(&sc->sc_lock);
		if (err)
			return err;

		return rumpusb_device_bulk_start(
		    SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
	} else {
		mutex_enter(&sc->sc_lock);
		err = usb_insert_transfer(xfer);
		mutex_exit(&sc->sc_lock);
		if (err)
			return err;
		kthread_create(PRI_NONE, 0, NULL, doxfer_kth, xfer->ux_pipe, NULL,
		    "rusbhcxf");

		return USBD_IN_PROGRESS;
	}
}

/* wait for transfer to abort.  yea, this is cheesy (from a spray can) */
static void
rumpusb_device_bulk_abort(struct usbd_xfer *xfer)
{
	struct rusb_xfer *rx = RUSB(xfer);

	rx->rusb_status = 1;
	while (rx->rusb_status < 2) {
		kpause("jopo", false, hz/10, NULL);
	}
}

static void
rumpusb_device_bulk_close(struct usbd_pipe *pipe)
{
	struct ugenhc_softc *sc = UGENHC_PIPE2SC(pipe);
	int endpt = pipe->up_endpoint->ue_edesc->bEndpointAddress;
	struct usbd_xfer *xfer;

	KASSERT(mutex_owned(&sc->sc_lock));

	endpt = UE_GET_ADDR(endpt);

	while ((xfer = SIMPLEQ_FIRST(&pipe->up_queue)) != NULL)
		rumpusb_device_bulk_abort(xfer);

	rumpuser_close(sc->sc_ugenfd[endpt]);
	sc->sc_ugenfd[endpt] = -1;
	sc->sc_fdmodes[endpt] = -1;
}

static void
rumpusb_device_bulk_cleartoggle(struct usbd_pipe *pipe)
{

}

static void
rumpusb_device_bulk_done(struct usbd_xfer *xfer)
{

}

static const struct usbd_pipe_methods rumpusb_device_bulk_methods = {
	.upm_transfer =	rumpusb_device_bulk_transfer,
	.upm_start =	rumpusb_device_bulk_start,
	.upm_abort =	rumpusb_device_bulk_abort,
	.upm_close =	rumpusb_device_bulk_close,
	.upm_cleartoggle =	rumpusb_device_bulk_cleartoggle,
	.upm_done =	rumpusb_device_bulk_done,
};

static usbd_status
ugenhc_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	struct ugenhc_softc *sc = UGENHC_PIPE2SC(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	uint8_t rhaddr = dev->ud_bus->ub_rhaddr;
	uint8_t addr = dev->ud_addr;
	uint8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	char buf[UGENDEV_BUFSIZE];
	int endpt, oflags, error;
	int fd, val;

	if (addr == rhaddr) {
		switch (xfertype) {
		case UE_CONTROL:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_INTERRUPT:
			pipe->up_methods = &rumpusb_root_intr_methods;
			break;
		default:
			panic("%d not supported", xfertype);
			break;
		}
	} else {
		switch (xfertype) {
		case UE_CONTROL:
			pipe->up_methods = &rumpusb_device_ctrl_methods;
			break;
		case UE_INTERRUPT:
		case UE_BULK:
		case UE_ISOCHRONOUS:
			pipe->up_methods = &rumpusb_device_bulk_methods;
			endpt = pipe->up_endpoint->ue_edesc->bEndpointAddress;
			if (UE_GET_DIR(endpt) == UE_DIR_IN) {
				oflags = O_RDONLY;
			} else {
				oflags = O_WRONLY;
			}
			endpt = UE_GET_ADDR(endpt);

			if (oflags != O_RDONLY && xfertype == UE_ISOCHRONOUS) {
				printf("WARNING: faking isoc write open\n");
				oflags = O_RDONLY;
			}

			if (sc->sc_fdmodes[endpt] == oflags
			    || sc->sc_fdmodes[endpt] == O_RDWR)
				break;

			if (sc->sc_fdmodes[endpt] != -1) {
				/* XXX: closing from under someone? */
				error = rumpuser_close(sc->sc_ugenfd[endpt]);
				oflags = O_RDWR;
			}

			makeugendevstr(sc->sc_devnum, endpt, buf, sizeof(buf));
			/* XXX: theoretically should convert oflags */
			error = rumpuser_open(buf, oflags, &fd);
			if (error != 0) {
				return USBD_INVAL; /* XXX: no mapping */
			}
			val = 100;
			if (rumpcomp_ugenhc_ioctl(fd, USB_SET_TIMEOUT, &val,
			    &error) == -1)
				panic("timeout set failed");
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
ugenhc_softint(void *arg)
{

}

static void
ugenhc_poll(struct usbd_bus *ubus)
{

}

static struct usbd_xfer *
ugenhc_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct usbd_xfer *xfer;

	xfer = kmem_zalloc(sizeof(struct usbd_xfer), KM_SLEEP);
	xfer->ux_state = XFER_BUSY;

	return xfer;
}

static void
ugenhc_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{

	kmem_free(xfer, sizeof(struct usbd_xfer));
}


static void
ugenhc_getlock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct ugenhc_softc *sc = UGENHC_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

struct ugenhc_pipe {
	struct usbd_pipe pipe;
};

static const struct usbd_bus_methods ugenhc_bus_methods = {
	.ubm_open =	ugenhc_open,
	.ubm_softint =	ugenhc_softint,
	.ubm_dopoll =	ugenhc_poll,
	.ubm_allocx = 	ugenhc_allocx,
	.ubm_freex =	ugenhc_freex,
	.ubm_getlock =	ugenhc_getlock,
	.ubm_rhctrl =	ugenhc_roothub_ctrl,
};

static int
ugenhc_probe(device_t parent, cfdata_t match, void *aux)
{
	char buf[UGENDEV_BUFSIZE];

	makeugendevstr(match->cf_unit, 0, buf, sizeof(buf));
	if (rumpuser_getfileinfo(buf, NULL, NULL) != 0)
		return 0;

	return 1;
}

static void
ugenhc_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	struct ugenhc_softc *sc = device_private(self);

	aprint_normal("\n");

	memset(sc, 0, sizeof(*sc));
	memset(&sc->sc_ugenfd, -1, sizeof(sc->sc_ugenfd));
	memset(&sc->sc_fdmodes, -1, sizeof(sc->sc_fdmodes));

	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_methods = &ugenhc_bus_methods;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_pipesize = sizeof(struct ugenhc_pipe);
	sc->sc_bus.ub_usedma = false;
	sc->sc_devnum = maa->maa_unit;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	config_found(self, &sc->sc_bus, usbctlprint);
}
