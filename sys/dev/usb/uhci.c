/*	$NetBSD: uhci.c,v 1.310 2022/03/03 06:08:50 riastradh Exp $	*/

/*
 * Copyright (c) 1998, 2004, 2011, 2012, 2016, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, Jared D. McNeill (jmcneill@invisible.ca),
 * Matthew R. Green (mrg@eterna.com.au) and Nick Hudson.
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
 * USB Universal Host Controller driver.
 * Handles e.g. PIIX3 and PIIX4.
 *
 * UHCI spec: http://www.intel.com/technology/usb/spec.htm
 * USB spec: http://www.usb.org/developers/docs/
 * PIIXn spec: ftp://download.intel.com/design/intarch/datashts/29055002.pdf
 *             ftp://download.intel.com/design/intarch/datashts/29056201.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uhci.c,v 1.310 2022/03/03 06:08:50 riastradh Exp $");

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

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/uhcireg.h>
#include <dev/usb/uhcivar.h>
#include <dev/usb/usbroothub.h>
#include <dev/usb/usbhist.h>

/* Use bandwidth reclamation for control transfers. Some devices choke on it. */
/*#define UHCI_CTL_LOOP */

#ifdef UHCI_DEBUG
uhci_softc_t *thesc;
int uhcinoloop = 0;
#endif

#ifdef USB_DEBUG
#ifndef UHCI_DEBUG
#define uhcidebug 0
#else
static int uhcidebug = 0;

