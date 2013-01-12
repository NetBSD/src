/*	$NetBSD: dwc_otg.c,v 1.18 2013/01/12 22:42:49 skrll Exp $	*/

/*-
 * Copyright (c) 2012 Hans Petter Selasky. All rights reserved.
 * Copyright (c) 2010-2011 Aleksandr Rybalko. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

/*
 * Designware USB 2.0 OTG
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwc_otg.c,v 1.18 2013/01/12 22:42:49 skrll Exp $");

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

#include <dev/usb/dwc_otgreg.h>
#include <dev/usb/dwc_otgvar.h>

#include <dev/usb/usbroothub_subr.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_mbox.h>

#ifdef DOTG_COUNTERS
#define	DOTG_EVCNT_ADD(a,b)	((void)((a).ev_count += (b)))
#else
#define	DOTG_EVCNT_ADD(a,b)	do { } while (/*CONSTCOND*/0)
#endif
#define	DOTG_EVCNT_INCR(a)	DOTG_EVCNT_ADD((a), 1)

#ifdef DWC_OTG_DEBUG
#define	DPRINTFN(n,fmt,...) do {			\
	if (dwc_otgdebug >= (n)) {			\
		printf("%s: " fmt,			\
		__FUNCTION__,## __VA_ARGS__);		\
	}						\
} while (0)
#define	DPRINTF(...)	DPRINTFN(1, __VA_ARGS__)
int dwc_otgdebug = 0;
#else
#define	DPRINTF(...) do { } while (0)
#define	DPRINTFN(...) do { } while (0)
#endif

#define	DWC_OTG_MSK_GINT_ENABLED	\
   (GINTMSK_ENUMDONEMSK |		\
   GINTMSK_USBRSTMSK |			\
   GINTMSK_USBSUSPMSK |			\
   GINTMSK_IEPINTMSK |			\
   GINTMSK_RXFLVLMSK |			\
   GINTMSK_SESSREQINTMSK |		\
   GINTMSK_OTGINTMSK |			\
   GINTMSK_HCHINTMSK |			\
   GINTMSK_PRTINTMSK)

#define	DWC_OTG_BUS2SC(bus)	((bus)->hci_private)

#define	DWC_OTG_XFER2SC(xfer)	DWC_OTG_BUS2SC((xfer)->pipe->device->bus)

#define	DWC_OTG_TD2SC(td)	DWC_OTG_XFER2SC((td)->xfer)

#define	DWC_OTG_DPIPE2SC(d) \
    DWC_OTG_BUS2SC((d)->pipe.device->bus)

#define	DWC_OTG_XFER2DXFER(x) (struct dwc_otg_xfer *)(x)

#define	DWC_OTG_XFER2DPIPE(x) (struct dwc_otg_pipe *)(x)->pipe;

#define usbd_copy_in(d, o, b, s) \
    memcpy(((char *)(d) + (o)), (b), (s))

#define usbd_copy_out(d, o, b, s) \
    memcpy((b), ((char *)(d) + (o)), (s))

struct dwc_otg_pipe;

Static usbd_status	dwc_otg_open(usbd_pipe_handle);
Static void		dwc_otg_poll(struct usbd_bus *);
Static void		dwc_otg_softintr(void *);
Static void		dwc_otg_waitintr(struct dwc_otg_softc *, usbd_xfer_handle);

Static usbd_status	dwc_otg_allocm(struct usbd_bus *, usb_dma_t *, uint32_t);
Static void		dwc_otg_freem(struct usbd_bus *, usb_dma_t *);

Static usbd_xfer_handle	dwc_otg_allocx(struct usbd_bus *);
Static void		dwc_otg_freex(struct usbd_bus *, usbd_xfer_handle);
Static void		dwc_otg_get_lock(struct usbd_bus *, kmutex_t **);

#if 0
Static usbd_status	dwc_otg_setup_isoc(usbd_pipe_handle pipe);
Static void		dwc_otg_device_isoc_enter(usbd_xfer_handle);
#endif

Static usbd_status	dwc_otg_root_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_root_ctrl_start(usbd_xfer_handle);
Static void		dwc_otg_root_ctrl_abort(usbd_xfer_handle);
Static void		dwc_otg_root_ctrl_close(usbd_pipe_handle);
Static void		dwc_otg_root_ctrl_done(usbd_xfer_handle);

Static usbd_status	dwc_otg_root_intr_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_root_intr_start(usbd_xfer_handle);
Static void		dwc_otg_root_intr_abort(usbd_xfer_handle);
Static void		dwc_otg_root_intr_close(usbd_pipe_handle);
Static void		dwc_otg_root_intr_done(usbd_xfer_handle);

Static usbd_status	dwc_otg_device_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_device_ctrl_start(usbd_xfer_handle);
Static void		dwc_otg_device_ctrl_abort(usbd_xfer_handle);
Static void		dwc_otg_device_ctrl_close(usbd_pipe_handle);
Static void		dwc_otg_device_ctrl_done(usbd_xfer_handle);

Static usbd_status	dwc_otg_device_bulk_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_device_bulk_start(usbd_xfer_handle);
Static void		dwc_otg_device_bulk_abort(usbd_xfer_handle);
Static void		dwc_otg_device_bulk_close(usbd_pipe_handle);
Static void		dwc_otg_device_bulk_done(usbd_xfer_handle);

Static usbd_status	dwc_otg_device_intr_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_device_intr_start(usbd_xfer_handle);
Static void		dwc_otg_device_intr_abort(usbd_xfer_handle);
Static void		dwc_otg_device_intr_close(usbd_pipe_handle);
Static void		dwc_otg_device_intr_done(usbd_xfer_handle);

Static usbd_status	dwc_otg_device_isoc_transfer(usbd_xfer_handle);
Static usbd_status	dwc_otg_device_isoc_start(usbd_xfer_handle);
Static void		dwc_otg_device_isoc_abort(usbd_xfer_handle);
Static void		dwc_otg_device_isoc_close(usbd_pipe_handle);
Static void		dwc_otg_device_isoc_done(usbd_xfer_handle);

#if 0
Static void		dwc_otg_close_pipe(usbd_pipe_handle, dwc_otg_soft_ed_t *);
#endif
Static void		dwc_otg_abort_xfer(usbd_xfer_handle, usbd_status);

Static void		dwc_otg_device_clear_toggle(usbd_pipe_handle pipe);
Static void		dwc_otg_noop(usbd_pipe_handle pipe);

#ifdef DWC_OTG_DEBUG
Static void		dwc_otg_dump_global_regs(struct dwc_otg_softc *);
Static void		dwc_otg_dump_host_regs(struct dwc_otg_softc *);
#endif

Static void		dwc_otg_setup_data_chain(usbd_xfer_handle);
Static void		dwc_otg_setup_ctrl_chain(usbd_xfer_handle);
Static void		dwc_otg_setup_intr_chain(usbd_xfer_handle);
Static void		dwc_otg_setup_bulk_chain(usbd_xfer_handle);
//Static void		dwc_otg_setup_isoc_chain(usbd_xfer_handle);

Static void		dwc_otg_timeout(void *);
Static void		dwc_otg_timeout_task(void *);

Static void		dwc_otg_xfer_setup(usbd_xfer_handle);
Static void		dwc_otg_xfer_start(usbd_xfer_handle);
Static void		dwc_otg_xfer_end(usbd_xfer_handle);

// static dwc_otg_cmd_t dwc_otg_setup_rx;
// static dwc_otg_cmd_t dwc_otg_data_rx;
// static dwc_otg_cmd_t dwc_otg_data_tx;
// static dwc_otg_cmd_t dwc_otg_data_tx_sync;

static dwc_otg_cmd_t	dwc_otg_host_setup_tx;
static dwc_otg_cmd_t	dwc_otg_host_data_tx;
static dwc_otg_cmd_t	dwc_otg_host_data_rx;

static int		dwc_otg_init_fifo(struct dwc_otg_softc *, uint8_t);
Static void 		dwc_otg_clocks_on(struct dwc_otg_softc*);
Static void	 	dwc_otg_clocks_off(struct dwc_otg_softc*);
Static void		dwc_otg_pull_up(struct dwc_otg_softc *);
Static void		dwc_otg_pull_down(struct dwc_otg_softc *);
Static void		dwc_otg_enable_sof_irq(struct dwc_otg_softc *);
Static void		dwc_otg_resume_irq(struct dwc_otg_softc *);
Static void		dwc_otg_suspend_irq(struct dwc_otg_softc *);
Static void		dwc_otg_wakeup_peer(struct dwc_otg_softc *);
Static int		dwc_otg_interrupt(struct dwc_otg_softc *);
Static void		dwc_otg_timer(struct dwc_otg_softc *);
Static void		dwc_otg_timer_tick(void *);
Static void		dwc_otg_timer_start(struct dwc_otg_softc *);
Static void		dwc_otg_timer_stop(struct dwc_otg_softc *);
Static void		dwc_otg_interrupt_poll(struct dwc_otg_softc *);
Static void		dwc_otg_do_poll(struct usbd_bus *);
Static void		dwc_otg_worker(struct work *, void *);
Static void		dwc_otg_rhc(void *);
Static void		dwc_otg_vbus_interrupt(struct dwc_otg_softc *);
Static void		dwc_otg_standard_done(usbd_xfer_handle);
Static usbd_status	dwc_otg_standard_done_sub(usbd_xfer_handle);
Static void		dwc_otg_device_done(usbd_xfer_handle, usbd_status);
Static void		dwc_otg_setup_standard_chain(usbd_xfer_handle);
Static void		dwc_otg_start_standard_chain(usbd_xfer_handle);

Static void dwc_otg_core_reset(struct dwc_otg_softc *sc);

static inline void
dwc_otg_root_intr(struct dwc_otg_softc *sc)
{

	softint_schedule(sc->sc_rhc_si);
}

#define DWC_OTG_READ_4(sc, reg) \
    bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define DWC_OTG_WRITE_4(sc, reg, data)  \
    bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (data));

#define DWC_OTG_MODIFY_4(sc, reg, off, on) \
  DWC_OTG_WRITE_4((sc),(reg),(DWC_OTG_READ_4((sc),(reg)) & ~(off)) | (on))


struct dwc_otg_pipe {
	struct usbd_pipe pipe;		/* Must be first */
};

#define DWC_OTG_INTR_ENDPT 1

Static const struct usbd_bus_methods dwc_otg_bus_methods = {
	.open_pipe =	dwc_otg_open,
	.soft_intr =	dwc_otg_softintr,
	.do_poll =	dwc_otg_poll,
	.allocm =	dwc_otg_allocm,
	.freem =	dwc_otg_freem,
	.allocx =	dwc_otg_allocx,
	.freex =	dwc_otg_freex,
	.get_lock =	dwc_otg_get_lock,
};

Static const struct usbd_pipe_methods dwc_otg_root_ctrl_methods = {
	.transfer =	dwc_otg_root_ctrl_transfer,
	.start =	dwc_otg_root_ctrl_start,
	.abort =	dwc_otg_root_ctrl_abort,
	.close =	dwc_otg_root_ctrl_close,
	.cleartoggle =	dwc_otg_noop,
	.done =		dwc_otg_root_ctrl_done,
};

Static const struct usbd_pipe_methods dwc_otg_root_intr_methods = {
	.transfer =	dwc_otg_root_intr_transfer,
	.start =	dwc_otg_root_intr_start,
	.abort =	dwc_otg_root_intr_abort,
	.close =	dwc_otg_root_intr_close,
	.cleartoggle =	dwc_otg_noop,
	.done =		dwc_otg_root_intr_done,
};

Static const struct usbd_pipe_methods dwc_otg_device_ctrl_methods = {
	.transfer =	dwc_otg_device_ctrl_transfer,
	.start =	dwc_otg_device_ctrl_start,
	.abort =	dwc_otg_device_ctrl_abort,
	.close =	dwc_otg_device_ctrl_close,
	.cleartoggle =	dwc_otg_noop,
	.done =		dwc_otg_device_ctrl_done,
};

Static const struct usbd_pipe_methods dwc_otg_device_intr_methods = {
	.transfer =	dwc_otg_device_intr_transfer,
	.start =	dwc_otg_device_intr_start,
	.abort =	dwc_otg_device_intr_abort,
	.close =	dwc_otg_device_intr_close,
	.cleartoggle =	dwc_otg_device_clear_toggle,
	.done =		dwc_otg_device_intr_done,
};

Static const struct usbd_pipe_methods dwc_otg_device_bulk_methods = {
	.transfer =	dwc_otg_device_bulk_transfer,
	.start =	dwc_otg_device_bulk_start,
	.abort =	dwc_otg_device_bulk_abort,
	.close =	dwc_otg_device_bulk_close,
	.cleartoggle =	dwc_otg_device_clear_toggle,
	.done =		dwc_otg_device_bulk_done,
};

Static const struct usbd_pipe_methods dwc_otg_device_isoc_methods = {
	.transfer =	dwc_otg_device_isoc_transfer,
	.start =	dwc_otg_device_isoc_start,
	.abort =	dwc_otg_device_isoc_abort,
	.close =	dwc_otg_device_isoc_close,
	.cleartoggle =	dwc_otg_noop,
	.done =		dwc_otg_device_isoc_done,
};

Static usbd_status
dwc_otg_allocm(struct usbd_bus *bus, usb_dma_t *dma, uint32_t size)
{
	struct dwc_otg_softc *sc = bus->hci_private;
	usbd_status status;

	status = usb_allocmem(&sc->sc_bus, size, 0, dma);
	if (status == USBD_NOMEM)
		status = usb_reserve_allocm(&sc->sc_dma_reserve, dma, size);
	return status;
}

Static void
dwc_otg_freem(struct usbd_bus *bus, usb_dma_t *dma)
{
        struct dwc_otg_softc *sc = bus->hci_private;

	DPRINTF("\n");

        if (dma->block->flags & USB_DMA_RESERVE) {
                usb_reserve_freem(&sc->sc_dma_reserve, dma);
                return;
        }
        usb_freemem(&sc->sc_bus, dma);
}

usbd_xfer_handle
dwc_otg_allocx(struct usbd_bus *bus)
{
	struct dwc_otg_softc *sc = bus->hci_private;
	usbd_xfer_handle xfer;

	DPRINTF("\n");

	xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers);
	if (xfer != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
#ifdef DIAGNOSTIC
		if (xfer->busy_free != XFER_FREE) {
			DPRINTF("xfer=%p not free, 0x%08x\n", xfer,
			    xfer->busy_free);
		}
#endif
		memset(xfer, 0, sizeof(struct dwc_otg_xfer));
	} else {
		xfer = kmem_zalloc(sizeof(struct dwc_otg_xfer), KM_SLEEP);
	}
#ifdef DIAGNOSTIC
	if (xfer != NULL) {
		xfer->busy_free = XFER_BUSY;
	}
#endif
	return xfer;
}

void
dwc_otg_freex(struct usbd_bus *bus, usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = bus->hci_private;

	DPRINTF("\n");

#ifdef DIAGNOSTIC
	if (xfer->busy_free != XFER_BUSY) {
		DPRINTF("xfer=%p not busy, 0x%08x\n", xfer, xfer->busy_free);
	}
	xfer->busy_free = XFER_FREE;
#endif
	SIMPLEQ_INSERT_HEAD(&sc->sc_free_xfers, xfer, next);
}


Static void
dwc_otg_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct dwc_otg_softc *sc = bus->hci_private;

	*lock = &sc->sc_lock;
}

Static void
dwc_otg_softintr(void *v)
{
	struct usbd_bus *bus = v;
	struct dwc_otg_softc *sc = bus->hci_private;
	struct dwc_otg_xfer *dxfer, *tmp;

	KASSERT(sc->sc_bus.use_polling || mutex_owned(&sc->sc_lock));

	DOTG_EVCNT_INCR(sc->sc_ev_soft_intr);

	DPRINTF("\n");
	TAILQ_FOREACH_SAFE(dxfer, &sc->sc_complete, xnext, tmp) {
		TAILQ_REMOVE(&sc->sc_complete, dxfer, xnext);

		usb_transfer_complete(&dxfer->xfer);
	}
}

