/*	$NetBSD: motg.c,v 1.20 2018/04/08 13:36:37 jmcneill Exp $	*/

/*
 * Copyright (c) 1998, 2004, 2011, 2012, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, Jared D. McNeill (jmcneill@invisible.ca),
 * Matthew R. Green (mrg@eterna.com.au), and Manuel Bouyer (bouyer@netbsd.org).
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
 * This file contains the driver for the Mentor Graphics Inventra USB
 * 2.0 High Speed Dual-Role controller.
 *
 * NOTE: The current implementation only supports Device Side Mode!
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: motg.c,v 1.20 2018/04/08 13:36:37 jmcneill Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/motgreg.h>
#include <dev/usb/motgvar.h>
#include <dev/usb/usbroothub.h>

#ifdef USB_DEBUG
#ifndef MOTG_DEBUG
#define motgdebug 0
#else
int motgdebug = 0;

SYSCTL_SETUP(sysctl_hw_motg_setup, "sysctl hw.motg setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "motg",
	    SYSCTL_DESCR("motg global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &motgdebug, sizeof(motgdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* MOTG_DEBUG */
#endif /* USB_DEBUG */

#define MD_ROOT 0x0002
#define MD_CTRL 0x0004
#define MD_BULK 0x0008

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(motgdebug,1,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGM(motgdebug,N,FMT,A,B,C,D)
#define	MOTGHIST_FUNC()		USBHIST_FUNC()
#define	MOTGHIST_CALLED(name)	USBHIST_CALLED(motgdebug)


/* various timeouts, for various speeds */
/* control NAK timeouts */
#define NAK_TO_CTRL	10	/* 1024 frames, about 1s */
#define NAK_TO_CTRL_HIGH 13	/* 8k microframes, about 0.8s */

/* intr/iso polling intervals */
#define POLL_TO		100	/* 100 frames, about 0.1s */
#define POLL_TO_HIGH	10	/* 100 microframes, about 0.12s */

/* bulk NAK timeouts */
#define NAK_TO_BULK	0 /* disabled */
#define NAK_TO_BULK_HIGH 0

static void 		motg_hub_change(struct motg_softc *);

static usbd_status	motg_root_intr_transfer(struct usbd_xfer *);
static usbd_status	motg_root_intr_start(struct usbd_xfer *);
static void		motg_root_intr_abort(struct usbd_xfer *);
static void		motg_root_intr_close(struct usbd_pipe *);
static void		motg_root_intr_done(struct usbd_xfer *);

static usbd_status	motg_open(struct usbd_pipe *);
static void		motg_poll(struct usbd_bus *);
static void		motg_softintr(void *);
static struct usbd_xfer *
			motg_allocx(struct usbd_bus *, unsigned int);
static void		motg_freex(struct usbd_bus *, struct usbd_xfer *);
static void		motg_get_lock(struct usbd_bus *, kmutex_t **);
static int		motg_roothub_ctrl(struct usbd_bus *, usb_device_request_t *,
			    void *, int);

static void		motg_noop(struct usbd_pipe *pipe);
static usbd_status	motg_portreset(struct motg_softc*);

static usbd_status	motg_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status	motg_device_ctrl_start(struct usbd_xfer *);
static void		motg_device_ctrl_abort(struct usbd_xfer *);
static void		motg_device_ctrl_close(struct usbd_pipe *);
static void		motg_device_ctrl_done(struct usbd_xfer *);
static usbd_status	motg_device_ctrl_start1(struct motg_softc *);
static void		motg_device_ctrl_read(struct usbd_xfer *);
static void		motg_device_ctrl_intr_rx(struct motg_softc *);
static void		motg_device_ctrl_intr_tx(struct motg_softc *);

static usbd_status	motg_device_data_transfer(struct usbd_xfer *);
static usbd_status	motg_device_data_start(struct usbd_xfer *);
static usbd_status	motg_device_data_start1(struct motg_softc *,
			    struct motg_hw_ep *);
static void		motg_device_data_abort(struct usbd_xfer *);
static void		motg_device_data_close(struct usbd_pipe *);
static void		motg_device_data_done(struct usbd_xfer *);
static void		motg_device_intr_rx(struct motg_softc *, int);
static void		motg_device_intr_tx(struct motg_softc *, int);
static void		motg_device_data_read(struct usbd_xfer *);
static void		motg_device_data_write(struct usbd_xfer *);

static void		motg_device_clear_toggle(struct usbd_pipe *);
static void		motg_device_xfer_abort(struct usbd_xfer *);

#define UBARR(sc) bus_space_barrier((sc)->sc_iot, (sc)->sc_ioh, 0, (sc)->sc_size, \
			BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)
#define UWRITE1(sc, r, x) \
 do { UBARR(sc); bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)
#define UWRITE2(sc, r, x) \
 do { UBARR(sc); bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)
#define UWRITE4(sc, r, x) \
 do { UBARR(sc); bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)

static __inline uint32_t
UREAD1(struct motg_softc *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, r);
}
static __inline uint32_t
UREAD2(struct motg_softc *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, r);
}

#if 0
static __inline uint32_t
UREAD4(struct motg_softc *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, r);
}
#endif

static void
musbotg_pull_common(struct motg_softc *sc, uint8_t on)
{
	uint8_t val;

	val = UREAD1(sc, MUSB2_REG_POWER);
	if (on)
		val |= MUSB2_MASK_SOFTC;
	else
		val &= ~MUSB2_MASK_SOFTC;

	UWRITE1(sc, MUSB2_REG_POWER, val);
}

const struct usbd_bus_methods motg_bus_methods = {
	.ubm_open =	motg_open,
	.ubm_softint =	motg_softintr,
	.ubm_dopoll =	motg_poll,
	.ubm_allocx =	motg_allocx,
	.ubm_freex =	motg_freex,
	.ubm_getlock =	motg_get_lock,
	.ubm_rhctrl =	motg_roothub_ctrl,
};

const struct usbd_pipe_methods motg_root_intr_methods = {
	.upm_transfer =	motg_root_intr_transfer,
	.upm_start =	motg_root_intr_start,
	.upm_abort =	motg_root_intr_abort,
	.upm_close =	motg_root_intr_close,
	.upm_cleartoggle =	motg_noop,
	.upm_done =	motg_root_intr_done,
};

const struct usbd_pipe_methods motg_device_ctrl_methods = {
	.upm_transfer =	motg_device_ctrl_transfer,
	.upm_start =	motg_device_ctrl_start,
	.upm_abort =	motg_device_ctrl_abort,
	.upm_close =	motg_device_ctrl_close,
	.upm_cleartoggle =	motg_noop,
	.upm_done =	motg_device_ctrl_done,
};

const struct usbd_pipe_methods motg_device_data_methods = {
	.upm_transfer =	motg_device_data_transfer,
	.upm_start =	motg_device_data_start,
	.upm_abort =	motg_device_data_abort,
	.upm_close =	motg_device_data_close,
	.upm_cleartoggle =	motg_device_clear_toggle,
	.upm_done =	motg_device_data_done,
};

