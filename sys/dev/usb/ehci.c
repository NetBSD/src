/*	$NetBSD: ehci.c,v 1.196 2013/01/05 23:34:16 christos Exp $ */

/*
 * Copyright (c) 2004-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net), Charles M. Hannum,
 * Jeremy Morse (jeremy.morse@gmail.com), Jared D. McNeill
 * (jmcneill@invisible.ca) and Matthew R. Green (mrg@eterna.com.au).
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
 * USB Enhanced Host Controller Driver, a.k.a. USB 2.0 controller.
 *
 * The EHCI 1.0 spec can be found at
 * http://www.intel.com/technology/usb/spec.htm
 * and the USB 2.0 spec at
 * http://www.usb.org/developers/docs/
 *
 */

/*
 * TODO:
 * 1) hold off explorations by companion controllers until ehci has started.
 *
 * 2) The hub driver needs to handle and schedule the transaction translator,
 *    to assign place in frame where different devices get to go. See chapter
 *    on hubs in USB 2.0 for details.
 *
 * 3) Command failures are not recovered correctly.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ehci.c,v 1.196 2013/01/05 23:34:16 christos Exp $");

#include "ohci.h"
#include "uhci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#include <dev/usb/usbroothub_subr.h>

#ifdef EHCI_DEBUG
#include <sys/kprintf.h>
static void
ehciprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, TOLOG|TOCONS, NULL, NULL, ap);
	va_end(ap);
}

#define DPRINTF(x)	do { if (ehcidebug) ehciprintf x; } while(0)
#define DPRINTFN(n,x)	do { if (ehcidebug>(n)) ehciprintf x; } while (0)
int ehcidebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct ehci_pipe {
	struct usbd_pipe pipe;
	int nexttoggle;

	ehci_soft_qh_t *sqh;
	union {
		ehci_soft_qtd_t *qtd;
		/* ehci_soft_itd_t *itd; */
	} tail;
	union {
		/* Control pipe */
		struct {
			usb_dma_t reqdma;
			u_int length;
		} ctl;
		/* Interrupt pipe */
		struct {
			u_int length;
		} intr;
		/* Bulk pipe */
		struct {
			u_int length;
		} bulk;
		/* Iso pipe */
		struct {
			u_int next_frame;
			u_int cur_xfers;
		} isoc;
	} u;
};

Static usbd_status	ehci_open(usbd_pipe_handle);
Static void		ehci_poll(struct usbd_bus *);
Static void		ehci_softintr(void *);
Static int		ehci_intr1(ehci_softc_t *);
Static void		ehci_waitintr(ehci_softc_t *, usbd_xfer_handle);
Static void		ehci_check_intr(ehci_softc_t *, struct ehci_xfer *);
Static void		ehci_check_qh_intr(ehci_softc_t *, struct ehci_xfer *);
Static void		ehci_check_itd_intr(ehci_softc_t *, struct ehci_xfer *);
Static void		ehci_idone(struct ehci_xfer *);
Static void		ehci_timeout(void *);
Static void		ehci_timeout_task(void *);
Static void		ehci_intrlist_timeout(void *);
Static void		ehci_doorbell(void *);
Static void		ehci_pcd(void *);

Static usbd_status	ehci_allocm(struct usbd_bus *, usb_dma_t *, u_int32_t);
Static void		ehci_freem(struct usbd_bus *, usb_dma_t *);

Static usbd_xfer_handle	ehci_allocx(struct usbd_bus *);
Static void		ehci_freex(struct usbd_bus *, usbd_xfer_handle);
Static void		ehci_get_lock(struct usbd_bus *, kmutex_t **);

Static usbd_status	ehci_root_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	ehci_root_ctrl_start(usbd_xfer_handle);
Static void		ehci_root_ctrl_abort(usbd_xfer_handle);
Static void		ehci_root_ctrl_close(usbd_pipe_handle);
Static void		ehci_root_ctrl_done(usbd_xfer_handle);

Static usbd_status	ehci_root_intr_transfer(usbd_xfer_handle);
Static usbd_status	ehci_root_intr_start(usbd_xfer_handle);
Static void		ehci_root_intr_abort(usbd_xfer_handle);
Static void		ehci_root_intr_close(usbd_pipe_handle);
Static void		ehci_root_intr_done(usbd_xfer_handle);

Static usbd_status	ehci_device_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	ehci_device_ctrl_start(usbd_xfer_handle);
Static void		ehci_device_ctrl_abort(usbd_xfer_handle);
Static void		ehci_device_ctrl_close(usbd_pipe_handle);
Static void		ehci_device_ctrl_done(usbd_xfer_handle);

Static usbd_status	ehci_device_bulk_transfer(usbd_xfer_handle);
Static usbd_status	ehci_device_bulk_start(usbd_xfer_handle);
Static void		ehci_device_bulk_abort(usbd_xfer_handle);
Static void		ehci_device_bulk_close(usbd_pipe_handle);
Static void		ehci_device_bulk_done(usbd_xfer_handle);

Static usbd_status	ehci_device_intr_transfer(usbd_xfer_handle);
Static usbd_status	ehci_device_intr_start(usbd_xfer_handle);
Static void		ehci_device_intr_abort(usbd_xfer_handle);
Static void		ehci_device_intr_close(usbd_pipe_handle);
Static void		ehci_device_intr_done(usbd_xfer_handle);

Static usbd_status	ehci_device_isoc_transfer(usbd_xfer_handle);
Static usbd_status	ehci_device_isoc_start(usbd_xfer_handle);
Static void		ehci_device_isoc_abort(usbd_xfer_handle);
Static void		ehci_device_isoc_close(usbd_pipe_handle);
Static void		ehci_device_isoc_done(usbd_xfer_handle);

Static void		ehci_device_clear_toggle(usbd_pipe_handle pipe);
Static void		ehci_noop(usbd_pipe_handle pipe);

Static void		ehci_disown(ehci_softc_t *, int, int);

Static ehci_soft_qh_t  *ehci_alloc_sqh(ehci_softc_t *);
Static void		ehci_free_sqh(ehci_softc_t *, ehci_soft_qh_t *);

Static ehci_soft_qtd_t  *ehci_alloc_sqtd(ehci_softc_t *);
Static void		ehci_free_sqtd(ehci_softc_t *, ehci_soft_qtd_t *);
Static usbd_status	ehci_alloc_sqtd_chain(struct ehci_pipe *,
			    ehci_softc_t *, int, int, usbd_xfer_handle,
			    ehci_soft_qtd_t **, ehci_soft_qtd_t **);
Static void		ehci_free_sqtd_chain(ehci_softc_t *, ehci_soft_qtd_t *,
					    ehci_soft_qtd_t *);

Static ehci_soft_itd_t	*ehci_alloc_itd(ehci_softc_t *sc);
Static void		ehci_free_itd(ehci_softc_t *sc, ehci_soft_itd_t *itd);
Static void 		ehci_rem_free_itd_chain(ehci_softc_t *sc,
						struct ehci_xfer *exfer);
Static void 		ehci_abort_isoc_xfer(usbd_xfer_handle xfer,
						usbd_status status);

Static usbd_status	ehci_device_request(usbd_xfer_handle xfer);

Static usbd_status	ehci_device_setintr(ehci_softc_t *, ehci_soft_qh_t *,
			    int ival);

Static void		ehci_add_qh(ehci_softc_t *, ehci_soft_qh_t *,
				    ehci_soft_qh_t *);
Static void		ehci_rem_qh(ehci_softc_t *, ehci_soft_qh_t *,
				    ehci_soft_qh_t *);
Static void		ehci_set_qh_qtd(ehci_soft_qh_t *, ehci_soft_qtd_t *);
Static void		ehci_sync_hc(ehci_softc_t *);

Static void		ehci_close_pipe(usbd_pipe_handle, ehci_soft_qh_t *);
Static void		ehci_abort_xfer(usbd_xfer_handle, usbd_status);

#ifdef EHCI_DEBUG
Static void		ehci_dump_regs(ehci_softc_t *);
void			ehci_dump(void);
Static ehci_softc_t 	*theehci;
Static void		ehci_dump_link(ehci_link_t, int);
Static void		ehci_dump_sqtds(ehci_soft_qtd_t *);
Static void		ehci_dump_sqtd(ehci_soft_qtd_t *);
Static void		ehci_dump_qtd(ehci_qtd_t *);
Static void		ehci_dump_sqh(ehci_soft_qh_t *);
#if notyet
Static void		ehci_dump_sitd(struct ehci_soft_itd *itd);
Static void		ehci_dump_itd(struct ehci_soft_itd *);
#endif
#ifdef DIAGNOSTIC
Static void		ehci_dump_exfer(struct ehci_xfer *);
#endif
#endif

#define EHCI_NULL htole32(EHCI_LINK_TERMINATE)

#define EHCI_INTR_ENDPT 1

#define ehci_add_intr_list(sc, ex) \
	TAILQ_INSERT_TAIL(&(sc)->sc_intrhead, (ex), inext);
#define ehci_del_intr_list(sc, ex) \
	do { \
		TAILQ_REMOVE(&sc->sc_intrhead, (ex), inext); \
		(ex)->inext.tqe_prev = NULL; \
	} while (0)
#define ehci_active_intr_list(ex) ((ex)->inext.tqe_prev != NULL)

Static const struct usbd_bus_methods ehci_bus_methods = {
	.open_pipe =	ehci_open,
	.soft_intr =	ehci_softintr,
	.do_poll =	ehci_poll,
	.allocm =	ehci_allocm,
	.freem =	ehci_freem,
	.allocx =	ehci_allocx,
	.freex =	ehci_freex,
	.get_lock =	ehci_get_lock,
};

Static const struct usbd_pipe_methods ehci_root_ctrl_methods = {
	.transfer =	ehci_root_ctrl_transfer,
	.start =	ehci_root_ctrl_start,
	.abort =	ehci_root_ctrl_abort,
	.close =	ehci_root_ctrl_close,
	.cleartoggle =	ehci_noop,
	.done =		ehci_root_ctrl_done,
};

Static const struct usbd_pipe_methods ehci_root_intr_methods = {
	.transfer =	ehci_root_intr_transfer,
	.start =	ehci_root_intr_start,
	.abort =	ehci_root_intr_abort,
	.close =	ehci_root_intr_close,
	.cleartoggle =	ehci_noop,
	.done =		ehci_root_intr_done,
};

Static const struct usbd_pipe_methods ehci_device_ctrl_methods = {
	.transfer =	ehci_device_ctrl_transfer,
	.start =	ehci_device_ctrl_start,
	.abort =	ehci_device_ctrl_abort,
	.close =	ehci_device_ctrl_close,
	.cleartoggle =	ehci_noop,
	.done =		ehci_device_ctrl_done,
};

Static const struct usbd_pipe_methods ehci_device_intr_methods = {
	.transfer =	ehci_device_intr_transfer,
	.start =	ehci_device_intr_start,
	.abort =	ehci_device_intr_abort,
	.close =	ehci_device_intr_close,
	.cleartoggle =	ehci_device_clear_toggle,
	.done =		ehci_device_intr_done,
};

Static const struct usbd_pipe_methods ehci_device_bulk_methods = {
	.transfer =	ehci_device_bulk_transfer,
	.start =	ehci_device_bulk_start,
	.abort =	ehci_device_bulk_abort,
	.close =	ehci_device_bulk_close,
	.cleartoggle =	ehci_device_clear_toggle,
	.done =		ehci_device_bulk_done,
};

Static const struct usbd_pipe_methods ehci_device_isoc_methods = {
	.transfer =	ehci_device_isoc_transfer,
	.start =	ehci_device_isoc_start,
	.abort =	ehci_device_isoc_abort,
	.close =	ehci_device_isoc_close,
	.cleartoggle =	ehci_noop,
	.done =		ehci_device_isoc_done,
};

static const uint8_t revbits[EHCI_MAX_POLLRATE] = {
0x00,0x40,0x20,0x60,0x10,0x50,0x30,0x70,0x08,0x48,0x28,0x68,0x18,0x58,0x38,0x78,
0x04,0x44,0x24,0x64,0x14,0x54,0x34,0x74,0x0c,0x4c,0x2c,0x6c,0x1c,0x5c,0x3c,0x7c,
0x02,0x42,0x22,0x62,0x12,0x52,0x32,0x72,0x0a,0x4a,0x2a,0x6a,0x1a,0x5a,0x3a,0x7a,
0x06,0x46,0x26,0x66,0x16,0x56,0x36,0x76,0x0e,0x4e,0x2e,0x6e,0x1e,0x5e,0x3e,0x7e,
0x01,0x41,0x21,0x61,0x11,0x51,0x31,0x71,0x09,0x49,0x29,0x69,0x19,0x59,0x39,0x79,
0x05,0x45,0x25,0x65,0x15,0x55,0x35,0x75,0x0d,0x4d,0x2d,0x6d,0x1d,0x5d,0x3d,0x7d,
0x03,0x43,0x23,0x63,0x13,0x53,0x33,0x73,0x0b,0x4b,0x2b,0x6b,0x1b,0x5b,0x3b,0x7b,
0x07,0x47,0x27,0x67,0x17,0x57,0x37,0x77,0x0f,0x4f,0x2f,0x6f,0x1f,0x5f,0x3f,0x7f,
};

usbd_status
ehci_init(ehci_softc_t *sc)
{
	u_int32_t vers, sparams, cparams, hcr;
	u_int i;
	usbd_status err;
	ehci_soft_qh_t *sqh;
	u_int ncomp;

	DPRINTF(("ehci_init: start\n"));
#ifdef EHCI_DEBUG
	theehci = sc;
#endif

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_softwake_cv, "ehciab");
	cv_init(&sc->sc_doorbell, "ehcidi");

	sc->sc_doorbell_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    ehci_doorbell, sc);
	sc->sc_pcd_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    ehci_pcd, sc);

	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);

	vers = EREAD2(sc, EHCI_HCIVERSION);
	aprint_verbose("%s: EHCI version %x.%x\n", device_xname(sc->sc_dev),
	       vers >> 8, vers & 0xff);

	sparams = EREAD4(sc, EHCI_HCSPARAMS);
	DPRINTF(("ehci_init: sparams=0x%x\n", sparams));
	sc->sc_npcomp = EHCI_HCS_N_PCC(sparams);
	ncomp = EHCI_HCS_N_CC(sparams);
	if (ncomp != sc->sc_ncomp) {
		aprint_verbose("%s: wrong number of companions (%d != %d)\n",
			       device_xname(sc->sc_dev), ncomp, sc->sc_ncomp);
#if NOHCI == 0 || NUHCI == 0
		aprint_error("%s: ohci or uhci probably not configured\n",
			     device_xname(sc->sc_dev));
#endif
		if (ncomp < sc->sc_ncomp)
			sc->sc_ncomp = ncomp;
	}
	if (sc->sc_ncomp > 0) {
		KASSERT(!(sc->sc_flags & EHCIF_ETTF));
		aprint_normal("%s: companion controller%s, %d port%s each:",
		    device_xname(sc->sc_dev), sc->sc_ncomp!=1 ? "s" : "",
		    EHCI_HCS_N_PCC(sparams),
		    EHCI_HCS_N_PCC(sparams)!=1 ? "s" : "");
		for (i = 0; i < sc->sc_ncomp; i++)
			aprint_normal(" %s", device_xname(sc->sc_comps[i]));
		aprint_normal("\n");
	}
	sc->sc_noport = EHCI_HCS_N_PORTS(sparams);
	cparams = EREAD4(sc, EHCI_HCCPARAMS);
	DPRINTF(("ehci_init: cparams=0x%x\n", cparams));
	sc->sc_hasppc = EHCI_HCS_PPC(sparams);

	if (EHCI_HCC_64BIT(cparams)) {
		/* MUST clear segment register if 64 bit capable. */
		EWRITE4(sc, EHCI_CTRLDSSEGMENT, 0);
	}

	sc->sc_bus.usbrev = USBREV_2_0;

	usb_setup_reserve(sc->sc_dev, &sc->sc_dma_reserve, sc->sc_bus.dmatag,
	    USB_MEM_RESERVE);

	/* Reset the controller */
	DPRINTF(("%s: resetting\n", device_xname(sc->sc_dev)));
	EOWRITE4(sc, EHCI_USBCMD, 0);	/* Halt controller */
	usb_delay_ms(&sc->sc_bus, 1);
	EOWRITE4(sc, EHCI_USBCMD, EHCI_CMD_HCRESET);
	for (i = 0; i < 100; i++) {
		usb_delay_ms(&sc->sc_bus, 1);
		hcr = EOREAD4(sc, EHCI_USBCMD) & EHCI_CMD_HCRESET;
		if (!hcr)
			break;
	}
	if (hcr) {
		aprint_error("%s: reset timeout\n", device_xname(sc->sc_dev));
		return (USBD_IOERROR);
	}
	if (sc->sc_vendor_init)
		sc->sc_vendor_init(sc);

	/*
	 * If we are doing embedded transaction translation function, force
	 * the controller to host mode.
	 */
	if (sc->sc_flags & EHCIF_ETTF) {
		uint32_t usbmode = EREAD4(sc, EHCI_USBMODE);
		usbmode &= ~EHCI_USBMODE_CM;
		usbmode |= EHCI_USBMODE_CM_HOST;
		EWRITE4(sc, EHCI_USBMODE, usbmode);
	}

	/* XXX need proper intr scheduling */
	sc->sc_rand = 96;

	/* frame list size at default, read back what we got and use that */
	switch (EHCI_CMD_FLS(EOREAD4(sc, EHCI_USBCMD))) {
	case 0: sc->sc_flsize = 1024; break;
	case 1: sc->sc_flsize = 512; break;
	case 2: sc->sc_flsize = 256; break;
	case 3: return (USBD_IOERROR);
	}
	err = usb_allocmem(&sc->sc_bus, sc->sc_flsize * sizeof(ehci_link_t),
	    EHCI_FLALIGN_ALIGN, &sc->sc_fldma);
	if (err)
		return (err);
	DPRINTF(("%s: flsize=%d\n", device_xname(sc->sc_dev),sc->sc_flsize));
	sc->sc_flist = KERNADDR(&sc->sc_fldma, 0);

	for (i = 0; i < sc->sc_flsize; i++) {
		sc->sc_flist[i] = EHCI_NULL;
	}

	EOWRITE4(sc, EHCI_PERIODICLISTBASE, DMAADDR(&sc->sc_fldma, 0));

	sc->sc_softitds = kmem_zalloc(sc->sc_flsize * sizeof(ehci_soft_itd_t *),
				     KM_SLEEP);
	if (sc->sc_softitds == NULL)
		return ENOMEM;
	LIST_INIT(&sc->sc_freeitds);
	TAILQ_INIT(&sc->sc_intrhead);

	/* Set up the bus struct. */
	sc->sc_bus.methods = &ehci_bus_methods;
	sc->sc_bus.pipe_size = sizeof(struct ehci_pipe);

	sc->sc_eintrs = EHCI_NORMAL_INTRS;

	/*
	 * Allocate the interrupt dummy QHs. These are arranged to give poll
	 * intervals that are powers of 2 times 1ms.
	 */
	for (i = 0; i < EHCI_INTRQHS; i++) {
		sqh = ehci_alloc_sqh(sc);
		if (sqh == NULL) {
			err = USBD_NOMEM;
			goto bad1;
		}
		sc->sc_islots[i].sqh = sqh;
	}
	for (i = 0; i < EHCI_INTRQHS; i++) {
		sqh = sc->sc_islots[i].sqh;
		if (i == 0) {
			/* The last (1ms) QH terminates. */
			sqh->qh.qh_link = EHCI_NULL;
			sqh->next = NULL;
		} else {
			/* Otherwise the next QH has half the poll interval */
			sqh->next = sc->sc_islots[(i + 1) / 2 - 1].sqh;
			sqh->qh.qh_link = htole32(sqh->next->physaddr |
			    EHCI_LINK_QH);
		}
		sqh->qh.qh_endp = htole32(EHCI_QH_SET_EPS(EHCI_QH_SPEED_HIGH));
		sqh->qh.qh_curqtd = EHCI_NULL;
		sqh->next = NULL;
		sqh->qh.qh_qtd.qtd_next = EHCI_NULL;
		sqh->qh.qh_qtd.qtd_altnext = EHCI_NULL;
		sqh->qh.qh_qtd.qtd_status = htole32(EHCI_QTD_HALTED);
		sqh->sqtd = NULL;
		usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}
	/* Point the frame list at the last level (128ms). */
	for (i = 0; i < sc->sc_flsize; i++) {
		int j;

		j = (i & ~(EHCI_MAX_POLLRATE-1)) |
		    revbits[i & (EHCI_MAX_POLLRATE-1)];
		sc->sc_flist[j] = htole32(EHCI_LINK_QH |
		    sc->sc_islots[EHCI_IQHIDX(EHCI_IPOLLRATES - 1,
		    i)].sqh->physaddr);
	}
	usb_syncmem(&sc->sc_fldma, 0, sc->sc_flsize * sizeof(ehci_link_t),
	    BUS_DMASYNC_PREWRITE);

	/* Allocate dummy QH that starts the async list. */
	sqh = ehci_alloc_sqh(sc);
	if (sqh == NULL) {
		err = USBD_NOMEM;
		goto bad1;
	}
	/* Fill the QH */
	sqh->qh.qh_endp =
	    htole32(EHCI_QH_SET_EPS(EHCI_QH_SPEED_HIGH) | EHCI_QH_HRECL);
	sqh->qh.qh_link =
	    htole32(sqh->physaddr | EHCI_LINK_QH);
	sqh->qh.qh_curqtd = EHCI_NULL;
	sqh->next = NULL;
	/* Fill the overlay qTD */
	sqh->qh.qh_qtd.qtd_next = EHCI_NULL;
	sqh->qh.qh_qtd.qtd_altnext = EHCI_NULL;
	sqh->qh.qh_qtd.qtd_status = htole32(EHCI_QTD_HALTED);
	sqh->sqtd = NULL;
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
#ifdef EHCI_DEBUG
	if (ehcidebug) {
		ehci_dump_sqh(sqh);
	}
