/*	$NetBSD: usbdi.c,v 1.227 2022/03/03 06:08:50 riastradh Exp $	*/

/*
 * Copyright (c) 1998, 2012, 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, Matthew R. Green (mrg@eterna.com.au),
 * and Nick Hudson.
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
__KERNEL_RCSID(0, "$NetBSD: usbdi.c,v 1.227 2022/03/03 06:08:50 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_compat_netbsd.h"
#include "usb_dma.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usb_sdt.h>
#include <dev/usb/usbhist.h>

/* UTF-8 encoding stuff */
#include <fs/unicode.h>

SDT_PROBE_DEFINE5(usb, device, pipe, open,
    "struct usbd_interface *"/*iface*/,
    "uint8_t"/*address*/,
    "uint8_t"/*flags*/,
    "int"/*ival*/,
    "struct usbd_pipe *"/*pipe*/);

SDT_PROBE_DEFINE7(usb, device, pipe, open__intr,
    "struct usbd_interface *"/*iface*/,
    "uint8_t"/*address*/,
    "uint8_t"/*flags*/,
    "int"/*ival*/,
    "usbd_callback"/*cb*/,
    "void *"/*cookie*/,
    "struct usbd_pipe *"/*pipe*/);

SDT_PROBE_DEFINE2(usb, device, pipe, transfer__start,
    "struct usbd_pipe *"/*pipe*/,
    "struct usbd_xfer *"/*xfer*/);
SDT_PROBE_DEFINE3(usb, device, pipe, transfer__done,
    "struct usbd_pipe *"/*pipe*/,
    "struct usbd_xfer *"/*xfer*/,
    "usbd_status"/*err*/);
SDT_PROBE_DEFINE2(usb, device, pipe, start,
    "struct usbd_pipe *"/*pipe*/,
    "struct usbd_xfer *"/*xfer*/);

SDT_PROBE_DEFINE1(usb, device, pipe, close,  "struct usbd_pipe *"/*pipe*/);
SDT_PROBE_DEFINE1(usb, device, pipe, abort__start,
    "struct usbd_pipe *"/*pipe*/);
SDT_PROBE_DEFINE1(usb, device, pipe, abort__done,
    "struct usbd_pipe *"/*pipe*/);
SDT_PROBE_DEFINE1(usb, device, pipe, clear__endpoint__stall,
    "struct usbd_pipe *"/*pipe*/);
SDT_PROBE_DEFINE1(usb, device, pipe, clear__endpoint__toggle,
    "struct usbd_pipe *"/*pipe*/);

SDT_PROBE_DEFINE5(usb, device, xfer, create,
    "struct usbd_xfer *"/*xfer*/,
    "struct usbd_pipe *"/*pipe*/,
    "size_t"/*len*/,
    "unsigned int"/*flags*/,
    "unsigned int"/*nframes*/);
SDT_PROBE_DEFINE1(usb, device, xfer, start,  "struct usbd_xfer *"/*xfer*/);
SDT_PROBE_DEFINE1(usb, device, xfer, preabort,  "struct usbd_xfer *"/*xfer*/);
SDT_PROBE_DEFINE1(usb, device, xfer, abort,  "struct usbd_xfer *"/*xfer*/);
SDT_PROBE_DEFINE1(usb, device, xfer, timeout,  "struct usbd_xfer *"/*xfer*/);
SDT_PROBE_DEFINE2(usb, device, xfer, done,
    "struct usbd_xfer *"/*xfer*/,
    "usbd_status"/*status*/);
SDT_PROBE_DEFINE1(usb, device, xfer, destroy,  "struct usbd_xfer *"/*xfer*/);

Static void usbd_ar_pipe(struct usbd_pipe *);
static usbd_status usb_insert_transfer(struct usbd_xfer *);
Static void usbd_start_next(struct usbd_pipe *);
Static usbd_status usbd_open_pipe_ival
	(struct usbd_interface *, uint8_t, uint8_t, struct usbd_pipe **, int);
static void *usbd_alloc_buffer(struct usbd_xfer *, uint32_t);
static void usbd_free_buffer(struct usbd_xfer *);
static struct usbd_xfer *usbd_alloc_xfer(struct usbd_device *, unsigned int);
static void usbd_free_xfer(struct usbd_xfer *);
static void usbd_request_async_cb(struct usbd_xfer *, void *, usbd_status);
static void usbd_xfer_timeout(void *);
static void usbd_xfer_timeout_task(void *);
static bool usbd_xfer_probe_timeout(struct usbd_xfer *);
static void usbd_xfer_cancel_timeout_async(struct usbd_xfer *);

#if defined(USB_DEBUG)
void
usbd_dump_iface(struct usbd_interface *iface)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface %#jx", (uintptr_t)iface, 0, 0, 0);

	if (iface == NULL)
		return;
	USBHIST_LOG(usbdebug, "     device = %#jx idesc = %#jx index = %jd",
	    (uintptr_t)iface->ui_dev, (uintptr_t)iface->ui_idesc,
	    iface->ui_index, 0);
	USBHIST_LOG(usbdebug, "     altindex=%jd",
	    iface->ui_altindex, 0, 0, 0);
}

void
usbd_dump_device(struct usbd_device *dev)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev = %#jx", (uintptr_t)dev, 0, 0, 0);

	if (dev == NULL)
		return;
	USBHIST_LOG(usbdebug, "     bus = %#jx default_pipe = %#jx",
	    (uintptr_t)dev->ud_bus, (uintptr_t)dev->ud_pipe0, 0, 0);
	USBHIST_LOG(usbdebug, "     address = %jd config = %jd depth = %jd ",
	    dev->ud_addr, dev->ud_config, dev->ud_depth, 0);
	USBHIST_LOG(usbdebug, "     speed = %jd self_powered = %jd "
	    "power = %jd langid = %jd",
	    dev->ud_speed, dev->ud_selfpowered, dev->ud_power, dev->ud_langid);
}

void
usbd_dump_endpoint(struct usbd_endpoint *endp)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "endp = %#jx", (uintptr_t)endp, 0, 0, 0);

	if (endp == NULL)
		return;
	USBHIST_LOG(usbdebug, "    edesc = %#jx refcnt = %jd",
	    (uintptr_t)endp->ue_edesc, endp->ue_refcnt, 0, 0);
	if (endp->ue_edesc)
		USBHIST_LOG(usbdebug, "     bEndpointAddress=0x%02jx",
		    endp->ue_edesc->bEndpointAddress, 0, 0, 0);
}

void
usbd_dump_queue(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "pipe = %#jx", (uintptr_t)pipe, 0, 0, 0);

	SIMPLEQ_FOREACH(xfer, &pipe->up_queue, ux_next) {
		USBHIST_LOG(usbdebug, "     xfer = %#jx", (uintptr_t)xfer,
		    0, 0, 0);
	}
}

void
usbd_dump_pipe(struct usbd_pipe *pipe)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "pipe = %#jx", (uintptr_t)pipe, 0, 0, 0);

	if (pipe == NULL)
		return;
	usbd_dump_iface(pipe->up_iface);
	usbd_dump_device(pipe->up_dev);
	usbd_dump_endpoint(pipe->up_endpoint);
	USBHIST_LOG(usbdebug, "(usbd_dump_pipe)", 0, 0, 0, 0);
	USBHIST_LOG(usbdebug, "     running = %jd aborting = %jd",
	    pipe->up_running, pipe->up_aborting, 0, 0);
	USBHIST_LOG(usbdebug, "     intrxfer = %#jx, repeat = %jd, "
	    "interval = %jd", (uintptr_t)pipe->up_intrxfer, pipe->up_repeat,
	    pipe->up_interval, 0);
}
#endif

usbd_status
usbd_open_pipe(struct usbd_interface *iface, uint8_t address,
	       uint8_t flags, struct usbd_pipe **pipe)
{
	return (usbd_open_pipe_ival(iface, address, flags, pipe,
				    USBD_DEFAULT_INTERVAL));
}

