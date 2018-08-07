/*	$NetBSD: dwc2.c,v 1.51 2018/08/07 16:35:08 skrll Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: dwc2.c,v 1.51 2018/08/07 16:35:08 skrll Exp $");

#include "opt_usb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/cpu.h>

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbroothub.h>

#include <dwc2/dwc2.h>
#include <dwc2/dwc2var.h>

#include "dwc2_core.h"
#include "dwc2_hcd.h"

#ifdef DWC2_COUNTERS
#define	DWC2_EVCNT_ADD(a,b)	((void)((a).ev_count += (b)))
#else
#define	DWC2_EVCNT_ADD(a,b)	do { } while (/*CONSTCOND*/0)
#endif
#define	DWC2_EVCNT_INCR(a)	DWC2_EVCNT_ADD((a), 1)

#ifdef DWC2_DEBUG
#define	DPRINTFN(n,fmt,...) do {			\
	if (dwc2debug >= (n)) {			\
		printf("%s: " fmt,			\
		__FUNCTION__,## __VA_ARGS__);		\
	}						\
} while (0)
#define	DPRINTF(...)	DPRINTFN(1, __VA_ARGS__)
int dwc2debug = 0;
#else
#define	DPRINTF(...) do { } while (0)
#define	DPRINTFN(...) do { } while (0)
#endif

Static usbd_status	dwc2_open(struct usbd_pipe *);
Static void		dwc2_poll(struct usbd_bus *);
Static void		dwc2_softintr(void *);

Static struct usbd_xfer *
			dwc2_allocx(struct usbd_bus *, unsigned int);
Static void		dwc2_freex(struct usbd_bus *, struct usbd_xfer *);
Static void		dwc2_get_lock(struct usbd_bus *, kmutex_t **);
Static int		dwc2_roothub_ctrl(struct usbd_bus *, usb_device_request_t *,
			    void *, int);

Static usbd_status	dwc2_root_intr_transfer(struct usbd_xfer *);
Static usbd_status	dwc2_root_intr_start(struct usbd_xfer *);
Static void		dwc2_root_intr_abort(struct usbd_xfer *);
Static void		dwc2_root_intr_close(struct usbd_pipe *);
Static void		dwc2_root_intr_done(struct usbd_xfer *);

Static usbd_status	dwc2_device_ctrl_transfer(struct usbd_xfer *);
Static usbd_status	dwc2_device_ctrl_start(struct usbd_xfer *);
Static void		dwc2_device_ctrl_abort(struct usbd_xfer *);
Static void		dwc2_device_ctrl_close(struct usbd_pipe *);
Static void		dwc2_device_ctrl_done(struct usbd_xfer *);

Static usbd_status	dwc2_device_bulk_transfer(struct usbd_xfer *);
Static void		dwc2_device_bulk_abort(struct usbd_xfer *);
Static void		dwc2_device_bulk_close(struct usbd_pipe *);
Static void		dwc2_device_bulk_done(struct usbd_xfer *);

Static usbd_status	dwc2_device_intr_transfer(struct usbd_xfer *);
Static usbd_status	dwc2_device_intr_start(struct usbd_xfer *);
Static void		dwc2_device_intr_abort(struct usbd_xfer *);
Static void		dwc2_device_intr_close(struct usbd_pipe *);
Static void		dwc2_device_intr_done(struct usbd_xfer *);

Static usbd_status	dwc2_device_isoc_transfer(struct usbd_xfer *);
Static void		dwc2_device_isoc_abort(struct usbd_xfer *);
Static void		dwc2_device_isoc_close(struct usbd_pipe *);
Static void		dwc2_device_isoc_done(struct usbd_xfer *);

Static usbd_status	dwc2_device_start(struct usbd_xfer *);

Static void		dwc2_close_pipe(struct usbd_pipe *);
Static void		dwc2_abort_xfer(struct usbd_xfer *, usbd_status);

Static void		dwc2_device_clear_toggle(struct usbd_pipe *);
Static void		dwc2_noop(struct usbd_pipe *pipe);

Static int		dwc2_interrupt(struct dwc2_softc *);
Static void		dwc2_rhc(void *);

Static void		dwc2_timeout(void *);
Static void		dwc2_timeout_task(void *);


static inline void
dwc2_allocate_bus_bandwidth(struct dwc2_hsotg *hsotg, u16 bw,
			    struct usbd_xfer *xfer)
{
}

static inline void
dwc2_free_bus_bandwidth(struct dwc2_hsotg *hsotg, u16 bw,
			struct usbd_xfer *xfer)
{
}

Static const struct usbd_bus_methods dwc2_bus_methods = {
	.ubm_open =	dwc2_open,
	.ubm_softint =	dwc2_softintr,
	.ubm_dopoll =	dwc2_poll,
	.ubm_allocx =	dwc2_allocx,
	.ubm_freex =	dwc2_freex,
	.ubm_getlock =	dwc2_get_lock,
	.ubm_rhctrl =	dwc2_roothub_ctrl,
};

Static const struct usbd_pipe_methods dwc2_root_intr_methods = {
	.upm_transfer =	dwc2_root_intr_transfer,
	.upm_start =	dwc2_root_intr_start,
	.upm_abort =	dwc2_root_intr_abort,
	.upm_close =	dwc2_root_intr_close,
	.upm_cleartoggle =	dwc2_noop,
	.upm_done =	dwc2_root_intr_done,
};

