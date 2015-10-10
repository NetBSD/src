/*	$NetBSD: usbdi.c,v 1.162.2.30 2015/10/10 07:23:25 skrll Exp $	*/

/*
 * Copyright (c) 1998, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna.com.au).
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
__KERNEL_RCSID(0, "$NetBSD: usbdi.c,v 1.162.2.30 2015/10/10 07:23:25 skrll Exp $");

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
#include <dev/usb/usbhist.h>

/* UTF-8 encoding stuff */
#include <fs/unicode.h>

extern int usbdebug;

Static usbd_status usbd_ar_pipe(struct usbd_pipe *);
Static void usbd_start_next(struct usbd_pipe *);
Static usbd_status usbd_open_pipe_ival
	(struct usbd_interface *, uint8_t, uint8_t, struct usbd_pipe **, int);
static void *usbd_alloc_buffer(struct usbd_xfer *, uint32_t);
static void usbd_free_buffer(struct usbd_xfer *);
static struct usbd_xfer *usbd_alloc_xfer(struct usbd_device *);
static usbd_status usbd_free_xfer(struct usbd_xfer *);

#if defined(USB_DEBUG)
void
usbd_dump_iface(struct usbd_interface *iface)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "iface %p", iface, 0, 0, 0);
	if (iface == NULL)
		return;
	USBHIST_LOG(usbdebug, "     device = %p idesc = %p index = %d",
	    iface->ui_dev, iface->ui_idesc, iface->ui_index, 0);
	USBHIST_LOG(usbdebug, "     altindex=%d priv=%p",
	    iface->ui_altindex, iface->ui_priv, 0, 0);
}

void
usbd_dump_device(struct usbd_device *dev)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "dev = %p", dev, 0, 0, 0);
	if (dev == NULL)
		return;
	USBHIST_LOG(usbdebug, "     bus = %p default_pipe = %p",
	    dev->ud_bus, dev->ud_pipe0, 0, 0);
	USBHIST_LOG(usbdebug, "     address = %d config = %d depth = %d ",
	    dev->ud_addr, dev->ud_config, dev->ud_depth, 0);
	USBHIST_LOG(usbdebug, "     speed = %d self_powered = %d "
	    "power = %d langid = %d",
	    dev->ud_speed, dev->ud_selfpowered, dev->ud_power, dev->ud_langid);
}

void
usbd_dump_endpoint(struct usbd_endpoint *endp)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "endp = %p", endp, 0, 0, 0);
	if (endp == NULL)
		return;
	USBHIST_LOG(usbdebug, "    edesc = %p refcnt = %d",
	    endp->ue_edesc, endp->ue_refcnt, 0, 0);
	if (endp->ue_edesc)
		USBHIST_LOG(usbdebug, "     bEndpointAddress=0x%02x",
		    endp->ue_edesc->bEndpointAddress, 0, 0, 0);
}

void
usbd_dump_queue(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "pipe = %p", pipe, 0, 0, 0);
	SIMPLEQ_FOREACH(xfer, &pipe->up_queue, ux_next) {
		USBHIST_LOG(usbdebug, "     xfer = %p", xfer, 0, 0, 0);
	}
}