usbd_status
usbd_open_pipe_ival(struct usbd_interface *iface, uint8_t address,
		    uint8_t flags, struct usbd_pipe **pipe, int ival)
{
	struct usbd_pipe *p = NULL;
	struct usbd_endpoint *ep = NULL /* XXXGCC */;
	bool piperef = false;
	usbd_status err;
	int i;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface = %#jx address = %#jx flags = %#jx",
	    (uintptr_t)iface, address, flags, 0);

	/*
	 * Block usbd_set_interface so we have a snapshot of the
	 * interface endpoints.  They will remain stable until we drop
	 * the reference in usbd_close_pipe (or on failure here).
	 */
	err = usbd_iface_piperef(iface);
	if (err)
		goto out;
	piperef = true;

	/* Find the endpoint at this address.  */
	for (i = 0; i < iface->ui_idesc->bNumEndpoints; i++) {
		ep = &iface->ui_endpoints[i];
		if (ep->ue_edesc == NULL) {
			err = USBD_IOERROR;
			goto out;
		}
		if (ep->ue_edesc->bEndpointAddress == address)
			break;
	}
	if (i == iface->ui_idesc->bNumEndpoints) {
		err = USBD_BAD_ADDRESS;
		goto out;
	}

	/* Set up the pipe with this endpoint.  */
	err = usbd_setup_pipe_flags(iface->ui_dev, iface, ep, ival, &p, flags);
	if (err)
		goto out;

	/* Success! */
	*pipe = p;
	p = NULL;		/* handed off to caller */
	piperef = false;	/* handed off to pipe */
	SDT_PROBE5(usb, device, pipe, open,
	    iface, address, flags, ival, p);
	err = USBD_NORMAL_COMPLETION;

out:	if (p)
		usbd_close_pipe(p);
	if (piperef)
		usbd_iface_pipeunref(iface);
	return err;
}

usbd_status
usbd_open_pipe_intr(struct usbd_interface *iface, uint8_t address,
		    uint8_t flags, struct usbd_pipe **pipe,
		    void *priv, void *buffer, uint32_t len,
		    usbd_callback cb, int ival)
{
	usbd_status err;
	struct usbd_xfer *xfer;
	struct usbd_pipe *ipipe;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "address = %#jx flags = %#jx len = %jd",
	    address, flags, len, 0);

	err = usbd_open_pipe_ival(iface, address,
				  USBD_EXCLUSIVE_USE | (flags & USBD_MPSAFE),
				  &ipipe, ival);
	if (err)
		return err;
	err = usbd_create_xfer(ipipe, len, flags, 0, &xfer);
	if (err)
		goto bad1;

	usbd_setup_xfer(xfer, priv, buffer, len, flags, USBD_NO_TIMEOUT, cb);
	ipipe->up_intrxfer = xfer;
	ipipe->up_repeat = 1;
	err = usbd_transfer(xfer);
	*pipe = ipipe;
	if (err != USBD_IN_PROGRESS)
		goto bad3;
	SDT_PROBE7(usb, device, pipe, open__intr,
	    iface, address, flags, ival, cb, priv, ipipe);
	return USBD_NORMAL_COMPLETION;

 bad3:
	ipipe->up_intrxfer = NULL;
	ipipe->up_repeat = 0;

	usbd_destroy_xfer(xfer);
 bad1:
	usbd_close_pipe(ipipe);
	return err;
}

void
usbd_close_pipe(struct usbd_pipe *pipe)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(pipe != NULL);

	usbd_lock_pipe(pipe);
	SDT_PROBE1(usb, device, pipe, close,  pipe);
	if (!SIMPLEQ_EMPTY(&pipe->up_queue)) {
		printf("WARNING: pipe closed with active xfers on addr %d\n",
		    pipe->up_dev->ud_addr);
		usbd_ar_pipe(pipe);
	}
	KASSERT(SIMPLEQ_EMPTY(&pipe->up_queue));
	pipe->up_methods->upm_close(pipe);
	usbd_unlock_pipe(pipe);

	cv_destroy(&pipe->up_callingcv);
	if (pipe->up_intrxfer)
		usbd_destroy_xfer(pipe->up_intrxfer);
	usb_rem_task_wait(pipe->up_dev, &pipe->up_async_task, USB_TASKQ_DRIVER,
	    NULL);
	usbd_endpoint_release(pipe->up_dev, pipe->up_endpoint);
	if (pipe->up_iface)
		usbd_iface_pipeunref(pipe->up_iface);
	kmem_free(pipe, pipe->up_dev->ud_bus->ub_pipesize);
}

usbd_status
usbd_transfer(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usbd_status err;
	unsigned int size, flags;

	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug,
	    "xfer = %#jx, flags = %#jx, pipe = %#jx, running = %jd",
	    (uintptr_t)xfer, xfer->ux_flags, (uintptr_t)pipe, pipe->up_running);
	KASSERT(xfer->ux_status == USBD_NOT_STARTED);
	SDT_PROBE1(usb, device, xfer, start,  xfer);

#ifdef USB_DEBUG
	if (usbdebug > 5)
		usbd_dump_queue(pipe);
#endif
	xfer->ux_done = 0;

	if (pipe->up_aborting) {
		USBHIST_LOG(usbdebug, "<- done xfer %#jx, aborting",
		    (uintptr_t)xfer, 0, 0, 0);
		SDT_PROBE2(usb, device, xfer, done,  xfer, USBD_CANCELLED);
		return USBD_CANCELLED;
	}

	KASSERT(xfer->ux_length == 0 || xfer->ux_buf != NULL);

	size = xfer->ux_length;
	flags = xfer->ux_flags;

	if (size != 0) {
		/*
		 * Use the xfer buffer if none specified in transfer setup.
		 * isoc transfers always use the xfer buffer, i.e.
		 * ux_buffer is always NULL for isoc.
		 */
		if (xfer->ux_buffer == NULL) {
			xfer->ux_buffer = xfer->ux_buf;
		}

		/*
		 * If not using the xfer buffer copy data to the
		 * xfer buffer for OUT transfers of >0 length
		 */
		if (xfer->ux_buffer != xfer->ux_buf) {
			KASSERT(xfer->ux_buf);
			if (!usbd_xfer_isread(xfer)) {
				memcpy(xfer->ux_buf, xfer->ux_buffer, size);
			}
		}
	}

	/* xfer is not valid after the transfer method unless synchronous */
	SDT_PROBE2(usb, device, pipe, transfer__start,  pipe, xfer);
	do {
		usbd_lock_pipe(pipe);
		err = usb_insert_transfer(xfer);
		usbd_unlock_pipe(pipe);
		if (err)
			break;
		err = pipe->up_methods->upm_transfer(xfer);
	} while (0);
	SDT_PROBE3(usb, device, pipe, transfer__done,  pipe, xfer, err);

	if (err != USBD_IN_PROGRESS && err) {
		/*
		 * The transfer made it onto the pipe queue, but didn't get
		 * accepted by the HCD for some reason.  It needs removing
		 * from the pipe queue.
		 */
		USBHIST_LOG(usbdebug, "xfer failed: %jd, reinserting",
		    err, 0, 0, 0);
		usbd_lock_pipe(pipe);
		SDT_PROBE1(usb, device, xfer, preabort,  xfer);
#ifdef DIAGNOSTIC
		xfer->ux_state = XFER_BUSY;
#endif
		SIMPLEQ_REMOVE_HEAD(&pipe->up_queue, ux_next);
		if (pipe->up_serialise)
			usbd_start_next(pipe);
		usbd_unlock_pipe(pipe);
	}

	if (!(flags & USBD_SYNCHRONOUS)) {
		USBHIST_LOG(usbdebug, "<- done xfer %#jx, not sync (err %jd)",
		    (uintptr_t)xfer, err, 0, 0);
		if (err != USBD_IN_PROGRESS) /* XXX Possible?  */
			SDT_PROBE2(usb, device, xfer, done,  xfer, err);
		return err;
	}

	if (err != USBD_IN_PROGRESS) {
		USBHIST_LOG(usbdebug, "<- done xfer %#jx, sync (err %jd)",
		    (uintptr_t)xfer, err, 0, 0);
		SDT_PROBE2(usb, device, xfer, done,  xfer, err);
		return err;
	}

	/* Sync transfer, wait for completion. */
	usbd_lock_pipe(pipe);
	while (!xfer->ux_done) {
		if (pipe->up_dev->ud_bus->ub_usepolling)
			panic("usbd_transfer: not done");
		USBHIST_LOG(usbdebug, "<- sleeping on xfer %#jx",
		    (uintptr_t)xfer, 0, 0, 0);

		err = 0;
		if ((flags & USBD_SYNCHRONOUS_SIG) != 0) {
			err = cv_wait_sig(&xfer->ux_cv, pipe->up_dev->ud_bus->ub_lock);
		} else {
			cv_wait(&xfer->ux_cv, pipe->up_dev->ud_bus->ub_lock);
		}
		if (err) {
			if (!xfer->ux_done) {
				SDT_PROBE1(usb, device, xfer, abort,  xfer);
				pipe->up_methods->upm_abort(xfer);
			}
			break;
		}
	}
	SDT_PROBE2(usb, device, xfer, done,  xfer, xfer->ux_status);
	/* XXX Race to read xfer->ux_status?  */
	usbd_unlock_pipe(pipe);
	return xfer->ux_status;
}

