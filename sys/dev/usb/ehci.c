/*	$NetBSD: ehci.c,v 1.309 2022/03/13 11:29:21 riastradh Exp $ */

/*
 * Copyright (c) 2004-2012,2016,2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net), Charles M. Hannum,
 * Jeremy Morse (jeremy.morse@gmail.com), Jared D. McNeill
 * (jmcneill@invisible.ca). Matthew R. Green (mrg@eterna.com.au), and
 * Nick Hudson .
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
__KERNEL_RCSID(0, "$NetBSD: ehci.c,v 1.309 2022/03/13 11:29:21 riastradh Exp $");

#include "ohci.h"
#include "uhci.h"

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#include <dev/usb/usbroothub.h>

#ifdef USB_DEBUG
#ifndef EHCI_DEBUG
#define ehcidebug 0
#else
static int ehcidebug = 0;

SYSCTL_SETUP(sysctl_hw_ehci_setup, "sysctl hw.ehci setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "ehci",
	    SYSCTL_DESCR("ehci global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &ehcidebug, sizeof(ehcidebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* EHCI_DEBUG */
#endif /* USB_DEBUG */

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(ehcidebug,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(ehcidebug,N,FMT,A,B,C,D)
#define	EHCIHIST_FUNC()		USBHIST_FUNC()
#define	EHCIHIST_CALLED()	USBHIST_CALLED(ehcidebug)

struct ehci_pipe {
	struct usbd_pipe pipe;
	int nexttoggle;

	ehci_soft_qh_t *sqh;
	union {
		/* Control pipe */
		struct {
			usb_dma_t reqdma;
		} ctrl;
		/* Interrupt pipe */
		struct {
			u_int length;
		} intr;
		/* Iso pipe */
		struct {
			u_int next_frame;
			u_int cur_xfers;
		} isoc;
	};
};

typedef TAILQ_HEAD(ex_completeq, ehci_xfer) ex_completeq_t;

Static usbd_status	ehci_open(struct usbd_pipe *);
Static void		ehci_poll(struct usbd_bus *);
Static void		ehci_softintr(void *);
Static int		ehci_intr1(ehci_softc_t *);
Static void		ehci_check_qh_intr(ehci_softc_t *, struct ehci_xfer *,
			    ex_completeq_t *);
Static void		ehci_check_itd_intr(ehci_softc_t *, struct ehci_xfer *,
			    ex_completeq_t *);
Static void		ehci_check_sitd_intr(ehci_softc_t *, struct ehci_xfer *,
			    ex_completeq_t *);
Static void		ehci_idone(struct ehci_xfer *, ex_completeq_t *);
Static void		ehci_intrlist_timeout(void *);
Static void		ehci_doorbell(void *);
Static void		ehci_pcd(void *);

Static struct usbd_xfer *
			ehci_allocx(struct usbd_bus *, unsigned int);
Static void		ehci_freex(struct usbd_bus *, struct usbd_xfer *);

Static void		ehci_get_lock(struct usbd_bus *, kmutex_t **);
Static bool		ehci_dying(struct usbd_bus *);
Static int		ehci_roothub_ctrl(struct usbd_bus *,
			    usb_device_request_t *, void *, int);

Static usbd_status	ehci_root_intr_transfer(struct usbd_xfer *);
Static usbd_status	ehci_root_intr_start(struct usbd_xfer *);
Static void		ehci_root_intr_abort(struct usbd_xfer *);
Static void		ehci_root_intr_close(struct usbd_pipe *);
Static void		ehci_root_intr_done(struct usbd_xfer *);

Static int		ehci_device_ctrl_init(struct usbd_xfer *);
Static void		ehci_device_ctrl_fini(struct usbd_xfer *);
Static usbd_status	ehci_device_ctrl_transfer(struct usbd_xfer *);
Static usbd_status	ehci_device_ctrl_start(struct usbd_xfer *);
Static void		ehci_device_ctrl_abort(struct usbd_xfer *);
Static void		ehci_device_ctrl_close(struct usbd_pipe *);
Static void		ehci_device_ctrl_done(struct usbd_xfer *);

Static int		ehci_device_bulk_init(struct usbd_xfer *);
Static void		ehci_device_bulk_fini(struct usbd_xfer *);
Static usbd_status	ehci_device_bulk_transfer(struct usbd_xfer *);
Static usbd_status	ehci_device_bulk_start(struct usbd_xfer *);
Static void		ehci_device_bulk_abort(struct usbd_xfer *);
Static void		ehci_device_bulk_close(struct usbd_pipe *);
Static void		ehci_device_bulk_done(struct usbd_xfer *);

Static int		ehci_device_intr_init(struct usbd_xfer *);
Static void		ehci_device_intr_fini(struct usbd_xfer *);
Static usbd_status	ehci_device_intr_transfer(struct usbd_xfer *);
Static usbd_status	ehci_device_intr_start(struct usbd_xfer *);
Static void		ehci_device_intr_abort(struct usbd_xfer *);
Static void		ehci_device_intr_close(struct usbd_pipe *);
Static void		ehci_device_intr_done(struct usbd_xfer *);

Static int		ehci_device_isoc_init(struct usbd_xfer *);
Static void		ehci_device_isoc_fini(struct usbd_xfer *);
Static usbd_status	ehci_device_isoc_transfer(struct usbd_xfer *);
Static void		ehci_device_isoc_abort(struct usbd_xfer *);
Static void		ehci_device_isoc_close(struct usbd_pipe *);
Static void		ehci_device_isoc_done(struct usbd_xfer *);

Static int		ehci_device_fs_isoc_init(struct usbd_xfer *);
Static void		ehci_device_fs_isoc_fini(struct usbd_xfer *);
Static usbd_status	ehci_device_fs_isoc_transfer(struct usbd_xfer *);
Static void		ehci_device_fs_isoc_abort(struct usbd_xfer *);
Static void		ehci_device_fs_isoc_close(struct usbd_pipe *);
Static void		ehci_device_fs_isoc_done(struct usbd_xfer *);

Static void		ehci_device_clear_toggle(struct usbd_pipe *);
Static void		ehci_noop(struct usbd_pipe *);

Static void		ehci_disown(ehci_softc_t *, int, int);

Static ehci_soft_qh_t *	ehci_alloc_sqh(ehci_softc_t *);
Static void		ehci_free_sqh(ehci_softc_t *, ehci_soft_qh_t *);

Static ehci_soft_qtd_t *ehci_alloc_sqtd(ehci_softc_t *);
Static void		ehci_free_sqtd(ehci_softc_t *, ehci_soft_qtd_t *);
Static int		ehci_alloc_sqtd_chain(ehci_softc_t *,
			    struct usbd_xfer *, int, int, ehci_soft_qtd_t **);
Static void		ehci_free_sqtds(ehci_softc_t *, struct ehci_xfer *);

Static void		ehci_reset_sqtd_chain(ehci_softc_t *, struct usbd_xfer *,
			    int, int, int *, ehci_soft_qtd_t **);
Static void		ehci_append_sqtd(ehci_soft_qtd_t *, ehci_soft_qtd_t *);

Static ehci_soft_itd_t *ehci_alloc_itd(ehci_softc_t *);
Static ehci_soft_sitd_t *
			ehci_alloc_sitd(ehci_softc_t *);

Static void 		ehci_remove_itd_chain(ehci_softc_t *, ehci_soft_itd_t *);
Static void		ehci_remove_sitd_chain(ehci_softc_t *, ehci_soft_sitd_t *);
Static void 		ehci_free_itd_chain(ehci_softc_t *, ehci_soft_itd_t *);
Static void		ehci_free_sitd_chain(ehci_softc_t *, ehci_soft_sitd_t *);

static inline void
ehci_free_itd_locked(ehci_softc_t *sc, ehci_soft_itd_t *itd)
{

	LIST_INSERT_HEAD(&sc->sc_freeitds, itd, free_list);
}

static inline void
ehci_free_sitd_locked(ehci_softc_t *sc, ehci_soft_sitd_t *sitd)
{

	LIST_INSERT_HEAD(&sc->sc_freesitds, sitd, free_list);
}

Static void 		ehci_abort_isoc_xfer(struct usbd_xfer *, usbd_status);

Static usbd_status	ehci_device_setintr(ehci_softc_t *, ehci_soft_qh_t *,
			    int);

Static void		ehci_add_qh(ehci_softc_t *, ehci_soft_qh_t *,
				    ehci_soft_qh_t *);
Static void		ehci_rem_qh(ehci_softc_t *, ehci_soft_qh_t *,
				    ehci_soft_qh_t *);
Static void		ehci_set_qh_qtd(ehci_soft_qh_t *, ehci_soft_qtd_t *);
Static void		ehci_sync_hc(ehci_softc_t *);

Static void		ehci_close_pipe(struct usbd_pipe *, ehci_soft_qh_t *);
Static void		ehci_abortx(struct usbd_xfer *);

#ifdef EHCI_DEBUG
Static ehci_softc_t 	*theehci;
void			ehci_dump(void);
#endif

#ifdef EHCI_DEBUG
Static void		ehci_dump_regs(ehci_softc_t *);
Static void		ehci_dump_sqtds(ehci_soft_qtd_t *);
Static void		ehci_dump_sqtd(ehci_soft_qtd_t *);
Static void		ehci_dump_qtd(ehci_qtd_t *);
Static void		ehci_dump_sqh(ehci_soft_qh_t *);
Static void		ehci_dump_sitd(struct ehci_soft_itd *);
Static void 		ehci_dump_itds(ehci_soft_itd_t *);
Static void		ehci_dump_itd(struct ehci_soft_itd *);
Static void		ehci_dump_exfer(struct ehci_xfer *);
#endif

#define EHCI_NULL htole32(EHCI_LINK_TERMINATE)

static inline void
ehci_add_intr_list(ehci_softc_t *sc, struct ehci_xfer *ex)
{

	TAILQ_INSERT_TAIL(&sc->sc_intrhead, ex, ex_next);
}

static inline void
ehci_del_intr_list(ehci_softc_t *sc, struct ehci_xfer *ex)
{

	TAILQ_REMOVE(&sc->sc_intrhead, ex, ex_next);
}

Static const struct usbd_bus_methods ehci_bus_methods = {
	.ubm_open =	ehci_open,
	.ubm_softint =	ehci_softintr,
	.ubm_dopoll =	ehci_poll,
	.ubm_allocx =	ehci_allocx,
	.ubm_freex =	ehci_freex,
	.ubm_abortx =	ehci_abortx,
	.ubm_dying =	ehci_dying,
	.ubm_getlock =	ehci_get_lock,
	.ubm_rhctrl =	ehci_roothub_ctrl,
};

Static const struct usbd_pipe_methods ehci_root_intr_methods = {
	.upm_transfer =	ehci_root_intr_transfer,
	.upm_start =	ehci_root_intr_start,
	.upm_abort =	ehci_root_intr_abort,
	.upm_close =	ehci_root_intr_close,
	.upm_cleartoggle =	ehci_noop,
	.upm_done =	ehci_root_intr_done,
};

Static const struct usbd_pipe_methods ehci_device_ctrl_methods = {
	.upm_init =	ehci_device_ctrl_init,
	.upm_fini =	ehci_device_ctrl_fini,
	.upm_transfer =	ehci_device_ctrl_transfer,
	.upm_start =	ehci_device_ctrl_start,
	.upm_abort =	ehci_device_ctrl_abort,
	.upm_close =	ehci_device_ctrl_close,
	.upm_cleartoggle =	ehci_noop,
	.upm_done =	ehci_device_ctrl_done,
};

Static const struct usbd_pipe_methods ehci_device_intr_methods = {
	.upm_init =	ehci_device_intr_init,
	.upm_fini =	ehci_device_intr_fini,
	.upm_transfer =	ehci_device_intr_transfer,
	.upm_start =	ehci_device_intr_start,
	.upm_abort =	ehci_device_intr_abort,
	.upm_close =	ehci_device_intr_close,
	.upm_cleartoggle =	ehci_device_clear_toggle,
	.upm_done =	ehci_device_intr_done,
};

Static const struct usbd_pipe_methods ehci_device_bulk_methods = {
	.upm_init =	ehci_device_bulk_init,
	.upm_fini =	ehci_device_bulk_fini,
	.upm_transfer =	ehci_device_bulk_transfer,
	.upm_start =	ehci_device_bulk_start,
	.upm_abort =	ehci_device_bulk_abort,
	.upm_close =	ehci_device_bulk_close,
	.upm_cleartoggle =	ehci_device_clear_toggle,
	.upm_done =	ehci_device_bulk_done,
};

Static const struct usbd_pipe_methods ehci_device_isoc_methods = {
	.upm_init =	ehci_device_isoc_init,
	.upm_fini =	ehci_device_isoc_fini,
	.upm_transfer =	ehci_device_isoc_transfer,
	.upm_abort =	ehci_device_isoc_abort,
	.upm_close =	ehci_device_isoc_close,
	.upm_cleartoggle =	ehci_noop,
	.upm_done =	ehci_device_isoc_done,
};

Static const struct usbd_pipe_methods ehci_device_fs_isoc_methods = {
	.upm_init =	ehci_device_fs_isoc_init,
	.upm_fini =	ehci_device_fs_isoc_fini,
	.upm_transfer =	ehci_device_fs_isoc_transfer,
	.upm_abort =	ehci_device_fs_isoc_abort,
	.upm_close =	ehci_device_fs_isoc_close,
	.upm_cleartoggle = ehci_noop,
	.upm_done =	ehci_device_fs_isoc_done,
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

int
ehci_init(ehci_softc_t *sc)
{
	uint32_t vers, hcr;
	u_int i;
	int err;
	ehci_soft_qh_t *sqh;
	u_int ncomp;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();
#ifdef EHCI_DEBUG
	theehci = sc;
#endif

	mutex_init(&sc->sc_rhlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);
	cv_init(&sc->sc_doorbell, "ehcidb");

	sc->sc_xferpool = pool_cache_init(sizeof(struct ehci_xfer), 0, 0, 0,
	    "ehcixfer", NULL, IPL_USB, NULL, NULL, NULL);

	sc->sc_doorbell_si = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    ehci_doorbell, sc);
	KASSERT(sc->sc_doorbell_si != NULL);
	sc->sc_pcd_si = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    ehci_pcd, sc);
	KASSERT(sc->sc_pcd_si != NULL);

	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);

	vers = EREAD2(sc, EHCI_HCIVERSION);
	aprint_verbose("%s: EHCI version %x.%x\n", device_xname(sc->sc_dev),
	    vers >> 8, vers & 0xff);

	const uint32_t hcsparams = EREAD4(sc, EHCI_HCSPARAMS);
	DPRINTF("hcsparams=%#jx", hcsparams, 0, 0, 0);
	sc->sc_npcomp = EHCI_HCS_N_PCC(hcsparams);
	ncomp = EHCI_HCS_N_CC(hcsparams);
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
		aprint_normal_dev(sc->sc_dev,
		    "%d companion controller%s, %d port%s%s",
		    sc->sc_ncomp,
		    sc->sc_ncomp!=1 ? "s" : "",
		    EHCI_HCS_N_PCC(hcsparams),
		    EHCI_HCS_N_PCC(hcsparams)!=1 ? "s" : "",
		    sc->sc_ncomp!=1 ? " each" : "");
		if (sc->sc_comps[0]) {
			aprint_normal(":");
			for (i = 0; i < sc->sc_ncomp; i++)
				aprint_normal(" %s",
				    device_xname(sc->sc_comps[i]));
		}
		aprint_normal("\n");

		mutex_init(&sc->sc_complock, MUTEX_DEFAULT, IPL_USB);
		callout_init(&sc->sc_compcallout, CALLOUT_MPSAFE);
		cv_init(&sc->sc_compcv, "ehciccv");
		sc->sc_comp_state = CO_EARLY;
	}
	sc->sc_noport = EHCI_HCS_N_PORTS(hcsparams);
	sc->sc_hasppc = EHCI_HCS_PPC(hcsparams);

	const uint32_t hccparams = EREAD4(sc, EHCI_HCCPARAMS);
	DPRINTF("hccparams=%#jx", hccparams, 0, 0, 0);

	if (EHCI_HCC_64BIT(hccparams)) {
		/* MUST clear segment register if 64 bit capable. */
		EOWRITE4(sc, EHCI_CTRLDSSEGMENT, 0);
	}

	if (hccparams & EHCI_HCC_IST_FULLFRAME) {
		sc->sc_istthreshold = 0;
	} else {
		sc->sc_istthreshold = EHCI_HCC_GET_IST_THRESHOLD(hccparams);
	}

	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_usedma = true;
	sc->sc_bus.ub_dmaflags = USBMALLOC_MULTISEG;

	/*
	 * The bus attachment code will possibly provide a 64bit DMA
	 * tag which we now limit to the bottom 4G range as
	 *
	 * - that's as much as ehci can address in its QH, TD, iTD, and siTD
	 *   structures; and
	 * - the driver doesn't currently set EHCI_CTRLDSSEGMENT to anything
	 *   other than 0.
	 */
	bus_dma_tag_t ntag = sc->sc_bus.ub_dmatag;
	sc->sc_dmatag = sc->sc_bus.ub_dmatag;
	err = bus_dmatag_subregion(sc->sc_bus.ub_dmatag, 0, UINT32_MAX,
	    &ntag, 0);
	if (err == 0) {
		sc->sc_dmatag = ntag;
		aprint_normal_dev(sc->sc_dev, "Using DMA subregion for control"
		    " data structures\n");
	}

	/* Reset the controller */
	DPRINTF("resetting", 0, 0, 0, 0);
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
		aprint_error_dev(sc->sc_dev, "reset timeout\n");
		err = EIO;
		goto fail1;
	}
	if (sc->sc_vendor_init)
		sc->sc_vendor_init(sc);

	/* XXX need proper intr scheduling */
	sc->sc_rand = 96;

	/* frame list size at default, read back what we got and use that */
	switch (EHCI_CMD_FLS(EOREAD4(sc, EHCI_USBCMD))) {
	case 0: sc->sc_flsize = 1024; break;
	case 1: sc->sc_flsize = 512; break;
	case 2: sc->sc_flsize = 256; break;
	case 3:
		err = EIO;
		goto fail1;
	}
	err = usb_allocmem(sc->sc_dmatag,
	    sc->sc_flsize * sizeof(ehci_link_t),
	    EHCI_FLALIGN_ALIGN, USBMALLOC_COHERENT, &sc->sc_fldma);
	if (err) {
		aprint_error_dev(sc->sc_dev, "failed to allocate frame list\n");
		goto fail1;
	}
	DPRINTF("flsize=%jd", sc->sc_flsize, 0, 0, 0);
	sc->sc_flist = KERNADDR(&sc->sc_fldma, 0);

	for (i = 0; i < sc->sc_flsize; i++) {
		sc->sc_flist[i] = EHCI_NULL;
	}

	const bus_addr_t flba = DMAADDR(&sc->sc_fldma, 0);
	const uint32_t hi32 = BUS_ADDR_HI32(flba);
	if (hi32 != 0) {
		aprint_error_dev(sc->sc_dev, "DMA memory segment error (%08x)\n",
		    hi32);
		goto fail2;
	}

	const uint32_t lo32 = BUS_ADDR_LO32(flba);
	EOWRITE4(sc, EHCI_PERIODICLISTBASE, lo32);

	sc->sc_softitds = kmem_zalloc(sc->sc_flsize * sizeof(ehci_soft_itd_t *),
	    KM_SLEEP);
	LIST_INIT(&sc->sc_freeitds);
	LIST_INIT(&sc->sc_freesitds);
	TAILQ_INIT(&sc->sc_intrhead);

	/* Set up the bus struct. */
	sc->sc_bus.ub_methods = &ehci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct ehci_pipe);

	sc->sc_eintrs = EHCI_NORMAL_INTRS;

	/*
	 * Allocate the interrupt dummy QHs. These are arranged to give poll
	 * intervals that are powers of 2 times 1ms.
	 */
	memset(sc->sc_islots, 0, sizeof(sc->sc_islots));
	for (i = 0; i < EHCI_INTRQHS; i++) {
		sqh = ehci_alloc_sqh(sc);
		if (sqh == NULL) {
			err = ENOMEM;
			goto fail3;
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
		sqh->qh.qh_endphub = htole32(EHCI_QH_SET_MULT(1));
		sqh->qh.qh_curqtd = EHCI_NULL;
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
		err = ENOMEM;
		goto fail3;
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
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
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
		err = EIO;
		goto fail4;
	}

	/* Enable interrupts */
	DPRINTF("enabling interrupts", 0, 0, 0, 0);
	EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs);

	return 0;