SYSCTL_SETUP(sysctl_hw_uhci_setup, "sysctl hw.uhci setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "uhci",
	    SYSCTL_DESCR("uhci global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &uhcidebug, sizeof(uhcidebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* UHCI_DEBUG */
#endif /* USB_DEBUG */

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(uhcidebug,1,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(uhcidebug,N,FMT,A,B,C,D)
#define	UHCIHIST_FUNC()		USBHIST_FUNC()
#define	UHCIHIST_CALLED(name)	USBHIST_CALLED(uhcidebug)

/*
 * The UHCI controller is little endian, so on big endian machines
 * the data stored in memory needs to be swapped.
 */

struct uhci_pipe {
	struct usbd_pipe pipe;
	int nexttoggle;

	u_char aborting;
	struct usbd_xfer *abortstart, abortend;

	/* Info needed for different pipe kinds. */
	union {
		/* Control pipe */
		struct {
			uhci_soft_qh_t *sqh;
			usb_dma_t reqdma;
			uhci_soft_td_t *setup;
			uhci_soft_td_t *stat;
		} ctrl;
		/* Interrupt pipe */
		struct {
			int npoll;
			uhci_soft_qh_t **qhs;
		} intr;
		/* Bulk pipe */
		struct {
			uhci_soft_qh_t *sqh;
		} bulk;
		/* Isochronous pipe */
		struct isoc {
			uhci_soft_td_t **stds;
			int next, inuse;
		} isoc;
	};
};

typedef TAILQ_HEAD(ux_completeq, uhci_xfer) ux_completeq_t;

Static void		uhci_globalreset(uhci_softc_t *);
Static usbd_status	uhci_portreset(uhci_softc_t*, int);
Static void		uhci_reset(uhci_softc_t *);
Static usbd_status	uhci_run(uhci_softc_t *, int, int);
Static uhci_soft_td_t  *uhci_alloc_std(uhci_softc_t *);
Static void		uhci_free_std(uhci_softc_t *, uhci_soft_td_t *);
Static void		uhci_free_std_locked(uhci_softc_t *, uhci_soft_td_t *);
Static uhci_soft_qh_t  *uhci_alloc_sqh(uhci_softc_t *);
Static void		uhci_free_sqh(uhci_softc_t *, uhci_soft_qh_t *);
#if 0
Static void		uhci_enter_ctl_q(uhci_softc_t *, uhci_soft_qh_t *,
			    uhci_intr_info_t *);
Static void		uhci_exit_ctl_q(uhci_softc_t *, uhci_soft_qh_t *);
#endif

#if 0
Static void		uhci_free_std_chain(uhci_softc_t *, uhci_soft_td_t *,
			    uhci_soft_td_t *);
#endif
Static int		uhci_alloc_std_chain(uhci_softc_t *, struct usbd_xfer *,
			    int, int, uhci_soft_td_t **);
Static void		uhci_free_stds(uhci_softc_t *, struct uhci_xfer *);

Static void		uhci_reset_std_chain(uhci_softc_t *, struct usbd_xfer *,
			    int, int, int *, uhci_soft_td_t **);

Static void		uhci_poll_hub(void *);
Static void		uhci_check_intr(uhci_softc_t *, struct uhci_xfer *,
			    ux_completeq_t *);
Static void		uhci_idone(struct uhci_xfer *, ux_completeq_t *);

Static void		uhci_abortx(struct usbd_xfer *);

Static void		uhci_add_ls_ctrl(uhci_softc_t *, uhci_soft_qh_t *);
Static void		uhci_add_hs_ctrl(uhci_softc_t *, uhci_soft_qh_t *);
Static void		uhci_add_bulk(uhci_softc_t *, uhci_soft_qh_t *);
Static void		uhci_remove_ls_ctrl(uhci_softc_t *,uhci_soft_qh_t *);
Static void		uhci_remove_hs_ctrl(uhci_softc_t *,uhci_soft_qh_t *);
Static void		uhci_remove_bulk(uhci_softc_t *,uhci_soft_qh_t *);
Static void		uhci_add_loop(uhci_softc_t *);
Static void		uhci_rem_loop(uhci_softc_t *);

Static usbd_status	uhci_setup_isoc(struct usbd_pipe *);

Static struct usbd_xfer *
			uhci_allocx(struct usbd_bus *, unsigned int);
Static void		uhci_freex(struct usbd_bus *, struct usbd_xfer *);
Static bool		uhci_dying(struct usbd_bus *);
Static void		uhci_get_lock(struct usbd_bus *, kmutex_t **);
Static int		uhci_roothub_ctrl(struct usbd_bus *,
			    usb_device_request_t *, void *, int);

Static int		uhci_device_ctrl_init(struct usbd_xfer *);
Static void		uhci_device_ctrl_fini(struct usbd_xfer *);
Static usbd_status	uhci_device_ctrl_transfer(struct usbd_xfer *);
Static usbd_status	uhci_device_ctrl_start(struct usbd_xfer *);
Static void		uhci_device_ctrl_abort(struct usbd_xfer *);
Static void		uhci_device_ctrl_close(struct usbd_pipe *);
Static void		uhci_device_ctrl_done(struct usbd_xfer *);

Static int		uhci_device_intr_init(struct usbd_xfer *);
Static void		uhci_device_intr_fini(struct usbd_xfer *);
Static usbd_status	uhci_device_intr_transfer(struct usbd_xfer *);
Static usbd_status	uhci_device_intr_start(struct usbd_xfer *);
Static void		uhci_device_intr_abort(struct usbd_xfer *);
Static void		uhci_device_intr_close(struct usbd_pipe *);
Static void		uhci_device_intr_done(struct usbd_xfer *);

Static int		uhci_device_bulk_init(struct usbd_xfer *);
Static void		uhci_device_bulk_fini(struct usbd_xfer *);
Static usbd_status	uhci_device_bulk_transfer(struct usbd_xfer *);
Static usbd_status	uhci_device_bulk_start(struct usbd_xfer *);
Static void		uhci_device_bulk_abort(struct usbd_xfer *);
Static void		uhci_device_bulk_close(struct usbd_pipe *);
Static void		uhci_device_bulk_done(struct usbd_xfer *);

Static int		uhci_device_isoc_init(struct usbd_xfer *);
Static void		uhci_device_isoc_fini(struct usbd_xfer *);
Static usbd_status	uhci_device_isoc_transfer(struct usbd_xfer *);
Static void		uhci_device_isoc_abort(struct usbd_xfer *);
Static void		uhci_device_isoc_close(struct usbd_pipe *);
Static void		uhci_device_isoc_done(struct usbd_xfer *);

Static usbd_status	uhci_root_intr_transfer(struct usbd_xfer *);
Static usbd_status	uhci_root_intr_start(struct usbd_xfer *);
Static void		uhci_root_intr_abort(struct usbd_xfer *);
Static void		uhci_root_intr_close(struct usbd_pipe *);
Static void		uhci_root_intr_done(struct usbd_xfer *);

Static usbd_status	uhci_open(struct usbd_pipe *);
Static void		uhci_poll(struct usbd_bus *);
Static void		uhci_softintr(void *);

Static void		uhci_add_intr(uhci_softc_t *, uhci_soft_qh_t *);
Static void		uhci_remove_intr(uhci_softc_t *, uhci_soft_qh_t *);
Static usbd_status	uhci_device_setintr(uhci_softc_t *,
			    struct uhci_pipe *, int);

Static void		uhci_device_clear_toggle(struct usbd_pipe *);
Static void		uhci_noop(struct usbd_pipe *);

static inline uhci_soft_qh_t *
			uhci_find_prev_qh(uhci_soft_qh_t *, uhci_soft_qh_t *);

#ifdef UHCI_DEBUG
Static void		uhci_dump_all(uhci_softc_t *);
Static void		uhci_dumpregs(uhci_softc_t *);
Static void		uhci_dump_qhs(uhci_soft_qh_t *);
Static void		uhci_dump_qh(uhci_soft_qh_t *);
Static void		uhci_dump_tds(uhci_soft_td_t *);
Static void		uhci_dump_td(uhci_soft_td_t *);
Static void		uhci_dump_ii(struct uhci_xfer *);
void			uhci_dump(void);
#endif

#define UBARR(sc) bus_space_barrier((sc)->iot, (sc)->ioh, 0, (sc)->sc_size, \
			BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)
#define UWRITE1(sc, r, x) \
 do { UBARR(sc); bus_space_write_1((sc)->iot, (sc)->ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)
#define UWRITE2(sc, r, x) \
 do { UBARR(sc); bus_space_write_2((sc)->iot, (sc)->ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)
#define UWRITE4(sc, r, x) \
 do { UBARR(sc); bus_space_write_4((sc)->iot, (sc)->ioh, (r), (x)); \
 } while (/*CONSTCOND*/0)

static __inline uint8_t
UREAD1(uhci_softc_t *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_1(sc->iot, sc->ioh, r);
}

static __inline uint16_t
UREAD2(uhci_softc_t *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_2(sc->iot, sc->ioh, r);
}

#ifdef UHCI_DEBUG
static __inline uint32_t
UREAD4(uhci_softc_t *sc, bus_size_t r)
{

	UBARR(sc);
	return bus_space_read_4(sc->iot, sc->ioh, r);
}
#endif

#define UHCICMD(sc, cmd) UWRITE2(sc, UHCI_CMD, cmd)
#define UHCISTS(sc) UREAD2(sc, UHCI_STS)

#define UHCI_RESET_TIMEOUT 100	/* ms, reset timeout */

#define UHCI_CURFRAME(sc) (UREAD2(sc, UHCI_FRNUM) & UHCI_FRNUM_MASK)

const struct usbd_bus_methods uhci_bus_methods = {
	.ubm_open =	uhci_open,
	.ubm_softint =	uhci_softintr,
	.ubm_dopoll =	uhci_poll,
	.ubm_allocx =	uhci_allocx,
	.ubm_freex =	uhci_freex,
	.ubm_abortx =	uhci_abortx,
	.ubm_dying =	uhci_dying,
	.ubm_getlock =	uhci_get_lock,
	.ubm_rhctrl =	uhci_roothub_ctrl,
};

const struct usbd_pipe_methods uhci_root_intr_methods = {
	.upm_transfer =	uhci_root_intr_transfer,
	.upm_start =	uhci_root_intr_start,
	.upm_abort =	uhci_root_intr_abort,
	.upm_close =	uhci_root_intr_close,
	.upm_cleartoggle =	uhci_noop,
	.upm_done =	uhci_root_intr_done,
};

const struct usbd_pipe_methods uhci_device_ctrl_methods = {
	.upm_init =	uhci_device_ctrl_init,
	.upm_fini =	uhci_device_ctrl_fini,
	.upm_transfer =	uhci_device_ctrl_transfer,
	.upm_start =	uhci_device_ctrl_start,
	.upm_abort =	uhci_device_ctrl_abort,
	.upm_close =	uhci_device_ctrl_close,
	.upm_cleartoggle =	uhci_noop,
	.upm_done =	uhci_device_ctrl_done,
};

const struct usbd_pipe_methods uhci_device_intr_methods = {
	.upm_init =	uhci_device_intr_init,
	.upm_fini =	uhci_device_intr_fini,
	.upm_transfer =	uhci_device_intr_transfer,
	.upm_start =	uhci_device_intr_start,
	.upm_abort =	uhci_device_intr_abort,
	.upm_close =	uhci_device_intr_close,
	.upm_cleartoggle =	uhci_device_clear_toggle,
	.upm_done =	uhci_device_intr_done,
};

const struct usbd_pipe_methods uhci_device_bulk_methods = {
	.upm_init =	uhci_device_bulk_init,
	.upm_fini =	uhci_device_bulk_fini,
	.upm_transfer =	uhci_device_bulk_transfer,
	.upm_start =	uhci_device_bulk_start,
	.upm_abort =	uhci_device_bulk_abort,
	.upm_close =	uhci_device_bulk_close,
	.upm_cleartoggle =	uhci_device_clear_toggle,
	.upm_done =	uhci_device_bulk_done,
};

const struct usbd_pipe_methods uhci_device_isoc_methods = {
	.upm_init =	uhci_device_isoc_init,
	.upm_fini =	uhci_device_isoc_fini,
	.upm_transfer =	uhci_device_isoc_transfer,
	.upm_abort =	uhci_device_isoc_abort,
	.upm_close =	uhci_device_isoc_close,
	.upm_cleartoggle =	uhci_noop,
	.upm_done =	uhci_device_isoc_done,
};

static inline void
uhci_add_intr_list(uhci_softc_t *sc, struct uhci_xfer *ux)
{

	TAILQ_INSERT_TAIL(&sc->sc_intrhead, ux, ux_list);
}

static inline void
uhci_del_intr_list(uhci_softc_t *sc, struct uhci_xfer *ux)
{

	TAILQ_REMOVE(&sc->sc_intrhead, ux, ux_list);
}

static inline uhci_soft_qh_t *
uhci_find_prev_qh(uhci_soft_qh_t *pqh, uhci_soft_qh_t *sqh)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(15, "pqh=%#jx sqh=%#jx", (uintptr_t)pqh, (uintptr_t)sqh, 0, 0);

	for (; pqh->hlink != sqh; pqh = pqh->hlink) {
#if defined(DIAGNOSTIC) || defined(UHCI_DEBUG)
		usb_syncmem(&pqh->dma,
		    pqh->offs + offsetof(uhci_qh_t, qh_hlink),
		    sizeof(pqh->qh.qh_hlink),
		    BUS_DMASYNC_POSTWRITE);
		if (le32toh(pqh->qh.qh_hlink) & UHCI_PTR_T) {
			printf("%s: QH not found\n", __func__);
			return NULL;
		}
#endif
	}
	return pqh;
}

void
uhci_globalreset(uhci_softc_t *sc)
{
	UHCICMD(sc, UHCI_CMD_GRESET);	/* global reset */
	usb_delay_ms(&sc->sc_bus, USB_BUS_RESET_DELAY); /* wait a little */
	UHCICMD(sc, 0);			/* do nothing */
}

int
uhci_init(uhci_softc_t *sc)
{
	int i, j;
	uhci_soft_qh_t *clsqh, *chsqh, *bsqh, *sqh, *lsqh;
	uhci_soft_td_t *std;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

#ifdef UHCI_DEBUG
	thesc = sc;

	if (uhcidebug >= 2)
		uhci_dumpregs(sc);
#endif

	sc->sc_suspend = PWR_RESUME;

	UWRITE2(sc, UHCI_INTR, 0);		/* disable interrupts */
	uhci_globalreset(sc);			/* reset the controller */
	uhci_reset(sc);

	/* Allocate and initialize real frame array. */
	int err = usb_allocmem(sc->sc_bus.ub_dmatag,
	    UHCI_FRAMELIST_COUNT * sizeof(uhci_physaddr_t),
	    UHCI_FRAMELIST_ALIGN, USBMALLOC_COHERENT, &sc->sc_dma);
	if (err)
		return err;
	sc->sc_pframes = KERNADDR(&sc->sc_dma, 0);
	/* set frame number to 0 */
	UWRITE2(sc, UHCI_FRNUM, 0);
	/* set frame list */
	UWRITE4(sc, UHCI_FLBASEADDR, DMAADDR(&sc->sc_dma, 0));

	/* Initialise mutex early for uhci_alloc_* */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);

	/*
	 * Allocate a TD, inactive, that hangs from the last QH.
	 * This is to avoid a bug in the PIIX that makes it run berserk
	 * otherwise.
	 */
	std = uhci_alloc_std(sc);
	if (std == NULL)
		return ENOMEM;
	std->link.std = NULL;
	std->td.td_link = htole32(UHCI_PTR_T);
	std->td.td_status = htole32(0); /* inactive */
	std->td.td_token = htole32(0);
	std->td.td_buffer = htole32(0);
	usb_syncmem(&std->dma, std->offs, sizeof(std->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate the dummy QH marking the end and used for looping the QHs.*/
	lsqh = uhci_alloc_sqh(sc);
	if (lsqh == NULL)
		goto fail1;
	lsqh->hlink = NULL;
	lsqh->qh.qh_hlink = htole32(UHCI_PTR_T);	/* end of QH chain */
	lsqh->elink = std;
	lsqh->qh.qh_elink = htole32(std->physaddr | UHCI_PTR_TD);
	sc->sc_last_qh = lsqh;
	usb_syncmem(&lsqh->dma, lsqh->offs, sizeof(lsqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate the dummy QH where bulk traffic will be queued. */
	bsqh = uhci_alloc_sqh(sc);
	if (bsqh == NULL)
		goto fail2;
	bsqh->hlink = lsqh;
	bsqh->qh.qh_hlink = htole32(lsqh->physaddr | UHCI_PTR_QH);
	bsqh->elink = NULL;
	bsqh->qh.qh_elink = htole32(UHCI_PTR_T);
	sc->sc_bulk_start = sc->sc_bulk_end = bsqh;
	usb_syncmem(&bsqh->dma, bsqh->offs, sizeof(bsqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate dummy QH where high speed control traffic will be queued. */
	chsqh = uhci_alloc_sqh(sc);
	if (chsqh == NULL)
		goto fail3;
	chsqh->hlink = bsqh;
	chsqh->qh.qh_hlink = htole32(bsqh->physaddr | UHCI_PTR_QH);
	chsqh->elink = NULL;
	chsqh->qh.qh_elink = htole32(UHCI_PTR_T);
	sc->sc_hctl_start = sc->sc_hctl_end = chsqh;
	usb_syncmem(&chsqh->dma, chsqh->offs, sizeof(chsqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate dummy QH where control traffic will be queued. */
	clsqh = uhci_alloc_sqh(sc);
	if (clsqh == NULL)
		goto fail4;
	clsqh->hlink = chsqh;
	clsqh->qh.qh_hlink = htole32(chsqh->physaddr | UHCI_PTR_QH);
	clsqh->elink = NULL;
	clsqh->qh.qh_elink = htole32(UHCI_PTR_T);
	sc->sc_lctl_start = sc->sc_lctl_end = clsqh;
	usb_syncmem(&clsqh->dma, clsqh->offs, sizeof(clsqh->qh),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/*
	 * Make all (virtual) frame list pointers point to the interrupt
	 * queue heads and the interrupt queue heads at the control
	 * queue head and point the physical frame list to the virtual.
	 */
	for (i = 0; i < UHCI_VFRAMELIST_COUNT; i++) {
		std = uhci_alloc_std(sc);
		sqh = uhci_alloc_sqh(sc);
		if (std == NULL || sqh == NULL)
			return USBD_NOMEM;
		std->link.sqh = sqh;
		std->td.td_link = htole32(sqh->physaddr | UHCI_PTR_QH);
		std->td.td_status = htole32(UHCI_TD_IOS); /* iso, inactive */
		std->td.td_token = htole32(0);
		std->td.td_buffer = htole32(0);
		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		sqh->hlink = clsqh;
		sqh->qh.qh_hlink = htole32(clsqh->physaddr | UHCI_PTR_QH);
		sqh->elink = NULL;
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		sc->sc_vframes[i].htd = std;
		sc->sc_vframes[i].etd = std;
		sc->sc_vframes[i].hqh = sqh;
		sc->sc_vframes[i].eqh = sqh;
		for (j = i;
		     j < UHCI_FRAMELIST_COUNT;
		     j += UHCI_VFRAMELIST_COUNT)
			sc->sc_pframes[j] = htole32(std->physaddr);
	}
	usb_syncmem(&sc->sc_dma, 0,
	    UHCI_FRAMELIST_COUNT * sizeof(uhci_physaddr_t),
	    BUS_DMASYNC_PREWRITE);


	TAILQ_INIT(&sc->sc_intrhead);

	sc->sc_xferpool = pool_cache_init(sizeof(struct uhci_xfer), 0, 0, 0,
	    "uhcixfer", NULL, IPL_USB, NULL, NULL, NULL);

	callout_init(&sc->sc_poll_handle, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_poll_handle, uhci_poll_hub, sc);

	/* Set up the bus struct. */
	sc->sc_bus.ub_methods = &uhci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct uhci_pipe);
	sc->sc_bus.ub_usedma = true;
	sc->sc_bus.ub_dmaflags = USBMALLOC_MULTISEG;

	UHCICMD(sc, UHCI_CMD_MAXP); /* Assume 64 byte packets at frame end */

	DPRINTF("Enabling...", 0, 0, 0, 0);

	err = uhci_run(sc, 1, 0);		/* and here we go... */
	UWRITE2(sc, UHCI_INTR, UHCI_INTR_TOCRCIE | UHCI_INTR_RIE |
		UHCI_INTR_IOCE | UHCI_INTR_SPIE);	/* enable interrupts */
	return err;

fail4:
	uhci_free_sqh(sc, chsqh);
fail3:
	uhci_free_sqh(sc, lsqh);
fail2:
	uhci_free_sqh(sc, lsqh);
fail1:
	uhci_free_std(sc, std);

	return ENOMEM;
}

int
uhci_activate(device_t self, enum devact act)
{
	struct uhci_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
uhci_childdet(device_t self, device_t child)
{
	struct uhci_softc *sc = device_private(self);

	KASSERT(sc->sc_child == child);
	sc->sc_child = NULL;
}

int
uhci_detach(struct uhci_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	if (rv != 0)
		return rv;

	callout_halt(&sc->sc_poll_handle, NULL);
	callout_destroy(&sc->sc_poll_handle);

	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);

	pool_cache_destroy(sc->sc_xferpool);

	/* XXX free other data structures XXX */

	return rv;
}

struct usbd_xfer *
uhci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct uhci_softc *sc = UHCI_BUS2SC(bus);
	struct usbd_xfer *xfer;

	xfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct uhci_xfer));

#ifdef DIAGNOSTIC
		struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
		uxfer->ux_isdone = true;
		xfer->ux_state = XFER_BUSY;
#endif
	}
	return xfer;
}

void
uhci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct uhci_softc *sc = UHCI_BUS2SC(bus);
	struct uhci_xfer *uxfer __diagused = UHCI_XFER2UXFER(xfer);

	KASSERTMSG(xfer->ux_state == XFER_BUSY ||
	    xfer->ux_status == USBD_NOT_STARTED,
	    "xfer %p state %d\n", xfer, xfer->ux_state);
	KASSERTMSG(uxfer->ux_isdone || xfer->ux_status == USBD_NOT_STARTED,
	    "xfer %p not done\n", xfer);
#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_FREE;
#endif
	pool_cache_put(sc->sc_xferpool, xfer);
}

Static bool
uhci_dying(struct usbd_bus *bus)
{
	struct uhci_softc *sc = UHCI_BUS2SC(bus);

	return sc->sc_dying;
}

Static void
uhci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct uhci_softc *sc = UHCI_BUS2SC(bus);

	*lock = &sc->sc_lock;
}


/*
 * Handle suspend/resume.
 *
 * We need to switch to polling mode here, because this routine is
 * called from an interrupt context.  This is all right since we
 * are almost suspended anyway.
 */
bool
uhci_resume(device_t dv, const pmf_qual_t *qual)
{
	uhci_softc_t *sc = device_private(dv);
	int cmd;

	mutex_spin_enter(&sc->sc_intr_lock);

	cmd = UREAD2(sc, UHCI_CMD);
	sc->sc_bus.ub_usepolling++;
	UWRITE2(sc, UHCI_INTR, 0);
	uhci_globalreset(sc);
	uhci_reset(sc);
	if (cmd & UHCI_CMD_RS)
		uhci_run(sc, 0, 1);

	/* restore saved state */
	UWRITE4(sc, UHCI_FLBASEADDR, DMAADDR(&sc->sc_dma, 0));
	UWRITE2(sc, UHCI_FRNUM, sc->sc_saved_frnum);
	UWRITE1(sc, UHCI_SOF, sc->sc_saved_sof);

	UHCICMD(sc, cmd | UHCI_CMD_FGR); /* force resume */
	usb_delay_ms_locked(&sc->sc_bus, USB_RESUME_DELAY, &sc->sc_intr_lock);
	UHCICMD(sc, cmd & ~UHCI_CMD_EGSM); /* back to normal */
	UWRITE2(sc, UHCI_INTR, UHCI_INTR_TOCRCIE |
	    UHCI_INTR_RIE | UHCI_INTR_IOCE | UHCI_INTR_SPIE);
	UHCICMD(sc, UHCI_CMD_MAXP);
	uhci_run(sc, 1, 1); /* and start traffic again */
	usb_delay_ms_locked(&sc->sc_bus, USB_RESUME_RECOVERY, &sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling--;
	if (sc->sc_intr_xfer != NULL)
		callout_schedule(&sc->sc_poll_handle, sc->sc_ival);
#ifdef UHCI_DEBUG
	if (uhcidebug >= 2)
		uhci_dumpregs(sc);
#endif

	sc->sc_suspend = PWR_RESUME;
	mutex_spin_exit(&sc->sc_intr_lock);

	return true;
}

bool
uhci_suspend(device_t dv, const pmf_qual_t *qual)
{
	uhci_softc_t *sc = device_private(dv);
	int cmd;

	mutex_spin_enter(&sc->sc_intr_lock);

	cmd = UREAD2(sc, UHCI_CMD);

#ifdef UHCI_DEBUG
	if (uhcidebug >= 2)
		uhci_dumpregs(sc);
#endif
	sc->sc_suspend = PWR_SUSPEND;
	if (sc->sc_intr_xfer != NULL)
		callout_halt(&sc->sc_poll_handle, &sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling++;

	uhci_run(sc, 0, 1); /* stop the controller */
	cmd &= ~UHCI_CMD_RS;

	/* save some state if BIOS doesn't */
	sc->sc_saved_frnum = UREAD2(sc, UHCI_FRNUM);
	sc->sc_saved_sof = UREAD1(sc, UHCI_SOF);

	UWRITE2(sc, UHCI_INTR, 0); /* disable intrs */

	UHCICMD(sc, cmd | UHCI_CMD_EGSM); /* enter suspend */
	usb_delay_ms_locked(&sc->sc_bus, USB_RESUME_WAIT, &sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling--;

	mutex_spin_exit(&sc->sc_intr_lock);

	return true;
}

#ifdef UHCI_DEBUG
Static void
uhci_dumpregs(uhci_softc_t *sc)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTF("cmd =%04jx  sts    =%04jx  intr   =%04jx  frnum =%04jx",
	    UREAD2(sc, UHCI_CMD), UREAD2(sc, UHCI_STS),
	    UREAD2(sc, UHCI_INTR), UREAD2(sc, UHCI_FRNUM));
	DPRINTF("sof =%04jx  portsc1=%04jx  portsc2=%04jx  flbase=%08jx",
	    UREAD1(sc, UHCI_SOF), UREAD2(sc, UHCI_PORTSC1),
	    UREAD2(sc, UHCI_PORTSC2), UREAD4(sc, UHCI_FLBASEADDR));
}

void
uhci_dump_td(uhci_soft_td_t *p)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	usb_syncmem(&p->dma, p->offs, sizeof(p->td),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	DPRINTF("TD(%#jx) at 0x%08jx", (uintptr_t)p, p->physaddr, 0, 0);
	DPRINTF("   link=0x%08jx status=0x%08jx "
	    "token=0x%08x buffer=0x%08x",
	     le32toh(p->td.td_link),
	     le32toh(p->td.td_status),
	     le32toh(p->td.td_token),
	     le32toh(p->td.td_buffer));

	DPRINTF("bitstuff=%jd crcto   =%jd nak     =%jd babble  =%jd",
	    !!(le32toh(p->td.td_status) & UHCI_TD_BITSTUFF),
	    !!(le32toh(p->td.td_status) & UHCI_TD_CRCTO),
	    !!(le32toh(p->td.td_status) & UHCI_TD_NAK),
	    !!(le32toh(p->td.td_status) & UHCI_TD_BABBLE));
	DPRINTF("dbuffer =%jd stalled =%jd active  =%jd ioc     =%jd",
	    !!(le32toh(p->td.td_status) & UHCI_TD_DBUFFER),
	    !!(le32toh(p->td.td_status) & UHCI_TD_STALLED),
	    !!(le32toh(p->td.td_status) & UHCI_TD_ACTIVE),
	    !!(le32toh(p->td.td_status) & UHCI_TD_IOC));
	DPRINTF("ios     =%jd ls      =%jd spd     =%jd",
	    !!(le32toh(p->td.td_status) & UHCI_TD_IOS),
	    !!(le32toh(p->td.td_status) & UHCI_TD_LS),
	    !!(le32toh(p->td.td_status) & UHCI_TD_SPD), 0);
	DPRINTF("errcnt  =%d actlen  =%d pid=%02x",
	    UHCI_TD_GET_ERRCNT(le32toh(p->td.td_status)),
	    UHCI_TD_GET_ACTLEN(le32toh(p->td.td_status)),
	    UHCI_TD_GET_PID(le32toh(p->td.td_token)), 0);
	DPRINTF("addr=%jd  endpt=%jd  D=%jd  maxlen=%jd,",
	    UHCI_TD_GET_DEVADDR(le32toh(p->td.td_token)),
	    UHCI_TD_GET_ENDPT(le32toh(p->td.td_token)),
	    UHCI_TD_GET_DT(le32toh(p->td.td_token)),
	    UHCI_TD_GET_MAXLEN(le32toh(p->td.td_token)));
}

void
uhci_dump_qh(uhci_soft_qh_t *sqh)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	DPRINTF("QH(%#jx) at 0x%08jx: hlink=%08jx elink=%08jx", (uintptr_t)sqh,
	    (int)sqh->physaddr, le32toh(sqh->qh.qh_hlink),
	    le32toh(sqh->qh.qh_elink));

	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh), BUS_DMASYNC_PREREAD);
}


#if 1
void
uhci_dump(void)
{
	uhci_dump_all(thesc);
}
#endif

void
uhci_dump_all(uhci_softc_t *sc)
{
	uhci_dumpregs(sc);
	/*printf("framelist[i].link = %08x\n", sc->sc_framelist[0].link);*/
	uhci_dump_qhs(sc->sc_lctl_start);
}


void
uhci_dump_qhs(uhci_soft_qh_t *sqh)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	uhci_dump_qh(sqh);

	/*
	 * uhci_dump_qhs displays all the QHs and TDs from the given QH onwards
	 * Traverses sideways first, then down.
	 *
	 * QH1
	 * QH2
	 * No QH
	 * TD2.1
	 * TD2.2
	 * TD1.1
	 * etc.
	 *
	 * TD2.x being the TDs queued at QH2 and QH1 being referenced from QH1.
	 */

	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	if (sqh->hlink != NULL && !(le32toh(sqh->qh.qh_hlink) & UHCI_PTR_T))
		uhci_dump_qhs(sqh->hlink);
	else
		DPRINTF("No QH", 0, 0, 0, 0);
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh), BUS_DMASYNC_PREREAD);

	if (sqh->elink != NULL && !(le32toh(sqh->qh.qh_elink) & UHCI_PTR_T))
		uhci_dump_tds(sqh->elink);
	else
		DPRINTF("No QH", 0, 0, 0, 0);
}