/* Like usbd_transfer(), but waits for completion. */
usbd_status
usbd_sync_transfer(struct usbd_xfer *xfer)
{
	xfer->ux_flags |= USBD_SYNCHRONOUS;
	return usbd_transfer(xfer);
}

/* Like usbd_transfer(), but waits for completion and listens for signals. */
usbd_status
usbd_sync_transfer_sig(struct usbd_xfer *xfer)
{
	xfer->ux_flags |= USBD_SYNCHRONOUS | USBD_SYNCHRONOUS_SIG;
	return usbd_transfer(xfer);
}

static void *
usbd_alloc_buffer(struct usbd_xfer *xfer, uint32_t size)
{
	KASSERT(xfer->ux_buf == NULL);
	KASSERT(size != 0);

	xfer->ux_bufsize = 0;
#if NUSB_DMA > 0
	struct usbd_bus *bus = xfer->ux_bus;

	if (bus->ub_usedma) {
		usb_dma_t *dmap = &xfer->ux_dmabuf;

		KASSERT((bus->ub_dmaflags & USBMALLOC_COHERENT) == 0);
		int err = usb_allocmem(bus->ub_dmatag, size, 0, bus->ub_dmaflags, dmap);
		if (err) {
			return NULL;
		}
		xfer->ux_buf = KERNADDR(&xfer->ux_dmabuf, 0);
		xfer->ux_bufsize = size;

		return xfer->ux_buf;
	}
#endif
	KASSERT(xfer->ux_bus->ub_usedma == false);
	xfer->ux_buf = kmem_alloc(size, KM_SLEEP);
	xfer->ux_bufsize = size;
	return xfer->ux_buf;
}

static void
usbd_free_buffer(struct usbd_xfer *xfer)
{
	KASSERT(xfer->ux_buf != NULL);
	KASSERT(xfer->ux_bufsize != 0);

	void *buf = xfer->ux_buf;
	uint32_t size = xfer->ux_bufsize;

	xfer->ux_buf = NULL;
	xfer->ux_bufsize = 0;

#if NUSB_DMA > 0
	struct usbd_bus *bus = xfer->ux_bus;

	if (bus->ub_usedma) {
		usb_dma_t *dmap = &xfer->ux_dmabuf;

		usb_freemem(dmap);
		return;
	}
#endif
	KASSERT(xfer->ux_bus->ub_usedma == false);

	kmem_free(buf, size);
}

void *
usbd_get_buffer(struct usbd_xfer *xfer)
{
	return xfer->ux_buf;
}

struct usbd_pipe *
usbd_get_pipe0(struct usbd_device *dev)
{

	return dev->ud_pipe0;
}

static struct usbd_xfer *
usbd_alloc_xfer(struct usbd_device *dev, unsigned int nframes)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC();

	ASSERT_SLEEPABLE();

	xfer = dev->ud_bus->ub_methods->ubm_allocx(dev->ud_bus, nframes);
	if (xfer == NULL)
		goto out;
	xfer->ux_bus = dev->ud_bus;
	callout_init(&xfer->ux_callout, CALLOUT_MPSAFE);
	callout_setfunc(&xfer->ux_callout, usbd_xfer_timeout, xfer);
	cv_init(&xfer->ux_cv, "usbxfer");
	usb_init_task(&xfer->ux_aborttask, usbd_xfer_timeout_task, xfer,
	    USB_TASKQ_MPSAFE);

out:
	USBHIST_CALLARGS(usbdebug, "returns %#jx", (uintptr_t)xfer, 0, 0, 0);

	return xfer;
}

static void
usbd_free_xfer(struct usbd_xfer *xfer)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "%#jx", (uintptr_t)xfer, 0, 0, 0);

	if (xfer->ux_buf) {
		usbd_free_buffer(xfer);
	}

	/* Wait for any straggling timeout to complete. */
	mutex_enter(xfer->ux_bus->ub_lock);
	xfer->ux_timeout_reset = false; /* do not resuscitate */
	callout_halt(&xfer->ux_callout, xfer->ux_bus->ub_lock);
	usb_rem_task_wait(xfer->ux_pipe->up_dev, &xfer->ux_aborttask,
	    USB_TASKQ_HC, xfer->ux_bus->ub_lock);
	mutex_exit(xfer->ux_bus->ub_lock);

	cv_destroy(&xfer->ux_cv);
	xfer->ux_bus->ub_methods->ubm_freex(xfer->ux_bus, xfer);
}

int
usbd_create_xfer(struct usbd_pipe *pipe, size_t len, unsigned int flags,
    unsigned int nframes, struct usbd_xfer **xp)
{
	KASSERT(xp != NULL);
	void *buf = NULL;

	struct usbd_xfer *xfer = usbd_alloc_xfer(pipe->up_dev, nframes);
	if (xfer == NULL)
		return ENOMEM;

	xfer->ux_pipe = pipe;
	xfer->ux_flags = flags;
	xfer->ux_nframes = nframes;
	xfer->ux_methods = pipe->up_methods;

	if (len) {
		buf = usbd_alloc_buffer(xfer, len);
		if (!buf) {
			usbd_free_xfer(xfer);
			return ENOMEM;
		}
	}

	if (xfer->ux_methods->upm_init) {
		int err = xfer->ux_methods->upm_init(xfer);
		if (err) {
			usbd_free_xfer(xfer);
			return err;
		}
	}

	*xp = xfer;
	SDT_PROBE5(usb, device, xfer, create,
	    xfer, pipe, len, flags, nframes);
	return 0;
}

void
usbd_destroy_xfer(struct usbd_xfer *xfer)
{

	SDT_PROBE1(usb, device, xfer, destroy,  xfer);
	if (xfer->ux_methods->upm_fini)
		xfer->ux_methods->upm_fini(xfer);

	usbd_free_xfer(xfer);
}

void
usbd_setup_xfer(struct usbd_xfer *xfer, void *priv, void *buffer,
    uint32_t length, uint16_t flags, uint32_t timeout, usbd_callback callback)
{
	KASSERT(xfer->ux_pipe);

	xfer->ux_priv = priv;
	xfer->ux_buffer = buffer;
	xfer->ux_length = length;
	xfer->ux_actlen = 0;
	xfer->ux_flags = flags;
	xfer->ux_timeout = timeout;
	xfer->ux_status = USBD_NOT_STARTED;
	xfer->ux_callback = callback;
	xfer->ux_rqflags &= ~URQ_REQUEST;
	xfer->ux_nframes = 0;
}