fail4:
	ehci_free_sqh(sc, sc->sc_async_head);

fail3:
	for (i = 0; i < EHCI_INTRQHS; i++) {
		sqh = sc->sc_islots[i].sqh;
		if (sqh)
			ehci_free_sqh(sc, sqh);
	}

	kmem_free(sc->sc_softitds, sc->sc_flsize * sizeof(ehci_soft_itd_t *));

fail2:
	usb_freemem(&sc->sc_fldma);

fail1:
	softint_disestablish(sc->sc_doorbell_si);
	softint_disestablish(sc->sc_pcd_si);
	mutex_destroy(&sc->sc_rhlock);
	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);

	return err;
}

int
ehci_intr(void *v)
{
	ehci_softc_t *sc = v;
	int ret = 0;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	if (sc == NULL)
		return 0;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	/* If we get an interrupt while polling, then just ignore it. */
	if (sc->sc_bus.ub_usepolling) {
		uint32_t intrs = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));

		if (intrs)
			EOWRITE4(sc, EHCI_USBSTS, intrs); /* Acknowledge */
		DPRINTFN(16, "ignored interrupt while polling", 0, 0, 0, 0);
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
	uint32_t intrs, eintrs;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	/* In case the interrupt occurs before initialization has completed. */
	if (sc == NULL) {
#ifdef DIAGNOSTIC
		printf("ehci_intr1: sc == NULL\n");
#endif
		return 0;
	}

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	intrs = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));
	if (!intrs)
		return 0;

	eintrs = intrs & sc->sc_eintrs;
	DPRINTF("sc=%#jx intrs=%#jx(%#jx) eintrs=%#jx", (uintptr_t)sc, intrs,
	    EOREAD4(sc, EHCI_USBSTS), eintrs);
	if (!eintrs)
		return 0;

	EOWRITE4(sc, EHCI_USBSTS, intrs); /* Acknowledge */
	if (eintrs & EHCI_STS_IAA) {
		DPRINTF("door bell", 0, 0, 0, 0);
		kpreempt_disable();
		KASSERT(sc->sc_doorbell_si != NULL);
		softint_schedule(sc->sc_doorbell_si);
		kpreempt_enable();
		eintrs &= ~EHCI_STS_IAA;
	}
	if (eintrs & (EHCI_STS_INT | EHCI_STS_ERRINT)) {
		DPRINTF("INT=%jd  ERRINT=%jd",
		    eintrs & EHCI_STS_INT ? 1 : 0,
		    eintrs & EHCI_STS_ERRINT ? 1 : 0, 0, 0);
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
		KASSERT(sc->sc_pcd_si != NULL);
		softint_schedule(sc->sc_pcd_si);
		kpreempt_enable();
		eintrs &= ~EHCI_STS_PCD;
	}

	if (eintrs != 0) {
		/* Block unprocessed interrupts. */
		sc->sc_eintrs &= ~eintrs;
		EOWRITE4(sc, EHCI_USBINTR, sc->sc_eintrs);
		printf("%s: blocking intrs %#x\n",
		       device_xname(sc->sc_dev), eintrs);
	}

	return 1;
}

Static void
ehci_doorbell(void *addr)
{
	ehci_softc_t *sc = addr;
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_doorbelllwp == NULL)
		DPRINTF("spurious doorbell interrupt", 0, 0, 0, 0);
	sc->sc_doorbelllwp = NULL;
	cv_signal(&sc->sc_doorbell);
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_pcd(void *addr)
{
	ehci_softc_t *sc = addr;
	struct usbd_xfer *xfer;
	u_char *p;
	int i, m;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	xfer = sc->sc_intrxfer;

	if (xfer == NULL) {
		/* Just ignore the change. */
		goto done;
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	p = xfer->ux_buf;
	m = uimin(sc->sc_noport, xfer->ux_length * 8 - 1);
	memset(p, 0, xfer->ux_length);
	for (i = 1; i <= m; i++) {
		/* Pick out CHANGE bits from the status reg. */
		if (EOREAD4(sc, EHCI_PORTSC(i)) & EHCI_PS_CLEAR)
			p[i/8] |= 1 << (i%8);
		if (i % 8 == 7)
			DPRINTF("change(%jd)=0x%02jx", i / 8, p[i/8], 0, 0);
	}
	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);

done:
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_softintr(void *v)
{
	struct usbd_bus *bus = v;
	ehci_softc_t *sc = EHCI_BUS2SC(bus);
	struct ehci_xfer *ex, *nextex;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	ex_completeq_t cq;
	TAILQ_INIT(&cq);

	/*
	 * The only explanation I can think of for why EHCI is as brain dead
	 * as UHCI interrupt-wise is that Intel was involved in both.
	 * An interrupt just tells us that something is done, we have no
	 * clue what, so we need to scan through all active transfers. :-(
	 */

	/*
	 * ehci_idone will remove transfer from sc->sc_intrhead if it's
	 * complete and add to our cq list
	 *
	 */
	TAILQ_FOREACH_SAFE(ex, &sc->sc_intrhead, ex_next, nextex) {
		switch (ex->ex_type) {
		case EX_CTRL:
		case EX_BULK:
		case EX_INTR:
			ehci_check_qh_intr(sc, ex, &cq);
			break;
		case EX_ISOC:
			ehci_check_itd_intr(sc, ex, &cq);
			break;
		case EX_FS_ISOC:
			ehci_check_sitd_intr(sc, ex, &cq);
			break;
		default:
			KASSERT(false);
		}

	}

	/*
	 * We abuse ex_next for the interrupt and complete lists and
	 * interrupt transfers will get re-added here so use
	 * the _SAFE version of TAILQ_FOREACH.
	 */
	TAILQ_FOREACH_SAFE(ex, &cq, ex_next, nextex) {
		usb_transfer_complete(&ex->ex_xfer);
	}

	/* Schedule a callout to catch any dropped transactions. */
	if ((sc->sc_flags & EHCIF_DROPPED_INTR_WORKAROUND) &&
	    !TAILQ_EMPTY(&sc->sc_intrhead))
		callout_reset(&sc->sc_tmo_intrlist,
		    hz, ehci_intrlist_timeout, sc);
}

Static void
ehci_check_qh_intr(ehci_softc_t *sc, struct ehci_xfer *ex, ex_completeq_t *cq)
{
	ehci_soft_qtd_t *sqtd, *fsqtd, *lsqtd;
	uint32_t status;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (ex->ex_type == EX_CTRL) {
		fsqtd = ex->ex_setup;
		lsqtd = ex->ex_status;
	} else {
		fsqtd = ex->ex_sqtdstart;
		lsqtd = ex->ex_sqtdend;
	}
	KASSERTMSG(fsqtd != NULL && lsqtd != NULL,
	    "xfer %p xt %d fsqtd %p lsqtd %p", ex, ex->ex_type, fsqtd, lsqtd);

	/*
	 * If the last TD is still active we need to check whether there
	 * is an error somewhere in the middle, or whether there was a
	 * short packet (SPD and not ACTIVE).
	 */
	usb_syncmem(&lsqtd->dma,
	    lsqtd->offs + offsetof(ehci_qtd_t, qtd_status),
	    sizeof(lsqtd->qtd.qtd_status),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	status = le32toh(lsqtd->qtd.qtd_status);
	usb_syncmem(&lsqtd->dma,
	    lsqtd->offs + offsetof(ehci_qtd_t, qtd_status),
	    sizeof(lsqtd->qtd.qtd_status), BUS_DMASYNC_PREREAD);
	if (status & EHCI_QTD_ACTIVE) {
		DPRINTFN(10, "active ex=%#jx", (uintptr_t)ex, 0, 0, 0);

		/* last qTD has already been checked */
		for (sqtd = fsqtd; sqtd != lsqtd; sqtd = sqtd->nextqtd) {
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
			/* Handle short packets */
			if (EHCI_QTD_GET_BYTES(status) != 0) {
				/*
				 * If we get here for a control transfer then
				 * we need to let the hardware complete the
				 * status phase.  That is, we're not done
				 * quite yet.
				 *
				 * Otherwise, we're done.
				 */
				if (ex->ex_type == EX_CTRL) {
					break;
				}
				goto done;
			}
		}
		DPRINTFN(10, "ex=%#jx std=%#jx still active",
		    (uintptr_t)ex, (uintptr_t)ex->ex_sqtdstart, 0, 0);
#ifdef EHCI_DEBUG
		DPRINTFN(5, "--- still active start ---", 0, 0, 0, 0);
		ehci_dump_sqtds(ex->ex_sqtdstart);
		DPRINTFN(5, "--- still active end ---", 0, 0, 0, 0);
#endif
		return;
	}
 done:
	DPRINTFN(10, "ex=%#jx done", (uintptr_t)ex, 0, 0, 0);
	ehci_idone(ex, cq);
}

Static void
ehci_check_itd_intr(ehci_softc_t *sc, struct ehci_xfer *ex, ex_completeq_t *cq)
{
	ehci_soft_itd_t *itd;
	int i;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	if (&ex->ex_xfer != SIMPLEQ_FIRST(&ex->ex_xfer.ux_pipe->up_queue))
		return;

	KASSERTMSG(ex->ex_itdstart != NULL && ex->ex_itdend != NULL,
	    "xfer %p fitd %p litd %p", ex, ex->ex_itdstart, ex->ex_itdend);

	itd = ex->ex_itdend;

	/*
	 * check no active transfers in last itd, meaning we're finished
	 */

	usb_syncmem(&itd->dma, itd->offs + offsetof(ehci_itd_t, itd_ctl),
	    sizeof(itd->itd.itd_ctl),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	for (i = 0; i < EHCI_ITD_NUFRAMES; i++) {
		if (le32toh(itd->itd.itd_ctl[i]) & EHCI_ITD_ACTIVE)
			break;
	}

	if (i == EHCI_ITD_NUFRAMES) {
		goto done; /* All 8 descriptors inactive, it's done */
	}

	usb_syncmem(&itd->dma, itd->offs + offsetof(ehci_itd_t, itd_ctl),
	    sizeof(itd->itd.itd_ctl), BUS_DMASYNC_PREREAD);

	DPRINTFN(10, "ex %#jx itd %#jx still active",
	    (uintptr_t)ex, (uintptr_t)ex->ex_itdstart, 0, 0);
	return;
done:
	DPRINTF("ex %#jx done", (uintptr_t)ex, 0, 0, 0);
	ehci_idone(ex, cq);
}

void
ehci_check_sitd_intr(ehci_softc_t *sc, struct ehci_xfer *ex, ex_completeq_t *cq)
{
	ehci_soft_sitd_t *sitd;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	if (&ex->ex_xfer != SIMPLEQ_FIRST(&ex->ex_xfer.ux_pipe->up_queue))
		return;

	KASSERTMSG(ex->ex_sitdstart != NULL && ex->ex_sitdend != NULL,
	    "xfer %p fsitd %p lsitd %p", ex, ex->ex_sitdstart, ex->ex_sitdend);

	sitd = ex->ex_sitdend;

	/*
	 * check no active transfers in last sitd, meaning we're finished
	 */

	usb_syncmem(&sitd->dma, sitd->offs + offsetof(ehci_sitd_t, sitd_trans),
	    sizeof(sitd->sitd.sitd_trans),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	bool active = ((le32toh(sitd->sitd.sitd_trans) & EHCI_SITD_ACTIVE) != 0);

	usb_syncmem(&sitd->dma, sitd->offs + offsetof(ehci_sitd_t, sitd_trans),
	    sizeof(sitd->sitd.sitd_trans), BUS_DMASYNC_PREREAD);

	if (active)
		return;

	DPRINTFN(10, "ex=%#jx done", (uintptr_t)ex, 0, 0, 0);
	ehci_idone(ex, cq);
}

Static void
ehci_idone(struct ehci_xfer *ex, ex_completeq_t *cq)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	struct usbd_xfer *xfer = &ex->ex_xfer;
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_softc *sc = EHCI_XFER2SC(xfer);
	ehci_soft_qtd_t *sqtd, *fsqtd, *lsqtd;
	uint32_t status = 0, nstatus = 0;
	int actlen = 0;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	DPRINTF("ex=%#jx", (uintptr_t)ex, 0, 0, 0);

	/*
	 * Try to claim this xfer for completion.  If it has already
	 * completed or aborted, drop it on the floor.
	 */
	if (!usbd_xfer_trycomplete(xfer))
		return;

#ifdef DIAGNOSTIC
#ifdef EHCI_DEBUG
	if (ex->ex_isdone) {
		DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
		ehci_dump_exfer(ex);
		DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
	}
#endif
	KASSERTMSG(!ex->ex_isdone, "xfer %p type %d status %d", xfer,
	    ex->ex_type, xfer->ux_status);
	ex->ex_isdone = true;
#endif

	DPRINTF("xfer=%#jx, pipe=%#jx ready", (uintptr_t)xfer,
	    (uintptr_t)epipe, 0, 0);

	/* The transfer is done, compute actual length and status. */
	if (ex->ex_type == EX_ISOC) {
		/* HS isoc transfer */

		struct ehci_soft_itd *itd;
		int i, nframes, len, uframes;

		nframes = 0;

#ifdef EHCI_DEBUG
		DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
		ehci_dump_itds(ex->ex_itdstart);
		DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

		i = xfer->ux_pipe->up_endpoint->ue_edesc->bInterval;
		uframes = uimin(1 << (i - 1), USB_UFRAMES_PER_FRAME);

		for (itd = ex->ex_itdstart; itd != NULL; itd = itd->xfer_next) {
			usb_syncmem(&itd->dma,
			    itd->offs + offsetof(ehci_itd_t,itd_ctl),
			    sizeof(itd->itd.itd_ctl),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

			for (i = 0; i < EHCI_ITD_NUFRAMES; i += uframes) {
				/*
				 * XXX - driver didn't fill in the frame full
				 *   of uframes. This leads to scheduling
				 *   inefficiencies, but working around
				 *   this doubles complexity of tracking
				 *   an xfer.
				 */
				if (nframes >= xfer->ux_nframes)
					break;

				status = le32toh(itd->itd.itd_ctl[i]);
				len = EHCI_ITD_GET_LEN(status);
				if (EHCI_ITD_GET_STATUS(status) != 0)
					len = 0; /*No valid data on error*/

				xfer->ux_frlengths[nframes++] = len;
				actlen += len;
			}
			usb_syncmem(&itd->dma,
			    itd->offs + offsetof(ehci_itd_t,itd_ctl),
			    sizeof(itd->itd.itd_ctl), BUS_DMASYNC_PREREAD);

			if (nframes >= xfer->ux_nframes)
				break;
		}

		xfer->ux_actlen = actlen;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		goto end;
	} else if (ex->ex_type == EX_FS_ISOC) {
		/* FS isoc transfer */
		struct ehci_soft_sitd *sitd;
		int nframes, len;

		nframes = 0;

		for (sitd = ex->ex_sitdstart; sitd != NULL;
		     sitd = sitd->xfer_next) {
			usb_syncmem(&sitd->dma,
			    sitd->offs + offsetof(ehci_sitd_t, sitd_trans),
			    sizeof(sitd->sitd.sitd_trans),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

			/*
			 * XXX - driver didn't fill in the frame full
			 *   of uframes. This leads to scheduling
			 *   inefficiencies, but working around
			 *   this doubles complexity of tracking
			 *   an xfer.
			 */
			if (nframes >= xfer->ux_nframes)
				break;

			status = le32toh(sitd->sitd.sitd_trans);
			usb_syncmem(&sitd->dma,
			    sitd->offs + offsetof(ehci_sitd_t, sitd_trans),
			    sizeof(sitd->sitd.sitd_trans), BUS_DMASYNC_PREREAD);

			len = EHCI_SITD_GET_LEN(status);
			if (status & (EHCI_SITD_ERR|EHCI_SITD_BUFERR|
			    EHCI_SITD_BABBLE|EHCI_SITD_XACTERR|EHCI_SITD_MISS)) {
				/* No valid data on error */
				len = xfer->ux_frlengths[nframes];
			}

			/*
			 * frlengths[i]: # of bytes to send
			 * len: # of bytes host didn't send
			 */
			xfer->ux_frlengths[nframes] -= len;
			/* frlengths[i]: # of bytes host sent */
			actlen += xfer->ux_frlengths[nframes++];

			if (nframes >= xfer->ux_nframes)
				break;
	    	}

		xfer->ux_actlen = actlen;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		goto end;
	}
	KASSERT(ex->ex_type == EX_CTRL || ex->ex_type == EX_INTR ||
	   ex->ex_type == EX_BULK);

	/* Continue processing xfers using queue heads */
	if (ex->ex_type == EX_CTRL) {
		fsqtd = ex->ex_setup;
		lsqtd = ex->ex_status;
	} else {
		fsqtd = ex->ex_sqtdstart;
		lsqtd = ex->ex_sqtdend;
	}
#ifdef EHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqtds(fsqtd);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	for (sqtd = fsqtd; sqtd != lsqtd->nextqtd; sqtd = sqtd->nextqtd) {
		usb_syncmem(&sqtd->dma, sqtd->offs, sizeof(sqtd->qtd),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		nstatus = le32toh(sqtd->qtd.qtd_status);
		usb_syncmem(&sqtd->dma, sqtd->offs, sizeof(sqtd->qtd),
		    BUS_DMASYNC_PREREAD);
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
	    xfer->ux_pipe->up_dev->ud_pipe0 != xfer->ux_pipe) {
		DPRINTF("toggle update status=0x%08jx nstatus=0x%08jx",
		    status, nstatus, 0, 0);
#if 0
		ehci_dump_sqh(epipe->sqh);
		ehci_dump_sqtds(ex->ex_sqtdstart);
#endif
		epipe->nexttoggle = EHCI_QTD_GET_TOGGLE(nstatus);
	}

	DPRINTF("len=%jd actlen=%jd status=0x%08jx", xfer->ux_length, actlen,
	    status, 0);
	xfer->ux_actlen = actlen;
	if (status & EHCI_QTD_HALTED) {
#ifdef EHCI_DEBUG
		DPRINTF("halted addr=%jd endpt=0x%02jx",
		    xfer->ux_pipe->up_dev->ud_addr,
		    xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress,
		    0, 0);
		DPRINTF("cerr=%jd pid=%jd",
		    EHCI_QTD_GET_CERR(status), EHCI_QTD_GET_PID(status),
		    0, 0);
		DPRINTF("active =%jd halted=%jd buferr=%jd babble=%jd",
		    status & EHCI_QTD_ACTIVE ? 1 : 0,
		    status & EHCI_QTD_HALTED ? 1 : 0,
		    status & EHCI_QTD_BUFERR ? 1 : 0,
		    status & EHCI_QTD_BABBLE ? 1 : 0);

		DPRINTF("xacterr=%jd missed=%jd split =%jd ping  =%jd",
		    status & EHCI_QTD_XACTERR ? 1 : 0,
		    status & EHCI_QTD_MISSEDMICRO ? 1 : 0,
		    status & EHCI_QTD_SPLITXSTATE ? 1 : 0,
		    status & EHCI_QTD_PINGSTATE ? 1 : 0);

		DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
		ehci_dump_sqh(epipe->sqh);
		ehci_dump_sqtds(ex->ex_sqtdstart);
		DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif
		/* low&full speed has an extra error flag */
		if (EHCI_QH_GET_EPS(epipe->sqh->qh.qh_endp) !=
		    EHCI_QH_SPEED_HIGH)
			status &= EHCI_QTD_STATERRS | EHCI_QTD_PINGSTATE;
		else
			status &= EHCI_QTD_STATERRS;
		if (status == 0) /* no other errors means a stall */ {
			xfer->ux_status = USBD_STALLED;
		} else {
			xfer->ux_status = USBD_IOERROR; /* more info XXX */
		}
		/* XXX need to reset TT on missed microframe */
		if (status & EHCI_QTD_MISSEDMICRO) {
			printf("%s: missed microframe, TT reset not "
			    "implemented, hub might be inoperational\n",
			    device_xname(sc->sc_dev));
		}
	} else {
		xfer->ux_status = USBD_NORMAL_COMPLETION;
	}

    end:

	ehci_del_intr_list(sc, ex);
	TAILQ_INSERT_TAIL(cq, ex, ex_next);

	DPRINTF("ex=%#jx done", (uintptr_t)ex, 0, 0, 0);
}

Static void
ehci_poll(struct usbd_bus *bus)
{
	ehci_softc_t *sc = EHCI_BUS2SC(bus);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

#ifdef EHCI_DEBUG
	static int last;
	int new;
	new = EHCI_STS_INTRS(EOREAD4(sc, EHCI_USBSTS));
	if (new != last) {
		DPRINTF("intrs=0x%04jx", new, 0, 0, 0);
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
	int rv = 0;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	if (sc->sc_child != NULL) {
		rv = config_detach(sc->sc_child, flags);
		if (rv != 0)
			return rv;
	}

	if (sc->sc_ncomp > 0) {
		mutex_enter(&sc->sc_complock);
		/* XXX try to halt callout instead of waiting */
		while (sc->sc_comp_state == CO_SCHED)
			cv_wait(&sc->sc_compcv, &sc->sc_complock);
		mutex_exit(&sc->sc_complock);

		callout_halt(&sc->sc_compcallout, NULL);
		callout_destroy(&sc->sc_compcallout);
		cv_destroy(&sc->sc_compcv);
		mutex_destroy(&sc->sc_complock);
	}

	callout_halt(&sc->sc_tmo_intrlist, NULL);
	callout_destroy(&sc->sc_tmo_intrlist);

	/* XXX free other data structures */
	if (sc->sc_softitds) {
		kmem_free(sc->sc_softitds,
		    sc->sc_flsize * sizeof(ehci_soft_itd_t *));
	}
	cv_destroy(&sc->sc_doorbell);

#if 0
	/* XXX destroyed in ehci_pci.c as it controls ehci_intr access */
	softint_disestablish(sc->sc_doorbell_si);
	softint_disestablish(sc->sc_pcd_si);
	mutex_destroy(&sc->sc_rhlock);
	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);
#endif

	pool_cache_destroy(sc->sc_xferpool);

	EOWRITE4(sc, EHCI_CONFIGFLAG, 0);

	return rv;
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
 * Note that this power handler isn't to be registered directly; the
 * bus glue needs to call out to it.
 */
bool
ehci_suspend(device_t dv, const pmf_qual_t *qual)
{
	ehci_softc_t *sc = device_private(dv);
	int i;
	uint32_t cmd, hcr;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_rhlock);

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

	mutex_exit(&sc->sc_rhlock);

	return true;
}

bool
ehci_resume(device_t dv, const pmf_qual_t *qual)
{
	ehci_softc_t *sc = device_private(dv);
	int i;
	uint32_t cmd, hcr;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_rhlock);

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

	mutex_exit(&sc->sc_rhlock);

	return true;
}

/*
 * Shut down the controller when the system is going down.
 */
bool
ehci_shutdown(device_t self, int flags)
{
	ehci_softc_t *sc = device_private(self);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	EOWRITE4(sc, EHCI_USBCMD, 0);	/* Halt controller */
	EOWRITE4(sc, EHCI_USBCMD, EHCI_CMD_HCRESET);
	return true;
}

Static struct usbd_xfer *
ehci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct ehci_softc *sc = EHCI_BUS2SC(bus);
	struct usbd_xfer *xfer;

	xfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct ehci_xfer));

#ifdef DIAGNOSTIC
		struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);
		ex->ex_isdone = true;
		xfer->ux_state = XFER_BUSY;
#endif
	}
	return xfer;
}