Static void
dwc_otg_waitintr(struct dwc_otg_softc *sc, usbd_xfer_handle xfer)
{
	int timo;
	uint32_t intrs;

	xfer->status = USBD_IN_PROGRESS;
	for (timo = xfer->timeout; timo >= 0; timo--) {
		usb_delay_ms(&sc->sc_bus, 1);
		if (sc->sc_dying)
			break;
		intrs = DWC_OTG_READ_4(sc, DOTG_GINTSTS);

		DPRINTFN(15, "0x%08x\n", intrs);

		if (intrs) {
			KASSERT(mutex_owned(&sc->sc_lock));
			mutex_spin_enter(&sc->sc_intr_lock);
			dwc_otg_interrupt(sc);
			mutex_spin_exit(&sc->sc_intr_lock);
			if (xfer->status != USBD_IN_PROGRESS)
				return;
		}
	}

	/* Timeout */
	DPRINTF("timeout\n");

	mutex_enter(&sc->sc_lock);
	xfer->status = USBD_TIMEOUT;
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
}

Static void
dwc_otg_timeout(void *addr)
{
	struct dwc_otg_xfer *dxfer = addr;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)dxfer->xfer.pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;

	DPRINTF("dxfer=%p\n", dxfer);

	if (sc->sc_dying) {
		mutex_enter(&sc->sc_lock);
		dwc_otg_abort_xfer(&dxfer->xfer, USBD_TIMEOUT);
		mutex_exit(&sc->sc_lock);
		return;
	}

	/* Execute the abort in a process context. */
	usb_init_task(&dxfer->abort_task, dwc_otg_timeout_task, addr);
	usb_add_task(dxfer->xfer.pipe->device, &dxfer->abort_task,
	    USB_TASKQ_HC);
}

Static void
dwc_otg_timeout_task(void *addr)
{
	usbd_xfer_handle xfer = addr;
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;

	DPRINTF("xfer=%p\n", xfer);

	mutex_enter(&sc->sc_lock);
	dwc_otg_abort_xfer(xfer, USBD_TIMEOUT);
	mutex_exit(&sc->sc_lock);
}

usbd_status
dwc_otg_open(usbd_pipe_handle pipe)
{
	usbd_device_handle dev = pipe->device;
	struct dwc_otg_softc *sc = dev->bus->hci_private;
	usb_endpoint_descriptor_t *ed = pipe->endpoint->edesc;
	uint8_t addr = dev->address;
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	usbd_status err;

	DPRINTF("pipe %p addr %d xfertype %d dir %s\n", pipe,
	    addr, xfertype,
	    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN ? "in" : "out");

	if (sc->sc_dying) {
		err = USBD_IOERROR;
		goto fail;
	}

	if (addr == sc->sc_addr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->methods = &dwc_otg_root_ctrl_methods;
			break;
		case UE_DIR_IN | DWC_OTG_INTR_ENDPT:
			pipe->methods = &dwc_otg_root_intr_methods;
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
		pipe->methods = &dwc_otg_device_ctrl_methods;
		DPRINTF("UE_CONTROL methods\n");
		break;
	case UE_INTERRUPT:
		DPRINTF("UE_INTERRUPT methods\n");
		pipe->methods = &dwc_otg_device_intr_methods;
		break;
	case UE_ISOCHRONOUS:
		DPRINTF("US_ISOCHRONOUS methods\n");
		pipe->methods = &dwc_otg_device_isoc_methods;
		break;
	case UE_BULK:
		DPRINTF("UE_BULK methods\n");
		pipe->methods = &dwc_otg_device_bulk_methods;
		break;
	default:
		DPRINTF("bad xfer type %d\n", xfertype);
		return USBD_INVAL;
	}

	return USBD_NORMAL_COMPLETION;

fail:
	return err;
}

Static void
dwc_otg_poll(struct usbd_bus *bus)
{
	struct dwc_otg_softc *sc = bus->hci_private;

	KASSERT(sc->sc_bus.use_polling || mutex_owned(&sc->sc_lock));
	mutex_spin_enter(&sc->sc_intr_lock);
	dwc_otg_interrupt(sc);
	mutex_spin_exit(&sc->sc_intr_lock);
}

#if 0
/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
Static void
dwc_otg_close_pipe(usbd_pipe_handle pipe, dwc_otg_soft_ed_t *head)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	dpipe = dpipe;
	usb_delay_ms(&sc->sc_bus, 1);
}
#endif

/*
 * Abort a device request.
 */
Static void
dwc_otg_abort_xfer(usbd_xfer_handle xfer, usbd_status status)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;
	bool wake;

	DPRINTF("xfer=%p\n", xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (sc->sc_dying) {
		xfer->status = status;
		callout_stop(&xfer->timeout_handle);
		usb_transfer_complete(xfer);
		return;
	}

	if (xfer->hcflags & UXFER_ABORTING) {
		xfer->status = status;
		xfer->hcflags |= UXFER_ABORTWAIT;
		while (xfer->hcflags & UXFER_ABORTING)
			cv_wait(&xfer->hccv, &sc->sc_lock);
		return;
	}
	xfer->hcflags |= UXFER_ABORTING;

	/*
	 * Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	xfer->status = status;	/* make software ignore it */
	callout_stop(&xfer->timeout_handle);

	if (dxfer->td_transfer_cache) {
		int ch = dxfer->td_transfer_cache->channel;
		if (ch < DWC_OTG_MAX_CHANNELS) {

			DPRINTF("Disabling channel %d\n", ch);
			DWC_OTG_WRITE_4(sc, DOTG_HCINTMSK(ch),
			    HCINTMSK_CHHLTDMSK);
			DWC_OTG_WRITE_4(sc, DOTG_HCINT(ch),
			    ~HCINTMSK_CHHLTDMSK);

			if ((DWC_OTG_READ_4(sc, DOTG_HCCHAR(ch)) &
			    HCCHAR_CHENA) != 0) {
				DWC_OTG_MODIFY_4(sc, DOTG_HCCHAR(ch),
				    HCCHAR_CHENA, HCCHAR_CHDIS);
			}
		}
	}

	mutex_spin_enter(&sc->sc_intr_lock);
	dxfer->queued = false;
	TAILQ_REMOVE(&sc->sc_active, dxfer, xnext);
	mutex_spin_exit(&sc->sc_intr_lock);

	/*
	 * Step 4: Execute callback.
	 */
	wake = xfer->hcflags & UXFER_ABORTWAIT;
	xfer->hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);
	usb_transfer_complete(xfer);
	if (wake) {
		cv_broadcast(&xfer->hccv);
	}

	KASSERT(mutex_owned(&sc->sc_lock));
}

Static void
dwc_otg_noop(usbd_pipe_handle pipe)
{

	DPRINTF("\n");
}

Static void
dwc_otg_device_clear_toggle(usbd_pipe_handle pipe)
{
	
	DPRINTF("toggle %d -> 0", pipe->endpoint->datatoggle);

	pipe->endpoint->datatoggle = 0;
}

/***********************************************************************/

/*
 * Data structures and routines to emulate the root hub.
 */

Static const usb_device_descriptor_t dwc_otg_devd = {
	.bLength = sizeof(usb_device_descriptor_t),
	.bDescriptorType = UDESC_DEVICE,
	.bcdUSB = {0x00, 0x02},
	.bDeviceClass = UDCLASS_HUB,
	.bDeviceSubClass = UDSUBCLASS_HUB,
	.bDeviceProtocol = UDPROTO_HSHUBSTT,
	.bMaxPacketSize = 64,
	.bcdDevice = {0x00, 0x01},
	.iManufacturer = 1,
	.iProduct = 2,
	.bNumConfigurations = 1,
};

struct dwc_otg_config_desc {
	usb_config_descriptor_t confd;
	usb_interface_descriptor_t ifcd;
	usb_endpoint_descriptor_t endpd;
} __packed;

Static const struct dwc_otg_config_desc dwc_otg_confd = {
	.confd = {
		.bLength = USB_CONFIG_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_CONFIG,
		.wTotalLength[0] = sizeof(dwc_otg_confd),
		.bNumInterface = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = UC_SELF_POWERED,
		.bMaxPower = 0,
	},
	.ifcd = {
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
	.endpd = {
		.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
		.bDescriptorType = UDESC_ENDPOINT,
		.bEndpointAddress = UE_DIR_IN | DWC_OTG_INTR_ENDPT,
		.bmAttributes = UE_INTERRUPT,
		.wMaxPacketSize = {8, 0},			/* max packet */
		.bInterval = 255,
	},
};

#define	HSETW(ptr, val) ptr = { (uint8_t)(val), (uint8_t)((val) >> 8) }
Static const usb_hub_descriptor_t dwc_otg_hubd = {
	.bDescLength = USB_HUB_DESCRIPTOR_SIZE,
	.bDescriptorType = UDESC_HUB,
	.bNbrPorts = 1,
	HSETW(.wHubCharacteristics, (UHD_PWR_NO_SWITCH | UHD_OC_INDIVIDUAL)),
	.bPwrOn2PwrGood = 50,
	.bHubContrCurrent = 0,
	.DeviceRemovable = {0},		/* port is removable */
};

Static usbd_status
dwc_otg_root_ctrl_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	return dwc_otg_root_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue));
}

Static usbd_status
dwc_otg_root_ctrl_start(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usb_device_request_t *req;
	uint8_t *buf;
	int len, value, index, l, totlen;
	usb_port_status_t ps;
	usb_hub_descriptor_t hubd;
	usbd_status err = USBD_IOERROR;

	if (sc->sc_dying)
		return USBD_IOERROR;

	req = &xfer->request;

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	DPRINTFN(4, "type=0x%02x request=%02x value=%04x len=%04x index=%04x\n",
	    req->bmRequestType, req->bRequest, value, len, index);

	buf = len ? KERNADDR(&xfer->dmabuf, 0) : NULL;

	totlen = 0;

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
			*buf = sc->sc_conf;
			totlen = 1;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(8, "wValue=0x%04x\n", value);

		if (len == 0)
			break;
		switch (value) {
		case C(0, UDESC_DEVICE):
			l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			memcpy(buf, &dwc_otg_devd, l);
			buf += l;
			len -= l;
			totlen += l;

			break;
		case C(0, UDESC_CONFIG):
			l = min(len, sizeof(dwc_otg_confd));
			memcpy(buf, &dwc_otg_confd, l);
			buf += l;
			len -= l;
			totlen += l;

			break;
#define sd ((usb_string_descriptor_t *)buf)
		case C(0, UDESC_STRING):
			totlen = usb_makelangtbl(sd, len);
			break;
		case C(1, UDESC_STRING):
			totlen = usb_makestrdesc(sd, len, sc->sc_vendor);
			break;
		case C(2, UDESC_STRING):
			totlen = usb_makestrdesc(sd, len, "DWC OTG root hub");
			break;
#undef sd
		default:
			goto fail;
		}
		break;
	case C(UR_GET_INTERFACE, UT_READ_INTERFACE):
		if (len > 0) {
			*buf = 0;
			totlen = 1;
		}
                break;
	case C(UR_GET_STATUS, UT_READ_DEVICE):
		if (len > 1) {
			USETW(((usb_status_t *)buf)->wStatus,UDS_SELF_POWERED);
			totlen = 2;
		}
		break;
	case C(UR_GET_STATUS, UT_READ_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_ENDPOINT):
		if (len > 1) {
			USETW(((usb_status_t *)buf)->wStatus, 0);
			totlen = 2;
		}
		break;
	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
		DPRINTF("UR_SET_ADDRESS, UT_WRITE_DEVICE: addr %d\n",
		    value);
		if (value >= USB_MAX_DEVICES)
                        goto fail;

		sc->sc_addr = value;
		break;
	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		if (value != 0 && value != 1)
                        goto fail;

		sc->sc_conf = value;
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_INTERFACE):
		err = USBD_IOERROR;
		goto fail;
	case C(UR_SET_FEATURE, UT_WRITE_ENDPOINT):
// 		switch (UGETW(req->wValue)) {
// 		case UF_ENDPOINT_HALT:
// 			goto tr_handle_clear_halt;
// 		case UF_DEVICE_REMOTE_WAKEUP:
// 			goto tr_handle_clear_wakeup;
// 		default:
// 			goto tr_stalled;
// 		}
// 		break;
//		err = USBD_IOERROR;
//		goto fail;
		err = USBD_NORMAL_COMPLETION;
		goto fail;
	case C(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
		break;
	case C(UR_SYNCH_FRAME, UT_WRITE_ENDPOINT):
		break;

	/* Hub requests */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		DPRINTFN(9, "UR_CLEAR_FEATURE port=%d feature=%d\n",
		    index, value);
		if (index < 1 || index > sc->sc_noport)
                        goto fail;

		switch (value) {
		case UHF_PORT_ENABLE:
			if (sc->sc_flags.status_device_mode == 0) {
				DWC_OTG_WRITE_4(sc, DOTG_HPRT,
				    sc->sc_hprt_val | HPRT_PRTENA);
			}
			sc->sc_flags.port_enabled = 0;
			break;

		case UHF_PORT_SUSPEND:
			dwc_otg_wakeup_peer(sc);
			break;

		case UHF_PORT_POWER:
			sc->sc_flags.port_powered = 0;
			if (sc->sc_mode == DWC_MODE_HOST ||
			    sc->sc_mode == DWC_MODE_OTG) {
				sc->sc_hprt_val = 0;
				DWC_OTG_WRITE_4(sc, DOTG_HPRT, HPRT_PRTENA);
			}
			dwc_otg_pull_down(sc);
			dwc_otg_clocks_off(sc);
			break;

		case UHF_C_PORT_CONNECTION:
			/* clear connect change flag */
			sc->sc_flags.change_connect = 0;
			break;
		case UHF_C_PORT_ENABLE:
			sc->sc_flags.change_enabled = 0;
			break;
		case UHF_C_PORT_SUSPEND:
			sc->sc_flags.change_suspend = 0;
			break;
		case UHF_C_PORT_OVER_CURRENT:
			sc->sc_flags.change_over_current = 0;
			break;
		case UHF_C_PORT_RESET:
			/* ??? *//* enable rhsc interrupt if condition is cleared */
			sc->sc_flags.change_reset = 0;
			break;
// 		case UHF_PORT_TEST:
// 		case UHF_PORT_INDICATOR:
// 			/* nops */
// 			break;
		default:
			goto fail;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0)
			goto fail;

		hubd = dwc_otg_hubd;
		hubd.bNbrPorts = sc->sc_noport;

		l = min(len, hubd.bDescLength);
		memcpy(buf, &hubd, l);
		buf += l;
		len -= l;
		totlen += l;

		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4)
			goto fail;
		memset(buf, 0, len); /* ? XXX */
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		DPRINTFN(8, "get port status i=%d\n", index);

		if (index < 1 || index > sc->sc_noport)
			goto fail;
		if (len != 4)
			goto fail;

		if (sc->sc_flags.status_vbus)
			dwc_otg_clocks_on(sc);
		else
			dwc_otg_clocks_off(sc);

		/* Select Device Side Mode */
		value = 0;
		if (sc->sc_flags.status_device_mode) {
			/*
			 * XXX FreeBSD specific, which value ?
			 * value = UPS_PORT_MODE_DEVICE;
			 */
			dwc_otg_timer_stop(sc);
		} else {
			dwc_otg_timer_start(sc);
		}

		if (sc->sc_flags.status_high_speed)
			value |= UPS_HIGH_SPEED;
		else if (sc->sc_flags.status_low_speed)
			value |= UPS_LOW_SPEED;

		if (sc->sc_flags.port_powered)
			value |= UPS_PORT_POWER;

		if (sc->sc_flags.port_enabled)
			value |= UPS_PORT_ENABLED;

		if (sc->sc_flags.port_over_current)
			value |= UPS_OVERCURRENT_INDICATOR;

		if (sc->sc_flags.status_vbus &&
		    sc->sc_flags.status_bus_reset)
			value |= UPS_CURRENT_CONNECT_STATUS;

		if (sc->sc_flags.status_suspend)
			value |= UPS_SUSPEND;

		USETW(ps.wPortStatus, value);

		value = 0;

		if (sc->sc_flags.change_connect)
			value |= UPS_C_CONNECT_STATUS;
		if (sc->sc_flags.change_suspend)
			value |= UPS_C_SUSPEND;
		if (sc->sc_flags.change_reset)
			value |= UPS_C_PORT_RESET;
		if (sc->sc_flags.change_over_current)
			value |= UPS_C_OVERCURRENT_INDICATOR;

		USETW(ps.wPortChange, value);

		l = min(len, sizeof(ps));
		memcpy(buf, &ps, l);
		buf += l;
		len -= l;
		totlen += l;

		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		goto fail;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		DPRINTFN(9, "UR_SET_FEATURE port=%d feature=%d\n",
		    index, value);

		if (index < 1 || index > sc->sc_noport)
			goto fail;

		switch (value) {
		case UHF_PORT_ENABLE:
			DPRINTF("UHF_PORT_ENABLE\n");
			break;

		case UHF_PORT_SUSPEND:
			DPRINTF("UHF_PORT_SUSPEND device mode %d\n",
			    sc->sc_flags.status_device_mode);

			if (sc->sc_flags.status_device_mode == 0) {
				/* set suspend BIT */
				sc->sc_hprt_val |= HPRT_PRTSUSP;
				DWC_OTG_WRITE_4(sc, DOTG_HPRT, sc->sc_hprt_val);

				/* generate HUB suspend event */
				dwc_otg_suspend_irq(sc);
			}
			break;

		case UHF_PORT_RESET:
			DPRINTF("UHF_PORT_RESET device_mode %d hprt %08x\n",
			    sc->sc_flags.status_device_mode, sc->sc_hprt_val);
			if (sc->sc_flags.status_device_mode == 0) {
				/* enable PORT reset */
				DWC_OTG_WRITE_4(sc, DOTG_HPRT,
				    sc->sc_hprt_val | HPRT_PRTRST);

				/* Wait 62.5ms for reset to complete */
				usb_delay_ms(&sc->sc_bus, 63);

				DWC_OTG_WRITE_4(sc, DOTG_HPRT, sc->sc_hprt_val);

				/* Wait 62.5ms for reset to complete */
				usb_delay_ms(&sc->sc_bus, 63);

				/* reset FIFOs */
				dwc_otg_init_fifo(sc, DWC_MODE_HOST);

				sc->sc_flags.change_reset = 1;
			} else {
				err = USBD_IOERROR;
			}
			break;

// 		case UHF_PORT_TEST:
// 		case UHF_PORT_INDICATOR:
// 			/* nops */
// 			break;
		case UHF_PORT_POWER:
			DPRINTF("UHF_PORT_POWER mode %d\n",
			   sc->sc_mode);

			if (sc->sc_mode == DWC_MODE_HOST ||
			    sc->sc_mode == DWC_MODE_OTG) {

				sc->sc_hprt_val |= HPRT_PRTPWR;
				DWC_OTG_WRITE_4(sc, DOTG_HPRT, sc->sc_hprt_val);
			}
			sc->sc_flags.port_powered = 1;
			break;
		default:
			err = USBD_IOERROR;
			goto fail;
		}
		break;
	default:
		goto fail;
	}
	xfer->actlen = totlen;
	err = USBD_NORMAL_COMPLETION;