void
uhci_dump_tds(uhci_soft_td_t *std)
{
	uhci_soft_td_t *td;
	int stop;

	for (td = std; td != NULL; td = td->link.std) {
		uhci_dump_td(td);

		/*
		 * Check whether the link pointer in this TD marks
		 * the link pointer as end of queue. This avoids
		 * printing the free list in case the queue/TD has
		 * already been moved there (seatbelt).
		 */
		usb_syncmem(&td->dma, td->offs + offsetof(uhci_td_t, td_link),
		    sizeof(td->td.td_link),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		stop = (le32toh(td->td.td_link) & UHCI_PTR_T ||
			le32toh(td->td.td_link) == 0);
		usb_syncmem(&td->dma, td->offs + offsetof(uhci_td_t, td_link),
		    sizeof(td->td.td_link), BUS_DMASYNC_PREREAD);
		if (stop)
			break;
	}
}

Static void
uhci_dump_ii(struct uhci_xfer *ux)
{
	struct usbd_pipe *pipe;
	usb_endpoint_descriptor_t *ed;
	struct usbd_device *dev;

	if (ux == NULL) {
		printf("ux NULL\n");
		return;
	}
	pipe = ux->ux_xfer.ux_pipe;
	if (pipe == NULL) {
		printf("ux %p: done=%d pipe=NULL\n", ux, ux->ux_isdone);
		return;
	}
	if (pipe->up_endpoint == NULL) {
		printf("ux %p: done=%d pipe=%p pipe->up_endpoint=NULL\n",
		       ux, ux->ux_isdone, pipe);
		return;
	}
	if (pipe->up_dev == NULL) {
		printf("ux %p: done=%d pipe=%p pipe->up_dev=NULL\n",
		       ux, ux->ux_isdone, pipe);
		return;
	}
	ed = pipe->up_endpoint->ue_edesc;
	dev = pipe->up_dev;
	printf("ux %p: done=%d dev=%p vid=0x%04x pid=0x%04x addr=%d pipe=%p ep=0x%02x attr=0x%02x\n",
	       ux, ux->ux_isdone, dev,
	       UGETW(dev->ud_ddesc.idVendor),
	       UGETW(dev->ud_ddesc.idProduct),
	       dev->ud_addr, pipe,
	       ed->bEndpointAddress, ed->bmAttributes);
}

void uhci_dump_iis(struct uhci_softc *sc);
void
uhci_dump_iis(struct uhci_softc *sc)
{
	struct uhci_xfer *ux;

	printf("interrupt list:\n");
	TAILQ_FOREACH(ux, &sc->sc_intrhead, ux_list)
		uhci_dump_ii(ux);
}

void iidump(void);
void iidump(void) { uhci_dump_iis(thesc); }

#endif

/*
 * This routine is executed periodically and simulates interrupts
 * from the root controller interrupt pipe for port status change.
 */
void
uhci_poll_hub(void *addr)
{
	struct uhci_softc *sc = addr;
	struct usbd_xfer *xfer;
	u_char *p;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);

	/*
	 * If the intr xfer has completed or been synchronously
	 * aborted, we have nothing to do.
	 */
	xfer = sc->sc_intr_xfer;
	if (xfer == NULL)
		goto out;
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	/*
	 * If the intr xfer for which we were scheduled is done, and
	 * another intr xfer has been submitted, let that one be dealt
	 * with when the callout fires again.
	 *
	 * The call to callout_pending is racy, but the transition
	 * from pending to invoking happens atomically.  The
	 * callout_ack ensures callout_invoking does not return true
	 * due to this invocation of the callout; the lock ensures the
	 * next invocation of the callout cannot callout_ack (unless it
	 * had already run to completion and nulled sc->sc_intr_xfer,
	 * in which case would have bailed out already).
	 */
	callout_ack(&sc->sc_poll_handle);
	if (callout_pending(&sc->sc_poll_handle) ||
	    callout_invoking(&sc->sc_poll_handle))
		goto out;

	/*
	 * Check flags for the two interrupt ports, and set them in the
	 * buffer if an interrupt arrived; otherwise arrange .
	 */
	p = xfer->ux_buf;
	p[0] = 0;
	if (UREAD2(sc, UHCI_PORTSC1) & (UHCI_PORTSC_CSC|UHCI_PORTSC_OCIC))
		p[0] |= 1<<1;
	if (UREAD2(sc, UHCI_PORTSC2) & (UHCI_PORTSC_CSC|UHCI_PORTSC_OCIC))
		p[0] |= 1<<2;
	if (p[0] == 0) {
		/*
		 * No change -- try again in a while, unless we're
		 * suspending, in which case we'll try again after
		 * resume.
		 */
		if (sc->sc_suspend != PWR_SUSPEND)
			callout_schedule(&sc->sc_poll_handle, sc->sc_ival);
		goto out;
	}

	/*
	 * Interrupt completed, and the xfer has not been completed or
	 * synchronously aborted.  Complete the xfer now.
	 */
	xfer->ux_actlen = 1;
	xfer->ux_status = USBD_NORMAL_COMPLETION;
#ifdef DIAGNOSTIC
	UHCI_XFER2UXFER(xfer)->ux_isdone = true;
#endif
	usb_transfer_complete(xfer);

out:	mutex_exit(&sc->sc_lock);
}

void
uhci_root_intr_done(struct usbd_xfer *xfer)
{
	struct uhci_softc *sc = UHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Claim the xfer so it doesn't get completed again.  */
	KASSERT(sc->sc_intr_xfer == xfer);
	KASSERT(xfer->ux_status != USBD_IN_PROGRESS);
	sc->sc_intr_xfer = NULL;
}

/*
 * Let the last QH loop back to the high speed control transfer QH.
 * This is what intel calls "bandwidth reclamation" and improves
 * USB performance a lot for some devices.
 * If we are already looping, just count it.
 */
void
uhci_add_loop(uhci_softc_t *sc)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

#ifdef UHCI_DEBUG
	if (uhcinoloop)
		return;
#endif
	if (++sc->sc_loops == 1) {
		DPRINTFN(5, "add loop", 0, 0, 0, 0);
		/* Note, we don't loop back the soft pointer. */
		sc->sc_last_qh->qh.qh_hlink =
		    htole32(sc->sc_hctl_start->physaddr | UHCI_PTR_QH);
		usb_syncmem(&sc->sc_last_qh->dma,
		    sc->sc_last_qh->offs + offsetof(uhci_qh_t, qh_hlink),
		    sizeof(sc->sc_last_qh->qh.qh_hlink),
		    BUS_DMASYNC_PREWRITE);
	}
}

void
uhci_rem_loop(uhci_softc_t *sc)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

#ifdef UHCI_DEBUG
	if (uhcinoloop)
		return;
#endif
	if (--sc->sc_loops == 0) {
		DPRINTFN(5, "remove loop", 0, 0, 0, 0);
		sc->sc_last_qh->qh.qh_hlink = htole32(UHCI_PTR_T);
		usb_syncmem(&sc->sc_last_qh->dma,
		    sc->sc_last_qh->offs + offsetof(uhci_qh_t, qh_hlink),
		    sizeof(sc->sc_last_qh->qh.qh_hlink),
		    BUS_DMASYNC_PREWRITE);
	}
}

/* Add high speed control QH, called with lock held. */
void
uhci_add_hs_ctrl(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *eqh;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);
	eqh = sc->sc_hctl_end;
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink),
	    BUS_DMASYNC_POSTWRITE);
	sqh->hlink       = eqh->hlink;
	sqh->qh.qh_hlink = eqh->qh.qh_hlink;
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREWRITE);
	eqh->hlink       = sqh;
	eqh->qh.qh_hlink = htole32(sqh->physaddr | UHCI_PTR_QH);
	sc->sc_hctl_end = sqh;
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
#ifdef UHCI_CTL_LOOP
	uhci_add_loop(sc);
#endif
}

/* Remove high speed control QH, called with lock held. */
void
uhci_remove_hs_ctrl(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *pqh;
	uint32_t elink;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);
#ifdef UHCI_CTL_LOOP
	uhci_rem_loop(sc);
#endif
	/*
	 * The T bit should be set in the elink of the QH so that the HC
	 * doesn't follow the pointer.  This condition may fail if the
	 * the transferred packet was short so that the QH still points
	 * at the last used TD.
	 * In this case we set the T bit and wait a little for the HC
	 * to stop looking at the TD.
	 * Note that if the TD chain is large enough, the controller
	 * may still be looking at the chain at the end of this function.
	 * uhci_free_std_chain() will make sure the controller stops
	 * looking at it quickly, but until then we should not change
	 * sqh->hlink.
	 */
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	elink = le32toh(sqh->qh.qh_elink);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink), BUS_DMASYNC_PREREAD);
	if (!(elink & UHCI_PTR_T)) {
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		delay(UHCI_QH_REMOVE_DELAY);
	}

	pqh = uhci_find_prev_qh(sc->sc_hctl_start, sqh);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(sqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	pqh->hlink = sqh->hlink;
	pqh->qh.qh_hlink = sqh->qh.qh_hlink;
	usb_syncmem(&pqh->dma, pqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(pqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	delay(UHCI_QH_REMOVE_DELAY);
	if (sc->sc_hctl_end == sqh)
		sc->sc_hctl_end = pqh;
}

/* Add low speed control QH, called with lock held. */
void
uhci_add_ls_ctrl(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *eqh;

	KASSERT(mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);

	eqh = sc->sc_lctl_end;
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	sqh->hlink = eqh->hlink;
	sqh->qh.qh_hlink = eqh->qh.qh_hlink;
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREWRITE);
	eqh->hlink = sqh;
	eqh->qh.qh_hlink = htole32(sqh->physaddr | UHCI_PTR_QH);
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	sc->sc_lctl_end = sqh;
}

/* Remove low speed control QH, called with lock held. */
void
uhci_remove_ls_ctrl(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *pqh;
	uint32_t elink;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);

	/* See comment in uhci_remove_hs_ctrl() */
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	elink = le32toh(sqh->qh.qh_elink);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink), BUS_DMASYNC_PREREAD);
	if (!(elink & UHCI_PTR_T)) {
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		delay(UHCI_QH_REMOVE_DELAY);
	}
	pqh = uhci_find_prev_qh(sc->sc_lctl_start, sqh);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(sqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	pqh->hlink = sqh->hlink;
	pqh->qh.qh_hlink = sqh->qh.qh_hlink;
	usb_syncmem(&pqh->dma, pqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(pqh->qh.qh_hlink),
	    BUS_DMASYNC_PREWRITE);
	delay(UHCI_QH_REMOVE_DELAY);
	if (sc->sc_lctl_end == sqh)
		sc->sc_lctl_end = pqh;
}

/* Add bulk QH, called with lock held. */
void
uhci_add_bulk(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *eqh;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);

	eqh = sc->sc_bulk_end;
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	sqh->hlink = eqh->hlink;
	sqh->qh.qh_hlink = eqh->qh.qh_hlink;
	usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
	    BUS_DMASYNC_PREWRITE);
	eqh->hlink = sqh;
	eqh->qh.qh_hlink = htole32(sqh->physaddr | UHCI_PTR_QH);
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	sc->sc_bulk_end = sqh;
	uhci_add_loop(sc);
}