int
motg_init(struct motg_softc *sc)
{
	uint32_t nrx, ntx, val;
	int dynfifo;
	int offset, i;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (sc->sc_mode == MOTG_MODE_DEVICE)
		return ENOTSUP; /* not supported */

	/* disable all interrupts */
	UWRITE1(sc, MUSB2_REG_INTUSBE, 0);
	UWRITE2(sc, MUSB2_REG_INTTXE, 0);
	UWRITE2(sc, MUSB2_REG_INTRXE, 0);
	/* disable pullup */

	musbotg_pull_common(sc, 0);

#ifdef MUSB2_REG_RXDBDIS
	/* disable double packet buffering XXX what's this ? */
	UWRITE2(sc, MUSB2_REG_RXDBDIS, 0xFFFF);
	UWRITE2(sc, MUSB2_REG_TXDBDIS, 0xFFFF);
#endif

	/* enable HighSpeed and ISO Update flags */

	UWRITE1(sc, MUSB2_REG_POWER,
	    MUSB2_MASK_HSENAB | MUSB2_MASK_ISOUPD);

	if (sc->sc_mode == MOTG_MODE_DEVICE) {
		/* clear Session bit, if set */
		val = UREAD1(sc, MUSB2_REG_DEVCTL);
		val &= ~MUSB2_MASK_SESS;
		UWRITE1(sc, MUSB2_REG_DEVCTL, val);
	} else {
		/* Enter session for Host mode */
		val = UREAD1(sc, MUSB2_REG_DEVCTL);
		val |= MUSB2_MASK_SESS;
		UWRITE1(sc, MUSB2_REG_DEVCTL, val);
	}
	delay(1000);
	DPRINTF("DEVCTL 0x%jx", UREAD1(sc, MUSB2_REG_DEVCTL), 0, 0, 0);

	/* disable testmode */

	UWRITE1(sc, MUSB2_REG_TESTMODE, 0);

#ifdef MUSB2_REG_MISC
	/* set default value */

	UWRITE1(sc, MUSB2_REG_MISC, 0);
#endif

	/* select endpoint index 0 */

	UWRITE1(sc, MUSB2_REG_EPINDEX, 0);

	if (sc->sc_ep_max == 0) {
		/* read out number of endpoints */
		nrx = (UREAD1(sc, MUSB2_REG_EPINFO) / 16);

		ntx = (UREAD1(sc, MUSB2_REG_EPINFO) % 16);

		/* these numbers exclude the control endpoint */

		DPRINTFN(1,"RX/TX endpoints: %ju/%ju", nrx, ntx, 0, 0);

		sc->sc_ep_max = MAX(nrx, ntx);
	} else {
		nrx = ntx = sc->sc_ep_max;
	}
	if (sc->sc_ep_max == 0) {
		aprint_error_dev(sc->sc_dev, " no endpoints\n");
		return -1;
	}
	KASSERT(sc->sc_ep_max <= MOTG_MAX_HW_EP);
	/* read out configuration data */
	val = UREAD1(sc, MUSB2_REG_CONFDATA);

	DPRINTF("Config Data: 0x%02jx", val, 0, 0, 0);

	dynfifo = (val & MUSB2_MASK_CD_DYNFIFOSZ) ? 1 : 0;

	if (dynfifo) {
		aprint_normal_dev(sc->sc_dev, "Dynamic FIFO sizing detected, "
		    "assuming 16Kbytes of FIFO RAM\n");
	}

	DPRINTF("HW version: 0x%04jx\n", UREAD1(sc, MUSB2_REG_HWVERS), 0, 0, 0);

	/* initialise endpoint profiles */
	sc->sc_in_ep[0].ep_fifo_size = 64;
	sc->sc_out_ep[0].ep_fifo_size = 0; /* not used */
	sc->sc_out_ep[0].ep_number = sc->sc_in_ep[0].ep_number = 0;
	SIMPLEQ_INIT(&sc->sc_in_ep[0].ep_pipes);
	offset = 64;

	for (i = 1; i <= sc->sc_ep_max; i++) {
		int fiforx_size, fifotx_size, fifo_size;

		/* select endpoint */
		UWRITE1(sc, MUSB2_REG_EPINDEX, i);

		if (sc->sc_ep_fifosize) {
			fiforx_size = fifotx_size = sc->sc_ep_fifosize;
		} else {
			val = UREAD1(sc, MUSB2_REG_FSIZE);
			fiforx_size = (val & MUSB2_MASK_RX_FSIZE) >> 4;
			fifotx_size = (val & MUSB2_MASK_TX_FSIZE);
		}

		DPRINTF("Endpoint %ju FIFO size: IN=%ju, OUT=%ju, DYN=%jd",
		    i, fifotx_size, fiforx_size, dynfifo);

		if (dynfifo) {
			if (sc->sc_ep_fifosize) {
				fifo_size = ffs(sc->sc_ep_fifosize) - 1;
			} else {
				if (i < 3) {
					fifo_size = 12;       /* 4K */
				} else if (i < 10) {
					fifo_size = 10;       /* 1K */
				} else {
					fifo_size = 7;        /* 128 bytes */
				}
			}
			if (fiforx_size && (i <= nrx)) {
				fiforx_size = fifo_size;
				if (fifo_size > 7) {
#if 0
					UWRITE1(sc, MUSB2_REG_RXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size) |
					    MUSB2_MASK_FIFODB);
#else
					UWRITE1(sc, MUSB2_REG_RXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size));
#endif
				} else {
					UWRITE1(sc, MUSB2_REG_RXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size));
				}
				UWRITE2(sc, MUSB2_REG_RXFIFOADD,
				    offset >> 3);
				offset += (1 << fiforx_size);
			}
			if (fifotx_size && (i <= ntx)) {
				fifotx_size = fifo_size;
				if (fifo_size > 7) {
#if 0
					UWRITE1(sc, MUSB2_REG_TXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size) |
					    MUSB2_MASK_FIFODB);
#else
					UWRITE1(sc, MUSB2_REG_TXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size));
#endif
				} else {
					UWRITE1(sc, MUSB2_REG_TXFIFOSZ,
					    MUSB2_VAL_FIFOSZ(fifo_size));
				}

				UWRITE2(sc, MUSB2_REG_TXFIFOADD,
				    offset >> 3);

				offset += (1 << fifotx_size);
			}
		}
		if (fiforx_size && (i <= nrx)) {
			sc->sc_in_ep[i].ep_fifo_size = (1 << fiforx_size);
			SIMPLEQ_INIT(&sc->sc_in_ep[i].ep_pipes);
		}
		if (fifotx_size && (i <= ntx)) {
			sc->sc_out_ep[i].ep_fifo_size = (1 << fifotx_size);
			SIMPLEQ_INIT(&sc->sc_out_ep[i].ep_pipes);
		}
		sc->sc_out_ep[i].ep_number = sc->sc_in_ep[i].ep_number = i;
	}


	DPRINTF("Dynamic FIFO size = %jd bytes", offset, 0, 0, 0);

	/* turn on default interrupts */

	if (sc->sc_mode == MOTG_MODE_HOST) {
		UWRITE1(sc, MUSB2_REG_INTUSBE, 0xff);
		UWRITE2(sc, MUSB2_REG_INTTXE, 0xffff);
		UWRITE2(sc, MUSB2_REG_INTRXE, 0xffff);
	} else
		UWRITE1(sc, MUSB2_REG_INTUSBE, MUSB2_MASK_IRESET);

	sc->sc_xferpool = pool_cache_init(sizeof(struct motg_xfer), 0, 0, 0,
	    "motgxfer", NULL, IPL_USB, NULL, NULL, NULL);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);

	/* Set up the bus struct. */
	sc->sc_bus.ub_methods = &motg_bus_methods;
	sc->sc_bus.ub_pipesize= sizeof(struct motg_pipe);
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_usedma = false;
	sc->sc_bus.ub_hcpriv = sc;
	snprintf(sc->sc_vendor, sizeof(sc->sc_vendor),
	    "Mentor Graphics");
	sc->sc_child = config_found(sc->sc_dev, &sc->sc_bus, usbctlprint);
	return 0;
}

static int
motg_select_ep(struct motg_softc *sc, struct usbd_pipe *pipe)
{
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	struct motg_hw_ep *ep;
	int i, size;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	ep = (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN) ?
	    sc->sc_in_ep : sc->sc_out_ep;
	size = UE_GET_SIZE(UGETW(pipe->up_endpoint->ue_edesc->wMaxPacketSize));

	for (i = sc->sc_ep_max; i >= 1; i--) {
		DPRINTF(UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN ?
		    "in_ep[%jd].ep_fifo_size %jd size %jd ref %jd" :
		    "out_ep[%jd].ep_fifo_size %jd size %jd ref %jd", i,
		    ep[i].ep_fifo_size, size, ep[i].refcount);
		if (ep[i].ep_fifo_size >= size) {
			/* found a suitable endpoint */
			otgpipe->hw_ep = &ep[i];
			mutex_enter(&sc->sc_lock);
			if (otgpipe->hw_ep->refcount > 0) {
				/* no luck, try next */
				mutex_exit(&sc->sc_lock);
				otgpipe->hw_ep = NULL;
			} else {
				otgpipe->hw_ep->refcount++;
				SIMPLEQ_INSERT_TAIL(&otgpipe->hw_ep->ep_pipes,
				    otgpipe, ep_pipe_list);
				mutex_exit(&sc->sc_lock);
				return 0;
			}
		}
	}
	return -1;
}

/* Open a new pipe. */
usbd_status
motg_open(struct usbd_pipe *pipe)
{
	struct motg_softc *sc = MOTG_PIPE2SC(pipe);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	uint8_t rhaddr = pipe->up_dev->ud_bus->ub_rhaddr;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	DPRINTF("pipe=%#jx, addr=%jd, endpt=%jd (%jd)", (uintptr_t)pipe,
	    pipe->up_dev->ud_addr, ed->bEndpointAddress, rhaddr);

	if (sc->sc_dying)
		return USBD_IOERROR;

	/* toggle state needed for bulk endpoints */
	otgpipe->nexttoggle = pipe->up_endpoint->ue_toggle;

	if (pipe->up_dev->ud_addr == rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &motg_root_intr_methods;
			break;
		default:
			return USBD_INVAL;
		}
	} else {
		switch (ed->bmAttributes & UE_XFERTYPE) {
		case UE_CONTROL:
			pipe->up_methods = &motg_device_ctrl_methods;
			/* always use sc_in_ep[0] for in and out */
			otgpipe->hw_ep = &sc->sc_in_ep[0];
			mutex_enter(&sc->sc_lock);
			otgpipe->hw_ep->refcount++;
			SIMPLEQ_INSERT_TAIL(&otgpipe->hw_ep->ep_pipes,
			    otgpipe, ep_pipe_list);
			mutex_exit(&sc->sc_lock);
			break;
		case UE_BULK:
		case UE_INTERRUPT:
			DPRINTFN(MD_BULK,
			    "type %jd dir %jd pipe wMaxPacketSize %jd",
			    UE_GET_XFERTYPE(ed->bmAttributes),
			    UE_GET_DIR(pipe->up_endpoint->ue_edesc->bEndpointAddress),
			    UGETW(pipe->up_endpoint->ue_edesc->wMaxPacketSize), 0);
			if (motg_select_ep(sc, pipe) != 0)
				goto bad;
			KASSERT(otgpipe->hw_ep != NULL);
			pipe->up_methods = &motg_device_data_methods;
			otgpipe->nexttoggle = pipe->up_endpoint->ue_toggle;
			break;
		default:
			goto bad;
#ifdef notyet
		case UE_ISOCHRONOUS:
			...
			break;
#endif /* notyet */
		}
	}
	return USBD_NORMAL_COMPLETION;

 bad:
	return USBD_NOMEM;
}

