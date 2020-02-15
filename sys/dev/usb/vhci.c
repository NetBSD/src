/*	$NetBSD: vhci.c,v 1.6 2020/02/15 01:21:56 riastradh Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: vhci.c,v 1.6 2020/02/15 01:21:56 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/filedesc.h>

#include <machine/endian.h>

#include "ioconf.h"

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/usb/usbroothub.h>

#ifdef VHCI_DEBUG
#define DPRINTF(fmt, ...)	printf(fmt, __VA_ARGS__)
#else
#define DPRINTF(fmt, ...)	__nothing
#endif

static usbd_status vhci_open(struct usbd_pipe *);
static void vhci_softintr(void *);

static struct usbd_xfer *vhci_allocx(struct usbd_bus *, unsigned int);
static void vhci_freex(struct usbd_bus *, struct usbd_xfer *);
static void vhci_get_lock(struct usbd_bus *, kmutex_t **);
static int vhci_roothub_ctrl(struct usbd_bus *, usb_device_request_t *,
    void *, int);

static const struct usbd_bus_methods vhci_bus_methods = {
	.ubm_open =	vhci_open,
	.ubm_softint =	vhci_softintr,
	.ubm_dopoll =	NULL,
	.ubm_allocx =	vhci_allocx,
	.ubm_freex =	vhci_freex,
	.ubm_getlock =	vhci_get_lock,
	.ubm_rhctrl =	vhci_roothub_ctrl,
};

static usbd_status vhci_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status vhci_device_ctrl_start(struct usbd_xfer *);
static void vhci_device_ctrl_abort(struct usbd_xfer *);
static void vhci_device_ctrl_close(struct usbd_pipe *);
static void vhci_device_ctrl_cleartoggle(struct usbd_pipe *);
static void vhci_device_ctrl_done(struct usbd_xfer *);

static const struct usbd_pipe_methods vhci_device_ctrl_methods = {
	.upm_init =		NULL,
	.upm_fini =		NULL,
	.upm_transfer =		vhci_device_ctrl_transfer,
	.upm_start =		vhci_device_ctrl_start,
	.upm_abort =		vhci_device_ctrl_abort,
	.upm_close =		vhci_device_ctrl_close,
	.upm_cleartoggle =	vhci_device_ctrl_cleartoggle,
	.upm_done =		vhci_device_ctrl_done,
};

static usbd_status vhci_root_intr_transfer(struct usbd_xfer *);
static usbd_status vhci_root_intr_start(struct usbd_xfer *);
static void vhci_root_intr_abort(struct usbd_xfer *);
static void vhci_root_intr_close(struct usbd_pipe *);
static void vhci_root_intr_cleartoggle(struct usbd_pipe *);
static void vhci_root_intr_done(struct usbd_xfer *);

static const struct usbd_pipe_methods vhci_root_intr_methods = {
	.upm_init =		NULL,
	.upm_fini =		NULL,
	.upm_transfer =		vhci_root_intr_transfer,
	.upm_start =		vhci_root_intr_start,
	.upm_abort =		vhci_root_intr_abort,
	.upm_close =		vhci_root_intr_close,
	.upm_cleartoggle =	vhci_root_intr_cleartoggle,
	.upm_done =		vhci_root_intr_done,
};

typedef struct vhci_packet {
	TAILQ_ENTRY(vhci_packet) portlist;
	TAILQ_ENTRY(vhci_packet) xferlist;
	struct usbd_xfer *xfer; /* also vxfer */
	bool utoh;
	uint8_t *buf;
	size_t size;
	size_t cursor;
} vhci_packet_t;

typedef TAILQ_HEAD(, vhci_packet) vhci_packet_list_t;

typedef struct {
	kmutex_t lock;
	int status;
	int change;
	struct {
		vhci_packet_list_t usb_to_host;
		vhci_packet_list_t host_to_usb;
	} pkts_device_ctrl;
} vhci_port_t;