void
usbd_setup_default_xfer(struct usbd_xfer *xfer, struct usbd_device *dev,
    void *priv, uint32_t timeout, usb_device_request_t *req, void *buffer,
    uint32_t length, uint16_t flags, usbd_callback callback)
{
	KASSERT(xfer->ux_pipe == dev->ud_pipe0);

	xfer->ux_priv = priv;
	xfer->ux_buffer = buffer;
	xfer->ux_length = length;
	xfer->ux_actlen = 0;
	xfer->ux_flags = flags;
	xfer->ux_timeout = timeout;
	xfer->ux_status = USBD_NOT_STARTED;
	xfer->ux_callback = callback;
	xfer->ux_request = *req;
	xfer->ux_rqflags |= URQ_REQUEST;
	xfer->ux_nframes = 0;
}

void
usbd_setup_isoc_xfer(struct usbd_xfer *xfer, void *priv, uint16_t *frlengths,
    uint32_t nframes, uint16_t flags, usbd_callback callback)
{
	xfer->ux_priv = priv;
	xfer->ux_buffer = NULL;
	xfer->ux_length = 0;
	xfer->ux_actlen = 0;
	xfer->ux_flags = flags;
	xfer->ux_timeout = USBD_NO_TIMEOUT;
	xfer->ux_status = USBD_NOT_STARTED;
	xfer->ux_callback = callback;
	xfer->ux_rqflags &= ~URQ_REQUEST;
	xfer->ux_frlengths = frlengths;
	xfer->ux_nframes = nframes;

	for (size_t i = 0; i < xfer->ux_nframes; i++)
		xfer->ux_length += xfer->ux_frlengths[i];
}

void
usbd_get_xfer_status(struct usbd_xfer *xfer, void **priv,
		     void **buffer, uint32_t *count, usbd_status *status)
{
	if (priv != NULL)
		*priv = xfer->ux_priv;
	if (buffer != NULL)
		*buffer = xfer->ux_buffer;
	if (count != NULL)
		*count = xfer->ux_actlen;
	if (status != NULL)
		*status = xfer->ux_status;
}

usb_config_descriptor_t *
usbd_get_config_descriptor(struct usbd_device *dev)
{
	KASSERT(dev != NULL);

	return dev->ud_cdesc;
}

usb_interface_descriptor_t *
usbd_get_interface_descriptor(struct usbd_interface *iface)
{
	KASSERT(iface != NULL);

	return iface->ui_idesc;
}

usb_device_descriptor_t *
usbd_get_device_descriptor(struct usbd_device *dev)
{
	KASSERT(dev != NULL);

	return &dev->ud_ddesc;
}

usb_endpoint_descriptor_t *
usbd_interface2endpoint_descriptor(struct usbd_interface *iface, uint8_t index)
{

	if (index >= iface->ui_idesc->bNumEndpoints)
		return NULL;
	return iface->ui_endpoints[index].ue_edesc;
}

/* Some drivers may wish to abort requests on the default pipe, *
 * but there is no mechanism for getting a handle on it.        */
void
usbd_abort_default_pipe(struct usbd_device *device)
{
	usbd_abort_pipe(device->ud_pipe0);
}

void
usbd_abort_pipe(struct usbd_pipe *pipe)
{

	KASSERT(pipe != NULL);

	usbd_lock_pipe(pipe);
	usbd_ar_pipe(pipe);
	usbd_unlock_pipe(pipe);
}

usbd_status
usbd_clear_endpoint_stall(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	SDT_PROBE1(usb, device, pipe, clear__endpoint__stall,  pipe);

	/*
	 * Clearing en endpoint stall resets the endpoint toggle, so
	 * do the same to the HC toggle.
	 */
	SDT_PROBE1(usb, device, pipe, clear__endpoint__toggle,  pipe);
	pipe->up_methods->upm_cleartoggle(pipe);

	err = usbd_clear_endpoint_feature(dev,
	    pipe->up_endpoint->ue_edesc->bEndpointAddress, UF_ENDPOINT_HALT);
#if 0
XXX should we do this?
	if (!err) {
		pipe->state = USBD_PIPE_ACTIVE;
		/* XXX activate pipe */
	}
#endif
	return err;
}

void
usbd_clear_endpoint_stall_task(void *arg)
{
	struct usbd_pipe *pipe = arg;
	struct usbd_device *dev = pipe->up_dev;

	SDT_PROBE1(usb, device, pipe, clear__endpoint__stall,  pipe);
	SDT_PROBE1(usb, device, pipe, clear__endpoint__toggle,  pipe);
	pipe->up_methods->upm_cleartoggle(pipe);

	(void)usbd_clear_endpoint_feature(dev,
	    pipe->up_endpoint->ue_edesc->bEndpointAddress, UF_ENDPOINT_HALT);
}

void
usbd_clear_endpoint_stall_async(struct usbd_pipe *pipe)
{
	usb_add_task(pipe->up_dev, &pipe->up_async_task, USB_TASKQ_DRIVER);
}

void
usbd_clear_endpoint_toggle(struct usbd_pipe *pipe)
{

	SDT_PROBE1(usb, device, pipe, clear__endpoint__toggle,  pipe);
	pipe->up_methods->upm_cleartoggle(pipe);
}

usbd_status
usbd_endpoint_count(struct usbd_interface *iface, uint8_t *count)
{
	KASSERT(iface != NULL);
	KASSERT(iface->ui_idesc != NULL);

	*count = iface->ui_idesc->bNumEndpoints;
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_interface_count(struct usbd_device *dev, uint8_t *count)
{

	if (dev->ud_cdesc == NULL)
		return USBD_NOT_CONFIGURED;
	*count = dev->ud_cdesc->bNumInterface;
	return USBD_NORMAL_COMPLETION;
}

void
usbd_interface2device_handle(struct usbd_interface *iface,
			     struct usbd_device **dev)
{

	*dev = iface->ui_dev;
}

usbd_status
usbd_device2interface_handle(struct usbd_device *dev,
			     uint8_t ifaceno, struct usbd_interface **iface)
{

	if (dev->ud_cdesc == NULL)
		return USBD_NOT_CONFIGURED;
	if (ifaceno >= dev->ud_cdesc->bNumInterface)
		return USBD_INVAL;
	*iface = &dev->ud_ifaces[ifaceno];
	return USBD_NORMAL_COMPLETION;
}

struct usbd_device *
usbd_pipe2device_handle(struct usbd_pipe *pipe)
{
	KASSERT(pipe != NULL);

	return pipe->up_dev;
}

/* XXXX use altno */
usbd_status
usbd_set_interface(struct usbd_interface *iface, int altidx)
{
	bool locked = false;
	usb_device_request_t req;
	usbd_status err;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface %#jx", (uintptr_t)iface, 0, 0, 0);

	err = usbd_iface_lock(iface);
	if (err)
		goto out;
	locked = true;

	err = usbd_fill_iface_data(iface->ui_dev, iface->ui_index, altidx);
	if (err)
		goto out;

	req.bmRequestType = UT_WRITE_INTERFACE;
	req.bRequest = UR_SET_INTERFACE;
	USETW(req.wValue, iface->ui_idesc->bAlternateSetting);
	USETW(req.wIndex, iface->ui_idesc->bInterfaceNumber);
	USETW(req.wLength, 0);
	err = usbd_do_request(iface->ui_dev, &req, 0);

out:	/* XXX back out iface data?  */
	if (locked)
		usbd_iface_unlock(iface);
	return err;
}

int
usbd_get_no_alts(usb_config_descriptor_t *cdesc, int ifaceno)
{
	char *p = (char *)cdesc;
	char *end = p + UGETW(cdesc->wTotalLength);
	usb_interface_descriptor_t *d;
	int n;

	for (n = 0; p < end; p += d->bLength) {
		d = (usb_interface_descriptor_t *)p;
		if (p + d->bLength <= end &&
		    d->bDescriptorType == UDESC_INTERFACE &&
		    d->bInterfaceNumber == ifaceno)
			n++;
	}
	return n;
}

int
usbd_get_interface_altindex(struct usbd_interface *iface)
{
	return iface->ui_altindex;
}

usbd_status
usbd_get_interface(struct usbd_interface *iface, uint8_t *aiface)
{
	usb_device_request_t req;

	req.bmRequestType = UT_READ_INTERFACE;
	req.bRequest = UR_GET_INTERFACE;
	USETW(req.wValue, 0);
	USETW(req.wIndex, iface->ui_idesc->bInterfaceNumber);
	USETW(req.wLength, 1);
	return usbd_do_request(iface->ui_dev, &req, aiface);
}

/*** Internal routines ***/

/* Dequeue all pipe operations, called with bus lock held. */
Static void
usbd_ar_pipe(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "pipe = %#jx", (uintptr_t)pipe, 0, 0, 0);
	SDT_PROBE1(usb, device, pipe, abort__start,  pipe);

	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));