void
usbd_dump_pipe(struct usbd_pipe *pipe)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "pipe = %p", pipe, 0, 0, 0);
	if (pipe == NULL)
		return;
	usbd_dump_iface(pipe->up_iface);
	usbd_dump_device(pipe->up_dev);
	usbd_dump_endpoint(pipe->up_endpoint);
	USBHIST_LOG(usbdebug, "(usbd_dump_pipe)", 0, 0, 0, 0);
	USBHIST_LOG(usbdebug, "     refcnt = %d running = %d aborting = %d",
	    pipe->up_refcnt, pipe->up_running, pipe->up_aborting, 0);
	USBHIST_LOG(usbdebug, "     intrxfer = %p, repeat = %d, interval = %d",
	    pipe->up_intrxfer, pipe->up_repeat, pipe->up_interval, 0);
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
	struct usbd_pipe *p;
	struct usbd_endpoint *ep;
	usbd_status err;
	int i;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "iface = %p address = 0x%x flags = 0x%x",
	    iface, address, flags, 0);

	for (i = 0; i < iface->ui_idesc->bNumEndpoints; i++) {
		ep = &iface->ui_endpoints[i];
		if (ep->ue_edesc == NULL)
			return USBD_IOERROR;
		if (ep->ue_edesc->bEndpointAddress == address)
			goto found;
	}
	return USBD_BAD_ADDRESS;
 found:
	if ((flags & USBD_EXCLUSIVE_USE) && ep->ue_refcnt != 0)
		return USBD_IN_USE;
	err = usbd_setup_pipe_flags(iface->ui_dev, iface, ep, ival, &p, flags);
	if (err)
		return err;
	LIST_INSERT_HEAD(&iface->ui_pipes, p, up_next);
	*pipe = p;
	return USBD_NORMAL_COMPLETION;
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

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "address = 0x%x flags = 0x%x len = %d",
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
	return USBD_NORMAL_COMPLETION;

 bad3:
	ipipe->up_intrxfer = NULL;
	ipipe->up_repeat = 0;

	usbd_destroy_xfer(xfer);
 bad1:
	usbd_close_pipe(ipipe);
	return err;
}

usbd_status
usbd_close_pipe(struct usbd_pipe *pipe)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(pipe != NULL);
#ifdef DIAGNOSTIC
	if (pipe == NULL) {
		USBHIST_LOG(usbdebug, "pipe == NULL", 0, 0, 0, 0);
		return USBD_NORMAL_COMPLETION;
	}
#endif

	usbd_lock_pipe(pipe);
	if (--pipe->up_refcnt != 0) {
		usbd_unlock_pipe(pipe);
		return USBD_NORMAL_COMPLETION;
	}
	if (! SIMPLEQ_EMPTY(&pipe->up_queue)) {
		usbd_unlock_pipe(pipe);
		return USBD_PENDING_REQUESTS;
	}
	LIST_REMOVE(pipe, up_next);
	pipe->up_endpoint->ue_refcnt--;
	pipe->up_methods->upm_close(pipe);
	usbd_unlock_pipe(pipe);
	if (pipe->up_intrxfer != NULL)
		usbd_free_xfer(pipe->up_intrxfer);
	kmem_free(pipe, pipe->up_dev->ud_bus->ub_pipesize);
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_transfer(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usbd_status err;
	unsigned int size, flags;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug,
	    "xfer = %p, flags = %#x, pipe = %p, running = %d",
	    xfer, xfer->ux_flags, pipe, pipe->up_running);

#ifdef USB_DEBUG
	if (usbdebug > 5)
		usbd_dump_queue(pipe);