/* Remove bulk QH, called with lock held. */
void
uhci_remove_bulk(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	uhci_soft_qh_t *pqh;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(10, "sqh %#jx", (uintptr_t)sqh, 0, 0, 0);

	uhci_rem_loop(sc);
	/* See comment in uhci_remove_hs_ctrl() */
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	if (!(sqh->qh.qh_elink & htole32(UHCI_PTR_T))) {
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		delay(UHCI_QH_REMOVE_DELAY);
	}
	pqh = uhci_find_prev_qh(sc->sc_bulk_start, sqh);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(sqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	pqh->hlink       = sqh->hlink;
	pqh->qh.qh_hlink = sqh->qh.qh_hlink;
	usb_syncmem(&pqh->dma, pqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(pqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	delay(UHCI_QH_REMOVE_DELAY);
	if (sc->sc_bulk_end == sqh)
		sc->sc_bulk_end = pqh;
}

Static int uhci_intr1(uhci_softc_t *);

int
uhci_intr(void *arg)
{
	uhci_softc_t *sc = arg;
	int ret = 0;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	if (sc->sc_bus.ub_usepolling || UREAD2(sc, UHCI_INTR) == 0) {
		DPRINTFN(16, "ignored interrupt while polling", 0, 0, 0, 0);
		goto done;
	}

	ret = uhci_intr1(sc);

 done:
	mutex_spin_exit(&sc->sc_intr_lock);
	return ret;
}

int
uhci_intr1(uhci_softc_t *sc)
{
	int status;
	int ack;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

#ifdef UHCI_DEBUG
	if (uhcidebug >= 15) {
		DPRINTF("sc %#jx", (uintptr_t)sc, 0, 0, 0);
		uhci_dumpregs(sc);
	}
#endif

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	status = UREAD2(sc, UHCI_STS) & UHCI_STS_ALLINTRS;
	/* Check if the interrupt was for us. */
	if (status == 0)
		return 0;

	if (sc->sc_suspend != PWR_RESUME) {
#ifdef DIAGNOSTIC
		printf("%s: interrupt while not operating ignored\n",
		       device_xname(sc->sc_dev));
#endif
		UWRITE2(sc, UHCI_STS, status); /* acknowledge the ints */
		return 0;
	}

	ack = 0;
	if (status & UHCI_STS_USBINT)
		ack |= UHCI_STS_USBINT;
	if (status & UHCI_STS_USBEI)
		ack |= UHCI_STS_USBEI;
	if (status & UHCI_STS_RD) {
		ack |= UHCI_STS_RD;
#ifdef UHCI_DEBUG
		printf("%s: resume detect\n", device_xname(sc->sc_dev));
#endif
	}
	if (status & UHCI_STS_HSE) {
		ack |= UHCI_STS_HSE;
		printf("%s: host system error\n", device_xname(sc->sc_dev));
	}
	if (status & UHCI_STS_HCPE) {
		ack |= UHCI_STS_HCPE;
		printf("%s: host controller process error\n",
		       device_xname(sc->sc_dev));
	}

	/* When HCHalted=1 and Run/Stop=0 , it is normal */
	if ((status & UHCI_STS_HCH) && (UREAD2(sc, UHCI_CMD) & UHCI_CMD_RS)) {
		/* no acknowledge needed */
		if (!sc->sc_dying) {
			printf("%s: host controller halted\n",
			    device_xname(sc->sc_dev));
#ifdef UHCI_DEBUG
			uhci_dump_all(sc);
#endif
		}
		sc->sc_dying = 1;
	}

	if (!ack)
		return 0;	/* nothing to acknowledge */
	UWRITE2(sc, UHCI_STS, ack); /* acknowledge the ints */

	usb_schedsoftintr(&sc->sc_bus);

	DPRINTFN(15, "sc %#jx done", (uintptr_t)sc, 0, 0, 0);

	return 1;
}

void
uhci_softintr(void *v)
{
	struct usbd_bus *bus = v;
	uhci_softc_t *sc = UHCI_BUS2SC(bus);
	struct uhci_xfer *ux, *nextux;
	ux_completeq_t cq;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTF("sc %#jx", (uintptr_t)sc, 0, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	TAILQ_INIT(&cq);
	/*
	 * Interrupts on UHCI really suck.  When the host controller
	 * interrupts because a transfer is completed there is no
	 * way of knowing which transfer it was.  You can scan down
	 * the TDs and QHs of the previous frame to limit the search,
	 * but that assumes that the interrupt was not delayed by more
	 * than 1 ms, which may not always be true (e.g. after debug
	 * output on a slow console).
	 * We scan all interrupt descriptors to see if any have
	 * completed.
	 */
	TAILQ_FOREACH_SAFE(ux, &sc->sc_intrhead, ux_list, nextux) {
		uhci_check_intr(sc, ux, &cq);
	}

	/*
	 * We abuse ux_list for the interrupt and complete lists and
	 * interrupt transfers will get re-added here so use
	 * the _SAFE version of TAILQ_FOREACH.
	 */
	TAILQ_FOREACH_SAFE(ux, &cq, ux_list, nextux) {
		DPRINTF("ux %#jx", (uintptr_t)ux, 0, 0, 0);
		usb_transfer_complete(&ux->ux_xfer);
	}

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
}

/* Check for an interrupt. */
void
uhci_check_intr(uhci_softc_t *sc, struct uhci_xfer *ux, ux_completeq_t *cqp)
{
	uhci_soft_td_t *std, *fstd = NULL, *lstd = NULL;
	uint32_t status;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(15, "ux %#jx", (uintptr_t)ux, 0, 0, 0);

	KASSERT(ux != NULL);

	struct usbd_xfer *xfer = &ux->ux_xfer;
	if (xfer->ux_status == USBD_CANCELLED ||
	    xfer->ux_status == USBD_TIMEOUT) {
		DPRINTF("aborted xfer %#jx", (uintptr_t)xfer, 0, 0, 0);
		return;
	}

	switch (ux->ux_type) {
	case UX_CTRL:
		fstd = ux->ux_setup;
		lstd = ux->ux_stat;
		break;
	case UX_BULK:
	case UX_INTR:
	case UX_ISOC:
		fstd = ux->ux_stdstart;
		lstd = ux->ux_stdend;
		break;
	default:
		KASSERT(false);
		break;
	}
	if (fstd == NULL)
		return;

	KASSERT(lstd != NULL);

	usb_syncmem(&lstd->dma,
	    lstd->offs + offsetof(uhci_td_t, td_status),
	    sizeof(lstd->td.td_status),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	status = le32toh(lstd->td.td_status);
	usb_syncmem(&lstd->dma,
	    lstd->offs + offsetof(uhci_td_t, td_status),
	    sizeof(lstd->td.td_status),
	    BUS_DMASYNC_PREREAD);

	/* If the last TD is not marked active we can complete */
	if (!(status & UHCI_TD_ACTIVE)) {
 done:
		DPRINTFN(12, "ux=%#jx done", (uintptr_t)ux, 0, 0, 0);
		uhci_idone(ux, cqp);
		return;
	}

	/*
	 * If the last TD is still active we need to check whether there
	 * is an error somewhere in the middle, or whether there was a
	 * short packet (SPD and not ACTIVE).
	 */
	DPRINTFN(12, "active ux=%#jx", (uintptr_t)ux, 0, 0, 0);
	for (std = fstd; std != lstd; std = std->link.std) {
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		status = le32toh(std->td.td_status);
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status), BUS_DMASYNC_PREREAD);

		/* If there's an active TD the xfer isn't done. */
		if (status & UHCI_TD_ACTIVE) {
			DPRINTFN(12, "ux=%#jx std=%#jx still active",
			    (uintptr_t)ux, (uintptr_t)std, 0, 0);
			return;
		}

		/* Any kind of error makes the xfer done. */
		if (status & UHCI_TD_STALLED)
			goto done;

		/*
		 * If the data phase of a control transfer is short, we need
		 * to complete the status stage
		 */

		if ((status & UHCI_TD_SPD) && ux->ux_type == UX_CTRL) {
			struct uhci_pipe *upipe =
			    UHCI_PIPE2UPIPE(xfer->ux_pipe);
			uhci_soft_qh_t *sqh = upipe->ctrl.sqh;
			uhci_soft_td_t *stat = upipe->ctrl.stat;

			DPRINTFN(12, "ux=%#jx std=%#jx control status"
			    "phase needs completion", (uintptr_t)ux,
			    (uintptr_t)ux->ux_stdstart, 0, 0);

			sqh->qh.qh_elink =
			    htole32(stat->physaddr | UHCI_PTR_TD);
			usb_syncmem(&sqh->dma, sqh->offs, sizeof(sqh->qh),
			    BUS_DMASYNC_PREWRITE);
			break;
		}

		/* We want short packets, and it is short: it's done */
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_token),
		    sizeof(std->td.td_token),
		    BUS_DMASYNC_POSTWRITE);

		if ((status & UHCI_TD_SPD) &&
			UHCI_TD_GET_ACTLEN(status) <
			UHCI_TD_GET_MAXLEN(le32toh(std->td.td_token))) {
			goto done;
		}
	}
}

/* Called with USB lock held. */
void
uhci_idone(struct uhci_xfer *ux, ux_completeq_t *cqp)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	struct usbd_xfer *xfer = &ux->ux_xfer;
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	uhci_soft_td_t *std;
	uint32_t status = 0, nstatus;
	int actlen;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	DPRINTFN(12, "ux=%#jx", (uintptr_t)ux, 0, 0, 0);

	/*
	 * Try to claim this xfer for completion.  If it has already
	 * completed or aborted, drop it on the floor.
	 */
	if (!usbd_xfer_trycomplete(xfer))
		return;

#ifdef DIAGNOSTIC
#ifdef UHCI_DEBUG
	if (ux->ux_isdone) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_ii(ux);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif
	KASSERT(!ux->ux_isdone);
	KASSERTMSG(!ux->ux_isdone, "xfer %p type %d status %d", xfer,
	    ux->ux_type, xfer->ux_status);
	ux->ux_isdone = true;
#endif

	if (xfer->ux_nframes != 0) {
		/* Isoc transfer, do things differently. */
		uhci_soft_td_t **stds = upipe->isoc.stds;
		int i, n, nframes, len;

		DPRINTFN(5, "ux=%#jx isoc ready", (uintptr_t)ux, 0, 0, 0);

		nframes = xfer->ux_nframes;
		actlen = 0;
		n = ux->ux_curframe;
		for (i = 0; i < nframes; i++) {
			std = stds[n];
#ifdef UHCI_DEBUG
			if (uhcidebug >= 5) {
				DPRINTF("isoc TD %jd", i, 0, 0, 0);
				DPRINTF("--- dump start ---", 0, 0, 0, 0);
				uhci_dump_td(std);
				DPRINTF("--- dump end ---", 0, 0, 0, 0);
			}
#endif
			if (++n >= UHCI_VFRAMELIST_COUNT)
				n = 0;
			usb_syncmem(&std->dma,
			    std->offs + offsetof(uhci_td_t, td_status),
			    sizeof(std->td.td_status),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
			status = le32toh(std->td.td_status);
			len = UHCI_TD_GET_ACTLEN(status);
			xfer->ux_frlengths[i] = len;
			actlen += len;
		}
		upipe->isoc.inuse -= nframes;
		xfer->ux_actlen = actlen;
		xfer->ux_status = USBD_NORMAL_COMPLETION;
		goto end;
	}

#ifdef UHCI_DEBUG
	DPRINTFN(10, "ux=%#jx, xfer=%#jx, pipe=%#jx ready", (uintptr_t)ux,
	    (uintptr_t)xfer, (uintptr_t)upipe, 0);
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_tds(ux->ux_stdstart);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	/* The transfer is done, compute actual length and status. */
	actlen = 0;
	for (std = ux->ux_stdstart; std != NULL; std = std->link.std) {
		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		nstatus = le32toh(std->td.td_status);
		if (nstatus & UHCI_TD_ACTIVE)
			break;

		status = nstatus;
		if (UHCI_TD_GET_PID(le32toh(std->td.td_token)) !=
			UHCI_TD_PID_SETUP)
			actlen += UHCI_TD_GET_ACTLEN(status);
		else {
			/*
			 * UHCI will report CRCTO in addition to a STALL or NAK
			 * for a SETUP transaction.  See section 3.2.2, "TD
			 * CONTROL AND STATUS".
			 */
			if (status & (UHCI_TD_STALLED | UHCI_TD_NAK))
				status &= ~UHCI_TD_CRCTO;
		}
	}
	/* If there are left over TDs we need to update the toggle. */
	if (std != NULL)
		upipe->nexttoggle = UHCI_TD_GET_DT(le32toh(std->td.td_token));

	status &= UHCI_TD_ERROR;
	DPRINTFN(10, "actlen=%jd, status=%#jx", actlen, status, 0, 0);
	xfer->ux_actlen = actlen;
	if (status != 0) {

		DPRINTFN((status == UHCI_TD_STALLED) * 10,
		    "error, addr=%jd, endpt=0x%02jx",
		    xfer->ux_pipe->up_dev->ud_addr,
		    xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress,
		    0, 0);
		DPRINTFN((status == UHCI_TD_STALLED) * 10,
		    "bitstuff=%jd crcto   =%jd nak     =%jd babble  =%jd",
		    !!(status & UHCI_TD_BITSTUFF),
		    !!(status & UHCI_TD_CRCTO),
		    !!(status & UHCI_TD_NAK),
		    !!(status & UHCI_TD_BABBLE));
		DPRINTFN((status == UHCI_TD_STALLED) * 10,
		    "dbuffer =%jd stalled =%jd active  =%jd",
		    !!(status & UHCI_TD_DBUFFER),
		    !!(status & UHCI_TD_STALLED),
		    !!(status & UHCI_TD_ACTIVE),
		    0);

		if (status == UHCI_TD_STALLED)
			xfer->ux_status = USBD_STALLED;
		else
			xfer->ux_status = USBD_IOERROR; /* more info XXX */
	} else {
		xfer->ux_status = USBD_NORMAL_COMPLETION;
	}

 end:
	uhci_del_intr_list(sc, ux);
	if (cqp)
		TAILQ_INSERT_TAIL(cqp, ux, ux_list);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
	DPRINTFN(12, "ux=%#jx done", (uintptr_t)ux, 0, 0, 0);
}

void
uhci_poll(struct usbd_bus *bus)
{
	uhci_softc_t *sc = UHCI_BUS2SC(bus);

	if (UREAD2(sc, UHCI_STS) & UHCI_STS_USBINT) {
		mutex_spin_enter(&sc->sc_intr_lock);
		uhci_intr1(sc);
		mutex_spin_exit(&sc->sc_intr_lock);
	}
}

void
uhci_reset(uhci_softc_t *sc)
{
	int n;

	UHCICMD(sc, UHCI_CMD_HCRESET);
	/* The reset bit goes low when the controller is done. */
	for (n = 0; n < UHCI_RESET_TIMEOUT &&
		    (UREAD2(sc, UHCI_CMD) & UHCI_CMD_HCRESET); n++)
		usb_delay_ms(&sc->sc_bus, 1);
	if (n >= UHCI_RESET_TIMEOUT)
		printf("%s: controller did not reset\n",
		       device_xname(sc->sc_dev));
}

usbd_status
uhci_run(uhci_softc_t *sc, int run, int locked)
{
	int n, running;
	uint16_t cmd;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	run = run != 0;
	if (!locked)
		mutex_spin_enter(&sc->sc_intr_lock);

	DPRINTF("setting run=%jd", run, 0, 0, 0);
	cmd = UREAD2(sc, UHCI_CMD);
	if (run)
		cmd |= UHCI_CMD_RS;
	else
		cmd &= ~UHCI_CMD_RS;
	UHCICMD(sc, cmd);
	for (n = 0; n < 10; n++) {
		running = !(UREAD2(sc, UHCI_STS) & UHCI_STS_HCH);
		/* return when we've entered the state we want */
		if (run == running) {
			if (!locked)
				mutex_spin_exit(&sc->sc_intr_lock);
			DPRINTF("done cmd=%#jx sts=%#jx",
			    UREAD2(sc, UHCI_CMD), UREAD2(sc, UHCI_STS), 0, 0);
			return USBD_NORMAL_COMPLETION;
		}
		usb_delay_ms_locked(&sc->sc_bus, 1, &sc->sc_intr_lock);
	}
	if (!locked)
		mutex_spin_exit(&sc->sc_intr_lock);
	printf("%s: cannot %s\n", device_xname(sc->sc_dev),
	       run ? "start" : "stop");
	return USBD_IOERROR;
}

/*
 * Memory management routines.
 *  uhci_alloc_std allocates TDs
 *  uhci_alloc_sqh allocates QHs
 * These two routines do their own free list management,
 * partly for speed, partly because allocating DMAable memory
 * has page size granularity so much memory would be wasted if
 * only one TD/QH (32 bytes) was placed in each allocated chunk.
 */

uhci_soft_td_t *
uhci_alloc_std(uhci_softc_t *sc)
{
	uhci_soft_td_t *std;
	int i, offs;
	usb_dma_t dma;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freetds == NULL) {
		DPRINTFN(2, "allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		int err = usb_allocmem(sc->sc_bus.ub_dmatag, UHCI_STD_SIZE * UHCI_STD_CHUNK,
		    UHCI_TD_ALIGN, USBMALLOC_COHERENT, &dma);
		if (err)
			return NULL;

		mutex_enter(&sc->sc_lock);
		for (i = 0; i < UHCI_STD_CHUNK; i++) {
			offs = i * UHCI_STD_SIZE;
			std = KERNADDR(&dma, offs);
			std->physaddr = DMAADDR(&dma, offs);
			std->dma = dma;
			std->offs = offs;
			std->link.std = sc->sc_freetds;
			sc->sc_freetds = std;
		}
	}
	std = sc->sc_freetds;
	sc->sc_freetds = std->link.std;
	mutex_exit(&sc->sc_lock);

	memset(&std->td, 0, sizeof(uhci_td_t));

	return std;
}