fail:
        xfer->status = err;

        mutex_enter(&sc->sc_lock);
        usb_transfer_complete(xfer);
        mutex_exit(&sc->sc_lock);

        return USBD_IN_PROGRESS;
}

Static void
dwc_otg_root_ctrl_abort(usbd_xfer_handle xfer)
{
	DPRINTF("\n");

	/* Nothing to do, all transfers are synchronous. */
}

Static void
dwc_otg_root_ctrl_close(usbd_pipe_handle pipe)
{
	DPRINTF("\n");

	/* Nothing to do. */
}

Static void
dwc_otg_root_ctrl_done(usbd_xfer_handle xfer)
{

	DPRINTF("\n");

	/* Nothing to do. */
}

Static usbd_status
dwc_otg_root_intr_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	DPRINTF("\n");

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (dwc_otg_root_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
dwc_otg_root_intr_start(usbd_xfer_handle xfer)
{
	usbd_pipe_handle pipe = xfer->pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	DPRINTF("\n");

	if (sc->sc_dying)
		return (USBD_IOERROR);

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	mutex_exit(&sc->sc_lock);

	return (USBD_IN_PROGRESS);
}

/* Abort a root interrupt request. */
Static void
dwc_otg_root_intr_abort(usbd_xfer_handle xfer)
{
#ifdef DIAGNOSTIC
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
#endif
	DPRINTF("xfer=%p\n", xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	if (xfer->pipe->intrxfer == xfer) {
		DPRINTF("remove\n");
		xfer->pipe->intrxfer = NULL;
	}
	xfer->status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

Static void
dwc_otg_root_intr_close(usbd_pipe_handle pipe)
{
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	DPRINTF("\n");

	KASSERT(mutex_owned(&sc->sc_lock));

	sc->sc_intrxfer = NULL;
}

Static void
dwc_otg_root_intr_done(usbd_xfer_handle xfer)
{

	DPRINTF("\n");
}

/***********************************************************************/
Static usbd_status
dwc_otg_device_ctrl_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	DPRINTF("\n");

	dwc_otg_xfer_setup(xfer);
	dwc_otg_setup_ctrl_chain(xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (dwc_otg_device_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
dwc_otg_device_ctrl_start(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;

	DPRINTF("\n");

	mutex_enter(&sc->sc_lock);
	xfer->status = USBD_IN_PROGRESS;
	dwc_otg_xfer_start(xfer);
	mutex_exit(&sc->sc_lock);

	if (sc->sc_bus.use_polling)
		dwc_otg_waitintr(sc, xfer);

	return USBD_IN_PROGRESS;
}

Static void
dwc_otg_device_ctrl_abort(usbd_xfer_handle xfer)
{
#ifdef DIAGNOSTIC
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
#endif
	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%p\n", xfer);
	dwc_otg_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc_otg_device_ctrl_close(usbd_pipe_handle pipe)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	dpipe = dpipe;
	sc = sc;
	DPRINTF("\n");
}

Static void
dwc_otg_device_ctrl_done(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = DWC_OTG_XFER2SC(xfer);

	DPRINTF("xfer=%p\n", xfer);
	KASSERT(mutex_owned(&sc->sc_lock));

	dwc_otg_xfer_end(xfer);
}

/***********************************************************************/

Static usbd_status
dwc_otg_device_bulk_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	DPRINTF("\n");
#if 0
	printf("%s: xfer = %p, length = %d, flags = 0x%x, toggle %d\n",
	    __func__,
	    xfer, xfer->length, xfer->flags, xfer->pipe->endpoint->datatoggle);
#endif

	dwc_otg_xfer_setup(xfer);
	dwc_otg_setup_bulk_chain(xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return (dwc_otg_device_bulk_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
dwc_otg_device_bulk_start(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;

	DPRINTF("xfer=%p\n", xfer);

	mutex_enter(&sc->sc_lock);
	xfer->status = USBD_IN_PROGRESS;
	dwc_otg_xfer_start(xfer);
	mutex_exit(&sc->sc_lock);

	if (sc->sc_bus.use_polling)
		dwc_otg_waitintr(sc, xfer);

	return USBD_IN_PROGRESS;
}

Static void
dwc_otg_device_bulk_abort(usbd_xfer_handle xfer)
{
#ifdef DIAGNOSTIC
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
#endif
	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%p\n", xfer);
	dwc_otg_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc_otg_device_bulk_close(usbd_pipe_handle pipe)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	DPRINTF("\n");

	dpipe = dpipe;
	sc = sc;
}

Static void
dwc_otg_device_bulk_done(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;

	DPRINTF("\n");

	dwc_otg_xfer_end(xfer);
	sc = sc;
}

/***********************************************************************/

Static usbd_status
dwc_otg_device_intr_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	DPRINTF("\n");

	dwc_otg_xfer_setup(xfer);
	dwc_otg_setup_intr_chain(xfer);

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return (dwc_otg_device_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
dwc_otg_device_intr_start(usbd_xfer_handle xfer)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usbd_device_handle dev = dpipe->pipe.device;
	struct dwc_otg_softc *sc = dev->bus->hci_private;

	DPRINTF("\n");

	mutex_enter(&sc->sc_lock);
	xfer->status = USBD_IN_PROGRESS;
	dwc_otg_xfer_start(xfer);
	mutex_exit(&sc->sc_lock);

	if (sc->sc_bus.use_polling)
		dwc_otg_waitintr(sc, xfer);

	return USBD_IN_PROGRESS;

}

/* Abort a device interrupt request. */
Static void
dwc_otg_device_intr_abort(usbd_xfer_handle xfer)
{
#ifdef DIAGNOSTIC
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
#endif
	DPRINTF("xfer=%p\n", xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	if (xfer->pipe->intrxfer == xfer) {
		DPRINTF("remove\n");
		xfer->pipe->intrxfer = NULL;
	}
	dwc_otg_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
dwc_otg_device_intr_close(usbd_pipe_handle pipe)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	dpipe = dpipe;
	sc = sc;
	DPRINTF("\n");
}

Static void
dwc_otg_device_intr_done(usbd_xfer_handle xfer)
{
	DPRINTF("\n");

#if 0
	printf("%s: xfer = %p, repeat = %d\n", __func__, xfer, xfer->pipe->repeat);
#endif

	if (xfer->pipe->repeat)	{
		/* XXX JDM */
		dwc_otg_xfer_end(xfer);
		dwc_otg_xfer_setup(xfer);
		dwc_otg_setup_intr_chain(xfer);
		xfer->actlen = 0;
		xfer->status = USBD_IN_PROGRESS;
		dwc_otg_xfer_start(xfer);
	} else {
		dwc_otg_xfer_end(xfer);
	}
}

/***********************************************************************/

usbd_status
dwc_otg_device_isoc_transfer(usbd_xfer_handle xfer)
{
	struct dwc_otg_softc *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	DPRINTF("\n");
	panic("not yet\n");

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return (dwc_otg_device_isoc_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

#if 0
void
dwc_otg_device_isoc_enter(usbd_xfer_handle xfer)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usbd_device_handle dev = dpipe->pipe.device;
	struct dwc_otg_softc *sc = dev->bus->hci_private;

	DPRINTF("\n");

	sc = sc;
}
#endif

usbd_status
dwc_otg_device_isoc_start(usbd_xfer_handle xfer)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;

	sc = sc;
	DPRINTF("\n");

	return USBD_IN_PROGRESS;
}

void
dwc_otg_device_isoc_abort(usbd_xfer_handle xfer)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;

	sc = sc;
	DPRINTF("\n");
}


#if 0
usbd_status
dwc_otg_setup_isoc(usbd_pipe_handle pipe)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	dpipe = dpipe;
	sc = sc;
	DPRINTF("\n");

	return USBD_NORMAL_COMPLETION;
}
#endif

void
dwc_otg_device_isoc_close(usbd_pipe_handle pipe)
{
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)pipe;
	struct dwc_otg_softc *sc = pipe->device->bus->hci_private;

	dpipe = dpipe;
	sc = sc;
	DPRINTF("\n");
}

void
dwc_otg_device_isoc_done(usbd_xfer_handle xfer)
{
	DPRINTF("\n");
}

/***********************************************************************/
Static void
dwc_otg_core_reset(struct dwc_otg_softc *sc)
{
	int loop = 0, idle;
	do {
		uint32_t grstctl;

		delay(10);
		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, 4,
			BUS_SPACE_BARRIER_READ);

		grstctl = DWC_OTG_READ_4(sc, DOTG_GRSTCTL);

		if (loop++ > 100000) {
			printf("%s: error\n", __func__);
			panic("%s", __func__);
		}

		idle = (grstctl & GRSTCTL_AHBIDLE);
	} while (idle == 0);

	DWC_OTG_WRITE_4(sc, DOTG_GRSTCTL, GRSTCTL_CSFTRST);

	loop = 0;
	volatile int reset;
	do {
		uint32_t grstctl;

		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, 0x1000,
		    BUS_SPACE_BARRIER_READ);

		grstctl = DWC_OTG_READ_4(sc, DOTG_GRSTCTL);

		if (loop++ > 100000) {
			printf("%s: soft reset error grstctl %08x\n",
			       __func__, grstctl);
			panic("%s", __func__);
		}

		reset = (grstctl & GRSTCTL_CSFTRST);
		delay(10);
	} while (reset == 1);
}

Static void
dwc_otg_do_poll(struct usbd_bus *bus)
{
	struct dwc_otg_softc *sc = DWC_OTG_BUS2SC(bus);

	KASSERT(mutex_owned(&sc->sc_lock));
	mutex_spin_enter(&sc->sc_intr_lock);
	dwc_otg_interrupt_poll(sc);
	mutex_spin_exit(&sc->sc_intr_lock);
}

Static void
dwc_otg_worker(struct work *wk, void *priv)
{
	struct dwc_otg_work *dwork = (struct dwc_otg_work *)wk;
	usbd_xfer_handle xfer = dwork->xfer;
	struct dwc_otg_softc *sc = dwork->sc;

	DOTG_EVCNT_INCR(sc->sc_ev_work);

	mutex_enter(&sc->sc_lock);
	if (dwork == &sc->sc_timer_work) {
		dwc_otg_timer(sc);
	} else {
		KASSERT(dwork->xfer != NULL);
		dwc_otg_start_standard_chain(xfer);
	}
	mutex_exit(&sc->sc_lock);
}

int dwc_otg_intr(void *p)
{
	struct dwc_otg_softc *sc = p;
	int ret = 0;

	DPRINTF("sc %p\n", sc);

	if (sc == NULL)
		return 0;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	if (sc->sc_bus.use_polling) {
		uint32_t status = DWC_OTG_READ_4(sc, DOTG_GINTSTS);
		DWC_OTG_WRITE_4(sc, DOTG_GINTSTS, status);
	} else {
		ret = dwc_otg_interrupt(sc);
	}

done:
	mutex_spin_exit(&sc->sc_intr_lock);

	return ret;
}

int
dwc_otg_detach(struct dwc_otg_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	return rv;
}

bool
dwc_otg_shutdown(device_t self, int flags)
{
	struct dwc_otg_softc *sc = device_private(self);

	sc = sc;

	return true;
}

void
dwc_otg_childdet(device_t self, device_t child)
{
	struct dwc_otg_softc *sc = device_private(self);

	sc = sc;
}

int
dwc_otg_activate(device_t self, enum devact act)
{
	struct dwc_otg_softc *sc = device_private(self);

	sc = sc;

	return 0;
}

bool
dwc_otg_resume(device_t dv, const pmf_qual_t *qual)
{
	struct dwc_otg_softc *sc = device_private(dv);

	sc = sc;

	return true;
}

bool
dwc_otg_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct dwc_otg_softc *sc = device_private(dv);

	sc = sc;

	return true;
}

/***********************************************************************/

#ifdef DWC_OTG_DEBUG
void
dwc_otg_dump_global_regs(struct dwc_otg_softc *sc)
{
	int i, n;

	printf("GOTGCTL        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GOTGCTL));
	printf("GOTGINT        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GOTGINT));
	printf("GAHBCFG        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GAHBCFG));
	printf("GUSBCFG        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GUSBCFG));
	printf("GRSTCTL        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GRSTCTL));
	printf("GINTSTS        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GINTSTS));
	printf("GINTMSK        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GINTMSK));
	printf("GRXSTSR        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GRXSTSRD));
	printf("GRXSTSP        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GRXSTSPD));
	printf("GRXFSIZ        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GRXFSIZ));
	printf("GNPTXFSIZ      0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GNPTXFSIZ));
	printf("GNPTXSTS       0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GNPTXSTS));
	printf("GI2CCTL        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GI2CCTL));
	printf("GPVNDCTL       0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GPVNDCTL));
	printf("GGPIO          0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GGPIO));
	printf("GUID           0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GUID));
	printf("GSNPSID        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GSNPSID));
	printf("GHWCFG1        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GHWCFG1));
	printf("GHWCFG2        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GHWCFG2));
	printf("GHWCFG3        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GHWCFG3));
	printf("GHWCFG4        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GHWCFG4));
	printf("GLPMCFG        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_GLPMCFG));
	printf("HPTXFSIZ       0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HPTXFSIZ));

	n = GHWCFG4_NUMDEVPERIOEPS_GET(DWC_OTG_READ_4(sc, DOTG_GHWCFG4));
	for (i=1; i<n; ++i) {
		printf("DPTXFSIZ[%2d]  0x%08x\n", i,
			DWC_OTG_READ_4(sc, DOTG_DPTXFSIZ(i)));
	}

	printf("PCGCCTL        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_PCGCCTL));


}

void
dwc_otg_dump_host_regs(struct dwc_otg_softc *sc)
{
	int i, n;

	printf("HCFG           0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HCFG));
	printf("HFIR           0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HFIR));
	printf("HFNUM          0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HFNUM));
	printf("HPTXSTS        0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HPTXSTS));
	printf("HAINT          0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HAINT));
	printf("HAINTMSK       0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HAINTMSK));
	printf("HFLBADDR       0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HFLBADDR));
	printf("HPRT           0x%08x\n", DWC_OTG_READ_4(sc, DOTG_HPRT));

	n = GHWCFG2_NUMHSTCHNL_GET(DWC_OTG_READ_4(sc, DOTG_GHWCFG2));
	for (i=0; i<n; ++i) {
		printf("Host Channel %d Specific Registers\n", i);
		printf("HCCHAR         0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCCHAR(i)));
		printf("HCSPLT         0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCSPLT(i)));
		printf("HCINT          0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCINT(i)));
		printf("HCINTMSK       0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCINTMSK(i)));
		printf("HCTSIZ         0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCTSIZ(i)));
		printf("HCDMA          0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCDMA(i)));
		printf("HCDMAB         0x%08x\n", DWC_OTG_READ_4(sc,DOTG_HCDMAB(i)));
	}


}
#endif

/***********************************************************************/



/* FreeBSD direct (mostly) copy functions */

static int
dwc_otg_init_fifo(struct dwc_otg_softc *sc, uint8_t mode)
{
// 	struct dwc_otg_profile *pf;
	uint32_t fifo_size;
	uint32_t fifo_regs;
	uint32_t tx_start;
	uint8_t x;

	DPRINTFN(5, "mode = %d\n", mode);

	fifo_size = sc->sc_fifo_size;

	fifo_regs = 4 * (sc->sc_dev_ep_max + sc->sc_dev_in_ep_max);

	if (fifo_size >= fifo_regs)
		fifo_size -= fifo_regs;
	else
		fifo_size = 0;

	/* split equally for IN and OUT */
	fifo_size /= 2;

	DWC_OTG_WRITE_4(sc, DOTG_GRXFSIZ, fifo_size / 4);

	/* align to 4-bytes */
	fifo_size &= ~3;

	tx_start = fifo_size;

	if (fifo_size < 0x40) {
		DPRINTFN(-1, "Not enough data space for EP0 FIFO.\n");
		return EINVAL;
	}

	if (mode == DWC_MODE_HOST) {

		/* reset active endpoints */
		sc->sc_active_rx_ep = 0;

		fifo_size /= 2;

		DWC_OTG_WRITE_4(sc, DOTG_GNPTXFSIZ,
		    ((fifo_size / 4) << 16) | (tx_start / 4));

		tx_start += fifo_size;

		DWC_OTG_WRITE_4(sc, DOTG_HPTXFSIZ,
		    ((fifo_size / 4) << 16) | (tx_start / 4));

		for (x = 0; x != sc->sc_host_ch_max; x++) {
			/* enable interrupts */
			DWC_OTG_WRITE_4(sc, DOTG_HCINTMSK(x),
			    HCINT_STALL | HCINT_BBLERR |
			    HCINT_XACTERR |
			    HCINT_NAK | HCINT_ACK | HCINT_NYET |
			    HCINT_CHHLTD | HCINT_FRMOVRUN |
			    HCINT_DATATGLERR);
		}

		/* enable host channel interrupts */
		DWC_OTG_WRITE_4(sc, DOTG_HAINTMSK,
		    (1U << sc->sc_host_ch_max) - 1U);

	}

#ifdef notyet
	if (mode == DWC_MODE_DEVICE) {

	    DWC_OTG_WRITE_4(sc, DOTG_GNPTXFSIZ,
		(0x10 << 16) | (tx_start / 4));
	    fifo_size -= 0x40;
	    tx_start += 0x40;

	    /* setup control endpoint profile */
	    sc->sc_hw_ep_profile[0].usb = dwc_otg_ep_profile[0];

	    /* reset active endpoints */
	    sc->sc_active_rx_ep = 1;

	    for (x = 1; x != sc->sc_dev_ep_max; x++) {

		pf = sc->sc_hw_ep_profile + x;

		pf->usb.max_out_frame_size = 1024 * 3;
		pf->usb.is_simplex = 0;	/* assume duplex */
		pf->usb.support_bulk = 1;
		pf->usb.support_interrupt = 1;
		pf->usb.support_isochronous = 1;
		pf->usb.support_out = 1;

		if (x < sc->sc_dev_in_ep_max) {
			uint32_t limit;

			limit = (x == 1) ? DWC_OTG_MAX_TXN :
			    (DWC_OTG_MAX_TXN / 2);

			if (fifo_size >= limit) {
				DWC_OTG_WRITE_4(sc, DOTG_DIEPTXF(x),
				    ((limit / 4) << 16) |
				    (tx_start / 4));
				tx_start += limit;
				fifo_size -= limit;
				pf->usb.max_in_frame_size = 0x200;
				pf->usb.support_in = 1;
				pf->max_buffer = limit;

			} else if (fifo_size >= 0x80) {
				DWC_OTG_WRITE_4(sc, DOTG_DIEPTXF(x),
				    ((0x80 / 4) << 16) | (tx_start / 4));
				tx_start += 0x80;
				fifo_size -= 0x80;
				pf->usb.max_in_frame_size = 0x40;
				pf->usb.support_in = 1;

			} else {
				pf->usb.is_simplex = 1;
				DWC_OTG_WRITE_4(sc, DOTG_DIEPTXF(x),
				    (0x0 << 16) | (tx_start / 4));
			}
		} else {
			pf->usb.is_simplex = 1;
		}

		DPRINTF("FIFO%d = IN:%d / OUT:%d\n", x,
		    pf->usb.max_in_frame_size, pf->usb.max_out_frame_size);
	    }
	}
#endif

	/* reset RX FIFO */
	DWC_OTG_WRITE_4(sc, DOTG_GRSTCTL, GRSTCTL_RXFFLSH);

	if (mode != DWC_MODE_OTG) {
		/* reset all TX FIFOs */
		DWC_OTG_WRITE_4(sc, DOTG_GRSTCTL,
		    GRSTCTL_TXFIFO(0x10) | GRSTCTL_TXFFLSH);
	} else {
		/* reset active endpoints */
		sc->sc_active_rx_ep = 0;
	}

	return 0;
}

Static void
dwc_otg_clocks_on(struct dwc_otg_softc* sc)
{
	if (sc->sc_flags.clocks_off &&
	    sc->sc_flags.port_powered) {

		DPRINTFN(5, "\n");

		/* TODO - platform specific */

		sc->sc_flags.clocks_off = 0;
	}
}

Static void
dwc_otg_clocks_off(struct dwc_otg_softc* sc)
{
	if (!sc->sc_flags.clocks_off) {

		DPRINTFN(5, "\n");

		/* TODO - platform specific */

		sc->sc_flags.clocks_off = 1;
	}
}

Static void
dwc_otg_pull_up(struct dwc_otg_softc *sc)
{
	uint32_t temp;

	/* pullup D+, if possible */

	if (!sc->sc_flags.d_pulled_up &&
	    sc->sc_flags.port_powered) {
		DPRINTF("up\n");
		sc->sc_flags.d_pulled_up = 1;

		temp = DWC_OTG_READ_4(sc, DOTG_DCTL);
		temp &= ~DCTL_SFTDISCON;
		DWC_OTG_WRITE_4(sc, DOTG_DCTL, temp);
	}
}

Static void
dwc_otg_pull_down(struct dwc_otg_softc *sc)
{
	uint32_t temp;

	/* pulldown D+, if possible */

	if (sc->sc_flags.d_pulled_up) {
		DPRINTF("down\n");

		sc->sc_flags.d_pulled_up = 0;

		temp = DWC_OTG_READ_4(sc, DOTG_DCTL);
		temp |= DCTL_SFTDISCON;
		DWC_OTG_WRITE_4(sc, DOTG_DCTL, temp);
	}
}

Static void
dwc_otg_enable_sof_irq(struct dwc_otg_softc *sc)
{
	if (sc->sc_irq_mask & GINTMSK_SOFMSK)
		return;
	sc->sc_irq_mask |= GINTMSK_SOFMSK;
	DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);
}

Static void
dwc_otg_resume_irq(struct dwc_otg_softc *sc)
{
	if (sc->sc_flags.status_suspend) {
		/* update status bits */
		sc->sc_flags.status_suspend = 0;
		sc->sc_flags.change_suspend = 1;

		if (sc->sc_flags.status_device_mode) {
			/*
			 * Disable resume interrupt and enable suspend
			 * interrupt:
			 */
			sc->sc_irq_mask &= ~GINTMSK_WKUPINTMSK;
			sc->sc_irq_mask |= GINTMSK_USBSUSPMSK;
			DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);
		}

		/* complete root HUB interrupt endpoint */
		dwc_otg_root_intr(sc);
	}
}

Static void
dwc_otg_suspend_irq(struct dwc_otg_softc *sc)
{
	if (!sc->sc_flags.status_suspend) {
		/* update status bits */
		sc->sc_flags.status_suspend = 1;
		sc->sc_flags.change_suspend = 1;

		if (sc->sc_flags.status_device_mode) {
			/*
			 * Disable suspend interrupt and enable resume
			 * interrupt:
			 */
			sc->sc_irq_mask &= ~GINTMSK_USBSUSPMSK;
			sc->sc_irq_mask |= GINTMSK_WKUPINTMSK;
			DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);
		}

		/* complete root HUB interrupt endpoint */
		dwc_otg_root_intr(sc);
	}
}

Static void
dwc_otg_wakeup_peer(struct dwc_otg_softc *sc)
{
	if (!sc->sc_flags.status_suspend)
		return;

	DPRINTFN(5, "Remote wakeup\n");

	if (sc->sc_flags.status_device_mode) {
		uint32_t temp;

		/* enable remote wakeup signalling */
		temp = DWC_OTG_READ_4(sc, DOTG_DCTL);
		temp |= DCTL_RMTWKUPSIG;
		DWC_OTG_WRITE_4(sc, DOTG_DCTL, temp);

		/* Wait 8ms for remote wakeup to complete. */
		usb_delay_ms_locked(&sc->sc_bus, 8, &sc->sc_lock);

		temp &= ~DCTL_RMTWKUPSIG;
		DWC_OTG_WRITE_4(sc, DOTG_DCTL, temp);
	} else {
		/* enable USB port */
		DWC_OTG_WRITE_4(sc, DOTG_PCGCCTL, 0);

		/* wait 10ms */
		usb_delay_ms_locked(&sc->sc_bus, 10, &sc->sc_lock);

		/* resume port */
		sc->sc_hprt_val |= HPRT_PRTRES;
		DWC_OTG_WRITE_4(sc, DOTG_HPRT, sc->sc_hprt_val);

		/* Wait 100ms for resume signalling to complete. */
		usb_delay_ms_locked(&sc->sc_bus, 100, &sc->sc_lock);

		/* clear suspend and resume */
		sc->sc_hprt_val &= ~(HPRT_PRTSUSP | HPRT_PRTRES);
		DWC_OTG_WRITE_4(sc, DOTG_HPRT, sc->sc_hprt_val);

		/* Wait 4ms */
		usb_delay_ms_locked(&sc->sc_bus, 4, &sc->sc_lock);
	}

	/* need to fake resume IRQ */
	dwc_otg_resume_irq(sc);
}

static void
dwc_otg_set_address(struct dwc_otg_softc *sc, uint8_t addr)
{
	uint32_t temp;

	DPRINTFN(5, "addr=%d\n", addr);

	temp = DWC_OTG_READ_4(sc, DOTG_DCFG);
	temp &= ~DCFG_DEVADDR_SET(0x7F);
	temp |= DCFG_DEVADDR_SET(addr);
	DWC_OTG_WRITE_4(sc, DOTG_DCFG, temp);
}

static void
dwc_otg_common_rx_ack(struct dwc_otg_softc *sc)
{
	DPRINTFN(5, "RX status clear\n");

	/* enable RX FIFO level interrupt */
	sc->sc_irq_mask |= GINTMSK_RXFLVLMSK;
	DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);

	/* clear cached status */
	sc->sc_last_rx_status = 0;
}

static void
dwc_otg_clear_hcint(struct dwc_otg_softc *sc, uint8_t x)
{
	uint32_t hcint;

	hcint = DWC_OTG_READ_4(sc, DOTG_HCINT(x));
	DWC_OTG_WRITE_4(sc, DOTG_HCINT(x), hcint);

	/* clear buffered interrupts */
	sc->sc_chan_state[x].hcint = 0;
}

static uint8_t
dwc_otg_host_channel_wait(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint8_t x;

	x = td->channel;

	DPRINTF("CH=%d\n", x);

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	if (sc->sc_chan_state[x].wait_sof == 0) {
		dwc_otg_clear_hcint(sc, x);
		return (1);	/* done */
	}

	if (x == 0)
		return (0);	/* wait */

	/* find new disabled channel */
	for (x = 1; x != sc->sc_host_ch_max; x++) {

		if (sc->sc_chan_state[x].allocated)
			continue;
		if (sc->sc_chan_state[x].wait_sof != 0)
			continue;

		sc->sc_chan_state[td->channel].allocated = 0;
		sc->sc_chan_state[x].allocated = 1;

		if (sc->sc_chan_state[td->channel].suspended) {
			sc->sc_chan_state[td->channel].suspended = 0;
			sc->sc_chan_state[x].suspended = 1;
		}

		/* clear interrupts */
		dwc_otg_clear_hcint(sc, x);

		DPRINTF("CH=%d HCCHAR=0x%08x "
		    "HCSPLT=0x%08x\n", x, td->hcchar, td->hcsplt);

		/* ack any pending messages */
		if (sc->sc_last_rx_status != 0 &&
		    GRXSTSRD_CHNUM_GET(sc->sc_last_rx_status) == td->channel) {
			/* get rid of message */
			dwc_otg_common_rx_ack(sc);
		}

		/* move active channel */
		sc->sc_active_rx_ep &= ~(1 << td->channel);
		sc->sc_active_rx_ep |= (1 << x);

		/* set channel */
		td->channel = x;

		return (1);	/* new channel allocated */
	}
	return (0);	/* wait */
}

static uint8_t
dwc_otg_host_channel_alloc(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint8_t x;
	uint8_t max_channel;

	DPRINTFN(9, "\n");

	if (td->channel < DWC_OTG_MAX_CHANNELS) {
		return (0);		/* already allocated */
	}

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	if ((td->hcchar & HCCHAR_EPNUM_MASK) == 0) {
		max_channel = 1;
		x = 0;
	} else {
		max_channel = sc->sc_host_ch_max;
		x = 1;
	}

	for (; x != max_channel; x++) {

		if (sc->sc_chan_state[x].allocated)
			continue;
		if (sc->sc_chan_state[x].wait_sof != 0)
			continue;

		sc->sc_chan_state[x].allocated = 1;

		/* clear interrupts */
		dwc_otg_clear_hcint(sc, x);

		DPRINTF("CH=%d HCCHAR=0x%08x HCSPLT=0x%08x\n",
		    x, td->hcchar, td->hcsplt);

		/* set active channel */
		sc->sc_active_rx_ep |= (1 << x);

		/* set channel */
		td->channel = x;

		return (0);	/* allocated */
	}
	return (1);	/* busy */
}

static void
dwc_otg_host_channel_disable(struct dwc_otg_softc *sc, uint8_t x)
{
	uint32_t hcchar;

	if (sc->sc_chan_state[x].wait_sof != 0)
		return;

	hcchar = DWC_OTG_READ_4(sc, DOTG_HCCHAR(x));
	if (hcchar & (HCCHAR_CHENA | HCCHAR_CHDIS)) {
		/* disable channel */
		DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(x),
		    HCCHAR_CHENA | HCCHAR_CHDIS);
		/* don't re-use channel until next SOF is transmitted */
		sc->sc_chan_state[x].wait_sof = 2;
		/* enable SOF interrupt */
		dwc_otg_enable_sof_irq(sc);
	}
}

static void
dwc_otg_host_channel_free(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint8_t x;

	if (td->channel >= DWC_OTG_MAX_CHANNELS)
		return;		/* already freed */

	/* free channel */
	x = td->channel;
	td->channel = DWC_OTG_MAX_CHANNELS;

	DPRINTF("CH=%d\n", x);

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	dwc_otg_host_channel_disable(sc, x);

	sc->sc_chan_state[x].allocated = 0;
	sc->sc_chan_state[x].suspended = 0;

	/* ack any pending messages */
	if (sc->sc_last_rx_status != 0 &&
	    GRXSTSRD_CHNUM_GET(sc->sc_last_rx_status) == x) {
		dwc_otg_common_rx_ack(sc);
	}

	/* clear active channel */
	sc->sc_active_rx_ep &= ~(1 << x);
}

static uint8_t
dwc_otg_host_setup_tx(struct dwc_otg_td *td)
{
	usb_device_request_t req __aligned(4);
	struct dwc_otg_softc *sc;
	uint32_t hcint;
	uint32_t hcchar;

	if (dwc_otg_host_channel_alloc(td))
		return (1);		/* busy */

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	hcint = sc->sc_chan_state[td->channel].hcint;

	DPRINTF("CH=%d ST=%d HCINT=0x%08x HCCHAR=0x%08x HCTSIZ=0x%08x\n",
	    td->channel, td->state, hcint,
	    DWC_OTG_READ_4(sc, DOTG_HCCHAR(td->channel)),
	    DWC_OTG_READ_4(sc, DOTG_HCTSIZ(td->channel)));

	if (hcint & (HCINT_RETRY |
	    HCINT_ACK | HCINT_NYET)) {
		/* give success bits priority over failure bits */
	} else if (hcint & HCINT_STALL) {
		DPRINTF("CH=%d STALL\n", td->channel);
		td->error_stall = 1;
		td->error_any = 1;
		return (0);		/* complete */
	} else if (hcint & HCINT_ERRORS) {
		td->errcnt++;
		if (td->hcsplt != 0 || td->errcnt >= 3) {
			td->error_any = 1;
			return (0);		/* complete */
		}
	}

	/* channel must be disabled before we can complete the transfer */

	if (hcint & (HCINT_ERRORS | HCINT_RETRY | HCINT_ACK | HCINT_NYET)) {

		dwc_otg_host_channel_disable(sc, td->channel);

		if (!(hcint & HCINT_ERRORS))
			td->errcnt = 0;
	}

	switch (td->state) {
	case DWC_CHAN_ST_START:
		goto send_pkt;

	case DWC_CHAN_ST_WAIT_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->offset += td->tx_bytes;
			td->remainder -= td->tx_bytes;
			td->toggle = 1;
			return (0);	/* complete */
		}
		break;
	case DWC_CHAN_ST_WAIT_S_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			goto send_cpkt;
		}
		break;
	case DWC_CHAN_ST_WAIT_C_ANE:
		if (hcint & HCINT_NYET) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			goto send_cpkt;
		}
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & HCINT_ACK) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->offset += td->tx_bytes;
			td->remainder -= td->tx_bytes;
			td->toggle = 1;
			return (0);	/* complete */
		}
		break;
	case DWC_CHAN_ST_TX_PKT_SYNC:
		goto send_pkt_sync;
	default:
		break;
	}
	return (1);		/* busy */

send_pkt:
	if (sizeof(req) != td->remainder) {
		td->error_any = 1;
		return (0);		/* complete */
	}

send_pkt_sync:
	if (td->hcsplt != 0) {
		uint32_t count;

		count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;
		/* check for not first microframe */
		if (count != 0) {
			/* enable SOF interrupt */
			dwc_otg_enable_sof_irq(sc);
			/* set state */
			td->state = DWC_CHAN_ST_TX_PKT_SYNC;
			dwc_otg_host_channel_free(td);
			return (1);	/* busy */
		}

		td->hcsplt &= ~HCSPLT_COMPSPLT;
		td->state = DWC_CHAN_ST_WAIT_S_ANE;
	} else {
		td->state = DWC_CHAN_ST_WAIT_ANE;
	}

	/* XXX Why ? */
	usbd_copy_out(td->buf, 0, &req, sizeof(req));

	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel),
	    (sizeof(req) << HCTSIZ_XFERSIZE_SHIFT) |
	    (1 << HCTSIZ_PKTCNT_SHIFT) |
	    (HCTSIZ_PID_SETUP << HCTSIZ_PID_SHIFT));

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar &= ~HCCHAR_EPDIR_IN;

	/* must enable channel before writing data to FIFO */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

	/* transfer data into FIFO */
	bus_space_write_region_4(sc->sc_iot, sc->sc_ioh,
	    DOTG_DFIFO(td->channel), (uint32_t *)&req, sizeof(req) / 4);

	/* store number of bytes transmitted */
	td->tx_bytes = sizeof(req);


	return (1);	/* busy */

send_cpkt:

	td->hcsplt |= HCSPLT_COMPSPLT;
	td->state = DWC_CHAN_ST_WAIT_C_ANE;

	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel),
	    (HCTSIZ_PID_SETUP << HCTSIZ_PID_SHIFT));

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar &= ~HCCHAR_EPDIR_IN;

	/* must enable channel before writing data to FIFO */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

	return (1);	/* busy */
}

static uint8_t
dwc_otg_host_rate_check(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint8_t ep_type;

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	ep_type = ((td->hcchar &
	    HCCHAR_EPTYPE_MASK) >> HCCHAR_EPTYPE_SHIFT);

	if (sc->sc_chan_state[td->channel].suspended)
		goto busy;

	if (ep_type == UE_ISOCHRONOUS) {
		if (td->tmr_val & 1)
			td->hcchar |= HCCHAR_ODDFRM;
		else
			td->hcchar &= ~HCCHAR_ODDFRM;
		td->tmr_val += td->tmr_res;
	} else if (ep_type == UE_INTERRUPT) {
		uint8_t delta;

		delta = sc->sc_tmr_val - td->tmr_val;
		if (delta >= 128)
			goto busy;
		td->tmr_val = sc->sc_tmr_val + td->tmr_res;
	} else if (td->did_nak != 0) {
		goto busy;
	}

	if (ep_type == UE_ISOCHRONOUS) {
		td->toggle = 0;
	} else if (td->set_toggle) {
		td->set_toggle = 0;
		td->toggle = 1;
	}
	return (0);
busy:
	return (1);
}

static uint8_t
dwc_otg_host_data_rx(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint32_t hcint;
	uint32_t hcchar;
	uint32_t count;
	uint8_t ep_type;

	if (dwc_otg_host_channel_alloc(td))
		return (1);		/* busy */

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	ep_type = ((td->hcchar &
	    HCCHAR_EPTYPE_MASK) >> HCCHAR_EPTYPE_SHIFT);

	hcint = sc->sc_chan_state[td->channel].hcint;

	DPRINTF("CH=%d ST=%d HCINT=0x%08x HCCHAR=0x%08x HCTSIZ=0x%08x\n",
	    td->channel, td->state, hcint,
	    DWC_OTG_READ_4(sc, DOTG_HCCHAR(td->channel)),
	    DWC_OTG_READ_4(sc, DOTG_HCTSIZ(td->channel)));

	/* check interrupt bits */

	if (hcint & (HCINT_RETRY |
	    HCINT_ACK | HCINT_NYET)) {
		/* give success bits priority over failure bits */
	} else if (hcint & HCINT_STALL) {
		DPRINTF("CH=%d STALL\n", td->channel);
		td->error_stall = 1;
		td->error_any = 1;
		return (0);		/* complete */
	} else if (hcint & HCINT_ERRORS) {
		DPRINTF("CH=%d ERROR\n", td->channel);
		td->errcnt++;
		if (td->hcsplt != 0 || td->errcnt >= 3) {
			td->error_any = 1;
			return (0);		/* complete */
		}
	}

	/* channel must be disabled before we can complete the transfer */

	if (hcint & (HCINT_ERRORS | HCINT_RETRY |
	    HCINT_ACK | HCINT_NYET)) {

		dwc_otg_host_channel_disable(sc, td->channel);

		if (!(hcint & HCINT_ERRORS))
			td->errcnt = 0;
	}

	/* check endpoint status */
	if (sc->sc_last_rx_status == 0)
		goto check_state;

	if (GRXSTSRD_CHNUM_GET(sc->sc_last_rx_status) != td->channel)
		goto check_state;

	switch (sc->sc_last_rx_status & GRXSTSRD_PKTSTS_MASK) {
	case GRXSTSRH_IN_DATA:

		DPRINTF("DATA ST=%d STATUS=0x%08x\n",
		    (int)td->state, (int)sc->sc_last_rx_status);

		if (hcint & HCINT_SOFTWARE_ONLY) {
			/*
			 * When using SPLIT transactions on interrupt
			 * endpoints, sometimes data occurs twice.
			 */
			DPRINTF("Data already received\n");
			break;
		}

		td->toggle ^= 1;

		/* get the packet byte count */
		count = GRXSTSRD_BCNT_GET(sc->sc_last_rx_status);

		/* verify the packet byte count */
		if (count != td->max_packet_size) {
			if (count < td->max_packet_size) {
				/* we have a short packet */
				td->short_pkt = 1;
				td->got_short = 1;
			} else {
				/* invalid USB packet */
				td->error_any = 1;

				/* release FIFO */
				dwc_otg_common_rx_ack(sc);
				return (0);	/* we are complete */
			}
		}

		/* verify the packet byte count */
		if (count > td->remainder) {
			/* invalid USB packet */
			td->error_any = 1;

			/* release FIFO */
			dwc_otg_common_rx_ack(sc);
			return (0);		/* we are complete */
		}

		usbd_copy_in(td->buf, td->offset,
		    sc->sc_rx_bounce_buffer, count);
		td->remainder -= count;
		td->offset += count;
		td->actlen += count;
 		hcint |= HCINT_SOFTWARE_ONLY | HCINT_ACK;
		sc->sc_chan_state[td->channel].hcint = hcint;
		break;

	default:
		DPRINTF("OTHER\n");
		break;
	}
	/* release FIFO */
	dwc_otg_common_rx_ack(sc);

check_state:
	switch (td->state) {
	case DWC_CHAN_ST_START:
		if (td->hcsplt != 0)
			goto receive_spkt;
		else
			goto receive_pkt;

	case DWC_CHAN_ST_WAIT_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;

			td->did_nak = 1;
			if (td->hcsplt != 0)
				goto receive_spkt;
			else
				goto receive_pkt;
		}
		if (!(hcint & HCINT_SOFTWARE_ONLY)) {
			if (hcint & HCINT_NYET) {
				if (td->hcsplt != 0) {
					if (!dwc_otg_host_channel_wait(td))
						break;
					goto receive_pkt;
				}
			}
			break;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;

			/* check if we are complete */
			if ((td->remainder == 0) || (td->got_short != 0)) {
				if (td->short_pkt)
					return (0);	/* complete */

				/*
				 * Else need to receive a zero length
				 * packet.
				 */
			}
			if (td->hcsplt != 0)
				goto receive_spkt;
			else
				goto receive_pkt;
		}
		break;

	case DWC_CHAN_ST_WAIT_S_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;

			td->did_nak = 1;
			goto receive_spkt;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			goto receive_pkt;
		}
		break;

	case DWC_CHAN_ST_RX_PKT:
		goto receive_pkt;

	case DWC_CHAN_ST_RX_SPKT:
		goto receive_spkt;

	case DWC_CHAN_ST_RX_SPKT_SYNC:
		goto receive_spkt_sync;

	default:
		break;
	}
	goto busy;

receive_pkt:
	DPRINTF("receive_pkt\n");

	if (td->hcsplt != 0) {
		count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;

		/* check for even microframes */
		if (count == td->curr_frame) {
			td->state = DWC_CHAN_ST_RX_PKT;
			dwc_otg_host_channel_free(td);
			/* enable SOF interrupt */
			dwc_otg_enable_sof_irq(sc);
			goto busy;
		} else if (count == 0) {
			/* check for start split timeout */
			goto receive_spkt;
		}

		td->curr_frame = count;
		td->hcsplt |= HCSPLT_COMPSPLT;
	} else if (dwc_otg_host_rate_check(td)) {
		td->state = DWC_CHAN_ST_RX_PKT;
		dwc_otg_host_channel_free(td);
		goto busy;
	}

	td->state = DWC_CHAN_ST_WAIT_ANE;

	/* receive one packet */
	uint32_t hctsiz =
	    (td->max_packet_size << HCTSIZ_XFERSIZE_SHIFT) |
	    (1 << HCTSIZ_PKTCNT_SHIFT) |
	    (td->toggle ? (HCTSIZ_PID_DATA1 << HCTSIZ_PID_SHIFT) :
	    (HCTSIZ_PID_DATA0 << HCTSIZ_PID_SHIFT));

	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel), hctsiz);

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar |= HCCHAR_EPDIR_IN;

	/* must enable channel before data can be received */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

	goto busy;

receive_spkt:
	if (dwc_otg_host_rate_check(td)) {
		td->state = DWC_CHAN_ST_RX_SPKT;
		dwc_otg_host_channel_free(td);
		goto busy;
	}

receive_spkt_sync:
	if (ep_type == UE_INTERRUPT ||
	    ep_type == UE_ISOCHRONOUS) {
		count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;
		td->curr_frame = count;

		/* check for non-zero microframe */
		if (count != 0) {
			/* enable SOF interrupt */
			dwc_otg_enable_sof_irq(sc);
			/* set state */
			td->state = DWC_CHAN_ST_RX_SPKT_SYNC;
			dwc_otg_host_channel_free(td);
			goto busy;
		}
	} else {
		count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;
		td->curr_frame = count;

		/* check for two last frames */
		if (count >= 6) {
			/* enable SOF interrupt */
			dwc_otg_enable_sof_irq(sc);
			/* set state */
			td->state = DWC_CHAN_ST_RX_SPKT_SYNC;
			dwc_otg_host_channel_free(td);
			goto busy;
		}
	}

	td->hcsplt &= ~HCSPLT_COMPSPLT;
	td->state = DWC_CHAN_ST_WAIT_S_ANE;

	/* receive one packet */
	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel),
	    (td->toggle ? (HCTSIZ_PID_DATA1 << HCTSIZ_PID_SHIFT) :
	    (HCTSIZ_PID_DATA0 << HCTSIZ_PID_SHIFT)));

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar |= HCCHAR_EPDIR_IN;

	/* must enable channel before data can be received */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

busy:
	return (1);	/* busy */
}


static uint8_t
dwc_otg_host_data_tx(struct dwc_otg_td *td)
{
	struct dwc_otg_softc *sc;
	uint32_t count;
	uint32_t hcint;
	uint32_t hcchar;
	uint8_t ep_type;

	if (dwc_otg_host_channel_alloc(td))
		return (1);		/* busy */

	/* get pointer to softc */
	sc = DWC_OTG_TD2SC(td);

	ep_type = ((td->hcchar &
	    HCCHAR_EPTYPE_MASK) >> HCCHAR_EPTYPE_SHIFT);

	hcint = sc->sc_chan_state[td->channel].hcint;

	DPRINTF("CH=%d ST=%d HCINT=0x%08x HCCHAR=0x%08x HCTSIZ=0x%08x\n",
	    td->channel, td->state, hcint,
	    DWC_OTG_READ_4(sc, DOTG_HCCHAR(td->channel)),
	    DWC_OTG_READ_4(sc, DOTG_HCTSIZ(td->channel)));

	if (hcint & (HCINT_RETRY |
	    HCINT_ACK | HCINT_NYET)) {
		/* give success bits priority over failure bits */
	} else if (hcint & HCINT_STALL) {
		DPRINTF("CH=%d STALL\n", td->channel);
		td->error_stall = 1;
		td->error_any = 1;
		return (0);		/* complete */
	} else if (hcint & HCINT_ERRORS) {
		DPRINTF("CH=%d ERROR\n", td->channel);
		td->errcnt++;
		if (td->hcsplt != 0 || td->errcnt >= 3) {
			td->error_any = 1;
			return (0);		/* complete */
		}
	}

	/* channel must be disabled before we can complete the transfer */

	if (hcint & (HCINT_ERRORS | HCINT_RETRY |
	    HCINT_ACK | HCINT_NYET)) {

		dwc_otg_host_channel_disable(sc, td->channel);

		if (!(hcint & HCINT_ERRORS))
			td->errcnt = 0;
	}

	switch (td->state) {
	case DWC_CHAN_ST_START:
		goto send_pkt;

	case DWC_CHAN_ST_WAIT_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;

			td->actlen += td->tx_bytes;
			td->offset += td->tx_bytes;
			td->remainder -= td->tx_bytes;
			td->toggle ^= 1;

			/* check remainder */
			if (td->remainder == 0) {
				if (td->short_pkt)
					return (0);	/* complete */

				/*
				 * Else we need to transmit a short
				 * packet:
				 */
			}
			goto send_pkt;
		}
		break;
	case DWC_CHAN_ST_WAIT_S_ANE:
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & (HCINT_ACK | HCINT_NYET)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			goto send_cpkt;
		}
		break;
	case DWC_CHAN_ST_WAIT_C_ANE:
		if (hcint & HCINT_NYET) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			goto send_cpkt;
		}
		if (hcint & (HCINT_RETRY | HCINT_ERRORS)) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->did_nak = 1;
			goto send_pkt;
		}
		if (hcint & HCINT_ACK) {
			if (!dwc_otg_host_channel_wait(td))
				break;
			td->actlen += td->tx_bytes;
			td->offset += td->tx_bytes;
			td->remainder -= td->tx_bytes;
			td->toggle ^= 1;

			/* check remainder */
			if (td->remainder == 0) {
				if (td->short_pkt)
					return (0);	/* complete */

				/* else we need to transmit a short packet */
			}
			goto send_pkt;
		}
		break;

	case DWC_CHAN_ST_TX_PKT:
		goto send_pkt;

	case DWC_CHAN_ST_TX_PKT_SYNC:
		goto send_pkt_sync;

	case DWC_CHAN_ST_TX_CPKT:
		goto send_cpkt;

	default:
		break;
	}
	goto busy;

send_pkt:
	if (dwc_otg_host_rate_check(td)) {
		td->state = DWC_CHAN_ST_TX_PKT;
		dwc_otg_host_channel_free(td);
		goto busy;
	}

send_pkt_sync:
	if (td->hcsplt != 0) {
 		count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;
		/* check for first or last microframe */
		if (count == 7 || count == 0) {
			/* enable SOF interrupt */
			dwc_otg_enable_sof_irq(sc);
			/* set state */
			td->state = DWC_CHAN_ST_TX_PKT_SYNC;
			dwc_otg_host_channel_free(td);
			goto busy;
		}

		td->hcsplt &= ~HCSPLT_COMPSPLT;
		td->state = DWC_CHAN_ST_WAIT_S_ANE;
	} else {
		td->state = DWC_CHAN_ST_WAIT_ANE;
	}

	/* send one packet at a time */
	count = td->max_packet_size;
	if (td->remainder < count) {
		/* we have a short packet */
		td->short_pkt = 1;
		count = td->remainder;
	}

	/* TODO: HCTSIZ_DOPNG */

	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel),
	    (count << HCTSIZ_XFERSIZE_SHIFT) |
	    (1 << HCTSIZ_PKTCNT_SHIFT) |
	    (td->toggle ? (HCTSIZ_PID_DATA1 << HCTSIZ_PID_SHIFT) :
	    (HCTSIZ_PID_DATA0 << HCTSIZ_PID_SHIFT)));

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar &= ~HCCHAR_EPDIR_IN;

	/* must enable before writing data to FIFO */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

	if (count != 0) {

		/* clear topmost word before copy */
		sc->sc_tx_bounce_buffer[(count - 1) / 4] = 0;

		DPRINTF("send_pkt_sync td->buf %p len %d\n", td->buf, count);

		/* copy out data */
		usbd_copy_out(td->buf, td->offset,
		    sc->sc_tx_bounce_buffer, count);

		/* transfer data into FIFO */
		bus_space_write_region_4(sc->sc_iot, sc->sc_ioh,
		    DOTG_DFIFO(td->channel),
		    sc->sc_tx_bounce_buffer, (count + 3) / 4);
 		bus_space_barrier(sc->sc_iot, sc->sc_ioh,
		    DOTG_DFIFO(td->channel), ((count + 3) / 4) * 4,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	}

	/* store number of bytes transmitted */
	td->tx_bytes = count;
	goto busy;

send_cpkt:
	count = DWC_OTG_READ_4(sc, DOTG_HFNUM) & 7;
	/* check for first microframe */
	if (count == 0) {
		/* send packet again */
		goto send_pkt;
	}

	td->hcsplt |= HCSPLT_COMPSPLT;
	td->state = DWC_CHAN_ST_WAIT_C_ANE;

	DWC_OTG_WRITE_4(sc, DOTG_HCTSIZ(td->channel),
	    (td->toggle ? (HCTSIZ_PID_DATA1 << HCTSIZ_PID_SHIFT) :
	    (HCTSIZ_PID_DATA0 << HCTSIZ_PID_SHIFT)));

	DWC_OTG_WRITE_4(sc, DOTG_HCSPLT(td->channel), td->hcsplt);

	hcchar = td->hcchar;
	hcchar &= ~HCCHAR_EPDIR_IN;

	/* must enable channel before writing data to FIFO */
	DWC_OTG_WRITE_4(sc, DOTG_HCCHAR(td->channel), hcchar);

busy:
	return (1);	/* busy */
}

uint32_t fifoenters;

static uint8_t
dwc_otg_xfer_do_fifo(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_td *td;
	uint8_t toggle;
	uint8_t channel;
	uint8_t tmr_val;
	uint8_t tmr_res;

	DPRINTFN(9, "xfer %p td %p\n", xfer, dxfer->td_transfer_cache);

	td = dxfer->td_transfer_cache;

	while (1) {
		if (td->func(td)) {
			/* operation in progress */
			break;
		}
		if (td == dxfer->td_transfer_last) {
			goto done;
		}
		if (td->error_any) {
			goto done;
		} else if (td->remainder > 0) {
			/*
			 * We had a short transfer. If there is no alternate
			 * next, stop processing !
			 */
			if (!td->alt_next)
				goto done;
		}

		/*
		 * Fetch the next transfer descriptor and transfer
		 * some flags to the next transfer descriptor
		 */
		tmr_res = td->tmr_res;
		tmr_val = td->tmr_val;
		toggle = td->toggle;
		channel = td->channel;
		td = td->obj_next;

		dxfer->td_transfer_cache = td;
		td->toggle = toggle;	/* transfer toggle */
		td->channel = channel;	/* transfer channel */
		td->tmr_res = tmr_res;
		td->tmr_val = tmr_val;
	}
	return (1);			/* not complete */

done:
	/* compute all actual lengths */
	dwc_otg_standard_done(xfer);
	return (0);			/* complete */
}

Static void
dwc_otg_timer_tick(void *_sc)
{
	struct dwc_otg_softc *sc = _sc;

	workqueue_enqueue(sc->sc_wq, (struct work *)&sc->sc_timer_work, NULL);
}

Static void
dwc_otg_timer(struct dwc_otg_softc *sc)
{
	struct dwc_otg_xfer *xfer;
	struct dwc_otg_td *td;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* increment timer value */
	sc->sc_tmr_val++;

	TAILQ_FOREACH(xfer, &sc->sc_active, xnext) {
		td = xfer->td_transfer_cache;
		if (td != NULL)
			td->did_nak = 0;
	}

	/* poll jobs */
	mutex_spin_enter(&sc->sc_intr_lock);
	dwc_otg_interrupt_poll(sc);
	mutex_spin_exit(&sc->sc_intr_lock);

	if (sc->sc_timer_active) {
		/* restart timer */
		callout_reset(&sc->sc_timer, mstohz(DWC_OTG_HOST_TIMER_RATE),
		    dwc_otg_timer_tick, sc);
	}
}

Static void
dwc_otg_timer_start(struct dwc_otg_softc *sc)
{
	if (sc->sc_timer_active != 0)
		return;

	sc->sc_timer_active = 1;

	/* restart timer */
	callout_reset(&sc->sc_timer, mstohz(DWC_OTG_HOST_TIMER_RATE),
	    dwc_otg_timer_tick, sc);
}

Static void
dwc_otg_timer_stop(struct dwc_otg_softc *sc)
{
	if (sc->sc_timer_active == 0)
		return;

	sc->sc_timer_active = 0;

	/* stop timer */
	callout_stop(&sc->sc_timer);
}

void
dwc_otg_interrupt_poll(struct dwc_otg_softc *sc)
{
	uint8_t ch;
	uint32_t temp;
	uint32_t intrs;
	struct dwc_otg_xfer *dxfer, *tmp;
	uint8_t got_rx_status;

// 	DPRINTF("\n");

	KASSERT(mutex_owned(&sc->sc_intr_lock));

repeat:
	/* get all channel interrupts */
	for (ch = 0; ch < sc->sc_host_ch_max; ++ch) {
		intrs = DWC_OTG_READ_4(sc, DOTG_HCINT(ch));
		if (intrs != 0) {
// 			DPRINTF("ch %d intrs %08x\n", ch, intrs);

			DWC_OTG_WRITE_4(sc, DOTG_HCINT(ch), intrs);
			intrs &= ~HCINT_SOFTWARE_ONLY;
			sc->sc_chan_state[ch].hcint |= intrs;
		}
	}

	if (sc->sc_last_rx_status == 0) {
		temp = DWC_OTG_READ_4(sc, DOTG_GINTSTS);
		if (temp & GINTSTS_RXFLVL) {
			/* pop current status */
			sc->sc_last_rx_status =
			    DWC_OTG_READ_4(sc, DOTG_GRXSTSPD);
		}

		if (sc->sc_last_rx_status != 0) {
			uint32_t bcnt;
			uint8_t ep_no;

			temp = sc->sc_last_rx_status &
			    GRXSTSRD_PKTSTS_MASK;

			/* non-data messages we simply skip */
			if (temp != GRXSTSRD_STP_DATA &&
			    temp != GRXSTSRD_OUT_DATA) {
				dwc_otg_common_rx_ack(sc);
				goto repeat;
			}

			bcnt = GRXSTSRD_BCNT_GET(sc->sc_last_rx_status);
			ep_no = GRXSTSRD_CHNUM_GET(sc->sc_last_rx_status);

			/* receive data, if any */
			if (bcnt != 0) {
				DPRINTF("Reading %d bytes from ep %d\n", bcnt,
				    ep_no);
				bus_space_barrier(sc->sc_iot, sc->sc_ioh,
				    DOTG_DFIFO(ep_no), ((bcnt + 3) / 4) * 4,
				    BUS_SPACE_BARRIER_READ);
				bus_space_read_region_4(sc->sc_iot, sc->sc_ioh,
				    DOTG_DFIFO(ep_no), sc->sc_rx_bounce_buffer,
				    (bcnt + 3) / 4);
			}

			/* check if we should dump the data */
			if (!(sc->sc_active_rx_ep & (1U << ep_no))) {
				dwc_otg_common_rx_ack(sc);
				goto repeat;
			}

			got_rx_status = 1;

			DPRINTFN(5, "RX status = 0x%08x: "
			    "ch=%d pid=%d bytes=%d sts=%d\n",
			    sc->sc_last_rx_status, ep_no,
			    (sc->sc_last_rx_status >> 15) & 3,
			    GRXSTSRD_BCNT_GET(sc->sc_last_rx_status),
			    (sc->sc_last_rx_status >> 17) & 15);
		} else {
			got_rx_status = 0;
		}
	} else {
		uint8_t ep_no;

		ep_no = GRXSTSRD_CHNUM_GET(sc->sc_last_rx_status);
		DPRINTF("%s: ep_no %d\n", __func__, ep_no);

		/* check if we should dump the data */
		if (!(sc->sc_active_rx_ep & (1U << ep_no))) {
			dwc_otg_common_rx_ack(sc);
			goto repeat;
		}

		got_rx_status = 1;
	}

	TAILQ_FOREACH_SAFE(dxfer, &sc->sc_active, xnext, tmp) {
		dwc_otg_xfer_do_fifo(&dxfer->xfer);
	}

	if (got_rx_status) {
		/* check if data was consumed */
		if (sc->sc_last_rx_status == 0)
			goto repeat;

		/* disable RX FIFO level interrupt */
		sc->sc_irq_mask &= ~GINTMSK_RXFLVLMSK;
		DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);
	}
}

Static void
dwc_otg_vbus_interrupt(struct dwc_otg_softc *sc)
{
	uint32_t temp;

	temp = DWC_OTG_READ_4(sc, DOTG_GOTGCTL);

	DPRINTFN(15, "GOTGCTL %08x\n", temp);

	if (temp & (GOTGCTL_ASESVLD | GOTGCTL_BSESVLD)) {
		if (!sc->sc_flags.status_vbus) {
			sc->sc_flags.status_vbus = 1;

			/* complete root HUB interrupt endpoint */
			dwc_otg_root_intr(sc);
		}
	} else {
		if (sc->sc_flags.status_vbus) {
			sc->sc_flags.status_vbus = 0;
			sc->sc_flags.status_bus_reset = 0;
			sc->sc_flags.status_suspend = 0;
			sc->sc_flags.change_suspend = 0;
			sc->sc_flags.change_connect = 1;

			/* complete root HUB interrupt endpoint */
			dwc_otg_root_intr(sc);
		}
	}
}

int
dwc_otg_interrupt(struct dwc_otg_softc *sc)
{
	uint32_t status;

	DOTG_EVCNT_INCR(sc->sc_ev_intr);

	/* read and clear interrupt status */
	status = DWC_OTG_READ_4(sc, DOTG_GINTSTS);
	DWC_OTG_WRITE_4(sc, DOTG_GINTSTS, status);

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	if (status & GINTSTS_USBRST) {
		DPRINTF("GINTSTS_USBRST\n");
		/* set correct state */
		sc->sc_flags.status_device_mode = 1;
		sc->sc_flags.status_bus_reset = 0;
		sc->sc_flags.status_suspend = 0;
		sc->sc_flags.change_suspend = 0;
		sc->sc_flags.change_connect = 1;

		/* complete root HUB interrupt endpoint */
		dwc_otg_root_intr(sc);
	}

	/* check for any bus state change interrupts */
	if (status & GINTSTS_ENUMDONE) {
		uint32_t temp;

		DPRINTFN(5, "end of reset\n");

		/* set correct state */
		sc->sc_flags.status_device_mode = 1;
		sc->sc_flags.status_bus_reset = 1;
		sc->sc_flags.status_suspend = 0;
		sc->sc_flags.change_suspend = 0;
		sc->sc_flags.change_connect = 1;
		sc->sc_flags.status_low_speed = 0;
		sc->sc_flags.port_enabled = 1;

		/* reset FIFOs */
		dwc_otg_init_fifo(sc, DWC_MODE_DEVICE);

		/* reset function address */
		dwc_otg_set_address(sc, 0);

		/* figure out enumeration speed */
		temp = DWC_OTG_READ_4(sc, DOTG_DSTS);
		if (DSTS_ENUMSPD_GET(temp) == DSTS_ENUMSPD_HI)
			sc->sc_flags.status_high_speed = 1;
		else
			sc->sc_flags.status_high_speed = 0;

		/* disable resume interrupt and enable suspend interrupt */

		sc->sc_irq_mask &= ~GINTMSK_WKUPINTMSK;
		sc->sc_irq_mask |= GINTMSK_USBSUSPMSK;
		DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);

		/* complete root HUB interrupt endpoint */
		dwc_otg_root_intr(sc);
	}

	if (status & GINTSTS_PRTINT) {
		DPRINTF("GINTSTS_PRTINT\n");
		uint32_t hprt;

		/* Why is this needed ? */
		delay(62000);

		hprt = DWC_OTG_READ_4(sc, DOTG_HPRT);

		/* clear change bits */
		DWC_OTG_WRITE_4(sc, DOTG_HPRT, (hprt & (
			HPRT_PRTPWR | HPRT_PRTENCHNG |
			HPRT_PRTCONNDET | HPRT_PRTOVRCURRCHNG)) |
			sc->sc_hprt_val);

		DPRINTFN(12, "GINTSTS=0x%08x, HPRT=0x%08x\n", status, hprt);

		sc->sc_flags.status_device_mode = 0;

		if (hprt & HPRT_PRTCONNSTS)
			sc->sc_flags.status_bus_reset = 1;
		else
			sc->sc_flags.status_bus_reset = 0;

		if (hprt & HPRT_PRTENCHNG)
			sc->sc_flags.change_enabled = 1;

		if (hprt & HPRT_PRTENA)
			sc->sc_flags.port_enabled = 1;
		else
			sc->sc_flags.port_enabled = 0;

		if (hprt & HPRT_PRTOVRCURRCHNG)
			sc->sc_flags.change_over_current = 1;

		if (hprt & HPRT_PRTOVRCURRACT)
			sc->sc_flags.port_over_current = 1;
		else
			sc->sc_flags.port_over_current = 0;

		if (hprt & HPRT_PRTPWR)
			sc->sc_flags.port_powered = 1;
		else
			sc->sc_flags.port_powered = 0;

		if (((hprt & HPRT_PRTSPD_MASK)
		    >> HPRT_PRTSPD_SHIFT) == HPRT_PRTSPD_LOW)
			sc->sc_flags.status_low_speed = 1;
		else
			sc->sc_flags.status_low_speed = 0;

		if (((hprt & HPRT_PRTSPD_MASK)
		    >> HPRT_PRTSPD_SHIFT) == HPRT_PRTSPD_HIGH)
			sc->sc_flags.status_high_speed = 1;
		else
			sc->sc_flags.status_high_speed = 0;

		if (hprt & HPRT_PRTCONNDET)
			sc->sc_flags.change_connect = 1;

		if (hprt & HPRT_PRTSUSP)
			dwc_otg_suspend_irq(sc);
		else
			dwc_otg_resume_irq(sc);

		/* complete root HUB interrupt endpoint */
		dwc_otg_root_intr(sc);
	}

	/*
	 * If resume and suspend is set at the same time we interpret
	 * that like RESUME. Resume is set when there is at least 3
	 * milliseconds of inactivity on the USB BUS.
	 */
	if (status & GINTSTS_WKUPINT) {
		DPRINTFN(5, "resume interrupt\n");

		dwc_otg_resume_irq(sc);
	}

	if (status & GINTSTS_USBSUSP) {

		DPRINTFN(5, "suspend interrupt\n");

		dwc_otg_suspend_irq(sc);
	}

	/* check VBUS */
	if (status & (GINTSTS_USBSUSP | GINTSTS_USBRST | GINTSTS_OTGINT |
	    GINTSTS_SESSREQINT)) {
		DPRINTFN(5, "vbus interrupt\n");

		dwc_otg_vbus_interrupt(sc);
	}

	/* clear all IN endpoint interrupts */
	if (status & GINTSTS_IEPINT) {
		uint32_t temp;
		uint8_t x;

		DPRINTFN(5, "endpoint interrupt\n");

		for (x = 0; x != sc->sc_dev_in_ep_max; x++) {
			temp = DWC_OTG_READ_4(sc, DOTG_DIEPINT(x));
			if (temp & DIEPMSK_XFERCOMPLMSK) {
				DWC_OTG_WRITE_4(sc, DOTG_DIEPINT(x),
				    DIEPMSK_XFERCOMPLMSK);
			}
		}
	}

	/* check for SOF interrupt */
	if (status & GINTSTS_SOF) {
		if (sc->sc_irq_mask & GINTMSK_SOFMSK) {
			uint8_t x;
			uint8_t y;

			for (x = y = 0; x != sc->sc_host_ch_max; x++) {
				if (sc->sc_chan_state[x].wait_sof != 0) {
					if (--(sc->sc_chan_state[x].wait_sof) != 0)
						y = 1;
				}
			}
			if (y == 0) {
				sc->sc_irq_mask &= ~GINTMSK_SOFMSK;
				DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);
			}
		}
	}

	/* poll FIFO(s) */
	dwc_otg_interrupt_poll(sc);

	return 1;
}

static void
dwc_otg_setup_standard_chain_sub(struct dwc_otg_std_temp *temp)
{
	struct dwc_otg_td *td;

	/* get current Transfer Descriptor */
	td = temp->td_next;
	temp->td = td;

	DPRINTF("td %p buf %p \n", td, temp->buf);
	DPRINTF("td %p func %p offset %08x remainder %08x shrt %d alt %d\n",
	    td, td->func, temp->offset, temp->len, temp->short_pkt,
	    temp->setup_alt_next);
	/* prepare for next TD */
	temp->td_next = td->obj_next;

	/* fill out the Transfer Descriptor */
	td->func = temp->func;
	td->xfer = temp->xfer;
	td->buf = temp->buf;
	td->offset = temp->offset;
	td->remainder = temp->len;
	td->actlen = 0;
	td->tx_bytes = 0;
	td->error_any = 0;
	td->error_stall = 0;
	td->npkt = 0;
	td->did_stall = temp->did_stall;
	td->short_pkt = temp->short_pkt;
	td->alt_next = temp->setup_alt_next;
	td->set_toggle = 0;
	td->got_short = 0;
	td->did_nak = 0;
	td->channel = DWC_OTG_MAX_CHANNELS;
	td->state = 0;
	td->errcnt = 0;
	td->retrycnt = 0;
	td->toggle = 0;
}

Static void
dwc_otg_setup_ctrl_chain(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usb_endpoint_descriptor_t *ed = dpipe->pipe.endpoint->edesc;
	usb_device_request_t *req = &xfer->request;
#ifdef DWC_OTG_DEBUG
	usbd_device_handle dev = dpipe->pipe.device;
#endif
	uint8_t dir = req->bmRequestType & UT_READ;
	uint32_t len = UGETW(req->wLength);
	struct dwc_otg_std_temp temp;	/* XXX */
	struct dwc_otg_td *td;
	int done;

	DPRINTFN(3, "type=0x%02x, request=0x%02x, wValue=0x%04x,"
	    "wIndex=0x%04x len=%d, addr=%d, endpt=%d, dir=%s, speed=%d\n",
	    req->bmRequestType, req->bRequest, UGETW(req->wValue),
	    UGETW(req->wIndex), UGETW(req->wLength), dev->address,
	    UE_GET_ADDR(ed->bEndpointAddress), dir == UT_READ ? "in" :"out",
	    dev->speed);

	temp.max_frame_size = UGETW(ed->wMaxPacketSize);

	td = dxfer->td_start[0];
	dxfer->td_transfer_first = td;
	dxfer->td_transfer_cache = td;

	/* Setup stage */
	temp.xfer = xfer;
	temp.buf = NULL;
	temp.td = NULL;
	temp.td_next = td;
	temp.offset = 0;
	temp.setup_alt_next = 0/*(xfer->flags & USBD_SHORT_XFER_OK)*/;
	temp.did_stall = 0; /* !xfer->flags_int.control_stall; */

	temp.func = &dwc_otg_host_setup_tx;
	temp.len = sizeof(*req);
	temp.buf = req;
	temp.short_pkt = 1;		/* We're 8 bytes this is short for HS */
	temp.setup_alt_next = 0;	/* XXXNH */

	DPRINTF("SE temp.len %d temp.pc %p\n", temp.len, temp.buf);

	dwc_otg_setup_standard_chain_sub(&temp);

	done = 0;
	KASSERT((temp.buf == NULL) == (temp.len == 0));
	if (dir == UT_READ) {
		temp.func = &dwc_otg_host_data_rx;
	} else {
		temp.func = &dwc_otg_host_data_tx;
	}

	/* Optional Data stage */
	while (done != len) {

		/* DATA0 / DATA1 message */

		temp.buf = len ? KERNADDR(&xfer->dmabuf, done) : NULL;
		temp.len = len - done;
		temp.short_pkt = ( (xfer->flags & USBD_FORCE_SHORT_XFER) ? 0 : 1);

		if (temp.len > UGETW(ed->wMaxPacketSize))
			temp.len = UGETW(ed->wMaxPacketSize);

		dwc_otg_setup_standard_chain_sub(&temp);

		done += temp.len;
		if (temp.len)
			temp.buf = (char *)KERNADDR(&xfer->dmabuf, 0) + done;
	};

	/* Status Stage */
	temp.buf = &req;	/* XXXNH not needed */
	temp.len = 0;
	temp.short_pkt = 0;
	temp.setup_alt_next = 0;

	/*
	 * Send a DATA1 message and invert the current endpoint direction.
	 */
	if (dir == UT_READ) {
		temp.func = &dwc_otg_host_data_tx;
	} else {
		temp.func = &dwc_otg_host_data_rx;
	}

	dwc_otg_setup_standard_chain_sub(&temp);

	/* data toggle should be DATA1 */
	td = temp.td;
	td->set_toggle = 1;

	/* must have at least one frame! */
	td = temp.td;
	dxfer->td_transfer_last = td;

	dwc_otg_setup_standard_chain(xfer);
}

Static void
dwc_otg_setup_data_chain(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usb_endpoint_descriptor_t *ed = dpipe->pipe.endpoint->edesc;
#ifdef DWC_OTG_DEBUG
	usbd_device_handle dev = dpipe->pipe.device;
#endif
	uint8_t dir = UE_GET_DIR(ed->bEndpointAddress);
	struct dwc_otg_std_temp temp;	/* XXX */
	struct dwc_otg_td *td;
	int off;
	int done;
	int len;

	DPRINTFN(3, "xfer=%p, len=%d, flags=%d, addr=%d, endpt=%d, dir %s\n",
	    xfer, xfer->length, xfer->flags, dev->address,
	    UE_GET_ADDR(ed->bEndpointAddress), dir == UT_READ ? "in" :"out");

	temp.max_frame_size = UGETW(ed->wMaxPacketSize);

	td = dxfer->td_start[0];
	dxfer->td_transfer_first = td;
	dxfer->td_transfer_cache = td;

	temp.xfer = xfer;
	temp.td = NULL;
	temp.td_next = td;
	temp.offset = 0;
	//temp.setup_alt_next = (xfer->flags & USBD_SHORT_XFER_OK) ? 1 : 0;
	temp.setup_alt_next = 0;
	temp.did_stall = 0; /* !xfer->flags_int.control_stall; */
	temp.func = NULL;

	done = 0;
	if (dir == UE_DIR_IN) {
		temp.func = &dwc_otg_host_data_rx;
	} else {
		temp.func = &dwc_otg_host_data_tx;
	}

	/* Data stage */
	off = 0;
	len = xfer->length;
	while (len > 0) {
		/* DATA0 / DATA1 message */
		temp.buf = KERNADDR(&xfer->dmabuf, off);
		temp.len = MIN(len, UGETW(ed->wMaxPacketSize));
		temp.short_pkt = (xfer->flags & USBD_FORCE_SHORT_XFER) ? 0 : 1;
		if (len <= UGETW(ed->wMaxPacketSize))
			temp.setup_alt_next = 0;

		dwc_otg_setup_standard_chain_sub(&temp);

		len -= temp.len;
		off += temp.len;
	};

	/* must have at least one frame! */
	td = temp.td;
	dxfer->td_transfer_last = td;

	dwc_otg_setup_standard_chain(xfer);
}

Static void
dwc_otg_setup_intr_chain(usbd_xfer_handle xfer)
{
	dwc_otg_setup_data_chain(xfer);
}

Static void
dwc_otg_setup_bulk_chain(usbd_xfer_handle xfer)
{
	dwc_otg_setup_data_chain(xfer);
}

#if 0
Static void
dwc_otg_setup_isoc_chain(usbd_xfer_handle xfer)
{
}
#endif

Static void
dwc_otg_setup_standard_chain(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usb_endpoint_descriptor_t *ed = dpipe->pipe.endpoint->edesc;
	usbd_device_handle dev = dpipe->pipe.device;
	usb_device_request_t *req = &xfer->request;
	struct dwc_otg_softc *sc = dev->bus->hci_private;
	uint8_t addr = dev->address;
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	uint8_t epnum = UE_GET_ADDR(ed->bEndpointAddress);
	struct dwc_otg_td *td;
	uint32_t hcchar;
	uint32_t hcsplt;
	uint32_t ival;

// 	DPRINTF(("%s: xfer->length %d\n", __func__, xfer->length));

	/* get first again */
	td = dxfer->td_transfer_first;
	td->toggle = dpipe->pipe.endpoint->datatoggle;

	hcsplt = 0;
	hcchar = HCCHAR_CHENA |
	    (addr << HCCHAR_DEVADDR_SHIFT) |
	    (xfertype << HCCHAR_EPTYPE_SHIFT) |
	    (epnum << HCCHAR_EPNUM_SHIFT) |
	    ((UGETW(ed->wMaxPacketSize)) << HCCHAR_MPS_SHIFT);

	switch (xfertype) {
	case UE_CONTROL:
		if ((req->bmRequestType & UT_READ) == UT_READ) {
			hcchar |= HCCHAR_EPDIR_IN;
		}
		break;
		
	case UE_INTERRUPT:
	case UE_BULK:
	case UE_ISOCHRONOUS:
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN) {
			hcchar |= HCCHAR_EPDIR_IN;
		}
		break;
	default:
		panic("Unknown transfer type");
	}

	switch (dev->speed) {
	case USB_SPEED_LOW:
		DPRINTF("USB_SPEED_LOW\n");
		hcchar |= HCCHAR_LSPDDEV;
		/* FALLTHROUGH */
	case USB_SPEED_FULL:
		DPRINTF("USB_SPEED_FULL\n");
		/* check if root HUB port is running High Speed */
		if (sc->sc_flags.status_high_speed != 0) {
			hcsplt = HCSPLT_SPLTENA |
			    (dev->myhsport->portno << HCSPLT_PRTADDR_SHIFT) |
			    (dev->myhsport->parent->address << HCSPLT_HUBADDR_SHIFT);
			if (xfertype == UE_ISOCHRONOUS)  /* XXX */
				hcsplt |= (3 << HCSPLT_XACTPOS_SHIFT);
		}
		break;

	case USB_SPEED_HIGH:
		DPRINTF("USB_SPEED_HIGH\n");
		if (xfertype == UE_ISOCHRONOUS || xfertype == UE_INTERRUPT) {
			hcchar |= (/*(xfer->max_packet_count & 3)*/ 1
				<< HCCHAR_MC_SHIFT);
		}
		break;

	default:
		break;
	}

	int fps_shift = 1;

	switch (xfertype) {
	case UE_ISOCHRONOUS:
// 		td->tmr_val = xfer->endpoint->isoc_next & 0xFF;

		ival = 1 << fps_shift;
		break;
	case UE_INTERRUPT:
		ival = dpipe->pipe.interval / DWC_OTG_HOST_TIMER_RATE;
		if (ival == 0)
			ival = 1;
		else if (ival > 127)
			ival = 127;
		td->tmr_val = sc->sc_tmr_val + ival;
		td->tmr_res = ival;
		break;
	default:
		td->tmr_val = 0;
		td->tmr_res = 0;
		ival = 0;
	}

	DPRINTF("hcchar 0x%08x hcchar 0x%08x ival %d\n", hcchar, hcsplt, ival);

	/* store configuration in all TD's */
	while (1) {
		td->hcchar = hcchar;
		td->hcsplt = hcsplt;

		if (td == dxfer->td_transfer_last)
			break;

		td = td->obj_next;
	}
}

Static void
dwc_otg_start_standard_chain(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	usbd_device_handle dev = dpipe->pipe.device;
	struct dwc_otg_softc *sc = dev->bus->hci_private;

	DPRINTFN(9, "\n");

	/* poll one time - will turn on interrupts */
	mutex_spin_enter(&sc->sc_intr_lock);
	if (dwc_otg_xfer_do_fifo(xfer)) {

		KASSERT(mutex_owned(&sc->sc_lock));
		/* put transfer on interrupt queue */

		if (!dxfer->queued) {
			dxfer->queued = true;
			TAILQ_INSERT_TAIL(&sc->sc_active, dxfer, xnext);
		} else {
			printf("%s: xfer %p already queued\n", __func__,
			    xfer);
		}

		/* start timeout, if any */
		if (xfer->timeout != 0) {
			callout_reset(&xfer->timeout_handle,
			    mstohz(xfer->timeout), dwc_otg_timeout, xfer);
		}
	}
	mutex_spin_exit(&sc->sc_intr_lock);

	DPRINTFN(9, "done\n");
}

Static void
dwc_otg_rhc(void *addr)
{
	struct dwc_otg_softc *sc = addr;
	usbd_xfer_handle xfer;
	usbd_pipe_handle pipe;
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
	pipe = xfer->pipe;

	p = KERNADDR(&xfer->dmabuf, 0);

	p[0] = 0x02;	/* we only have one port (1 << 1) */

	xfer->actlen = xfer->length;
	xfer->status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);

}

Static usbd_status
dwc_otg_standard_done_sub(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	usbd_pipe_handle pipe = xfer->pipe;
	struct dwc_otg_td *td;
	uint32_t len;
	usbd_status error;

	DPRINTFN(9, "td %p\n", dxfer->td_transfer_cache);

	td = dxfer->td_transfer_cache;

	do {
		xfer->actlen += td->actlen;

		len = td->remainder;

		/* store last data toggle */
		pipe->endpoint->datatoggle = td->toggle;

		/* Check for transfer error */
		if (td->error_any) {
			/* the transfer is finished */
			error = (td->error_stall ? USBD_STALLED : USBD_IOERROR);
			td = NULL;
			break;
		}
		/* Check for short transfer */
		if (len > 0) {
			//if (xfer->flags & USBD_SHORT_XFER_OK) {
			if (0) {
				/* follow alt next */
				if (td->alt_next) {
					td = td->obj_next;
				} else {
					td = NULL;
				}
			} else {
				/* the transfer is finished */
				td = NULL;
			}
			error = 0;
			break;
		}

		td = td->obj_next;

		/* this USB frame is complete */
		error = 0;
		break;

	} while (0);

	/* update transfer cache */

	dxfer->td_transfer_cache = td;

	return (error);
}

Static void
dwc_otg_standard_done(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = DWC_OTG_XFER2DXFER(xfer);

	struct dwc_otg_td *td;
	usbd_status err = 0;

	DPRINTFN(13, "xfer=%p endpoint=%p transfer done\n",
	    xfer, xfer->pipe->endpoint);

	/* reset scanner */

	dxfer->td_transfer_cache = dxfer->td_transfer_first;
	td = dxfer->td_transfer_first;

	while (td != NULL) {
		err = dwc_otg_standard_done_sub(xfer);
		if (dxfer->td_transfer_cache == NULL) {
			goto done;
		}
		if (td == dxfer->td_transfer_last)
			break;
		td = td->obj_next;
	};
done:
	dwc_otg_device_done(xfer, err);
}


/*------------------------------------------------------------------------*
 *	dwc_otg_device_done
 *
 * NOTE: this function can be called more than one time on the
 * same USB transfer!
 *------------------------------------------------------------------------*/
Static void
dwc_otg_device_done(usbd_xfer_handle xfer, usbd_status error)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_softc *sc = DWC_OTG_XFER2SC(xfer);

	DPRINTFN(9, "xfer=%p, endpoint=%p, error=%d\n",
	    xfer, xfer->pipe->endpoint, error);
	struct dwc_otg_td *td;

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	td = dxfer->td_transfer_first;

	if (td != NULL)
		dwc_otg_host_channel_free(td);

	xfer->status = error;
	TAILQ_REMOVE(&sc->sc_active, dxfer, xnext);

	callout_stop(&xfer->timeout_handle);

	dxfer->queued = false;
	TAILQ_INSERT_TAIL(&sc->sc_complete, dxfer, xnext);

	usb_schedsoftintr(&sc->sc_bus);
}

usbd_status
dwc_otg_init(struct dwc_otg_softc *sc)
{
	const char * const xname = device_xname(sc->sc_dev);
	uint32_t temp;

	sc->sc_bus.hci_private = sc;
	sc->sc_bus.usbrev = USBREV_2_0;
	sc->sc_bus.methods = &dwc_otg_bus_methods;
	sc->sc_bus.pipe_size = sizeof(struct dwc_otg_pipe);

	sc->sc_noport = 1;

	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	TAILQ_INIT(&sc->sc_active);
	TAILQ_INIT(&sc->sc_complete);

	sc->sc_tdpool = pool_cache_init(sizeof(struct dwc_otg_td), 0, 0, 0,
	    "dotgtd", NULL, IPL_USB, NULL, NULL, NULL);

	sc->sc_rhc_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    dwc_otg_rhc, sc);

	workqueue_create(&sc->sc_wq, xname, dwc_otg_worker, sc, PRI_NONE,
	    IPL_USB, WQ_MPSAFE);
	sc->sc_timer_work.sc = sc;

	usb_setup_reserve(sc->sc_dev, &sc->sc_dma_reserve, sc->sc_bus.dmatag,
	    USB_MEM_RESERVE);
	
#ifdef DOTG_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "intr");
	evcnt_attach_dynamic(&sc->sc_ev_soft_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "soft intr");
	evcnt_attach_dynamic(&sc->sc_ev_work, EVCNT_TYPE_MISC,
	    NULL, xname, "work items");

	evcnt_attach_dynamic(&sc->sc_ev_tdpoolget, EVCNT_TYPE_MISC,
	    NULL, xname, "pool get");
	evcnt_attach_dynamic(&sc->sc_ev_tdpoolput, EVCNT_TYPE_MISC,
	    NULL, xname, "pool put");
#endif

	temp = DWC_OTG_READ_4(sc, DOTG_GUSBCFG);
	temp |= GUSBCFG_FORCEHOSTMODE;
	temp &= ~GUSBCFG_TERMSELDLPULSE;
	temp &= ~GUSBCFG_USBTRDTIM_MASK;
	temp |= GUSBCFG_TRD_TIM_SET(9);
	temp |= GUSBCFG_HNPCAP | GUSBCFG_SRPCAP;

	DWC_OTG_WRITE_4(sc, DOTG_GUSBCFG, temp);
	delay(100000);

	temp = DWC_OTG_READ_4(sc, DOTG_GUSBCFG);
	temp &= ~GUSBCFG_FORCEHOSTMODE;
	DWC_OTG_WRITE_4(sc, DOTG_GUSBCFG, temp);
	delay(100000);

	temp = DWC_OTG_READ_4(sc, DOTG_GSNPSID);
	DPRINTF("Version =          0x%08x\n", temp);
	switch (temp & 0xfffff000) {
	case 0x4f542000:
	case 0x4f543000:
		break;
	default:
		printf("oops\n");
		return 1;
	}

	temp =	DWC_OTG_READ_4(sc, DOTG_GAHBCFG);
	temp &= ~GAHBCFG_GLBLINTRMSK;
	DWC_OTG_WRITE_4(sc, DOTG_GAHBCFG, temp);

	temp = 	DWC_OTG_READ_4(sc, DOTG_GUSBCFG);
	temp &= ~GUSBCFG_ULPIEXTVBUSDRV;
	temp &= ~GUSBCFG_TERMSELDLPULSE;

	DWC_OTG_WRITE_4(sc, DOTG_GUSBCFG, temp);

  	dwc_otg_core_reset(sc);

	/* --------------------------------*/

	DWC_OTG_WRITE_4(sc, DOTG_PCGCCTL, 0);

	temp = 0;
// 	temp = DWC_OTG_READ_4(sc, DOTG_HCFG);
	temp &= ~HCFG_FSLSPCLKSEL_MASK;
	temp |= 1; /* 30 or 60 Mhz */

	DWC_OTG_WRITE_4(sc, DOTG_HCFG, temp);

	/* enable PORT reset */
	temp = DWC_OTG_READ_4(sc, DOTG_HPRT);
	temp |= HPRT_PRTRST;
	DWC_OTG_WRITE_4(sc, DOTG_HPRT, temp);

	delay(1000);
	temp &= ~HPRT_PRTRST;
	DWC_OTG_WRITE_4(sc, DOTG_HPRT, temp);
	delay(1000);

	temp = DWC_OTG_READ_4(sc, DOTG_HPRT);

	usb_delay_ms(&sc->sc_bus, 8);

	sc->sc_mode = DWC_MODE_HOST;

	switch (sc->sc_mode) {
	case DWC_MODE_DEVICE:
		temp = GUSBCFG_FORCEDEVMODE;
		break;
	case DWC_MODE_HOST:
		temp = GUSBCFG_FORCEHOSTMODE;
		break;
	default:
		temp = 0;
		break;
	}

	/* flag based? */
#ifdef DWC_OTG_USE_HSIC
	DWC_OTG_WRITE_4(sc, DOTG_GUSBCFG,
		GUSBCFG_PHYIF |
		GUSBCFG_TRD_TIM_SET(5) | temp);
	DWC_OTG_WRITE_4(sc, DOTG_GOTGCTL, 0x000000ec);

	temp = DWC_OTG_READ_4(sc, DOTG_GLPMCFG);
	DWC_OTG_WRITE_4(sc, DOTG_GLPMCFG, temp & ~GLPMCFG_HSIC_CONN);
	DWC_OTG_WRITE_4(sc, DOTG_GLPMCFG, temp | GLPMCFG_HSIC_CONN);
#else
	DWC_OTG_WRITE_4(sc, DOTG_GUSBCFG,
		GUSBCFG_ULPI_UTMI_SEL |
		GUSBCFG_TRD_TIM_SET(5) | temp);
	DWC_OTG_WRITE_4(sc, DOTG_GOTGCTL, 0);

	temp = DWC_OTG_READ_4(sc, DOTG_GLPMCFG);
	DWC_OTG_WRITE_4(sc, DOTG_GLPMCFG, temp & ~GLPMCFG_HSIC_CONN);
#endif

	/* clear global nak */
	DWC_OTG_WRITE_4(sc, DOTG_DCTL, DCTL_CGOUTNAK | DCTL_CGNPINNAK);

	/* disable USB port */
	DWC_OTG_WRITE_4(sc, DOTG_PCGCCTL, 0xffffffff);
	usb_delay_ms(&sc->sc_bus, 10);

	/* enable USB port */
	DWC_OTG_WRITE_4(sc, DOTG_PCGCCTL, 0);

	/* wait 10ms */
	usb_delay_ms(&sc->sc_bus, 10);

	/* pull up D+ */
	dwc_otg_pull_up(sc);

	temp = DWC_OTG_READ_4(sc, DOTG_GHWCFG3);

	sc->sc_fifo_size = 4 * GHWCFG3_DFIFODEPTH_GET(temp);

	temp = DWC_OTG_READ_4(sc, DOTG_GHWCFG2);

	sc->sc_dev_ep_max = min(GHWCFG2_NUMDEVEPS_GET(temp),DWC_OTG_MAX_ENDPOINTS);
	sc->sc_host_ch_max = min(GHWCFG2_NUMHSTCHNL_GET(temp),DWC_OTG_MAX_CHANNELS);

	temp = DWC_OTG_READ_4(sc, DOTG_GHWCFG4);
	sc->sc_dev_in_ep_max = GHWCFG4_NUM_IN_EP_GET(temp);

	DPRINTF("Total FIFO size = %d bytes, Device EPs = %d/%d Host CHs = %d\n",
		sc->sc_fifo_size, sc->sc_dev_ep_max, sc->sc_dev_in_ep_max,
		sc->sc_host_ch_max);

	/* setup fifo */
	if (dwc_otg_init_fifo(sc, DWC_MODE_OTG))
		return EINVAL;

	DWC_OTG_WRITE_4(sc, DOTG_GOTGINT, 0xffffffff);
	DWC_OTG_WRITE_4(sc, DOTG_GINTSTS, 0xffffffff);

	/* enable interrupts */
	sc->sc_irq_mask = DWC_OTG_MSK_GINT_ENABLED;
	DWC_OTG_WRITE_4(sc, DOTG_GINTMSK, sc->sc_irq_mask);

	if (sc->sc_mode == DWC_MODE_OTG || sc->sc_mode == DWC_MODE_DEVICE) {

		/* enable all endpoint interrupts */
		temp = DWC_OTG_READ_4(sc, DOTG_GHWCFG2);
		if (temp & GHWCFG2_MPI) {
			uint8_t x;

			DPRINTF("Multi Process Interrupts\n");

			for (x = 0; x != sc->sc_dev_in_ep_max; x++) {
				DWC_OTG_WRITE_4(sc, DOTG_DIEPEACHINTMSK(x),
				    DIEPMSK_XFERCOMPLMSK);
				DWC_OTG_WRITE_4(sc, DOTG_DOEPEACHINTMSK(x), 0);
			}
			DWC_OTG_WRITE_4(sc, DOTG_DEACHINTMSK, 0xFFFF);
		} else {
			DWC_OTG_WRITE_4(sc, DOTG_DIEPMSK,
			    DIEPMSK_XFERCOMPLMSK);
			DWC_OTG_WRITE_4(sc, DOTG_DOEPMSK, 0);
			DWC_OTG_WRITE_4(sc, DOTG_DAINTMSK, 0xFFFF);
		}
	}

	if (sc->sc_mode == DWC_MODE_OTG || sc->sc_mode == DWC_MODE_HOST) {
		/* setup clocks */
		temp = DWC_OTG_READ_4(sc, DOTG_HCFG);
		temp &= ~(HCFG_FSLSSUPP | HCFG_FSLSPCLKSEL_MASK);
		temp |= (1 << HCFG_FSLSPCLKSEL_SHIFT);
		DWC_OTG_WRITE_4(sc, DOTG_HCFG, temp);
	}

	/* only enable global IRQ */
	DWC_OTG_WRITE_4(sc, DOTG_GAHBCFG, GAHBCFG_GLBLINTRMSK);

	/* turn off clocks */
	dwc_otg_clocks_off(sc);

	/* read initial VBUS state */

	temp = DWC_OTG_READ_4(sc, DOTG_GOTGCTL);

	DPRINTFN(5, "GOTGCTL=0x%08x\n", temp);

	dwc_otg_vbus_interrupt(sc);

	mutex_enter(&sc->sc_lock);
	/* catch any lost interrupts */
	dwc_otg_do_poll(&sc->sc_bus);
	mutex_exit(&sc->sc_lock);

	return 0;
}

/***********************************************************************/

Static void
dwc_otg_xfer_setup(usbd_xfer_handle xfer)
{
	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;
	usb_endpoint_descriptor_t *ed = dpipe->pipe.endpoint->edesc;
	uint16_t mps = UGETW(ed->wMaxPacketSize);
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	uint8_t ep_no = UE_GET_ADDR(ed->bEndpointAddress);
	void *last_obj;
	int ntd, n;

	dxfer->work.sc = sc;
	dxfer->work.xfer = xfer;

	/*
	 * compute maximum number of TDs
	 */
	if (xfertype == UE_CONTROL) {
		ntd = howmany(xfer->length, mps) + 1 /* STATUS */ + 1 /* SYNC 1 */
		    + 1 /* SYNC 2 */ + 1 /* SYNC 3 */;
	} else {
		ntd = howmany(xfer->length, mps) + 1 /* SYNC */ ;
	}

	/*
	 * allocate transfer descriptors
	 */
	last_obj = NULL;
	for (n = 0; n != ntd; n++) {
		struct dwc_otg_td *td;

		DOTG_EVCNT_INCR(sc->sc_ev_tdpoolget);

		td = pool_cache_get(sc->sc_tdpool, PR_NOWAIT);
		if (td == NULL) {
			printf("%s: pool empty\n", __func__);
			goto done;
		}

		/* init TD */
		memset(td, 0, sizeof(*td));
		td->max_packet_size = UGETW(ed->wMaxPacketSize);
		td->ep_no = ep_no;
		td->obj_next = last_obj;
		td->channel = DWC_OTG_MAX_CHANNELS;

		last_obj = td;
	}

done:
	dxfer->td_start[0] = last_obj;
}

Static void
dwc_otg_xfer_start(usbd_xfer_handle xfer)
{
 	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_bus.use_polling) {
		dwc_otg_start_standard_chain(xfer);
	} else {
		workqueue_enqueue(sc->sc_wq,
		    (struct work *)&dxfer->work, NULL);
	}
}

Static void
dwc_otg_xfer_end(usbd_xfer_handle xfer)
{
 	struct dwc_otg_xfer *dxfer = (struct dwc_otg_xfer *)xfer;
	struct dwc_otg_pipe *dpipe = (struct dwc_otg_pipe *)xfer->pipe;
	struct dwc_otg_softc *sc = dpipe->pipe.device->bus->hci_private;
	struct dwc_otg_td *td, *td_next;

	DPRINTF("\n");

	for (td = dxfer->td_start[0]; td; ) {
		td_next = td->obj_next;
		DOTG_EVCNT_INCR(sc->sc_ev_tdpoolput);

		pool_cache_put(sc->sc_tdpool, td);
		td = td_next;
	}
}