#ifdef USB_DEBUG
	if (usbdebug > 5)
		usbd_dump_queue(pipe);
#endif
	pipe->up_repeat = 0;
	pipe->up_running = 0;
	pipe->up_aborting = 1;
	while ((xfer = SIMPLEQ_FIRST(&pipe->up_queue)) != NULL) {
		USBHIST_LOG(usbdebug, "pipe = %#jx xfer = %#jx "
		    "(methods = %#jx)", (uintptr_t)pipe, (uintptr_t)xfer,
		    (uintptr_t)pipe->up_methods, 0);
		if (xfer->ux_status == USBD_NOT_STARTED) {
			SDT_PROBE1(usb, device, xfer, preabort,  xfer);
#ifdef DIAGNOSTIC
			xfer->ux_state = XFER_BUSY;
#endif
			SIMPLEQ_REMOVE_HEAD(&pipe->up_queue, ux_next);
		} else {
			/* Make the HC abort it (and invoke the callback). */
			SDT_PROBE1(usb, device, xfer, abort,  xfer);
			pipe->up_methods->upm_abort(xfer);
			while (pipe->up_callingxfer == xfer) {
				USBHIST_LOG(usbdebug, "wait for callback"
				    "pipe = %#jx xfer = %#jx",
				    (uintptr_t)pipe, (uintptr_t)xfer, 0, 0);
				cv_wait(&pipe->up_callingcv,
				    pipe->up_dev->ud_bus->ub_lock);
			}
			/* XXX only for non-0 usbd_clear_endpoint_stall(pipe); */
		}
	}
	pipe->up_aborting = 0;
	SDT_PROBE1(usb, device, pipe, abort__done,  pipe);
}

/* Called with USB lock held. */
void
usb_transfer_complete(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct usbd_bus *bus = pipe->up_dev->ud_bus;
	int sync = xfer->ux_flags & USBD_SYNCHRONOUS;
	int erred;
	int polling = bus->ub_usepolling;
	int repeat = pipe->up_repeat;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "pipe = %#jx xfer = %#jx status = %jd "
	    "actlen = %jd", (uintptr_t)pipe, (uintptr_t)xfer, xfer->ux_status,
	    xfer->ux_actlen);

	KASSERT(polling || mutex_owned(pipe->up_dev->ud_bus->ub_lock));
	KASSERTMSG(xfer->ux_state == XFER_ONQU, "xfer %p state is %x", xfer,
	    xfer->ux_state);
	KASSERT(pipe != NULL);

	/*
	 * If device is known to miss out ack, then pretend that
	 * output timeout is a success. Userland should handle
	 * the logic to verify that the operation succeeded.
	 */
	if (pipe->up_dev->ud_quirks &&
	    pipe->up_dev->ud_quirks->uq_flags & UQ_MISS_OUT_ACK &&
	    xfer->ux_status == USBD_TIMEOUT &&
	    !usbd_xfer_isread(xfer)) {
		USBHIST_LOG(usbdebug, "Possible output ack miss for xfer %#jx: "
		    "hiding write timeout to %jd.%jd for %ju bytes written",
		    (uintptr_t)xfer, curlwp->l_proc->p_pid, curlwp->l_lid,
		    xfer->ux_length);

		xfer->ux_status = USBD_NORMAL_COMPLETION;
		xfer->ux_actlen = xfer->ux_length;
	}

	erred = xfer->ux_status == USBD_CANCELLED ||
	        xfer->ux_status == USBD_TIMEOUT;

	if (!repeat) {
		/* Remove request from queue. */

		KASSERTMSG(!SIMPLEQ_EMPTY(&pipe->up_queue),
		    "pipe %p is empty, but xfer %p wants to complete", pipe,
		     xfer);
		KASSERTMSG(xfer == SIMPLEQ_FIRST(&pipe->up_queue),
		    "xfer %p is not start of queue (%p is at start)", xfer,
		   SIMPLEQ_FIRST(&pipe->up_queue));

#ifdef DIAGNOSTIC
		xfer->ux_state = XFER_BUSY;
#endif
		SIMPLEQ_REMOVE_HEAD(&pipe->up_queue, ux_next);
	}
	USBHIST_LOG(usbdebug, "xfer %#jx: repeat %jd new head = %#jx",
	    (uintptr_t)xfer, repeat, (uintptr_t)SIMPLEQ_FIRST(&pipe->up_queue),
	    0);

	/* Count completed transfers. */
	++pipe->up_dev->ud_bus->ub_stats.uds_requests
		[pipe->up_endpoint->ue_edesc->bmAttributes & UE_XFERTYPE];

	xfer->ux_done = 1;
	if (!xfer->ux_status && xfer->ux_actlen < xfer->ux_length &&
	    !(xfer->ux_flags & USBD_SHORT_XFER_OK)) {
		USBHIST_LOG(usbdebug, "short transfer %jd < %jd",
		    xfer->ux_actlen, xfer->ux_length, 0, 0);
		xfer->ux_status = USBD_SHORT_XFER;
	}

	USBHIST_LOG(usbdebug, "xfer %#jx doing done %#jx", (uintptr_t)xfer,
	    (uintptr_t)pipe->up_methods->upm_done, 0, 0);
	SDT_PROBE2(usb, device, xfer, done,  xfer, xfer->ux_status);
	pipe->up_methods->upm_done(xfer);

	if (xfer->ux_length != 0 && xfer->ux_buffer != xfer->ux_buf) {
		KDASSERTMSG(xfer->ux_actlen <= xfer->ux_length,
		    "actlen %d length %d",xfer->ux_actlen, xfer->ux_length);

		/* Only if IN transfer */
		if (usbd_xfer_isread(xfer)) {
			memcpy(xfer->ux_buffer, xfer->ux_buf, xfer->ux_actlen);
		}
	}

	USBHIST_LOG(usbdebug, "xfer %#jx doing callback %#jx status %jd",
	    (uintptr_t)xfer, (uintptr_t)xfer->ux_callback, xfer->ux_status, 0);

	if (xfer->ux_callback) {
		if (!polling) {
			KASSERT(pipe->up_callingxfer == NULL);
			pipe->up_callingxfer = xfer;
			mutex_exit(pipe->up_dev->ud_bus->ub_lock);
			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_LOCK(1, curlwp);
		}

		xfer->ux_callback(xfer, xfer->ux_priv, xfer->ux_status);

		if (!polling) {
			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_UNLOCK_ONE(curlwp);
			mutex_enter(pipe->up_dev->ud_bus->ub_lock);
			KASSERT(pipe->up_callingxfer == xfer);
			pipe->up_callingxfer = NULL;
			cv_broadcast(&pipe->up_callingcv);
		}
	}

	if (sync && !polling) {
		USBHIST_LOG(usbdebug, "<- done xfer %#jx, wakeup",
		    (uintptr_t)xfer, 0, 0, 0);
		cv_broadcast(&xfer->ux_cv);
	}

	if (repeat) {
		xfer->ux_actlen = 0;
		xfer->ux_status = USBD_NOT_STARTED;
	} else {
		/* XXX should we stop the queue on all errors? */
		if (erred && pipe->up_iface != NULL)	/* not control pipe */
			pipe->up_running = 0;
	}
	if (pipe->up_running && pipe->up_serialise)
		usbd_start_next(pipe);
}