void
motg_softintr(void *v)
{
	struct usbd_bus *bus = v;
	struct motg_softc *sc = MOTG_BUS2SC(bus);
	uint16_t rx_status, tx_status;
	uint8_t ctrl_status;
	uint32_t val;
	int i;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	DPRINTFN(MD_ROOT | MD_CTRL, "sc %#jx", (uintptr_t)sc, 0 ,0 ,0);

	mutex_spin_enter(&sc->sc_intr_lock);
	rx_status = sc->sc_intr_rx_ep;
	sc->sc_intr_rx_ep = 0;
	tx_status = sc->sc_intr_tx_ep;
	sc->sc_intr_tx_ep = 0;
	ctrl_status = sc->sc_intr_ctrl;
	sc->sc_intr_ctrl = 0;
	mutex_spin_exit(&sc->sc_intr_lock);

	ctrl_status |= UREAD1(sc, MUSB2_REG_INTUSB);

	if (ctrl_status & (MUSB2_MASK_IRESET |
	    MUSB2_MASK_IRESUME | MUSB2_MASK_ISUSP |
	    MUSB2_MASK_ICONN | MUSB2_MASK_IDISC)) {
		DPRINTFN(MD_ROOT | MD_CTRL, "bus 0x%jx", ctrl_status, 0, 0, 0);

		if (ctrl_status & MUSB2_MASK_IRESET) {
			sc->sc_isreset = 1;
			sc->sc_port_suspended = 0;
			sc->sc_port_suspended_change = 1;
			sc->sc_connected_changed = 1;
			sc->sc_port_enabled = 1;

			val = UREAD1(sc, MUSB2_REG_POWER);
			if (val & MUSB2_MASK_HSMODE)
				sc->sc_high_speed = 1;
			else
				sc->sc_high_speed = 0;
			DPRINTFN(MD_ROOT | MD_CTRL, "speed %jd", sc->sc_high_speed,
			    0, 0, 0);

			/* turn off interrupts */
			val = MUSB2_MASK_IRESET;
			val &= ~MUSB2_MASK_IRESUME;
			val |= MUSB2_MASK_ISUSP;
			UWRITE1(sc, MUSB2_REG_INTUSBE, val);
			UWRITE2(sc, MUSB2_REG_INTTXE, 0);
			UWRITE2(sc, MUSB2_REG_INTRXE, 0);
		}
		if (ctrl_status & MUSB2_MASK_IRESUME) {
			if (sc->sc_port_suspended) {
				sc->sc_port_suspended = 0;
				sc->sc_port_suspended_change = 1;
				val = UREAD1(sc, MUSB2_REG_INTUSBE);
				/* disable resume interrupt */
				val &= ~MUSB2_MASK_IRESUME;
				/* enable suspend interrupt */
				val |= MUSB2_MASK_ISUSP;
				UWRITE1(sc, MUSB2_REG_INTUSBE, val);
			}
		} else if (ctrl_status & MUSB2_MASK_ISUSP) {
			if (!sc->sc_port_suspended) {
				sc->sc_port_suspended = 1;
				sc->sc_port_suspended_change = 1;

				val = UREAD1(sc, MUSB2_REG_INTUSBE);
				/* disable suspend interrupt */
				val &= ~MUSB2_MASK_ISUSP;
				/* enable resume interrupt */
				val |= MUSB2_MASK_IRESUME;
				UWRITE1(sc, MUSB2_REG_INTUSBE, val);
			}
		}
		if (ctrl_status & MUSB2_MASK_ICONN) {
			sc->sc_connected = 1;
			sc->sc_connected_changed = 1;
			sc->sc_isreset = 1;
			sc->sc_port_enabled = 1;
		} else if (ctrl_status & MUSB2_MASK_IDISC) {
			sc->sc_connected = 0;
			sc->sc_connected_changed = 1;
			sc->sc_isreset = 0;
			sc->sc_port_enabled = 0;
		}

		/* complete root HUB interrupt endpoint */

		motg_hub_change(sc);
	}
	/*
	 * read in interrupt status and mix with the status we
	 * got from the wrapper
	 */
	rx_status |= UREAD2(sc, MUSB2_REG_INTRX);
	tx_status |= UREAD2(sc, MUSB2_REG_INTTX);

	KASSERTMSG((rx_status & 0x01) == 0, "ctrl_rx %08x", rx_status);
	if (tx_status & 0x01)
		motg_device_ctrl_intr_tx(sc);
	for (i = 1; i <= sc->sc_ep_max; i++) {
		if (rx_status & (0x01 << i))
			motg_device_intr_rx(sc, i);
		if (tx_status & (0x01 << i))
			motg_device_intr_tx(sc, i);
	}
	return;
}

void
motg_poll(struct usbd_bus *bus)
{
	struct motg_softc *sc = MOTG_BUS2SC(bus);

	sc->sc_intr_poll(sc->sc_intr_poll_arg);
	mutex_enter(&sc->sc_lock);
	motg_softintr(bus);
	mutex_exit(&sc->sc_lock);
}

int
motg_intr(struct motg_softc *sc, uint16_t rx_ep, uint16_t tx_ep,
    uint8_t ctrl)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	sc->sc_intr_tx_ep = tx_ep;
	sc->sc_intr_rx_ep = rx_ep;
	sc->sc_intr_ctrl = ctrl;

	if (!sc->sc_bus.ub_usepolling) {
		usb_schedsoftintr(&sc->sc_bus);
	}
	return 1;
}

int
motg_intr_vbus(struct motg_softc *sc, int vbus)
{
	uint8_t val;
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (sc->sc_mode == MOTG_MODE_HOST && vbus == 0) {
		DPRINTF("vbus down, try to re-enable", 0, 0, 0, 0);
		/* try to re-enter session for Host mode */
		val = UREAD1(sc, MUSB2_REG_DEVCTL);
		val |= MUSB2_MASK_SESS;
		UWRITE1(sc, MUSB2_REG_DEVCTL, val);
	}
	return 1;
}

struct usbd_xfer *
motg_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct motg_softc *sc = MOTG_BUS2SC(bus);
	struct usbd_xfer *xfer;

	xfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct motg_xfer));
#ifdef DIAGNOSTIC
		xfer->ux_state = XFER_BUSY;
#endif
	}
	return xfer;
}

void
motg_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_BUS2SC(bus);

#ifdef DIAGNOSTIC
	if (xfer->ux_state != XFER_BUSY) {
		printf("motg_freex: xfer=%p not busy, 0x%08x\n", xfer,
		       xfer->ux_state);
	}
	xfer->ux_state = XFER_FREE;
#endif
	pool_cache_put(sc->sc_xferpool, xfer);
}

static void
motg_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct motg_softc *sc = MOTG_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

/*
 * Routines to emulate the root hub.
 */
Static int
motg_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct motg_softc *sc = MOTG_BUS2SC(bus);
	int status, change, totlen = 0;
	uint16_t len, value, index;
	usb_port_status_t ps;
	usbd_status err;
	uint32_t val;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (sc->sc_dying)
		return -1;

	DPRINTFN(MD_ROOT, "type=0x%02jx request=%02jx", req->bmRequestType,
	    req->bRequest, 0, 0);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(MD_ROOT, "wValue=0x%04jx", value, 0, 0, 0);
		switch (value) {
		case C(0, UDESC_DEVICE): {
			usb_device_descriptor_t devd;

			totlen = min(buflen, sizeof(devd));
			memcpy(&devd, buf, totlen);
			USETW(devd.idVendor, sc->sc_id_vendor);
			memcpy(buf, &devd, totlen);
			break;
		}
		case C(1, UDESC_STRING):
#define sd ((usb_string_descriptor_t *)buf)
			/* Vendor */
			totlen = usb_makestrdesc(sd, len, sc->sc_vendor);
			break;
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "MOTG root hub");
			break;