Static const struct usbd_pipe_methods dwc2_device_ctrl_methods = {
	.upm_transfer =	dwc2_device_ctrl_transfer,
	.upm_start =	dwc2_device_ctrl_start,
	.upm_abort =	dwc2_device_ctrl_abort,
	.upm_close =	dwc2_device_ctrl_close,
	.upm_cleartoggle =	dwc2_noop,
	.upm_done =	dwc2_device_ctrl_done,
};

Static const struct usbd_pipe_methods dwc2_device_intr_methods = {
	.upm_transfer =	dwc2_device_intr_transfer,
	.upm_start =	dwc2_device_intr_start,
	.upm_abort =	dwc2_device_intr_abort,
	.upm_close =	dwc2_device_intr_close,
	.upm_cleartoggle =	dwc2_device_clear_toggle,
	.upm_done =	dwc2_device_intr_done,
};

Static const struct usbd_pipe_methods dwc2_device_bulk_methods = {
	.upm_transfer =	dwc2_device_bulk_transfer,
	.upm_abort =	dwc2_device_bulk_abort,
	.upm_close =	dwc2_device_bulk_close,
	.upm_cleartoggle =	dwc2_device_clear_toggle,
	.upm_done =	dwc2_device_bulk_done,
};

Static const struct usbd_pipe_methods dwc2_device_isoc_methods = {
	.upm_transfer =	dwc2_device_isoc_transfer,
	.upm_abort =	dwc2_device_isoc_abort,
	.upm_close =	dwc2_device_isoc_close,
	.upm_cleartoggle =	dwc2_noop,
	.upm_done =	dwc2_device_isoc_done,
};

struct usbd_xfer *
dwc2_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct dwc2_softc *sc = DWC2_BUS2SC(bus);
	struct dwc2_xfer *dxfer;

	DPRINTFN(10, "\n");

	DWC2_EVCNT_INCR(sc->sc_ev_xferpoolget);
	dxfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (dxfer != NULL) {
		memset(dxfer, 0, sizeof(*dxfer));

		dxfer->urb = dwc2_hcd_urb_alloc(sc->sc_hsotg,
		    nframes, GFP_KERNEL);

#ifdef DIAGNOSTIC
		dxfer->xfer.ux_state = XFER_BUSY;
#endif
	}
	return (struct usbd_xfer *)dxfer;
}

void
dwc2_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct dwc2_xfer *dxfer = DWC2_XFER2DXFER(xfer);
	struct dwc2_softc *sc = DWC2_BUS2SC(bus);

	DPRINTFN(10, "\n");

#ifdef DIAGNOSTIC
	if (xfer->ux_state != XFER_BUSY) {
		DPRINTF("xfer=%p not busy, 0x%08x\n", xfer, xfer->ux_state);
	}
	xfer->ux_state = XFER_FREE;
#endif
	DWC2_EVCNT_INCR(sc->sc_ev_xferpoolput);
	dwc2_hcd_urb_free(sc->sc_hsotg, dxfer->urb, dxfer->urb->packet_count);
	pool_cache_put(sc->sc_xferpool, xfer);
}

Static void
dwc2_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct dwc2_softc *sc = DWC2_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

Static void
dwc2_rhc(void *addr)
{
	struct dwc2_softc *sc = addr;
	struct usbd_xfer *xfer;
	u_char *p;

	DPRINTF("\n");
	mutex_enter(&sc->sc_lock);
	xfer = sc->sc_intrxfer;

	if (xfer == NULL) {
		/* Just ignore the change. */
		mutex_exit(&sc->sc_lock);
		return;

	}
	/* set port bit */
	p = KERNADDR(&xfer->ux_dmabuf, 0);

	p[0] = 0x02;	/* we only have one port (1 << 1) */

	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
}

Static void
dwc2_softintr(void *v)
{
	struct usbd_bus *bus = v;
	struct dwc2_softc *sc = DWC2_BUS2SC(bus);
	struct dwc2_hsotg *hsotg = sc->sc_hsotg;
	struct dwc2_xfer *dxfer;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	mutex_spin_enter(&hsotg->lock);
	while ((dxfer = TAILQ_FIRST(&sc->sc_complete)) != NULL) {

		KASSERTMSG(!callout_pending(&dxfer->xfer.ux_callout),
		    "xfer %p pipe %p\n", dxfer, dxfer->xfer.ux_pipe);

		/*
		 * dwc2_abort_xfer will remove this transfer from the
		 * sc_complete queue
		 */
		/*XXXNH not tested */
		if (dxfer->xfer.ux_hcflags & UXFER_ABORTING) {
			cv_broadcast(&dxfer->xfer.ux_hccv);
			continue;
		}

		TAILQ_REMOVE(&sc->sc_complete, dxfer, xnext);

		mutex_spin_exit(&hsotg->lock);
		usb_transfer_complete(&dxfer->xfer);
		mutex_spin_enter(&hsotg->lock);
	}
	mutex_spin_exit(&hsotg->lock);
}