/* Called with USB lock held. */
static usbd_status
usb_insert_transfer(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug,
	    "xfer = %#jx pipe = %#jx running = %jd timeout = %jd",
	    (uintptr_t)xfer, (uintptr_t)pipe,
	    pipe->up_running, xfer->ux_timeout);

	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));
	KASSERTMSG(xfer->ux_state == XFER_BUSY, "xfer %p state is %x", xfer,
	    xfer->ux_state);

#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_ONQU;
#endif
	SIMPLEQ_INSERT_TAIL(&pipe->up_queue, xfer, ux_next);
	if (pipe->up_running && pipe->up_serialise)
		err = USBD_IN_PROGRESS;
	else {
		pipe->up_running = 1;
		err = USBD_NORMAL_COMPLETION;
	}
	USBHIST_LOG(usbdebug, "<- done xfer %#jx, err %jd", (uintptr_t)xfer,
	    err, 0, 0);
	return err;
}

/* Called with USB lock held. */
void
usbd_start_next(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;
	usbd_status err;

	USBHIST_FUNC();

	KASSERT(pipe != NULL);
	KASSERT(pipe->up_methods != NULL);
	KASSERT(pipe->up_methods->upm_start != NULL);
	KASSERT(pipe->up_serialise == true);

	int polling = pipe->up_dev->ud_bus->ub_usepolling;
	KASSERT(polling || mutex_owned(pipe->up_dev->ud_bus->ub_lock));

	/* Get next request in queue. */
	xfer = SIMPLEQ_FIRST(&pipe->up_queue);
	USBHIST_CALLARGS(usbdebug, "pipe = %#jx, xfer = %#jx", (uintptr_t)pipe,
	    (uintptr_t)xfer, 0, 0);
	if (xfer == NULL) {
		pipe->up_running = 0;
	} else {
		if (!polling)
			mutex_exit(pipe->up_dev->ud_bus->ub_lock);
		SDT_PROBE2(usb, device, pipe, start,  pipe, xfer);
		err = pipe->up_methods->upm_start(xfer);
		if (!polling)
			mutex_enter(pipe->up_dev->ud_bus->ub_lock);

		if (err != USBD_IN_PROGRESS) {
			USBHIST_LOG(usbdebug, "error = %jd", err, 0, 0, 0);
			pipe->up_running = 0;
			/* XXX do what? */
		}
	}

	KASSERT(polling || mutex_owned(pipe->up_dev->ud_bus->ub_lock));
}

usbd_status
usbd_do_request(struct usbd_device *dev, usb_device_request_t *req, void *data)
{

	return usbd_do_request_flags(dev, req, data, 0, 0,
	    USBD_DEFAULT_TIMEOUT);
}

usbd_status
usbd_do_request_flags(struct usbd_device *dev, usb_device_request_t *req,
    void *data, uint16_t flags, int *actlen, uint32_t timeout)
{
	size_t len = UGETW(req->wLength);

	return usbd_do_request_len(dev, req, len, data, flags, actlen, timeout);
}

usbd_status
usbd_do_request_len(struct usbd_device *dev, usb_device_request_t *req,
    size_t len, void *data, uint16_t flags, int *actlen, uint32_t timeout)
{
	struct usbd_xfer *xfer;
	usbd_status err;

	KASSERT(len >= UGETW(req->wLength));

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev=%#jx req=%jx flags=%jx len=%jx",
	    (uintptr_t)dev, (uintptr_t)req, flags, len);

	ASSERT_SLEEPABLE();

	int error = usbd_create_xfer(dev->ud_pipe0, len, 0, 0, &xfer);
	if (error)
		return USBD_NOMEM;

	usbd_setup_default_xfer(xfer, dev, 0, timeout, req, data,
	    UGETW(req->wLength), flags, NULL);
	KASSERT(xfer->ux_pipe == dev->ud_pipe0);
	err = usbd_sync_transfer(xfer);
#if defined(USB_DEBUG) || defined(DIAGNOSTIC)
	if (xfer->ux_actlen > xfer->ux_length) {
		USBHIST_LOG(usbdebug, "overrun addr = %jd type = 0x%02jx",
		    dev->ud_addr, xfer->ux_request.bmRequestType, 0, 0);
		USBHIST_LOG(usbdebug, "     req = 0x%02jx val = %jd "
		    "index = %jd",
		    xfer->ux_request.bRequest, UGETW(xfer->ux_request.wValue),
		    UGETW(xfer->ux_request.wIndex), 0);
		USBHIST_LOG(usbdebug, "     rlen = %jd length = %jd "
		    "actlen = %jd",
		    UGETW(xfer->ux_request.wLength),
		    xfer->ux_length, xfer->ux_actlen, 0);
	}
#endif
	if (actlen != NULL)
		*actlen = xfer->ux_actlen;

	usbd_destroy_xfer(xfer);

	if (err) {
		USBHIST_LOG(usbdebug, "returning err = %jd", err, 0, 0, 0);
	}
	return err;
}

static void
usbd_request_async_cb(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	usbd_destroy_xfer(xfer);
}

/*
 * Execute a request without waiting for completion.
 * Can be used from interrupt context.
 */
usbd_status
usbd_request_async(struct usbd_device *dev, struct usbd_xfer *xfer,
    usb_device_request_t *req, void *priv, usbd_callback callback)
{
	usbd_status err;

	if (callback == NULL)
		callback = usbd_request_async_cb;

	usbd_setup_default_xfer(xfer, dev, priv,
	    USBD_DEFAULT_TIMEOUT, req, NULL, UGETW(req->wLength), 0,
	    callback);
	err = usbd_transfer(xfer);
	if (err != USBD_IN_PROGRESS) {
		usbd_destroy_xfer(xfer);
		return (err);
	}
	return (USBD_NORMAL_COMPLETION);
}

const struct usbd_quirks *
usbd_get_quirks(struct usbd_device *dev)
{
#ifdef DIAGNOSTIC
	if (dev == NULL) {
		printf("usbd_get_quirks: dev == NULL\n");
		return 0;
	}
#endif
	return dev->ud_quirks;
}

/* XXX do periodic free() of free list */

/*
 * Called from keyboard driver when in polling mode.
 */
void
usbd_dopoll(struct usbd_interface *iface)
{
	iface->ui_dev->ud_bus->ub_methods->ubm_dopoll(iface->ui_dev->ud_bus);
}

/*
 * This is for keyboard driver as well, which only operates in polling
 * mode from the ask root, etc., prompt and from DDB.
 */
void
usbd_set_polling(struct usbd_device *dev, int on)
{
	if (on)
		dev->ud_bus->ub_usepolling++;
	else
		dev->ud_bus->ub_usepolling--;

	/* Kick the host controller when switching modes */
	mutex_enter(dev->ud_bus->ub_lock);
	dev->ud_bus->ub_methods->ubm_softint(dev->ud_bus);
	mutex_exit(dev->ud_bus->ub_lock);
}


usb_endpoint_descriptor_t *
usbd_get_endpoint_descriptor(struct usbd_interface *iface, uint8_t address)
{
	struct usbd_endpoint *ep;
	int i;

	for (i = 0; i < iface->ui_idesc->bNumEndpoints; i++) {
		ep = &iface->ui_endpoints[i];
		if (ep->ue_edesc->bEndpointAddress == address)
			return iface->ui_endpoints[i].ue_edesc;
	}
	return NULL;
}