Static void
ehci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct ehci_softc *sc = EHCI_BUS2SC(bus);
	struct ehci_xfer *ex __diagused = EHCI_XFER2EXFER(xfer);

	KASSERTMSG(xfer->ux_state == XFER_BUSY ||
	    xfer->ux_status == USBD_NOT_STARTED,
	    "xfer %p state %d\n", xfer, xfer->ux_state);
	KASSERT(ex->ex_isdone || xfer->ux_status == USBD_NOT_STARTED);

#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_FREE;
#endif

	pool_cache_put(sc->sc_xferpool, xfer);
}

Static bool
ehci_dying(struct usbd_bus *bus)
{
	struct ehci_softc *sc = EHCI_BUS2SC(bus);

	return sc->sc_dying;
}

Static void
ehci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct ehci_softc *sc = EHCI_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

Static void
ehci_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(pipe);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("epipe=%#jx status=0x%08jx", (uintptr_t)epipe,
	    epipe->sqh->qh.qh_qtd.qtd_status, 0, 0);
#ifdef EHCI_DEBUG
	if (ehcidebug)
		usbd_dump_pipe(pipe);
#endif
	epipe->nexttoggle = 0;
}

Static void
ehci_noop(struct usbd_pipe *pipe)
{
}

#ifdef EHCI_DEBUG
/*
 * Unused function - this is meant to be called from a kernel
 * debugger.
 */
void
ehci_dump(void)
{
	ehci_softc_t *sc = theehci;
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

Static void
ehci_dump_regs(ehci_softc_t *sc)
{
	int i;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("cmd     = 0x%08jx  sts      = 0x%08jx  ien      = 0x%08jx",
	    EOREAD4(sc, EHCI_USBCMD), EOREAD4(sc, EHCI_USBSTS),
	    EOREAD4(sc, EHCI_USBINTR), 0);
	DPRINTF("frindex = 0x%08jx  ctrdsegm = 0x%08jx  periodic = 0x%08jx  "
	    "async   = 0x%08jx",
	    EOREAD4(sc, EHCI_FRINDEX), EOREAD4(sc, EHCI_CTRLDSSEGMENT),
	    EOREAD4(sc, EHCI_PERIODICLISTBASE),
	    EOREAD4(sc, EHCI_ASYNCLISTADDR));
	for (i = 1; i <= sc->sc_noport; i += 2) {
		if (i == sc->sc_noport) {
			DPRINTF("port %jd status = 0x%08jx", i,
			    EOREAD4(sc, EHCI_PORTSC(i)), 0, 0);
		} else {
			DPRINTF("port %jd status = 0x%08jx  port %jd "
			    "status = 0x%08jx",
			    i, EOREAD4(sc, EHCI_PORTSC(i)),
			    i+1, EOREAD4(sc, EHCI_PORTSC(i+1)));
		}
	}
}

#define ehci_dump_link(link, type) do {					\
	DPRINTF("    link 0x%08jx (T = %jd):",				\
	    link,							\
	    link & EHCI_LINK_TERMINATE ? 1 : 0, 0, 0);			\
	if (type) {							\
		DPRINTF(						\
		    "        ITD  = %jd  QH   = %jd  SITD = %jd  FSTN = %jd",\
		    EHCI_LINK_TYPE(link) == EHCI_LINK_ITD ? 1 : 0,	\
		    EHCI_LINK_TYPE(link) == EHCI_LINK_QH ? 1 : 0,	\
		    EHCI_LINK_TYPE(link) == EHCI_LINK_SITD ? 1 : 0,	\
		    EHCI_LINK_TYPE(link) == EHCI_LINK_FSTN ? 1 : 0);	\
	}								\
} while(0)

Static void
ehci_dump_sqtds(ehci_soft_qtd_t *sqtd)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	int i;
	uint32_t stop = 0;

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
	if (!stop)
		DPRINTF("dump aborted, too many TDs", 0, 0, 0, 0);
}

Static void
ehci_dump_sqtd(ehci_soft_qtd_t *sqtd)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	usb_syncmem(&sqtd->dma, sqtd->offs,
	    sizeof(sqtd->qtd), BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	DPRINTFN(10, "QTD(%#jx) at 0x%08jx:", (uintptr_t)sqtd, sqtd->physaddr,
	    0, 0);
	ehci_dump_qtd(&sqtd->qtd);

	usb_syncmem(&sqtd->dma, sqtd->offs,
	    sizeof(sqtd->qtd), BUS_DMASYNC_PREREAD);
}

Static void
ehci_dump_qtd(ehci_qtd_t *qtd)
{
	EHCIHIST_FUNC();	EHCIHIST_CALLED();
	uint32_t s = le32toh(qtd->qtd_status);

	DPRINTFN(10,
	    "     next = 0x%08jx  altnext = 0x%08jx  status = 0x%08jx",
	    qtd->qtd_next, qtd->qtd_altnext, s, 0);
	DPRINTFN(10,
	    "   toggle = %jd ioc = %jd bytes = %#jx c_page = %#jx",
	    EHCI_QTD_GET_TOGGLE(s), EHCI_QTD_GET_IOC(s),
	    EHCI_QTD_GET_BYTES(s), EHCI_QTD_GET_C_PAGE(s));
	DPRINTFN(10,
	    "     cerr = %jd pid = %jd stat  = %jx",
	    EHCI_QTD_GET_CERR(s), EHCI_QTD_GET_PID(s), EHCI_QTD_GET_STATUS(s),
	    0);
	DPRINTFN(10,
	    "active =%jd halted=%jd buferr=%jd babble=%jd",
	    s & EHCI_QTD_ACTIVE ? 1 : 0,
	    s & EHCI_QTD_HALTED ? 1 : 0,
	    s & EHCI_QTD_BUFERR ? 1 : 0,
	    s & EHCI_QTD_BABBLE ? 1 : 0);
	DPRINTFN(10,
	    "xacterr=%jd missed=%jd split =%jd ping  =%jd",
	    s & EHCI_QTD_XACTERR ? 1 : 0,
	    s & EHCI_QTD_MISSEDMICRO ? 1 : 0,
	    s & EHCI_QTD_SPLITXSTATE ? 1 : 0,
	    s & EHCI_QTD_PINGSTATE ? 1 : 0);
	DPRINTFN(10,
	    "buffer[0] = %#jx  buffer[1] = %#jx  "
	    "buffer[2] = %#jx  buffer[3] = %#jx",
	    le32toh(qtd->qtd_buffer[0]), le32toh(qtd->qtd_buffer[1]),
	    le32toh(qtd->qtd_buffer[2]), le32toh(qtd->qtd_buffer[3]));
	DPRINTFN(10,
	    "buffer[4] = %#jx", le32toh(qtd->qtd_buffer[4]), 0, 0, 0);
}