#endif
	xfer->ux_done = 0;

	if (pipe->up_aborting) {
		USBHIST_LOG(usbdebug, "<- done xfer %p, aborting", xfer, 0, 0,
		    0);
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
	err = pipe->up_methods->upm_transfer(xfer);
	USBHIST_LOG(usbdebug, "<- done transfer %p, err = %d", xfer, err, 0, 0);

	if (!(flags & USBD_SYNCHRONOUS)) {
		USBHIST_LOG(usbdebug, "<- done xfer %p, not sync", xfer, 0, 0,
		    0);
		return err;
	}

	/* Sync transfer, wait for completion. */
	if (err != USBD_IN_PROGRESS) {
		USBHIST_LOG(usbdebug, "<- done xfer %p, err %d (complete/error)", xfer,
		    err, 0, 0);
		return err;
	}
	usbd_lock_pipe(pipe);
	while (!xfer->ux_done) {
		if (pipe->up_dev->ud_bus->ub_usepolling)
			panic("usbd_transfer: not done");
		USBHIST_LOG(usbdebug, "<- sleeping on xfer %p", xfer, 0, 0, 0);

		err = 0;
		if ((flags & USBD_SYNCHRONOUS_SIG) != 0) {
			err = cv_wait_sig(&xfer->ux_cv, pipe->up_dev->ud_bus->ub_lock);
		} else {
			cv_wait(&xfer->ux_cv, pipe->up_dev->ud_bus->ub_lock);
		}
		if (err) {
			if (!xfer->ux_done)
				pipe->up_methods->upm_abort(xfer);
			break;
		}
	}
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
	struct usbd_bus *bus = xfer->ux_dev->ud_bus;

	if (bus->ub_usedma) {
		usb_dma_t *dmap = &xfer->ux_dmabuf;

		int err = usb_allocmem_flags(bus, size, 0, dmap, bus->ub_dmaflags);
		if (err) {
			return NULL;
		}
		xfer->ux_buf = KERNADDR(&xfer->ux_dmabuf, 0);
		xfer->ux_bufsize = size;

		return xfer->ux_buf;
	}
#endif
	KASSERT(xfer->ux_dev->ud_bus->ub_usedma == false);
	xfer->ux_buf = kmem_alloc(size, KM_SLEEP);

	if (xfer->ux_buf != NULL) {
		xfer->ux_bufsize = size;
	}

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
	struct usbd_bus *bus = xfer->ux_dev->ud_bus;

	if (bus->ub_usedma) {
		usb_dma_t *dmap = &xfer->ux_dmabuf;

		usb_freemem(bus, dmap);
		return;
	}
#endif
	KASSERT(xfer->ux_dev->ud_bus->ub_usedma == false);

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
usbd_alloc_xfer(struct usbd_device *dev)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	ASSERT_SLEEPABLE();

	xfer = dev->ud_bus->ub_methods->ubm_allocx(dev->ud_bus);
	if (xfer == NULL)
		return NULL;
	xfer->ux_dev = dev;
	callout_init(&xfer->ux_callout, CALLOUT_MPSAFE);
	cv_init(&xfer->ux_cv, "usbxfer");
	cv_init(&xfer->ux_hccv, "usbhcxfer");

	USBHIST_LOG(usbdebug, "returns %p", xfer, 0, 0, 0);

	return xfer;
}

static usbd_status
usbd_free_xfer(struct usbd_xfer *xfer)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "%p", xfer, 0, 0, 0);
	if (xfer->ux_buf) {
		usbd_free_buffer(xfer);
	}
#if defined(DIAGNOSTIC)
	if (callout_pending(&xfer->ux_callout)) {
		callout_stop(&xfer->ux_callout);
		printf("usbd_free_xfer: timeout_handle pending\n");
	}
#endif
	cv_destroy(&xfer->ux_cv);
	cv_destroy(&xfer->ux_hccv);
	xfer->ux_dev->ud_bus->ub_methods->ubm_freex(xfer->ux_dev->ud_bus, xfer);
	return USBD_NORMAL_COMPLETION;
}

int
usbd_create_xfer(struct usbd_pipe *pipe, size_t len, unsigned int flags,
    unsigned int nframes, struct usbd_xfer **xp)
{
	KASSERT(xp != NULL);

	struct usbd_xfer *xfer = usbd_alloc_xfer(pipe->up_dev);
	if (xfer == NULL)
		return ENOMEM;

	if (len) {
		void *buf = usbd_alloc_buffer(xfer, len);
		if (!buf) {
			usbd_free_xfer(xfer);
			return ENOMEM;
		}
	}
	xfer->ux_pipe = pipe;
	xfer->ux_flags = flags;
	xfer->ux_nframes = nframes;

	*xp = xfer;
	return 0;
}

void usbd_destroy_xfer(struct usbd_xfer *xfer)
{

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
	xfer->ux_pipe = dev->ud_pipe0;
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
usbd_status
usbd_abort_default_pipe(struct usbd_device *device)
{
	return usbd_abort_pipe(device->ud_pipe0);
}

usbd_status
usbd_abort_pipe(struct usbd_pipe *pipe)
{
	usbd_status err;

	KASSERT(pipe != NULL);

	usbd_lock_pipe(pipe);
	err = usbd_ar_pipe(pipe);
	usbd_unlock_pipe(pipe);
	return err;
}

usbd_status
usbd_clear_endpoint_stall(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	usb_device_request_t req;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	/*
	 * Clearing en endpoint stall resets the endpoint toggle, so
	 * do the same to the HC toggle.
	 */
	pipe->up_methods->upm_cleartoggle(pipe);

	req.bmRequestType = UT_WRITE_ENDPOINT;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, UF_ENDPOINT_HALT);
	USETW(req.wIndex, pipe->up_endpoint->ue_edesc->bEndpointAddress);
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
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
	usb_device_request_t req;

	pipe->up_methods->upm_cleartoggle(pipe);

	req.bmRequestType = UT_WRITE_ENDPOINT;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, UF_ENDPOINT_HALT);
	USETW(req.wIndex, pipe->up_endpoint->ue_edesc->bEndpointAddress);
	USETW(req.wLength, 0);
	(void)usbd_do_request(dev, &req, 0);
}