#undef sd
		default:
			/* default from usbroothub */
			return buflen;
		}
		break;
	/* Hub requests */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		DPRINTFN(MD_ROOT,
		    "UR_CLEAR_PORT_FEATURE port=%jd feature=%jd", index, value,
		    0, 0);
		if (index != 1) {
			return -1;
		}
		switch (value) {
		case UHF_PORT_ENABLE:
			sc->sc_port_enabled = 0;
			break;
		case UHF_PORT_SUSPEND:
			if (sc->sc_port_suspended != 0) {
				val = UREAD1(sc, MUSB2_REG_POWER);
				val &= ~MUSB2_MASK_SUSPMODE;
				val |= MUSB2_MASK_RESUME;
				UWRITE1(sc, MUSB2_REG_POWER, val);
				/* wait 20 milliseconds */
				usb_delay_ms(&sc->sc_bus, 20);
				val = UREAD1(sc, MUSB2_REG_POWER);
				val &= ~MUSB2_MASK_RESUME;
				UWRITE1(sc, MUSB2_REG_POWER, val);
				sc->sc_port_suspended = 0;
				sc->sc_port_suspended_change = 1;
			}
			break;
		case UHF_PORT_RESET:
			break;
		case UHF_C_PORT_CONNECTION:
			break;
		case UHF_C_PORT_ENABLE:
			break;
		case UHF_C_PORT_OVER_CURRENT:
			break;
		case UHF_C_PORT_RESET:
			sc->sc_isreset = 0;
			break;
		case UHF_PORT_POWER:
			/* XXX todo */
			break;
		case UHF_PORT_CONNECTION:
		case UHF_PORT_OVER_CURRENT:
		case UHF_PORT_LOW_SPEED:
		case UHF_C_PORT_SUSPEND:
		default:
			return -1;
		}
		break;
	case C(UR_GET_BUS_STATE, UT_READ_CLASS_OTHER):
		return -1;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0) {
			return -1;
		}
		totlen = buflen;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			return -1;
		}
		memset(buf, 0, len);
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		if (index != 1) {
			return -1;
		}
		if (len != 4) {
			return -1;
		}
		status = change = 0;
		if (sc->sc_connected)
			status |= UPS_CURRENT_CONNECT_STATUS;
		if (sc->sc_connected_changed) {
			change |= UPS_C_CONNECT_STATUS;
			sc->sc_connected_changed = 0;
		}
		if (sc->sc_port_enabled)
			status |= UPS_PORT_ENABLED;
		if (sc->sc_port_enabled_changed) {
			change |= UPS_C_PORT_ENABLED;
			sc->sc_port_enabled_changed = 0;
		}
		if (sc->sc_port_suspended)
			status |= UPS_SUSPEND;
		if (sc->sc_high_speed)
			status |= UPS_HIGH_SPEED;
		status |= UPS_PORT_POWER; /* XXX */
		if (sc->sc_isreset)
			change |= UPS_C_PORT_RESET;
		USETW(ps.wPortStatus, status);
		USETW(ps.wPortChange, change);
		totlen = min(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		return -1;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index != 1) {
			return -1;
		}
		switch(value) {
		case UHF_PORT_ENABLE:
			sc->sc_port_enabled = 1;
			break;
		case UHF_PORT_SUSPEND:
			if (sc->sc_port_suspended == 0) {
				val = UREAD1(sc, MUSB2_REG_POWER);
				val |= MUSB2_MASK_SUSPMODE;
				UWRITE1(sc, MUSB2_REG_POWER, val);
				/* wait 20 milliseconds */
				usb_delay_ms(&sc->sc_bus, 20);
				sc->sc_port_suspended = 1;
				sc->sc_port_suspended_change = 1;
			}
			break;
		case UHF_PORT_RESET:
			err = motg_portreset(sc);
			if (err != USBD_NORMAL_COMPLETION)
				return -1;
			return 0;
		case UHF_PORT_POWER:
			/* XXX todo */
			return 0;
		case UHF_C_PORT_CONNECTION:
		case UHF_C_PORT_ENABLE:
		case UHF_C_PORT_OVER_CURRENT:
		case UHF_PORT_CONNECTION:
		case UHF_PORT_OVER_CURRENT:
		case UHF_PORT_LOW_SPEED:
		case UHF_C_PORT_SUSPEND:
		case UHF_C_PORT_RESET:
		default:
			return -1;
		}
		break;
	default:
		/* default from usbroothub */
		return buflen;
	}

	return totlen;
}

/* Abort a root interrupt request. */
void
motg_root_intr_abort(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	sc->sc_intr_xfer = NULL;

	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

usbd_status
motg_root_intr_transfer(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * start first
	 */
	return motg_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

/* Start a transfer on the root interrupt pipe */
usbd_status
motg_root_intr_start(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct motg_softc *sc = MOTG_PIPE2SC(pipe);

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	DPRINTFN(MD_ROOT, "xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, 0);

	if (sc->sc_dying)
		return USBD_IOERROR;

	sc->sc_intr_xfer = xfer;
	return USBD_IN_PROGRESS;
}

/* Close the root interrupt pipe. */
void
motg_root_intr_close(struct usbd_pipe *pipe)
{
	struct motg_softc *sc = MOTG_PIPE2SC(pipe);
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	sc->sc_intr_xfer = NULL;
}

void
motg_root_intr_done(struct usbd_xfer *xfer)
{
}

void
motg_noop(struct usbd_pipe *pipe)
{
}

static usbd_status
motg_portreset(struct motg_softc *sc)
{
	uint32_t val;
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	val = UREAD1(sc, MUSB2_REG_POWER);
	val |= MUSB2_MASK_RESET;
	UWRITE1(sc, MUSB2_REG_POWER, val);
	/* Wait for 20 msec */
	usb_delay_ms(&sc->sc_bus, 20);

	val = UREAD1(sc, MUSB2_REG_POWER);
	val &= ~MUSB2_MASK_RESET;
	UWRITE1(sc, MUSB2_REG_POWER, val);

	/* determine line speed */
	val = UREAD1(sc, MUSB2_REG_POWER);
	if (val & MUSB2_MASK_HSMODE)
		sc->sc_high_speed = 1;
	else
		sc->sc_high_speed = 0;
	DPRINTFN(MD_ROOT | MD_CTRL, "speed %jd", sc->sc_high_speed, 0, 0, 0);

	sc->sc_isreset = 1;
	sc->sc_port_enabled = 1;
	return USBD_NORMAL_COMPLETION;
}

/*
 * This routine is executed when an interrupt on the root hub is detected
 */
static void
motg_hub_change(struct motg_softc *sc)
{
	struct usbd_xfer *xfer = sc->sc_intr_xfer;
	struct usbd_pipe *pipe;
	u_char *p;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (xfer == NULL)
		return; /* the interrupt pipe is not open */

	pipe = xfer->ux_pipe;
	if (pipe->up_dev == NULL || pipe->up_dev->ud_bus == NULL)
		return;	/* device has detached */

	p = xfer->ux_buf;
	p[0] = 1<<1;
	xfer->ux_actlen = 1;
	xfer->ux_status = USBD_NORMAL_COMPLETION;
	usb_transfer_complete(xfer);
}

static uint8_t
motg_speed(uint8_t speed)
{
	switch(speed) {
	case USB_SPEED_LOW:
		return MUSB2_MASK_TI_SPEED_LO;
	case USB_SPEED_FULL:
		return MUSB2_MASK_TI_SPEED_FS;
	case USB_SPEED_HIGH:
		return MUSB2_MASK_TI_SPEED_HS;
	default:
		panic("motg: unknown speed %d", speed);
		/* NOTREACHED */
	}
}

static uint8_t
motg_type(uint8_t type)
{
	switch(type) {
	case UE_CONTROL:
		return MUSB2_MASK_TI_PROTO_CTRL;
	case UE_ISOCHRONOUS:
		return MUSB2_MASK_TI_PROTO_ISOC;
	case UE_BULK:
		return MUSB2_MASK_TI_PROTO_BULK;
	case UE_INTERRUPT:
		return MUSB2_MASK_TI_PROTO_INTR;
	default:
		panic("motg: unknown type %d", type);
		/* NOTREACHED */
	}
}

static void
motg_setup_endpoint_tx(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	struct usbd_device *dev = otgpipe->pipe.up_dev;
	int epnumber = otgpipe->hw_ep->ep_number;

	UWRITE1(sc, MUSB2_REG_TXFADDR(epnumber), dev->ud_addr);
	if (dev->ud_myhsport) {
		UWRITE1(sc, MUSB2_REG_TXHADDR(epnumber),
		    dev->ud_myhsport->up_parent->ud_addr);
		UWRITE1(sc, MUSB2_REG_TXHUBPORT(epnumber),
		    dev->ud_myhsport->up_portno);
	} else {
		UWRITE1(sc, MUSB2_REG_TXHADDR(epnumber), 0);
		UWRITE1(sc, MUSB2_REG_TXHUBPORT(epnumber), 0);
	}
	UWRITE1(sc, MUSB2_REG_TXTI,
	    motg_speed(dev->ud_speed) |
	    UE_GET_ADDR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress) |
	    motg_type(UE_GET_XFERTYPE(xfer->ux_pipe->up_endpoint->ue_edesc->bmAttributes))
	    );
	if (epnumber == 0) {
		if (sc->sc_high_speed) {
			UWRITE1(sc, MUSB2_REG_TXNAKLIMIT,
			    NAK_TO_CTRL_HIGH);
		} else {
			UWRITE1(sc, MUSB2_REG_TXNAKLIMIT, NAK_TO_CTRL);
		}
	} else {
		if ((xfer->ux_pipe->up_endpoint->ue_edesc->bmAttributes & UE_XFERTYPE)
		    == UE_BULK) {
			if (sc->sc_high_speed) {
				UWRITE1(sc, MUSB2_REG_TXNAKLIMIT,
				    NAK_TO_BULK_HIGH);
			} else {
				UWRITE1(sc, MUSB2_REG_TXNAKLIMIT, NAK_TO_BULK);
			}
		} else {
			if (sc->sc_high_speed) {
				UWRITE1(sc, MUSB2_REG_TXNAKLIMIT, POLL_TO_HIGH);
			} else {
				UWRITE1(sc, MUSB2_REG_TXNAKLIMIT, POLL_TO);
			}
		}
	}
}

static void
motg_setup_endpoint_rx(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct usbd_device *dev = xfer->ux_pipe->up_dev;
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	int epnumber = otgpipe->hw_ep->ep_number;

	UWRITE1(sc, MUSB2_REG_RXFADDR(epnumber), dev->ud_addr);
	if (dev->ud_myhsport) {
		UWRITE1(sc, MUSB2_REG_RXHADDR(epnumber),
		    dev->ud_myhsport->up_parent->ud_addr);
		UWRITE1(sc, MUSB2_REG_RXHUBPORT(epnumber),
		    dev->ud_myhsport->up_portno);
	} else {
		UWRITE1(sc, MUSB2_REG_RXHADDR(epnumber), 0);
		UWRITE1(sc, MUSB2_REG_RXHUBPORT(epnumber), 0);
	}
	UWRITE1(sc, MUSB2_REG_RXTI,
	    motg_speed(dev->ud_speed) |
	    UE_GET_ADDR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress) |
	    motg_type(UE_GET_XFERTYPE(xfer->ux_pipe->up_endpoint->ue_edesc->bmAttributes))
	    );
	if (epnumber == 0) {
		if (sc->sc_high_speed) {
			UWRITE1(sc, MUSB2_REG_RXNAKLIMIT,
			    NAK_TO_CTRL_HIGH);
		} else {
			UWRITE1(sc, MUSB2_REG_RXNAKLIMIT, NAK_TO_CTRL);
		}
	} else {
		if ((xfer->ux_pipe->up_endpoint->ue_edesc->bmAttributes & UE_XFERTYPE)
		    == UE_BULK) {
			if (sc->sc_high_speed) {
				UWRITE1(sc, MUSB2_REG_RXNAKLIMIT,
				    NAK_TO_BULK_HIGH);
			} else {
				UWRITE1(sc, MUSB2_REG_RXNAKLIMIT, NAK_TO_BULK);
			}
		} else {
			if (sc->sc_high_speed) {
				UWRITE1(sc, MUSB2_REG_RXNAKLIMIT, POLL_TO_HIGH);
			} else {
				UWRITE1(sc, MUSB2_REG_RXNAKLIMIT, POLL_TO);
			}
		}
	}
}

static usbd_status
motg_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	xfer->ux_status = USBD_NOT_STARTED;
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return motg_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
motg_device_ctrl_start(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	usbd_status err;
	mutex_enter(&sc->sc_lock);
	err = motg_device_ctrl_start1(sc);
	mutex_exit(&sc->sc_lock);
	if (err != USBD_IN_PROGRESS)
		return err;
	return USBD_IN_PROGRESS;
}

static usbd_status
motg_device_ctrl_start1(struct motg_softc *sc)
{
	struct motg_hw_ep *ep = &sc->sc_in_ep[0];
	struct usbd_xfer *xfer = NULL;
	struct motg_pipe *otgpipe;
	usbd_status err = 0;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	if (sc->sc_dying)
		return USBD_IOERROR;

	if (!sc->sc_connected)
		return USBD_IOERROR;

	if (ep->xfer != NULL) {
		err = USBD_IN_PROGRESS;
		goto end;
	}
	/* locate the first pipe with work to do */
	SIMPLEQ_FOREACH(otgpipe, &ep->ep_pipes, ep_pipe_list) {
		xfer = SIMPLEQ_FIRST(&otgpipe->pipe.up_queue);
		DPRINTFN(MD_CTRL, "pipe %#jx xfer %#jx status %jd",
		    (uintptr_t)otgpipe, (uintptr_t)xfer,
		    (xfer != NULL) ? xfer->ux_status : 0, 0);

		if (xfer != NULL) {
			/* move this pipe to the end of the list */
			SIMPLEQ_REMOVE(&ep->ep_pipes, otgpipe,
			    motg_pipe, ep_pipe_list);
			SIMPLEQ_INSERT_TAIL(&ep->ep_pipes,
			    otgpipe, ep_pipe_list);
			break;
		}
	}
	if (xfer == NULL) {
		err = USBD_NOT_STARTED;
		goto end;
	}
	xfer->ux_status = USBD_IN_PROGRESS;
	KASSERT(otgpipe == MOTG_PIPE2MPIPE(xfer->ux_pipe));
	KASSERT(otgpipe->hw_ep == ep);
	KASSERT(xfer->ux_rqflags & URQ_REQUEST);
	// KASSERT(xfer->ux_actlen == 0);
	xfer->ux_actlen = 0;

	ep->xfer = xfer;
	ep->datalen = xfer->ux_length;
	if (ep->datalen > 0)
		ep->data = xfer->ux_buf;
	else
		ep->data = NULL;
	if ((xfer->ux_flags & USBD_FORCE_SHORT_XFER) &&
	    (ep->datalen % 64) == 0)
		ep->need_short_xfer = 1;
	else
		ep->need_short_xfer = 0;
	/* now we need send this request */
	DPRINTFN(MD_CTRL,
	    "xfer %#jx send data %#jx len %jd short %jd",
	    (uintptr_t)xfer, (uintptr_t)ep->data, ep->datalen,
	    ep->need_short_xfer);
	DPRINTFN(MD_CTRL,
	    "xfer %#jx ... speed %jd to %jd", (uintptr_t)xfer,
	    xfer->ux_pipe->up_dev->ud_speed,
	    xfer->ux_pipe->up_dev->ud_addr, 0);
	KASSERT(ep->phase == IDLE);
	ep->phase = SETUP;
	/* select endpoint 0 */
	UWRITE1(sc, MUSB2_REG_EPINDEX, 0);
	/* fifo should be empty at this point */
	KASSERT((UREAD1(sc, MUSB2_REG_TXCSRL) & MUSB2_MASK_CSR0L_TXPKTRDY) == 0);
	/* send data */
	// KASSERT(((vaddr_t)(&xfer->ux_request) & 3) == 0);
	KASSERT(sizeof(xfer->ux_request) == 8);
	bus_space_write_multi_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_EPFIFO(0),
	    (void *)&xfer->ux_request, sizeof(xfer->ux_request));

	motg_setup_endpoint_tx(xfer);
	/* start transaction */
	UWRITE1(sc, MUSB2_REG_TXCSRL,
	    MUSB2_MASK_CSR0L_TXPKTRDY | MUSB2_MASK_CSR0L_SETUPPKT);

end:
	if (err)
		return err;

	return USBD_IN_PROGRESS;
}