/*
 * usbd_ratecheck() can limit the number of error messages that occurs.
 * When a device is unplugged it may take up to 0.25s for the hub driver
 * to notice it.  If the driver continuously tries to do I/O operations
 * this can generate a large number of messages.
 */
int
usbd_ratecheck(struct timeval *last)
{
	static struct timeval errinterval = { 0, 250000 }; /* 0.25 s*/

	return ratecheck(last, &errinterval);
}

/*
 * Search for a vendor/product pair in an array.  The item size is
 * given as an argument.
 */
const struct usb_devno *
usb_match_device(const struct usb_devno *tbl, u_int nentries, u_int sz,
		 uint16_t vendor, uint16_t product)
{
	while (nentries-- > 0) {
		uint16_t tproduct = tbl->ud_product;
		if (tbl->ud_vendor == vendor &&
		    (tproduct == product || tproduct == USB_PRODUCT_ANY))
			return tbl;
		tbl = (const struct usb_devno *)((const char *)tbl + sz);
	}
	return NULL;
}

usbd_status
usbd_get_string(struct usbd_device *dev, int si, char *buf)
{
	return usbd_get_string0(dev, si, buf, 1);
}

usbd_status
usbd_get_string0(struct usbd_device *dev, int si, char *buf, int unicode)
{
	int swap = dev->ud_quirks->uq_flags & UQ_SWAP_UNICODE;
	usb_string_descriptor_t us;
	char *s;
	int i, n;
	uint16_t c;
	usbd_status err;
	int size;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	buf[0] = '\0';
	if (si == 0)
		return USBD_INVAL;
	if (dev->ud_quirks->uq_flags & UQ_NO_STRINGS)
		return USBD_STALLED;
	if (dev->ud_langid == USBD_NOLANG) {
		/* Set up default language */
		err = usbd_get_string_desc(dev, USB_LANGUAGE_TABLE, 0, &us,
		    &size);
		if (err || size < 4) {
			USBHIST_LOG(usbdebug, "getting lang failed, using 0",
			    0, 0, 0, 0);
			dev->ud_langid = 0; /* Well, just pick something then */
		} else {
			/* Pick the first language as the default. */
			dev->ud_langid = UGETW(us.bString[0]);
		}
	}
	err = usbd_get_string_desc(dev, si, dev->ud_langid, &us, &size);
	if (err)
		return err;
	s = buf;
	n = size / 2 - 1;
	if (unicode) {
		for (i = 0; i < n; i++) {
			c = UGETW(us.bString[i]);
			if (swap)
				c = (c >> 8) | (c << 8);
			s += wput_utf8(s, 3, c);
		}
		*s++ = 0;
	}
#ifdef COMPAT_30
	else {
		for (i = 0; i < n; i++) {
			c = UGETW(us.bString[i]);
			if (swap)
				c = (c >> 8) | (c << 8);
			*s++ = (c < 0x80) ? c : '?';
		}
		*s++ = 0;
	}
#endif
	return USBD_NORMAL_COMPLETION;
}

/*
 * usbd_xfer_trycomplete(xfer)
 *
 *	Try to claim xfer for completion.  Return true if successful,
 *	false if the xfer has been synchronously aborted or has timed
 *	out.
 *
 *	If this returns true, caller is responsible for setting
 *	xfer->ux_status and calling usb_transfer_complete.  To be used
 *	in a host controller interrupt handler.
 *
 *	Caller must either hold the bus lock or have the bus in polling
 *	mode.
 */
bool
usbd_xfer_trycomplete(struct usbd_xfer *xfer)
{
	struct usbd_bus *bus __diagused = xfer->ux_bus;

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));

	/*
	 * If software has completed it, either by synchronous abort or
	 * by timeout, too late.
	 */
	if (xfer->ux_status != USBD_IN_PROGRESS)
		return false;

	/*
	 * We are completing the xfer.  Cancel the timeout if we can,
	 * but only asynchronously.  See usbd_xfer_cancel_timeout_async
	 * for why we need not wait for the callout or task here.
	 */
	usbd_xfer_cancel_timeout_async(xfer);

	/* Success!  Note: Caller must set xfer->ux_status afterwar.  */
	return true;
}

/*
 * usbd_xfer_abort(xfer)
 *
 *	Try to claim xfer to abort.  If successful, mark it completed
 *	with USBD_CANCELLED and call the bus-specific method to abort
 *	at the hardware level.
 *
 *	To be called in thread context from struct
 *	usbd_pipe_methods::upm_abort.
 *
 *	Caller must hold the bus lock.
 */
void
usbd_xfer_abort(struct usbd_xfer *xfer)
{
	struct usbd_bus *bus = xfer->ux_bus;

	KASSERT(mutex_owned(bus->ub_lock));

	/*
	 * If host controller interrupt or timer interrupt has
	 * completed it, too late.  But the xfer cannot be
	 * cancelled already -- only one caller can synchronously
	 * abort.
	 */
	KASSERT(xfer->ux_status != USBD_CANCELLED);
	if (xfer->ux_status != USBD_IN_PROGRESS)
		return;

	/*
	 * Cancel the timeout if we can, but only asynchronously; see
	 * usbd_xfer_cancel_timeout_async for why we need not wait for
	 * the callout or task here.
	 */
	usbd_xfer_cancel_timeout_async(xfer);

	/*
	 * We beat everyone else.  Claim the status as cancelled, do
	 * the bus-specific dance to abort the hardware, and complete
	 * the xfer.
	 */
	xfer->ux_status = USBD_CANCELLED;
	bus->ub_methods->ubm_abortx(xfer);
	usb_transfer_complete(xfer);
}

/*
 * usbd_xfer_timeout(xfer)
 *
 *	Called at IPL_SOFTCLOCK when too much time has elapsed waiting
 *	for xfer to complete.  Since we can't abort the xfer at
 *	IPL_SOFTCLOCK, defer to a usb_task to run it in thread context,
 *	unless the xfer has completed or aborted concurrently -- and if
 *	the xfer has also been resubmitted, take care of rescheduling
 *	the callout.
 */
static void
usbd_xfer_timeout(void *cookie)
{
	struct usbd_xfer *xfer = cookie;
	struct usbd_bus *bus = xfer->ux_bus;
	struct usbd_device *dev = xfer->ux_pipe->up_dev;

	/* Acquire the lock so we can transition the timeout state.  */
	mutex_enter(bus->ub_lock);

	/*
	 * Use usbd_xfer_probe_timeout to check whether the timeout is
	 * still valid, or to reschedule the callout if necessary.  If
	 * it is still valid, schedule the task.
	 */
	if (usbd_xfer_probe_timeout(xfer))
		usb_add_task(dev, &xfer->ux_aborttask, USB_TASKQ_HC);

	/*
	 * Notify usbd_xfer_cancel_timeout_async that we may have
	 * scheduled the task.  This causes callout_invoking to return
	 * false in usbd_xfer_cancel_timeout_async so that it can tell
	 * which stage in the callout->task->abort process we're at.
	 */
	callout_ack(&xfer->ux_callout);

	/* All done -- release the lock.  */
	mutex_exit(bus->ub_lock);
}

/*
 * usbd_xfer_timeout_task(xfer)
 *
 *	Called in thread context when too much time has elapsed waiting
 *	for xfer to complete.  Abort the xfer with USBD_TIMEOUT, unless
 *	it has completed or aborted concurrently -- and if the xfer has
 *	also been resubmitted, take care of rescheduling the callout.
 */