void
usbd_clear_endpoint_stall_async(struct usbd_pipe *pipe)
{
	usb_add_task(pipe->up_dev, &pipe->up_async_task, USB_TASKQ_DRIVER);
}

void
usbd_clear_endpoint_toggle(struct usbd_pipe *pipe)
{
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
	usb_device_request_t req;
	usbd_status err;
	void *endpoints;

	if (LIST_FIRST(&iface->ui_pipes) != NULL)
		return USBD_IN_USE;

	endpoints = iface->ui_endpoints;
	int nendpt = iface->ui_idesc->bNumEndpoints;
	err = usbd_fill_iface_data(iface->ui_dev, iface->ui_index, altidx);
	if (err)
		return err;

	/* new setting works, we can free old endpoints */
	if (endpoints != NULL) {
		kmem_free(endpoints, nendpt * sizeof(struct usbd_endpoint));
	}
	KASSERT(iface->ui_idesc != NULL);

	req.bmRequestType = UT_WRITE_INTERFACE;
	req.bRequest = UR_SET_INTERFACE;
	USETW(req.wValue, iface->ui_idesc->bAlternateSetting);
	USETW(req.wIndex, iface->ui_idesc->bInterfaceNumber);
	USETW(req.wLength, 0);
	return usbd_do_request(iface->ui_dev, &req, 0);
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

/* Dequeue all pipe operations, called at splusb(). */
Static usbd_status
usbd_ar_pipe(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));

	USBHIST_LOG(usbdebug, "pipe = %p", pipe, 0, 0, 0);
#ifdef USB_DEBUG
	if (usbdebug > 5)
		usbd_dump_queue(pipe);
#endif
	pipe->up_repeat = 0;
	pipe->up_aborting = 1;
	while ((xfer = SIMPLEQ_FIRST(&pipe->up_queue)) != NULL) {
		USBHIST_LOG(usbdebug, "pipe = %p xfer = %p (methods = %p)",
		    pipe, xfer, pipe->up_methods, 0);
		/* Make the HC abort it (and invoke the callback). */
		pipe->up_methods->upm_abort(xfer);
		/* XXX only for non-0 usbd_clear_endpoint_stall(pipe); */
	}
	pipe->up_aborting = 0;
	return USBD_NORMAL_COMPLETION;
}