#endif

	/* Point to async list */
	sc->sc_async_head = sqh;
	EOWRITE4(sc, EHCI_ASYNCLISTADDR, sqh->physaddr | EHCI_LINK_QH);

	callout_init(&sc->sc_tmo_intrlist, CALLOUT_MPSAFE);

	/* Turn on controller */
	EOWRITE4(sc, EHCI_USBCMD,
		 EHCI_CMD_ITC_2 | /* 2 microframes interrupt delay */
		 (EOREAD4(sc, EHCI_USBCMD) & EHCI_CMD_FLS_M) |
		 EHCI_CMD_ASE |
		 EHCI_CMD_PSE |
		 EHCI_CMD_RS);

	/* Take over port ownership */
	EOWRITE4(sc, EHCI_CONFIGFLAG, EHCI_CONF_CF);

	for (i = 0; i < 100; i++) {
		usb_delay_ms(&sc->sc_bus, 1);
		hcr = EOREAD4(sc, EHCI_USBSTS) & EHCI_STS_HCH;
		if (!hcr)
			break;
	}
	if (hcr) {
		aprint_error("%s: run timeout\n", device_xname(sc->sc_dev));
		return (USBD_IOERROR);
	}

	/* Enable interrupts */
	DPRINTFN(1,("ehci_init: enabling\n"));
	EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs);

	return (USBD_NORMAL_COMPLETION);

#if 0
 bad2:
	ehci_free_sqh(sc, sc->sc_async_head);
#endif
 bad1:
	usb_freemem(&sc->sc_bus, &sc->sc_fldma);
	return (err);
}

int
ehci_intr(void *v)
{
	ehci_softc_t *sc = v;
	int ret = 0;

	if (sc == NULL)
		return 0;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	/* If we get an interrupt while polling, then just ignore it. */
	if (sc->sc_bus.use_polling) {
		u_int32_t intrs = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));

		if (intrs)
			EOWRITE4(sc, EHCI_USBSTS, intrs); /* Acknowledge */
#ifdef DIAGNOSTIC
		DPRINTFN(16, ("ehci_intr: ignored interrupt while polling\n"));
#endif
		goto done;
	}

	ret = ehci_intr1(sc);

done:
	mutex_spin_exit(&sc->sc_intr_lock);
	return ret;
}

Static int
ehci_intr1(ehci_softc_t *sc)
{
	u_int32_t intrs, eintrs;

	DPRINTFN(20,("ehci_intr1: enter\n"));

	/* In case the interrupt occurs before initialization has completed. */
	if (sc == NULL) {
#ifdef DIAGNOSTIC
		printf("ehci_intr1: sc == NULL\n");
#endif
		return (0);
	}

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	intrs = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));
	if (!intrs)
		return (0);

	eintrs = intrs & sc->sc_eintrs;
	DPRINTFN(7, ("ehci_intr1: sc=%p intrs=0x%x(0x%x) eintrs=0x%x\n",
		     sc, (u_int)intrs, EOREAD4(sc, EHCI_USBSTS),
		     (u_int)eintrs));
	if (!eintrs)
		return (0);

	EOWRITE4(sc, EHCI_USBSTS, intrs); /* Acknowledge */
	sc->sc_bus.no_intrs++;
	if (eintrs & EHCI_STS_IAA) {
		DPRINTF(("ehci_intr1: door bell\n"));
		kpreempt_disable();
		softint_schedule(sc->sc_doorbell_si);
		kpreempt_enable();
		eintrs &= ~EHCI_STS_IAA;
	}
	if (eintrs & (EHCI_STS_INT | EHCI_STS_ERRINT)) {
		DPRINTFN(5,("ehci_intr1: %s %s\n",
			    eintrs & EHCI_STS_INT ? "INT" : "",
			    eintrs & EHCI_STS_ERRINT ? "ERRINT" : ""));
		usb_schedsoftintr(&sc->sc_bus);
		eintrs &= ~(EHCI_STS_INT | EHCI_STS_ERRINT);
	}
	if (eintrs & EHCI_STS_HSE) {
		printf("%s: unrecoverable error, controller halted\n",
		       device_xname(sc->sc_dev));
		/* XXX what else */
	}
	if (eintrs & EHCI_STS_PCD) {
		kpreempt_disable();
		softint_schedule(sc->sc_pcd_si);
		kpreempt_enable();
		eintrs &= ~EHCI_STS_PCD;
	}

	if (eintrs != 0) {
		/* Block unprocessed interrupts. */
		sc->sc_eintrs &= ~eintrs;
		EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs);
		printf("%s: blocking intrs 0x%x\n",
		       device_xname(sc->sc_dev), eintrs);
	}

	return (1);
}

Static void
ehci_doorbell(void *addr)
{
	ehci_softc_t *sc = addr;

	mutex_enter(&sc->sc_lock);
	cv_broadcast(&sc->sc_doorbell);
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_pcd(void *addr)
{
	ehci_softc_t *sc = addr;
	usbd_xfer_handle xfer;
	usbd_pipe_handle pipe;
	u_char *p;
	int i, m;

	mutex_enter(&sc->sc_lock);
	xfer = sc->sc_intrxfer;

	if (xfer == NULL) {
		/* Just ignore the change. */
		goto done;
	}

	pipe = xfer->pipe;

	p = KERNADDR(&xfer->dmabuf, 0);
	m = min(sc->sc_noport, xfer->length * 8 - 1);
	memset(p, 0, xfer->length);
	for (i = 1; i <= m; i++) {
		/* Pick out CHANGE bits from the status reg. */
		if (EOREAD4(sc, EHCI_PORTSC(i)) & EHCI_PS_CLEAR)
			p[i/8] |= 1 << (i%8);
	}
	DPRINTF(("ehci_pcd: change=0x%02x\n", *p));
	xfer->actlen = xfer->length;
	xfer->status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);

done:
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_softintr(void *v)
{
	struct usbd_bus *bus = v;
	ehci_softc_t *sc = bus->hci_private;
	struct ehci_xfer *ex, *nextex;

	KASSERT(sc->sc_bus.use_polling || mutex_owned(&sc->sc_lock));

	DPRINTFN(10,("%s: ehci_softintr\n", device_xname(sc->sc_dev)));

	/*
	 * The only explanation I can think of for why EHCI is as brain dead
	 * as UHCI interrupt-wise is that Intel was involved in both.
	 * An interrupt just tells us that something is done, we have no
	 * clue what, so we need to scan through all active transfers. :-(
	 */
	for (ex = TAILQ_FIRST(&sc->sc_intrhead); ex; ex = nextex) {
		nextex = TAILQ_NEXT(ex, inext);
		ehci_check_intr(sc, ex);
	}

	/* Schedule a callout to catch any dropped transactions. */
	if ((sc->sc_flags & EHCIF_DROPPED_INTR_WORKAROUND) &&
	    !TAILQ_EMPTY(&sc->sc_intrhead))
		callout_reset(&sc->sc_tmo_intrlist,
		    hz, ehci_intrlist_timeout, sc);

	if (sc->sc_softwake) {
		sc->sc_softwake = 0;
		cv_broadcast(&sc->sc_softwake_cv);
	}
}

/* Check for an interrupt. */
Static void
ehci_check_intr(ehci_softc_t *sc, struct ehci_xfer *ex)
{
	int attr;

	DPRINTFN(/*15*/2, ("ehci_check_intr: ex=%p\n", ex));

	KASSERT(mutex_owned(&sc->sc_lock));

	attr = ex->xfer.pipe->endpoint->edesc->bmAttributes;
	if (UE_GET_XFERTYPE(attr) == UE_ISOCHRONOUS)
		ehci_check_itd_intr(sc, ex);
	else
		ehci_check_qh_intr(sc, ex);

	return;
}

Static void
ehci_check_qh_intr(ehci_softc_t *sc, struct ehci_xfer *ex)
{
	ehci_soft_qtd_t *sqtd, *lsqtd;
	__uint32_t status;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (ex->sqtdstart == NULL) {
		printf("ehci_check_qh_intr: not valid sqtd\n");
		return;
	}

	lsqtd = ex->sqtdend;
#ifdef DIAGNOSTIC
	if (lsqtd == NULL) {
		printf("ehci_check_qh_intr: lsqtd==0\n");
		return;
	}
#endif
	/*
	 * If the last TD is still active we need to check whether there
	 * is a an error somewhere in the middle, or whether there was a
	 * short packet (SPD and not ACTIVE).
	 */
	usb_syncmem(&lsqtd->dma,
	    lsqtd->offs + offsetof(ehci_qtd_t, qtd_status),
	    sizeof(lsqtd->qtd.qtd_status),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	if (le32toh(lsqtd->qtd.qtd_status) & EHCI_QTD_ACTIVE) {
		DPRINTFN(12, ("ehci_check_intr: active ex=%p\n", ex));
		for (sqtd = ex->sqtdstart; sqtd != lsqtd; sqtd=sqtd->nextqtd) {
			usb_syncmem(&sqtd->dma,
			    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
			    sizeof(sqtd->qtd.qtd_status),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
			status = le32toh(sqtd->qtd.qtd_status);
			usb_syncmem(&sqtd->dma,
			    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
			    sizeof(sqtd->qtd.qtd_status), BUS_DMASYNC_PREREAD);
			/* If there's an active QTD the xfer isn't done. */
			if (status & EHCI_QTD_ACTIVE)
				break;
			/* Any kind of error makes the xfer done. */
			if (status & EHCI_QTD_HALTED)
				goto done;
			/* We want short packets, and it is short: it's done */
			if (EHCI_QTD_GET_BYTES(status) != 0)
				goto done;
		}
		DPRINTFN(12, ("ehci_check_intr: ex=%p std=%p still active\n",
			      ex, ex->sqtdstart));
		usb_syncmem(&lsqtd->dma,
		    lsqtd->offs + offsetof(ehci_qtd_t, qtd_status),
		    sizeof(lsqtd->qtd.qtd_status), BUS_DMASYNC_PREREAD);
		return;
	}
 done:
	DPRINTFN(12, ("ehci_check_intr: ex=%p done\n", ex));
	callout_stop(&ex->xfer.timeout_handle);
	ehci_idone(ex);
}

Static void
ehci_check_itd_intr(ehci_softc_t *sc, struct ehci_xfer *ex)
{
	ehci_soft_itd_t *itd;
	int i;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (&ex->xfer != SIMPLEQ_FIRST(&ex->xfer.pipe->queue))
		return;

	if (ex->itdstart == NULL) {
		printf("ehci_check_itd_intr: not valid itd\n");
		return;
	}

	itd = ex->itdend;
#ifdef DIAGNOSTIC
	if (itd == NULL) {
		printf("ehci_check_itd_intr: itdend == 0\n");
		return;
	}
#endif

	/*
	 * check no active transfers in last itd, meaning we're finished
	 */

	usb_syncmem(&itd->dma, itd->offs + offsetof(ehci_itd_t, itd_ctl),
		    sizeof(itd->itd.itd_ctl), BUS_DMASYNC_POSTWRITE |
		    BUS_DMASYNC_POSTREAD);

	for (i = 0; i < EHCI_ITD_NUFRAMES; i++) {
		if (le32toh(itd->itd.itd_ctl[i]) & EHCI_ITD_ACTIVE)
			break;
	}

	if (i == EHCI_ITD_NUFRAMES) {
		goto done; /* All 8 descriptors inactive, it's done */
	}

	DPRINTFN(12, ("ehci_check_itd_intr: ex %p itd %p still active\n", ex,
			ex->itdstart));
	return;
done:
	DPRINTFN(12, ("ehci_check_itd_intr: ex=%p done\n", ex));
	callout_stop(&ex->xfer.timeout_handle);
	ehci_idone(ex);
}

Static void
ehci_idone(struct ehci_xfer *ex)
{
	usbd_xfer_handle xfer = &ex->xfer;
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	struct ehci_softc *sc = xfer->pipe->device->bus->hci_private;
	ehci_soft_qtd_t *sqtd, *lsqtd;
	u_int32_t status = 0, nstatus = 0;
	int actlen;

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(/*12*/2, ("ehci_idone: ex=%p\n", ex));

#ifdef DIAGNOSTIC
	{
		if (ex->isdone) {
#ifdef EHCI_DEBUG
			printf("ehci_idone: ex is done!\n   ");
			ehci_dump_exfer(ex);
#else
			printf("ehci_idone: ex=%p is done!\n", ex);
#endif
			return;
		}
		ex->isdone = 1;
	}
#endif
	if (xfer->status == USBD_CANCELLED ||
	    xfer->status == USBD_TIMEOUT) {
		DPRINTF(("ehci_idone: aborted xfer=%p\n", xfer));
		return;
	}

#ifdef EHCI_DEBUG
	DPRINTFN(/*10*/2, ("ehci_idone: xfer=%p, pipe=%p ready\n", xfer, epipe));
	if (ehcidebug > 10)
		ehci_dump_sqtds(ex->sqtdstart);
#endif

	/* The transfer is done, compute actual length and status. */

	if (UE_GET_XFERTYPE(xfer->pipe->endpoint->edesc->bmAttributes)
				== UE_ISOCHRONOUS) {
		/* Isoc transfer */
		struct ehci_soft_itd *itd;
		int i, nframes, len, uframes;

		nframes = 0;
		actlen = 0;

		i = xfer->pipe->endpoint->edesc->bInterval;
		uframes = min(1 << (i - 1), USB_UFRAMES_PER_FRAME);

		for (itd = ex->itdstart; itd != NULL; itd = itd->xfer_next) {
			usb_syncmem(&itd->dma,itd->offs + offsetof(ehci_itd_t,itd_ctl),
			    sizeof(itd->itd.itd_ctl), BUS_DMASYNC_POSTWRITE |
			    BUS_DMASYNC_POSTREAD);

			for (i = 0; i < EHCI_ITD_NUFRAMES; i += uframes) {
				/* XXX - driver didn't fill in the frame full
				 *   of uframes. This leads to scheduling
				 *   inefficiencies, but working around
				 *   this doubles complexity of tracking
				 *   an xfer.
				 */
				if (nframes >= xfer->nframes)
					break;

				status = le32toh(itd->itd.itd_ctl[i]);
				len = EHCI_ITD_GET_LEN(status);
				if (EHCI_ITD_GET_STATUS(status) != 0)
					len = 0; /*No valid data on error*/

				xfer->frlengths[nframes++] = len;
				actlen += len;
			}

			if (nframes >= xfer->nframes)
				break;
	    	}

		xfer->actlen = actlen;
		xfer->status = USBD_NORMAL_COMPLETION;
		goto end;
	}

	/* Continue processing xfers using queue heads */

	lsqtd = ex->sqtdend;
	actlen = 0;
	for (sqtd = ex->sqtdstart; sqtd != lsqtd->nextqtd; sqtd = sqtd->nextqtd) {
		usb_syncmem(&sqtd->dma, sqtd->offs, sizeof(sqtd->qtd),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		nstatus = le32toh(sqtd->qtd.qtd_status);
		if (nstatus & EHCI_QTD_ACTIVE)
			break;

		status = nstatus;
		if (EHCI_QTD_GET_PID(status) != EHCI_QTD_PID_SETUP)
			actlen += sqtd->len - EHCI_QTD_GET_BYTES(status);
	}


	/*
	 * If there are left over TDs we need to update the toggle.
	 * The default pipe doesn't need it since control transfers
	 * start the toggle at 0 every time.
	 * For a short transfer we need to update the toggle for the missing
	 * packets within the qTD.
	 */
	if ((sqtd != lsqtd->nextqtd || EHCI_QTD_GET_BYTES(status)) &&
	    xfer->pipe->device->default_pipe != xfer->pipe) {
		DPRINTFN(2, ("ehci_idone: need toggle update "
			     "status=%08x nstatus=%08x\n", status, nstatus));
#if 0
		ehci_dump_sqh(epipe->sqh);
		ehci_dump_sqtds(ex->sqtdstart);
#endif
		epipe->nexttoggle = EHCI_QTD_GET_TOGGLE(nstatus);
	}

	DPRINTFN(/*10*/2, ("ehci_idone: len=%d, actlen=%d, status=0x%x\n",
			   xfer->length, actlen, status));
	xfer->actlen = actlen;
	if (status & EHCI_QTD_HALTED) {
#ifdef EHCI_DEBUG
		char sbuf[128];

		snprintb(sbuf, sizeof(sbuf),
		    "\20\7HALTED\6BUFERR\5BABBLE\4XACTERR\3MISSED\1PINGSTATE",
		    (u_int32_t)status);

		DPRINTFN(2, ("ehci_idone: error, addr=%d, endpt=0x%02x, "
			  "status 0x%s\n",
			  xfer->pipe->device->address,
			  xfer->pipe->endpoint->edesc->bEndpointAddress,
			  sbuf));
		if (ehcidebug > 2) {
			ehci_dump_sqh(epipe->sqh);
			ehci_dump_sqtds(ex->sqtdstart);
		}
#endif
		/* low&full speed has an extra error flag */
		if (EHCI_QH_GET_EPS(epipe->sqh->qh.qh_endp) !=
		    EHCI_QH_SPEED_HIGH)
			status &= EHCI_QTD_STATERRS | EHCI_QTD_PINGSTATE;
		else
			status &= EHCI_QTD_STATERRS;
		if (status == 0) /* no other errors means a stall */ {
			xfer->status = USBD_STALLED;
		} else {
			xfer->status = USBD_IOERROR; /* more info XXX */
		}
		/* XXX need to reset TT on missed microframe */
		if (status & EHCI_QTD_MISSEDMICRO) {
			printf("%s: missed microframe, TT reset not "
			    "implemented, hub might be inoperational\n",
			    device_xname(sc->sc_dev));
		}
	} else {
		xfer->status = USBD_NORMAL_COMPLETION;
	}

    end:
	/* XXX transfer_complete memcpys out transfer data (for in endpoints)
	 * during this call, before methods->done is called: dma sync required
	 * beforehand? */
	usb_transfer_complete(xfer);
	DPRINTFN(/*12*/2, ("ehci_idone: ex=%p done\n", ex));
}

/*
 * Wait here until controller claims to have an interrupt.
 * Then call ehci_intr and return.  Use timeout to avoid waiting
 * too long.
 */
Static void
ehci_waitintr(ehci_softc_t *sc, usbd_xfer_handle xfer)
{
	int timo;
	u_int32_t intrs;

	xfer->status = USBD_IN_PROGRESS;
	for (timo = xfer->timeout; timo >= 0; timo--) {
		usb_delay_ms(&sc->sc_bus, 1);
		if (sc->sc_dying)
			break;
		intrs = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS)) &
			sc->sc_eintrs;
		DPRINTFN(15,("ehci_waitintr: 0x%04x\n", intrs));
#ifdef EHCI_DEBUG
		if (ehcidebug > 15)
			ehci_dump_regs(sc);
#endif
		if (intrs) {
			mutex_spin_enter(&sc->sc_intr_lock);
			ehci_intr1(sc);
			mutex_spin_exit(&sc->sc_intr_lock);
			if (xfer->status != USBD_IN_PROGRESS)
				return;
		}
	}

	/* Timeout */
	DPRINTF(("ehci_waitintr: timeout\n"));
	xfer->status = USBD_TIMEOUT;
	mutex_enter(&sc->sc_lock);
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
	/* XXX should free TD */
}

Static void
ehci_poll(struct usbd_bus *bus)
{
	ehci_softc_t *sc = bus->hci_private;
#ifdef EHCI_DEBUG
	static int last;
	int new;
	new = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));
	if (new != last) {
		DPRINTFN(10,("ehci_poll: intrs=0x%04x\n", new));
		last = new;
	}
#endif

	if (EOREAD4(sc, EHCI_USBSTS) & sc->sc_eintrs) {
		mutex_spin_enter(&sc->sc_intr_lock);
		ehci_intr1(sc);
		mutex_spin_exit(&sc->sc_intr_lock);
	}
}