Static void
dwc2_timeout(void *addr)
{
	struct usbd_xfer *xfer = addr;
	struct dwc2_xfer *dxfer = DWC2_XFER2DXFER(xfer);
// 	struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer);
 	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);

	DPRINTF("dxfer=%p\n", dxfer);

	if (sc->sc_dying) {
		mutex_enter(&sc->sc_lock);
		dwc2_abort_xfer(&dxfer->xfer, USBD_TIMEOUT);
		mutex_exit(&sc->sc_lock);
		return;
	}

	/* Execute the abort in a process context. */
	usb_init_task(&dxfer->abort_task, dwc2_timeout_task, addr,
	    USB_TASKQ_MPSAFE);
	usb_add_task(dxfer->xfer.ux_pipe->up_dev, &dxfer->abort_task,
	    USB_TASKQ_HC);
}

Static void
dwc2_timeout_task(void *addr)
{
	struct usbd_xfer *xfer = addr;
 	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);

	DPRINTF("xfer=%p\n", xfer);

	mutex_enter(&sc->sc_lock);
	dwc2_abort_xfer(xfer, USBD_TIMEOUT);
	mutex_exit(&sc->sc_lock);
}

usbd_status
dwc2_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	struct dwc2_softc *sc = DWC2_PIPE2SC(pipe);
	struct dwc2_pipe *dpipe = DWC2_PIPE2DPIPE(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	uint8_t addr = dev->ud_addr;
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	usbd_status err;

	DPRINTF("pipe %p addr %d xfertype %d dir %s\n", pipe, addr, xfertype,
	    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN ? "in" : "out");

	if (sc->sc_dying) {
		return USBD_IOERROR;
	}

	if (addr == dev->ud_bus->ub_rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &dwc2_root_intr_methods;
			break;
		default:
			DPRINTF("bad bEndpointAddress 0x%02x\n",
			    ed->bEndpointAddress);
			return USBD_INVAL;
		}
		DPRINTF("root hub pipe open\n");
		return USBD_NORMAL_COMPLETION;
	}

	switch (xfertype) {
	case UE_CONTROL:
		pipe->up_methods = &dwc2_device_ctrl_methods;
		err = usb_allocmem(&sc->sc_bus, sizeof(usb_device_request_t),
		    0, &dpipe->req_dma);
		if (err)
			return err;
		break;
	case UE_INTERRUPT:
		pipe->up_methods = &dwc2_device_intr_methods;
		break;
	case UE_ISOCHRONOUS:
		pipe->up_serialise = false;
		pipe->up_methods = &dwc2_device_isoc_methods;
		break;
	case UE_BULK:
		pipe->up_serialise = false;
		pipe->up_methods = &dwc2_device_bulk_methods;
		break;
	default:
		DPRINTF("bad xfer type %d\n", xfertype);
		return USBD_INVAL;
	}

	/* QH */
	dpipe->priv = NULL;

	return USBD_NORMAL_COMPLETION;
}

Static void
dwc2_poll(struct usbd_bus *bus)
{
	struct dwc2_softc *sc = DWC2_BUS2SC(bus);
	struct dwc2_hsotg *hsotg = sc->sc_hsotg;

	mutex_spin_enter(&hsotg->lock);
	dwc2_interrupt(sc);
	mutex_spin_exit(&hsotg->lock);
}

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
Static void
dwc2_close_pipe(struct usbd_pipe *pipe)
{
#ifdef DIAGNOSTIC
	struct dwc2_softc *sc = pipe->up_dev->ud_bus->ub_hcpriv;
#endif

	KASSERT(mutex_owned(&sc->sc_lock));
}

/*
 * Abort a device request.
 */
Static void
dwc2_abort_xfer(struct usbd_xfer *xfer, usbd_status status)
{
	struct dwc2_xfer *dxfer = DWC2_XFER2DXFER(xfer);
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	struct dwc2_hsotg *hsotg = sc->sc_hsotg;
	struct dwc2_xfer *d, *tmp;
	bool wake;
	int err;

	DPRINTF("xfer=%p\n", xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (sc->sc_dying) {
		xfer->ux_status = status;
		callout_stop(&xfer->ux_callout);
		usb_transfer_complete(xfer);
		return;
	}

	/*
	 * If an abort is already in progress then just wait for it to
	 * complete and return.
	 */
	if (xfer->ux_hcflags & UXFER_ABORTING) {
		xfer->ux_status = status;
		xfer->ux_hcflags |= UXFER_ABORTWAIT;
		while (xfer->ux_hcflags & UXFER_ABORTING)
			cv_wait(&xfer->ux_hccv, &sc->sc_lock);
		return;
	}

	/*
	 * Step 1: Make the stack ignore it and stop the callout.
	 */
	mutex_spin_enter(&hsotg->lock);
	xfer->ux_hcflags |= UXFER_ABORTING;

	xfer->ux_status = status;	/* make software ignore it */
	callout_stop(&xfer->ux_callout);

	/* XXXNH suboptimal */
	TAILQ_FOREACH_SAFE(d, &sc->sc_complete, xnext, tmp) {
		if (d == dxfer) {
			TAILQ_REMOVE(&sc->sc_complete, dxfer, xnext);
		}
	}

	err = dwc2_hcd_urb_dequeue(hsotg, dxfer->urb);
	if (err) {
		DPRINTF("dwc2_hcd_urb_dequeue failed\n");
	}

	mutex_spin_exit(&hsotg->lock);

	/*
	 * Step 2: Execute callback.
	 */
	wake = xfer->ux_hcflags & UXFER_ABORTWAIT;
	xfer->ux_hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);

	usb_transfer_complete(xfer);
	if (wake) {
		cv_broadcast(&xfer->ux_hccv);
	}
}

Static void
dwc2_noop(struct usbd_pipe *pipe)
{

}