#define TD_IS_FREE 0x12345678

void
uhci_free_std_locked(uhci_softc_t *sc, uhci_soft_td_t *std)
{
	KASSERT(mutex_owned(&sc->sc_lock));

#ifdef DIAGNOSTIC
	if (le32toh(std->td.td_token) == TD_IS_FREE) {
		printf("%s: freeing free TD %p\n", __func__, std);
		return;
	}
	std->td.td_token = htole32(TD_IS_FREE);
#endif

	std->link.std = sc->sc_freetds;
	sc->sc_freetds = std;
}

void
uhci_free_std(uhci_softc_t *sc, uhci_soft_td_t *std)
{
	mutex_enter(&sc->sc_lock);
	uhci_free_std_locked(sc, std);
	mutex_exit(&sc->sc_lock);
}

uhci_soft_qh_t *
uhci_alloc_sqh(uhci_softc_t *sc)
{
	uhci_soft_qh_t *sqh;
	int i, offs;
	usb_dma_t dma;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freeqhs == NULL) {
		DPRINTFN(2, "allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		int err = usb_allocmem(sc->sc_bus.ub_dmatag, UHCI_SQH_SIZE * UHCI_SQH_CHUNK,
		    UHCI_QH_ALIGN, USBMALLOC_COHERENT, &dma);
		if (err)
			return NULL;

		mutex_enter(&sc->sc_lock);
		for (i = 0; i < UHCI_SQH_CHUNK; i++) {
			offs = i * UHCI_SQH_SIZE;
			sqh = KERNADDR(&dma, offs);
			sqh->physaddr = DMAADDR(&dma, offs);
			sqh->dma = dma;
			sqh->offs = offs;
			sqh->hlink = sc->sc_freeqhs;
			sc->sc_freeqhs = sqh;
		}
	}
	sqh = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh->hlink;
	mutex_exit(&sc->sc_lock);

	memset(&sqh->qh, 0, sizeof(uhci_qh_t));

	return sqh;
}

void
uhci_free_sqh(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	sqh->hlink = sc->sc_freeqhs;
	sc->sc_freeqhs = sqh;
}

#if 0
void
uhci_free_std_chain(uhci_softc_t *sc, uhci_soft_td_t *std,
		    uhci_soft_td_t *stdend)
{
	uhci_soft_td_t *p;
	uint32_t td_link;

	/*
	 * to avoid race condition with the controller which may be looking
	 * at this chain, we need to first invalidate all links, and
	 * then wait for the controller to move to another queue
	 */
	for (p = std; p != stdend; p = p->link.std) {
		usb_syncmem(&p->dma,
		    p->offs + offsetof(uhci_td_t, td_link),
		    sizeof(p->td.td_link),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		td_link = le32toh(p->td.td_link);
		usb_syncmem(&p->dma,
		    p->offs + offsetof(uhci_td_t, td_link),
		    sizeof(p->td.td_link),
		    BUS_DMASYNC_PREREAD);
		if ((td_link & UHCI_PTR_T) == 0) {
			p->td.td_link = htole32(UHCI_PTR_T);
			usb_syncmem(&p->dma,
			    p->offs + offsetof(uhci_td_t, td_link),
			    sizeof(p->td.td_link),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		}
	}
	delay(UHCI_QH_REMOVE_DELAY);

	for (; std != stdend; std = p) {
		p = std->link.std;
		uhci_free_std(sc, std);
	}
}
#endif

int
uhci_alloc_std_chain(uhci_softc_t *sc, struct usbd_xfer *xfer, int len,
    int rd, uhci_soft_td_t **sp)
{
	struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
	uint16_t flags = xfer->ux_flags;
	uhci_soft_td_t *p;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	DPRINTFN(8, "xfer=%#jx pipe=%#jx", (uintptr_t)xfer,
	    (uintptr_t)xfer->ux_pipe, 0, 0);

	ASSERT_SLEEPABLE();
	KASSERT(sp);

	int maxp = UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize);
	if (maxp == 0) {
		printf("%s: maxp=0\n", __func__);
		return EINVAL;
	}
	size_t ntd = howmany(len, maxp);
	/*
	 * if our transfer is bigger than PAGE_SIZE and maxp not a factor of
	 * PAGE_SIZE then we will need another TD per page.
	 */
	if (len > PAGE_SIZE && (PAGE_SIZE % maxp) != 0) {
		ntd += howmany(len, PAGE_SIZE);
	}

	/*
	 * Might need one more TD if we're writing a ZLP
	 */
	if (!rd && (flags & USBD_FORCE_SHORT_XFER)) {
		ntd++;
	}
	DPRINTFN(10, "maxp=%jd ntd=%jd", maxp, ntd, 0, 0);

	uxfer->ux_stds = NULL;
	uxfer->ux_nstd = ntd;
	if (ntd == 0) {
		*sp = NULL;
		DPRINTF("ntd=0", 0, 0, 0, 0);
		return 0;
	}
	uxfer->ux_stds = kmem_alloc(sizeof(uhci_soft_td_t *) * ntd,
	    KM_SLEEP);

	for (int i = 0; i < ntd; i++) {
		p = uhci_alloc_std(sc);
		if (p == NULL) {
			if (i != 0) {
				uxfer->ux_nstd = i;
				uhci_free_stds(sc, uxfer);
			}
			kmem_free(uxfer->ux_stds,
			    sizeof(uhci_soft_td_t *) * ntd);
			return ENOMEM;
		}
		uxfer->ux_stds[i] = p;
	}

	*sp = uxfer->ux_stds[0];

	return 0;
}

Static void
uhci_free_stds(uhci_softc_t *sc, struct uhci_xfer *ux)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	DPRINTFN(8, "ux=%#jx", (uintptr_t)ux, 0, 0, 0);

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < ux->ux_nstd; i++) {
		uhci_soft_td_t *std = ux->ux_stds[i];
#ifdef DIAGNOSTIC
		if (le32toh(std->td.td_token) == TD_IS_FREE) {
			printf("%s: freeing free TD %p\n", __func__, std);
			return;
		}
		std->td.td_token = htole32(TD_IS_FREE);
#endif
		ux->ux_stds[i]->link.std = sc->sc_freetds;
		sc->sc_freetds = std;
	}
	mutex_exit(&sc->sc_lock);
}


Static void
uhci_reset_std_chain(uhci_softc_t *sc, struct usbd_xfer *xfer,
    int length, int isread, int *toggle, uhci_soft_td_t **lstd)
{
	struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usb_dma_t *dma = &xfer->ux_dmabuf;
	uint16_t flags = xfer->ux_flags;
	uhci_soft_td_t *std, *prev;
	int len = length;
	int tog = *toggle;
	int maxp;
	uint32_t status;
	size_t i, offs;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(8, "xfer=%#jx len %jd isread %jd toggle %jd", (uintptr_t)xfer,
	    len, isread, *toggle);

	KASSERT(len != 0 || (!isread && (flags & USBD_FORCE_SHORT_XFER)));

	maxp = UGETW(pipe->up_endpoint->ue_edesc->wMaxPacketSize);
	KASSERT(maxp != 0);

	int addr = xfer->ux_pipe->up_dev->ud_addr;
	int endpt = xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress;

	status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE);
	if (pipe->up_dev->ud_speed == USB_SPEED_LOW)
		status |= UHCI_TD_LS;
	if (flags & USBD_SHORT_XFER_OK)
		status |= UHCI_TD_SPD;
	usb_syncmem(dma, 0, len,
	    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	std = prev = NULL;
	for (offs = i = 0; len != 0 && i < uxfer->ux_nstd; i++, prev = std) {
		int l = len;
		std = uxfer->ux_stds[i];

		const bus_addr_t sbp = DMAADDR(dma, offs);
		const bus_addr_t ebp = DMAADDR(dma, offs + l - 1);
		if (((sbp ^ ebp) & ~PAGE_MASK) != 0)
			l = PAGE_SIZE - (DMAADDR(dma, offs) & PAGE_MASK);

		if (l > maxp)
			l = maxp;

		if (prev) {
			prev->link.std = std;
			prev->td.td_link = htole32(
			    std->physaddr | UHCI_PTR_VF | UHCI_PTR_TD
			    );
			usb_syncmem(&prev->dma, prev->offs, sizeof(prev->td),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		}

		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		std->td.td_link = htole32(UHCI_PTR_T | UHCI_PTR_VF | UHCI_PTR_TD);
		std->td.td_status = htole32(status);
		std->td.td_token = htole32(
		    UHCI_TD_SET_ENDPT(UE_GET_ADDR(endpt)) |
		    UHCI_TD_SET_DEVADDR(addr) |
		    UHCI_TD_SET_PID(isread ? UHCI_TD_PID_IN : UHCI_TD_PID_OUT) |
		    UHCI_TD_SET_DT(tog) |
		    UHCI_TD_SET_MAXLEN(l)
		    );
		std->td.td_buffer = htole32(DMAADDR(dma, offs));

		std->link.std = NULL;

		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		tog ^= 1;

		offs += l;
		len -= l;
	}
	KASSERTMSG(len == 0, "xfer %p alen %d len %d mps %d ux_nqtd %zu i %zu",
	    xfer, length, len, maxp, uxfer->ux_nstd, i);

	if (!isread &&
	    (flags & USBD_FORCE_SHORT_XFER) &&
	    length % maxp == 0) {
		/* Force a 0 length transfer at the end. */
		KASSERTMSG(i < uxfer->ux_nstd, "i=%zu nstd=%zu", i,
		    uxfer->ux_nstd);
		std = uxfer->ux_stds[i++];

		std->td.td_link = htole32(UHCI_PTR_T | UHCI_PTR_VF | UHCI_PTR_TD);
		std->td.td_status = htole32(status);
		std->td.td_token = htole32(
		    UHCI_TD_SET_ENDPT(UE_GET_ADDR(endpt)) |
		    UHCI_TD_SET_DEVADDR(addr) |
		    UHCI_TD_SET_PID(UHCI_TD_PID_OUT) |
		    UHCI_TD_SET_DT(tog) |
		    UHCI_TD_SET_MAXLEN(0)
		    );
		std->td.td_buffer = 0;
		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		std->link.std = NULL;
		if (prev) {
			prev->link.std = std;
			prev->td.td_link = htole32(
			    std->physaddr | UHCI_PTR_VF | UHCI_PTR_TD
			    );
			usb_syncmem(&prev->dma, prev->offs, sizeof(prev->td),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		}
		tog ^= 1;
	}
	*lstd = std;
	*toggle = tog;
}

void
uhci_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	upipe->nexttoggle = 0;
}

void
uhci_noop(struct usbd_pipe *pipe)
{
}

int
uhci_device_bulk_init(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	int endpt = ed->bEndpointAddress;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int len = xfer->ux_bufsize;
	int err = 0;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(3, "xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer, len,
	    xfer->ux_flags, 0);

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));

	uxfer->ux_type = UX_BULK;
	err = uhci_alloc_std_chain(sc, xfer, len, isread, &uxfer->ux_stdstart);
	if (err)
		return err;

#ifdef UHCI_DEBUG
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_tds(uxfer->ux_stdstart);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	return 0;
}

Static void
uhci_device_bulk_fini(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);

	KASSERT(ux->ux_type == UX_BULK);

	if (ux->ux_nstd) {
		uhci_free_stds(sc, ux);
		kmem_free(ux->ux_stds, sizeof(uhci_soft_td_t *) * ux->ux_nstd);
	}
}

usbd_status
uhci_device_bulk_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, so start it first.  */
	return uhci_device_bulk_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