static void
motg_device_ctrl_read(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	/* assume endpoint already selected */
	motg_setup_endpoint_rx(xfer);
	/* start transaction */
	UWRITE1(sc, MUSB2_REG_TXCSRL, MUSB2_MASK_CSR0L_REQPKT);
	otgpipe->hw_ep->phase = DATA_IN;
}

static void
motg_device_ctrl_intr_rx(struct motg_softc *sc)
{
	struct motg_hw_ep *ep = &sc->sc_in_ep[0];
	struct usbd_xfer *xfer = ep->xfer;
	uint8_t csr;
	int datalen, max_datalen;
	char *data;
	bool got_short;
	usbd_status new_status = USBD_IN_PROGRESS;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	KASSERT(ep->phase == DATA_IN || ep->phase == STATUS_IN);
	/* select endpoint 0 */
	UWRITE1(sc, MUSB2_REG_EPINDEX, 0);

	/* read out FIFO status */
	csr = UREAD1(sc, MUSB2_REG_TXCSRL);
	DPRINTFN(MD_CTRL, "phase %jd csr 0x%jx xfer %#jx status %jd",
	    ep->phase, csr, (uintptr_t)xfer,
	    (xfer != NULL) ? xfer->ux_status : 0);

	if (csr & MUSB2_MASK_CSR0L_NAKTIMO) {
		csr &= ~MUSB2_MASK_CSR0L_REQPKT;
		UWRITE1(sc, MUSB2_REG_TXCSRL, csr);

		csr &= ~MUSB2_MASK_CSR0L_NAKTIMO;
		UWRITE1(sc, MUSB2_REG_TXCSRL, csr);
		new_status = USBD_TIMEOUT; /* XXX */
		goto complete;
	}
	if (csr & (MUSB2_MASK_CSR0L_RXSTALL | MUSB2_MASK_CSR0L_ERROR)) {
		if (csr & MUSB2_MASK_CSR0L_RXSTALL)
			new_status = USBD_STALLED;
		else
			new_status = USBD_IOERROR;
		/* clear status */
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	if ((csr & MUSB2_MASK_CSR0L_RXPKTRDY) == 0)
		return; /* no data yet */

	if (xfer == NULL || xfer->ux_status != USBD_IN_PROGRESS)
		goto complete;

	if (ep->phase == STATUS_IN) {
		new_status = USBD_NORMAL_COMPLETION;
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	datalen = UREAD2(sc, MUSB2_REG_RXCOUNT);
	DPRINTFN(MD_CTRL, "phase %jd datalen %jd", ep->phase, datalen, 0, 0);
	KASSERT(UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize) > 0);
	max_datalen = min(UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize),
	    ep->datalen);
	if (datalen > max_datalen) {
		new_status = USBD_IOERROR;
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	got_short = (datalen < max_datalen);
	if (datalen > 0) {
		KASSERT(ep->phase == DATA_IN);
		data = ep->data;
		ep->data += datalen;
		ep->datalen -= datalen;
		xfer->ux_actlen += datalen;
		if (((vaddr_t)data & 0x3) == 0 &&
		    (datalen >> 2) > 0) {
			DPRINTFN(MD_CTRL, "r4 data %#jx len %jd",
			    (uintptr_t)data, datalen, 0, 0);
			bus_space_read_multi_4(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(0), (void *)data, datalen >> 2);
			data += (datalen & ~0x3);
			datalen -= (datalen & ~0x3);
		}
		DPRINTFN(MD_CTRL, "r1 data %#jx len %jd", (uintptr_t)data,
		    datalen, 0, 0);
		if (datalen) {
			bus_space_read_multi_1(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(0), data, datalen);
		}
	}
	UWRITE1(sc, MUSB2_REG_TXCSRL, csr & ~MUSB2_MASK_CSR0L_RXPKTRDY);
	KASSERT(ep->phase == DATA_IN);
	if (got_short || (ep->datalen == 0)) {
		if (ep->need_short_xfer == 0) {
			ep->phase = STATUS_OUT;
			UWRITE1(sc, MUSB2_REG_TXCSRH,
			    UREAD1(sc, MUSB2_REG_TXCSRH) |
			    MUSB2_MASK_CSR0H_PING_DIS);
			motg_setup_endpoint_tx(xfer);
			UWRITE1(sc, MUSB2_REG_TXCSRL,
			    MUSB2_MASK_CSR0L_STATUSPKT |
			    MUSB2_MASK_CSR0L_TXPKTRDY);
			return;
		}
		ep->need_short_xfer = 0;
	}
	motg_device_ctrl_read(xfer);
	return;
complete:
	ep->phase = IDLE;
	ep->xfer = NULL;
	if (xfer && xfer->ux_status == USBD_IN_PROGRESS) {
		KASSERT(new_status != USBD_IN_PROGRESS);
		xfer->ux_status = new_status;
		usb_transfer_complete(xfer);
	}
	motg_device_ctrl_start1(sc);
}

static void
motg_device_ctrl_intr_tx(struct motg_softc *sc)
{
	struct motg_hw_ep *ep = &sc->sc_in_ep[0];
	struct usbd_xfer *xfer = ep->xfer;
	uint8_t csr;
	int datalen;
	char *data;
	usbd_status new_status = USBD_IN_PROGRESS;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	if (ep->phase == DATA_IN || ep->phase == STATUS_IN) {
		motg_device_ctrl_intr_rx(sc);
		return;
	}

	KASSERT(ep->phase == SETUP || ep->phase == DATA_OUT ||
	    ep->phase == STATUS_OUT);

	/* select endpoint 0 */
	UWRITE1(sc, MUSB2_REG_EPINDEX, 0);

	csr = UREAD1(sc, MUSB2_REG_TXCSRL);
	DPRINTFN(MD_CTRL, "phase %jd csr 0x%jx xfer %#jx status %jd",
	    ep->phase, csr, (uintptr_t)xfer,
	    (xfer != NULL) ? xfer->ux_status : 0);

	if (csr & MUSB2_MASK_CSR0L_RXSTALL) {
		/* command not accepted */
		new_status = USBD_STALLED;
		/* clear status */
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	if (csr & MUSB2_MASK_CSR0L_NAKTIMO) {
		new_status = USBD_TIMEOUT; /* XXX */
		/* flush fifo */
		while (csr & MUSB2_MASK_CSR0L_TXFIFONEMPTY) {
			UWRITE1(sc, MUSB2_REG_TXCSRH,
			    UREAD1(sc, MUSB2_REG_TXCSRH) |
				MUSB2_MASK_CSR0H_FFLUSH);
			csr = UREAD1(sc, MUSB2_REG_TXCSRL);
		}
		csr &= ~MUSB2_MASK_CSR0L_NAKTIMO;
		UWRITE1(sc, MUSB2_REG_TXCSRL, csr);
		goto complete;
	}
	if (csr & MUSB2_MASK_CSR0L_ERROR) {
		new_status = USBD_IOERROR;
		/* clear status */
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	if (csr & MUSB2_MASK_CSR0L_TXFIFONEMPTY) {
		/* data still not sent */
		return;
	}
	if (xfer == NULL)
		goto complete;
	if (ep->phase == STATUS_OUT) {
		/*
		 * we have sent status and got no error;
		 * declare transfer complete
		 */
		DPRINTFN(MD_CTRL, "xfer %#jx status %jd complete",
		    (uintptr_t)xfer, xfer->ux_status, 0, 0);
		new_status = USBD_NORMAL_COMPLETION;
		goto complete;
	}
	if (ep->datalen == 0) {
		if (ep->need_short_xfer) {
			ep->need_short_xfer = 0;
			/* one more data phase */
			if (xfer->ux_request.bmRequestType & UT_READ) {
				DPRINTFN(MD_CTRL, "xfer %#jx to DATA_IN",
				    (uintptr_t)xfer, 0, 0, 0);
				motg_device_ctrl_read(xfer);
				return;
			} /*  else fall back to DATA_OUT */
		} else {
			DPRINTFN(MD_CTRL, "xfer %#jx to STATUS_IN, csrh 0x%jx",
			    (uintptr_t)xfer, UREAD1(sc, MUSB2_REG_TXCSRH),
			    0, 0);
			ep->phase = STATUS_IN;
			UWRITE1(sc, MUSB2_REG_RXCSRH,
			    UREAD1(sc, MUSB2_REG_RXCSRH) |
			    MUSB2_MASK_CSR0H_PING_DIS);
			motg_setup_endpoint_rx(xfer);
			UWRITE1(sc, MUSB2_REG_TXCSRL,
			    MUSB2_MASK_CSR0L_STATUSPKT |
			    MUSB2_MASK_CSR0L_REQPKT);
			return;
		}
	}
	if (xfer->ux_request.bmRequestType & UT_READ) {
		motg_device_ctrl_read(xfer);
		return;
	}
	/* setup a dataout phase */
	datalen = min(ep->datalen,
	    UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize));
	ep->phase = DATA_OUT;
	DPRINTFN(MD_CTRL, "xfer %#jx to DATA_OUT, csrh 0x%jx", (uintptr_t)xfer,
	    UREAD1(sc, MUSB2_REG_TXCSRH), 0, 0);
	if (datalen) {
		data = ep->data;
		ep->data += datalen;
		ep->datalen -= datalen;
		xfer->ux_actlen += datalen;
		if (((vaddr_t)data & 0x3) == 0 &&
		    (datalen >> 2) > 0) {
			bus_space_write_multi_4(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(0), (void *)data, datalen >> 2);
			data += (datalen & ~0x3);
			datalen -= (datalen & ~0x3);
		}
		if (datalen) {
			bus_space_write_multi_1(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(0), data, datalen);
		}
	}
	/* send data */
	motg_setup_endpoint_tx(xfer);
	UWRITE1(sc, MUSB2_REG_TXCSRL, MUSB2_MASK_CSR0L_TXPKTRDY);
	return;

complete:
	ep->phase = IDLE;
	ep->xfer = NULL;
	if (xfer && xfer->ux_status == USBD_IN_PROGRESS) {
		KASSERT(new_status != USBD_IN_PROGRESS);
		xfer->ux_status = new_status;
		usb_transfer_complete(xfer);
	}
	motg_device_ctrl_start1(sc);
}

/* Abort a device control request. */
void
motg_device_ctrl_abort(struct usbd_xfer *xfer)
{
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	motg_device_xfer_abort(xfer);
}

/* Close a device control pipe */
void
motg_device_ctrl_close(struct usbd_pipe *pipe)
{
	struct motg_softc *sc __diagused = MOTG_PIPE2SC(pipe);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(pipe);
	struct motg_pipe *otgpipeiter;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(otgpipe->hw_ep->xfer == NULL ||
	    otgpipe->hw_ep->xfer->ux_pipe != pipe);

	SIMPLEQ_FOREACH(otgpipeiter, &otgpipe->hw_ep->ep_pipes, ep_pipe_list) {
		if (otgpipeiter == otgpipe) {
			/* remove from list */
			SIMPLEQ_REMOVE(&otgpipe->hw_ep->ep_pipes, otgpipe,
			    motg_pipe, ep_pipe_list);
			otgpipe->hw_ep->refcount--;
			/* we're done */
			return;
		}
	}
	panic("motg_device_ctrl_close: not found");
}

void
motg_device_ctrl_done(struct usbd_xfer *xfer)
{
	struct motg_pipe *otgpipe __diagused = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(otgpipe->hw_ep->xfer != xfer);
}

static usbd_status
motg_device_data_transfer(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	usbd_status err;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	DPRINTF("xfer %#jx status %jd", (uintptr_t)xfer, xfer->ux_status, 0, 0);
	err = usb_insert_transfer(xfer);
	xfer->ux_status = USBD_NOT_STARTED;
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return motg_device_data_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
motg_device_data_start(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	usbd_status err;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	DPRINTF("xfer %#jx status %jd", (uintptr_t)xfer, xfer->ux_status, 0, 0);
	err = motg_device_data_start1(sc, otgpipe->hw_ep);
	mutex_exit(&sc->sc_lock);
	if (err != USBD_IN_PROGRESS)
		return err;
	return USBD_IN_PROGRESS;
}

static usbd_status
motg_device_data_start1(struct motg_softc *sc, struct motg_hw_ep *ep)
{
	struct usbd_xfer *xfer = NULL;
	struct motg_pipe *otgpipe;
	usbd_status err = 0;
	uint32_t val __diagused;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	if (sc->sc_dying)
		return USBD_IOERROR;

	if (!sc->sc_connected)
		return USBD_IOERROR;

	if (ep->xfer != NULL) {
		err = USBD_IN_PROGRESS;
		goto end;
	}
	/* locate the first pipe with work to do */
	SIMPLEQ_FOREACH(otgpipe, &ep->ep_pipes, ep_pipe_list) {
		xfer = SIMPLEQ_FIRST(&otgpipe->pipe.up_queue);
		DPRINTFN(MD_BULK, "pipe %#jx xfer %#jx status %jd",
		    (uintptr_t)otgpipe, (uintptr_t)xfer,
		    (xfer != NULL) ? xfer->ux_status : 0, 0);
		if (xfer != NULL) {
			/* move this pipe to the end of the list */
			SIMPLEQ_REMOVE(&ep->ep_pipes, otgpipe,
			    motg_pipe, ep_pipe_list);
			SIMPLEQ_INSERT_TAIL(&ep->ep_pipes,
			    otgpipe, ep_pipe_list);
			break;
		}
	}
	if (xfer == NULL) {
		err = USBD_NOT_STARTED;
		goto end;
	}
	xfer->ux_status = USBD_IN_PROGRESS;
	KASSERT(otgpipe == MOTG_PIPE2MPIPE(xfer->ux_pipe));
	KASSERT(otgpipe->hw_ep == ep);
	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	// KASSERT(xfer->ux_actlen == 0);
	xfer->ux_actlen = 0;

	ep->xfer = xfer;
	ep->datalen = xfer->ux_length;
	KASSERT(ep->datalen > 0);
	ep->data = xfer->ux_buf;
	if ((xfer->ux_flags & USBD_FORCE_SHORT_XFER) &&
	    (ep->datalen % 64) == 0)
		ep->need_short_xfer = 1;
	else
		ep->need_short_xfer = 0;
	/* now we need send this request */
	DPRINTFN(MD_BULK,
	    UE_GET_DIR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN ?
	    "xfer %#jx in  data %#jx len %jd short %jd" :
	    "xfer %#jx out data %#jx len %jd short %jd",
	    (uintptr_t)xfer, (uintptr_t)ep->data, ep->datalen,
	    ep->need_short_xfer);
	DPRINTFN(MD_BULK, "... speed %jd to %jd",
	    xfer->ux_pipe->up_dev->ud_speed,
	    xfer->ux_pipe->up_dev->ud_addr, 0, 0);
	KASSERT(ep->phase == IDLE);
	/* select endpoint */
	UWRITE1(sc, MUSB2_REG_EPINDEX, ep->ep_number);
	if (UE_GET_DIR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress)
	    == UE_DIR_IN) {
		val = UREAD1(sc, MUSB2_REG_RXCSRL);
		KASSERT((val & MUSB2_MASK_CSRL_RXPKTRDY) == 0);
		motg_device_data_read(xfer);
	} else {
		ep->phase = DATA_OUT;
		val = UREAD1(sc, MUSB2_REG_TXCSRL);
		KASSERT((val & MUSB2_MASK_CSRL_TXPKTRDY) == 0);
		motg_device_data_write(xfer);
	}
end:
	if (err)
		return err;

	return USBD_IN_PROGRESS;
}

static void
motg_device_data_read(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	uint32_t val;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	/* assume endpoint already selected */
	motg_setup_endpoint_rx(xfer);
	/* Max packet size */
	UWRITE2(sc, MUSB2_REG_RXMAXP,
	    UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize));
	/* Data Toggle */
	val = UREAD1(sc, MUSB2_REG_RXCSRH);
	val |= MUSB2_MASK_CSRH_RXDT_WREN;
	if (otgpipe->nexttoggle)
		val |= MUSB2_MASK_CSRH_RXDT_VAL;
	else
		val &= ~MUSB2_MASK_CSRH_RXDT_VAL;
	UWRITE1(sc, MUSB2_REG_RXCSRH, val);

	DPRINTFN(MD_BULK, "%#jx to DATA_IN on ep %jd, csrh 0x%jx",
	    (uintptr_t)xfer, otgpipe->hw_ep->ep_number,
	    UREAD1(sc, MUSB2_REG_RXCSRH), 0);
	/* start transaction */
	UWRITE1(sc, MUSB2_REG_RXCSRL, MUSB2_MASK_CSRL_RXREQPKT);
	otgpipe->hw_ep->phase = DATA_IN;
}

static void
motg_device_data_write(struct usbd_xfer *xfer)
{
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	struct motg_hw_ep *ep = otgpipe->hw_ep;
	int datalen;
	char *data;
	uint32_t val;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(xfer!=NULL);
	KASSERT(mutex_owned(&sc->sc_lock));

	datalen = min(ep->datalen,
	    UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize));
	ep->phase = DATA_OUT;
	DPRINTFN(MD_BULK, "%#jx to DATA_OUT on ep %jd, len %jd csrh 0x%jx",
	    (uintptr_t)xfer, ep->ep_number, datalen,
	    UREAD1(sc, MUSB2_REG_TXCSRH));

	/* assume endpoint already selected */
	/* write data to fifo */
	data = ep->data;
	ep->data += datalen;
	ep->datalen -= datalen;
	xfer->ux_actlen += datalen;
	if (((vaddr_t)data & 0x3) == 0 &&
	    (datalen >> 2) > 0) {
		bus_space_write_multi_4(sc->sc_iot, sc->sc_ioh,
		    MUSB2_REG_EPFIFO(ep->ep_number),
		    (void *)data, datalen >> 2);
		data += (datalen & ~0x3);
		datalen -= (datalen & ~0x3);
	}
	if (datalen) {
		bus_space_write_multi_1(sc->sc_iot, sc->sc_ioh,
		    MUSB2_REG_EPFIFO(ep->ep_number), data, datalen);
	}

	motg_setup_endpoint_tx(xfer);
	/* Max packet size */
	UWRITE2(sc, MUSB2_REG_TXMAXP,
	    UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize));
	/* Data Toggle */
	val = UREAD1(sc, MUSB2_REG_TXCSRH);
	val |= MUSB2_MASK_CSRH_TXDT_WREN;
	if (otgpipe->nexttoggle)
		val |= MUSB2_MASK_CSRH_TXDT_VAL;
	else
		val &= ~MUSB2_MASK_CSRH_TXDT_VAL;
	UWRITE1(sc, MUSB2_REG_TXCSRH, val);

	/* start transaction */
	UWRITE1(sc, MUSB2_REG_TXCSRL, MUSB2_MASK_CSRL_TXPKTRDY);
}