Static void
dwc2_device_clear_toggle(struct usbd_pipe *pipe)
{

	DPRINTF("toggle %d -> 0", pipe->up_endpoint->ue_toggle);
}

/***********************************************************************/

Static int
dwc2_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct dwc2_softc *sc = bus->ub_hcpriv;
	usbd_status err = USBD_IOERROR;
	uint16_t len, value, index;
	int totlen = 0;

	if (sc->sc_dying)
		return -1;

	DPRINTFN(4, "type=0x%02x request=%02x\n",
	    req->bmRequestType, req->bRequest);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(8, "wValue=0x%04x\n", value);

		if (len == 0)
			break;
		switch (value) {
#define sd ((usb_string_descriptor_t *)buf)
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "DWC2 root hub");
			break;
#undef sd
		default:
			/* default from usbroothub */
			return buflen;
		}
		break;

	case C(UR_GET_CONFIG, UT_READ_DEVICE):
	case C(UR_GET_INTERFACE, UT_READ_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_ENDPOINT):
	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		/* default from usbroothub */
		DPRINTFN(4, "returning %d (usbroothub default)", buflen);

		return buflen;

	default:
		/* Hub requests */
		err = dwc2_hcd_hub_control(sc->sc_hsotg,
		    C(req->bRequest, req->bmRequestType), value, index,
		    buf, len);
		if (err) {
			return -1;
		}
		totlen = len;
	}

	return totlen;
}