/* Called with USB lock held. */
void
usb_transfer_complete(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
 	struct usbd_bus *bus = pipe->up_dev->ud_bus;
	int sync = xfer->ux_flags & USBD_SYNCHRONOUS;
	int erred = xfer->ux_status == USBD_CANCELLED ||
	    xfer->ux_status == USBD_TIMEOUT;
	int polling = bus->ub_usepolling;
	int repeat;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "pipe = %p xfer = %p status = %d actlen = %d",
		pipe, xfer, xfer->ux_status, xfer->ux_actlen);

	KASSERT(polling || mutex_owned(pipe->up_dev->ud_bus->ub_lock));
	KASSERT(xfer->ux_state == XFER_ONQU);
	KASSERT(pipe != NULL);

	repeat = pipe->up_repeat;
	/* XXXX */
	if (polling)
		pipe->up_running = 0;

	if (xfer->ux_length != 0 && xfer->ux_buffer != xfer->ux_buf) {
		KDASSERTMSG(xfer->ux_actlen <= xfer->ux_length,
		    "actlen %d length %d",xfer->ux_actlen, xfer->ux_length);

		/* Only if IN transfer */
		if (usbd_xfer_isread(xfer)) {
			memcpy(xfer->ux_buffer, xfer->ux_buf, xfer->ux_actlen);
		}
	}

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
	USBHIST_LOG(usbdebug, "xfer %p: repeat %d new head = %p",
	    xfer, repeat, SIMPLEQ_FIRST(&pipe->up_queue), 0);

	/* Count completed transfers. */
	++pipe->up_dev->ud_bus->ub_stats.uds_requests
		[pipe->up_endpoint->ue_edesc->bmAttributes & UE_XFERTYPE];

	xfer->ux_done = 1;
	if (!xfer->ux_status && xfer->ux_actlen < xfer->ux_length &&
	    !(xfer->ux_flags & USBD_SHORT_XFER_OK)) {
		USBHIST_LOG(usbdebug, "short transfer %d < %d",
		    xfer->ux_actlen, xfer->ux_length, 0, 0);
		xfer->ux_status = USBD_SHORT_XFER;
	}

	if (repeat) {
		USBHIST_LOG(usbdebug, "xfer %p doing callback %p status %x",
		    xfer, xfer->ux_callback, xfer->ux_status, 0);
		if (xfer->ux_callback) {
			if (!polling)
				mutex_exit(pipe->up_dev->ud_bus->ub_lock);

			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_LOCK(1, curlwp);
			xfer->ux_callback(xfer, xfer->ux_priv, xfer->ux_status);
			USBHIST_LOG(usbdebug, "xfer %p doing done %p", xfer,
			    pipe->up_methods->upm_done, 0, 0);
			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_UNLOCK_ONE(curlwp);

			if (!polling)
				mutex_enter(pipe->up_dev->ud_bus->ub_lock);
		}
		pipe->up_methods->upm_done(xfer);
	} else {
		USBHIST_LOG(usbdebug, "xfer %p doing done %p", xfer,
		    pipe->up_methods->upm_done, 0, 0);
		pipe->up_methods->upm_done(xfer);
		USBHIST_LOG(usbdebug, "xfer %p doing callback %p status %x",
		    xfer, xfer->ux_callback, xfer->ux_status, 0);
		if (xfer->ux_callback) {
			if (!polling)
				mutex_exit(pipe->up_dev->ud_bus->ub_lock);

			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_LOCK(1, curlwp);
			xfer->ux_callback(xfer, xfer->ux_priv, xfer->ux_status);
			if (!(pipe->up_flags & USBD_MPSAFE))
				KERNEL_UNLOCK_ONE(curlwp);

			if (!polling)
				mutex_enter(pipe->up_dev->ud_bus->ub_lock);
		}
	}

	if (sync && !polling) {
		USBHIST_LOG(usbdebug, "<- done xfer %p, wakeup", xfer, 0, 0, 0);
		cv_broadcast(&xfer->ux_cv);
	}

	if (!repeat) {
		/* XXX should we stop the queue on all errors? */
		if (erred && pipe->up_iface != NULL)	/* not control pipe */
			pipe->up_running = 0;
		else
			usbd_start_next(pipe);
	}
}

/* Called with USB lock held. */
usbd_status
usb_insert_transfer(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	USBHIST_LOG(usbdebug, "pipe = %p running = %d timeout = %d",
	    pipe, pipe->up_running, xfer->ux_timeout, 0);

	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));
	KASSERT(xfer->ux_state == XFER_BUSY);

#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_ONQU;
#endif
	SIMPLEQ_INSERT_TAIL(&pipe->up_queue, xfer, ux_next);
	if (pipe->up_running)
		err = USBD_IN_PROGRESS;
	else {
		pipe->up_running = 1;
		err = USBD_NORMAL_COMPLETION;
	}
	USBHIST_LOG(usbdebug, "<- done xfer %p, err %d", xfer, err, 0, 0);
	return err;
}