typedef struct {
	struct usbd_pipe pipe;
} vhci_pipe_t;

typedef struct vhci_xfer {
	/* General. */
	struct usbd_xfer xfer;

	/* vHCI-specific. */
	size_t refcnt;
	vhci_port_t *port;
	vhci_packet_list_t pkts;
	TAILQ_ENTRY(vhci_xfer) freelist;
} vhci_xfer_t;

typedef TAILQ_HEAD(, vhci_xfer) vhci_xfer_list_t;

#define VHCI_INDEX2PORT(idx)	(idx)
#define VHCI_NPORTS		4

typedef struct {
	device_t sc_dev;

	struct usbd_bus sc_bus;
	bool sc_dying;
	kmutex_t sc_lock;

	/*
	 * Intr Root. Used to attach the devices.
	 */
	struct usbd_xfer *sc_intrxfer;

	/*
	 * The ports. Zero is for the roothub, one and beyond for the USB
	 * devices.
	 */
	size_t sc_nports;
	vhci_port_t sc_port[VHCI_NPORTS];

	device_t sc_child; /* /dev/usb# device */
} vhci_softc_t;

typedef struct {
	u_int port;
	vhci_softc_t *softc;
} vhci_fd_t;

extern struct cfdriver vhci_cd;

/* -------------------------------------------------------------------------- */

static void
vhci_pkt_create(vhci_port_t *port, struct usbd_xfer *xfer, bool usb_to_host)
{
	vhci_xfer_t *vxfer = (vhci_xfer_t *)xfer;
	vhci_packet_list_t *reqlist, *pktlist;
	vhci_packet_t *req, *pkt = NULL;
	size_t refcnt = 0;

	/* Setup packet. */
	reqlist = &port->pkts_device_ctrl.host_to_usb;
	req = kmem_zalloc(sizeof(*req), KM_SLEEP);
	req->xfer = xfer;
	req->utoh = false;
	req->buf = (uint8_t *)&xfer->ux_request;
	req->size = sizeof(xfer->ux_request);
	req->cursor = 0;
	refcnt++;

	/* Data packet. */
	if (xfer->ux_length > 0) {
		if (usb_to_host) {
			pktlist = &port->pkts_device_ctrl.usb_to_host;
		} else {
			pktlist = &port->pkts_device_ctrl.host_to_usb;
		}
		pkt = kmem_zalloc(sizeof(*pkt), KM_SLEEP);
		pkt->xfer = xfer;
		pkt->utoh = usb_to_host;
		pkt->buf = xfer->ux_buf;
		pkt->size = xfer->ux_length;
		pkt->cursor = 0;
		refcnt++;
	}

	/* Insert in the xfer. */
	vxfer->refcnt = refcnt;
	vxfer->port = port;
	TAILQ_INIT(&vxfer->pkts);
	TAILQ_INSERT_TAIL(&vxfer->pkts, req, xferlist);
	if (pkt != NULL)
		TAILQ_INSERT_TAIL(&vxfer->pkts, pkt, xferlist);

	/* Insert in the port. */
	KASSERT(mutex_owned(&port->lock));
	TAILQ_INSERT_TAIL(reqlist, req, portlist);
	if (pkt != NULL)
		TAILQ_INSERT_TAIL(pktlist, pkt, portlist);
}

static void
vhci_pkt_destroy(vhci_softc_t *sc, vhci_packet_t *pkt)
{
	struct usbd_xfer *xfer = pkt->xfer;
	vhci_xfer_t *vxfer = (vhci_xfer_t *)xfer;
	vhci_port_t *port = vxfer->port;
	vhci_packet_list_t *pktlist;

	KASSERT(mutex_owned(&port->lock));

	if (pkt->utoh) {
		pktlist = &port->pkts_device_ctrl.usb_to_host;
	} else {
		pktlist = &port->pkts_device_ctrl.host_to_usb;
	}
	TAILQ_REMOVE(pktlist, pkt, portlist);

	TAILQ_REMOVE(&vxfer->pkts, pkt, xferlist);
	kmem_free(pkt, sizeof(*pkt));

	KASSERT(vxfer->refcnt > 0);
	vxfer->refcnt--;
	if (vxfer->refcnt > 0)
		return;

	KASSERT(TAILQ_FIRST(&vxfer->pkts) == NULL);
}