usbd_status
uhci_device_bulk_start(struct usbd_xfer *xfer)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	uhci_soft_td_t *data, *dataend;
	uhci_soft_qh_t *sqh;
	const bool polling = sc->sc_bus.ub_usepolling;
	int len;
	int endpt;
	int isread;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(3, "xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, 0);

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_length <= xfer->ux_bufsize);

	len = xfer->ux_length;
	endpt = upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sqh = upipe->bulk.sqh;

	/* Take lock here to protect nexttoggle */
	if (!polling)
		mutex_enter(&sc->sc_lock);

	uhci_reset_std_chain(sc, xfer, len, isread, &upipe->nexttoggle,
	    &dataend);

	data = ux->ux_stdstart;
	ux->ux_stdend = dataend;
	dataend->td.td_status |= htole32(UHCI_TD_IOC);
	usb_syncmem(&dataend->dma,
	    dataend->offs + offsetof(uhci_td_t, td_status),
	    sizeof(dataend->td.td_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef UHCI_DEBUG
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		DPRINTFN(10, "before transfer", 0, 0, 0, 0);
		uhci_dump_tds(data);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	KASSERT(ux->ux_isdone);
#ifdef DIAGNOSTIC
	ux->ux_isdone = false;
#endif

	sqh->elink = data;
	sqh->qh.qh_elink = htole32(data->physaddr | UHCI_PTR_TD);
	/* uhci_add_bulk() will do usb_syncmem(sqh) */

	uhci_add_bulk(sc, sqh);
	uhci_add_intr_list(sc, ux);
	usbd_xfer_schedule_timeout(xfer);
	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Abort a device bulk request. */
void
uhci_device_bulk_abort(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	usbd_xfer_abort(xfer);
}

/*
 * To allow the hardware time to notice we simply wait.
 */
Static void
uhci_abortx(struct usbd_xfer *xfer)
{
	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	uhci_soft_td_t *std;

	DPRINTFN(1,"xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

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
		DPRINTFN(4, "xfer %#jx dying %ju", (uintptr_t)xfer,
		    xfer->ux_status, 0, 0);
		goto dying;
	}

	/*
	 * HC Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	uhci_del_intr_list(sc, ux);

	DPRINTF("stop ux=%#jx", (uintptr_t)ux, 0, 0, 0);
	for (std = ux->ux_stdstart; std != NULL; std = std->link.std) {
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		std->td.td_status &= htole32(~(UHCI_TD_ACTIVE | UHCI_TD_IOC));
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}

	/*
	 * HC Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.
	 */
	/* Hardware finishes in 1ms */
	usb_delay_ms_locked(upipe->pipe.up_dev->ud_bus, 2, &sc->sc_lock);
dying:
#ifdef DIAGNOSTIC
	ux->ux_isdone = true;
#endif
	DPRINTFN(14, "end", 0, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
}

/* Close a device bulk pipe. */
void
uhci_device_bulk_close(struct usbd_pipe *pipe)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));

	uhci_free_sqh(sc, upipe->bulk.sqh);

	pipe->up_endpoint->ue_toggle = upipe->nexttoggle;
}

int
uhci_device_ctrl_init(struct usbd_xfer *xfer)
{
	struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	usb_device_request_t *req = &xfer->ux_request;
	struct usbd_device *dev = upipe->pipe.up_dev;
	uhci_softc_t *sc = dev->ud_bus->ub_hcpriv;
	uhci_soft_td_t *data = NULL;
	int len;
	usbd_status err;
	int isread;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(3, "xfer=%#jx len=%jd, addr=%jd, endpt=%jd",
	    (uintptr_t)xfer, xfer->ux_bufsize, dev->ud_addr,
	    upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress);

	isread = req->bmRequestType & UT_READ;
	len = xfer->ux_bufsize;

	uxfer->ux_type = UX_CTRL;
	/* Set up data transaction */
	if (len != 0) {
		err = uhci_alloc_std_chain(sc, xfer, len, isread, &data);
		if (err)
			return err;
	}
	/* Set up interrupt info. */
	uxfer->ux_setup = upipe->ctrl.setup;
	uxfer->ux_stat = upipe->ctrl.stat;
	uxfer->ux_data = data;

	return 0;
}

Static void
uhci_device_ctrl_fini(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);

	KASSERT(ux->ux_type == UX_CTRL);

	if (ux->ux_nstd) {
		uhci_free_stds(sc, ux);
		kmem_free(ux->ux_stds, sizeof(uhci_soft_td_t *) * ux->ux_nstd);
	}
}

usbd_status
uhci_device_ctrl_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, so start it first.  */
	return uhci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