/* Called with USB lock held. */
void
usbd_start_next(struct usbd_pipe *pipe)
{
	struct usbd_xfer *xfer;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(pipe != NULL);
	KASSERT(pipe->up_methods != NULL);
	KASSERT(pipe->up_methods->upm_start != NULL);
	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));

	/* Get next request in queue. */
	xfer = SIMPLEQ_FIRST(&pipe->up_queue);
	USBHIST_LOG(usbdebug, "pipe = %p, xfer = %p", pipe, xfer, 0, 0);
	if (xfer == NULL) {
		pipe->up_running = 0;
	} else {
		mutex_exit(pipe->up_dev->ud_bus->ub_lock);
		err = pipe->up_methods->upm_start(xfer);
		mutex_enter(pipe->up_dev->ud_bus->ub_lock);

		if (err != USBD_IN_PROGRESS) {
			USBHIST_LOG(usbdebug, "error = %d", err, 0, 0, 0);
			pipe->up_running = 0;
			/* XXX do what? */
		}
	}

	KASSERT(mutex_owned(pipe->up_dev->ud_bus->ub_lock));
}

usbd_status
usbd_do_request(struct usbd_device *dev, usb_device_request_t *req, void *data)
{
	return (usbd_do_request_flags(dev, req, data, 0, 0,
				      USBD_DEFAULT_TIMEOUT));
}

usbd_status
usbd_do_request_flags(struct usbd_device *dev, usb_device_request_t *req,
		      void *data, uint16_t flags, int *actlen, uint32_t timo)
{
	return (usbd_do_request_flags_pipe(dev, dev->ud_pipe0, req,
					   data, flags, actlen, timo));
}

usbd_status
usbd_do_request_flags_pipe(struct usbd_device *dev, struct usbd_pipe *pipe,
	usb_device_request_t *req, void *data, uint16_t flags, int *actlen,
	uint32_t timeout)
{
	struct usbd_xfer *xfer;
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	ASSERT_SLEEPABLE();

	xfer = usbd_alloc_xfer(dev);
	if (xfer == NULL)
		return USBD_NOMEM;

	if (UGETW(req->wLength) != 0) {
		void *buf = usbd_alloc_buffer(xfer, UGETW(req->wLength));
		if (buf == NULL) {
			err = USBD_NOMEM;
			goto bad;
		}
	}

	usbd_setup_default_xfer(xfer, dev, 0, timeout, req,
				data, UGETW(req->wLength), flags, 0);
	xfer->ux_pipe = pipe;
	err = usbd_sync_transfer(xfer);
#if defined(USB_DEBUG) || defined(DIAGNOSTIC)
	if (xfer->ux_actlen > xfer->ux_length) {
		USBHIST_LOG(usbdebug, "overrun addr = %d type = 0x%02x",
		    dev->ud_addr, xfer->ux_request.bmRequestType, 0, 0);
		USBHIST_LOG(usbdebug, "     req = 0x%02x val = %d index = %d",
		    xfer->ux_request.bRequest, UGETW(xfer->ux_request.wValue),
		    UGETW(xfer->ux_request.wIndex), 0);
		USBHIST_LOG(usbdebug, "     rlen = %d length = %d actlen = %d",
		    UGETW(xfer->ux_request.wLength),
		    xfer->ux_length, xfer->ux_actlen, 0);
	}
#endif
	if (actlen != NULL)
		*actlen = xfer->ux_actlen;

 bad:
	if (err) {
		USBHIST_LOG(usbdebug, "returning err = %d", err, 0, 0, 0);
	}
	usbd_free_xfer(xfer);
	return err;
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
 * XXX use this more???  ub_usepolling it touched manually all over
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


void
usb_desc_iter_init(struct usbd_device *dev, usbd_desc_iter_t *iter)
{
	const usb_config_descriptor_t *cd = usbd_get_config_descriptor(dev);

	iter->cur = (const uByte *)cd;
	iter->end = (const uByte *)cd + UGETW(cd->wTotalLength);
}

const usb_descriptor_t *
usb_desc_iter_next(usbd_desc_iter_t *iter)
{
	const usb_descriptor_t *desc;

	if (iter->cur + sizeof(usb_descriptor_t) >= iter->end) {
		if (iter->cur != iter->end)
			printf("usb_desc_iter_next: bad descriptor\n");
		return NULL;
	}
	desc = (const usb_descriptor_t *)iter->cur;
	if (desc->bLength == 0) {
		printf("usb_desc_iter_next: descriptor length = 0\n");
		return NULL;
	}
	iter->cur += desc->bLength;
	if (iter->cur > iter->end) {
		printf("usb_desc_iter_next: descriptor length too large\n");
		return NULL;
	}
	return desc;
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