Static void
ehci_dump_sqh(ehci_soft_qh_t *sqh)
{
	ehci_qh_t *qh = &sqh->qh;
	ehci_link_t link;
	uint32_t endp, endphub;
	EHCIHIST_FUNC();	EHCIHIST_CALLED();

	usb_syncmem(&sqh->dma, sqh->offs,
	    sizeof(sqh->qh), BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	DPRINTFN(10, "QH(%#jx) at %#jx:", (uintptr_t)sqh, sqh->physaddr, 0, 0);
	link = le32toh(qh->qh_link);
	ehci_dump_link(link, true);

	endp = le32toh(qh->qh_endp);
	DPRINTFN(10, "    endp = %#jx", endp, 0, 0, 0);
	DPRINTFN(10, "        addr = 0x%02jx  inact = %jd  endpt = %jd  "
	    "eps = %jd",
	    EHCI_QH_GET_ADDR(endp), EHCI_QH_GET_INACT(endp),
	    EHCI_QH_GET_ENDPT(endp), EHCI_QH_GET_EPS(endp));
	DPRINTFN(10, "        dtc  = %jd     hrecl = %jd",
	    EHCI_QH_GET_DTC(endp), EHCI_QH_GET_HRECL(endp), 0, 0);
	DPRINTFN(10, "        ctl  = %jd     nrl   = %jd  mpl   = %#jx(%jd)",
	    EHCI_QH_GET_CTL(endp),EHCI_QH_GET_NRL(endp),
	    EHCI_QH_GET_MPL(endp), EHCI_QH_GET_MPL(endp));

	endphub = le32toh(qh->qh_endphub);
	DPRINTFN(10, " endphub = %#jx", endphub, 0, 0, 0);
	DPRINTFN(10, "      smask = 0x%02jx  cmask = 0x%02jx one %jx",
	    EHCI_QH_GET_SMASK(endphub), EHCI_QH_GET_CMASK(endphub), 1, 0);
	DPRINTFN(10, "      huba  = 0x%02jx  port  = %jd  mult = %jd",
	    EHCI_QH_GET_HUBA(endphub), EHCI_QH_GET_PORT(endphub),
	    EHCI_QH_GET_MULT(endphub), 0);

	link = le32toh(qh->qh_curqtd);
	ehci_dump_link(link, false);
	DPRINTFN(10, "Overlay qTD:", 0, 0, 0, 0);
	ehci_dump_qtd(&qh->qh_qtd);

	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREREAD);
}

Static void
ehci_dump_itds(ehci_soft_itd_t *itd)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	int i;
	uint32_t stop = 0;

	for (i = 0; itd && i < 20 && !stop; itd = itd->xfer_next, i++) {
		ehci_dump_itd(itd);
		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_next),
		    sizeof(itd->itd),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		stop = itd->itd.itd_next & htole32(EHCI_LINK_TERMINATE);
		usb_syncmem(&itd->dma,
		    itd->offs + offsetof(ehci_itd_t, itd_next),
		    sizeof(itd->itd), BUS_DMASYNC_PREREAD);
	}
	if (!stop)
		DPRINTF("dump aborted, too many TDs", 0, 0, 0, 0);
}

Static void
ehci_dump_itd(struct ehci_soft_itd *itd)
{
	ehci_isoc_trans_t t;
	ehci_isoc_bufr_ptr_t b, b2, b3;
	int i;

	EHCIHIST_FUNC();	EHCIHIST_CALLED();

	DPRINTF("ITD: next phys = %#jx", itd->itd.itd_next, 0, 0, 0);

	for (i = 0; i < EHCI_ITD_NUFRAMES; i++) {
		t = le32toh(itd->itd.itd_ctl[i]);
		DPRINTF("ITDctl %jd: stat = %jx len = %jx",
		    i, EHCI_ITD_GET_STATUS(t), EHCI_ITD_GET_LEN(t), 0);
		DPRINTF("     ioc = %jx pg = %jx offs = %jx",
		    EHCI_ITD_GET_IOC(t), EHCI_ITD_GET_PG(t),
		    EHCI_ITD_GET_OFFS(t), 0);
	}
	DPRINTF("ITDbufr: ", 0, 0, 0, 0);
	for (i = 0; i < EHCI_ITD_NBUFFERS; i++)
		DPRINTF("      %jx",
		    EHCI_ITD_GET_BPTR(le32toh(itd->itd.itd_bufr[i])), 0, 0, 0);

	b = le32toh(itd->itd.itd_bufr[0]);
	b2 = le32toh(itd->itd.itd_bufr[1]);
	b3 = le32toh(itd->itd.itd_bufr[2]);
	DPRINTF("     ep = %jx daddr = %jx dir = %jd",
	    EHCI_ITD_GET_EP(b), EHCI_ITD_GET_DADDR(b), EHCI_ITD_GET_DIR(b2), 0);
	DPRINTF("     maxpkt = %jx multi = %jx",
	    EHCI_ITD_GET_MAXPKT(b2), EHCI_ITD_GET_MULTI(b3), 0, 0);
}

Static void
ehci_dump_sitd(struct ehci_soft_itd *itd)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("SITD %#jx next = %p prev = %#jx",
	    (uintptr_t)itd, (uintptr_t)itd->frame_list.next,
	    (uintptr_t)itd->frame_list.prev, 0);
	DPRINTF("        xfernext=%#jx physaddr=%jX slot=%jd",
	    (uintptr_t)itd->xfer_next, itd->physaddr, itd->slot, 0);
}

Static void
ehci_dump_exfer(struct ehci_xfer *ex)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("ex = %#jx type %jd isdone %jd", (uintptr_t)ex, ex->ex_type,
	    ex->ex_isdone, 0);

	switch (ex->ex_type) {
	case EX_CTRL:
		DPRINTF("   setup = %#jx data = %#jx status = %#jx",
		    (uintptr_t)ex->ex_setup, (uintptr_t)ex->ex_data,
		    (uintptr_t)ex->ex_status, 0);
		break;
	case EX_BULK:
	case EX_INTR:
		DPRINTF("   qtdstart = %#jx qtdend = %#jx",
		    (uintptr_t)ex->ex_sqtdstart, (uintptr_t)ex->ex_sqtdend,
		    0, 0);
		break;
	case EX_ISOC:
		DPRINTF("   itdstart = %#jx itdend = %#jx",
		    (uintptr_t)ex->ex_itdstart, (uintptr_t)ex->ex_itdend, 0, 0);
		break;
	case EX_FS_ISOC:
		DPRINTF("   sitdstart = %#jx sitdend = %#jx",
		    (uintptr_t)ex->ex_sitdstart, (uintptr_t)ex->ex_sitdend,
		    0, 0);
		break;
	default:
		DPRINTF("   unknown type", 0, 0, 0, 0);
	}
}
#endif

Static usbd_status
ehci_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	ehci_softc_t *sc = EHCI_PIPE2SC(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	uint8_t rhaddr = dev->ud_bus->ub_rhaddr;
	uint8_t addr = dev->ud_addr;
	uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(pipe);
	ehci_soft_qh_t *sqh;
	usbd_status err;
	int ival, speed, naks;
	int hshubaddr, hshubport;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("pipe=%#jx, addr=%jd, endpt=%jd (%jd)", (uintptr_t)pipe, addr,
	    ed->bEndpointAddress, rhaddr);

	if (dev->ud_myhsport) {
		/*
		 * When directly attached FS/LS device while doing embedded
		 * transaction translations and we are the hub, set the hub
		 * address to 0 (us).
		 */
		if (!(sc->sc_flags & EHCIF_ETTF)
		    || (dev->ud_myhsport->up_parent->ud_addr != rhaddr)) {
			hshubaddr = dev->ud_myhsport->up_parent->ud_addr;
		} else {
			hshubaddr = 0;
		}
		hshubport = dev->ud_myhsport->up_portno;
	} else {
		hshubaddr = 0;
		hshubport = 0;
	}

	if (sc->sc_dying)
		return USBD_IOERROR;

	/* toggle state needed for bulk endpoints */
	epipe->nexttoggle = pipe->up_endpoint->ue_toggle;

	if (addr == rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &ehci_root_intr_methods;
			break;
		default:
			DPRINTF("bad bEndpointAddress 0x%02jx",
			    ed->bEndpointAddress, 0, 0, 0);
			return USBD_INVAL;
		}
		return USBD_NORMAL_COMPLETION;
	}

	/* XXX All this stuff is only valid for async. */
	switch (dev->ud_speed) {
	case USB_SPEED_LOW:  speed = EHCI_QH_SPEED_LOW;  break;
	case USB_SPEED_FULL: speed = EHCI_QH_SPEED_FULL; break;
	case USB_SPEED_HIGH: speed = EHCI_QH_SPEED_HIGH; break;
	default: panic("ehci_open: bad device speed %d", dev->ud_speed);
	}
	if (speed == EHCI_QH_SPEED_LOW && xfertype == UE_ISOCHRONOUS) {
		DPRINTF("hshubaddr=%jd hshubport=%jd", hshubaddr, hshubport, 0,
		    0);
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
			return USBD_NOMEM;
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
			    (xfertype == UE_INTERRUPT ?
				 EHCI_QH_SET_CMASK(0x08) : 0)
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
		/* we can use 64bit DMA for the reqdma buffer */
		err = usb_allocmem(sc->sc_bus.ub_dmatag,
		    sizeof(usb_device_request_t), 0, USBMALLOC_COHERENT,
		    &epipe->ctrl.reqdma);
#ifdef EHCI_DEBUG
		if (err)
			printf("ehci_open: usb_allocmem()=%d\n", err);
#endif
		if (err)
			goto bad;
		pipe->up_methods = &ehci_device_ctrl_methods;
		mutex_enter(&sc->sc_lock);
		ehci_add_qh(sc, sqh, sc->sc_async_head);
		mutex_exit(&sc->sc_lock);
		break;
	case UE_BULK:
		pipe->up_methods = &ehci_device_bulk_methods;
		mutex_enter(&sc->sc_lock);
		ehci_add_qh(sc, sqh, sc->sc_async_head);
		mutex_exit(&sc->sc_lock);
		break;
	case UE_INTERRUPT:
		pipe->up_methods = &ehci_device_intr_methods;
		ival = pipe->up_interval;
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
		pipe->up_serialise = false;
		if (speed == EHCI_QH_SPEED_HIGH)
			pipe->up_methods = &ehci_device_isoc_methods;
		else
			pipe->up_methods = &ehci_device_fs_isoc_methods;
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
		epipe->isoc.next_frame = 0;
		epipe->isoc.cur_xfers = 0;
		break;
	default:
		DPRINTF("bad xfer type %jd", xfertype, 0, 0, 0);
		err = USBD_INVAL;
		goto bad;
	}
	return USBD_NORMAL_COMPLETION;

 bad:
	if (sqh != NULL) {
		mutex_enter(&sc->sc_lock);
		ehci_free_sqh(sc, sqh);
		mutex_exit(&sc->sc_lock);
	}
	return err;
}

/*
 * Add an ED to the schedule.  Called with USB lock held.
 */
Static void
ehci_add_qh(ehci_softc_t *sc, ehci_soft_qh_t *sqh, ehci_soft_qh_t *head)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

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
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
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
	uint32_t status;

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
 *
 * Releases the bus lock to sleep while waiting for interrupt.
 */
Static void
ehci_sync_hc(ehci_softc_t *sc)
{
	unsigned delta = hz;
	unsigned starttime = getticks();
	unsigned endtime = starttime + delta;
	unsigned now;

	KASSERT(mutex_owned(&sc->sc_lock));

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	if (sc->sc_dying) {
		DPRINTF("dying", 0, 0, 0, 0);
		return;
	}

	/*
	 * Wait until any concurrent ehci_sync_hc has completed so we
	 * have exclusive access to the doorbell.
	 */
	while (sc->sc_doorbelllwp)
		cv_wait(&sc->sc_doorbell, &sc->sc_lock);
	sc->sc_doorbelllwp = curlwp;

	/* ask for doorbell */
	EOWRITE4(sc, EHCI_USBCMD, EOREAD4(sc, EHCI_USBCMD) | EHCI_CMD_IAAD);
	DPRINTF("cmd = 0x%08jx sts = 0x%08jx",
	    EOREAD4(sc, EHCI_USBCMD), EOREAD4(sc, EHCI_USBSTS), 0, 0);

	/*
	 * Wait for the ehci to ring our doorbell.
	 */
	while (sc->sc_doorbelllwp == curlwp) {
		now = getticks();
		if (endtime - now > delta) {
			sc->sc_doorbelllwp = NULL;
			cv_signal(&sc->sc_doorbell);
			DPRINTF("doorbell timeout", 0, 0, 0, 0);
#ifdef DIAGNOSTIC		/* XXX DIAGNOSTIC abuse, do this differently */
			printf("ehci_sync_hc: timed out\n");
#endif
			break;
		}
		(void)cv_timedwait(&sc->sc_doorbell, &sc->sc_lock,
		    endtime - now);
	}

	DPRINTF("cmd = 0x%08jx sts = 0x%08jx ... done",
	    EOREAD4(sc, EHCI_USBCMD), EOREAD4(sc, EHCI_USBSTS), 0, 0);
}

Static void
ehci_remove_itd_chain(ehci_softc_t *sc, struct ehci_soft_itd *itd)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	for (; itd != NULL; itd = itd->xfer_next) {
		struct ehci_soft_itd *prev = itd->frame_list.prev;

		/* Unlink itd from hardware chain, or frame array */
		if (prev == NULL) { /* We're at the table head */
			sc->sc_softitds[itd->slot] = itd->frame_list.next;
			sc->sc_flist[itd->slot] = itd->itd.itd_next;
			usb_syncmem(&sc->sc_fldma,
			    sizeof(ehci_link_t) * itd->slot,
			    sizeof(ehci_link_t),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			if (itd->frame_list.next != NULL)
				itd->frame_list.next->frame_list.prev = NULL;
		} else {
			/* XXX this part is untested... */
			prev->itd.itd_next = itd->itd.itd_next;
			usb_syncmem(&itd->dma,
			    itd->offs + offsetof(ehci_itd_t, itd_next),
			    sizeof(itd->itd.itd_next), BUS_DMASYNC_PREWRITE);

			prev->frame_list.next = itd->frame_list.next;
			if (itd->frame_list.next != NULL)
				itd->frame_list.next->frame_list.prev = prev;
		}
	}
}