/* -------------------------------------------------------------------------- */

static usbd_status
vhci_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	struct usbd_bus *bus = dev->ud_bus;
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	vhci_softc_t *sc = bus->ub_hcpriv;
	uint8_t addr = dev->ud_addr;

	if (sc->sc_dying)
		return USBD_IOERROR;

	DPRINTF("%s: called, type=%d\n", __func__,
	    UE_GET_XFERTYPE(ed->bmAttributes));

	if (addr == bus->ub_rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			DPRINTF("%s: roothub_ctrl\n", __func__);
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			DPRINTF("%s: root_intr\n", __func__);
			pipe->up_methods = &vhci_root_intr_methods;
			break;
		default:
			DPRINTF("%s: inval\n", __func__);
			return USBD_INVAL;
		}
	} else {
		switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
		case UE_CONTROL:
			pipe->up_methods = &vhci_device_ctrl_methods;
			break;
		case UE_BULK:
		case UE_INTERRUPT:
		default:
			goto bad;
		}
	}

	return USBD_NORMAL_COMPLETION;

bad:
	return USBD_NOMEM;
}

static void
vhci_softintr(void *v)
{
	DPRINTF("%s: called\n", __func__);
}

static struct usbd_xfer *
vhci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	vhci_xfer_t *vxfer;

	vxfer = kmem_zalloc(sizeof(*vxfer), KM_SLEEP);
#ifdef DIAGNOSTIC
	vxfer->xfer.ux_state = XFER_BUSY;
#endif
	return (struct usbd_xfer *)vxfer;
}

static void
vhci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	vhci_xfer_t *vxfer = (vhci_xfer_t *)xfer;

	KASSERT(vxfer->refcnt == 0);
	KASSERT(TAILQ_FIRST(&vxfer->pkts) == NULL);

#ifdef DIAGNOSTIC
	vxfer->xfer.ux_state = XFER_FREE;
#endif
	kmem_free(vxfer, sizeof(*vxfer));
}

static void
vhci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	vhci_softc_t *sc = bus->ub_hcpriv;

	*lock = &sc->sc_lock;
}

static int
vhci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	vhci_softc_t *sc = bus->ub_hcpriv;
	vhci_port_t *port;
	usb_hub_descriptor_t hubd;
	uint16_t len, value, index;
	int totlen = 0;

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		switch (value) {
		case C(0, UDESC_DEVICE): {
			usb_device_descriptor_t devd;

			totlen = uimin(buflen, sizeof(devd));
			memcpy(&devd, buf, totlen);
			USETW(devd.idVendor, 0);
			USETW(devd.idProduct, 0);
			memcpy(buf, &devd, totlen);
			break;
		}
#define sd ((usb_string_descriptor_t *)buf)
		case C(1, UDESC_STRING):
			/* Vendor */
			totlen = usb_makestrdesc(sd, len, "NetBSD");
			break;
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "VHCI root hub");
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
			if (index < 1 || index >= sc->sc_nports) {
				return -1;
			}
			port = &sc->sc_port[VHCI_INDEX2PORT(index)];
			port->status |= UPS_C_PORT_RESET;
			break;
		case UHF_PORT_POWER:
			break;
		default:
			return -1;
		}
		break;

	/* Hub requests. */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index < 1 || index >= sc->sc_nports) {
			return -1;
		}
		port = &sc->sc_port[VHCI_INDEX2PORT(index)];
		switch (value) {
		case UHF_PORT_ENABLE:
			port->status &= ~UPS_PORT_ENABLED;
			break;
		case UHF_C_PORT_ENABLE:
			port->change |= UPS_C_PORT_ENABLED;
			break;
		default:
			return -1;
		}
		break;

	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		totlen = uimin(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);
		hubd.bNbrPorts = sc->sc_nports - 1;
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE;
		totlen = uimin(totlen, hubd.bDescLength);
		memcpy(buf, &hubd, totlen);
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		/* XXX The other HCs do this */
		memset(buf, 0, len);
		totlen = len;
		break;

	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER): {
		usb_port_status_t ps;

		if (index < 1 || index >= sc->sc_nports) {
			return -1;
		}
		port = &sc->sc_port[VHCI_INDEX2PORT(index)];
		USETW(ps.wPortStatus, port->status);
		USETW(ps.wPortChange, port->change);
		totlen = uimin(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	}
	default:
		/* default from usbroothub */
		return buflen;
	}

	return totlen;
}