void
ehci_childdet(device_t self, device_t child)
{
	struct ehci_softc *sc = device_private(self);

	KASSERT(sc->sc_child == child);
	sc->sc_child = NULL;
}

int
ehci_detach(struct ehci_softc *sc, int flags)
{
	usbd_xfer_handle xfer;
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	if (rv != 0)
		return (rv);

	callout_halt(&sc->sc_tmo_intrlist, NULL);
	callout_destroy(&sc->sc_tmo_intrlist);

	/* XXX free other data structures XXX */
	if (sc->sc_softitds)
		kmem_free(sc->sc_softitds,
		    sc->sc_flsize * sizeof(ehci_soft_itd_t *));
	cv_destroy(&sc->sc_doorbell);
	cv_destroy(&sc->sc_softwake_cv);

#if 0
	/* XXX destroyed in ehci_pci.c as it controls ehci_intr access */

	softint_disestablish(sc->sc_doorbell_si);
	softint_disestablish(sc->sc_pcd_si);

	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);
#endif

	while ((xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
		kmem_free(xfer, sizeof(struct ehci_xfer));
	}

	EOWRITE4(sc, EHCI_CONFIGFLAG, 0);

	return (rv);
}


int
ehci_activate(device_t self, enum devact act)
{
	struct ehci_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

/*
 * Handle suspend/resume.
 *
 * We need to switch to polling mode here, because this routine is
 * called from an interrupt context.  This is all right since we
 * are almost suspended anyway.
 *
 * Note that this power handler isn't to be registered directly; the
 * bus glue needs to call out to it.
 */
bool
ehci_suspend(device_t dv, const pmf_qual_t *qual)
{
	ehci_softc_t *sc = device_private(dv);
	int i;
	uint32_t cmd, hcr;

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.use_polling++;
	mutex_spin_exit(&sc->sc_intr_lock);

	for (i = 1; i <= sc->sc_noport; i++) {
		cmd = EOREAD4(sc, EHCI_PORTSC(i)) & ~EHCI_PS_CLEAR;
		if ((cmd & EHCI_PS_PO) == 0 && (cmd & EHCI_PS_PE) == EHCI_PS_PE)
			EOWRITE4(sc, EHCI_PORTSC(i), cmd | EHCI_PS_SUSP);
	}

	sc->sc_cmd = EOREAD4(sc, EHCI_USBCMD);

	cmd = sc->sc_cmd & ~(EHCI_CMD_ASE | EHCI_CMD_PSE);
	EOWRITE4(sc, EHCI_USBCMD, cmd);

	for (i = 0; i < 100; i++) {
		hcr = EOREAD4(sc, EHCI_USBSTS) & (EHCI_STS_ASS | EHCI_STS_PSS);
		if (hcr == 0)
			break;

		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (hcr != 0)
		printf("%s: reset timeout\n", device_xname(dv));

	cmd &= ~EHCI_CMD_RS;
	EOWRITE4(sc, EHCI_USBCMD, cmd);

	for (i = 0; i < 100; i++) {
		hcr = EOREAD4(sc, EHCI_USBSTS) & EHCI_STS_HCH;
		if (hcr == EHCI_STS_HCH)
			break;

		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (hcr != EHCI_STS_HCH)
		printf("%s: config timeout\n", device_xname(dv));

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.use_polling--;
	mutex_spin_exit(&sc->sc_intr_lock);

	return true;
}

bool
ehci_resume(device_t dv, const pmf_qual_t *qual)
{
	ehci_softc_t *sc = device_private(dv);
	int i;
	uint32_t cmd, hcr;

	/* restore things in case the bios sucks */
	EOWRITE4(sc, EHCI_CTRLDSSEGMENT, 0);
	EOWRITE4(sc, EHCI_PERIODICLISTBASE, DMAADDR(&sc->sc_fldma, 0));
	EOWRITE4(sc, EHCI_ASYNCLISTADDR,
	    sc->sc_async_head->physaddr | EHCI_LINK_QH);

	EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs & ~EHCI_INTR_PCIE);

	EOWRITE4(sc, EHCI_USBCMD, sc->sc_cmd);

	hcr = 0;
	for (i = 1; i <= sc->sc_noport; i++) {
		cmd = EOREAD4(sc, EHCI_PORTSC(i)) & ~EHCI_PS_CLEAR;
		if ((cmd & EHCI_PS_PO) == 0 &&
		    (cmd & EHCI_PS_SUSP) == EHCI_PS_SUSP) {
			EOWRITE4(sc, EHCI_PORTSC(i), cmd | EHCI_PS_FPR);
			hcr = 1;
		}
	}

	if (hcr) {
		usb_delay_ms(&sc->sc_bus, USB_RESUME_WAIT);

		for (i = 1; i <= sc->sc_noport; i++) {
			cmd = EOREAD4(sc, EHCI_PORTSC(i)) & ~EHCI_PS_CLEAR;
			if ((cmd & EHCI_PS_PO) == 0 &&
			    (cmd & EHCI_PS_SUSP) == EHCI_PS_SUSP)
				EOWRITE4(sc, EHCI_PORTSC(i),
				    cmd & ~EHCI_PS_FPR);
		}
	}

	EOWRITE4(sc, EHCI_USBCMD, sc->sc_cmd);
	EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs);

	for (i = 0; i < 100; i++) {
		hcr = EOREAD4(sc, EHCI_USBSTS) & EHCI_STS_HCH;
		if (hcr != EHCI_STS_HCH)
			break;

		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (hcr == EHCI_STS_HCH)
		printf("%s: config timeout\n", device_xname(dv));

	return true;
}

/*
 * Shut down the controller when the system is going down.
 */
bool
ehci_shutdown(device_t self, int flags)
{
	ehci_softc_t *sc = device_private(self);

	DPRINTF(("ehci_shutdown: stopping the HC\n"));
	EOWRITE4(sc, EHCI_USBCMD, 0);	/* Halt controller */
	EOWRITE4(sc, EHCI_USBCMD, EHCI_CMD_HCRESET);
	return true;
}

Static usbd_status
ehci_allocm(struct usbd_bus *bus, usb_dma_t *dma, u_int32_t size)
{
	struct ehci_softc *sc = bus->hci_private;
	usbd_status err;

	err = usb_allocmem(&sc->sc_bus, size, 0, dma);
	if (err == USBD_NOMEM)
		err = usb_reserve_allocm(&sc->sc_dma_reserve, dma, size);
#ifdef EHCI_DEBUG
	if (err)
		printf("ehci_allocm: usb_allocmem()=%d\n", err);
#endif
	return (err);
}

Static void
ehci_freem(struct usbd_bus *bus, usb_dma_t *dma)
{
	struct ehci_softc *sc = bus->hci_private;

	if (dma->block->flags & USB_DMA_RESERVE) {
		usb_reserve_freem(&sc->sc_dma_reserve,
		    dma);
		return;
	}
	usb_freemem(&sc->sc_bus, dma);
}

Static usbd_xfer_handle
ehci_allocx(struct usbd_bus *bus)
{
	struct ehci_softc *sc = bus->hci_private;
	usbd_xfer_handle xfer;

	xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers);
	if (xfer != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
#ifdef DIAGNOSTIC
		if (xfer->busy_free != XFER_FREE) {
			printf("ehci_allocx: xfer=%p not free, 0x%08x\n", xfer,
			       xfer->busy_free);
		}
#endif
	} else {
		xfer = kmem_alloc(sizeof(struct ehci_xfer), KM_SLEEP);
	}
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct ehci_xfer));
#ifdef DIAGNOSTIC
		EXFER(xfer)->isdone = 1;
		xfer->busy_free = XFER_BUSY;
#endif
	}
	return (xfer);
}

Static void
ehci_freex(struct usbd_bus *bus, usbd_xfer_handle xfer)
{
	struct ehci_softc *sc = bus->hci_private;

#ifdef DIAGNOSTIC
	if (xfer->busy_free != XFER_BUSY) {
		printf("ehci_freex: xfer=%p not busy, 0x%08x\n", xfer,
		       xfer->busy_free);
	}
	xfer->busy_free = XFER_FREE;
	if (!EXFER(xfer)->isdone) {
		printf("ehci_freex: !isdone\n");
	}
#endif
	SIMPLEQ_INSERT_HEAD(&sc->sc_free_xfers, xfer, next);
}

Static void
ehci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct ehci_softc *sc = bus->hci_private;

	*lock = &sc->sc_lock;
}

Static void
ehci_device_clear_toggle(usbd_pipe_handle pipe)
{
	struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;

	DPRINTF(("ehci_device_clear_toggle: epipe=%p status=0x%x\n",
		 epipe, epipe->sqh->qh.qh_qtd.qtd_status));
#ifdef EHCI_DEBUG
	if (ehcidebug)
		usbd_dump_pipe(pipe);
#endif
	epipe->nexttoggle = 0;
}

Static void
ehci_noop(usbd_pipe_handle pipe)
{
}

#ifdef EHCI_DEBUG
Static void
ehci_dump_regs(ehci_softc_t *sc)
{
	int i;
	printf("cmd=0x%08x, sts=0x%08x, ien=0x%08x\n",
	       EOREAD4(sc, EHCI_USBCMD),
	       EOREAD4(sc, EHCI_USBSTS),
	       EOREAD4(sc, EHCI_USBINTR));
	printf("frindex=0x%08x ctrdsegm=0x%08x periodic=0x%08x async=0x%08x\n",
	       EOREAD4(sc, EHCI_FRINDEX),
	       EOREAD4(sc, EHCI_CTRLDSSEGMENT),
	       EOREAD4(sc, EHCI_PERIODICLISTBASE),
	       EOREAD4(sc, EHCI_ASYNCLISTADDR));
	for (i = 1; i <= sc->sc_noport; i++)
		printf("port %d status=0x%08x\n", i,
		       EOREAD4(sc, EHCI_PORTSC(i)));
}

/*
 * Unused function - this is meant to be called from a kernel
 * debugger.
 */
void
ehci_dump(void)
{
	ehci_dump_regs(theehci);
}

Static void
ehci_dump_link(ehci_link_t link, int type)
{
	link = le32toh(link);
	printf("0x%08x", link);
	if (link & EHCI_LINK_TERMINATE)
		printf("<T>");
	else {
		printf("<");
		if (type) {
			switch (EHCI_LINK_TYPE(link)) {
			case EHCI_LINK_ITD: printf("ITD"); break;
			case EHCI_LINK_QH: printf("QH"); break;
			case EHCI_LINK_SITD: printf("SITD"); break;
			case EHCI_LINK_FSTN: printf("FSTN"); break;
			}
		}
		printf(">");
	}
}

Static void
ehci_dump_sqtds(ehci_soft_qtd_t *sqtd)
{
	int i;
	u_int32_t stop;

	stop = 0;
	for (i = 0; sqtd && i < 20 && !stop; sqtd = sqtd->nextqtd, i++) {
		ehci_dump_sqtd(sqtd);
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_next),
		    sizeof(sqtd->qtd),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		stop = sqtd->qtd.qtd_next & htole32(EHCI_LINK_TERMINATE);
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_next),
		    sizeof(sqtd->qtd), BUS_DMASYNC_PREREAD);
	}
	if (sqtd)
		printf("dump aborted, too many TDs\n");
}