Static void
ehci_free_itd_chain(ehci_softc_t *sc, struct ehci_soft_itd *itd)
{
	struct ehci_soft_itd *next;

	mutex_enter(&sc->sc_lock);
	next = NULL;
	for (; itd != NULL; itd = next) {
		next = itd->xfer_next;
		ehci_free_itd_locked(sc, itd);
	}
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_remove_sitd_chain(ehci_softc_t *sc, struct ehci_soft_sitd *sitd)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	for (; sitd != NULL; sitd = sitd->xfer_next) {
		struct ehci_soft_sitd *prev = sitd->frame_list.prev;

		/* Unlink sitd from hardware chain, or frame array */
		if (prev == NULL) { /* We're at the table head */
			sc->sc_softsitds[sitd->slot] = sitd->frame_list.next;
			sc->sc_flist[sitd->slot] = sitd->sitd.sitd_next;
			usb_syncmem(&sc->sc_fldma,
			    sizeof(ehci_link_t) * sitd->slot,
			    sizeof(ehci_link_t),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			if (sitd->frame_list.next != NULL)
				sitd->frame_list.next->frame_list.prev = NULL;
		} else {
			/* XXX this part is untested... */
			prev->sitd.sitd_next = sitd->sitd.sitd_next;
			usb_syncmem(&sitd->dma,
			    sitd->offs + offsetof(ehci_sitd_t, sitd_next),
			    sizeof(sitd->sitd.sitd_next), BUS_DMASYNC_PREWRITE);

			prev->frame_list.next = sitd->frame_list.next;
			if (sitd->frame_list.next != NULL)
				sitd->frame_list.next->frame_list.prev = prev;
		}
	}
}

Static void
ehci_free_sitd_chain(ehci_softc_t *sc, struct ehci_soft_sitd *sitd)
{

	mutex_enter(&sc->sc_lock);
	struct ehci_soft_sitd *next  = NULL;
	for (; sitd != NULL; sitd = next) {
		next = sitd->xfer_next;
		ehci_free_sitd_locked(sc, sitd);
	}
	mutex_exit(&sc->sc_lock);
}

/***********/

static int
ehci_roothub_ctrl_locked(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	ehci_softc_t *sc = EHCI_BUS2SC(bus);
	usb_hub_descriptor_t hubd;
	usb_port_status_t ps;
	uint16_t len, value, index;
	int l, totlen = 0;
	int port, i;
	uint32_t v;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_rhlock));

	if (sc->sc_dying)
		return -1;

	DPRINTF("type=0x%02jx request=%02jx", req->bmRequestType, req->bRequest,
	    0, 0);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		if (len == 0)
			break;
		switch (value) {
#define sd ((usb_string_descriptor_t *)buf)
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "EHCI root hub");
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
		DPRINTF("UR_CLEAR_PORT_FEATURE port=%jd feature=%jd", index,
		    value, 0, 0);
		if (index < 1 || index > sc->sc_noport) {
			return -1;
		}
		port = EHCI_PORTSC(index);
		v = EOREAD4(sc, port);
		DPRINTF("portsc=0x%08jx", v, 0, 0, 0);
		v &= ~EHCI_PS_CLEAR;
		switch (value) {
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
			DPRINTF("clear port test %jd", index, 0, 0, 0);
			break;
		case UHF_PORT_INDICATOR:
			DPRINTF("clear port ind %jd", index, 0, 0, 0);
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
			return -1;
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
			return -1;
		}
		totlen = uimin(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);
		hubd.bNbrPorts = sc->sc_noport;
		v = EREAD4(sc, EHCI_HCSPARAMS);
		USETW(hubd.wHubCharacteristics,
		    (EHCI_HCS_PPC(v) ? UHD_PWR_INDIVIDUAL : UHD_PWR_NO_SWITCH) |
		    (EHCI_HCS_P_INDICATOR(v) ? UHD_PORT_IND : 0));
		hubd.bPwrOn2PwrGood = 200; /* XXX can't find out? */
		for (i = 0, l = sc->sc_noport; l > 0; i++, l -= 8, v >>= 8)
			hubd.DeviceRemovable[i++] = 0; /* XXX can't find out? */
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + i;
		totlen = uimin(totlen, hubd.bDescLength);
		memcpy(buf, &hubd, totlen);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			return -1;
		}
		memset(buf, 0, len); /* ? XXX */
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		DPRINTF("get port status i=%jd", index, 0, 0, 0);
		if (index < 1 || index > sc->sc_noport) {
			return -1;
		}
		if (len != 4) {
			return -1;
		}
		v = EOREAD4(sc, EHCI_PORTSC(index));
		DPRINTF("port status=0x%04jx", v, 0, 0, 0);

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
		totlen = uimin(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		return -1;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index < 1 || index > sc->sc_noport) {
			return -1;
		}
		port = EHCI_PORTSC(index);
		v = EOREAD4(sc, port);
		DPRINTF("portsc=0x%08jx", v, 0, 0, 0);
		v &= ~EHCI_PS_CLEAR;
		switch(value) {
		case UHF_PORT_ENABLE:
			EOWRITE4(sc, port, v | EHCI_PS_PE);
			break;
		case UHF_PORT_SUSPEND:
			EOWRITE4(sc, port, v | EHCI_PS_SUSP);
			break;
		case UHF_PORT_RESET:
			DPRINTF("reset port %jd", index, 0, 0, 0);
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
				return -1;
			}
			/*
			 * An embedded transaction translator will automatically
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
					return -1;
				}
			}

			v = EOREAD4(sc, port);
			DPRINTF("ehci after reset, status=0x%08jx", v, 0, 0, 0);
			if (v & EHCI_PS_PR) {
				printf("%s: port reset timeout\n",
				       device_xname(sc->sc_dev));
				return USBD_TIMEOUT;
			}
			if (!(v & EHCI_PS_PE)) {
				/* Not a high speed device, give up ownership.*/
				ehci_disown(sc, index, 0);
				break;
			}
			sc->sc_isreset[index] = 1;
			DPRINTF("ehci port %jd reset, status = 0x%08jx", index,
			    v, 0, 0);
			break;
		case UHF_PORT_POWER:
			DPRINTF("set port power %jd (has PPC = %jd)", index,
			    sc->sc_hasppc, 0, 0);
			if (sc->sc_hasppc)
				EOWRITE4(sc, port, v | EHCI_PS_PP);
			break;
		case UHF_PORT_TEST:
			DPRINTF("set port test %jd", index, 0, 0, 0);
			break;
		case UHF_PORT_INDICATOR:
			DPRINTF("set port ind %jd", index, 0, 0, 0);
			EOWRITE4(sc, port, v | EHCI_PS_PIC);
			break;
		default:
			return -1;
		}
		break;
	case C(UR_CLEAR_TT_BUFFER, UT_WRITE_CLASS_OTHER):
	case C(UR_RESET_TT, UT_WRITE_CLASS_OTHER):
	case C(UR_GET_TT_STATE, UT_READ_CLASS_OTHER):
	case C(UR_STOP_TT, UT_WRITE_CLASS_OTHER):
		break;
	default:
		/* default from usbroothub */
		DPRINTF("returning %jd (usbroothub default)", buflen, 0, 0, 0);

		return buflen;
	}

	DPRINTF("returning %jd", totlen, 0, 0, 0);

	return totlen;
}

Static int
ehci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct ehci_softc *sc = EHCI_BUS2SC(bus);
	int actlen;

	mutex_enter(&sc->sc_rhlock);
	actlen = ehci_roothub_ctrl_locked(bus, req, buf, buflen);
	mutex_exit(&sc->sc_rhlock);

	return actlen;
}

/*
 * Handle ehci hand-off in early boot vs RB_ASKNAME/RB_SINGLE.
 *
 * This pile of garbage below works around the following problem without
 * holding boots with no hand-over devices present, while penalising
 * boots where the first ehci probe hands off devices with a 5 second
 * delay, if RB_ASKNAME/RB_SINGLE is set.  This is typically not a problem
 * for RB_SINGLE, but the same basic issue exists.
 *
 * The way ehci hand-off works, the companion controller does not get the
 * device until after its initial bus explore, so the reference dropped
 * after the first explore is not enough.  5 seconds should be enough,
 * and EHCI_DISOWN_DELAY_SECONDS can be set to another value.
 *
 * There are 3 states.  CO_EARLY is set during attach.  CO_SCHED is set
 * if the callback is scheduled.  CO_DONE is set when the callout has
 * called config_pending_decr().
 *
 * There's a mutex, a cv and a callout here, and we delay detach if the
 * callout has been set.
 */
#ifndef EHCI_DISOWN_DELAY_SECONDS
#define EHCI_DISOWN_DELAY_SECONDS 5
#endif
static int ehci_disown_delay_seconds = EHCI_DISOWN_DELAY_SECONDS;

static void
ehci_disown_callback(void *arg)
{
	ehci_softc_t *sc = arg;

	config_pending_decr(sc->sc_dev);

	mutex_enter(&sc->sc_complock);
	KASSERT(sc->sc_comp_state == CO_SCHED);
	sc->sc_comp_state = CO_DONE;
	cv_signal(&sc->sc_compcv);
	mutex_exit(&sc->sc_complock);
}

static void
ehci_disown_sched_callback(ehci_softc_t *sc)
{
	extern bool root_is_mounted;

	mutex_enter(&sc->sc_complock);

	if (root_is_mounted ||
	    (boothowto & (RB_ASKNAME|RB_SINGLE)) == 0 ||
	    sc->sc_comp_state != CO_EARLY) {
		mutex_exit(&sc->sc_complock);
		return;
	}

	callout_reset(&sc->sc_compcallout, ehci_disown_delay_seconds * hz,
	    ehci_disown_callback, &sc->sc_dev);
	sc->sc_comp_state = CO_SCHED;

	mutex_exit(&sc->sc_complock);

	config_pending_incr(sc->sc_dev);
	aprint_normal_dev(sc->sc_dev,
	    "delaying %s by %u seconds due to USB owner change.\n",
	    (boothowto & RB_ASKNAME) != 0 ? "ask root" : "single user",
	    ehci_disown_delay_seconds);
}

Static void
ehci_disown(ehci_softc_t *sc, int index, int lowspeed)
{
	int port;
	uint32_t v;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("index=%jd lowspeed=%jd", index, lowspeed, 0, 0);
	if (sc->sc_npcomp != 0) {
		int i = (index-1) / sc->sc_npcomp;
		if (i < sc->sc_ncomp) {
			ehci_disown_sched_callback(sc);
#ifdef DIAGNOSTIC
			printf("%s: handing over %s speed device on "
			       "port %d to %s\n",
			       device_xname(sc->sc_dev),
			       lowspeed ? "low" : "full",
			       index, sc->sc_comps[i] ?
			         device_xname(sc->sc_comps[i]) :
			         "companion controller");
		} else {
			printf("%s: strange port\n",
			       device_xname(sc->sc_dev));
#endif
		}
	} else {
#ifdef DIAGNOSTIC
		printf("%s: npcomp == 0\n", device_xname(sc->sc_dev));
#endif
	}
	port = EHCI_PORTSC(index);
	v = EOREAD4(sc, port) &~ EHCI_PS_CLEAR;
	EOWRITE4(sc, port, v | EHCI_PS_PO);
}

Static usbd_status
ehci_root_intr_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, start first */
	return ehci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ehci_root_intr_start(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	xfer->ux_status = USBD_IN_PROGRESS;

	return USBD_IN_PROGRESS;
}

/* Abort a root interrupt request. */
Static void
ehci_root_intr_abort(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);

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

/* Close the root pipe. */
Static void
ehci_root_intr_close(struct usbd_pipe *pipe)
{
	ehci_softc_t *sc __diagused = EHCI_PIPE2SC(pipe);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * Caller must guarantee the xfer has completed first, by
	 * closing the pipe only after normal completion or an abort.
	 */
	KASSERT(sc->sc_intrxfer == NULL);
}

Static void
ehci_root_intr_done(struct usbd_xfer *xfer)
{
	struct ehci_softc *sc = EHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Claim the xfer so it doesn't get completed again.  */
	KASSERT(sc->sc_intrxfer == xfer);
	KASSERT(xfer->ux_status != USBD_IN_PROGRESS);
	sc->sc_intrxfer = NULL;
}

/************************/

Static ehci_soft_qh_t *
ehci_alloc_sqh(ehci_softc_t *sc)
{
	ehci_soft_qh_t *sqh;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freeqhs == NULL) {
		DPRINTF("allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		usb_dma_t dma;
		int err = usb_allocmem(sc->sc_dmatag,
		    EHCI_SQH_SIZE * EHCI_SQH_CHUNK,
		    EHCI_PAGE_SIZE, USBMALLOC_COHERENT, &dma);

		if (err) {
			DPRINTF("alloc returned %jd", err, 0, 0, 0);
			return NULL;
		}

		mutex_enter(&sc->sc_lock);
		for (size_t i = 0; i < EHCI_SQH_CHUNK; i++) {
			const int offs = i * EHCI_SQH_SIZE;
			const bus_addr_t baddr = DMAADDR(&dma, offs);

			KASSERT(BUS_ADDR_HI32(baddr) == 0);

			sqh = KERNADDR(&dma, offs);
			sqh->physaddr = BUS_ADDR_LO32(baddr);
			sqh->dma = dma;
			sqh->offs = offs;

			sqh->next = sc->sc_freeqhs;
			sc->sc_freeqhs = sqh;
		}
	}
	sqh = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh->next;
	mutex_exit(&sc->sc_lock);

	memset(&sqh->qh, 0, sizeof(ehci_qh_t));
	sqh->next = NULL;
	return sqh;
}

Static void
ehci_free_sqh(ehci_softc_t *sc, ehci_soft_qh_t *sqh)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	sqh->next = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh;
}

Static ehci_soft_qtd_t *
ehci_alloc_sqtd(ehci_softc_t *sc)
{
	ehci_soft_qtd_t *sqtd = NULL;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freeqtds == NULL) {
		DPRINTF("allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		usb_dma_t dma;
		int err = usb_allocmem(sc->sc_dmatag,
		    EHCI_SQTD_SIZE * EHCI_SQTD_CHUNK,
		    EHCI_PAGE_SIZE, USBMALLOC_COHERENT, &dma);

		if (err) {
			DPRINTF("alloc returned %jd", err, 0, 0, 0);
			return NULL;
		}

		mutex_enter(&sc->sc_lock);
		for (size_t i = 0; i < EHCI_SQTD_CHUNK; i++) {
			const int offs = i * EHCI_SQTD_SIZE;
			const bus_addr_t baddr = DMAADDR(&dma, offs);

			KASSERT(BUS_ADDR_HI32(baddr) == 0);

			sqtd = KERNADDR(&dma, offs);
			sqtd->physaddr = BUS_ADDR_LO32(baddr);
			sqtd->dma = dma;
			sqtd->offs = offs;

			sqtd->nextqtd = sc->sc_freeqtds;
			sc->sc_freeqtds = sqtd;
		}
	}

	sqtd = sc->sc_freeqtds;
	sc->sc_freeqtds = sqtd->nextqtd;
	mutex_exit(&sc->sc_lock);

	memset(&sqtd->qtd, 0, sizeof(ehci_qtd_t));
	sqtd->nextqtd = NULL;
	sqtd->xfer = NULL;

	return sqtd;
}

Static void
ehci_free_sqtd(ehci_softc_t *sc, ehci_soft_qtd_t *sqtd)
{

	mutex_enter(&sc->sc_lock);
	sqtd->nextqtd = sc->sc_freeqtds;
	sc->sc_freeqtds = sqtd;
	mutex_exit(&sc->sc_lock);
}

Static int
ehci_alloc_sqtd_chain(ehci_softc_t *sc, struct usbd_xfer *xfer,
    int alen, int rd, ehci_soft_qtd_t **sp)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	uint16_t flags = xfer->ux_flags;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	ASSERT_SLEEPABLE();
	KASSERT(sp);
	KASSERT(alen != 0 || (!rd && (flags & USBD_FORCE_SHORT_XFER)));

	size_t nsqtd = (!rd && (flags & USBD_FORCE_SHORT_XFER)) ? 1 : 0;
	nsqtd += howmany(alen, EHCI_PAGE_SIZE);
	exfer->ex_sqtds = kmem_zalloc(sizeof(ehci_soft_qtd_t *) * nsqtd,
	    KM_SLEEP);
	exfer->ex_nsqtd = nsqtd;

	DPRINTF("xfer %#jx len %jd nsqtd %jd flags %jx", (uintptr_t)xfer,
	    alen, nsqtd, flags);

	for (size_t j = 0; j < exfer->ex_nsqtd;) {
		ehci_soft_qtd_t *cur = ehci_alloc_sqtd(sc);
		if (cur == NULL)
			goto nomem;
		exfer->ex_sqtds[j++] = cur;

		cur->xfer = xfer;
		cur->len = 0;

	}

	*sp = exfer->ex_sqtds[0];
	DPRINTF("return sqtd=%#jx", (uintptr_t)*sp, 0, 0, 0);

	return 0;

 nomem:
	ehci_free_sqtds(sc, exfer);
	kmem_free(exfer->ex_sqtds, sizeof(ehci_soft_qtd_t *) * nsqtd);
	DPRINTF("no memory", 0, 0, 0, 0);
	return ENOMEM;
}

Static void
ehci_free_sqtds(ehci_softc_t *sc, struct ehci_xfer *exfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	DPRINTF("exfer=%#jx", (uintptr_t)exfer, 0, 0, 0);

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < exfer->ex_nsqtd; i++) {
		ehci_soft_qtd_t *sqtd = exfer->ex_sqtds[i];

		if (sqtd == NULL)
			break;

		sqtd->nextqtd = sc->sc_freeqtds;
		sc->sc_freeqtds = sqtd;
	}
	mutex_exit(&sc->sc_lock);
}