usbd_status
uhci_device_ctrl_start(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *uxfer = UHCI_XFER2UXFER(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	usb_device_request_t *req = &xfer->ux_request;
	struct usbd_device *dev = upipe->pipe.up_dev;
	int addr = dev->ud_addr;
	int endpt = upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	uhci_soft_td_t *setup, *stat, *next, *dataend;
	uhci_soft_qh_t *sqh;
	const bool polling = sc->sc_bus.ub_usepolling;
	int len;
	int isread;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	DPRINTFN(3, "type=0x%02jx, request=0x%02jx, "
	    "wValue=0x%04jx, wIndex=0x%04jx",
	    req->bmRequestType, req->bRequest, UGETW(req->wValue),
	    UGETW(req->wIndex));
	DPRINTFN(3, "len=%jd, addr=%jd, endpt=%jd",
	    UGETW(req->wLength), dev->ud_addr, endpt, 0);

	isread = req->bmRequestType & UT_READ;
	len = UGETW(req->wLength);

	setup = upipe->ctrl.setup;
	stat = upipe->ctrl.stat;
	sqh = upipe->ctrl.sqh;

	memcpy(KERNADDR(&upipe->ctrl.reqdma, 0), req, sizeof(*req));
	usb_syncmem(&upipe->ctrl.reqdma, 0, sizeof(*req), BUS_DMASYNC_PREWRITE);

	if (!polling)
		mutex_enter(&sc->sc_lock);

	/* Set up data transaction */
	if (len != 0) {
		upipe->nexttoggle = 1;
		next = uxfer->ux_data;
		uhci_reset_std_chain(sc, xfer, len, isread,
		    &upipe->nexttoggle, &dataend);
		dataend->link.std = stat;
		dataend->td.td_link = htole32(stat->physaddr | UHCI_PTR_TD);
		usb_syncmem(&dataend->dma,
		    dataend->offs + offsetof(uhci_td_t, td_link),
		    sizeof(dataend->td.td_link),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	} else {
		next = stat;
	}

	const uint32_t status = UHCI_TD_ZERO_ACTLEN(
	    UHCI_TD_SET_ERRCNT(3) |
	    UHCI_TD_ACTIVE |
	    (dev->ud_speed == USB_SPEED_LOW ? UHCI_TD_LS : 0)
	    );
	setup->link.std = next;
	setup->td.td_link = htole32(next->physaddr | UHCI_PTR_TD);
	setup->td.td_status = htole32(status);
	setup->td.td_token = htole32(UHCI_TD_SETUP(sizeof(*req), endpt, addr));
	setup->td.td_buffer = htole32(DMAADDR(&upipe->ctrl.reqdma, 0));

	usb_syncmem(&setup->dma, setup->offs, sizeof(setup->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	stat->link.std = NULL;
	stat->td.td_link = htole32(UHCI_PTR_T);
	stat->td.td_status = htole32(status | UHCI_TD_IOC);
	stat->td.td_token =
		htole32(isread ? UHCI_TD_OUT(0, endpt, addr, 1) :
				 UHCI_TD_IN (0, endpt, addr, 1));
	stat->td.td_buffer = htole32(0);
	usb_syncmem(&stat->dma, stat->offs, sizeof(stat->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef UHCI_DEBUG
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		DPRINTF("before transfer", 0, 0, 0, 0);
		uhci_dump_tds(setup);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	/* Set up interrupt info. */
	uxfer->ux_setup = setup;
	uxfer->ux_stat = stat;
	KASSERT(uxfer->ux_isdone);
#ifdef DIAGNOSTIC
	uxfer->ux_isdone = false;
#endif

	sqh->elink = setup;
	sqh->qh.qh_elink = htole32(setup->physaddr | UHCI_PTR_TD);
	/* uhci_add_?s_ctrl() will do usb_syncmem(sqh) */

	if (dev->ud_speed == USB_SPEED_LOW)
		uhci_add_ls_ctrl(sc, sqh);
	else
		uhci_add_hs_ctrl(sc, sqh);
	uhci_add_intr_list(sc, uxfer);
#ifdef UHCI_DEBUG
	if (uhcidebug >= 12) {
		uhci_soft_td_t *std;
		uhci_soft_qh_t *xqh;
		uhci_soft_qh_t *sxqh;
		int maxqh = 0;
		uhci_physaddr_t link;
		DPRINTFN(12, "--- dump start ---", 0, 0, 0, 0);
		DPRINTFN(12, "follow from [0]", 0, 0, 0, 0);
		for (std = sc->sc_vframes[0].htd, link = 0;
		     (link & UHCI_PTR_QH) == 0;
		     std = std->link.std) {
			link = le32toh(std->td.td_link);
			uhci_dump_td(std);
		}
		sxqh = (uhci_soft_qh_t *)std;
		uhci_dump_qh(sxqh);
		for (xqh = sxqh;
		     xqh != NULL;
		     xqh = (maxqh++ == 5 || xqh->hlink == sxqh ||
			xqh->hlink == xqh ? NULL : xqh->hlink)) {
			uhci_dump_qh(xqh);
		}
		DPRINTFN(12, "Enqueued QH:", 0, 0, 0, 0);
		uhci_dump_qh(sqh);
		uhci_dump_tds(sqh->elink);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif
	usbd_xfer_schedule_timeout(xfer);
	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

int
uhci_device_intr_init(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	int endpt = ed->bEndpointAddress;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int len = xfer->ux_bufsize;
	int err;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	DPRINTFN(3, "xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, 0);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(len != 0);

	ux->ux_type = UX_INTR;
	ux->ux_nstd = 0;
	err = uhci_alloc_std_chain(sc, xfer, len, isread, &ux->ux_stdstart);

	return err;
}

Static void
uhci_device_intr_fini(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);

	KASSERT(ux->ux_type == UX_INTR);

	if (ux->ux_nstd) {
		uhci_free_stds(sc, ux);
		kmem_free(ux->ux_stds, sizeof(uhci_soft_td_t *) * ux->ux_nstd);
	}
}

usbd_status
uhci_device_intr_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, so start it first.  */
	return uhci_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

usbd_status
uhci_device_intr_start(struct usbd_xfer *xfer)
{
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	uhci_soft_td_t *data, *dataend;
	uhci_soft_qh_t *sqh;
	const bool polling = sc->sc_bus.ub_usepolling;
	int isread, endpt;
	int i;

	if (sc->sc_dying)
		return USBD_IOERROR;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	DPRINTFN(3, "xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, 0);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_length <= xfer->ux_bufsize);

	endpt = upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;

	data = ux->ux_stdstart;

	KASSERT(ux->ux_isdone);
#ifdef DIAGNOSTIC
	ux->ux_isdone = false;
#endif

	/* Take lock to protect nexttoggle */
	if (!polling)
		mutex_enter(&sc->sc_lock);
	uhci_reset_std_chain(sc, xfer, xfer->ux_length, isread,
	    &upipe->nexttoggle, &dataend);

	dataend->td.td_status |= htole32(UHCI_TD_IOC);
	usb_syncmem(&dataend->dma,
	    dataend->offs + offsetof(uhci_td_t, td_status),
	    sizeof(dataend->td.td_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	ux->ux_stdend = dataend;

#ifdef UHCI_DEBUG
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_tds(data);
		uhci_dump_qh(upipe->intr.qhs[0]);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	DPRINTFN(10, "qhs[0]=%#jx", (uintptr_t)upipe->intr.qhs[0], 0, 0, 0);
	for (i = 0; i < upipe->intr.npoll; i++) {
		sqh = upipe->intr.qhs[i];
		sqh->elink = data;
		sqh->qh.qh_elink = htole32(data->physaddr | UHCI_PTR_TD);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}
	uhci_add_intr_list(sc, ux);
	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

#ifdef UHCI_DEBUG
	if (uhcidebug >= 10) {
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_tds(data);
		uhci_dump_qh(upipe->intr.qhs[0]);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
	}
#endif

	return USBD_IN_PROGRESS;
}

/* Abort a device control request. */
void
uhci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	usbd_xfer_abort(xfer);
}

/* Close a device control pipe. */
void
uhci_device_ctrl_close(struct usbd_pipe *pipe)
{
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);

	uhci_free_sqh(sc, upipe->ctrl.sqh);
	uhci_free_std_locked(sc, upipe->ctrl.setup);
	uhci_free_std_locked(sc, upipe->ctrl.stat);

	usb_freemem(&upipe->ctrl.reqdma);
}

/* Abort a device interrupt request. */
void
uhci_device_intr_abort(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	usbd_xfer_abort(xfer);
}

/* Close a device interrupt pipe. */
void
uhci_device_intr_close(struct usbd_pipe *pipe)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	int i, npoll;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Unlink descriptors from controller data structures. */
	npoll = upipe->intr.npoll;
	for (i = 0; i < npoll; i++)
		uhci_remove_intr(sc, upipe->intr.qhs[i]);

	/*
	 * We now have to wait for any activity on the physical
	 * descriptors to stop.
	 */
	usb_delay_ms_locked(&sc->sc_bus, 2, &sc->sc_lock);

	for (i = 0; i < npoll; i++)
		uhci_free_sqh(sc, upipe->intr.qhs[i]);
	kmem_free(upipe->intr.qhs, npoll * sizeof(uhci_soft_qh_t *));
}

int
uhci_device_isoc_init(struct usbd_xfer *xfer)
{
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(xfer->ux_nframes != 0);
	KASSERT(ux->ux_isdone);

	ux->ux_type = UX_ISOC;
	return 0;
}

Static void
uhci_device_isoc_fini(struct usbd_xfer *xfer)
{
	struct uhci_xfer *ux __diagused = UHCI_XFER2UXFER(xfer);

	KASSERT(ux->ux_type == UX_ISOC);
}

usbd_status
uhci_device_isoc_transfer(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(5, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	/* insert into schedule, */

	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	struct isoc *isoc = &upipe->isoc;
	uhci_soft_td_t *std = NULL;
	uint32_t buf, len, status, offs;
	int i, next, nframes;
	int rd = UE_GET_DIR(upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN;

	DPRINTFN(5, "used=%jd next=%jd xfer=%#jx nframes=%jd",
	    isoc->inuse, isoc->next, (uintptr_t)xfer, xfer->ux_nframes);

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (xfer->ux_status == USBD_IN_PROGRESS) {
		/* This request has already been entered into the frame list */
		printf("%s: xfer=%p in frame list\n", __func__, xfer);
		/* XXX */
	}

#ifdef DIAGNOSTIC
	if (isoc->inuse >= UHCI_VFRAMELIST_COUNT)
		printf("%s: overflow!\n", __func__);
#endif

	KASSERT(xfer->ux_nframes != 0);

	if (xfer->ux_length)
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    rd ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	mutex_enter(&sc->sc_lock);
	next = isoc->next;
	if (next == -1) {
		/* Not in use yet, schedule it a few frames ahead. */
		next = (UREAD2(sc, UHCI_FRNUM) + 3) % UHCI_VFRAMELIST_COUNT;
		DPRINTFN(2, "start next=%jd", next, 0, 0, 0);
	}

	xfer->ux_status = USBD_IN_PROGRESS;
	ux->ux_curframe = next;

	offs = 0;
	status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(0) |
				     UHCI_TD_ACTIVE |
				     UHCI_TD_IOS);
	nframes = xfer->ux_nframes;
	for (i = 0; i < nframes; i++) {
		buf = DMAADDR(&xfer->ux_dmabuf, offs);
		std = isoc->stds[next];
		if (++next >= UHCI_VFRAMELIST_COUNT)
			next = 0;
		len = xfer->ux_frlengths[i];

		KASSERTMSG(len <= __SHIFTOUT_MASK(UHCI_TD_MAXLEN_MASK),
		    "len %d", len);
		std->td.td_buffer = htole32(buf);
		usb_syncmem(&xfer->ux_dmabuf, offs, len,
		    rd ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
		if (i == nframes - 1)
			status |= UHCI_TD_IOC;
		std->td.td_status = htole32(status);
		std->td.td_token &= htole32(~UHCI_TD_MAXLEN_MASK);
		std->td.td_token |= htole32(UHCI_TD_SET_MAXLEN(len));
		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
#ifdef UHCI_DEBUG
		if (uhcidebug >= 5) {
			DPRINTF("--- dump start ---", 0, 0, 0, 0);
			DPRINTF("TD %jd", i, 0, 0, 0);
			uhci_dump_td(std);
			DPRINTF("--- dump end ---", 0, 0, 0, 0);
		}
#endif
		offs += len;
		const bus_addr_t bend __diagused =
		    DMAADDR(&xfer->ux_dmabuf, offs - 1);

		KASSERT(((buf ^ bend) & ~PAGE_MASK) == 0);
	}
	isoc->next = next;
	isoc->inuse += xfer->ux_nframes;

	/* Set up interrupt info. */
	ux->ux_stdstart = std;
	ux->ux_stdend = std;

	KASSERT(ux->ux_isdone);
#ifdef DIAGNOSTIC
	ux->ux_isdone = false;
#endif
	uhci_add_intr_list(sc, ux);

	mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

void
uhci_device_isoc_abort(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	uhci_soft_td_t **stds = upipe->isoc.stds;
	uhci_soft_td_t *std;
	int i, n, nframes, maxlen, len;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Transfer is already done. */
	if (xfer->ux_status != USBD_NOT_STARTED &&
	    xfer->ux_status != USBD_IN_PROGRESS) {
		return;
	}

	/* Give xfer the requested abort code. */
	xfer->ux_status = USBD_CANCELLED;

	/* make hardware ignore it, */
	nframes = xfer->ux_nframes;
	n = ux->ux_curframe;
	maxlen = 0;
	for (i = 0; i < nframes; i++) {
		std = stds[n];
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		std->td.td_status &= htole32(~(UHCI_TD_ACTIVE | UHCI_TD_IOC));
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_token),
		    sizeof(std->td.td_token),
		    BUS_DMASYNC_POSTWRITE);
		len = UHCI_TD_GET_MAXLEN(le32toh(std->td.td_token));
		if (len > maxlen)
			maxlen = len;
		if (++n >= UHCI_VFRAMELIST_COUNT)
			n = 0;
	}

	/* and wait until we are sure the hardware has finished. */
	delay(maxlen);

#ifdef DIAGNOSTIC
	ux->ux_isdone = true;
#endif
	/* Remove from interrupt list. */
	uhci_del_intr_list(sc, ux);

	/* Run callback. */
	usb_transfer_complete(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
}

void
uhci_device_isoc_close(struct usbd_pipe *pipe)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	uhci_soft_td_t *std, *vstd;
	struct isoc *isoc;
	int i;

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * Make sure all TDs are marked as inactive.
	 * Wait for completion.
	 * Unschedule.
	 * Deallocate.
	 */
	isoc = &upipe->isoc;

	for (i = 0; i < UHCI_VFRAMELIST_COUNT; i++) {
		std = isoc->stds[i];
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		std->td.td_status &= htole32(~UHCI_TD_ACTIVE);
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_status),
		    sizeof(std->td.td_status),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}
	/* wait for completion */
	usb_delay_ms_locked(&sc->sc_bus, 2, &sc->sc_lock);

	for (i = 0; i < UHCI_VFRAMELIST_COUNT; i++) {
		std = isoc->stds[i];
		for (vstd = sc->sc_vframes[i].htd;
		     vstd != NULL && vstd->link.std != std;
		     vstd = vstd->link.std)
			;
		if (vstd == NULL) {
			/*panic*/
			printf("%s: %p not found\n", __func__, std);
			mutex_exit(&sc->sc_lock);
			return;
		}
		vstd->link = std->link;
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_link),
		    sizeof(std->td.td_link),
		    BUS_DMASYNC_POSTWRITE);
		vstd->td.td_link = std->td.td_link;
		usb_syncmem(&vstd->dma,
		    vstd->offs + offsetof(uhci_td_t, td_link),
		    sizeof(vstd->td.td_link),
		    BUS_DMASYNC_PREWRITE);
		uhci_free_std_locked(sc, std);
	}

	kmem_free(isoc->stds, UHCI_VFRAMELIST_COUNT * sizeof(uhci_soft_td_t *));
}

usbd_status
uhci_setup_isoc(struct usbd_pipe *pipe)
{
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	int addr = upipe->pipe.up_dev->ud_addr;
	int endpt = upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	int rd = UE_GET_DIR(endpt) == UE_DIR_IN;
	uhci_soft_td_t *std, *vstd;
	uint32_t token;
	struct isoc *isoc;
	int i;

	isoc = &upipe->isoc;

	isoc->stds = kmem_alloc(
	    UHCI_VFRAMELIST_COUNT * sizeof(uhci_soft_td_t *), KM_SLEEP);
	if (isoc->stds == NULL)
		return USBD_NOMEM;

	token = rd ? UHCI_TD_IN (0, endpt, addr, 0) :
		     UHCI_TD_OUT(0, endpt, addr, 0);

	/* Allocate the TDs and mark as inactive; */
	for (i = 0; i < UHCI_VFRAMELIST_COUNT; i++) {
		std = uhci_alloc_std(sc);
		if (std == 0)
			goto bad;
		std->td.td_status = htole32(UHCI_TD_IOS); /* iso, inactive */
		std->td.td_token = htole32(token);
		usb_syncmem(&std->dma, std->offs, sizeof(std->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		isoc->stds[i] = std;
	}

	mutex_enter(&sc->sc_lock);

	/* Insert TDs into schedule. */
	for (i = 0; i < UHCI_VFRAMELIST_COUNT; i++) {
		std = isoc->stds[i];
		vstd = sc->sc_vframes[i].htd;
		usb_syncmem(&vstd->dma,
		    vstd->offs + offsetof(uhci_td_t, td_link),
		    sizeof(vstd->td.td_link),
		    BUS_DMASYNC_POSTWRITE);
		std->link = vstd->link;
		std->td.td_link = vstd->td.td_link;
		usb_syncmem(&std->dma,
		    std->offs + offsetof(uhci_td_t, td_link),
		    sizeof(std->td.td_link),
		    BUS_DMASYNC_PREWRITE);
		vstd->link.std = std;
		vstd->td.td_link = htole32(std->physaddr | UHCI_PTR_TD);
		usb_syncmem(&vstd->dma,
		    vstd->offs + offsetof(uhci_td_t, td_link),
		    sizeof(vstd->td.td_link),
		    BUS_DMASYNC_PREWRITE);
	}
	mutex_exit(&sc->sc_lock);

	isoc->next = -1;
	isoc->inuse = 0;

	return USBD_NORMAL_COMPLETION;

 bad:
	while (--i >= 0)
		uhci_free_std(sc, isoc->stds[i]);
	kmem_free(isoc->stds, UHCI_VFRAMELIST_COUNT * sizeof(uhci_soft_td_t *));
	return USBD_NOMEM;
}

void
uhci_device_isoc_done(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	struct uhci_xfer *ux = UHCI_XFER2UXFER(xfer);
	int i, offs;
	int rd = UE_GET_DIR(upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(4, "length=%jd, ux_state=0x%08jx",
	    xfer->ux_actlen, xfer->ux_state, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

#ifdef DIAGNOSTIC
	if (ux->ux_stdend == NULL) {
		printf("%s: xfer=%p stdend==NULL\n", __func__, xfer);
#ifdef UHCI_DEBUG
		DPRINTF("--- dump start ---", 0, 0, 0, 0);
		uhci_dump_ii(ux);
		DPRINTF("--- dump end ---", 0, 0, 0, 0);
#endif
		return;
	}
#endif

	/* Turn off the interrupt since it is active even if the TD is not. */
	usb_syncmem(&ux->ux_stdend->dma,
	    ux->ux_stdend->offs + offsetof(uhci_td_t, td_status),
	    sizeof(ux->ux_stdend->td.td_status),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	ux->ux_stdend->td.td_status &= htole32(~UHCI_TD_IOC);
	usb_syncmem(&ux->ux_stdend->dma,
	    ux->ux_stdend->offs + offsetof(uhci_td_t, td_status),
	    sizeof(ux->ux_stdend->td.td_status),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	offs = 0;
	for (i = 0; i < xfer->ux_nframes; i++) {
		usb_syncmem(&xfer->ux_dmabuf, offs, xfer->ux_frlengths[i],
		    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		offs += xfer->ux_frlengths[i];
	}
}

void
uhci_device_intr_done(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc __diagused = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	uhci_soft_qh_t *sqh;
	int i, npoll;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(5, "length=%jd", xfer->ux_actlen, 0, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	npoll = upipe->intr.npoll;
	for (i = 0; i < npoll; i++) {
		sqh = upipe->intr.qhs[i];
		sqh->elink = NULL;
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}
	const int endpt = upipe->pipe.up_endpoint->ue_edesc->bEndpointAddress;
	const bool isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

/* Deallocate request data structures */
void
uhci_device_ctrl_done(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	int len = UGETW(xfer->ux_request.wLength);
	int isread = (xfer->ux_request.bmRequestType & UT_READ);

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	/* XXXNH move to uhci_idone??? */
	if (upipe->pipe.up_dev->ud_speed == USB_SPEED_LOW)
		uhci_remove_ls_ctrl(sc, upipe->ctrl.sqh);
	else
		uhci_remove_hs_ctrl(sc, upipe->ctrl.sqh);

	if (len) {
		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}
	usb_syncmem(&upipe->ctrl.reqdma, 0,
	    sizeof(usb_device_request_t),  BUS_DMASYNC_POSTWRITE);

	DPRINTF("length=%jd", xfer->ux_actlen, 0, 0, 0);
}

/* Deallocate request data structures */
void
uhci_device_bulk_done(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(xfer->ux_pipe);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	int endpt = ed->bEndpointAddress;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(5, "xfer=%#jx sc=%#jx upipe=%#jx", (uintptr_t)xfer,
	    (uintptr_t)sc, (uintptr_t)upipe, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	uhci_remove_bulk(sc, upipe->bulk.sqh);

	if (xfer->ux_length) {
		usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}

	DPRINTFN(5, "length=%jd", xfer->ux_actlen, 0, 0, 0);
}

/* Add interrupt QH, called with vflock. */
void
uhci_add_intr(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	struct uhci_vframe *vf = &sc->sc_vframes[sqh->pos];
	uhci_soft_qh_t *eqh;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(4, "n=%jd sqh=%#jx", sqh->pos, (uintptr_t)sqh, 0, 0);

	eqh = vf->eqh;
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_POSTWRITE);
	sqh->hlink       = eqh->hlink;
	sqh->qh.qh_hlink = eqh->qh.qh_hlink;
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(sqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	eqh->hlink       = sqh;
	eqh->qh.qh_hlink = htole32(sqh->physaddr | UHCI_PTR_QH);
	usb_syncmem(&eqh->dma, eqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(eqh->qh.qh_hlink), BUS_DMASYNC_PREWRITE);
	vf->eqh = sqh;
	vf->bandwidth++;
}

/* Remove interrupt QH. */
void
uhci_remove_intr(uhci_softc_t *sc, uhci_soft_qh_t *sqh)
{
	struct uhci_vframe *vf = &sc->sc_vframes[sqh->pos];
	uhci_soft_qh_t *pqh;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(4, "n=%jd sqh=%#jx", sqh->pos, (uintptr_t)sqh, 0, 0);

	/* See comment in uhci_remove_ctrl() */

	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_elink),
	    sizeof(sqh->qh.qh_elink),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	if (!(sqh->qh.qh_elink & htole32(UHCI_PTR_T))) {
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		delay(UHCI_QH_REMOVE_DELAY);
	}

	pqh = uhci_find_prev_qh(vf->hqh, sqh);
	usb_syncmem(&sqh->dma, sqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(sqh->qh.qh_hlink),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	pqh->hlink       = sqh->hlink;
	pqh->qh.qh_hlink = sqh->qh.qh_hlink;
	usb_syncmem(&pqh->dma, pqh->offs + offsetof(uhci_qh_t, qh_hlink),
	    sizeof(pqh->qh.qh_hlink),
	    BUS_DMASYNC_PREWRITE);
	delay(UHCI_QH_REMOVE_DELAY);
	if (vf->eqh == sqh)
		vf->eqh = pqh;
	vf->bandwidth--;
}

usbd_status
uhci_device_setintr(uhci_softc_t *sc, struct uhci_pipe *upipe, int ival)
{
	uhci_soft_qh_t *sqh;
	int i, npoll;
	u_int bestbw, bw, bestoffs, offs;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTFN(2, "pipe=%#jx", (uintptr_t)upipe, 0, 0, 0);
	if (ival == 0) {
		printf("%s: 0 interval\n", __func__);
		return USBD_INVAL;
	}

	if (ival > UHCI_VFRAMELIST_COUNT)
		ival = UHCI_VFRAMELIST_COUNT;
	npoll = howmany(UHCI_VFRAMELIST_COUNT, ival);
	DPRINTF("ival=%jd npoll=%jd", ival, npoll, 0, 0);

	upipe->intr.npoll = npoll;
	upipe->intr.qhs =
		kmem_alloc(npoll * sizeof(uhci_soft_qh_t *), KM_SLEEP);

	/*
	 * Figure out which offset in the schedule that has most
	 * bandwidth left over.
	 */
#define MOD(i) ((i) & (UHCI_VFRAMELIST_COUNT-1))
	for (bestoffs = offs = 0, bestbw = ~0; offs < ival; offs++) {
		for (bw = i = 0; i < npoll; i++)
			bw += sc->sc_vframes[MOD(i * ival + offs)].bandwidth;
		if (bw < bestbw) {
			bestbw = bw;
			bestoffs = offs;
		}
	}
	DPRINTF("bw=%jd offs=%jd", bestbw, bestoffs, 0, 0);
	for (i = 0; i < npoll; i++) {
		upipe->intr.qhs[i] = sqh = uhci_alloc_sqh(sc);
		sqh->elink = NULL;
		sqh->qh.qh_elink = htole32(UHCI_PTR_T);
		usb_syncmem(&sqh->dma,
		    sqh->offs + offsetof(uhci_qh_t, qh_elink),
		    sizeof(sqh->qh.qh_elink),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		sqh->pos = MOD(i * ival + bestoffs);
	}
#undef MOD

	mutex_enter(&sc->sc_lock);
	/* Enter QHs into the controller data structures. */
	for (i = 0; i < npoll; i++)
		uhci_add_intr(sc, upipe->intr.qhs[i]);
	mutex_exit(&sc->sc_lock);

	DPRINTFN(5, "returns %#jx", (uintptr_t)upipe, 0, 0, 0);

	return USBD_NORMAL_COMPLETION;
}

/* Open a new pipe. */
usbd_status
uhci_open(struct usbd_pipe *pipe)
{
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	struct usbd_bus *bus = pipe->up_dev->ud_bus;
	struct uhci_pipe *upipe = UHCI_PIPE2UPIPE(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	int ival;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTF("pipe=%#jx, addr=%jd, endpt=%jd (%jd)",
	    (uintptr_t)pipe, pipe->up_dev->ud_addr, ed->bEndpointAddress,
	    bus->ub_rhaddr);

	if (sc->sc_dying)
		return USBD_IOERROR;

	upipe->aborting = 0;
	/* toggle state needed for bulk endpoints */
	upipe->nexttoggle = pipe->up_endpoint->ue_toggle;

	if (pipe->up_dev->ud_addr == bus->ub_rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &uhci_root_intr_methods;
			break;
		default:
			return USBD_INVAL;
		}
	} else {
		switch (ed->bmAttributes & UE_XFERTYPE) {
		case UE_CONTROL:
			pipe->up_methods = &uhci_device_ctrl_methods;
			upipe->ctrl.sqh = uhci_alloc_sqh(sc);
			if (upipe->ctrl.sqh == NULL)
				goto bad;
			upipe->ctrl.setup = uhci_alloc_std(sc);
			if (upipe->ctrl.setup == NULL) {
				uhci_free_sqh(sc, upipe->ctrl.sqh);
				goto bad;
			}
			upipe->ctrl.stat = uhci_alloc_std(sc);
			if (upipe->ctrl.stat == NULL) {
				uhci_free_sqh(sc, upipe->ctrl.sqh);
				uhci_free_std(sc, upipe->ctrl.setup);
				goto bad;
			}
			int err = usb_allocmem(sc->sc_bus.ub_dmatag,
			    sizeof(usb_device_request_t), 0,
			    USBMALLOC_COHERENT, &upipe->ctrl.reqdma);
			if (err) {
				uhci_free_sqh(sc, upipe->ctrl.sqh);
				uhci_free_std(sc, upipe->ctrl.setup);
				uhci_free_std(sc, upipe->ctrl.stat);
				goto bad;
			}
			break;
		case UE_INTERRUPT:
			pipe->up_methods = &uhci_device_intr_methods;
			ival = pipe->up_interval;
			if (ival == USBD_DEFAULT_INTERVAL)
				ival = ed->bInterval;
			return uhci_device_setintr(sc, upipe, ival);
		case UE_ISOCHRONOUS:
			pipe->up_serialise = false;
			pipe->up_methods = &uhci_device_isoc_methods;
			return uhci_setup_isoc(pipe);
		case UE_BULK:
			pipe->up_methods = &uhci_device_bulk_methods;
			upipe->bulk.sqh = uhci_alloc_sqh(sc);
			if (upipe->bulk.sqh == NULL)
				goto bad;
			break;
		}
	}
	return USBD_NORMAL_COMPLETION;

 bad:
	return USBD_NOMEM;
}

/*
 * Data structures and routines to emulate the root hub.
 */
/*
 * The USB hub protocol requires that SET_FEATURE(PORT_RESET) also
 * enables the port, and also states that SET_FEATURE(PORT_ENABLE)
 * should not be used by the USB subsystem.  As we cannot issue a
 * SET_FEATURE(PORT_ENABLE) externally, we must ensure that the port
 * will be enabled as part of the reset.
 *
 * On the VT83C572, the port cannot be successfully enabled until the
 * outstanding "port enable change" and "connection status change"
 * events have been reset.
 */
Static usbd_status
uhci_portreset(uhci_softc_t *sc, int index)
{
	int lim, port, x;
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	if (index == 1)
		port = UHCI_PORTSC1;
	else if (index == 2)
		port = UHCI_PORTSC2;
	else
		return USBD_IOERROR;

	x = URWMASK(UREAD2(sc, port));
	UWRITE2(sc, port, x | UHCI_PORTSC_PR);

	usb_delay_ms(&sc->sc_bus, USB_PORT_ROOT_RESET_DELAY);

	DPRINTF("uhci port %jd reset, status0 = 0x%04jx", index,
	    UREAD2(sc, port), 0, 0);

	x = URWMASK(UREAD2(sc, port));
	UWRITE2(sc, port, x & ~(UHCI_PORTSC_PR | UHCI_PORTSC_SUSP));

	delay(100);

	DPRINTF("uhci port %jd reset, status1 = 0x%04jx", index,
	    UREAD2(sc, port), 0, 0);

	x = URWMASK(UREAD2(sc, port));
	UWRITE2(sc, port, x  | UHCI_PORTSC_PE);

	for (lim = 10; --lim > 0;) {
		usb_delay_ms(&sc->sc_bus, USB_PORT_RESET_DELAY);

		x = UREAD2(sc, port);
		DPRINTF("uhci port %jd iteration %ju, status = 0x%04jx", index,
		    lim, x, 0);

		if (!(x & UHCI_PORTSC_CCS)) {
			/*
			 * No device is connected (or was disconnected
			 * during reset).  Consider the port reset.
			 * The delay must be long enough to ensure on
			 * the initial iteration that the device
			 * connection will have been registered.  50ms
			 * appears to be sufficient, but 20ms is not.
			 */
			DPRINTFN(3, "uhci port %jd loop %ju, device detached",
			    index, lim, 0, 0);
			break;
		}

		if (x & (UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC)) {
			/*
			 * Port enabled changed and/or connection
			 * status changed were set.  Reset either or
			 * both raised flags (by writing a 1 to that
			 * bit), and wait again for state to settle.
			 */
			UWRITE2(sc, port, URWMASK(x) |
				(x & (UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC)));
			continue;
		}

		if (x & UHCI_PORTSC_PE)
			/* Port is enabled */
			break;

		UWRITE2(sc, port, URWMASK(x) | UHCI_PORTSC_PE);
	}

	DPRINTFN(3, "uhci port %jd reset, status2 = 0x%04jx", index,
	    UREAD2(sc, port), 0, 0);

	if (lim <= 0) {
		DPRINTF("uhci port %jd reset timed out", index,
		    0, 0, 0);
		return USBD_TIMEOUT;
	}

	sc->sc_isreset = 1;
	return USBD_NORMAL_COMPLETION;
}

Static int
uhci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	uhci_softc_t *sc = UHCI_BUS2SC(bus);
	int port, x;
	int status, change, totlen = 0;
	uint16_t len, value, index;
	usb_port_status_t ps;
	usbd_status err;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	if (sc->sc_dying)
		return -1;

	DPRINTF("type=0x%02jx request=%02jx", req->bmRequestType,
	    req->bRequest, 0, 0);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTF("wValue=0x%04jx", value, 0, 0, 0);
		if (len == 0)
			break;
		switch (value) {
#define sd ((usb_string_descriptor_t *)buf)
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "UHCI root hub");
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
		if (index == 1)
			port = UHCI_PORTSC1;
		else if (index == 2)
			port = UHCI_PORTSC2;
		else {
			return -1;
		}
		switch(value) {
		case UHF_PORT_ENABLE:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x & ~UHCI_PORTSC_PE);
			break;
		case UHF_PORT_SUSPEND:
			x = URWMASK(UREAD2(sc, port));
			if (!(x & UHCI_PORTSC_SUSP)) /* not suspended */
				break;
			UWRITE2(sc, port, x | UHCI_PORTSC_RD);
			/* see USB2 spec ch. 7.1.7.7 */
			usb_delay_ms(&sc->sc_bus, 20);
			UWRITE2(sc, port, x & ~UHCI_PORTSC_SUSP);
			/* 10ms resume delay must be provided by caller */
			break;
		case UHF_PORT_RESET:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x & ~UHCI_PORTSC_PR);
			break;
		case UHF_C_PORT_CONNECTION:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x | UHCI_PORTSC_CSC);
			break;
		case UHF_C_PORT_ENABLE:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x | UHCI_PORTSC_POEDC);
			break;
		case UHF_C_PORT_OVER_CURRENT:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x | UHCI_PORTSC_OCIC);
			break;
		case UHF_C_PORT_RESET:
			sc->sc_isreset = 0;
			break;
		case UHF_PORT_CONNECTION:
		case UHF_PORT_OVER_CURRENT:
		case UHF_PORT_POWER:
		case UHF_PORT_LOW_SPEED:
		case UHF_C_PORT_SUSPEND:
		default:
			return -1;
		}
		break;
	case C(UR_GET_BUS_STATE, UT_READ_CLASS_OTHER):
		if (index == 1)
			port = UHCI_PORTSC1;
		else if (index == 2)
			port = UHCI_PORTSC2;
		else {
			return -1;
		}
		if (len > 0) {
			*(uint8_t *)buf =
			    UHCI_PORTSC_GET_LS(UREAD2(sc, port));
			totlen = 1;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0) {
			return -1;
		}
		usb_hub_descriptor_t hubd;

		totlen = uimin(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);
		hubd.bNbrPorts = 2;
		memcpy(buf, &hubd, totlen);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			return -1;
		}
		memset(buf, 0, len);
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		if (index == 1)
			port = UHCI_PORTSC1;
		else if (index == 2)
			port = UHCI_PORTSC2;
		else {
			return -1;
		}
		if (len != 4) {
			return -1;
		}
		x = UREAD2(sc, port);
		status = change = 0;
		if (x & UHCI_PORTSC_CCS)
			status |= UPS_CURRENT_CONNECT_STATUS;
		if (x & UHCI_PORTSC_CSC)
			change |= UPS_C_CONNECT_STATUS;
		if (x & UHCI_PORTSC_PE)
			status |= UPS_PORT_ENABLED;
		if (x & UHCI_PORTSC_POEDC)
			change |= UPS_C_PORT_ENABLED;
		if (x & UHCI_PORTSC_OCI)
			status |= UPS_OVERCURRENT_INDICATOR;
		if (x & UHCI_PORTSC_OCIC)
			change |= UPS_C_OVERCURRENT_INDICATOR;
		if (x & UHCI_PORTSC_SUSP)
			status |= UPS_SUSPEND;
		if (x & UHCI_PORTSC_LSDA)
			status |= UPS_LOW_SPEED;
		status |= UPS_PORT_POWER;
		if (sc->sc_isreset)
			change |= UPS_C_PORT_RESET;
		USETW(ps.wPortStatus, status);
		USETW(ps.wPortChange, change);
		totlen = uimin(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		return -1;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index == 1)
			port = UHCI_PORTSC1;
		else if (index == 2)
			port = UHCI_PORTSC2;
		else {
			return -1;
		}
		switch(value) {
		case UHF_PORT_ENABLE:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x | UHCI_PORTSC_PE);
			break;
		case UHF_PORT_SUSPEND:
			x = URWMASK(UREAD2(sc, port));
			UWRITE2(sc, port, x | UHCI_PORTSC_SUSP);
			break;
		case UHF_PORT_RESET:
			err = uhci_portreset(sc, index);
			if (err != USBD_NORMAL_COMPLETION)
				return -1;
			return 0;
		case UHF_PORT_POWER:
			/* Pretend we turned on power */
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
		DPRINTF("returning %jd (usbroothub default)",
		    buflen, 0, 0, 0);
		return buflen;
	}

	DPRINTF("returning %jd", totlen, 0, 0, 0);

	return totlen;
}

/* Abort a root interrupt request. */
void
uhci_root_intr_abort(struct usbd_xfer *xfer)
{
	uhci_softc_t *sc = UHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	/*
	 * Try to stop the callout before it starts.  If we got in too
	 * late, too bad; but if the callout had yet to run and time
	 * out the xfer, cancel it ourselves.
	 */
	callout_stop(&sc->sc_poll_handle);
	if (sc->sc_intr_xfer == NULL)
		return;

	KASSERT(sc->sc_intr_xfer == xfer);
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);
	xfer->ux_status = USBD_CANCELLED;
#ifdef DIAGNOSTIC
	UHCI_XFER2UXFER(xfer)->ux_isdone = true;
#endif
	usb_transfer_complete(xfer);
}

usbd_status
uhci_root_intr_transfer(struct usbd_xfer *xfer)
{

	/* Pipe isn't running, start first */
	return uhci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

/* Start a transfer on the root interrupt pipe */
usbd_status
uhci_root_intr_start(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	uhci_softc_t *sc = UHCI_PIPE2SC(pipe);
	unsigned int ival;
	const bool polling = sc->sc_bus.ub_usepolling;

	UHCIHIST_FUNC(); UHCIHIST_CALLED();
	DPRINTF("xfer=%#jx len=%jd flags=%jd", (uintptr_t)xfer, xfer->ux_length,
	    xfer->ux_flags, 0);

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (!polling)
		mutex_enter(&sc->sc_lock);

	KASSERT(sc->sc_intr_xfer == NULL);

	/* XXX temporary variable needed to avoid gcc3 warning */
	ival = xfer->ux_pipe->up_endpoint->ue_edesc->bInterval;
	sc->sc_ival = mstohz(ival);
	callout_schedule(&sc->sc_poll_handle, sc->sc_ival);
	sc->sc_intr_xfer = xfer;
	xfer->ux_status = USBD_IN_PROGRESS;

	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Close the root interrupt pipe. */
void
uhci_root_intr_close(struct usbd_pipe *pipe)
{
	uhci_softc_t *sc __diagused = UHCI_PIPE2SC(pipe);
	UHCIHIST_FUNC(); UHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * The caller must arrange to have aborted the pipe already, so
	 * there can be no intr xfer in progress.  The callout may
	 * still be pending from a prior intr xfer -- if it has already
	 * fired, it will see there is nothing to do, and do nothing.
	 */
	KASSERT(sc->sc_intr_xfer == NULL);
	KASSERT(!callout_pending(&sc->sc_poll_handle));
}