static void
motg_device_intr_rx(struct motg_softc *sc, int epnumber)
{
	struct motg_hw_ep *ep = &sc->sc_in_ep[epnumber];
	struct usbd_xfer *xfer = ep->xfer;
	uint8_t csr;
	int datalen, max_datalen;
	char *data;
	bool got_short;
	usbd_status new_status = USBD_IN_PROGRESS;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(ep->ep_number == epnumber);

	DPRINTFN(MD_BULK, "on ep %jd", epnumber, 0, 0, 0);
	/* select endpoint */
	UWRITE1(sc, MUSB2_REG_EPINDEX, epnumber);

	/* read out FIFO status */
	csr = UREAD1(sc, MUSB2_REG_RXCSRL);
	DPRINTFN(MD_BULK, "phase %jd csr 0x%jx", ep->phase, csr ,0 ,0);

	if ((csr & (MUSB2_MASK_CSRL_RXNAKTO | MUSB2_MASK_CSRL_RXSTALL |
	    MUSB2_MASK_CSRL_RXERROR | MUSB2_MASK_CSRL_RXPKTRDY)) == 0)
		return;

	KASSERTMSG(ep->phase == DATA_IN, "phase %d", ep->phase);
	if (csr & MUSB2_MASK_CSRL_RXNAKTO) {
		csr &= ~MUSB2_MASK_CSRL_RXREQPKT;
		UWRITE1(sc, MUSB2_REG_RXCSRL, csr);

		csr &= ~MUSB2_MASK_CSRL_RXNAKTO;
		UWRITE1(sc, MUSB2_REG_RXCSRL, csr);
		new_status = USBD_TIMEOUT; /* XXX */
		goto complete;
	}
	if (csr & (MUSB2_MASK_CSRL_RXSTALL | MUSB2_MASK_CSRL_RXERROR)) {
		if (csr & MUSB2_MASK_CSRL_RXSTALL)
			new_status = USBD_STALLED;
		else
			new_status = USBD_IOERROR;
		/* clear status */
		UWRITE1(sc, MUSB2_REG_RXCSRL, 0);
		goto complete;
	}
	KASSERT(csr & MUSB2_MASK_CSRL_RXPKTRDY);

	if (xfer == NULL || xfer->ux_status != USBD_IN_PROGRESS) {
		UWRITE1(sc, MUSB2_REG_RXCSRL, 0);
		goto complete;
	}

	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	otgpipe->nexttoggle = otgpipe->nexttoggle ^ 1;

	datalen = UREAD2(sc, MUSB2_REG_RXCOUNT);
	DPRINTFN(MD_BULK, "phase %jd datalen %jd", ep->phase, datalen ,0 ,0);
	KASSERT(UE_GET_SIZE(UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize)) > 0);
	max_datalen = min(
	    UE_GET_SIZE(UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize)),
	    ep->datalen);
	if (datalen > max_datalen) {
		new_status = USBD_IOERROR;
		UWRITE1(sc, MUSB2_REG_RXCSRL, 0);
		goto complete;
	}
	got_short = (datalen < max_datalen);
	if (datalen > 0) {
		KASSERT(ep->phase == DATA_IN);
		data = ep->data;
		ep->data += datalen;
		ep->datalen -= datalen;
		xfer->ux_actlen += datalen;
		if (((vaddr_t)data & 0x3) == 0 &&
		    (datalen >> 2) > 0) {
			DPRINTFN(MD_BULK, "r4 data %#jx len %jd",
			    (uintptr_t)data, datalen, 0, 0);
			bus_space_read_multi_4(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(ep->ep_number),
			    (void *)data, datalen >> 2);
			data += (datalen & ~0x3);
			datalen -= (datalen & ~0x3);
		}
		DPRINTFN(MD_BULK, "r1 data %#jx len %jd", (uintptr_t)data,
		    datalen ,0 ,0);
		if (datalen) {
			bus_space_read_multi_1(sc->sc_iot, sc->sc_ioh,
			    MUSB2_REG_EPFIFO(ep->ep_number), data, datalen);
		}
	}
	UWRITE1(sc, MUSB2_REG_RXCSRL, 0);
	KASSERT(ep->phase == DATA_IN);
	if (got_short || (ep->datalen == 0)) {
		if (ep->need_short_xfer == 0) {
			new_status = USBD_NORMAL_COMPLETION;
			goto complete;
		}
		ep->need_short_xfer = 0;
	}
	motg_device_data_read(xfer);
	return;