static void
usbd_xfer_timeout_task(void *cookie)
{
	struct usbd_xfer *xfer = cookie;
	struct usbd_bus *bus = xfer->ux_bus;

	/* Acquire the lock so we can transition the timeout state.  */
	mutex_enter(bus->ub_lock);

	/*
	 * Use usbd_xfer_probe_timeout to check whether the timeout is
	 * still valid, or to reschedule the callout if necessary.  If
	 * it is not valid -- the timeout has been asynchronously
	 * cancelled, or the xfer has already been resubmitted -- then
	 * we're done here.
	 */
	if (!usbd_xfer_probe_timeout(xfer))
		goto out;

	/*
	 * May have completed or been aborted, but we're the only one
	 * who can time it out.  If it has completed or been aborted,
	 * no need to timeout.
	 */
	KASSERT(xfer->ux_status != USBD_TIMEOUT);
	if (xfer->ux_status != USBD_IN_PROGRESS)
		goto out;

	/*
	 * We beat everyone else.  Claim the status as timed out, do
	 * the bus-specific dance to abort the hardware, and complete
	 * the xfer.
	 */
	xfer->ux_status = USBD_TIMEOUT;
	bus->ub_methods->ubm_abortx(xfer);
	usb_transfer_complete(xfer);

out:	/* All done -- release the lock.  */
	mutex_exit(bus->ub_lock);
}

/*
 * usbd_xfer_probe_timeout(xfer)
 *
 *	Probe the status of xfer's timeout.  Acknowledge and process a
 *	request to reschedule.  Return true if the timeout is still
 *	valid and the caller should take further action (queueing a
 *	task or aborting the xfer), false if it must stop here.
 */
static bool
usbd_xfer_probe_timeout(struct usbd_xfer *xfer)
{
	struct usbd_bus *bus = xfer->ux_bus;
	bool valid;

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));

	/* The timeout must be set.  */
	KASSERT(xfer->ux_timeout_set);

	/*
	 * Neither callout nor task may be pending; they execute
	 * alternately in lock step.
	 */
	KASSERT(!callout_pending(&xfer->ux_callout));
	KASSERT(!usb_task_pending(xfer->ux_pipe->up_dev, &xfer->ux_aborttask));

	/* There are a few cases... */
	if (bus->ub_methods->ubm_dying(bus)) {
		/* Host controller dying.  Drop it all on the floor.  */
		xfer->ux_timeout_set = false;
		xfer->ux_timeout_reset = false;
		valid = false;
	} else if (xfer->ux_timeout_reset) {
		/*
		 * The xfer completed _and_ got resubmitted while we
		 * waited for the lock.  Acknowledge the request to
		 * reschedule, and reschedule it if there is a timeout
		 * and the bus is not polling.
		 */
		xfer->ux_timeout_reset = false;
		if (xfer->ux_timeout && !bus->ub_usepolling) {
			KASSERT(xfer->ux_timeout_set);
			callout_schedule(&xfer->ux_callout,
			    mstohz(xfer->ux_timeout));
		} else {
			/* No more callout or task scheduled.  */
			xfer->ux_timeout_set = false;
		}
		valid = false;
	} else if (xfer->ux_status != USBD_IN_PROGRESS) {
		/*
		 * The xfer has completed by hardware completion or by
		 * software abort, and has not been resubmitted, so the
		 * timeout must be unset, and is no longer valid for
		 * the caller.
		 */
		xfer->ux_timeout_set = false;
		valid = false;
	} else {
		/*
		 * The xfer has not yet completed, so the timeout is
		 * valid.
		 */
		valid = true;
	}

	/* Any reset must have been processed.  */
	KASSERT(!xfer->ux_timeout_reset);

	/*
	 * Either we claim the timeout is set, or the callout is idle.
	 * If the timeout is still set, we may be handing off to the
	 * task instead, so this is an if but not an iff.
	 */
	KASSERT(xfer->ux_timeout_set || !callout_pending(&xfer->ux_callout));

	/*
	 * The task must be idle now.
	 *
	 * - If the caller is the callout, _and_ the timeout is still
	 *   valid, the caller will schedule it, but it hasn't been
	 *   scheduled yet.  (If the timeout is not valid, the task
	 *   should not be scheduled.)
	 *
	 * - If the caller is the task, it cannot be scheduled again
	 *   until the callout runs again, which won't happen until we
	 *   next release the lock.
	 */
	KASSERT(!usb_task_pending(xfer->ux_pipe->up_dev, &xfer->ux_aborttask));

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));

	return valid;
}

/*
 * usbd_xfer_schedule_timeout(xfer)
 *
 *	Ensure that xfer has a timeout.  If the callout is already
 *	queued or the task is already running, request that they
 *	reschedule the callout.  If not, and if we're not polling,
 *	schedule the callout anew.
 *
 *	To be called in thread context from struct
 *	usbd_pipe_methods::upm_start.
 */
void
usbd_xfer_schedule_timeout(struct usbd_xfer *xfer)
{
	struct usbd_bus *bus = xfer->ux_bus;

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));

	if (xfer->ux_timeout_set) {
		/*
		 * Callout or task has fired from a prior completed
		 * xfer but has not yet noticed that the xfer is done.
		 * Ask it to reschedule itself to ux_timeout.
		 */
		xfer->ux_timeout_reset = true;
	} else if (xfer->ux_timeout && !bus->ub_usepolling) {
		/* Callout is not scheduled.  Schedule it.  */
		KASSERT(!callout_pending(&xfer->ux_callout));
		callout_schedule(&xfer->ux_callout, mstohz(xfer->ux_timeout));
		xfer->ux_timeout_set = true;
	}

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));
}

/*
 * usbd_xfer_cancel_timeout_async(xfer)
 *
 *	Cancel the callout and the task of xfer, which have not yet run
 *	to completion, but don't wait for the callout or task to finish
 *	running.
 *
 *	If they have already fired, at worst they are waiting for the
 *	bus lock.  They will see that the xfer is no longer in progress
 *	and give up, or they will see that the xfer has been
 *	resubmitted with a new timeout and reschedule the callout.
 *
 *	If a resubmitted request completed so fast that the callout
 *	didn't have time to process a timer reset, just cancel the
 *	timer reset.
 */
static void
usbd_xfer_cancel_timeout_async(struct usbd_xfer *xfer)
{
	struct usbd_bus *bus __diagused = xfer->ux_bus;

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));

	/*
	 * If the timer wasn't running anyway, forget about it.  This
	 * can happen if we are completing an isochronous transfer
	 * which doesn't use the same timeout logic.
	 */
	if (!xfer->ux_timeout_set)
		return;

	xfer->ux_timeout_reset = false;
	if (!callout_stop(&xfer->ux_callout)) {
		/*
		 * We stopped the callout before it ran.  The timeout
		 * is no longer set.
		 */
		xfer->ux_timeout_set = false;
	} else if (callout_invoking(&xfer->ux_callout)) {
		/*
		 * The callout has begun to run but it has not yet
		 * acquired the lock and called callout_ack.  The task
		 * cannot be queued yet, and the callout cannot have
		 * been rescheduled yet.
		 *
		 * By the time the callout acquires the lock, we will
		 * have transitioned from USBD_IN_PROGRESS to a
		 * completed status, and possibly also resubmitted the
		 * xfer and set xfer->ux_timeout_reset = true.  In both
		 * cases, the callout will DTRT, so no further action
		 * is needed here.
		 */
	} else if (usb_rem_task(xfer->ux_pipe->up_dev, &xfer->ux_aborttask)) {
		/*
		 * The callout had fired and scheduled the task, but we
		 * stopped the task before it could run.  The timeout
		 * is therefore no longer set -- the next resubmission
		 * of the xfer must schedule a new timeout.
		 *
		 * The callout should not be pending at this point:
		 * it is scheduled only under the lock, and only when
		 * xfer->ux_timeout_set is false, or by the callout or
		 * task itself when xfer->ux_timeout_reset is true.
		 */
		xfer->ux_timeout_set = false;
	}

	/*
	 * The callout cannot be scheduled and the task cannot be
	 * queued at this point.  Either we cancelled them, or they are
	 * already running and waiting for the bus lock.
	 */
	KASSERT(!callout_pending(&xfer->ux_callout));
	KASSERT(!usb_task_pending(xfer->ux_pipe->up_dev, &xfer->ux_aborttask));

	KASSERT(bus->ub_usepolling || mutex_owned(bus->ub_lock));
}