/* -------------------------------------------------------------------------- */

static usbd_status
vhci_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;
	usbd_status err;

	DPRINTF("%s: called\n", __func__);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return vhci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
vhci_device_ctrl_start(struct usbd_xfer *xfer)
{
	usb_device_request_t *req = &xfer->ux_request;
	struct usbd_device *dev = xfer->ux_pipe->up_dev;
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;
	vhci_port_t *port;
	bool polling = sc->sc_bus.ub_usepolling;
	bool isread = (req->bmRequestType & UT_READ) != 0;
	int portno, ret;

	KASSERT(xfer->ux_rqflags & URQ_REQUEST);
	KASSERT(dev->ud_myhsport != NULL);
	portno = dev->ud_myhsport->up_portno;

	DPRINTF("%s: type=0x%02x, len=%d, isread=%d, portno=%d\n",
	    __func__, req->bmRequestType, UGETW(req->wLength), isread, portno);

	if (sc->sc_dying)
		return USBD_IOERROR;

	port = &sc->sc_port[portno];

	if (!polling)
		mutex_enter(&sc->sc_lock);

	mutex_enter(&port->lock);
	if (port->status & UPS_PORT_ENABLED) {
		xfer->ux_status = USBD_IN_PROGRESS;
		vhci_pkt_create(port, xfer, isread);
		ret = USBD_IN_PROGRESS;
	} else {
		ret = USBD_IOERROR;
	}
	mutex_exit(&port->lock);

	if (!polling)
		mutex_exit(&sc->sc_lock);

	return ret;
}

static void
vhci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	vhci_xfer_t *vxfer = (vhci_xfer_t *)xfer;
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;
	vhci_port_t *port = vxfer->port;
	vhci_packet_t *pkt;

	DPRINTF("%s: called\n", __func__);

	KASSERT(mutex_owned(&sc->sc_lock));

	callout_halt(&xfer->ux_callout, &sc->sc_lock);

	KASSERT(xfer->ux_status != USBD_CANCELLED);

	/* If anyone else beat us, we're done.  */
	if (xfer->ux_status != USBD_IN_PROGRESS)
		return;

	mutex_enter(&port->lock);
	while (vxfer->refcnt > 0) {
		pkt = TAILQ_FIRST(&vxfer->pkts);
		KASSERT(pkt != NULL);
		vhci_pkt_destroy(sc, pkt);
	}
	KASSERT(TAILQ_FIRST(&vxfer->pkts) == NULL);
	mutex_exit(&port->lock);

	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
	KASSERT(mutex_owned(&sc->sc_lock));
}

static void
vhci_device_ctrl_close(struct usbd_pipe *pipe)
{
	DPRINTF("%s: called\n", __func__);
}

static void
vhci_device_ctrl_cleartoggle(struct usbd_pipe *pipe)
{
	DPRINTF("%s: called\n", __func__);
}

static void
vhci_device_ctrl_done(struct usbd_xfer *xfer)
{
	DPRINTF("%s: called\n", __func__);
}

/* -------------------------------------------------------------------------- */