Static void
ehci_append_sqtd(ehci_soft_qtd_t *sqtd, ehci_soft_qtd_t *prev)
{
	if (prev) {
		prev->nextqtd = sqtd;
		prev->qtd.qtd_next = htole32(sqtd->physaddr);
		prev->qtd.qtd_altnext = prev->qtd.qtd_next;
		usb_syncmem(&prev->dma, prev->offs, sizeof(prev->qtd),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	}
}

Static void
ehci_reset_sqtd_chain(ehci_softc_t *sc, struct usbd_xfer *xfer,
    int length, int isread, int *toggle, ehci_soft_qtd_t **lsqtd)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	usb_dma_t *dma = &xfer->ux_dmabuf;
	uint16_t flags = xfer->ux_flags;
	ehci_soft_qtd_t *sqtd, *prev;
	int tog = *toggle;
	int mps = UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize);
	int len = length;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	DPRINTF("xfer=%#jx len %jd isread %jd toggle %jd", (uintptr_t)xfer,
	    len, isread, tog);
	DPRINTF("    VA %#jx", (uintptr_t)KERNADDR(&xfer->ux_dmabuf, 0),
	    0, 0, 0);

	KASSERT(length != 0 || (!isread && (flags & USBD_FORCE_SHORT_XFER)));

	const uint32_t qtdstatus = EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_PID(isread ? EHCI_QTD_PID_IN : EHCI_QTD_PID_OUT) |
	    EHCI_QTD_SET_CERR(3)
	    ;

	sqtd = prev = NULL;
	size_t curoffs = 0;
	size_t j = 0;
	for (; len != 0 && j < exfer->ex_nsqtd; prev = sqtd) {
		sqtd = exfer->ex_sqtds[j++];
		DPRINTF("sqtd[%jd]=%#jx prev %#jx", j, (uintptr_t)sqtd,
		    (uintptr_t)prev, 0);

		/*
		 * The EHCI hardware can handle at most 5 pages and they do
		 * not have to be contiguous
		 */
		vaddr_t va = (vaddr_t)KERNADDR(dma, curoffs);
		vaddr_t va_offs = EHCI_PAGE_OFFSET(va);
		size_t curlen = len;
		if (curlen >= EHCI_QTD_MAXTRANSFER - va_offs) {
			/* must use multiple TDs, fill as much as possible. */
			curlen = EHCI_QTD_MAXTRANSFER - va_offs;

			/* the length must be a multiple of the max size */
			curlen -= curlen % mps;
		}
		KASSERT(curlen != 0);
		DPRINTF("    len=%jd curlen=%jd curoffs=%ju", len, curlen,
		    curoffs, 0);

		/* Fill the qTD */
		sqtd->qtd.qtd_next = sqtd->qtd.qtd_altnext = EHCI_NULL;
		sqtd->qtd.qtd_status = htole32(
		    qtdstatus |
		    EHCI_QTD_SET_BYTES(curlen) |
		    EHCI_QTD_SET_TOGGLE(tog));

		/* Find number of pages we'll be using, insert dma addresses */
		size_t pages = EHCI_NPAGES(curlen);
		KASSERT(pages <= EHCI_QTD_NBUFFERS);
		size_t pageoffs = EHCI_PAGE(curoffs);
		for (size_t i = 0; i < pages; i++) {
			paddr_t a = EHCI_PAGE(DMAADDR(dma,
			    pageoffs + i * EHCI_PAGE_SIZE));
			sqtd->qtd.qtd_buffer[i] = htole32(BUS_ADDR_LO32(a));
			sqtd->qtd.qtd_buffer_hi[i] = htole32(BUS_ADDR_HI32(a));
			DPRINTF("      buffer[%jd/%jd] 0x%08jx 0x%08jx",
			    i, pages,
			    le32toh(sqtd->qtd.qtd_buffer_hi[i]),
			    le32toh(sqtd->qtd.qtd_buffer[i]));
		}
		/* First buffer pointer requires a page offset to start at */
		sqtd->qtd.qtd_buffer[0] |= htole32(va_offs);

		usb_syncmem(&sqtd->dma, sqtd->offs, sizeof(sqtd->qtd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		sqtd->len = curlen;

		DPRINTF("    va %#jx pa %#jx len %jd", (uintptr_t)va,
		    (uintptr_t)DMAADDR(&xfer->ux_dmabuf, curoffs), curlen, 0);

		ehci_append_sqtd(sqtd, prev);

		if (howmany(curlen, mps) & 1) {
			tog ^= 1;
		}

		curoffs += curlen;
		len -= curlen;
	}
	KASSERTMSG(len == 0, "xfer %p olen %d len %d mps %d ex_nsqtd %zu j %zu",
	    xfer, length, len, mps, exfer->ex_nsqtd, j);

	if (!isread &&
	    (flags & USBD_FORCE_SHORT_XFER) &&
	    length % mps == 0) {
		/* Force a 0 length transfer at the end. */

		KASSERTMSG(j < exfer->ex_nsqtd, "j=%zu nsqtd=%zu", j,
		    exfer->ex_nsqtd);
		prev = sqtd;
		sqtd = exfer->ex_sqtds[j++];
		memset(&sqtd->qtd, 0, sizeof(sqtd->qtd));
		sqtd->qtd.qtd_next = sqtd->qtd.qtd_altnext = EHCI_NULL;
		sqtd->qtd.qtd_status = htole32(
		    qtdstatus |
		    EHCI_QTD_SET_BYTES(0) |
		    EHCI_QTD_SET_TOGGLE(tog));

		usb_syncmem(&sqtd->dma, sqtd->offs, sizeof(sqtd->qtd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		ehci_append_sqtd(sqtd, prev);
		tog ^= 1;
	}

	*lsqtd = sqtd;
	*toggle = tog;
}

Static ehci_soft_itd_t *
ehci_alloc_itd(ehci_softc_t *sc)
{
	struct ehci_soft_itd *itd, *freeitd;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);

	freeitd = LIST_FIRST(&sc->sc_freeitds);
	if (freeitd == NULL) {
		DPRINTF("allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		usb_dma_t dma;
		int err = usb_allocmem(sc->sc_dmatag,
		    EHCI_ITD_SIZE * EHCI_ITD_CHUNK,
		    EHCI_PAGE_SIZE, USBMALLOC_COHERENT, &dma);

		if (err) {
			DPRINTF("alloc returned %jd", err, 0, 0, 0);
			return NULL;
		}

		mutex_enter(&sc->sc_lock);
		for (size_t i = 0; i < EHCI_ITD_CHUNK; i++) {
			const int offs = i * EHCI_ITD_SIZE;
			const bus_addr_t baddr = DMAADDR(&dma, offs);

			KASSERT(BUS_ADDR_HI32(baddr) == 0);

			itd = KERNADDR(&dma, offs);
			itd->physaddr = BUS_ADDR_LO32(baddr);
	 		itd->dma = dma;
			itd->offs = offs;

			LIST_INSERT_HEAD(&sc->sc_freeitds, itd, free_list);
		}
		freeitd = LIST_FIRST(&sc->sc_freeitds);
	}

	itd = freeitd;
	LIST_REMOVE(itd, free_list);
	mutex_exit(&sc->sc_lock);
	memset(&itd->itd, 0, sizeof(ehci_itd_t));

	itd->frame_list.next = NULL;
	itd->frame_list.prev = NULL;
	itd->xfer_next = NULL;
	itd->slot = 0;

	return itd;
}

Static ehci_soft_sitd_t *
ehci_alloc_sitd(ehci_softc_t *sc)
{
	struct ehci_soft_sitd *sitd, *freesitd;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	freesitd = LIST_FIRST(&sc->sc_freesitds);
	if (freesitd == NULL) {
		DPRINTF("allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		usb_dma_t dma;
		int err = usb_allocmem(sc->sc_dmatag,
		    EHCI_SITD_SIZE * EHCI_SITD_CHUNK,
		    EHCI_PAGE_SIZE, USBMALLOC_COHERENT, &dma);

		if (err) {
			DPRINTF("alloc returned %jd", err, 0, 0, 0);
			return NULL;
		}

		mutex_enter(&sc->sc_lock);
		for (size_t i = 0; i < EHCI_SITD_CHUNK; i++) {
			const int offs = i * EHCI_SITD_SIZE;
			const bus_addr_t baddr = DMAADDR(&dma, offs);

			KASSERT(BUS_ADDR_HI32(baddr) == 0);

			sitd = KERNADDR(&dma, offs);
			sitd->physaddr = BUS_ADDR_LO32(baddr);
	 		sitd->dma = dma;
			sitd->offs = offs;

			LIST_INSERT_HEAD(&sc->sc_freesitds, sitd, free_list);
		}
		freesitd = LIST_FIRST(&sc->sc_freesitds);
	}

	sitd = freesitd;
	LIST_REMOVE(sitd, free_list);
	mutex_exit(&sc->sc_lock);

	memset(&sitd->sitd, 0, sizeof(ehci_sitd_t));

	sitd->frame_list.next = NULL;
	sitd->frame_list.prev = NULL;
	sitd->xfer_next = NULL;
	sitd->slot = 0;

	return sitd;
}

/****************/

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
Static void
ehci_close_pipe(struct usbd_pipe *pipe, ehci_soft_qh_t *head)
{
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(pipe);
	ehci_softc_t *sc = EHCI_PIPE2SC(pipe);
	ehci_soft_qh_t *sqh = epipe->sqh;

	KASSERT(mutex_owned(&sc->sc_lock));

	ehci_rem_qh(sc, sqh, head);
	ehci_free_sqh(sc, epipe->sqh);
}

/*
 * Arrange for the hardware to tells us that it is not still
 * processing the TDs by setting the QH halted bit and wait for the ehci
 * door bell
 */
Static void
ehci_abortx(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	ehci_soft_qh_t *sqh = epipe->sqh;
	ehci_soft_qtd_t *sqtd, *fsqtd, *lsqtd;
	ehci_physaddr_t cur;
	uint32_t qhstatus;
	int hit;

	DPRINTF("xfer=%#jx pipe=%#jx", (uintptr_t)xfer, (uintptr_t)epipe, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	ASSERT_SLEEPABLE();

	KASSERTMSG((xfer->ux_status == USBD_CANCELLED ||
		xfer->ux_status == USBD_TIMEOUT),
	    "bad abort status: %d", xfer->ux_status);

	/*
	 * If we're dying, skip the hardware action and just notify the
	 * software that we're done.
	 */
	if (sc->sc_dying) {
		goto dying;
	}

	/*
	 * HC Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	ehci_del_intr_list(sc, exfer);

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

	if (exfer->ex_type == EX_CTRL) {
		fsqtd = exfer->ex_setup;
		lsqtd = exfer->ex_status;
	} else {
		fsqtd = exfer->ex_sqtdstart;
		lsqtd = exfer->ex_sqtdend;
	}
	for (sqtd = fsqtd; ; sqtd = sqtd->nextqtd) {
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
		    sizeof(sqtd->qtd.qtd_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		sqtd->qtd.qtd_status |= htole32(EHCI_QTD_HALTED);
		usb_syncmem(&sqtd->dma,
		    sqtd->offs + offsetof(ehci_qtd_t, qtd_status),
		    sizeof(sqtd->qtd.qtd_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		if (sqtd == lsqtd)
			break;
	}

	/*
	 * HC Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.
	 */
	ehci_sync_hc(sc);

	/*
	 * HC Step 3: Remove any vestiges of the xfer from the hardware.
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
	for (sqtd = fsqtd; ; sqtd = sqtd->nextqtd) {
		hit |= cur == sqtd->physaddr;
		if (sqtd == lsqtd)
			break;
	}
	sqtd = sqtd->nextqtd;
	/* Zap curqtd register if hardware pointed inside the xfer. */
	if (hit && sqtd != NULL) {
		DPRINTF("cur=0x%08jx", sqtd->physaddr, 0, 0, 0);
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
		DPRINTF("no hit", 0, 0, 0, 0);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(ehci_qh_t, qh_curqtd),
		    sizeof(sqh->qh.qh_curqtd),
		    BUS_DMASYNC_PREREAD);
	}

dying:
#ifdef DIAGNOSTIC
	exfer->ex_isdone = true;
#endif
	DPRINTFN(14, "end", 0, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
}

Static void
ehci_abort_isoc_xfer(struct usbd_xfer *xfer, usbd_status status)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	ehci_isoc_trans_t trans_status;
	struct ehci_xfer *exfer;
	ehci_softc_t *sc;
	struct ehci_soft_itd *itd;
	struct ehci_soft_sitd *sitd;
	int i;

	KASSERTMSG(status == USBD_CANCELLED,
	    "invalid status for abort: %d", (int)status);

	exfer = EHCI_XFER2EXFER(xfer);
	sc = EHCI_XFER2SC(xfer);

	DPRINTF("xfer %#jx pipe %#jx", (uintptr_t)xfer,
	    (uintptr_t)xfer->ux_pipe, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	ASSERT_SLEEPABLE();

	/* No timeout or task here. */

	/*
	 * The xfer cannot have been cancelled already.  It is the
	 * responsibility of the caller of usbd_abort_pipe not to try
	 * to abort a pipe multiple times, whether concurrently or
	 * sequentially.
	 */
	KASSERT(xfer->ux_status != USBD_CANCELLED);

	/* If anyone else beat us, we're done.  */
	if (xfer->ux_status != USBD_IN_PROGRESS)
		return;

	/* We beat everyone else.  Claim the status.  */
	xfer->ux_status = status;

	/*
	 * If we're dying, skip the hardware action and just notify the
	 * software that we're done.
	 */
	if (sc->sc_dying) {
		goto dying;
	}

	/*
	 * HC Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	ehci_del_intr_list(sc, exfer);

	if (xfer->ux_pipe->up_dev->ud_speed == USB_SPEED_HIGH) {
		for (itd = exfer->ex_itdstart; itd != NULL;
		     itd = itd->xfer_next) {
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
	} else {
		for (sitd = exfer->ex_sitdstart; sitd != NULL;
		     sitd = sitd->xfer_next) {
			usb_syncmem(&sitd->dma,
			    sitd->offs + offsetof(ehci_sitd_t, sitd_buffer),
			    sizeof(sitd->sitd.sitd_buffer),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

			trans_status = le32toh(sitd->sitd.sitd_trans);
			trans_status &= ~EHCI_SITD_ACTIVE;
			sitd->sitd.sitd_trans = htole32(trans_status);

			usb_syncmem(&sitd->dma,
			    sitd->offs + offsetof(ehci_sitd_t, sitd_buffer),
			    sizeof(sitd->sitd.sitd_buffer),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		}
	}

dying:
#ifdef DIAGNOSTIC
	exfer->ex_isdone = true;
#endif
	usb_transfer_complete(xfer);
	DPRINTFN(14, "end", 0, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
}

/************************/

Static int
ehci_device_ctrl_init(struct usbd_xfer *xfer)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	usb_device_request_t *req = &xfer->ux_request;
	ehci_soft_qtd_t *setup, *status, *next;
	int isread = req->bmRequestType & UT_READ;
	int len = xfer->ux_bufsize;
	int err;

	exfer->ex_type = EX_CTRL;
	exfer->ex_status = NULL;
	exfer->ex_data = NULL;
	exfer->ex_setup = ehci_alloc_sqtd(sc);
	if (exfer->ex_setup == NULL) {
		err = ENOMEM;
		goto bad1;
	}
	exfer->ex_status = ehci_alloc_sqtd(sc);
	if (exfer->ex_status == NULL) {
		err = ENOMEM;
		goto bad2;
	}
	setup = exfer->ex_setup;
	status = exfer->ex_status;
	exfer->ex_nsqtd = 0;
	next = status;
	/* Set up data transaction */
	if (len != 0) {
		err = ehci_alloc_sqtd_chain(sc, xfer, len, isread,
		    &exfer->ex_data);
		if (err)
			goto bad3;
		next = exfer->ex_data;
	}

	/* Clear toggle */
	setup->qtd.qtd_status = htole32(
	    EHCI_QTD_SET_PID(EHCI_QTD_PID_SETUP) |
	    EHCI_QTD_SET_TOGGLE(0) |
	    EHCI_QTD_SET_BYTES(sizeof(*req))
	    );

	const bus_addr_t ba = DMAADDR(&epipe->ctrl.reqdma, 0);
	setup->qtd.qtd_buffer[0] = htole32(BUS_ADDR_LO32(ba));
	setup->qtd.qtd_buffer_hi[0] = htole32(BUS_ADDR_HI32(ba));
	setup->qtd.qtd_next = setup->qtd.qtd_altnext = htole32(next->physaddr);
	setup->nextqtd = next;
	setup->xfer = xfer;
	setup->len = sizeof(*req);

	status->qtd.qtd_status = htole32(
	    EHCI_QTD_SET_PID(isread ? EHCI_QTD_PID_OUT : EHCI_QTD_PID_IN) |
	    EHCI_QTD_SET_TOGGLE(1) |
	    EHCI_QTD_IOC
	    );
	status->qtd.qtd_buffer[0] = 0;
	status->qtd.qtd_buffer_hi[0] = 0;
	status->qtd.qtd_next = status->qtd.qtd_altnext = EHCI_NULL;
	status->nextqtd = NULL;
	status->xfer = xfer;
	status->len = 0;

	return 0;
bad3:
	ehci_free_sqtd(sc, exfer->ex_status);
bad2:
	ehci_free_sqtd(sc, exfer->ex_setup);
bad1:
	return err;
}

Static void
ehci_device_ctrl_fini(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);

	KASSERT(ex->ex_type == EX_CTRL);

	ehci_free_sqtd(sc, ex->ex_setup);
	ehci_free_sqtd(sc, ex->ex_status);
	ehci_free_sqtds(sc, ex);
	if (ex->ex_nsqtd)
		kmem_free(ex->ex_sqtds,
		    sizeof(ehci_soft_qtd_t *) * ex->ex_nsqtd);
}

Static usbd_status
ehci_device_ctrl_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, start first */
	return ehci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ehci_device_ctrl_start(struct usbd_xfer *xfer)
{
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	usb_device_request_t *req = &xfer->ux_request;
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	ehci_soft_qtd_t *setup, *status, *next;
	ehci_soft_qh_t *sqh;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	if (sc->sc_dying)
		return USBD_IOERROR;

	const int isread = req->bmRequestType & UT_READ;
	const int len = UGETW(req->wLength);

	DPRINTF("type=0x%02jx, request=0x%02jx, wValue=0x%04jx, wIndex=0x%04jx",
	    req->bmRequestType, req->bRequest, UGETW(req->wValue),
	    UGETW(req->wIndex));
	DPRINTF("len=%jd, addr=%jd, endpt=%jd",
	    len, epipe->pipe.up_dev->ud_addr,
	    epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress, 0);

	sqh = epipe->sqh;

	KASSERTMSG(EHCI_QH_GET_ADDR(le32toh(sqh->qh.qh_endp)) == epipe->pipe.up_dev->ud_addr,
	    "address QH %" __PRIuBIT " pipe %d\n",
	    EHCI_QH_GET_ADDR(le32toh(sqh->qh.qh_endp)),
	    epipe->pipe.up_dev->ud_addr);
	KASSERTMSG(EHCI_QH_GET_MPL(le32toh(sqh->qh.qh_endp)) ==
	    UGETW(epipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize),
	    "MPS QH %" __PRIuBIT " pipe %d\n",
	    EHCI_QH_GET_MPL(le32toh(sqh->qh.qh_endp)),
	    UGETW(epipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize));

	setup = exfer->ex_setup;
	status = exfer->ex_status;

	DPRINTF("setup %#jx status %#jx data %#jx",
	    (uintptr_t)setup, (uintptr_t)status, (uintptr_t)exfer->ex_data, 0);
	KASSERTMSG(setup != NULL && status != NULL,
	    "Failed memory allocation, setup %p status %p",
	    setup, status);

	memcpy(KERNADDR(&epipe->ctrl.reqdma, 0), req, sizeof(*req));
	usb_syncmem(&epipe->ctrl.reqdma, 0, sizeof(*req), BUS_DMASYNC_PREWRITE);

	/* Clear toggle */
	setup->qtd.qtd_status &= ~htole32(
	    EHCI_QTD_STATUS_MASK |
	    EHCI_QTD_BYTES_MASK |
	    EHCI_QTD_TOGGLE_MASK |
	    EHCI_QTD_CERR_MASK
	    );
	setup->qtd.qtd_status |= htole32(
	    EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_CERR(3) |
	    EHCI_QTD_SET_TOGGLE(0) |
	    EHCI_QTD_SET_BYTES(sizeof(*req))
	    );

	const bus_addr_t ba = DMAADDR(&epipe->ctrl.reqdma, 0);
	setup->qtd.qtd_buffer[0] = htole32(BUS_ADDR_LO32(ba));
	setup->qtd.qtd_buffer_hi[0] = htole32(BUS_ADDR_HI32(ba));

	next = status;
	status->qtd.qtd_status &= ~htole32(
	    EHCI_QTD_STATUS_MASK |
	    EHCI_QTD_PID_MASK |
	    EHCI_QTD_BYTES_MASK |
	    EHCI_QTD_TOGGLE_MASK |
	    EHCI_QTD_CERR_MASK
	    );
	status->qtd.qtd_status |= htole32(
	    EHCI_QTD_ACTIVE |
	    EHCI_QTD_SET_PID(isread ? EHCI_QTD_PID_OUT : EHCI_QTD_PID_IN) |
	    EHCI_QTD_SET_CERR(3) |
	    EHCI_QTD_SET_TOGGLE(1) |
	    EHCI_QTD_SET_BYTES(0) |
	    EHCI_QTD_IOC
	    );
	KASSERT(status->qtd.qtd_status & htole32(EHCI_QTD_TOGGLE_MASK));

	KASSERT(exfer->ex_isdone);
#ifdef DIAGNOSTIC
	exfer->ex_isdone = false;
#endif

	/* Set up data transaction */
	if (len != 0) {
		ehci_soft_qtd_t *end;

		/* Start toggle at 1. */
		int toggle = 1;
		next = exfer->ex_data;
		KASSERTMSG(next != NULL, "Failed memory allocation");
		ehci_reset_sqtd_chain(sc, xfer, len, isread, &toggle, &end);
		end->nextqtd = status;
		end->qtd.qtd_next = end->qtd.qtd_altnext =
		    htole32(status->physaddr);

		usb_syncmem(&end->dma, end->offs, sizeof(end->qtd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	}

	setup->nextqtd = next;
	setup->qtd.qtd_next = setup->qtd.qtd_altnext = htole32(next->physaddr);

	usb_syncmem(&setup->dma, setup->offs, sizeof(setup->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	 usb_syncmem(&status->dma, status->offs, sizeof(status->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	KASSERT(status->qtd.qtd_status & htole32(EHCI_QTD_TOGGLE_MASK));

#ifdef EHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(setup);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	/* Insert qTD in QH list - also does usb_syncmem(sqh) */
	ehci_set_qh_qtd(sqh, setup);
	usbd_xfer_schedule_timeout(xfer);
	ehci_add_intr_list(sc, exfer);
	xfer->ux_status = USBD_IN_PROGRESS;

#if 0
#ifdef EHCI_DEBUG
	DPRINTFN(10, "status=%jx, dump:", EOREAD4(sc, EHCI_USBSTS), 0, 0, 0);
//	delay(10000);
	ehci_dump_regs(sc);
	ehci_dump_sqh(sc->sc_async_head);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(setup);
#endif
#endif

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_ctrl_done(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc __diagused = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	usb_device_request_t *req = &xfer->ux_request;
	int len = UGETW(req->wLength);
	int rd = req->bmRequestType & UT_READ;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();
	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	usb_syncmem(&epipe->ctrl.reqdma, 0, sizeof(*req),
	    BUS_DMASYNC_POSTWRITE);
	if (len)
		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);

	DPRINTF("length=%jd", xfer->ux_actlen, 0, 0, 0);
}

/* Abort a device control request. */
Static void
ehci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);
	usbd_xfer_abort(xfer);
}

/* Close a device control pipe. */
Static void
ehci_device_ctrl_close(struct usbd_pipe *pipe)
{
	ehci_softc_t *sc = EHCI_PIPE2SC(pipe);
	struct ehci_pipe * const epipe = EHCI_PIPE2EPIPE(pipe);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("pipe=%#jx", (uintptr_t)pipe, 0, 0, 0);

	ehci_close_pipe(pipe, sc->sc_async_head);

	usb_freemem(&epipe->ctrl.reqdma);
}

/*
 * Some EHCI chips from VIA seem to trigger interrupts before writing back the
 * qTD status, or miss signalling occasionally under heavy load.  If the host
 * machine is too fast, we can miss transaction completion - when we scan
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

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	usb_schedsoftintr(&sc->sc_bus);
}

/************************/

Static int
ehci_device_bulk_init(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	int endpt = ed->bEndpointAddress;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int len = xfer->ux_bufsize;
	int err = 0;

	exfer->ex_type = EX_BULK;
	exfer->ex_nsqtd = 0;
	err = ehci_alloc_sqtd_chain(sc, xfer, len, isread,
	    &exfer->ex_sqtdstart);

	return err;
}

Static void
ehci_device_bulk_fini(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);

	KASSERT(ex->ex_type == EX_BULK);

	ehci_free_sqtds(sc, ex);
	if (ex->ex_nsqtd)
		kmem_free(ex->ex_sqtds, sizeof(ehci_soft_qtd_t *) * ex->ex_nsqtd);
}

Static usbd_status
ehci_device_bulk_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, start first */
	return ehci_device_bulk_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ehci_device_bulk_start(struct usbd_xfer *xfer)
{
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	ehci_soft_qh_t *sqh;
	ehci_soft_qtd_t *end;
	int len, isread, endpt;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_length <= xfer->ux_bufsize);

	len = xfer->ux_length;
	endpt = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sqh = epipe->sqh;

	KASSERT(exfer->ex_isdone);
#ifdef DIAGNOSTIC
	exfer->ex_isdone = false;
#endif

	ehci_reset_sqtd_chain(sc, xfer, len, isread, &epipe->nexttoggle, &end);

	exfer->ex_sqtdend = end;
	end->qtd.qtd_status |= htole32(EHCI_QTD_IOC);
	usb_syncmem(&end->dma, end->offs, sizeof(end->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef EHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(exfer->ex_sqtdstart);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	/* also does usb_syncmem(sqh) */
	ehci_set_qh_qtd(sqh, exfer->ex_sqtdstart);
	usbd_xfer_schedule_timeout(xfer);
	ehci_add_intr_list(sc, exfer);
	xfer->ux_status = USBD_IN_PROGRESS;

#if 0
#ifdef EHCI_DEBUG
	DPRINTFN(5, "data(2)", 0, 0, 0, 0);
//	delay(10000);
	DPRINTFN(5, "data(3)", 0, 0, 0, 0);
	ehci_dump_regs(sc);
#if 0
	printf("async_head:\n");
	ehci_dump_sqh(sc->sc_async_head);
#endif
	DPRINTF("sqh:", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(exfer->ex_sqtdstart);
#endif
#endif

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_bulk_abort(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer %#jx", (uintptr_t)xfer, 0, 0, 0);
	usbd_xfer_abort(xfer);
}

/*
 * Close a device bulk pipe.
 */
Static void
ehci_device_bulk_close(struct usbd_pipe *pipe)
{
	ehci_softc_t *sc = EHCI_PIPE2SC(pipe);
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(pipe);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("pipe=%#jx", (uintptr_t)pipe, 0, 0, 0);
	pipe->up_endpoint->ue_toggle = epipe->nexttoggle;
	ehci_close_pipe(pipe, sc->sc_async_head);
}

Static void
ehci_device_bulk_done(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc __diagused = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	int endpt = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	int rd = UE_GET_DIR(endpt) == UE_DIR_IN;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx, actlen=%jd", (uintptr_t)xfer, xfer->ux_actlen, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);

	DPRINTF("length=%jd", xfer->ux_actlen, 0, 0, 0);
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

	return USBD_NORMAL_COMPLETION;
}

Static int
ehci_device_intr_init(struct usbd_xfer *xfer)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	int endpt = ed->bEndpointAddress;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int len = xfer->ux_bufsize;
	int err;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(len != 0);

	exfer->ex_type = EX_INTR;
	exfer->ex_nsqtd = 0;
	err = ehci_alloc_sqtd_chain(sc, xfer, len, isread,
	    &exfer->ex_sqtdstart);

	return err;
}

Static void
ehci_device_intr_fini(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);

	KASSERT(ex->ex_type == EX_INTR);

	ehci_free_sqtds(sc, ex);
	if (ex->ex_nsqtd)
		kmem_free(ex->ex_sqtds, sizeof(ehci_soft_qtd_t *) * ex->ex_nsqtd);
}

Static usbd_status
ehci_device_intr_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, so start it first.  */
	return ehci_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ehci_device_intr_start(struct usbd_xfer *xfer)
{
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	ehci_soft_qtd_t *end;
	ehci_soft_qh_t *sqh;
	int len, isread, endpt;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_length <= xfer->ux_bufsize);

	len = xfer->ux_length;
	endpt = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sqh = epipe->sqh;

	KASSERT(exfer->ex_isdone);
#ifdef DIAGNOSTIC
	exfer->ex_isdone = false;
#endif

	ehci_reset_sqtd_chain(sc, xfer, len, isread, &epipe->nexttoggle, &end);

	end->qtd.qtd_status |= htole32(EHCI_QTD_IOC);
	usb_syncmem(&end->dma, end->offs, sizeof(end->qtd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	exfer->ex_sqtdend = end;

#ifdef EHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(exfer->ex_sqtdstart);
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	/* also does usb_syncmem(sqh) */
	ehci_set_qh_qtd(sqh, exfer->ex_sqtdstart);
	usbd_xfer_schedule_timeout(xfer);
	ehci_add_intr_list(sc, exfer);
	xfer->ux_status = USBD_IN_PROGRESS;

#if 0
#ifdef EHCI_DEBUG
	DPRINTFN(5, "data(2)", 0, 0, 0, 0);
//	delay(10000);
	DPRINTFN(5, "data(3)", 0, 0, 0, 0);
	ehci_dump_regs(sc);
	DPRINTFN(5, "sqh:", 0, 0, 0, 0);
	ehci_dump_sqh(sqh);
	ehci_dump_sqtds(exfer->ex_sqtdstart);
#endif
#endif

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_intr_abort(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	/*
	 * XXX - abort_xfer uses ehci_sync_hc, which syncs via the advance
	 *       async doorbell. That's dependent on the async list, wheras
	 *       intr xfers are periodic, should not use this?
	 */
	usbd_xfer_abort(xfer);
}

Static void
ehci_device_intr_close(struct usbd_pipe *pipe)
{
	ehci_softc_t *sc = EHCI_PIPE2SC(pipe);
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(pipe);
	struct ehci_soft_islot *isp;

	KASSERT(mutex_owned(&sc->sc_lock));

	isp = &sc->sc_islots[epipe->sqh->islot];
	ehci_close_pipe(pipe, isp->sqh);
}

Static void
ehci_device_intr_done(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc __diagused = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer=%#jx, actlen=%jd", (uintptr_t)xfer, xfer->ux_actlen, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (xfer->ux_length) {
		int isread, endpt;

		endpt = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
		isread = UE_GET_DIR(endpt) == UE_DIR_IN;
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}
}

/************************/
Static int
ehci_device_fs_isoc_init(struct usbd_xfer *xfer)
{
	struct ehci_pipe *epipe = EHCI_PIPE2EPIPE(xfer->ux_pipe);
	struct usbd_device *dev = xfer->ux_pipe->up_dev;
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_soft_sitd_t *sitd, *prev, *start, *stop;
	int i, k, frames;
	u_int huba, dir;
	int err;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	start = NULL;
	sitd = NULL;

	DPRINTF("xfer %#jx len %jd flags %jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_nframes != 0);
	KASSERT(exfer->ex_isdone);

	exfer->ex_type = EX_FS_ISOC;
	/*
	 * Step 1: Allocate and initialize sitds.
	 */
	i = epipe->pipe.up_endpoint->ue_edesc->bInterval;
	if (i > 16 || i == 0) {
		/* Spec page 271 says intervals > 16 are invalid */
		DPRINTF("bInterval %jd invalid", i, 0, 0, 0);

		return EINVAL;
	}

	frames = xfer->ux_nframes;
	for (i = 0, prev = NULL; i < frames; i++, prev = sitd) {
		sitd = ehci_alloc_sitd(sc);
		if (sitd == NULL) {
			err = ENOMEM;
			goto fail;
		}

		if (prev)
			prev->xfer_next = sitd;
		else
			start = sitd;

		huba = dev->ud_myhsport->up_parent->ud_addr;

#if 0
		if (sc->sc_flags & EHCIF_FREESCALE) {
			// Set hub address to 0 if embedded TT is used.
			if (huba == sc->sc_addr)
				huba = 0;
		}
#endif

		k = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
		dir = UE_GET_DIR(k) ? 1 : 0;
		sitd->sitd.sitd_endp =
		    htole32(EHCI_SITD_SET_ENDPT(UE_GET_ADDR(k)) |
		    EHCI_SITD_SET_DADDR(dev->ud_addr) |
		    EHCI_SITD_SET_PORT(dev->ud_myhsport->up_portno) |
		    EHCI_SITD_SET_HUBA(huba) |
		    EHCI_SITD_SET_DIR(dir));

		sitd->sitd.sitd_back = htole32(EHCI_LINK_TERMINATE);
	} /* End of frame */

	sitd->sitd.sitd_trans |= htole32(EHCI_SITD_IOC);

	stop = sitd;
	stop->xfer_next = NULL;
	exfer->ex_sitdstart = start;
	exfer->ex_sitdend = stop;

	return 0;

fail:
	mutex_enter(&sc->sc_lock);
	ehci_soft_sitd_t *next;
	for (sitd = start; sitd; sitd = next) {
		next = sitd->xfer_next;
		ehci_free_sitd_locked(sc, sitd);
	}
	mutex_exit(&sc->sc_lock);

	return err;
}

Static void
ehci_device_fs_isoc_fini(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);

	KASSERT(ex->ex_type == EX_FS_ISOC);

	ehci_free_sitd_chain(sc, ex->ex_sitdstart);
}

Static usbd_status
ehci_device_fs_isoc_transfer(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct usbd_device *dev = xfer->ux_pipe->up_dev;
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_soft_sitd_t *sitd;
	usb_dma_t *dma_buf;
	int i, j, k, frames;
	int offs;
	int frindex;
	u_int dir;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	sitd = NULL;

	DPRINTF("xfer %#jx len %jd flags %jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	/*
	 * To avoid complication, don't allow a request right now that'll span
	 * the entire frame table. To within 4 frames, to allow some leeway
	 * on either side of where the hc currently is.
	 */
	if (epipe->pipe.up_endpoint->ue_edesc->bInterval *
			xfer->ux_nframes >= sc->sc_flsize - 4) {
		printf("ehci: isoc descriptor requested that spans the entire"
		    "frametable, too many frames\n");
		return USBD_INVAL;
	}

	KASSERT(xfer->ux_nframes != 0 && xfer->ux_frlengths);
	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(exfer->ex_isdone);
#ifdef DIAGNOSTIC
	exfer->ex_isdone = false;
#endif

	/*
	 * Step 1: Initialize sitds.
	 */

	frames = xfer->ux_nframes;
	dma_buf = &xfer->ux_dmabuf;
	offs = 0;

	for (sitd = exfer->ex_sitdstart, i = 0; i < frames;
	    i++, sitd = sitd->xfer_next) {
		KASSERT(sitd != NULL);
		KASSERT(xfer->ux_frlengths[i] <= 0x3ff);

		sitd->sitd.sitd_trans = htole32(EHCI_SITD_ACTIVE |
		    EHCI_SITD_SET_LEN(xfer->ux_frlengths[i]));

		/* Set page0 index and offset - TP and T-offset are set below */
		const bus_addr_t sba = DMAADDR(dma_buf, offs);
		sitd->sitd.sitd_buffer[0] = htole32(BUS_ADDR_LO32(sba));
		sitd->sitd.sitd_buffer_hi[0] = htole32(BUS_ADDR_HI32(sba));

		offs += xfer->ux_frlengths[i];

		const bus_addr_t eba = DMAADDR(dma_buf, offs - 1);
		sitd->sitd.sitd_buffer[1] =
		    htole32(EHCI_SITD_SET_BPTR(BUS_ADDR_LO32(eba)));
		sitd->sitd.sitd_buffer_hi[1] = htole32(BUS_ADDR_HI32(eba));

		u_int huba __diagused = dev->ud_myhsport->up_parent->ud_addr;

#if 0
		if (sc->sc_flags & EHCIF_FREESCALE) {
			// Set hub address to 0 if embedded TT is used.
			if (huba == sc->sc_addr)
				huba = 0;
		}
#endif

		k = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
		dir = UE_GET_DIR(k) ? 1 : 0;
		KASSERT(sitd->sitd.sitd_endp == htole32(
		    EHCI_SITD_SET_ENDPT(UE_GET_ADDR(k)) |
		    EHCI_SITD_SET_DADDR(dev->ud_addr) |
		    EHCI_SITD_SET_PORT(dev->ud_myhsport->up_portno) |
		    EHCI_SITD_SET_HUBA(huba) |
		    EHCI_SITD_SET_DIR(dir)));
		KASSERT(sitd->sitd.sitd_back == htole32(EHCI_LINK_TERMINATE));

		uint8_t sa = 0;
		uint8_t sb = 0;
		u_int temp, tlen;

		if (dir == 0) {	/* OUT */
			temp = 0;
			tlen = xfer->ux_frlengths[i];
			if (tlen <= 188) {
				temp |= 1;	/* T-count = 1, TP = ALL */
				tlen = 1;
			} else {
				tlen += 187;
				tlen /= 188;
				temp |= tlen;	/* T-count = [1..6] */
				temp |= 8;	/* TP = Begin */
			}
			sitd->sitd.sitd_buffer[1] |= htole32(temp);

			tlen += sa;

			if (tlen >= 8) {
				sb = 0;
			} else {
				sb = (1 << tlen);
			}

			sa = (1 << sa);
			sa = (sb - sa) & 0x3F;
			sb = 0;
		} else {
			sb = (-(4 << sa)) & 0xFE;
			sa = (1 << sa) & 0x3F;
			sa = 0x01;
			sb = 0xfc;
		}

		sitd->sitd.sitd_sched = htole32(
		    EHCI_SITD_SET_SMASK(sa) |
		    EHCI_SITD_SET_CMASK(sb)
		    );

		usb_syncmem(&sitd->dma, sitd->offs, sizeof(ehci_sitd_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	} /* End of frame */

	sitd = exfer->ex_sitdend;
	sitd->sitd.sitd_trans |= htole32(EHCI_SITD_IOC);

	usb_syncmem(&sitd->dma, sitd->offs + offsetof(ehci_sitd_t, sitd_trans),
	    sizeof(sitd->sitd.sitd_trans),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	if (xfer->ux_length)
		usb_syncmem(&exfer->ex_xfer.ux_dmabuf, 0, xfer->ux_length,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Part 2: Transfer descriptors have now been set up, now they must
	 * be scheduled into the periodic frame list. Erk. Not wanting to
	 * complicate matters, transfer is denied if the transfer spans
	 * more than the period frame list.
	 */

	/* Start inserting frames */
	if (epipe->isoc.cur_xfers > 0) {
		frindex = epipe->isoc.next_frame;
	} else {
		frindex = EOREAD4(sc, EHCI_FRINDEX);
		frindex = frindex >> 3; /* Erase microframe index */
		frindex += 2;
	}

	if (frindex >= sc->sc_flsize)
		frindex &= (sc->sc_flsize - 1);

	/* Whats the frame interval? */
	i = epipe->pipe.up_endpoint->ue_edesc->bInterval;

	for (sitd = exfer->ex_sitdstart, j = 0; j < frames;
	    j++, sitd = sitd->xfer_next) {
		KASSERT(sitd);

		usb_syncmem(&sc->sc_fldma,
		    sizeof(ehci_link_t) * frindex,
		    sizeof(ehci_link_t),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		sitd->sitd.sitd_next = sc->sc_flist[frindex];
		if (sitd->sitd.sitd_next == 0)
			/*
			 * FIXME: frindex table gets initialized to NULL
			 * or EHCI_NULL?
			 */
			sitd->sitd.sitd_next = EHCI_NULL;

		usb_syncmem(&sitd->dma,
		    sitd->offs + offsetof(ehci_sitd_t, sitd_next),
		    sizeof(ehci_sitd_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		sc->sc_flist[frindex] =
		    htole32(EHCI_LINK_SITD | sitd->physaddr);

		usb_syncmem(&sc->sc_fldma,
		    sizeof(ehci_link_t) * frindex,
		    sizeof(ehci_link_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		sitd->frame_list.next = sc->sc_softsitds[frindex];
		sc->sc_softsitds[frindex] = sitd;
		if (sitd->frame_list.next != NULL)
			sitd->frame_list.next->frame_list.prev = sitd;
		sitd->slot = frindex;
		sitd->frame_list.prev = NULL;

		frindex += i;
		if (frindex >= sc->sc_flsize)
			frindex -= sc->sc_flsize;
	}

	epipe->isoc.cur_xfers++;
	epipe->isoc.next_frame = frindex;

	ehci_add_intr_list(sc, exfer);
	xfer->ux_status = USBD_IN_PROGRESS;

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_fs_isoc_abort(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer = %#jx", (uintptr_t)xfer, 0, 0, 0);
	ehci_abort_isoc_xfer(xfer, USBD_CANCELLED);
}

Static void
ehci_device_fs_isoc_close(struct usbd_pipe *pipe)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("nothing in the pipe to free?", 0, 0, 0, 0);
}

Static void
ehci_device_fs_isoc_done(struct usbd_xfer *xfer)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	epipe->isoc.cur_xfers--;
	ehci_remove_sitd_chain(sc, exfer->ex_itdstart);

	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
}

/* -------------------------------------------------------------------------- */

Static int
ehci_device_isoc_init(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_soft_itd_t *itd, *prev, *start, *stop;
	int i, j, k;
	int frames, ufrperframe;
	int err;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	start = NULL;
	prev = NULL;
	itd = NULL;

	KASSERT(xfer->ux_nframes != 0);
	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(exfer->ex_isdone);

	exfer->ex_type = EX_ISOC;

	/*
	 * Step 1: Allocate and initialize itds, how many do we need?
	 * One per transfer if interval >= 8 microframes, less if we use
	 * multiple microframes per frame.
	 */
	i = epipe->pipe.up_endpoint->ue_edesc->bInterval;
	if (i > 16 || i == 0) {
		/* Spec page 271 says intervals > 16 are invalid */
		DPRINTF("bInterval %jd invalid", i, 0, 0, 0);
		return USBD_INVAL;
	}

	ufrperframe = uimax(1, USB_UFRAMES_PER_FRAME / (1 << (i - 1)));
	frames = howmany(xfer->ux_nframes, ufrperframe);

	for (i = 0, prev = NULL; i < frames; i++, prev = itd) {
		itd = ehci_alloc_itd(sc);
		if (itd == NULL) {
			err = ENOMEM;
			goto fail;
		}

		if (prev != NULL) {
			/* Maybe not as it's updated by the scheduling? */
			prev->itd.itd_next =
			    htole32(itd->physaddr | EHCI_LINK_ITD);

			prev->xfer_next = itd;
		} else {
			start = itd;
		}

		/*
		 * Other special values
		 */
		k = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
		itd->itd.itd_bufr[0] = htole32(
		    EHCI_ITD_SET_EP(UE_GET_ADDR(k)) |
		    EHCI_ITD_SET_DADDR(epipe->pipe.up_dev->ud_addr));

		k = (UE_GET_DIR(epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress))
		    ? 1 : 0;
		j = UGETW(epipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize);
		itd->itd.itd_bufr[1] |= htole32(
		    EHCI_ITD_SET_DIR(k) |
		    EHCI_ITD_SET_MAXPKT(UE_GET_SIZE(j)));

		/* FIXME: handle invalid trans - should be done in openpipe */
		itd->itd.itd_bufr[2] |=
		    htole32(EHCI_ITD_SET_MULTI(UE_GET_TRANS(j)+1));
	} /* End of frame */

	stop = itd;
	stop->xfer_next = NULL;

	exfer->ex_itdstart = start;
	exfer->ex_itdend = stop;

	return 0;
fail:
	mutex_enter(&sc->sc_lock);
	ehci_soft_itd_t *next;
	for (itd = start; itd; itd = next) {
		next = itd->xfer_next;
		ehci_free_itd_locked(sc, itd);
	}
	mutex_exit(&sc->sc_lock);

	return err;

}

Static void
ehci_device_isoc_fini(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_xfer *ex = EHCI_XFER2EXFER(xfer);

	KASSERT(ex->ex_type == EX_ISOC);

	ehci_free_itd_chain(sc, ex->ex_itdstart);
}

Static usbd_status
ehci_device_isoc_transfer(struct usbd_xfer *xfer)
{
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_soft_itd_t *itd, *prev;
	usb_dma_t *dma_buf;
	int i, j;
	int frames, uframes, ufrperframe;
	int trans_count, offs;
	int frindex;

	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	prev = NULL;
	itd = NULL;
	trans_count = 0;

	DPRINTF("xfer %#jx flags %jd", (uintptr_t)xfer, xfer->ux_flags, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	/*
	 * To avoid complication, don't allow a request right now that'll span
	 * the entire frame table. To within 4 frames, to allow some leeway
	 * on either side of where the hc currently is.
	 */
	if ((1 << (epipe->pipe.up_endpoint->ue_edesc->bInterval)) *
			xfer->ux_nframes >= (sc->sc_flsize - 4) * 8) {
		DPRINTF(
		    "isoc descriptor spans entire frametable", 0, 0, 0, 0);
		printf("ehci: isoc descriptor requested that spans the entire frametable, too many frames\n");
		return USBD_INVAL;
	}

	KASSERT(xfer->ux_nframes != 0 && xfer->ux_frlengths);
	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(exfer->ex_isdone);
#ifdef DIAGNOSTIC
	exfer->ex_isdone = false;
#endif

	/*
	 * Step 1: Re-Initialize itds
	 */

	i = epipe->pipe.up_endpoint->ue_edesc->bInterval;
	if (i > 16 || i == 0) {
		/* Spec page 271 says intervals > 16 are invalid */
		DPRINTF("bInterval %jd invalid", i, 0, 0, 0);
		return USBD_INVAL;
	}

	ufrperframe = uimax(1, USB_UFRAMES_PER_FRAME / (1 << (i - 1)));
	frames = howmany(xfer->ux_nframes, ufrperframe);
	uframes = USB_UFRAMES_PER_FRAME / ufrperframe;

	if (frames == 0) {
		DPRINTF("frames == 0", 0, 0, 0, 0);
		return USBD_INVAL;
	}

	dma_buf = &xfer->ux_dmabuf;
	offs = 0;

	itd = exfer->ex_itdstart;
	for (i = 0; i < frames; i++, itd = itd->xfer_next) {
		int froffs = offs;

		if (prev != NULL) {
			prev->itd.itd_next =
			    htole32(itd->physaddr | EHCI_LINK_ITD);
			usb_syncmem(&prev->dma,
			    prev->offs + offsetof(ehci_itd_t, itd_next),
			    sizeof(prev->itd.itd_next), BUS_DMASYNC_POSTWRITE);
			prev->xfer_next = itd;
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

			/*
			 * This gets the initial offset into the first page,
			 * looks how far further along the current uframe
			 * offset is. Works out how many pages that is.
			 */

			itd->itd.itd_ctl[j] = htole32 ( EHCI_ITD_ACTIVE |
			    EHCI_ITD_SET_LEN(xfer->ux_frlengths[trans_count]) |
			    EHCI_ITD_SET_PG(addr) |
			    EHCI_ITD_SET_OFFS(EHCI_PAGE_OFFSET(DMAADDR(dma_buf,offs))));

			offs += xfer->ux_frlengths[trans_count];
			trans_count++;

			if (trans_count >= xfer->ux_nframes) { /*Set IOC*/
				itd->itd.itd_ctl[j] |= htole32(EHCI_ITD_IOC);
				break;
			}
		}

		/*
		 * Step 1.75, set buffer pointers. To simplify matters, all
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
			if (page_offs >= dma_buf->udma_block->size)
				break;

			uint64_t page = DMAADDR(dma_buf, page_offs);
			page = EHCI_PAGE(page);
			itd->itd.itd_bufr[j] = htole32(EHCI_ITD_SET_BPTR(page));
			itd->itd.itd_bufr_hi[j] = htole32(page >> 32);
		}
		/*
		 * Other special values
		 */

		int k = epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
		itd->itd.itd_bufr[0] |= htole32(EHCI_ITD_SET_EP(UE_GET_ADDR(k)) |
		    EHCI_ITD_SET_DADDR(epipe->pipe.up_dev->ud_addr));

		k = (UE_GET_DIR(epipe->pipe.up_endpoint->ue_edesc->bEndpointAddress))
		    ? 1 : 0;
		j = UGETW(epipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize);
		itd->itd.itd_bufr[1] |= htole32(EHCI_ITD_SET_DIR(k) |
		    EHCI_ITD_SET_MAXPKT(UE_GET_SIZE(j)));

		/* FIXME: handle invalid trans */
		itd->itd.itd_bufr[2] |=
		    htole32(EHCI_ITD_SET_MULTI(UE_GET_TRANS(j)+1));

		usb_syncmem(&itd->dma, itd->offs, sizeof(ehci_itd_t),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		prev = itd;
	} /* End of frame */

	if (xfer->ux_length)
		usb_syncmem(&exfer->ex_xfer.ux_dmabuf, 0, xfer->ux_length,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Part 2: Transfer descriptors have now been set up, now they must
	 * be scheduled into the period frame list. Erk. Not wanting to
	 * complicate matters, transfer is denied if the transfer spans
	 * more than the period frame list.
	 */

	/* Start inserting frames */
	if (epipe->isoc.cur_xfers > 0) {
		frindex = epipe->isoc.next_frame;
	} else {
		frindex = EOREAD4(sc, EHCI_FRINDEX);
		frindex = frindex >> 3; /* Erase microframe index */
		frindex += 2;
	}

	if (frindex >= sc->sc_flsize)
		frindex &= (sc->sc_flsize - 1);

	/* What's the frame interval? */
	i = (1 << (epipe->pipe.up_endpoint->ue_edesc->bInterval - 1));
	if (i / USB_UFRAMES_PER_FRAME == 0)
		i = 1;
	else
		i /= USB_UFRAMES_PER_FRAME;

	itd = exfer->ex_itdstart;
	for (j = 0; j < frames; j++) {
		KASSERTMSG(itd != NULL, "frame %d\n", j);

		usb_syncmem(&sc->sc_fldma,
		    sizeof(ehci_link_t) * frindex,
		    sizeof(ehci_link_t),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		itd->itd.itd_next = sc->sc_flist[frindex];
		if (itd->itd.itd_next == 0)
			/*
			 * FIXME: frindex table gets initialized to NULL
			 * or EHCI_NULL?
			 */
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

		itd->frame_list.next = sc->sc_softitds[frindex];
		sc->sc_softitds[frindex] = itd;
		if (itd->frame_list.next != NULL)
			itd->frame_list.next->frame_list.prev = itd;
		itd->slot = frindex;
		itd->frame_list.prev = NULL;

		frindex += i;
		if (frindex >= sc->sc_flsize)
			frindex -= sc->sc_flsize;

		itd = itd->xfer_next;
	}

	epipe->isoc.cur_xfers++;
	epipe->isoc.next_frame = frindex;

	ehci_add_intr_list(sc, exfer);
	xfer->ux_status = USBD_IN_PROGRESS;

	return USBD_IN_PROGRESS;
}

Static void
ehci_device_isoc_abort(struct usbd_xfer *xfer)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("xfer = %#jx", (uintptr_t)xfer, 0, 0, 0);
	ehci_abort_isoc_xfer(xfer, USBD_CANCELLED);
}

Static void
ehci_device_isoc_close(struct usbd_pipe *pipe)
{
	EHCIHIST_FUNC(); EHCIHIST_CALLED();

	DPRINTF("nothing in the pipe to free?", 0, 0, 0, 0);
}

Static void
ehci_device_isoc_done(struct usbd_xfer *xfer)
{
	struct ehci_xfer *exfer = EHCI_XFER2EXFER(xfer);
	ehci_softc_t *sc = EHCI_XFER2SC(xfer);
	struct ehci_pipe *epipe = EHCI_XFER2EPIPE(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	epipe->isoc.cur_xfers--;
	ehci_remove_itd_chain(sc, exfer->ex_sitdstart);
	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
}