Static usbd_status
dwc2_root_intr_transfer(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("\n");

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return dwc2_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
dwc2_root_intr_start(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);

	DPRINTF("\n");

	if (sc->sc_dying)
		return USBD_IOERROR;

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Abort a root interrupt request. */
Static void
dwc2_root_intr_abort(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);

	DPRINTF("xfer=%p\n", xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

Static void
dwc2_root_intr_close(struct usbd_pipe *pipe)
{
	struct dwc2_softc *sc = DWC2_PIPE2SC(pipe);

	DPRINTF("\n");

	KASSERT(mutex_owned(&sc->sc_lock));

	sc->sc_intrxfer = NULL;
}

Static void
dwc2_root_intr_done(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);

	KASSERT(sc->sc_intrxfer != NULL);
	sc->sc_intrxfer = NULL;
	DPRINTF("\n");
}

/***********************************************************************/

Static usbd_status
dwc2_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("\n");

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return dwc2_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
dwc2_device_ctrl_start(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("\n");

	mutex_enter(&sc->sc_lock);
	xfer->ux_status = USBD_IN_PROGRESS;
	err = dwc2_device_start(xfer);
	mutex_exit(&sc->sc_lock);

	if (err)
		return err;

	return USBD_IN_PROGRESS;
}

Static void
dwc2_device_ctrl_abort(struct usbd_xfer *xfer)
{
#ifdef DIAGNOSTIC
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
#endif
	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%p\n", xfer);
	dwc2_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc2_device_ctrl_close(struct usbd_pipe *pipe)
{

	DPRINTF("pipe=%p\n", pipe);
	dwc2_close_pipe(pipe);
}

Static void
dwc2_device_ctrl_done(struct usbd_xfer *xfer)
{

	DPRINTF("xfer=%p\n", xfer);
}

/***********************************************************************/

Static usbd_status
dwc2_device_bulk_transfer(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("xfer=%p\n", xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);

	KASSERT(err == USBD_NORMAL_COMPLETION);

	xfer->ux_status = USBD_IN_PROGRESS;
	err = dwc2_device_start(xfer);
	mutex_exit(&sc->sc_lock);

	return err;
}

Static void
dwc2_device_bulk_abort(struct usbd_xfer *xfer)
{
#ifdef DIAGNOSTIC
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
#endif
	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%p\n", xfer);
	dwc2_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc2_device_bulk_close(struct usbd_pipe *pipe)
{

	DPRINTF("pipe=%p\n", pipe);

	dwc2_close_pipe(pipe);
}

Static void
dwc2_device_bulk_done(struct usbd_xfer *xfer)
{

	DPRINTF("xfer=%p\n", xfer);
}

/***********************************************************************/

Static usbd_status
dwc2_device_intr_transfer(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("xfer=%p\n", xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return dwc2_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
dwc2_device_intr_start(struct usbd_xfer *xfer)
{
	struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer)
	struct usbd_device *dev = dpipe->pipe.up_dev;
	struct dwc2_softc *sc = dev->ud_bus->ub_hcpriv;
	usbd_status err;

	mutex_enter(&sc->sc_lock);
	xfer->ux_status = USBD_IN_PROGRESS;
	err = dwc2_device_start(xfer);
	mutex_exit(&sc->sc_lock);

	if (err)
		return err;

	return USBD_IN_PROGRESS;
}

/* Abort a device interrupt request. */
Static void
dwc2_device_intr_abort(struct usbd_xfer *xfer)
{
#ifdef DIAGNOSTIC
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
#endif

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	DPRINTF("xfer=%p\n", xfer);

	dwc2_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc2_device_intr_close(struct usbd_pipe *pipe)
{

	DPRINTF("pipe=%p\n", pipe);

	dwc2_close_pipe(pipe);
}

Static void
dwc2_device_intr_done(struct usbd_xfer *xfer)
{

	DPRINTF("\n");
}

/***********************************************************************/

usbd_status
dwc2_device_isoc_transfer(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	usbd_status err;

	DPRINTF("xfer=%p\n", xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);

	KASSERT(err == USBD_NORMAL_COMPLETION);

	xfer->ux_status = USBD_IN_PROGRESS;
	err = dwc2_device_start(xfer);
	mutex_exit(&sc->sc_lock);

	return err;
}

void
dwc2_device_isoc_abort(struct usbd_xfer *xfer)
{
	struct dwc2_softc *sc __diagused = DWC2_XFER2SC(xfer);
	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%p\n", xfer);
	dwc2_abort_xfer(xfer, USBD_CANCELLED);
}

void
dwc2_device_isoc_close(struct usbd_pipe *pipe)
{
	DPRINTF("\n");

	dwc2_close_pipe(pipe);
}

void
dwc2_device_isoc_done(struct usbd_xfer *xfer)
{

	DPRINTF("\n");
}


usbd_status
dwc2_device_start(struct usbd_xfer *xfer)
{
 	struct dwc2_xfer *dxfer = DWC2_XFER2DXFER(xfer);
	struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer);
	struct dwc2_softc *sc = DWC2_XFER2SC(xfer);
	struct dwc2_hsotg *hsotg = sc->sc_hsotg;
	struct dwc2_hcd_urb *dwc2_urb;

	struct usbd_device *dev = xfer->ux_pipe->up_dev;
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	uint8_t addr = dev->ud_addr;
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	uint8_t epnum = UE_GET_ADDR(ed->bEndpointAddress);
	uint8_t dir = UE_GET_DIR(ed->bEndpointAddress);
	uint16_t mps = UE_GET_SIZE(UGETW(ed->wMaxPacketSize));
	uint32_t len;

	uint32_t flags = 0;
	uint32_t off = 0;
	int retval, err;
	int alloc_bandwidth = 0;
	int i;

	DPRINTFN(1, "xfer=%p pipe=%p\n", xfer, xfer->ux_pipe);

	if (xfertype == UE_ISOCHRONOUS ||
	    xfertype == UE_INTERRUPT) {
		mutex_spin_enter(&hsotg->lock);
		if (!dwc2_hcd_is_bandwidth_allocated(hsotg, xfer))
			alloc_bandwidth = 1;
		mutex_spin_exit(&hsotg->lock);
	}

	/*
	 * For Control pipe the direction is from the request, all other
	 * transfers have been set correctly at pipe open time.
	 */
	if (xfertype == UE_CONTROL) {
		usb_device_request_t *req = &xfer->ux_request;

		DPRINTFN(3, "xfer=%p type=0x%02x request=0x%02x wValue=0x%04x "
		    "wIndex=0x%04x len=%d addr=%d endpt=%d dir=%s speed=%d "
		    "mps=%d\n",
		    xfer, req->bmRequestType, req->bRequest, UGETW(req->wValue),
		    UGETW(req->wIndex), UGETW(req->wLength), dev->ud_addr,
		    epnum, dir == UT_READ ? "in" :"out", dev->ud_speed, mps);

		/* Copy request packet to our DMA buffer */
		memcpy(KERNADDR(&dpipe->req_dma, 0), req, sizeof(*req));
		usb_syncmem(&dpipe->req_dma, 0, sizeof(*req),
		    BUS_DMASYNC_PREWRITE);
		len = UGETW(req->wLength);
		if ((req->bmRequestType & UT_READ) == UT_READ) {
			dir = UE_DIR_IN;
		} else {
			dir = UE_DIR_OUT;
		}

		DPRINTFN(3, "req = %p dma = %" PRIxBUSADDR " len %d dir %s\n",
		    KERNADDR(&dpipe->req_dma, 0), DMAADDR(&dpipe->req_dma, 0),
		    len, dir == UE_DIR_IN ? "in" : "out");
	} else {
		DPRINTFN(3, "xfer=%p len=%d flags=%d addr=%d endpt=%d,"
		    " mps=%d dir %s\n", xfer, xfer->ux_length, xfer->ux_flags, addr,
		    epnum, mps, dir == UT_READ ? "in" :"out");

		len = xfer->ux_length;
	}

	dwc2_urb = dxfer->urb;
	if (!dwc2_urb)
		return USBD_NOMEM;

	KASSERT(dwc2_urb->packet_count == xfer->ux_nframes);
	memset(dwc2_urb, 0, sizeof(*dwc2_urb) +
	    sizeof(dwc2_urb->iso_descs[0]) * dwc2_urb->packet_count);

	dwc2_urb->priv = xfer;
	dwc2_urb->packet_count = xfer->ux_nframes;

	dwc2_hcd_urb_set_pipeinfo(hsotg, dwc2_urb, addr, epnum, xfertype, dir,
	    mps);

	if (xfertype == UE_CONTROL) {
		dwc2_urb->setup_usbdma = &dpipe->req_dma;
		dwc2_urb->setup_packet = KERNADDR(&dpipe->req_dma, 0);
		dwc2_urb->setup_dma = DMAADDR(&dpipe->req_dma, 0);
	} else {
		/* XXXNH - % mps required? */
		if ((xfer->ux_flags & USBD_FORCE_SHORT_XFER) && (len % mps) == 0)
		    flags |= URB_SEND_ZERO_PACKET;
	}
	flags |= URB_GIVEBACK_ASAP;

	/*
	 * control transfers with no data phase don't touch usbdma, but
	 * everything else does.
	 */
	if (!(xfertype == UE_CONTROL && len == 0)) {
		dwc2_urb->usbdma = &xfer->ux_dmabuf;
		dwc2_urb->buf = KERNADDR(dwc2_urb->usbdma, 0);
		dwc2_urb->dma = DMAADDR(dwc2_urb->usbdma, 0);

		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    dir == UE_DIR_IN ?
			BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
 	}
	dwc2_urb->length = len;
 	dwc2_urb->flags = flags;
	dwc2_urb->status = -EINPROGRESS;

	if (xfertype == UE_INTERRUPT ||
	    xfertype == UE_ISOCHRONOUS) {
		uint16_t ival;

		if (xfertype == UE_INTERRUPT &&
		    dpipe->pipe.up_interval != USBD_DEFAULT_INTERVAL) {
			ival = dpipe->pipe.up_interval;
		} else {
			ival = ed->bInterval;
		}

		if (ival < 1) {
			retval = -ENODEV;
			goto fail;
		}
		if (dev->ud_speed == USB_SPEED_HIGH ||
		   (dev->ud_speed == USB_SPEED_FULL && xfertype == UE_ISOCHRONOUS)) {
			if (ival > 16) {
				/*
				 * illegal with HS/FS, but there were
				 * documentation bugs in the spec
				 */
				ival = 256;
			} else {
				ival = (1 << (ival - 1));
			}
		} else {
			if (xfertype == UE_INTERRUPT && ival < 10)
				ival = 10;
		}
		dwc2_urb->interval = ival;
	}

	/* XXXNH bring down from callers?? */
// 	mutex_enter(&sc->sc_lock);

	xfer->ux_actlen = 0;

	KASSERT(xfertype != UE_ISOCHRONOUS ||
	    xfer->ux_nframes <= dwc2_urb->packet_count);
	KASSERTMSG(xfer->ux_nframes == 0 || xfertype == UE_ISOCHRONOUS,
	    "nframes %d xfertype %d\n", xfer->ux_nframes, xfertype);

	for (off = i = 0; i < xfer->ux_nframes; ++i) {
		DPRINTFN(3, "xfer=%p frame=%d offset=%d length=%d\n", xfer, i,
		    off, xfer->ux_frlengths[i]);

		dwc2_hcd_urb_set_iso_desc_params(dwc2_urb, i, off,
		    xfer->ux_frlengths[i]);
		off += xfer->ux_frlengths[i];
	}

	struct dwc2_qh *qh = dpipe->priv;
	struct dwc2_qtd *qtd;
	bool qh_allocated = false;

	/* Create QH for the endpoint if it doesn't exist */
	if (!qh) {
		qh = dwc2_hcd_qh_create(hsotg, dwc2_urb, GFP_ATOMIC);
		if (!qh) {
			retval = -ENOMEM;
			goto fail;
		}
		dpipe->priv = qh;
		qh_allocated = true;
	}

	qtd = pool_cache_get(sc->sc_qtdpool, PR_NOWAIT);
	if (!qtd) {
		retval = -ENOMEM;
		goto fail1;
	}
	memset(qtd, 0, sizeof(*qtd));

	/* might need to check cpu_intr_p */
	mutex_spin_enter(&hsotg->lock);

	if (xfer->ux_timeout && !sc->sc_bus.ub_usepolling) {
		callout_reset(&xfer->ux_callout, mstohz(xfer->ux_timeout),
		    dwc2_timeout, xfer);
	}
	retval = dwc2_hcd_urb_enqueue(hsotg, dwc2_urb, qh, qtd);
	if (retval)
		goto fail2;

	if (alloc_bandwidth) {
		dwc2_allocate_bus_bandwidth(hsotg,
				dwc2_hcd_get_ep_bandwidth(hsotg, dpipe),
				xfer);
	}

	mutex_spin_exit(&hsotg->lock);
// 	mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;

fail2:
	callout_stop(&xfer->ux_callout);
	dwc2_urb->priv = NULL;
	mutex_spin_exit(&hsotg->lock);
	pool_cache_put(sc->sc_qtdpool, qtd);

fail1:
	if (qh_allocated) {
		dpipe->priv = NULL;
		dwc2_hcd_qh_free(hsotg, qh);
	}
fail:

	switch (retval) {
	case -EINVAL:
	case -ENODEV:
		err = USBD_INVAL;
		break;
	case -ENOMEM:
		err = USBD_NOMEM;
		break;
	default:
		err = USBD_IOERROR;
	}

	return err;

}

int dwc2_intr(void *p)
{
	struct dwc2_softc *sc = p;
	struct dwc2_hsotg *hsotg;
	int ret = 0;

	if (sc == NULL)
		return 0;

	hsotg = sc->sc_hsotg;
	mutex_spin_enter(&hsotg->lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	if (sc->sc_bus.ub_usepolling) {
		uint32_t intrs;

		intrs = dwc2_read_core_intr(hsotg);
		DWC2_WRITE_4(hsotg, GINTSTS, intrs);
	} else {
		ret = dwc2_interrupt(sc);
	}

done:
	mutex_spin_exit(&hsotg->lock);

	return ret;
}

int
dwc2_interrupt(struct dwc2_softc *sc)
{
	int ret = 0;

	if (sc->sc_hcdenabled) {
		ret |= dwc2_handle_hcd_intr(sc->sc_hsotg);
	}

	ret |= dwc2_handle_common_intr(sc->sc_hsotg);

	return ret;
}

/***********************************************************************/

int
dwc2_detach(struct dwc2_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	return rv;
}

bool
dwc2_shutdown(device_t self, int flags)
{
	struct dwc2_softc *sc = device_private(self);

	sc = sc;

	return true;
}

void
dwc2_childdet(device_t self, device_t child)
{
	struct dwc2_softc *sc = device_private(self);

	sc = sc;
}

int
dwc2_activate(device_t self, enum devact act)
{
	struct dwc2_softc *sc = device_private(self);

	sc = sc;

	return 0;
}

bool
dwc2_resume(device_t dv, const pmf_qual_t *qual)
{
	struct dwc2_softc *sc = device_private(dv);

	sc = sc;

	return true;
}

bool
dwc2_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct dwc2_softc *sc = device_private(dv);

	sc = sc;

	return true;
}

/***********************************************************************/
int
dwc2_init(struct dwc2_softc *sc)
{
	int err = 0;

	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_methods = &dwc2_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct dwc2_pipe);
	sc->sc_bus.ub_usedma = true;
	sc->sc_hcdenabled = false;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);

	TAILQ_INIT(&sc->sc_complete);

	sc->sc_rhc_si = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    dwc2_rhc, sc);

	sc->sc_xferpool = pool_cache_init(sizeof(struct dwc2_xfer), 0, 0, 0,
	    "dwc2xfer", NULL, IPL_USB, NULL, NULL, NULL);
	sc->sc_qhpool = pool_cache_init(sizeof(struct dwc2_qh), 0, 0, 0,
	    "dwc2qh", NULL, IPL_USB, NULL, NULL, NULL);
	sc->sc_qtdpool = pool_cache_init(sizeof(struct dwc2_qtd), 0, 0, 0,
	    "dwc2qtd", NULL, IPL_USB, NULL, NULL, NULL);

	sc->sc_hsotg = kmem_zalloc(sizeof(struct dwc2_hsotg), KM_SLEEP);
	sc->sc_hsotg->hsotg_sc = sc;
	sc->sc_hsotg->dev = sc->sc_dev;
	sc->sc_hcdenabled = true;

	struct dwc2_hsotg *hsotg = sc->sc_hsotg;
	struct dwc2_core_params defparams;
	int retval;

	if (sc->sc_params == NULL) {
		/* Default all params to autodetect */
		dwc2_set_all_params(&defparams, -1);
		sc->sc_params = &defparams;

		/*
		 * Disable descriptor dma mode by default as the HW can support
		 * it, but does not support it for SPLIT transactions.
		 */
		defparams.dma_desc_enable = 0;
	}
	hsotg->dr_mode = USB_DR_MODE_HOST;

	/* Detect config values from hardware */
	retval = dwc2_get_hwparams(hsotg);
	if (retval) {
		goto fail2;
	}

	hsotg->core_params = kmem_zalloc(sizeof(*hsotg->core_params), KM_SLEEP);
	dwc2_set_all_params(hsotg->core_params, -1);

	/* Validate parameter values */
	dwc2_set_parameters(hsotg, sc->sc_params);

#if IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || \
    IS_ENABLED(CONFIG_USB_DWC2_DUAL_ROLE)
	if (hsotg->dr_mode != USB_DR_MODE_HOST) {
		retval = dwc2_gadget_init(hsotg);
		if (retval)
			goto fail2;
		hsotg->gadget_enabled = 1;
	}
#endif
#if IS_ENABLED(CONFIG_USB_DWC2_HOST) || \
    IS_ENABLED(CONFIG_USB_DWC2_DUAL_ROLE)
	if (hsotg->dr_mode != USB_DR_MODE_PERIPHERAL) {
		retval = dwc2_hcd_init(hsotg);
		if (retval) {
			if (hsotg->gadget_enabled)
				dwc2_hsotg_remove(hsotg);
			goto fail2;
		}
	    hsotg->hcd_enabled = 1;
        }
#endif

	uint32_t snpsid = hsotg->hw_params.snpsid;
	aprint_verbose_dev(sc->sc_dev, "Core Release: %x.%x%x%x (snpsid=%x)\n",
	    snpsid >> 12 & 0xf, snpsid >> 8 & 0xf,
	    snpsid >> 4 & 0xf, snpsid & 0xf, snpsid);

	return 0;

fail2:
	err = -retval;
	kmem_free(sc->sc_hsotg, sizeof(struct dwc2_hsotg));
	softint_disestablish(sc->sc_rhc_si);

	return err;
}

#if 0
/*
 * curmode is a mode indication bit 0 = device, 1 = host
 */
static const char * const intnames[32] = {
	"curmode",	"modemis",	"otgint",	"sof",
	"rxflvl",	"nptxfemp",	"ginnakeff",	"goutnakeff",
	"ulpickint",	"i2cint",	"erlysusp",	"usbsusp",
	"usbrst",	"enumdone",	"isooutdrop",	"eopf",
	"restore_done",	"epmis",	"iepint",	"oepint",
	"incompisoin",	"incomplp",	"fetsusp",	"resetdet",
	"prtint",	"hchint",	"ptxfemp",	"lpm",
	"conidstschng",	"disconnint",	"sessreqint",	"wkupint"
};


/***********************************************************************/

#endif

void dwc2_host_hub_info(struct dwc2_hsotg *hsotg, void *context, int *hub_addr,
			int *hub_port)
{
	struct usbd_xfer *xfer = context;
	struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer);
	struct usbd_device *dev = dpipe->pipe.up_dev;

	*hub_addr = dev->ud_myhsport->up_parent->ud_addr;
 	*hub_port = dev->ud_myhsport->up_portno;
}