Static void
ehci_dump_sqtd(ehci_soft_qtd_t *sqtd)
{
	usb_syncmem(&sqtd->dma, sqtd->offs,
	    sizeof(sqtd->qtd), BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	printf("QTD(%p) at 0x%08x:\n", sqtd, sqtd->physaddr);
	ehci_dump_qtd(&sqtd->qtd);
	usb_syncmem(&sqtd->dma, sqtd->offs,
	    sizeof(sqtd->qtd), BUS_DMASYNC_PREREAD);
}

Static void
ehci_dump_qtd(ehci_qtd_t *qtd)
{
	u_int32_t s;
	char sbuf[128];

	printf("  next="); ehci_dump_link(qtd->qtd_next, 0);
	printf(" altnext="); ehci_dump_link(qtd->qtd_altnext, 0);
	printf("\n");
	s = le32toh(qtd->qtd_status);
	snprintb(sbuf, sizeof(sbuf),
	    "\20\10ACTIVE\7HALTED\6BUFERR\5BABBLE\4XACTERR"
	    "\3MISSED\2SPLIT\1PING", EHCI_QTD_GET_STATUS(s));
	printf("  status=0x%08x: toggle=%d bytes=0x%x ioc=%d c_page=0x%x\n",
	       s, EHCI_QTD_GET_TOGGLE(s), EHCI_QTD_GET_BYTES(s),
	       EHCI_QTD_GET_IOC(s), EHCI_QTD_GET_C_PAGE(s));
	printf("    cerr=%d pid=%d stat=0x%s\n", EHCI_QTD_GET_CERR(s),
	       EHCI_QTD_GET_PID(s), sbuf);
	for (s = 0; s < 5; s++)
		printf("  buffer[%d]=0x%08x\n", s, le32toh(qtd->qtd_buffer[s]));
}

Static void
ehci_dump_sqh(ehci_soft_qh_t *sqh)
{
	ehci_qh_t *qh = &sqh->qh;
	u_int32_t endp, endphub;

	usb_syncmem(&sqh->dma, sqh->offs,
	    sizeof(sqh->qh), BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	printf("QH(%p) at 0x%08x:\n", sqh, sqh->physaddr);
	printf("  link="); ehci_dump_link(qh->qh_link, 1); printf("\n");
	endp = le32toh(qh->qh_endp);
	printf("  endp=0x%08x\n", endp);
	printf("    addr=0x%02x inact=%d endpt=%d eps=%d dtc=%d hrecl=%d\n",
	       EHCI_QH_GET_ADDR(endp), EHCI_QH_GET_INACT(endp),
	       EHCI_QH_GET_ENDPT(endp),  EHCI_QH_GET_EPS(endp),
	       EHCI_QH_GET_DTC(endp), EHCI_QH_GET_HRECL(endp));
	printf("    mpl=0x%x ctl=%d nrl=%d\n",
	       EHCI_QH_GET_MPL(endp), EHCI_QH_GET_CTL(endp),
	       EHCI_QH_GET_NRL(endp));
	endphub = le32toh(qh->qh_endphub);
	printf("  endphub=0x%08x\n", endphub);
	printf("    smask=0x%02x cmask=0x%02x huba=0x%02x port=%d mult=%d\n",
	       EHCI_QH_GET_SMASK(endphub), EHCI_QH_GET_CMASK(endphub),
	       EHCI_QH_GET_HUBA(endphub), EHCI_QH_GET_PORT(endphub),
	       EHCI_QH_GET_MULT(endphub));
	printf("  curqtd="); ehci_dump_link(qh->qh_curqtd, 0); printf("\n");
	printf("Overlay qTD:\n");
	ehci_dump_qtd(&qh->qh_qtd);
	usb_syncmem(&sqh->dma, sqh->offs,
	    sizeof(sqh->qh), BUS_DMASYNC_PREREAD);
}

#if notyet
Static void
ehci_dump_itd(struct ehci_soft_itd *itd)
{
	ehci_isoc_trans_t t;
	ehci_isoc_bufr_ptr_t b, b2, b3;
	int i;

	printf("ITD: next phys=%X\n", itd->itd.itd_next);

	for (i = 0; i < EHCI_ITD_NUFRAMES; i++) {
		t = le32toh(itd->itd.itd_ctl[i]);
		printf("ITDctl %d: stat=%X len=%X ioc=%X pg=%X offs=%X\n", i,
		    EHCI_ITD_GET_STATUS(t), EHCI_ITD_GET_LEN(t),
		    EHCI_ITD_GET_IOC(t), EHCI_ITD_GET_PG(t),
		    EHCI_ITD_GET_OFFS(t));
	}
	printf("ITDbufr: ");
	for (i = 0; i < EHCI_ITD_NBUFFERS; i++)
		printf("%X,", EHCI_ITD_GET_BPTR(le32toh(itd->itd.itd_bufr[i])));

	b = le32toh(itd->itd.itd_bufr[0]);
	b2 = le32toh(itd->itd.itd_bufr[1]);
	b3 = le32toh(itd->itd.itd_bufr[2]);
	printf("\nep=%X daddr=%X dir=%d maxpkt=%X multi=%X\n",
	    EHCI_ITD_GET_EP(b), EHCI_ITD_GET_DADDR(b), EHCI_ITD_GET_DIR(b2),
	    EHCI_ITD_GET_MAXPKT(b2), EHCI_ITD_GET_MULTI(b3));
}

Static void
ehci_dump_sitd(struct ehci_soft_itd *itd)
{
	printf("SITD %p next=%p prev=%p xfernext=%p physaddr=%X slot=%d\n",
			itd, itd->u.frame_list.next, itd->u.frame_list.prev,
			itd->xfer_next, itd->physaddr, itd->slot);
}
#endif

#ifdef DIAGNOSTIC
Static void
ehci_dump_exfer(struct ehci_xfer *ex)
{
	printf("ehci_dump_exfer: ex=%p sqtdstart=%p end=%p itdstart=%p end=%p isdone=%d\n", ex, ex->sqtdstart, ex->sqtdend, ex->itdstart, ex->itdend, ex->isdone);
}
#endif
#endif

Static usbd_status
ehci_open(usbd_pipe_handle pipe)
{
	usbd_device_handle dev = pipe->device;
	ehci_softc_t *sc = dev->bus->hci_private;
	usb_endpoint_descriptor_t *ed = pipe->endpoint->edesc;
	u_int8_t addr = dev->address;
	u_int8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int ival, speed, naks;
	int hshubaddr, hshubport;

	DPRINTFN(1, ("ehci_open: pipe=%p, addr=%d, endpt=%d (%d)\n",
		     pipe, addr, ed->bEndpointAddress, sc->sc_addr));

	if (dev->myhsport) {
		/*
		 * When directly attached FS/LS device while doing embedded
		 * transaction translations and we are the hub, set the hub
		 * address to 0 (us).
		 */
		if (!(sc->sc_flags & EHCIF_ETTF)
		    || (dev->myhsport->parent->address != sc->sc_addr)) {
			hshubaddr = dev->myhsport->parent->address;
		} else {
			hshubaddr = 0;
		}
		hshubport = dev->myhsport->portno;
	} else {
		hshubaddr = 0;
		hshubport = 0;
	}

	if (sc->sc_dying)
		return (USBD_IOERROR);

	/* toggle state needed for bulk endpoints */
	epipe->nexttoggle = pipe->endpoint->datatoggle;

	if (addr == sc->sc_addr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->methods = &ehci_root_ctrl_methods;
			break;
		case UE_DIR_IN | EHCI_INTR_ENDPT:
			pipe->methods = &ehci_root_intr_methods;
			break;
		default:
			DPRINTF(("ehci_open: bad bEndpointAddress 0x%02x\n",
			    ed->bEndpointAddress));
			return (USBD_INVAL);
		}
		return (USBD_NORMAL_COMPLETION);
	}

	/* XXX All this stuff is only valid for async. */
	switch (dev->speed) {
	case USB_SPEED_LOW:  speed = EHCI_QH_SPEED_LOW;  break;
	case USB_SPEED_FULL: speed = EHCI_QH_SPEED_FULL; break;
	case USB_SPEED_HIGH: speed = EHCI_QH_SPEED_HIGH; break;
	default: panic("ehci_open: bad device speed %d", dev->speed);
	}
	if (speed != EHCI_QH_SPEED_HIGH && xfertype == UE_ISOCHRONOUS) {
		aprint_error_dev(sc->sc_dev, "error opening low/full speed "
		    "isoc endpoint.\n");
		aprint_normal_dev(sc->sc_dev, "a low/full speed device is "
		    "attached to a USB2 hub, and transaction translations are "
		    "not yet supported.\n");
		aprint_normal_dev(sc->sc_dev, "reattach the device to the "
		    "root hub instead.\n");
		DPRINTFN(1,("ehci_open: hshubaddr=%d hshubport=%d\n",
			    hshubaddr, hshubport));
		return USBD_INVAL;
	}

	/*
	 * For interrupt transfer, nak throttling must be disabled, but for
	 * the other transfer type, nak throttling should be enabled from the
	 * viewpoint that avoids the memory thrashing.
	 */
	naks = (xfertype == UE_INTERRUPT) ? 0
	    : ((speed == EHCI_QH_SPEED_HIGH) ? 4 : 0);

	/* Allocate sqh for everything, save isoc xfers */
	if (xfertype != UE_ISOCHRONOUS) {
		sqh = ehci_alloc_sqh(sc);
		if (sqh == NULL)
			return (USBD_NOMEM);
		/* qh_link filled when the QH is added */
		sqh->qh.qh_endp = htole32(
		    EHCI_QH_SET_ADDR(addr) |
		    EHCI_QH_SET_ENDPT(UE_GET_ADDR(ed->bEndpointAddress)) |
		    EHCI_QH_SET_EPS(speed) |
		    EHCI_QH_DTC |
		    EHCI_QH_SET_MPL(UGETW(ed->wMaxPacketSize)) |
		    (speed != EHCI_QH_SPEED_HIGH && xfertype == UE_CONTROL ?
		     EHCI_QH_CTL : 0) |
		    EHCI_QH_SET_NRL(naks)
		    );
		sqh->qh.qh_endphub = htole32(
		    EHCI_QH_SET_MULT(1) |
		    EHCI_QH_SET_SMASK(xfertype == UE_INTERRUPT ? 0x02 : 0)
		    );
		if (speed != EHCI_QH_SPEED_HIGH)
			sqh->qh.qh_endphub |= htole32(
			    EHCI_QH_SET_PORT(hshubport) |
			    EHCI_QH_SET_HUBA(hshubaddr) |
			    EHCI_QH_SET_CMASK(0x08) /* XXX */
			);
		sqh->qh.qh_curqtd = EHCI_NULL;
		/* Fill the overlay qTD */
		sqh->qh.qh_qtd.qtd_next = EHCI_NULL;
		sqh->qh.qh_qtd.qtd_altnext = EHCI_NULL;
		sqh->qh.qh_qtd.qtd_status = htole32(0);

		usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		epipe->sqh = sqh;
	} else {
		sqh = NULL;
	} /*xfertype == UE_ISOC*/

	switch (xfertype) {
	case UE_CONTROL:
		err = usb_allocmem(&sc->sc_bus, sizeof(usb_device_request_t),
				   0, &epipe->u.ctl.reqdma);
#ifdef EHCI_DEBUG
		if (err)
			printf("ehci_open: usb_allocmem()=%d\n", err);
#endif
		if (err)
			goto bad;
		pipe->methods = &ehci_device_ctrl_methods;
		mutex_enter(&sc->sc_lock);
		ehci_add_qh(sc, sqh, sc->sc_async_head);
		mutex_exit(&sc->sc_lock);
		break;
	case UE_BULK:
		pipe->methods = &ehci_device_bulk_methods;
		mutex_enter(&sc->sc_lock);
		ehci_add_qh(sc, sqh, sc->sc_async_head);
		mutex_exit(&sc->sc_lock);
		break;
	case UE_INTERRUPT:
		pipe->methods = &ehci_device_intr_methods;
		ival = pipe->interval;
		if (ival == USBD_DEFAULT_INTERVAL) {
			if (speed == EHCI_QH_SPEED_HIGH) {
				if (ed->bInterval > 16) {
					/*
					 * illegal with high-speed, but there
					 * were documentation bugs in the spec,
					 * so be generous
					 */
					ival = 256;
				} else
					ival = (1 << (ed->bInterval - 1)) / 8;
			} else
				ival = ed->bInterval;
		}
		err = ehci_device_setintr(sc, sqh, ival);
		if (err)
			goto bad;
		break;
	case UE_ISOCHRONOUS:
		pipe->methods = &ehci_device_isoc_methods;
		if (ed->bInterval == 0 || ed->bInterval > 16) {
			printf("ehci: opening pipe with invalid bInterval\n");
			err = USBD_INVAL;
			goto bad;
		}
		if (UGETW(ed->wMaxPacketSize) == 0) {
			printf("ehci: zero length endpoint open request\n");
			err = USBD_INVAL;
			goto bad;
		}
		epipe->u.isoc.next_frame = 0;
		epipe->u.isoc.cur_xfers = 0;
		break;
	default:
		DPRINTF(("ehci: bad xfer type %d\n", xfertype));
		err = USBD_INVAL;
		goto bad;
	}
	return (USBD_NORMAL_COMPLETION);

 bad:
	if (sqh != NULL)
		ehci_free_sqh(sc, sqh);
	return (err);
}

/*
 * Add an ED to the schedule.  Called with USB lock held.
 */
Static void
ehci_add_qh(ehci_softc_t *sc, ehci_soft_qh_t *sqh, ehci_soft_qh_t *head)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	usb_syncmem(&head->dma, head->offs + offsetof(ehci_qh_t, qh_link),
	    sizeof(head->qh.qh_link), BUS_DMASYNC_POSTWRITE);
	sqh->next = head->next;
	sqh->qh.qh_link = head->qh.qh_link;
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(ehci_qh_t, qh_link),
	    sizeof(sqh->qh.qh_link), BUS_DMASYNC_PREWRITE);
	head->next = sqh;
	head->qh.qh_link = htole32(sqh->physaddr | EHCI_LINK_QH);
	usb_syncmem(&head->dma, head->offs + offsetof(ehci_qh_t, qh_link),
	    sizeof(head->qh.qh_link), BUS_DMASYNC_PREWRITE);

#ifdef EHCI_DEBUG
	if (ehcidebug > 5) {
		printf("ehci_add_qh:\n");
		ehci_dump_sqh(sqh);
	}
#endif
}

/*
 * Remove an ED from the schedule.  Called with USB lock held.
 */
Static void
ehci_rem_qh(ehci_softc_t *sc, ehci_soft_qh_t *sqh, ehci_soft_qh_t *head)
{
	ehci_soft_qh_t *p;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* XXX */
	for (p = head; p != NULL && p->next != sqh; p = p->next)
		;
	if (p == NULL)
		panic("ehci_rem_qh: ED not found");
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(ehci_qh_t, qh_link),
	    sizeof(sqh->qh.qh_link), BUS_DMASYNC_POSTWRITE);
	p->next = sqh->next;
	p->qh.qh_link = sqh->qh.qh_link;
	usb_syncmem(&p->dma, p->offs + offsetof(ehci_qh_t, qh_link),
	    sizeof(p->qh.qh_link), BUS_DMASYNC_PREWRITE);

	ehci_sync_hc(sc);
}