complete:
	DPRINTFN(MD_BULK, "xfer %#jx complete, status %jd", (uintptr_t)xfer,
	    (xfer != NULL) ? xfer->ux_status : 0, 0, 0);
	ep->phase = IDLE;
	ep->xfer = NULL;
	if (xfer && xfer->ux_status == USBD_IN_PROGRESS) {
		KASSERT(new_status != USBD_IN_PROGRESS);
		xfer->ux_status = new_status;
		usb_transfer_complete(xfer);
	}
	motg_device_data_start1(sc, ep);
}

static void
motg_device_intr_tx(struct motg_softc *sc, int epnumber)
{
	struct motg_hw_ep *ep = &sc->sc_out_ep[epnumber];
	struct usbd_xfer *xfer = ep->xfer;
	uint8_t csr;
	struct motg_pipe *otgpipe;
	usbd_status new_status = USBD_IN_PROGRESS;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(ep->ep_number == epnumber);

	DPRINTFN(MD_BULK, " on ep %jd", epnumber, 0, 0, 0);
	/* select endpoint */
	UWRITE1(sc, MUSB2_REG_EPINDEX, epnumber);

	csr = UREAD1(sc, MUSB2_REG_TXCSRL);
	DPRINTFN(MD_BULK, "phase %jd csr 0x%jx", ep->phase, csr, 0, 0);

	if (csr & (MUSB2_MASK_CSRL_TXSTALLED|MUSB2_MASK_CSRL_TXERROR)) {
		/* command not accepted */
		if (csr & MUSB2_MASK_CSRL_TXSTALLED)
			new_status = USBD_STALLED;
		else
			new_status = USBD_IOERROR;
		/* clear status */
		UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
		goto complete;
	}
	if (csr & MUSB2_MASK_CSRL_TXNAKTO) {
		new_status = USBD_TIMEOUT; /* XXX */
		csr &= ~MUSB2_MASK_CSRL_TXNAKTO;
		UWRITE1(sc, MUSB2_REG_TXCSRL, csr);
		/* flush fifo */
		while (csr & MUSB2_MASK_CSRL_TXFIFONEMPTY) {
			csr |= MUSB2_MASK_CSRL_TXFFLUSH;
			csr &= ~MUSB2_MASK_CSRL_TXNAKTO;
			UWRITE1(sc, MUSB2_REG_TXCSRL, csr);
			delay(1000);
			csr = UREAD1(sc, MUSB2_REG_TXCSRL);
			DPRINTFN(MD_BULK, "TX fifo flush ep %jd CSR 0x%jx",
			    epnumber, csr, 0, 0);
		}
		goto complete;
	}
	if (csr & (MUSB2_MASK_CSRL_TXFIFONEMPTY|MUSB2_MASK_CSRL_TXPKTRDY)) {
		/* data still not sent */
		return;
	}
	if (xfer == NULL || xfer->ux_status != USBD_IN_PROGRESS)
		goto complete;
	KASSERT(ep->phase == DATA_OUT);

	otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	otgpipe->nexttoggle = otgpipe->nexttoggle ^ 1;

	if (ep->datalen == 0) {
		if (ep->need_short_xfer) {
			ep->need_short_xfer = 0;
			/* one more data phase */
		} else {
			new_status = USBD_NORMAL_COMPLETION;
			goto complete;
		}
	}
	motg_device_data_write(xfer);
	return;

complete:
	DPRINTFN(MD_BULK, "xfer %#jx complete, status %jd", (uintptr_t)xfer,
	    (xfer != NULL) ? xfer->ux_status : 0, 0, 0);
#ifdef DIAGNOSTIC
	if (xfer && xfer->ux_status == USBD_IN_PROGRESS && ep->phase != DATA_OUT)
		panic("motg_device_intr_tx: bad phase %d", ep->phase);
#endif
	ep->phase = IDLE;
	ep->xfer = NULL;
	if (xfer && xfer->ux_status == USBD_IN_PROGRESS) {
		KASSERT(new_status != USBD_IN_PROGRESS);
		xfer->ux_status = new_status;
		usb_transfer_complete(xfer);
	}
	motg_device_data_start1(sc, ep);
}