static usbd_status
vhci_root_intr_transfer(struct usbd_xfer *xfer)
{
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;
	usbd_status err;

	DPRINTF("%s: called\n", __func__);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return vhci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
vhci_root_intr_start(struct usbd_xfer *xfer)
{
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;
	const bool polling = sc->sc_bus.ub_usepolling;

	DPRINTF("%s: called, len=%zu\n", __func__, (size_t)xfer->ux_length);

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (!polling)
		mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

static void
vhci_root_intr_abort(struct usbd_xfer *xfer)
{
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;

	DPRINTF("%s: called\n", __func__);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	/* If xfer has already completed, nothing to do here.  */
	if (sc->sc_intrxfer == NULL)
		return;

	/*
	 * Otherwise, sc->sc_intrxfer had better be this transfer.
	 * Cancel it.
	 */
	KASSERT(sc->sc_intrxfer == xfer);
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);
	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

static void
vhci_root_intr_close(struct usbd_pipe *pipe)
{
	vhci_softc_t *sc __diagused = pipe->up_dev->ud_bus->ub_hcpriv;

	DPRINTF("%s: called\n", __func__);

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * Caller must guarantee the xfer has completed first, by
	 * closing the pipe only after normal completion or an abort.
	 */
	KASSERT(sc->sc_intrxfer == NULL);
}

static void
vhci_root_intr_cleartoggle(struct usbd_pipe *pipe)
{
	DPRINTF("%s: called\n", __func__);
}

static void
vhci_root_intr_done(struct usbd_xfer *xfer)
{
	vhci_softc_t *sc = xfer->ux_bus->ub_hcpriv;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Claim the xfer so it doesn't get completed again.  */
	KASSERT(sc->sc_intrxfer == xfer);
	KASSERT(xfer->ux_status != USBD_IN_PROGRESS);
	sc->sc_intrxfer = NULL;
}

/* -------------------------------------------------------------------------- */

struct vhci_ioc_get_info {
	/* General. */
	size_t nports;

	/* Current port. */
	u_int port;
	int status;
};

struct vhci_ioc_set_port {
	u_int port;
};

struct vhci_ioc_usb_attach {
	u_int port;
};

struct vhci_ioc_usb_detach {
	u_int port;
};

#define VHCI_IOC_GET_INFO	_IOR('V', 0, struct vhci_ioc_get_info)
#define VHCI_IOC_SET_PORT	_IOW('V', 1, struct vhci_ioc_set_port)
#define VHCI_IOC_USB_ATTACH	_IOW('V', 10, struct vhci_ioc_usb_attach)
#define VHCI_IOC_USB_DETACH	_IOW('V', 11, struct vhci_ioc_usb_detach)

static int
vhci_usb_attach(vhci_fd_t *vfd, struct vhci_ioc_usb_attach *args)
{
	vhci_softc_t *sc = vfd->softc;
	vhci_port_t *port;
	struct usbd_xfer *xfer;
	u_char *p;
	int ret = 0;

	if (args->port == 0 || args->port >= sc->sc_nports)
		return EINVAL;
	port = &sc->sc_port[args->port];

	mutex_enter(&sc->sc_lock);

	mutex_enter(&port->lock);
	port->status = UPS_CURRENT_CONNECT_STATUS | UPS_PORT_ENABLED |
	    UPS_PORT_POWER;
	port->change = UPS_C_CONNECT_STATUS | UPS_C_PORT_RESET;
	mutex_exit(&port->lock);

	xfer = sc->sc_intrxfer;

	if (xfer == NULL) {
		ret = ENOBUFS;
		goto done;
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	p = xfer->ux_buf;
	memset(p, 0, xfer->ux_length);
	p[0] = __BIT(args->port);
	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);

done:
	mutex_exit(&sc->sc_lock);
	return ret;
}

static void
vhci_port_flush(vhci_softc_t *sc, vhci_port_t *port)
{
	vhci_packet_list_t *pktlist;
	vhci_packet_t *pkt, *nxt;
	vhci_xfer_list_t vxferlist;
	vhci_xfer_t *vxfer;

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(mutex_owned(&port->lock));

	TAILQ_INIT(&vxferlist);

	pktlist = &port->pkts_device_ctrl.host_to_usb;
	TAILQ_FOREACH_SAFE(pkt, pktlist, portlist, nxt) {
		vxfer = (vhci_xfer_t *)pkt->xfer;
		KASSERT(vxfer->xfer.ux_status == USBD_IN_PROGRESS);
		vhci_pkt_destroy(sc, pkt);
		if (vxfer->refcnt == 0)
			TAILQ_INSERT_TAIL(&vxferlist, vxfer, freelist);
	}
	KASSERT(TAILQ_FIRST(pktlist) == NULL);

	pktlist = &port->pkts_device_ctrl.usb_to_host;
	TAILQ_FOREACH_SAFE(pkt, pktlist, portlist, nxt) {
		vxfer = (vhci_xfer_t *)pkt->xfer;
		KASSERT(vxfer->xfer.ux_status == USBD_IN_PROGRESS);
		vhci_pkt_destroy(sc, pkt);
		if (vxfer->refcnt == 0)
			TAILQ_INSERT_TAIL(&vxferlist, vxfer, freelist);
	}
	KASSERT(TAILQ_FIRST(pktlist) == NULL);

	while ((vxfer = TAILQ_FIRST(&vxferlist)) != NULL) {
		struct usbd_xfer *xfer = &vxfer->xfer;
		TAILQ_REMOVE(&vxferlist, vxfer, freelist);

		xfer->ux_status = USBD_TIMEOUT;
		usb_transfer_complete(xfer);
	}
}

static int
vhci_usb_detach(vhci_fd_t *vfd, struct vhci_ioc_usb_detach *args)
{
	vhci_softc_t *sc = vfd->softc;
	vhci_port_t *port;
	struct usbd_xfer *xfer;
	u_char *p;

	if (args->port == 0 || args->port >= sc->sc_nports)
		return EINVAL;
	port = &sc->sc_port[args->port];

	mutex_enter(&sc->sc_lock);

	xfer = sc->sc_intrxfer;
	if (xfer == NULL) {
		mutex_exit(&sc->sc_lock);
		return ENOBUFS;
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	mutex_enter(&port->lock);

	port->status = 0;
	port->change = UPS_C_CONNECT_STATUS | UPS_C_PORT_RESET;

	p = xfer->ux_buf;
	memset(p, 0, xfer->ux_length);
	p[0] = __BIT(args->port);
	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);
	vhci_port_flush(sc, port);

	mutex_exit(&port->lock);
	mutex_exit(&sc->sc_lock);
	return 0;
}

static int
vhci_get_info(vhci_fd_t *vfd, struct vhci_ioc_get_info *args)
{
	vhci_softc_t *sc = vfd->softc;
	vhci_port_t *port;

	port = &sc->sc_port[vfd->port];

	args->nports = VHCI_NPORTS;
	args->port = vfd->port;
	mutex_enter(&port->lock);
	args->status = port->status;
	mutex_exit(&port->lock);

	return 0;
}

static int
vhci_set_port(vhci_fd_t *vfd, struct vhci_ioc_set_port *args)
{
	vhci_softc_t *sc = vfd->softc;

	if (args->port == 0 || args->port >= sc->sc_nports)
		return EINVAL;

	vfd->port = args->port;

	return 0;
}

/* -------------------------------------------------------------------------- */

static dev_type_open(vhci_fd_open);

const struct cdevsw vhci_cdevsw = {
	.d_open = vhci_fd_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

static int vhci_fd_ioctl(file_t *, u_long, void *);
static int vhci_fd_close(file_t *);
static int vhci_fd_read(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int vhci_fd_write(struct file *, off_t *, struct uio *, kauth_cred_t, int);

const struct fileops vhci_fileops = {
	.fo_read = vhci_fd_read,
	.fo_write = vhci_fd_write,
	.fo_ioctl = vhci_fd_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = vhci_fd_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = NULL,
};

static int
vhci_fd_open(dev_t dev, int flags, int type, struct lwp *l)
{
	vhci_fd_t *vfd;
	struct file *fp;
	int error, fd;

	if (minor(dev) != 0)
		return EXDEV;
	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	vfd = kmem_alloc(sizeof(*vfd), KM_SLEEP);
	vfd->port = 1;
	vfd->softc = device_lookup_private(&vhci_cd, minor(dev));

	return fd_clone(fp, fd, flags, &vhci_fileops, vfd);
}

static int
vhci_fd_close(file_t *fp)
{
	struct vhci_ioc_usb_detach args;
	vhci_fd_t *vfd = fp->f_data;
	int ret __diagused;

	KASSERT(vfd != NULL);

	args.port = vfd->port;
	ret = vhci_usb_detach(vfd, &args);
	KASSERT(ret == 0);

	kmem_free(vfd, sizeof(*vfd));
	fp->f_data = NULL;

	return 0;
}

static int
vhci_fd_read(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	vhci_fd_t *vfd = fp->f_data;
	vhci_softc_t *sc = vfd->softc;
	vhci_packet_list_t *pktlist;
	vhci_packet_t *pkt, *nxt;
	vhci_xfer_list_t vxferlist;
	vhci_xfer_t *vxfer;
	vhci_port_t *port;
	int error = 0;
	uint8_t *buf;
	size_t size;

	if (uio->uio_resid == 0)
		return 0;
	port = &sc->sc_port[vfd->port];
	pktlist = &port->pkts_device_ctrl.host_to_usb;

	TAILQ_INIT(&vxferlist);

	mutex_enter(&port->lock);

	if (!(port->status & UPS_PORT_ENABLED)) {
		error = ENOBUFS;
		goto out;
	}

	TAILQ_FOREACH_SAFE(pkt, pktlist, portlist, nxt) {
		vxfer = (vhci_xfer_t *)pkt->xfer;
		buf = pkt->buf + pkt->cursor;
		KASSERT(pkt->size >= pkt->cursor);
		size = uimin(uio->uio_resid, pkt->size - pkt->cursor);

		KASSERT(vxfer->xfer.ux_status == USBD_IN_PROGRESS);

		error = uiomove(buf, size, uio);
		if (error) {
			DPRINTF("%s: error = %d\n", __func__, error);
			goto out;
		}

		pkt->cursor += size;

		if (pkt->cursor == pkt->size) {
			vhci_pkt_destroy(sc, pkt);
			if (vxfer->refcnt == 0) {
				TAILQ_INSERT_TAIL(&vxferlist, vxfer, freelist);
			}
		}
		if (uio->uio_resid == 0) {
			break;
		}
	}

out:
	mutex_exit(&port->lock);

	while ((vxfer = TAILQ_FIRST(&vxferlist)) != NULL) {
		struct usbd_xfer *xfer = &vxfer->xfer;
		TAILQ_REMOVE(&vxferlist, vxfer, freelist);

		mutex_enter(&sc->sc_lock);
		xfer->ux_actlen = xfer->ux_length;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);
	}

	return error;
}

static int
vhci_fd_write(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	vhci_fd_t *vfd = fp->f_data;
	vhci_softc_t *sc = vfd->softc;
	vhci_packet_list_t *pktlist;
	vhci_packet_t *pkt, *nxt;
	vhci_xfer_list_t vxferlist;
	vhci_xfer_t *vxfer;
	vhci_port_t *port;
	int error = 0;
	uint8_t *buf;
	size_t size;

	if (uio->uio_resid == 0)
		return 0;
	port = &sc->sc_port[vfd->port];
	pktlist = &port->pkts_device_ctrl.usb_to_host;

	TAILQ_INIT(&vxferlist);

	mutex_enter(&port->lock);

	if (!(port->status & UPS_PORT_ENABLED)) {
		error = ENOBUFS;
		goto out;
	}

	TAILQ_FOREACH_SAFE(pkt, pktlist, portlist, nxt) {
		vxfer = (vhci_xfer_t *)pkt->xfer;
		buf = pkt->buf + pkt->cursor;
		KASSERT(pkt->size >= pkt->cursor);
		size = uimin(uio->uio_resid, pkt->size - pkt->cursor);

		KASSERT(vxfer->xfer.ux_status == USBD_IN_PROGRESS);

		error = uiomove(buf, size, uio);
		if (error) {
			DPRINTF("%s: error = %d\n", __func__, error);
			goto out;
		}

		pkt->cursor += size;

		if (pkt->cursor == pkt->size) {
			vhci_pkt_destroy(sc, pkt);
			if (vxfer->refcnt == 0) {
				TAILQ_INSERT_TAIL(&vxferlist, vxfer, freelist);
			}
		}
		if (uio->uio_resid == 0) {
			break;
		}
	}

out:
	mutex_exit(&port->lock);

	while ((vxfer = TAILQ_FIRST(&vxferlist)) != NULL) {
		struct usbd_xfer *xfer = &vxfer->xfer;
		TAILQ_REMOVE(&vxferlist, vxfer, freelist);

		mutex_enter(&sc->sc_lock);
		xfer->ux_actlen = xfer->ux_length;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);
	}

	return error;
}

static int
vhci_fd_ioctl(file_t *fp, u_long cmd, void *data)
{
	vhci_fd_t *vfd = fp->f_data;

	KASSERT(vfd != NULL);

	switch (cmd) {
	case VHCI_IOC_GET_INFO:
		return vhci_get_info(vfd, data);
	case VHCI_IOC_SET_PORT:
		return vhci_set_port(vfd, data);
	case VHCI_IOC_USB_ATTACH:
		return vhci_usb_attach(vfd, data);
	case VHCI_IOC_USB_DETACH:
		return vhci_usb_detach(vfd, data);
	default:
		return EINVAL;
	}
}

/* -------------------------------------------------------------------------- */

static int vhci_match(device_t, cfdata_t, void *);
static void vhci_attach(device_t, device_t, void *);
static int vhci_activate(device_t, enum devact);

CFATTACH_DECL_NEW(vhci, sizeof(vhci_softc_t), vhci_match, vhci_attach,
    NULL, vhci_activate);

void
vhciattach(int nunits)
{
	static struct cfdata vhci_cfdata = {
		.cf_name = "vhci",
		.cf_atname = "vhci",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
	};
	int error;

	error = config_cfattach_attach(vhci_cd.cd_name, &vhci_ca);
	if (error) {
		aprint_error("%s: unable to register cfattach\n",
		    vhci_cd.cd_name);
		(void)config_cfdriver_detach(&vhci_cd);
		return;
	}

	config_attach_pseudo(&vhci_cfdata);
}

static int
vhci_activate(device_t self, enum devact act)
{
	vhci_softc_t *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static int
vhci_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
vhci_attach(device_t parent, device_t self, void *aux)
{
	vhci_softc_t *sc = device_private(self);
	vhci_port_t *port;
	size_t i;

	sc->sc_dev = self;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_usedma = false;
	sc->sc_bus.ub_methods = &vhci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(vhci_pipe_t);
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_dying = false;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);

	sc->sc_nports = VHCI_NPORTS;
	for (i = 0; i < sc->sc_nports; i++) {
		port = &sc->sc_port[i];
		mutex_init(&port->lock, MUTEX_DEFAULT, IPL_SOFTUSB);
		TAILQ_INIT(&port->pkts_device_ctrl.usb_to_host);
		TAILQ_INIT(&port->pkts_device_ctrl.host_to_usb);
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}