Static void
ehci_set_qh_qtd(ehci_soft_qh_t *sqh, ehci_soft_qtd_t *sqtd)
{
	int i;
	u_int32_t status;

	/* Save toggle bit and ping status. */
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	status = sqh->qh.qh_qtd.qtd_status &
	    htole32(EHCI_QTD_TOGGLE_MASK |
		    EHCI_QTD_SET_STATUS(EHCI_QTD_PINGSTATE));
	/* Set HALTED to make hw leave it alone. */
	sqh->qh.qh_qtd.qtd_status =
	    htole32(EHCI_QTD_SET_STATUS(EHCI_QTD_HALTED));
	usb_syncmem(&sqh->dma,
	    sqh->offs + offsetof(ehci_qh_t, qh_qtd.qtd_status),
	    sizeof(sqh->qh.qh_qtd.qtd_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	sqh->qh.qh_curqtd = 0;
	sqh->qh.qh_qtd.qtd_next = htole32(sqtd->physaddr);
	sqh->qh.qh_qtd.qtd_altnext = EHCI_NULL;
	for (i = 0; i < EHCI_QTD_NBUFFERS; i++)
		sqh->qh.qh_qtd.qtd_buffer[i] = 0;
	sqh->sqtd = sqtd;
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	/* Set !HALTED && !ACTIVE to start execution, preserve some fields */
	sqh->qh.qh_qtd.qtd_status = status;
	usb_syncmem(&sqh->dma,
	    sqh->offs + offsetof(ehci_qh_t, qh_qtd.qtd_status),
	    sizeof(sqh->qh.qh_qtd.qtd_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
}

/*
 * Ensure that the HC has released all references to the QH.  We do this
 * by asking for a Async Advance Doorbell interrupt and then we wait for
 * the interrupt.
 * To make this easier we first obtain exclusive use of the doorbell.
 */
Static void
ehci_sync_hc(ehci_softc_t *sc)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_dying) {
		DPRINTFN(2,("ehci_sync_hc: dying\n"));
		return;
	}
	DPRINTFN(2,("ehci_sync_hc: enter\n"));
	/* ask for doorbell */
	EOWRITE4(sc, EHCI_USBCMD, EOREAD4(sc, EHCI_USBCMD) | EHCI_CMD_IAAD);
	DPRINTFN(1,("ehci_sync_hc: cmd=0x%08x sts=0x%08x\n",
		    EOREAD4(sc, EHCI_USBCMD), EOREAD4(sc, EHCI_USBSTS)));
	error = cv_timedwait(&sc->sc_doorbell, &sc->sc_lock, hz); /* bell wait */
	DPRINTFN(1,("ehci_sync_hc: cmd=0x%08x sts=0x%08x\n",
		    EOREAD4(sc, EHCI_USBCMD), EOREAD4(sc, EHCI_USBSTS)));
#ifdef DIAGNOSTIC
	if (error)
		printf("ehci_sync_hc: cv_timedwait() = %d\n", error);
#endif
	DPRINTFN(2,("ehci_sync_hc: exit\n"));
}

Static void
ehci_rem_free_itd_chain(ehci_softc_t *sc, struct ehci_xfer *exfer)
{
	struct ehci_soft_itd *itd, *prev;

	prev = NULL;

	if (exfer->itdstart == NULL || exfer->itdend == NULL)
		panic("ehci isoc xfer being freed, but with no itd chain\n");

	for (itd = exfer->itdstart; itd != NULL; itd = itd->xfer_next) {
		prev = itd->u.frame_list.prev;
		/* Unlink itd from hardware chain, or frame array */
		if (prev == NULL) { /* We're at the table head */
			sc->sc_softitds[itd->slot] = itd->u.frame_list.next;
			sc->sc_flist[itd->slot] = itd->itd.itd_next;
			usb_syncmem(&sc->sc_fldma,
			    sizeof(ehci_link_t) * itd->slot,
                	    sizeof(ehci_link_t),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			if (itd->u.frame_list.next != NULL)
				itd->u.frame_list.next->u.frame_list.prev = NULL;
		} else {
			/* XXX this part is untested... */
			prev->itd.itd_next = itd->itd.itd_next;
			usb_syncmem(&itd->dma,
			    itd->offs + offsetof(ehci_itd_t, itd_next),
                	    sizeof(itd->itd.itd_next), BUS_DMASYNC_PREWRITE);

			prev->u.frame_list.next = itd->u.frame_list.next;
			if (itd->u.frame_list.next != NULL)
				itd->u.frame_list.next->u.frame_list.prev = prev;
		}
	}

	prev = NULL;
	for (itd = exfer->itdstart; itd != NULL; itd = itd->xfer_next) {
		if (prev != NULL)
			ehci_free_itd(sc, prev);
		prev = itd;
	}
	if (prev)
		ehci_free_itd(sc, prev);
	exfer->itdstart = NULL;
	exfer->itdend = NULL;
}

/***********/

/*
 * Data structures and routines to emulate the root hub.
 */
Static usb_device_descriptor_t ehci_devd = {
	USB_DEVICE_DESCRIPTOR_SIZE,
	UDESC_DEVICE,		/* type */
	{0x00, 0x02},		/* USB version */
	UDCLASS_HUB,		/* class */
	UDSUBCLASS_HUB,		/* subclass */
	UDPROTO_HSHUBSTT,	/* protocol */
	64,			/* max packet */
	{0},{0},{0x00,0x01},	/* device id */
	1,2,0,			/* string indicies */
	1			/* # of configurations */
};

Static const usb_device_qualifier_t ehci_odevd = {
	USB_DEVICE_DESCRIPTOR_SIZE,
	UDESC_DEVICE_QUALIFIER,	/* type */
	{0x00, 0x02},		/* USB version */
	UDCLASS_HUB,		/* class */
	UDSUBCLASS_HUB,		/* subclass */
	UDPROTO_FSHUB,		/* protocol */
	64,			/* max packet */
	1,			/* # of configurations */
	0
};

Static const usb_config_descriptor_t ehci_confd = {
	USB_CONFIG_DESCRIPTOR_SIZE,
	UDESC_CONFIG,
	{USB_CONFIG_DESCRIPTOR_SIZE +
	 USB_INTERFACE_DESCRIPTOR_SIZE +
	 USB_ENDPOINT_DESCRIPTOR_SIZE},
	1,
	1,
	0,
	UC_ATTR_MBO | UC_SELF_POWERED,
	0			/* max power */
};

Static const usb_interface_descriptor_t ehci_ifcd = {
	USB_INTERFACE_DESCRIPTOR_SIZE,
	UDESC_INTERFACE,
	0,
	0,
	1,
	UICLASS_HUB,
	UISUBCLASS_HUB,
	UIPROTO_HSHUBSTT,
	0
};

Static const usb_endpoint_descriptor_t ehci_endpd = {
	USB_ENDPOINT_DESCRIPTOR_SIZE,
	UDESC_ENDPOINT,
	UE_DIR_IN | EHCI_INTR_ENDPT,
	UE_INTERRUPT,
	{8, 0},			/* max packet */
	12
};

Static const usb_hub_descriptor_t ehci_hubd = {
	USB_HUB_DESCRIPTOR_SIZE,
	UDESC_HUB,
	0,
	{0,0},
	0,
	0,
	{""},
	{""},
};

/*
 * Simulate a hardware hub by handling all the necessary requests.
 */
Static usbd_status
ehci_root_ctrl_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ehci_root_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ehci_root_ctrl_start(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usb_device_request_t *req;
	void *buf = NULL;
	int port, i;
	int len, value, index, l, totlen = 0;
	usb_port_status_t ps;
	usb_hub_descriptor_t hubd;
	usbd_status err;
	u_int32_t v;

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST))
		/* XXX panic */
		return (USBD_INVAL);
#endif
	req = &xfer->request;

	DPRINTFN(4,("ehci_root_ctrl_start: type=0x%02x request=%02x\n",
		    req->bmRequestType, req->bRequest));

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	if (len != 0)
		buf = KERNADDR(&xfer->dmabuf, 0);

#define C(x,y) ((x) | ((y) << 8))
	switch(C(req->bRequest, req->bmRequestType)) {
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
			*(u_int8_t *)buf = sc->sc_conf;
			totlen = 1;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(8,("ehci_root_ctrl_start: wValue=0x%04x\n", value));
		if (len == 0)
			break;
		switch(value >> 8) {
		case UDESC_DEVICE:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			USETW(ehci_devd.idVendor, sc->sc_id_vendor);
			memcpy(buf, &ehci_devd, l);
			break;
		/*
		 * We can't really operate at another speed, but the spec says
		 * we need this descriptor.
		 */
		case UDESC_DEVICE_QUALIFIER:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			memcpy(buf, &ehci_odevd, l);
			break;
		/*
		 * We can't really operate at another speed, but the spec says
		 * we need this descriptor.
		 */
		case UDESC_OTHER_SPEED_CONFIGURATION:
		case UDESC_CONFIG:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_CONFIG_DESCRIPTOR_SIZE);
			memcpy(buf, &ehci_confd, l);
			((usb_config_descriptor_t *)buf)->bDescriptorType =
				value >> 8;
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_INTERFACE_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &ehci_ifcd, l);
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_ENDPOINT_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &ehci_endpd, l);
			break;
		case UDESC_STRING:
#define sd ((usb_string_descriptor_t *)buf)
			switch (value & 0xff) {
			case 0: /* Language table */
				totlen = usb_makelangtbl(sd, len);
				break;
			case 1: /* Vendor */
				totlen = usb_makestrdesc(sd, len,
							 sc->sc_vendor);
				break;
			case 2: /* Product */
				totlen = usb_makestrdesc(sd, len,
							 "EHCI root hub");
				break;
			}
#undef sd
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
		break;
	case C(UR_GET_INTERFACE, UT_READ_INTERFACE):
		if (len > 0) {
			*(u_int8_t *)buf = 0;
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
	case C(UR_SET_DESCRIPTOR, UT_WRITE_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_INTERFACE):
	case C(UR_SET_FEATURE, UT_WRITE_ENDPOINT):
		err = USBD_IOERROR;
		goto ret;
	case C(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
		break;
	case C(UR_SYNCH_FRAME, UT_WRITE_ENDPOINT):
		break;
	/* Hub requests */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		DPRINTFN(4, ("ehci_root_ctrl_start: UR_CLEAR_PORT_FEATURE "
			     "port=%d feature=%d\n",
			     index, value));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		port = EHCI_PORTSC(index);
		v = EOREAD4(sc, port);
		DPRINTFN(4, ("ehci_root_ctrl_start: portsc=0x%08x\n", v));
		v &= ~EHCI_PS_CLEAR;
		switch(value) {
		case UHF_PORT_ENABLE:
			EOWRITE4(sc, port, v &~ EHCI_PS_PE);
			break;
		case UHF_PORT_SUSPEND:
			if (!(v & EHCI_PS_SUSP)) /* not suspended */
				break;
			v &= ~EHCI_PS_SUSP;
			EOWRITE4(sc, port, v | EHCI_PS_FPR);
			/* see USB2 spec ch. 7.1.7.7 */
			usb_delay_ms(&sc->sc_bus, 20);
			EOWRITE4(sc, port, v);
			usb_delay_ms(&sc->sc_bus, 2);
#ifdef DEBUG
			v = EOREAD4(sc, port);
			if (v & (EHCI_PS_FPR | EHCI_PS_SUSP))
				printf("ehci: resume failed: %x\n", v);
#endif
			break;
		case UHF_PORT_POWER:
			if (sc->sc_hasppc)
				EOWRITE4(sc, port, v &~ EHCI_PS_PP);
			break;
		case UHF_PORT_TEST:
			DPRINTFN(2,("ehci_root_ctrl_start: clear port test "
				    "%d\n", index));
			break;
		case UHF_PORT_INDICATOR:
			DPRINTFN(2,("ehci_root_ctrl_start: clear port ind "
				    "%d\n", index));
			EOWRITE4(sc, port, v &~ EHCI_PS_PIC);
			break;
		case UHF_C_PORT_CONNECTION:
			EOWRITE4(sc, port, v | EHCI_PS_CSC);
			break;
		case UHF_C_PORT_ENABLE:
			EOWRITE4(sc, port, v | EHCI_PS_PEC);
			break;
		case UHF_C_PORT_SUSPEND:
			/* how? */
			break;
		case UHF_C_PORT_OVER_CURRENT:
			EOWRITE4(sc, port, v | EHCI_PS_OCC);
			break;
		case UHF_C_PORT_RESET:
			sc->sc_isreset[index] = 0;
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
#if 0
		switch(value) {
		case UHF_C_PORT_CONNECTION:
		case UHF_C_PORT_ENABLE:
		case UHF_C_PORT_SUSPEND:
		case UHF_C_PORT_OVER_CURRENT:
		case UHF_C_PORT_RESET:
		default:
			break;
		}
#endif
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0) {
			err = USBD_IOERROR;
			goto ret;
		}
		hubd = ehci_hubd;
		hubd.bNbrPorts = sc->sc_noport;
		v = EOREAD4(sc, EHCI_HCSPARAMS);
		USETW(hubd.wHubCharacteristics,
		    EHCI_HCS_PPC(v) ? UHD_PWR_INDIVIDUAL : UHD_PWR_NO_SWITCH |
		    EHCI_HCS_P_INDICATOR(EREAD4(sc, EHCI_HCSPARAMS))
			? UHD_PORT_IND : 0);
		hubd.bPwrOn2PwrGood = 200; /* XXX can't find out? */
		for (i = 0, l = sc->sc_noport; l > 0; i++, l -= 8, v >>= 8)
			hubd.DeviceRemovable[i++] = 0; /* XXX can't find out? */
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + i;
		l = min(len, hubd.bDescLength);
		totlen = l;
		memcpy(buf, &hubd, l);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			err = USBD_IOERROR;
			goto ret;
		}
		memset(buf, 0, len); /* ? XXX */
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		DPRINTFN(8,("ehci_root_ctrl_start: get port status i=%d\n",
			    index));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		if (len != 4) {
			err = USBD_IOERROR;
			goto ret;
		}
		v = EOREAD4(sc, EHCI_PORTSC(index));
		DPRINTFN(8,("ehci_root_ctrl_start: port status=0x%04x\n", v));

		i = UPS_HIGH_SPEED;
		if (sc->sc_flags & EHCIF_ETTF) {
			/*
			 * If we are doing embedded transaction translation,
			 * then directly attached LS/FS devices are reset by
			 * the EHCI controller itself.  PSPD is encoded
			 * the same way as in USBSTATUS.
			 */
			i = __SHIFTOUT(v, EHCI_PS_PSPD) * UPS_LOW_SPEED;
		}
		if (v & EHCI_PS_CS)	i |= UPS_CURRENT_CONNECT_STATUS;
		if (v & EHCI_PS_PE)	i |= UPS_PORT_ENABLED;
		if (v & EHCI_PS_SUSP)	i |= UPS_SUSPEND;
		if (v & EHCI_PS_OCA)	i |= UPS_OVERCURRENT_INDICATOR;
		if (v & EHCI_PS_PR)	i |= UPS_RESET;
		if (v & EHCI_PS_PP)	i |= UPS_PORT_POWER;
		if (sc->sc_vendor_port_status)
			i = sc->sc_vendor_port_status(sc, v, i);
		USETW(ps.wPortStatus, i);
		i = 0;
		if (v & EHCI_PS_CSC)	i |= UPS_C_CONNECT_STATUS;
		if (v & EHCI_PS_PEC)	i |= UPS_C_PORT_ENABLED;
		if (v & EHCI_PS_OCC)	i |= UPS_C_OVERCURRENT_INDICATOR;
		if (sc->sc_isreset[index]) i |= UPS_C_PORT_RESET;
		USETW(ps.wPortChange, i);
		l = min(len, sizeof ps);
		memcpy(buf, &ps, l);
		totlen = l;
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		err = USBD_IOERROR;
		goto ret;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		port = EHCI_PORTSC(index);
		v = EOREAD4(sc, port);
		DPRINTFN(4, ("ehci_root_ctrl_start: portsc=0x%08x\n", v));
		v &= ~EHCI_PS_CLEAR;
		switch(value) {
		case UHF_PORT_ENABLE:
			EOWRITE4(sc, port, v | EHCI_PS_PE);
			break;
		case UHF_PORT_SUSPEND:
			EOWRITE4(sc, port, v | EHCI_PS_SUSP);
			break;
		case UHF_PORT_RESET:
			DPRINTFN(5,("ehci_root_ctrl_start: reset port %d\n",
				    index));
			if (EHCI_PS_IS_LOWSPEED(v)
			    && sc->sc_ncomp > 0
			    && !(sc->sc_flags & EHCIF_ETTF)) {
				/*
				 * Low speed device on non-ETTF controller or
				 * unaccompanied controller, give up ownership.
				 */
				ehci_disown(sc, index, 1);
				break;
			}
			/* Start reset sequence. */
			v &= ~ (EHCI_PS_PE | EHCI_PS_PR);
			EOWRITE4(sc, port, v | EHCI_PS_PR);
			/* Wait for reset to complete. */
			usb_delay_ms(&sc->sc_bus, USB_PORT_ROOT_RESET_DELAY);
			if (sc->sc_dying) {
				err = USBD_IOERROR;
				goto ret;
			}
			/*
			 * An embedded transaction translater will automatically
			 * terminate the reset sequence so there's no need to
			 * it.
			 */
			v = EOREAD4(sc, port);
			if (v & EHCI_PS_PR) {
				/* Terminate reset sequence. */
				EOWRITE4(sc, port, v & ~EHCI_PS_PR);
				/* Wait for HC to complete reset. */
				usb_delay_ms(&sc->sc_bus,
				    EHCI_PORT_RESET_COMPLETE);
				if (sc->sc_dying) {
					err = USBD_IOERROR;
					goto ret;
				}
			}

			v = EOREAD4(sc, port);
			DPRINTF(("ehci after reset, status=0x%08x\n", v));
			if (v & EHCI_PS_PR) {
				printf("%s: port reset timeout\n",
				       device_xname(sc->sc_dev));
				return (USBD_TIMEOUT);
			}
			if (!(v & EHCI_PS_PE)) {
				/* Not a high speed device, give up ownership.*/
				ehci_disown(sc, index, 0);
				break;
			}
			sc->sc_isreset[index] = 1;
			DPRINTF(("ehci port %d reset, status = 0x%08x\n",
				 index, v));
			break;
		case UHF_PORT_POWER:
			DPRINTFN(2,("ehci_root_ctrl_start: set port power "
				    "%d (has PPC = %d)\n", index,
				    sc->sc_hasppc));
			if (sc->sc_hasppc)
				EOWRITE4(sc, port, v | EHCI_PS_PP);
			break;
		case UHF_PORT_TEST:
			DPRINTFN(2,("ehci_root_ctrl_start: set port test "
				    "%d\n", index));
			break;
		case UHF_PORT_INDICATOR:
			DPRINTFN(2,("ehci_root_ctrl_start: set port ind "
				    "%d\n", index));
			EOWRITE4(sc, port, v | EHCI_PS_PIC);
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
		break;
	case C(UR_CLEAR_TT_BUFFER, UT_WRITE_CLASS_OTHER):
	case C(UR_RESET_TT, UT_WRITE_CLASS_OTHER):
	case C(UR_GET_TT_STATE, UT_READ_CLASS_OTHER):
	case C(UR_STOP_TT, UT_WRITE_CLASS_OTHER):
		break;
	default:
		err = USBD_IOERROR;
		goto ret;
	}
	xfer->actlen = totlen;
	err = USBD_NORMAL_COMPLETION;
 ret:
	mutex_enter(&sc->sc_lock);
	xfer->status = err;
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
	return (USBD_IN_PROGRESS);
}

Static void
ehci_disown(ehci_softc_t *sc, int index, int lowspeed)
{
	int port;
	u_int32_t v;

	DPRINTF(("ehci_disown: index=%d lowspeed=%d\n", index, lowspeed));
#ifdef DIAGNOSTIC
	if (sc->sc_npcomp != 0) {
		int i = (index-1) / sc->sc_npcomp;
		if (i >= sc->sc_ncomp)
			printf("%s: strange port\n",
			       device_xname(sc->sc_dev));
		else
			printf("%s: handing over %s speed device on "
			       "port %d to %s\n",
			       device_xname(sc->sc_dev),
			       lowspeed ? "low" : "full",
			       index, device_xname(sc->sc_comps[i]));
	} else {
		printf("%s: npcomp == 0\n", device_xname(sc->sc_dev));
	}
#endif
	port = EHCI_PORTSC(index);
	v = EOREAD4(sc, port) &~ EHCI_PS_CLEAR;
	EOWRITE4(sc, port, v | EHCI_PS_PO);
}

/* Abort a root control request. */
Static void
ehci_root_ctrl_abort(usbd_xfer_handle xfer)
{
	/* Nothing to do, all transfers are synchronous. */
}

/* Close the root pipe. */
Static void
ehci_root_ctrl_close(usbd_pipe_handle pipe)
{
	DPRINTF(("ehci_root_ctrl_close\n"));
	/* Nothing to do. */
}

Static void
ehci_root_intr_done(usbd_xfer_handle xfer)
{
	xfer->hcpriv = NULL;
}

Static usbd_status
ehci_root_intr_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ehci_root_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ehci_root_intr_start(usbd_xfer_handle xfer)
{
	usbd_pipe_handle pipe = xfer->pipe;
	ehci_softc_t *sc = pipe->device->bus->hci_private;

	if (sc->sc_dying)
		return (USBD_IOERROR);

	mutex_enter(&sc->sc_lock);
	sc->sc_intrxfer = xfer;
	mutex_exit(&sc->sc_lock);

	return (USBD_IN_PROGRESS);
}

/* Abort a root interrupt request. */
Static void
ehci_root_intr_abort(usbd_xfer_handle xfer)
{
#ifdef DIAGNOSTIC
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
#endif

	KASSERT(mutex_owned(&sc->sc_lock));
	if (xfer->pipe->intrxfer == xfer) {
		DPRINTF(("ehci_root_intr_abort: remove\n"));
		xfer->pipe->intrxfer = NULL;
	}
	xfer->status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

/* Close the root pipe. */
Static void
ehci_root_intr_close(usbd_pipe_handle pipe)
{
	ehci_softc_t *sc = pipe->device->bus->hci_private;

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF(("ehci_root_intr_close\n"));

	sc->sc_intrxfer = NULL;
}

Static void
ehci_root_ctrl_done(usbd_xfer_handle xfer)
{
	xfer->hcpriv = NULL;
}

/************************/

Static ehci_soft_qh_t *
ehci_alloc_sqh(ehci_softc_t *sc)
{
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;

	if (sc->sc_freeqhs == NULL) {
		DPRINTFN(2, ("ehci_alloc_sqh: allocating chunk\n"));
		err = usb_allocmem(&sc->sc_bus, EHCI_SQH_SIZE * EHCI_SQH_CHUNK,
			  EHCI_PAGE_SIZE, &dma);
#ifdef EHCI_DEBUG
		if (err)
			printf("ehci_alloc_sqh: usb_allocmem()=%d\n", err);
#endif
		if (err)
			return (NULL);
		for(i = 0; i < EHCI_SQH_CHUNK; i++) {
			offs = i * EHCI_SQH_SIZE;
			sqh = KERNADDR(&dma, offs);
			sqh->physaddr = DMAADDR(&dma, offs);
			sqh->dma = dma;
			sqh->offs = offs;
			sqh->next = sc->sc_freeqhs;
			sc->sc_freeqhs = sqh;
		}
	}
	sqh = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh->next;
	memset(&sqh->qh, 0, sizeof(ehci_qh_t));
	sqh->next = NULL;
	return (sqh);
}

Static void
ehci_free_sqh(ehci_softc_t *sc, ehci_soft_qh_t *sqh)
{
	sqh->next = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh;
}

Static ehci_soft_qtd_t *
ehci_alloc_sqtd(ehci_softc_t *sc)
{
	ehci_soft_qtd_t *sqtd = NULL;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;

	if (sc->sc_freeqtds == NULL) {
		DPRINTFN(2, ("ehci_alloc_sqtd: allocating chunk\n"));

		err = usb_allocmem(&sc->sc_bus, EHCI_SQTD_SIZE*EHCI_SQTD_CHUNK,
			  EHCI_PAGE_SIZE, &dma);
#ifdef EHCI_DEBUG
		if (err)
			printf("ehci_alloc_sqtd: usb_allocmem()=%d\n", err);
#endif
		if (err)
			goto done;

		for(i = 0; i < EHCI_SQTD_CHUNK; i++) {
			offs = i * EHCI_SQTD_SIZE;
			sqtd = KERNADDR(&dma, offs);
			sqtd->physaddr = DMAADDR(&dma, offs);
			sqtd->dma = dma;
			sqtd->offs = offs;

			sqtd->nextqtd = sc->sc_freeqtds;
			sc->sc_freeqtds = sqtd;
		}
	}

	sqtd = sc->sc_freeqtds;
	sc->sc_freeqtds = sqtd->nextqtd;
	memset(&sqtd->qtd, 0, sizeof(ehci_qtd_t));
	sqtd->nextqtd = NULL;
	sqtd->xfer = NULL;

done:
	return (sqtd);
}

Static void
ehci_free_sqtd(ehci_softc_t *sc, ehci_soft_qtd_t *sqtd)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	sqtd->nextqtd = sc->sc_freeqtds;
	sc->sc_freeqtds = sqtd;
}

Static usbd_status
ehci_alloc_sqtd_chain(struct ehci_pipe *epipe, ehci_softc_t *sc,
		     int alen, int rd, usbd_xfer_handle xfer,
		     ehci_soft_qtd_t **sp, ehci_soft_qtd_t **ep)
{
	ehci_soft_qtd_t *next, *cur;
	ehci_physaddr_t dataphys, dataphyspage, dataphyslastpage, nextphys;
	u_int32_t qtdstatus;
	int len, curlen, mps;
	int i, tog;
	usb_dma_t *dma = &xfer->dmabuf;
	u_int16_t flags = xfer->flags;

	DPRINTFN(alen<4*4096,("ehci_alloc_sqtd_chain: start len=%d\n", alen));

	len = alen;
	dataphys = DMAADDR(dma, 0);
	dataphyslastpage = EHCI_PAGE(dataphys + len - 1);
	qtdstatus = EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_PID(rd ? EHCI_QTD_PID_IN : EHCI_QTD_PID_OUT) |
	    EHCI_QTD_SET_CERR(3)
	    /* IOC set below */
	    /* BYTES set below */
	    ;
	mps = UGETW(epipe->pipe.endpoint->edesc->wMaxPacketSize);
	tog = epipe->nexttoggle;
	qtdstatus |= EHCI_QTD_SET_TOGGLE(tog);

	cur = ehci_alloc_sqtd(sc);
	*sp = cur;
	if (cur == NULL)
		goto nomem;

	usb_syncmem(dma, 0, alen,
	    rd ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	for (;;) {
		dataphyspage = EHCI_PAGE(dataphys);
		/* The EHCI hardware can handle at most 5 pages. */
		if (dataphyslastpage - dataphyspage <
		    EHCI_QTD_NBUFFERS * EHCI_PAGE_SIZE) {
			/* we can handle it in this QTD */
			curlen = len;
		} else {
			/* must use multiple TDs, fill as much as possible. */
			curlen = EHCI_QTD_NBUFFERS * EHCI_PAGE_SIZE -
				 EHCI_PAGE_OFFSET(dataphys);
#ifdef DIAGNOSTIC
			if (curlen > len) {
				printf("ehci_alloc_sqtd_chain: curlen=0x%x "
				       "len=0x%x offs=0x%x\n", curlen, len,
				       EHCI_PAGE_OFFSET(dataphys));
				printf("lastpage=0x%x page=0x%x phys=0x%x\n",
				       dataphyslastpage, dataphyspage,
				       dataphys);
				curlen = len;
			}
#endif
			/* the length must be a multiple of the max size */
			curlen -= curlen % mps;
			DPRINTFN(1,("ehci_alloc_sqtd_chain: multiple QTDs, "
				    "curlen=%d\n", curlen));
#ifdef DIAGNOSTIC
			if (curlen == 0)
				panic("ehci_alloc_sqtd_chain: curlen == 0");
#endif
		}
		DPRINTFN(4,("ehci_alloc_sqtd_chain: dataphys=0x%08x "
			    "dataphyslastpage=0x%08x len=%d curlen=%d\n",
			    dataphys, dataphyslastpage,
			    len, curlen));
		len -= curlen;

		/*
		 * Allocate another transfer if there's more data left,
		 * or if force last short transfer flag is set and we're
		 * allocating a multiple of the max packet size.
		 */
		if (len != 0 ||
		    ((curlen % mps) == 0 && !rd && curlen != 0 &&
		     (flags & USBD_FORCE_SHORT_XFER))) {
			next = ehci_alloc_sqtd(sc);
			if (next == NULL)
				goto nomem;
			nextphys = htole32(next->physaddr);
		} else {
			next = NULL;
			nextphys = EHCI_NULL;
		}

		for (i = 0; i * EHCI_PAGE_SIZE <
		            curlen + EHCI_PAGE_OFFSET(dataphys); i++) {
			ehci_physaddr_t a = dataphys + i * EHCI_PAGE_SIZE;
			if (i != 0) /* use offset only in first buffer */
				a = EHCI_PAGE(a);
			if (i >= EHCI_QTD_NBUFFERS) {
#ifdef DIAGNOSTIC
				printf("ehci_alloc_sqtd_chain: i=%d\n", i);
#endif
				goto nomem;
			}
			cur->qtd.qtd_buffer[i] = htole32(a);
			cur->qtd.qtd_buffer_hi[i] = 0;
		}
		cur->nextqtd = next;
		cur->qtd.qtd_next = cur->qtd.qtd_altnext = nextphys;
		cur->qtd.qtd_status =
		    htole32(qtdstatus | EHCI_QTD_SET_BYTES(curlen));
		cur->xfer = xfer;
		cur->len = curlen;

		DPRINTFN(10,("ehci_alloc_sqtd_chain: cbp=0x%08x end=0x%08x\n",
			    dataphys, dataphys + curlen));
		/* adjust the toggle based on the number of packets in this
		   qtd */
		if (((curlen + mps - 1) / mps) & 1) {
			tog ^= 1;
			qtdstatus ^= EHCI_QTD_TOGGLE_MASK;
		}
		if (next == NULL)
			break;
		usb_syncmem(&cur->dma, cur->offs, sizeof(cur->qtd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		DPRINTFN(10,("ehci_alloc_sqtd_chain: extend chain\n"));
		if (len)
			dataphys += curlen;
		cur = next;
	}
	cur->qtd.qtd_status |= htole32(EHCI_QTD_IOC);
	usb_syncmem(&cur->dma, cur->offs, sizeof(cur->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	*ep = cur;
	epipe->nexttoggle = tog;

	DPRINTFN(10,("ehci_alloc_sqtd_chain: return sqtd=%p sqtdend=%p\n",
		     *sp, *ep));

	return (USBD_NORMAL_COMPLETION);

 nomem:
	/* XXX free chain */
	DPRINTFN(-1,("ehci_alloc_sqtd_chain: no memory\n"));
	return (USBD_NOMEM);
}

Static void
ehci_free_sqtd_chain(ehci_softc_t *sc, ehci_soft_qtd_t *sqtd,
		    ehci_soft_qtd_t *sqtdend)
{
	ehci_soft_qtd_t *p;
	int i;

	DPRINTFN(10,("ehci_free_sqtd_chain: sqtd=%p sqtdend=%p\n",
		     sqtd, sqtdend));

	for (i = 0; sqtd != sqtdend; sqtd = p, i++) {
		p = sqtd->nextqtd;
		ehci_free_sqtd(sc, sqtd);
	}
}

Static ehci_soft_itd_t *
ehci_alloc_itd(ehci_softc_t *sc)
{
	struct ehci_soft_itd *itd, *freeitd;
	usbd_status err;
	int i, offs, frindex, previndex;
	usb_dma_t dma;

	mutex_enter(&sc->sc_lock);

	/* Find an itd that wasn't freed this frame or last frame. This can
	 * discard itds that were freed before frindex wrapped around
	 * XXX - can this lead to thrashing? Could fix by enabling wrap-around
	 *       interrupt and fiddling with list when that happens */
	frindex = (EOREAD4(sc, EHCI_FRINDEX) + 1) >> 3;
	previndex = (frindex != 0) ? frindex - 1 : sc->sc_flsize;

	freeitd = NULL;
	LIST_FOREACH(itd, &sc->sc_freeitds, u.free_list) {
		if (itd == NULL)
			break;
		if (itd->slot != frindex && itd->slot != previndex) {
			freeitd = itd;
			break;
		}
	}

	if (freeitd == NULL) {
		DPRINTFN(2, ("ehci_alloc_itd allocating chunk\n"));
		err = usb_allocmem(&sc->sc_bus, EHCI_ITD_SIZE * EHCI_ITD_CHUNK,
				EHCI_PAGE_SIZE, &dma);

		if (err) {
			DPRINTF(("ehci_alloc_itd, alloc returned %d\n", err));
			mutex_exit(&sc->sc_lock);
			return NULL;
		}

		for (i = 0; i < EHCI_ITD_CHUNK; i++) {
			offs = i * EHCI_ITD_SIZE;
			itd = KERNADDR(&dma, offs);
			itd->physaddr = DMAADDR(&dma, offs);
	 		itd->dma = dma;
			itd->offs = offs;
			LIST_INSERT_HEAD(&sc->sc_freeitds, itd, u.free_list);
		}
		freeitd = LIST_FIRST(&sc->sc_freeitds);
	}

	itd = freeitd;
	LIST_REMOVE(itd, u.free_list);
	memset(&itd->itd, 0, sizeof(ehci_itd_t));
	usb_syncmem(&itd->dma, itd->offs + offsetof(ehci_itd_t, itd_next),
                    sizeof(itd->itd.itd_next), BUS_DMASYNC_PREWRITE |
                    BUS_DMASYNC_PREREAD);

	itd->u.frame_list.next = NULL;
	itd->u.frame_list.prev = NULL;
	itd->xfer_next = NULL;
	itd->slot = 0;

	mutex_exit(&sc->sc_lock);

	return itd;
}

Static void
ehci_free_itd(ehci_softc_t *sc, ehci_soft_itd_t *itd)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	LIST_INSERT_HEAD(&sc->sc_freeitds, itd, u.free_list);
}

/****************/

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
Static void
ehci_close_pipe(usbd_pipe_handle pipe, ehci_soft_qh_t *head)
{
	struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;
	ehci_softc_t *sc = pipe->device->bus->hci_private;
	ehci_soft_qh_t *sqh = epipe->sqh;

	KASSERT(mutex_owned(&sc->sc_lock));

	ehci_rem_qh(sc, sqh, head);
	ehci_free_sqh(sc, epipe->sqh);
}

/*
 * Abort a device request.
 * If this routine is called at splusb() it guarantees that the request
 * will be removed from the hardware scheduling and that the callback
 * for it will be called with USBD_CANCELLED status.
 * It's impossible to guarantee that the requested transfer will not
 * have happened since the hardware runs concurrently.
 * If the transaction has already happened we rely on the ordinary
 * interrupt processing to process it.
 * XXX This is most probably wrong.
 * XXXMRG this doesn't make sense anymore.
 */
Static void
ehci_abort_xfer(usbd_xfer_handle xfer, usbd_status status)
{
#define exfer EXFER(xfer)
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	ehci_softc_t *sc = epipe->pipe.device->bus->hci_private;
	ehci_soft_qh_t *sqh = epipe->sqh;
	ehci_soft_qtd_t *sqtd;
	ehci_physaddr_t cur;
	u_int32_t qhstatus;
	int hit;
	int wake;

	DPRINTF(("ehci_abort_xfer: xfer=%p pipe=%p\n", xfer, epipe));

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_dying) {
		/* If we're dying, just do the software part. */
		xfer->status = status;	/* make software ignore it */
		callout_stop(&xfer->timeout_handle);
		usb_transfer_complete(xfer);
		return;
	}

	if (cpu_intr_p() || cpu_softintr_p())
		panic("ehci_abort_xfer: not in process context");

	/*
	 * If an abort is already in progress then just wait for it to
	 * complete and return.
	 */
	if (xfer->hcflags & UXFER_ABORTING) {
		DPRINTFN(2, ("ehci_abort_xfer: already aborting\n"));
#ifdef DIAGNOSTIC
		if (status == USBD_TIMEOUT)
			printf("ehci_abort_xfer: TIMEOUT while aborting\n");
#endif
		/* Override the status which might be USBD_TIMEOUT. */
		xfer->status = status;
		DPRINTFN(2, ("ehci_abort_xfer: waiting for abort to finish\n"));
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

	usb_syncmem(&sqh->dma,
	    sqh->offs + offsetof(ehci_qh_t, qh_qtd.qtd_status),
	    sizeof(sqh->qh.qh_qtd.qtd_status),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	qhstatus = sqh->qh.qh_qtd.qtd_status;
	sqh->qh.qh_qtd.qtd_status = qhstatus | htole32(EHCI_QTD_HALTED);
	usb_syncmem(&sqh->dma,
	    sqh->offs + offsetof(ehci_qh_t, qh_qtd.qtd_status),
	    sizeof(sqh->qh.qh_qtd.qtd_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	for (sqtd = exfer->sqtdstart; ; sqtd = sqtd->nextqtd) {
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
		    sizeof(sqtd->qtd.qtd_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		sqtd->qtd.qtd_status |= htole32(EHCI_QTD_HALTED);
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
		    sizeof(sqtd->qtd.qtd_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		if (sqtd == exfer->sqtdend)
			break;
	}

	/*
	 * Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.  Also make sure the soft interrupt routine
	 * has run.
	 */
	ehci_sync_hc(sc);
	sc->sc_softwake = 1;
	usb_schedsoftintr(&sc->sc_bus);
	cv_wait(&sc->sc_softwake_cv, &sc->sc_lock);

	/*
	 * Step 3: Remove any vestiges of the xfer from the hardware.
	 * The complication here is that the hardware may have executed
	 * beyond the xfer we're trying to abort.  So as we're scanning
	 * the TDs of this xfer we check if the hardware points to
	 * any of them.
	 */

	usb_syncmem(&sqh->dma,
	    sqh->offs + offsetof(ehci_qh_t, qh_curqtd),
	    sizeof(sqh->qh.qh_curqtd),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	cur = EHCI_LINK_ADDR(le32toh(sqh->qh.qh_curqtd));
	hit = 0;
	for (sqtd = exfer->sqtdstart; ; sqtd = sqtd->nextqtd) {
		hit |= cur == sqtd->physaddr;
		if (sqtd == exfer->sqtdend)
			break;
	}
	sqtd = sqtd->nextqtd;
	/* Zap curqtd register if hardware pointed inside the xfer. */
	if (hit && sqtd != NULL) {
		DPRINTFN(1,("ehci_abort_xfer: cur=0x%08x\n", sqtd->physaddr));
		sqh->qh.qh_curqtd = htole32(sqtd->physaddr); /* unlink qTDs */
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(ehci_qh_t, qh_curqtd),
		    sizeof(sqh->qh.qh_curqtd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		sqh->qh.qh_qtd.qtd_status = qhstatus;
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(ehci_qh_t, qh_qtd.qtd_status),
		    sizeof(sqh->qh.qh_qtd.qtd_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	} else {
		DPRINTFN(1,("ehci_abort_xfer: no hit\n"));
	}

	/*
	 * Step 4: Execute callback.
	 */
#ifdef DIAGNOSTIC
	exfer->isdone = 1;
#endif
	wake = xfer->hcflags & UXFER_ABORTWAIT;
	xfer->hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);
	usb_transfer_complete(xfer);
	if (wake) {
		cv_broadcast(&xfer->hccv);
	}

	KASSERT(mutex_owned(&sc->sc_lock));
#undef exfer
}

Static void
ehci_abort_isoc_xfer(usbd_xfer_handle xfer, usbd_status status)
{
	ehci_isoc_trans_t trans_status;
	struct ehci_pipe *epipe;
	struct ehci_xfer *exfer;
	ehci_softc_t *sc;
	struct ehci_soft_itd *itd;
	int i, wake;

	epipe = (struct ehci_pipe *) xfer->pipe;
	exfer = EXFER(xfer);
	sc = epipe->pipe.device->bus->hci_private;

	DPRINTF(("ehci_abort_isoc_xfer: xfer %p pipe %p\n", xfer, epipe));

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_dying) {
		xfer->status = status;
		callout_stop(&xfer->timeout_handle);
		usb_transfer_complete(xfer);
		return;
	}

	if (xfer->hcflags & UXFER_ABORTING) {
		DPRINTFN(2, ("ehci_abort_isoc_xfer: already aborting\n"));

#ifdef DIAGNOSTIC
		if (status == USBD_TIMEOUT)
			printf("ehci_abort_isoc_xfer: TIMEOUT while aborting\n");
#endif

		xfer->status = status;
		DPRINTFN(2, ("ehci_abort_isoc_xfer: waiting for abort to finish\n"));
		xfer->hcflags |= UXFER_ABORTWAIT;
		while (xfer->hcflags & UXFER_ABORTING)
			cv_wait(&xfer->hccv, &sc->sc_lock);
		goto done;
	}
	xfer->hcflags |= UXFER_ABORTING;

	xfer->status = status;
	callout_stop(&xfer->timeout_handle);

	for (itd = exfer->itdstart; itd != NULL; itd = itd->xfer_next) {
		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_ctl),
		    sizeof(itd->itd.itd_ctl),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		for (i = 0; i < 8; i++) {
			trans_status = le32toh(itd->itd.itd_ctl[i]);
			trans_status &= ~EHCI_ITD_ACTIVE;
			itd->itd.itd_ctl[i] = htole32(trans_status);
		}

		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_ctl),
		    sizeof(itd->itd.itd_ctl),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}

        sc->sc_softwake = 1;
        usb_schedsoftintr(&sc->sc_bus);
	cv_wait(&sc->sc_softwake_cv, &sc->sc_lock);

#ifdef DIAGNOSTIC
	exfer->isdone = 1;
#endif
	wake = xfer->hcflags & UXFER_ABORTWAIT;
	xfer->hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);
	usb_transfer_complete(xfer);
	if (wake) {
		cv_broadcast(&xfer->hccv);
	}

done:
	KASSERT(mutex_owned(&sc->sc_lock));
	return;
}

Static void
ehci_timeout(void *addr)
{
	struct ehci_xfer *exfer = addr;
	struct ehci_pipe *epipe = (struct ehci_pipe *)exfer->xfer.pipe;
	ehci_softc_t *sc = epipe->pipe.device->bus->hci_private;

	DPRINTF(("ehci_timeout: exfer=%p\n", exfer));
#ifdef EHCI_DEBUG
	if (ehcidebug > 1)
		usbd_dump_pipe(exfer->xfer.pipe);
#endif

	if (sc->sc_dying) {
		mutex_enter(&sc->sc_lock);
		ehci_abort_xfer(&exfer->xfer, USBD_TIMEOUT);
		mutex_exit(&sc->sc_lock);
		return;
	}

	/* Execute the abort in a process context. */
	usb_init_task(&exfer->abort_task, ehci_timeout_task, addr);
	usb_add_task(exfer->xfer.pipe->device, &exfer->abort_task,
	    USB_TASKQ_HC);
}

Static void
ehci_timeout_task(void *addr)
{
	usbd_xfer_handle xfer = addr;
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;

	DPRINTF(("ehci_timeout_task: xfer=%p\n", xfer));

	mutex_enter(&sc->sc_lock);
	ehci_abort_xfer(xfer, USBD_TIMEOUT);
	mutex_exit(&sc->sc_lock);
}

/************************/

Static usbd_status
ehci_device_ctrl_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ehci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ehci_device_ctrl_start(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST)) {
		/* XXX panic */
		printf("ehci_device_ctrl_transfer: not a request\n");
		return (USBD_INVAL);
	}
#endif

	err = ehci_device_request(xfer);
	if (err) {
		return (err);
	}

	if (sc->sc_bus.use_polling)
		ehci_waitintr(sc, xfer);

	return (USBD_IN_PROGRESS);
}

Static void
ehci_device_ctrl_done(usbd_xfer_handle xfer)
{
	struct ehci_xfer *ex = EXFER(xfer);
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	usb_device_request_t *req = &xfer->request;
	int len = UGETW(req->wLength);
	int rd = req->bmRequestType & UT_READ;

	DPRINTFN(10,("ehci_ctrl_done: xfer=%p\n", xfer));

	KASSERT(mutex_owned(&sc->sc_lock));

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST)) {
		panic("ehci_ctrl_done: not a request");
	}
#endif

	if (xfer->status != USBD_NOMEM && ehci_active_intr_list(ex)) {
		ehci_del_intr_list(sc, ex);	/* remove from active list */
		ehci_free_sqtd_chain(sc, ex->sqtdstart, NULL);
		usb_syncmem(&epipe->u.ctl.reqdma, 0, sizeof *req,
		    BUS_DMASYNC_POSTWRITE);
		if (len)
			usb_syncmem(&xfer->dmabuf, 0, len,
			    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}

	DPRINTFN(5, ("ehci_ctrl_done: length=%d\n", xfer->actlen));
}

/* Abort a device control request. */
Static void
ehci_device_ctrl_abort(usbd_xfer_handle xfer)
{
	DPRINTF(("ehci_device_ctrl_abort: xfer=%p\n", xfer));
	ehci_abort_xfer(xfer, USBD_CANCELLED);
}

/* Close a device control pipe. */
Static void
ehci_device_ctrl_close(usbd_pipe_handle pipe)
{
	ehci_softc_t *sc = pipe->device->bus->hci_private;
	/*struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;*/

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF(("ehci_device_ctrl_close: pipe=%p\n", pipe));

	ehci_close_pipe(pipe, sc->sc_async_head);
}

Static usbd_status
ehci_device_request(usbd_xfer_handle xfer)
{
#define exfer EXFER(xfer)
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	usb_device_request_t *req = &xfer->request;
	usbd_device_handle dev = epipe->pipe.device;
	ehci_softc_t *sc = dev->bus->hci_private;
	int addr = dev->address;
	ehci_soft_qtd_t *setup, *stat, *next;
	ehci_soft_qh_t *sqh;
	int isread;
	int len;
	usbd_status err;

	isread = req->bmRequestType & UT_READ;
	len = UGETW(req->wLength);

	DPRINTFN(3,("ehci_device_request: type=0x%02x, request=0x%02x, "
		    "wValue=0x%04x, wIndex=0x%04x len=%d, addr=%d, endpt=%d\n",
		    req->bmRequestType, req->bRequest, UGETW(req->wValue),
		    UGETW(req->wIndex), len, addr,
		    epipe->pipe.endpoint->edesc->bEndpointAddress));

	setup = ehci_alloc_sqtd(sc);
	if (setup == NULL) {
		err = USBD_NOMEM;
		goto bad1;
	}
	stat = ehci_alloc_sqtd(sc);
	if (stat == NULL) {
		err = USBD_NOMEM;
		goto bad2;
	}

	mutex_enter(&sc->sc_lock);

	sqh = epipe->sqh;
	epipe->u.ctl.length = len;

	/*
	 * Update device address and length since they may have changed
	 * during the setup of the control pipe in usbd_new_device().
	 */
	/* XXX This only needs to be done once, but it's too early in open. */
	/* XXXX Should not touch ED here! */
	sqh->qh.qh_endp =
	    (sqh->qh.qh_endp & htole32(~(EHCI_QH_ADDRMASK | EHCI_QH_MPLMASK))) |
	    htole32(
	     EHCI_QH_SET_ADDR(addr) |
	     EHCI_QH_SET_MPL(UGETW(epipe->pipe.endpoint->edesc->wMaxPacketSize))
	    );

	/* Set up data transaction */
	if (len != 0) {
		ehci_soft_qtd_t *end;

		/* Start toggle at 1. */
		epipe->nexttoggle = 1;
		err = ehci_alloc_sqtd_chain(epipe, sc, len, isread, xfer,
			  &next, &end);
		if (err)
			goto bad3;
		end->qtd.qtd_status &= htole32(~EHCI_QTD_IOC);
		end->nextqtd = stat;
		end->qtd.qtd_next =
		end->qtd.qtd_altnext = htole32(stat->physaddr);
		usb_syncmem(&end->dma, end->offs, sizeof(end->qtd),
		   BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	} else {
		next = stat;
	}

	memcpy(KERNADDR(&epipe->u.ctl.reqdma, 0), req, sizeof *req);
	usb_syncmem(&epipe->u.ctl.reqdma, 0, sizeof *req, BUS_DMASYNC_PREWRITE);

	/* Clear toggle */
	setup->qtd.qtd_status = htole32(
	    EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_PID(EHCI_QTD_PID_SETUP) |
	    EHCI_QTD_SET_CERR(3) |
	    EHCI_QTD_SET_TOGGLE(0) |
	    EHCI_QTD_SET_BYTES(sizeof *req)
	    );
	setup->qtd.qtd_buffer[0] = htole32(DMAADDR(&epipe->u.ctl.reqdma, 0));
	setup->qtd.qtd_buffer_hi[0] = 0;
	setup->nextqtd = next;
	setup->qtd.qtd_next = setup->qtd.qtd_altnext = htole32(next->physaddr);
	setup->xfer = xfer;
	setup->len = sizeof *req;
	usb_syncmem(&setup->dma, setup->offs, sizeof(setup->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	stat->qtd.qtd_status = htole32(
	    EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_PID(isread ? EHCI_QTD_PID_OUT : EHCI_QTD_PID_IN) |
	    EHCI_QTD_SET_CERR(3) |
	    EHCI_QTD_SET_TOGGLE(1) |
	    EHCI_QTD_IOC
	    );
	stat->qtd.qtd_buffer[0] = 0; /* XXX not needed? */
	stat->qtd.qtd_buffer_hi[0] = 0; /* XXX not needed? */
	stat->nextqtd = NULL;
	stat->qtd.qtd_next = stat->qtd.qtd_altnext = EHCI_NULL;
	stat->xfer = xfer;
	stat->len = 0;
	usb_syncmem(&stat->dma, stat->offs, sizeof(stat->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef EHCI_DEBUG
	if (ehcidebug > 5) {
		DPRINTF(("ehci_device_request:\n"));
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(setup);
	}
#endif

	exfer->sqtdstart = setup;
	exfer->sqtdend = stat;
#ifdef DIAGNOSTIC
	if (!exfer->isdone) {
		printf("ehci_device_request: not done, exfer=%p\n", exfer);
	}
	exfer->isdone = 0;
#endif

	/* Insert qTD in QH list. */
	ehci_set_qh_qtd(sqh, setup); /* also does usb_syncmem(sqh) */
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		callout_reset(&xfer->timeout_handle, mstohz(xfer->timeout),
		    ehci_timeout, xfer);
	}
	ehci_add_intr_list(sc, exfer);
	xfer->status = USBD_IN_PROGRESS;
	mutex_exit(&sc->sc_lock);

#ifdef EHCI_DEBUG
	if (ehcidebug > 10) {
		DPRINTF(("ehci_device_request: status=%x\n",
			 EOREAD4(sc, EHCI_USBSTS)));
		delay(10000);
		ehci_dump_regs(sc);
		ehci_dump_sqh(sc->sc_async_head);
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(setup);
	}
#endif

	return (USBD_NORMAL_COMPLETION);

 bad3:
	mutex_exit(&sc->sc_lock);
	ehci_free_sqtd(sc, stat);
 bad2:
	ehci_free_sqtd(sc, setup);
 bad1:
	DPRINTFN(-1,("ehci_device_request: no memory\n"));
	mutex_enter(&sc->sc_lock);
	xfer->status = err;
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
	return (err);
#undef exfer
}

/*
 * Some EHCI chips from VIA seem to trigger interrupts before writing back the
 * qTD status, or miss signalling occasionally under heavy load.  If the host
 * machine is too fast, we we can miss transaction completion - when we scan
 * the active list the transaction still seems to be active.  This generally
 * exhibits itself as a umass stall that never recovers.
 *
 * We work around this behaviour by setting up this callback after any softintr
 * that completes with transactions still pending, giving us another chance to
 * check for completion after the writeback has taken place.
 */
Static void
ehci_intrlist_timeout(void *arg)
{
	ehci_softc_t *sc = arg;

	DPRINTF(("ehci_intrlist_timeout\n"));
	usb_schedsoftintr(&sc->sc_bus);
}

/************************/

Static usbd_status
ehci_device_bulk_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ehci_device_bulk_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ehci_device_bulk_start(usbd_xfer_handle xfer)
{
#define exfer EXFER(xfer)
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	usbd_device_handle dev = epipe->pipe.device;
	ehci_softc_t *sc = dev->bus->hci_private;
	ehci_soft_qtd_t *data, *dataend;
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int len, isread, endpt;

	DPRINTFN(2, ("ehci_device_bulk_start: xfer=%p len=%d flags=%d\n",
		     xfer, xfer->length, xfer->flags));

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (xfer->rqflags & URQ_REQUEST)
		panic("ehci_device_bulk_start: a request");
#endif

	mutex_enter(&sc->sc_lock);

	len = xfer->length;
	endpt = epipe->pipe.endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sqh = epipe->sqh;

	epipe->u.bulk.length = len;

	err = ehci_alloc_sqtd_chain(epipe, sc, len, isread, xfer, &data,
				   &dataend);
	if (err) {
		DPRINTFN(-1,("ehci_device_bulk_transfer: no memory\n"));
		xfer->status = err;
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);
		return (err);
	}

#ifdef EHCI_DEBUG
	if (ehcidebug > 5) {
		DPRINTF(("ehci_device_bulk_start: data(1)\n"));
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(data);
	}
#endif

	/* Set up interrupt info. */
	exfer->sqtdstart = data;
	exfer->sqtdend = dataend;
#ifdef DIAGNOSTIC
	if (!exfer->isdone) {
		printf("ehci_device_bulk_start: not done, ex=%p\n", exfer);
	}
	exfer->isdone = 0;
#endif

	ehci_set_qh_qtd(sqh, data); /* also does usb_syncmem(sqh) */
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		callout_reset(&xfer->timeout_handle, mstohz(xfer->timeout),
		    ehci_timeout, xfer);
	}
	ehci_add_intr_list(sc, exfer);
	xfer->status = USBD_IN_PROGRESS;
	mutex_exit(&sc->sc_lock);

#ifdef EHCI_DEBUG
	if (ehcidebug > 10) {
		DPRINTF(("ehci_device_bulk_start: data(2)\n"));
		delay(10000);
		DPRINTF(("ehci_device_bulk_start: data(3)\n"));
		ehci_dump_regs(sc);
#if 0
		printf("async_head:\n");
		ehci_dump_sqh(sc->sc_async_head);
#endif
		printf("sqh:\n");
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(data);
	}
#endif

	if (sc->sc_bus.use_polling)
		ehci_waitintr(sc, xfer);

	return (USBD_IN_PROGRESS);
#undef exfer
}

Static void
ehci_device_bulk_abort(usbd_xfer_handle xfer)
{
	DPRINTF(("ehci_device_bulk_abort: xfer=%p\n", xfer));
	ehci_abort_xfer(xfer, USBD_CANCELLED);
}

/*
 * Close a device bulk pipe.
 */
Static void
ehci_device_bulk_close(usbd_pipe_handle pipe)
{
	ehci_softc_t *sc = pipe->device->bus->hci_private;
	struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF(("ehci_device_bulk_close: pipe=%p\n", pipe));
	pipe->endpoint->datatoggle = epipe->nexttoggle;
	ehci_close_pipe(pipe, sc->sc_async_head);
}

Static void
ehci_device_bulk_done(usbd_xfer_handle xfer)
{
	struct ehci_xfer *ex = EXFER(xfer);
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	int endpt = epipe->pipe.endpoint->edesc->bEndpointAddress;
	int rd = UE_GET_DIR(endpt) == UE_DIR_IN;

	DPRINTFN(10,("ehci_bulk_done: xfer=%p, actlen=%d\n",
		     xfer, xfer->actlen));

	KASSERT(mutex_owned(&sc->sc_lock));

	if (xfer->status != USBD_NOMEM && ehci_active_intr_list(ex)) {
		ehci_del_intr_list(sc, ex);	/* remove from active list */
		ehci_free_sqtd_chain(sc, ex->sqtdstart, NULL);
		usb_syncmem(&xfer->dmabuf, 0, xfer->length,
		    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}

	DPRINTFN(5, ("ehci_bulk_done: length=%d\n", xfer->actlen));
}

/************************/

Static usbd_status
ehci_device_setintr(ehci_softc_t *sc, ehci_soft_qh_t *sqh, int ival)
{
	struct ehci_soft_islot *isp;
	int islot, lev;

	/* Find a poll rate that is large enough. */
	for (lev = EHCI_IPOLLRATES - 1; lev > 0; lev--)
		if (EHCI_ILEV_IVAL(lev) <= ival)
			break;

	/* Pick an interrupt slot at the right level. */
	/* XXX could do better than picking at random */
	sc->sc_rand = (sc->sc_rand + 191) % sc->sc_flsize;
	islot = EHCI_IQHIDX(lev, sc->sc_rand);

	sqh->islot = islot;
	isp = &sc->sc_islots[islot];
	mutex_enter(&sc->sc_lock);
	ehci_add_qh(sc, sqh, isp->sqh);
	mutex_exit(&sc->sc_lock);

	return (USBD_NORMAL_COMPLETION);
}

Static usbd_status
ehci_device_intr_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return (err);

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return (ehci_device_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ehci_device_intr_start(usbd_xfer_handle xfer)
{
#define exfer EXFER(xfer)
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	usbd_device_handle dev = xfer->pipe->device;
	ehci_softc_t *sc = dev->bus->hci_private;
	ehci_soft_qtd_t *data, *dataend;
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int len, isread, endpt;

	DPRINTFN(2, ("ehci_device_intr_start: xfer=%p len=%d flags=%d\n",
	    xfer, xfer->length, xfer->flags));

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (xfer->rqflags & URQ_REQUEST)
		panic("ehci_device_intr_start: a request");
#endif

	mutex_enter(&sc->sc_lock);

	len = xfer->length;
	endpt = epipe->pipe.endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sqh = epipe->sqh;

	epipe->u.intr.length = len;

	err = ehci_alloc_sqtd_chain(epipe, sc, len, isread, xfer, &data,
	    &dataend);
	if (err) {
		DPRINTFN(-1, ("ehci_device_intr_start: no memory\n"));
		xfer->status = err;
		usb_transfer_complete(xfer);
		mutex_exit(&sc->sc_lock);
		return (err);
	}

#ifdef EHCI_DEBUG
	if (ehcidebug > 5) {
		DPRINTF(("ehci_device_intr_start: data(1)\n"));
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(data);
	}
#endif

	/* Set up interrupt info. */
	exfer->sqtdstart = data;
	exfer->sqtdend = dataend;
#ifdef DIAGNOSTIC
	if (!exfer->isdone) {
		printf("ehci_device_intr_start: not done, ex=%p\n", exfer);
	}
	exfer->isdone = 0;
#endif

	ehci_set_qh_qtd(sqh, data); /* also does usb_syncmem(sqh) */
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		callout_reset(&xfer->timeout_handle, mstohz(xfer->timeout),
		    ehci_timeout, xfer);
	}
	ehci_add_intr_list(sc, exfer);
	xfer->status = USBD_IN_PROGRESS;
	mutex_exit(&sc->sc_lock);

#ifdef EHCI_DEBUG
	if (ehcidebug > 10) {
		DPRINTF(("ehci_device_intr_start: data(2)\n"));
		delay(10000);
		DPRINTF(("ehci_device_intr_start: data(3)\n"));
		ehci_dump_regs(sc);
		printf("sqh:\n");
		ehci_dump_sqh(sqh);
		ehci_dump_sqtds(data);
	}
#endif

	if (sc->sc_bus.use_polling)
		ehci_waitintr(sc, xfer);

	return (USBD_IN_PROGRESS);
#undef exfer
}

Static void
ehci_device_intr_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(1, ("ehci_device_intr_abort: xfer=%p\n", xfer));
	if (xfer->pipe->intrxfer == xfer) {
		DPRINTFN(1, ("echi_device_intr_abort: remove\n"));
		xfer->pipe->intrxfer = NULL;
	}
	/*
	 * XXX - abort_xfer uses ehci_sync_hc, which syncs via the advance
	 *       async doorbell. That's dependent on the async list, wheras
	 *       intr xfers are periodic, should not use this?
	 */
	ehci_abort_xfer(xfer, USBD_CANCELLED);
}

Static void
ehci_device_intr_close(usbd_pipe_handle pipe)
{
	ehci_softc_t *sc = pipe->device->bus->hci_private;
	struct ehci_pipe *epipe = (struct ehci_pipe *)pipe;
	struct ehci_soft_islot *isp;

	KASSERT(mutex_owned(&sc->sc_lock));

	isp = &sc->sc_islots[epipe->sqh->islot];
	ehci_close_pipe(pipe, isp->sqh);
}

Static void
ehci_device_intr_done(usbd_xfer_handle xfer)
{
#define exfer EXFER(xfer)
	struct ehci_xfer *ex = EXFER(xfer);
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	struct ehci_pipe *epipe = (struct ehci_pipe *)xfer->pipe;
	ehci_soft_qtd_t *data, *dataend;
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int len, isread, endpt;

	DPRINTFN(10, ("ehci_device_intr_done: xfer=%p, actlen=%d\n",
	    xfer, xfer->actlen));

	KASSERT(mutex_owned(&sc->sc_lock));

	if (xfer->pipe->repeat) {
		ehci_free_sqtd_chain(sc, ex->sqtdstart, NULL);

		len = epipe->u.intr.length;
		xfer->length = len;
		endpt = epipe->pipe.endpoint->edesc->bEndpointAddress;
		isread = UE_GET_DIR(endpt) == UE_DIR_IN;
		usb_syncmem(&xfer->dmabuf, 0, len,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		sqh = epipe->sqh;

		err = ehci_alloc_sqtd_chain(epipe, sc, len, isread, xfer,
		    &data, &dataend);
		if (err) {
			DPRINTFN(-1, ("ehci_device_intr_done: no memory\n"));
			xfer->status = err;
			return;
		}

		/* Set up interrupt info. */
		exfer->sqtdstart = data;
		exfer->sqtdend = dataend;
#ifdef DIAGNOSTIC
		if (!exfer->isdone) {
			printf("ehci_device_intr_done: not done, ex=%p\n",
			    exfer);
		}
		exfer->isdone = 0;
#endif

		ehci_set_qh_qtd(sqh, data); /* also does usb_syncmem(sqh) */
		if (xfer->timeout && !sc->sc_bus.use_polling) {
			callout_reset(&xfer->timeout_handle,
			    mstohz(xfer->timeout), ehci_timeout, xfer);
		}

		xfer->status = USBD_IN_PROGRESS;
	} else if (xfer->status != USBD_NOMEM && ehci_active_intr_list(ex)) {
		ehci_del_intr_list(sc, ex); /* remove from active list */
		ehci_free_sqtd_chain(sc, ex->sqtdstart, NULL);
		endpt = epipe->pipe.endpoint->edesc->bEndpointAddress;
		isread = UE_GET_DIR(endpt) == UE_DIR_IN;
		usb_syncmem(&xfer->dmabuf, 0, xfer->length,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}
#undef exfer
}

/************************/

Static usbd_status
ehci_device_isoc_transfer(usbd_xfer_handle xfer)
{
	ehci_softc_t *sc = xfer->pipe->device->bus->hci_private;
	usbd_status err;

	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err && err != USBD_IN_PROGRESS)
		return err;

	return ehci_device_isoc_start(xfer);
}

Static usbd_status
ehci_device_isoc_start(usbd_xfer_handle xfer)
{
	struct ehci_pipe *epipe;
	usbd_device_handle dev;
	ehci_softc_t *sc;
	struct ehci_xfer *exfer;
	ehci_soft_itd_t *itd, *prev, *start, *stop;
	usb_dma_t *dma_buf;
	int i, j, k, frames, uframes, ufrperframe;
	int trans_count, offs, total_length;
	int frindex;

	start = NULL;
	prev = NULL;
	itd = NULL;
	trans_count = 0;
	total_length = 0;
	exfer = (struct ehci_xfer *) xfer;
	sc = xfer->pipe->device->bus->hci_private;
	dev = xfer->pipe->device;
	epipe = (struct ehci_pipe *)xfer->pipe;

	/*
	 * To allow continuous transfers, above we start all transfers
	 * immediately. However, we're still going to get usbd_start_next call
	 * this when another xfer completes. So, check if this is already
	 * in progress or not
	 */

	if (exfer->itdstart != NULL)
		return USBD_IN_PROGRESS;

	DPRINTFN(2, ("ehci_device_isoc_start: xfer %p len %d flags %d\n",
			xfer, xfer->length, xfer->flags));

	if (sc->sc_dying)
		return USBD_IOERROR;

	/*
	 * To avoid complication, don't allow a request right now that'll span
	 * the entire frame table. To within 4 frames, to allow some leeway
	 * on either side of where the hc currently is.
	 */
	if ((1 << (epipe->pipe.endpoint->edesc->bInterval)) *
			xfer->nframes >= (sc->sc_flsize - 4) * 8) {
		printf("ehci: isoc descriptor requested that spans the entire frametable, too many frames\n");
		return USBD_INVAL;
	}

#ifdef DIAGNOSTIC
	if (xfer->rqflags & URQ_REQUEST)
		panic("ehci_device_isoc_start: request\n");

	if (!exfer->isdone)
		printf("ehci_device_isoc_start: not done, ex = %p\n", exfer);
	exfer->isdone = 0;
#endif

	/*
	 * Step 1: Allocate and initialize itds, how many do we need?
	 * One per transfer if interval >= 8 microframes, fewer if we use
	 * multiple microframes per frame.
	 */

	i = epipe->pipe.endpoint->edesc->bInterval;
	if (i > 16 || i == 0) {
		/* Spec page 271 says intervals > 16 are invalid */
		DPRINTF(("ehci_device_isoc_start: bInvertal %d invalid\n", i));
		return USBD_INVAL;
	}

	ufrperframe = max(1, USB_UFRAMES_PER_FRAME / (1 << (i - 1)));
	frames = (xfer->nframes + (ufrperframe - 1)) / ufrperframe;
	uframes = USB_UFRAMES_PER_FRAME / ufrperframe;

	if (frames == 0) {
		DPRINTF(("ehci_device_isoc_start: frames == 0\n"));
		return USBD_INVAL;
	}

	dma_buf = &xfer->dmabuf;
	offs = 0;

	for (i = 0; i < frames; i++) {
		int froffs = offs;
		itd = ehci_alloc_itd(sc);

		if (prev != NULL) {
			prev->itd.itd_next =
			    htole32(itd->physaddr | EHCI_LINK_ITD);
			usb_syncmem(&itd->dma,
			    itd->offs + offsetof(ehci_itd_t, itd_next),
                	    sizeof(itd->itd.itd_next), BUS_DMASYNC_POSTWRITE);

			prev->xfer_next = itd;
	    	} else {
			start = itd;
		}

		/*
		 * Step 1.5, initialize uframes
		 */
		for (j = 0; j < EHCI_ITD_NUFRAMES; j += uframes) {
			/* Calculate which page in the list this starts in */
			int addr = DMAADDR(dma_buf, froffs);
			addr = EHCI_PAGE_OFFSET(addr);
			addr += (offs - froffs);
			addr = EHCI_PAGE(addr);
			addr /= EHCI_PAGE_SIZE;

			/* This gets the initial offset into the first page,
			 * looks how far further along the current uframe
			 * offset is. Works out how many pages that is.
			 */

			itd->itd.itd_ctl[j] = htole32 ( EHCI_ITD_ACTIVE |
			    EHCI_ITD_SET_LEN(xfer->frlengths[trans_count]) |
			    EHCI_ITD_SET_PG(addr) |
			    EHCI_ITD_SET_OFFS(EHCI_PAGE_OFFSET(DMAADDR(dma_buf,offs))));

			total_length += xfer->frlengths[trans_count];
			offs += xfer->frlengths[trans_count];
			trans_count++;

			if (trans_count >= xfer->nframes) { /*Set IOC*/
				itd->itd.itd_ctl[j] |= htole32(EHCI_ITD_IOC);
				break;
			}
		}

		/* Step 1.75, set buffer pointers. To simplify matters, all
		 * pointers are filled out for the next 7 hardware pages in
		 * the dma block, so no need to worry what pages to cover
		 * and what to not.
		 */

		for (j = 0; j < EHCI_ITD_NBUFFERS; j++) {
			/*
			 * Don't try to lookup a page that's past the end
			 * of buffer
			 */
			int page_offs = EHCI_PAGE(froffs + (EHCI_PAGE_SIZE * j));
			if (page_offs >= dma_buf->block->size)
				break;

			unsigned long long page = DMAADDR(dma_buf, page_offs);
			page = EHCI_PAGE(page);
			itd->itd.itd_bufr[j] =
			    htole32(EHCI_ITD_SET_BPTR(page));
			itd->itd.itd_bufr_hi[j] =
			    htole32(page >> 32);
		}

		/*
		 * Other special values
		 */

		k = epipe->pipe.endpoint->edesc->bEndpointAddress;
		itd->itd.itd_bufr[0] |= htole32(EHCI_ITD_SET_EP(UE_GET_ADDR(k)) |
		    EHCI_ITD_SET_DADDR(epipe->pipe.device->address));

		k = (UE_GET_DIR(epipe->pipe.endpoint->edesc->bEndpointAddress))
		    ? 1 : 0;
		j = UGETW(epipe->pipe.endpoint->edesc->wMaxPacketSize);
		itd->itd.itd_bufr[1] |= htole32(EHCI_ITD_SET_DIR(k) |
		    EHCI_ITD_SET_MAXPKT(UE_GET_SIZE(j)));

		/* FIXME: handle invalid trans */
		itd->itd.itd_bufr[2] |=
		    htole32(EHCI_ITD_SET_MULTI(UE_GET_TRANS(j)+1));

		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_next),
                    sizeof(ehci_itd_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		prev = itd;
	} /* End of frame */

	stop = itd;
	stop->xfer_next = NULL;
	exfer->isoc_len = total_length;

	usb_syncmem(&exfer->xfer.dmabuf, 0, total_length,
		BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Part 2: Transfer descriptors have now been set up, now they must
	 * be scheduled into the period frame list. Erk. Not wanting to
	 * complicate matters, transfer is denied if the transfer spans
	 * more than the period frame list.
	 */

	mutex_enter(&sc->sc_lock);

	/* Start inserting frames */
	if (epipe->u.isoc.cur_xfers > 0) {
		frindex = epipe->u.isoc.next_frame;
	} else {
		frindex = EOREAD4(sc, EHCI_FRINDEX);
		frindex = frindex >> 3; /* Erase microframe index */
		frindex += 2;
	}

	if (frindex >= sc->sc_flsize)
		frindex &= (sc->sc_flsize - 1);

	/* What's the frame interval? */
	i = (1 << (epipe->pipe.endpoint->edesc->bInterval - 1));
	if (i / USB_UFRAMES_PER_FRAME == 0)
		i = 1;
	else
		i /= USB_UFRAMES_PER_FRAME;

	itd = start;
	for (j = 0; j < frames; j++) {
		if (itd == NULL)
			panic("ehci: unexpectedly ran out of isoc itds, isoc_start\n");

		itd->itd.itd_next = sc->sc_flist[frindex];
		if (itd->itd.itd_next == 0)
			/* FIXME: frindex table gets initialized to NULL
			 * or EHCI_NULL? */
			itd->itd.itd_next = EHCI_NULL;

		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_next),
                    sizeof(itd->itd.itd_next),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		sc->sc_flist[frindex] = htole32(EHCI_LINK_ITD | itd->physaddr);

		usb_syncmem(&sc->sc_fldma,
		    sizeof(ehci_link_t) * frindex,
                    sizeof(ehci_link_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		itd->u.frame_list.next = sc->sc_softitds[frindex];
		sc->sc_softitds[frindex] = itd;
		if (itd->u.frame_list.next != NULL)
			itd->u.frame_list.next->u.frame_list.prev = itd;
		itd->slot = frindex;
		itd->u.frame_list.prev = NULL;

		frindex += i;
		if (frindex >= sc->sc_flsize)
			frindex -= sc->sc_flsize;

		itd = itd->xfer_next;
	}

	epipe->u.isoc.cur_xfers++;
	epipe->u.isoc.next_frame = frindex;

	exfer->itdstart = start;
	exfer->itdend = stop;
	exfer->sqtdstart = NULL;
	exfer->sqtdstart = NULL;

	ehci_add_intr_list(sc, exfer);
	xfer->status = USBD_IN_PROGRESS;
	xfer->done = 0;
	mutex_exit(&sc->sc_lock);

	if (sc->sc_bus.use_polling) {
		printf("Starting ehci isoc xfer with polling. Bad idea?\n");
		ehci_waitintr(sc, xfer);
	}

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_isoc_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(1, ("ehci_device_isoc_abort: xfer = %p\n", xfer));
	ehci_abort_isoc_xfer(xfer, USBD_CANCELLED);
}

Static void
ehci_device_isoc_close(usbd_pipe_handle pipe)
{
	DPRINTFN(1, ("ehci_device_isoc_close: nothing in the pipe to free?\n"));
}

Static void
ehci_device_isoc_done(usbd_xfer_handle xfer)
{
	struct ehci_xfer *exfer;
	ehci_softc_t *sc;
	struct ehci_pipe *epipe;

	exfer = EXFER(xfer);
	sc = xfer->pipe->device->bus->hci_private;
	epipe = (struct ehci_pipe *) xfer->pipe;

	KASSERT(mutex_owned(&sc->sc_lock));

	epipe->u.isoc.cur_xfers--;
	if (xfer->status != USBD_NOMEM && ehci_active_intr_list(exfer)) {
		ehci_del_intr_list(sc, exfer);
		ehci_rem_free_itd_chain(sc, exfer);
	}

	usb_syncmem(&xfer->dmabuf, 0, xfer->length, BUS_DMASYNC_POSTWRITE |
                    BUS_DMASYNC_POSTREAD);

}