/* Abort a device control request. */
void
motg_device_data_abort(struct usbd_xfer *xfer)
{
	struct motg_softc __diagused *sc = MOTG_XFER2SC(xfer);
	KASSERT(mutex_owned(&sc->sc_lock));

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	motg_device_xfer_abort(xfer);
}

/* Close a device control pipe */
void
motg_device_data_close(struct usbd_pipe *pipe)
{
	struct motg_softc *sc __diagused = MOTG_PIPE2SC(pipe);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(pipe);
	struct motg_pipe *otgpipeiter;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(otgpipe->hw_ep->xfer == NULL ||
	    otgpipe->hw_ep->xfer->ux_pipe != pipe);

	pipe->up_endpoint->ue_toggle = otgpipe->nexttoggle;
	SIMPLEQ_FOREACH(otgpipeiter, &otgpipe->hw_ep->ep_pipes, ep_pipe_list) {
		if (otgpipeiter == otgpipe) {
			/* remove from list */
			SIMPLEQ_REMOVE(&otgpipe->hw_ep->ep_pipes, otgpipe,
			    motg_pipe, ep_pipe_list);
			otgpipe->hw_ep->refcount--;
			/* we're done */
			return;
		}
	}
	panic("motg_device_data_close: not found");
}

void
motg_device_data_done(struct usbd_xfer *xfer)
{
	struct motg_pipe *otgpipe __diagused = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	KASSERT(otgpipe->hw_ep->xfer != xfer);
}

void
motg_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(pipe);
	otgpipe->nexttoggle = 0;
}

/* Abort a device control request. */
static void
motg_device_xfer_abort(struct usbd_xfer *xfer)
{
	int wake;
	uint8_t csr;
	struct motg_softc *sc = MOTG_XFER2SC(xfer);
	struct motg_pipe *otgpipe = MOTG_PIPE2MPIPE(xfer->ux_pipe);
	KASSERT(mutex_owned(&sc->sc_lock));

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (xfer->ux_hcflags & UXFER_ABORTING) {
		DPRINTF("already aborting", 0, 0, 0, 0);
		xfer->ux_hcflags |= UXFER_ABORTWAIT;
		while (xfer->ux_hcflags & UXFER_ABORTING)
			cv_wait(&xfer->ux_hccv, &sc->sc_lock);
		return;
	}
	xfer->ux_hcflags |= UXFER_ABORTING;
	if (otgpipe->hw_ep->xfer == xfer) {
		KASSERT(xfer->ux_status == USBD_IN_PROGRESS);
		otgpipe->hw_ep->xfer = NULL;
		if (otgpipe->hw_ep->ep_number > 0) {
			/* select endpoint */
			UWRITE1(sc, MUSB2_REG_EPINDEX,
			    otgpipe->hw_ep->ep_number);
			if (otgpipe->hw_ep->phase == DATA_OUT) {
				csr = UREAD1(sc, MUSB2_REG_TXCSRL);
				while (csr & MUSB2_MASK_CSRL_TXFIFONEMPTY) {
					csr |= MUSB2_MASK_CSRL_TXFFLUSH;
					UWRITE1(sc, MUSB2_REG_TXCSRL, csr);
					csr = UREAD1(sc, MUSB2_REG_TXCSRL);
				}
				UWRITE1(sc, MUSB2_REG_TXCSRL, 0);
			} else if (otgpipe->hw_ep->phase == DATA_IN) {
				csr = UREAD1(sc, MUSB2_REG_RXCSRL);
				while (csr & MUSB2_MASK_CSRL_RXPKTRDY) {
					csr |= MUSB2_MASK_CSRL_RXFFLUSH;
					UWRITE1(sc, MUSB2_REG_RXCSRL, csr);
					csr = UREAD1(sc, MUSB2_REG_RXCSRL);
				}
				UWRITE1(sc, MUSB2_REG_RXCSRL, 0);
			}
			otgpipe->hw_ep->phase = IDLE;
		}
	}
	xfer->ux_status = USBD_CANCELLED; /* make software ignore it */
	wake = xfer->ux_hcflags & UXFER_ABORTWAIT;
	xfer->ux_hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);
	usb_transfer_complete(xfer);
	if (wake)
		cv_broadcast(&xfer->ux_hccv);
}