int dwc2_host_get_speed(struct dwc2_hsotg *hsotg, void *context)
{
	struct usbd_xfer *xfer = context;
	struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer);
	struct usbd_device *dev = dpipe->pipe.up_dev;

	return dev->ud_speed;
}

/*
 * Sets the final status of an URB and returns it to the upper layer. Any
 * required cleanup of the URB is performed.
 *
 * Must be called with interrupt disabled and spinlock held
 */
void dwc2_host_complete(struct dwc2_hsotg *hsotg, struct dwc2_qtd *qtd,
    int status)
{
	struct usbd_xfer *xfer;
	struct dwc2_xfer *dxfer;
	struct dwc2_softc *sc;
	usb_endpoint_descriptor_t *ed;
	uint8_t xfertype;

	if (!qtd) {
		dev_dbg(hsotg->dev, "## %s: qtd is NULL ##\n", __func__);
		return;
	}

	if (!qtd->urb) {
		dev_dbg(hsotg->dev, "## %s: qtd->urb is NULL ##\n", __func__);
		return;
	}

	xfer = qtd->urb->priv;
	if (!xfer) {
		dev_dbg(hsotg->dev, "## %s: urb->priv is NULL ##\n", __func__);
		return;
	}

	dxfer = DWC2_XFER2DXFER(xfer);
	sc = DWC2_XFER2SC(xfer);
	ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	xfertype = UE_GET_XFERTYPE(ed->bmAttributes);

	struct dwc2_hcd_urb *urb = qtd->urb;
	xfer->ux_actlen = dwc2_hcd_urb_get_actual_length(urb);

	DPRINTFN(3, "xfer=%p actlen=%d\n", xfer, xfer->ux_actlen);

	if (xfertype == UE_ISOCHRONOUS) {
		int i;

		xfer->ux_actlen = 0;
		for (i = 0; i < xfer->ux_nframes; ++i) {
			xfer->ux_frlengths[i] =
				dwc2_hcd_urb_get_iso_desc_actual_length(
						urb, i);
			xfer->ux_actlen += xfer->ux_frlengths[i];
		}
	}

	if (xfertype == UE_ISOCHRONOUS && dbg_perio()) {
		int i;

		for (i = 0; i < xfer->ux_nframes; i++)
			dev_vdbg(hsotg->dev, " ISO Desc %d status %d\n",
				 i, urb->iso_descs[i].status);
	}

	if (!status) {
		if (!(xfer->ux_flags & USBD_SHORT_XFER_OK) &&
		    xfer->ux_actlen < xfer->ux_length)
			status = -EIO;
	}

	switch (status) {
	case 0:
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		break;
	case -EPIPE:
		xfer->ux_status = USBD_STALLED;
		break;
	case -ETIMEDOUT:
		xfer->ux_status = USBD_TIMEOUT;
		break;
	case -EPROTO:
		xfer->ux_status = USBD_INVAL;
		break;
	case -EIO:
		xfer->ux_status = USBD_IOERROR;
		break;
	case -EOVERFLOW:
		xfer->ux_status = USBD_IOERROR;
		break;
	default:
		xfer->ux_status = USBD_IOERROR;
		printf("%s: unknown error status %d\n", __func__, status);
	}

	if (xfer->ux_status == USBD_NORMAL_COMPLETION) {
		/*
		 * control transfers with no data phase don't touch dmabuf, but
		 * everything else does.
		 */
		if (!(xfertype == UE_CONTROL &&
		    UGETW(xfer->ux_request.wLength) == 0) &&
		    xfer->ux_actlen > 0	/* XXX PR/53503 */
		    ) {
			int rd = usbd_xfer_isread(xfer);

			usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_actlen,
			    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		}
	}

	if (xfertype == UE_ISOCHRONOUS ||
	    xfertype == UE_INTERRUPT) {
		struct dwc2_pipe *dpipe = DWC2_XFER2DPIPE(xfer);

		dwc2_free_bus_bandwidth(hsotg,
					dwc2_hcd_get_ep_bandwidth(hsotg, dpipe),
					xfer);
	}

	qtd->urb = NULL;
	callout_stop(&xfer->ux_callout);

	KASSERT(mutex_owned(&hsotg->lock));

	TAILQ_INSERT_TAIL(&sc->sc_complete, dxfer, xnext);

	mutex_spin_exit(&hsotg->lock);
	usb_schedsoftintr(&sc->sc_bus);
	mutex_spin_enter(&hsotg->lock);
}


int
_dwc2_hcd_start(struct dwc2_hsotg *hsotg)
{
	dev_dbg(hsotg->dev, "DWC OTG HCD START\n");

	mutex_spin_enter(&hsotg->lock);

	hsotg->lx_state = DWC2_L0;

	if (dwc2_is_device_mode(hsotg)) {
		mutex_spin_exit(&hsotg->lock);
		return 0;	/* why 0 ?? */
	}

	dwc2_hcd_reinit(hsotg);

	mutex_spin_exit(&hsotg->lock);
	return 0;
}

int dwc2_host_is_b_hnp_enabled(struct dwc2_hsotg *hsotg)
{

	return false;
}
