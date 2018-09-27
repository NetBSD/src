/*	$NetBSD: ohci.c,v 1.273.6.4 2018/09/27 14:52:26 martin Exp $	*/

/*
 * Copyright (c) 1998, 2004, 2005, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, Jared D. McNeill (jmcneill@invisible.ca)
 * and Matthew R. Green (mrg@eterna.com.au).
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * USB Open Host Controller driver.
 *
 * OHCI spec: http://www.compaq.com/productinfo/development/openhci.html
 * USB spec: http://www.usb.org/developers/docs/
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ohci.c,v 1.273.6.4 2018/09/27 14:52:26 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

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
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>
#include <dev/usb/usbroothub.h>
#include <dev/usb/usbhist.h>

#ifdef USB_DEBUG
#ifndef OHCI_DEBUG
#define ohcidebug 0
#else
static int ohcidebug = 10;

SYSCTL_SETUP(sysctl_hw_ohci_setup, "sysctl hw.ohci setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "ohci",
	    SYSCTL_DESCR("ohci global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &ohcidebug, sizeof(ohcidebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* OHCI_DEBUG */
#endif /* USB_DEBUG */

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(ohcidebug,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(ohcidebug,N,FMT,A,B,C,D)
#define	OHCIHIST_FUNC()		USBHIST_FUNC()
#define	OHCIHIST_CALLED(name)	USBHIST_CALLED(ohcidebug)

#if BYTE_ORDER == BIG_ENDIAN
#define	SWAP_ENDIAN	OHCI_LITTLE_ENDIAN
#else
#define	SWAP_ENDIAN	OHCI_BIG_ENDIAN
#endif

#define	O16TOH(val)	(sc->sc_endian == SWAP_ENDIAN ? bswap16(val) : val)
#define	O32TOH(val)	(sc->sc_endian == SWAP_ENDIAN ? bswap32(val) : val)
#define	HTOO16(val)	O16TOH(val)
#define	HTOO32(val)	O32TOH(val)

struct ohci_pipe;

Static ohci_soft_ed_t  *ohci_alloc_sed(ohci_softc_t *);
Static void		ohci_free_sed(ohci_softc_t *, ohci_soft_ed_t *);

Static ohci_soft_td_t  *ohci_alloc_std(ohci_softc_t *);
Static void		ohci_free_std(ohci_softc_t *, ohci_soft_td_t *);
Static void		ohci_free_std_locked(ohci_softc_t *, ohci_soft_td_t *);

Static ohci_soft_itd_t *ohci_alloc_sitd(ohci_softc_t *);
Static void		ohci_free_sitd(ohci_softc_t *,ohci_soft_itd_t *);
Static void		ohci_free_sitd_locked(ohci_softc_t *,
			    ohci_soft_itd_t *);

Static int		ohci_alloc_std_chain(ohci_softc_t *, struct usbd_xfer *,
			    int, int);
Static void		ohci_free_stds(ohci_softc_t *, struct ohci_xfer *);

Static void		ohci_reset_std_chain(ohci_softc_t *, struct usbd_xfer *,
			    int, int, ohci_soft_td_t *, ohci_soft_td_t **);

Static usbd_status	ohci_open(struct usbd_pipe *);
Static void		ohci_poll(struct usbd_bus *);
Static void		ohci_softintr(void *);
Static void		ohci_rhsc(ohci_softc_t *, struct usbd_xfer *);
Static void		ohci_rhsc_softint(void *);

Static void		ohci_add_ed(ohci_softc_t *, ohci_soft_ed_t *,
			    ohci_soft_ed_t *);

Static void		ohci_rem_ed(ohci_softc_t *, ohci_soft_ed_t *,
				    ohci_soft_ed_t *);
Static void		ohci_hash_add_td(ohci_softc_t *, ohci_soft_td_t *);
Static void		ohci_hash_rem_td(ohci_softc_t *, ohci_soft_td_t *);
Static ohci_soft_td_t  *ohci_hash_find_td(ohci_softc_t *, ohci_physaddr_t);
Static void		ohci_hash_add_itd(ohci_softc_t *, ohci_soft_itd_t *);
Static void		ohci_hash_rem_itd(ohci_softc_t *, ohci_soft_itd_t *);
Static ohci_soft_itd_t  *ohci_hash_find_itd(ohci_softc_t *, ohci_physaddr_t);

Static usbd_status	ohci_setup_isoc(struct usbd_pipe *);
Static void		ohci_device_isoc_enter(struct usbd_xfer *);

Static struct usbd_xfer *
			ohci_allocx(struct usbd_bus *, unsigned int);
Static void		ohci_freex(struct usbd_bus *, struct usbd_xfer *);
Static void		ohci_get_lock(struct usbd_bus *, kmutex_t **);
Static int		ohci_roothub_ctrl(struct usbd_bus *,
			    usb_device_request_t *, void *, int);

Static usbd_status	ohci_root_intr_transfer(struct usbd_xfer *);
Static usbd_status	ohci_root_intr_start(struct usbd_xfer *);
Static void		ohci_root_intr_abort(struct usbd_xfer *);
Static void		ohci_root_intr_close(struct usbd_pipe *);
Static void		ohci_root_intr_done(struct usbd_xfer *);

Static int		ohci_device_ctrl_init(struct usbd_xfer *);
Static void		ohci_device_ctrl_fini(struct usbd_xfer *);
Static usbd_status	ohci_device_ctrl_transfer(struct usbd_xfer *);
Static usbd_status	ohci_device_ctrl_start(struct usbd_xfer *);
Static void		ohci_device_ctrl_abort(struct usbd_xfer *);
Static void		ohci_device_ctrl_close(struct usbd_pipe *);
Static void		ohci_device_ctrl_done(struct usbd_xfer *);

Static int		ohci_device_bulk_init(struct usbd_xfer *);
Static void		ohci_device_bulk_fini(struct usbd_xfer *);
Static usbd_status	ohci_device_bulk_transfer(struct usbd_xfer *);
Static usbd_status	ohci_device_bulk_start(struct usbd_xfer *);
Static void		ohci_device_bulk_abort(struct usbd_xfer *);
Static void		ohci_device_bulk_close(struct usbd_pipe *);
Static void		ohci_device_bulk_done(struct usbd_xfer *);

Static int		ohci_device_intr_init(struct usbd_xfer *);
Static void		ohci_device_intr_fini(struct usbd_xfer *);
Static usbd_status	ohci_device_intr_transfer(struct usbd_xfer *);
Static usbd_status	ohci_device_intr_start(struct usbd_xfer *);
Static void		ohci_device_intr_abort(struct usbd_xfer *);
Static void		ohci_device_intr_close(struct usbd_pipe *);
Static void		ohci_device_intr_done(struct usbd_xfer *);

Static int		ohci_device_isoc_init(struct usbd_xfer *);
Static void		ohci_device_isoc_fini(struct usbd_xfer *);
Static usbd_status	ohci_device_isoc_transfer(struct usbd_xfer *);
Static void		ohci_device_isoc_abort(struct usbd_xfer *);
Static void		ohci_device_isoc_close(struct usbd_pipe *);
Static void		ohci_device_isoc_done(struct usbd_xfer *);

Static usbd_status	ohci_device_setintr(ohci_softc_t *,
			    struct ohci_pipe *, int);

Static void		ohci_timeout(void *);
Static void		ohci_timeout_task(void *);
Static void		ohci_rhsc_enable(void *);

Static void		ohci_close_pipe(struct usbd_pipe *, ohci_soft_ed_t *);
Static void		ohci_abort_xfer(struct usbd_xfer *, usbd_status);

Static void		ohci_device_clear_toggle(struct usbd_pipe *);
Static void		ohci_noop(struct usbd_pipe *);

#ifdef OHCI_DEBUG
Static void		ohci_dumpregs(ohci_softc_t *);
Static void		ohci_dump_tds(ohci_softc_t *, ohci_soft_td_t *);
Static void		ohci_dump_td(ohci_softc_t *, ohci_soft_td_t *);
Static void		ohci_dump_ed(ohci_softc_t *, ohci_soft_ed_t *);
Static void		ohci_dump_itd(ohci_softc_t *, ohci_soft_itd_t *);
Static void		ohci_dump_itds(ohci_softc_t *, ohci_soft_itd_t *);
#endif

#define OBARR(sc) bus_space_barrier((sc)->iot, (sc)->ioh, 0, (sc)->sc_size, \
			BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)
#define OWRITE1(sc, r, x) \
 do { OBARR(sc); bus_space_write_1((sc)->iot, (sc)->ioh, (r), (x)); } while (0)
#define OWRITE2(sc, r, x) \
 do { OBARR(sc); bus_space_write_2((sc)->iot, (sc)->ioh, (r), (x)); } while (0)
#define OWRITE4(sc, r, x) \
 do { OBARR(sc); bus_space_write_4((sc)->iot, (sc)->ioh, (r), (x)); } while (0)

static __inline uint32_t
OREAD4(ohci_softc_t *sc, bus_size_t r)
{

	OBARR(sc);
	return bus_space_read_4(sc->iot, sc->ioh, r);
}

/* Reverse the bits in a value 0 .. 31 */
Static uint8_t revbits[OHCI_NO_INTRS] =
  { 0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
    0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
    0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f };

struct ohci_pipe {
	struct usbd_pipe pipe;
	ohci_soft_ed_t *sed;
	union {
		ohci_soft_td_t *td;
		ohci_soft_itd_t *itd;
	} tail;
	/* Info needed for different pipe kinds. */
	union {
		/* Control pipe */
		struct {
			usb_dma_t reqdma;
		} ctrl;
		/* Interrupt pipe */
		struct {
			int nslots;
			int pos;
		} intr;
		/* Isochronous pipe */
		struct isoc {
			int next, inuse;
		} isoc;
	};
};

Static const struct usbd_bus_methods ohci_bus_methods = {
	.ubm_open =	ohci_open,
	.ubm_softint =	ohci_softintr,
	.ubm_dopoll =	ohci_poll,
	.ubm_allocx =	ohci_allocx,
	.ubm_freex =	ohci_freex,
	.ubm_getlock =	ohci_get_lock,
	.ubm_rhctrl =	ohci_roothub_ctrl,
};

Static const struct usbd_pipe_methods ohci_root_intr_methods = {
	.upm_transfer =	ohci_root_intr_transfer,
	.upm_start =	ohci_root_intr_start,
	.upm_abort =	ohci_root_intr_abort,
	.upm_close =	ohci_root_intr_close,
	.upm_cleartoggle =	ohci_noop,
	.upm_done =	ohci_root_intr_done,
};

Static const struct usbd_pipe_methods ohci_device_ctrl_methods = {
	.upm_init =	ohci_device_ctrl_init,
	.upm_fini =	ohci_device_ctrl_fini,
	.upm_transfer =	ohci_device_ctrl_transfer,
	.upm_start =	ohci_device_ctrl_start,
	.upm_abort =	ohci_device_ctrl_abort,
	.upm_close =	ohci_device_ctrl_close,
	.upm_cleartoggle =	ohci_noop,
	.upm_done =	ohci_device_ctrl_done,
};

Static const struct usbd_pipe_methods ohci_device_intr_methods = {
	.upm_init =	ohci_device_intr_init,
	.upm_fini =	ohci_device_intr_fini,
	.upm_transfer =	ohci_device_intr_transfer,
	.upm_start =	ohci_device_intr_start,
	.upm_abort =	ohci_device_intr_abort,
	.upm_close =	ohci_device_intr_close,
	.upm_cleartoggle =	ohci_device_clear_toggle,
	.upm_done =	ohci_device_intr_done,
};

Static const struct usbd_pipe_methods ohci_device_bulk_methods = {
	.upm_init =	ohci_device_bulk_init,
	.upm_fini =	ohci_device_bulk_fini,
	.upm_transfer =	ohci_device_bulk_transfer,
	.upm_start =	ohci_device_bulk_start,
	.upm_abort =	ohci_device_bulk_abort,
	.upm_close =	ohci_device_bulk_close,
	.upm_cleartoggle =	ohci_device_clear_toggle,
	.upm_done =	ohci_device_bulk_done,
};

Static const struct usbd_pipe_methods ohci_device_isoc_methods = {
	.upm_init =	ohci_device_isoc_init,
	.upm_fini =	ohci_device_isoc_fini,
	.upm_transfer =	ohci_device_isoc_transfer,
	.upm_abort =	ohci_device_isoc_abort,
	.upm_close =	ohci_device_isoc_close,
	.upm_cleartoggle =	ohci_noop,
	.upm_done =	ohci_device_isoc_done,
};

int
ohci_activate(device_t self, enum devact act)
{
	struct ohci_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
ohci_childdet(device_t self, device_t child)
{
	struct ohci_softc *sc = device_private(self);

	KASSERT(sc->sc_child == child);
	sc->sc_child = NULL;
}

int
ohci_detach(struct ohci_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	if (rv != 0)
		return rv;

	callout_halt(&sc->sc_tmo_rhsc, &sc->sc_lock);

	usb_delay_ms(&sc->sc_bus, 300); /* XXX let stray task complete */
	callout_destroy(&sc->sc_tmo_rhsc);

	softint_disestablish(sc->sc_rhsc_si);

	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);

	if (sc->sc_hcca != NULL)
		usb_freemem(&sc->sc_bus, &sc->sc_hccadma);
	pool_cache_destroy(sc->sc_xferpool);

	return rv;
}

ohci_soft_ed_t *
ohci_alloc_sed(ohci_softc_t *sc)
{
	ohci_soft_ed_t *sed;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freeeds == NULL) {
		DPRINTFN(2, "allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		err = usb_allocmem(&sc->sc_bus, OHCI_SED_SIZE * OHCI_SED_CHUNK,
			  OHCI_ED_ALIGN, &dma);
		if (err)
			return 0;

		mutex_enter(&sc->sc_lock);
		for (i = 0; i < OHCI_SED_CHUNK; i++) {
			offs = i * OHCI_SED_SIZE;
			sed = KERNADDR(&dma, offs);
			sed->physaddr = DMAADDR(&dma, offs);
			sed->dma = dma;
			sed->offs = offs;
			sed->next = sc->sc_freeeds;
			sc->sc_freeeds = sed;
		}
	}
	sed = sc->sc_freeeds;
	sc->sc_freeeds = sed->next;
	mutex_exit(&sc->sc_lock);

	memset(&sed->ed, 0, sizeof(ohci_ed_t));
	sed->next = 0;
	return sed;
}

static inline void
ohci_free_sed_locked(ohci_softc_t *sc, ohci_soft_ed_t *sed)
{

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	sed->next = sc->sc_freeeds;
	sc->sc_freeeds = sed;
}

void
ohci_free_sed(ohci_softc_t *sc, ohci_soft_ed_t *sed)
{

	mutex_enter(&sc->sc_lock);
	ohci_free_sed_locked(sc, sed);
	mutex_exit(&sc->sc_lock);
}

ohci_soft_td_t *
ohci_alloc_std(ohci_softc_t *sc)
{
	ohci_soft_td_t *std;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freetds == NULL) {
		DPRINTFN(2, "allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		err = usb_allocmem(&sc->sc_bus, OHCI_STD_SIZE * OHCI_STD_CHUNK,
			  OHCI_TD_ALIGN, &dma);
		if (err)
			return NULL;

		mutex_enter(&sc->sc_lock);
		for (i = 0; i < OHCI_STD_CHUNK; i++) {
			offs = i * OHCI_STD_SIZE;
			std = KERNADDR(&dma, offs);
			std->physaddr = DMAADDR(&dma, offs);
			std->dma = dma;
			std->offs = offs;
			std->nexttd = sc->sc_freetds;
			sc->sc_freetds = std;
		}
	}

	std = sc->sc_freetds;
	sc->sc_freetds = std->nexttd;
	mutex_exit(&sc->sc_lock);

	memset(&std->td, 0, sizeof(ohci_td_t));
	std->nexttd = NULL;
	std->xfer = NULL;

	return std;
}

void
ohci_free_std_locked(ohci_softc_t *sc, ohci_soft_td_t *std)
{

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	std->nexttd = sc->sc_freetds;
	sc->sc_freetds = std;
}

void
ohci_free_std(ohci_softc_t *sc, ohci_soft_td_t *std)
{

	mutex_enter(&sc->sc_lock);
	ohci_free_std_locked(sc, std);
	mutex_exit(&sc->sc_lock);
}

Static int
ohci_alloc_std_chain(ohci_softc_t *sc, struct usbd_xfer *xfer, int length, int rd)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	uint16_t flags = xfer->ux_flags;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTFN(8, "addr=%jd endpt=%jd len=%jd speed=%jd",
	    xfer->ux_pipe->up_dev->ud_addr,
	    UE_GET_ADDR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress),
	    length, xfer->ux_pipe->up_dev->ud_speed);

	ASSERT_SLEEPABLE();
	KASSERT(length != 0 || (!rd && (flags & USBD_FORCE_SHORT_XFER)));

	size_t nstd = (!rd && (flags & USBD_FORCE_SHORT_XFER)) ? 1 : 0;
	nstd += ((length + OHCI_PAGE_SIZE - 1) / OHCI_PAGE_SIZE);
	ox->ox_stds = kmem_zalloc(sizeof(ohci_soft_td_t *) * nstd,
	    KM_SLEEP);
	ox->ox_nstd = nstd;

	DPRINTFN(8, "xfer %#jx nstd %jd", (uintptr_t)xfer, nstd, 0, 0);

	for (size_t j = 0; j < ox->ox_nstd;) {
		ohci_soft_td_t *cur = ohci_alloc_std(sc);
		if (cur == NULL)
			goto nomem;

		ox->ox_stds[j++] = cur;
		cur->xfer = xfer;
		cur->flags = 0;
	}

	return 0;

 nomem:
	ohci_free_stds(sc, ox);
	kmem_free(ox->ox_stds, sizeof(ohci_soft_td_t *) * nstd);

	return ENOMEM;
}

Static void
ohci_free_stds(ohci_softc_t *sc, struct ohci_xfer *ox)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("ox=%#jx", (uintptr_t)ox, 0, 0, 0);

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < ox->ox_nstd; i++) {
		ohci_soft_td_t *std = ox->ox_stds[i];
		if (std == NULL)
			break;
		ohci_free_std_locked(sc, std);
	}
	mutex_exit(&sc->sc_lock);
}

void
ohci_reset_std_chain(ohci_softc_t *sc, struct usbd_xfer *xfer,
    int alen, int rd, ohci_soft_td_t *sp, ohci_soft_td_t **ep)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	ohci_soft_td_t *next, *cur;
	int len, curlen;
	usb_dma_t *dma = &xfer->ux_dmabuf;
	uint16_t flags = xfer->ux_flags;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("start len=%jd", alen, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(8, "addr=%jd endpt=%jd len=%jd speed=%jd",
	    xfer->ux_pipe->up_dev->ud_addr,
	    UE_GET_ADDR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress),
	    alen, xfer->ux_pipe->up_dev->ud_speed);

	KASSERT(sp);

	int mps = UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize);

	/*
	 * Assign next for the len == 0 case where we don't go through the
	 * main loop.
	 */
	len = alen;
	cur = next = sp;

	usb_syncmem(dma, 0, len,
	    rd ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	const uint32_t tdflags = HTOO32(
	    (rd ? OHCI_TD_IN : OHCI_TD_OUT) |
	    OHCI_TD_NOCC | OHCI_TD_TOGGLE_CARRY | OHCI_TD_NOINTR);

	size_t curoffs = 0;
	for (size_t j = 1; len != 0;) {
		if (j == ox->ox_nstd)
			next = NULL;
		else
			next = ox->ox_stds[j++];
		KASSERT(next != cur);

		curlen = 0;
		const ohci_physaddr_t sdataphys = DMAADDR(dma, curoffs);
		ohci_physaddr_t edataphys = DMAADDR(dma, curoffs + len - 1);

		const ohci_physaddr_t sphyspg = OHCI_PAGE(sdataphys);
		ohci_physaddr_t ephyspg = OHCI_PAGE(edataphys);
		/*
		 * The OHCI hardware can handle at most one page
		 * crossing per TD
		 */
		curlen = len;
		if (sphyspg != ephyspg &&
		    sphyspg + OHCI_PAGE_SIZE != ephyspg) {
			/* must use multiple TDs, fill as much as possible. */
			curlen = 2 * OHCI_PAGE_SIZE -
			    OHCI_PAGE_OFFSET(sdataphys);
			/* the length must be a multiple of the max size */
			curlen -= curlen % mps;
			edataphys = DMAADDR(dma, curoffs + curlen - 1);
		}
		KASSERT(curlen != 0);
		DPRINTFN(4, "sdataphys=0x%08jx edataphys=0x%08jx "
		    "len=%jd curlen=%jd", sdataphys, edataphys, len, curlen);

		cur->td.td_flags = tdflags;
		cur->td.td_cbp = HTOO32(sdataphys);
		cur->td.td_be = HTOO32(edataphys);
		cur->td.td_nexttd = (next != NULL) ? HTOO32(next->physaddr) : 0;
		cur->nexttd = next;
		cur->len = curlen;
		cur->flags = OHCI_ADD_LEN;
		cur->xfer = xfer;
	 	ohci_hash_add_td(sc, cur);

		curoffs += curlen;
		len -= curlen;

		if (len != 0) {
			KASSERT(next != NULL);
			DPRINTFN(10, "extend chain", 0, 0, 0, 0);
			usb_syncmem(&cur->dma, cur->offs, sizeof(cur->td),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			cur = next;
		}
	}
	cur->td.td_flags |=
	    HTOO32(xfer->ux_flags & USBD_SHORT_XFER_OK ? OHCI_TD_R : 0);

	if (!rd &&
	    (flags & USBD_FORCE_SHORT_XFER) &&
	    alen % mps == 0) {
		/* We're adding a ZLP so sync the previous TD */
		usb_syncmem(&cur->dma, cur->offs, sizeof(cur->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		/* Force a 0 length transfer at the end. */

		KASSERT(next != NULL);
		cur = next;

		cur->td.td_flags = tdflags;
		cur->td.td_cbp = 0; /* indicate 0 length packet */
		cur->td.td_nexttd = 0;
		cur->td.td_be = ~0;
		cur->nexttd = NULL;
		cur->len = 0;
		cur->flags = 0;
		cur->xfer = xfer;
	 	ohci_hash_add_td(sc, cur);

		DPRINTFN(2, "add 0 xfer", 0, 0, 0, 0);
	}

	/* Last TD gets usb_syncmem'ed by caller */
	*ep = cur;
}

ohci_soft_itd_t *
ohci_alloc_sitd(ohci_softc_t *sc)
{
	ohci_soft_itd_t *sitd;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_freeitds == NULL) {
		DPRINTFN(2, "allocating chunk", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);

		err = usb_allocmem(&sc->sc_bus, OHCI_SITD_SIZE * OHCI_SITD_CHUNK,
			  OHCI_ITD_ALIGN, &dma);
		if (err)
			return NULL;
		mutex_enter(&sc->sc_lock);
		for (i = 0; i < OHCI_SITD_CHUNK; i++) {
			offs = i * OHCI_SITD_SIZE;
			sitd = KERNADDR(&dma, offs);
			sitd->physaddr = DMAADDR(&dma, offs);
			sitd->dma = dma;
			sitd->offs = offs;
			sitd->nextitd = sc->sc_freeitds;
			sc->sc_freeitds = sitd;
		}
	}

	sitd = sc->sc_freeitds;
	sc->sc_freeitds = sitd->nextitd;
	mutex_exit(&sc->sc_lock);

	memset(&sitd->itd, 0, sizeof(ohci_itd_t));
	sitd->nextitd = NULL;
	sitd->xfer = NULL;

#ifdef DIAGNOSTIC
	sitd->isdone = true;
#endif

	return sitd;
}

Static void
ohci_free_sitd_locked(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(10, "sitd=%#jx", (uintptr_t)sitd, 0, 0, 0);

	KASSERT(sitd->isdone);
#ifdef DIAGNOSTIC
	/* Warn double free */
	sitd->isdone = false;
#endif

	sitd->nextitd = sc->sc_freeitds;
	sc->sc_freeitds = sitd;
}

void
ohci_free_sitd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	ohci_free_sitd_locked(sc, sitd);
	mutex_exit(&sc->sc_lock);
}

int
ohci_init(ohci_softc_t *sc)
{
	ohci_soft_ed_t *sed, *psed;
	usbd_status err;
	int i;
	uint32_t s, ctl, rwc, ival, hcr, fm, per, rev, desca /*, descb */;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	aprint_normal_dev(sc->sc_dev, "");

	sc->sc_hcca = NULL;
	callout_init(&sc->sc_tmo_rhsc, CALLOUT_MPSAFE);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);

	sc->sc_rhsc_si = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    ohci_rhsc_softint, sc);

	for (i = 0; i < OHCI_HASH_SIZE; i++)
		LIST_INIT(&sc->sc_hash_tds[i]);
	for (i = 0; i < OHCI_HASH_SIZE; i++)
		LIST_INIT(&sc->sc_hash_itds[i]);

	sc->sc_xferpool = pool_cache_init(sizeof(struct ohci_xfer), 0, 0, 0,
	    "ohcixfer", NULL, IPL_USB, NULL, NULL, NULL);

	rev = OREAD4(sc, OHCI_REVISION);
	aprint_normal("OHCI version %d.%d%s\n",
	    OHCI_REV_HI(rev), OHCI_REV_LO(rev),
	    OHCI_REV_LEGACY(rev) ? ", legacy support" : "");

	if (OHCI_REV_HI(rev) != 1 || OHCI_REV_LO(rev) != 0) {
		aprint_error_dev(sc->sc_dev, "unsupported OHCI revision\n");
		sc->sc_bus.ub_revision = USBREV_UNKNOWN;
		return -1;
	}
	sc->sc_bus.ub_revision = USBREV_1_0;
	sc->sc_bus.ub_usedma = true;

	/* XXX determine alignment by R/W */
	/* Allocate the HCCA area. */
	err = usb_allocmem(&sc->sc_bus, OHCI_HCCA_SIZE,
			 OHCI_HCCA_ALIGN, &sc->sc_hccadma);
	if (err) {
		sc->sc_hcca = NULL;
		return err;
	}
	sc->sc_hcca = KERNADDR(&sc->sc_hccadma, 0);
	memset(sc->sc_hcca, 0, OHCI_HCCA_SIZE);

	sc->sc_eintrs = OHCI_NORMAL_INTRS;

	/* Allocate dummy ED that starts the control list. */
	sc->sc_ctrl_head = ohci_alloc_sed(sc);
	if (sc->sc_ctrl_head == NULL) {
		err = ENOMEM;
		goto bad1;
	}
	sc->sc_ctrl_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);

	/* Allocate dummy ED that starts the bulk list. */
	sc->sc_bulk_head = ohci_alloc_sed(sc);
	if (sc->sc_bulk_head == NULL) {
		err = ENOMEM;
		goto bad2;
	}
	sc->sc_bulk_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	usb_syncmem(&sc->sc_bulk_head->dma, sc->sc_bulk_head->offs,
	    sizeof(sc->sc_bulk_head->ed),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate dummy ED that starts the isochronous list. */
	sc->sc_isoc_head = ohci_alloc_sed(sc);
	if (sc->sc_isoc_head == NULL) {
		err = ENOMEM;
		goto bad3;
	}
	sc->sc_isoc_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	usb_syncmem(&sc->sc_isoc_head->dma, sc->sc_isoc_head->offs,
	    sizeof(sc->sc_isoc_head->ed),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Allocate all the dummy EDs that make up the interrupt tree. */
	for (i = 0; i < OHCI_NO_EDS; i++) {
		sed = ohci_alloc_sed(sc);
		if (sed == NULL) {
			while (--i >= 0)
				ohci_free_sed(sc, sc->sc_eds[i]);
			err = ENOMEM;
			goto bad4;
		}
		/* All ED fields are set to 0. */
		sc->sc_eds[i] = sed;
		sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
		if (i != 0)
			psed = sc->sc_eds[(i-1) / 2];
		else
			psed= sc->sc_isoc_head;
		sed->next = psed;
		sed->ed.ed_nexted = HTOO32(psed->physaddr);
		usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}
	/*
	 * Fill HCCA interrupt table.  The bit reversal is to get
	 * the tree set up properly to spread the interrupts.
	 */
	for (i = 0; i < OHCI_NO_INTRS; i++)
		sc->sc_hcca->hcca_interrupt_table[revbits[i]] =
		    HTOO32(sc->sc_eds[OHCI_NO_EDS-OHCI_NO_INTRS+i]->physaddr);
	usb_syncmem(&sc->sc_hccadma, 0, OHCI_HCCA_SIZE,
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef OHCI_DEBUG
	DPRINTFN(15, "--- dump start ---", 0, 0, 0 ,0);
	if (ohcidebug >= 15) {
		for (i = 0; i < OHCI_NO_EDS; i++) {
			DPRINTFN(15, "ed#%jd ", i, 0, 0, 0);
			ohci_dump_ed(sc, sc->sc_eds[i]);
		}
		DPRINTFN(15, "iso", 0, 0, 0 ,0);
		ohci_dump_ed(sc, sc->sc_isoc_head);
	}
	DPRINTFN(15, "--- dump end ---", 0, 0, 0 ,0);
#endif

	/* Preserve values programmed by SMM/BIOS but lost over reset. */
	ctl = OREAD4(sc, OHCI_CONTROL);
	rwc = ctl & OHCI_RWC;
	fm = OREAD4(sc, OHCI_FM_INTERVAL);
	desca = OREAD4(sc, OHCI_RH_DESCRIPTOR_A);
	/* descb = OREAD4(sc, OHCI_RH_DESCRIPTOR_B); */

	/* Determine in what context we are running. */
	if (ctl & OHCI_IR) {
		/* SMM active, request change */
		DPRINTF("SMM active, request owner change", 0, 0, 0, 0);
		if ((sc->sc_intre & (OHCI_OC | OHCI_MIE)) ==
		    (OHCI_OC | OHCI_MIE))
			OWRITE4(sc, OHCI_INTERRUPT_ENABLE, OHCI_MIE);
		s = OREAD4(sc, OHCI_COMMAND_STATUS);
		OWRITE4(sc, OHCI_COMMAND_STATUS, s | OHCI_OCR);
		for (i = 0; i < 100 && (ctl & OHCI_IR); i++) {
			usb_delay_ms(&sc->sc_bus, 1);
			ctl = OREAD4(sc, OHCI_CONTROL);
		}
		OWRITE4(sc, OHCI_INTERRUPT_DISABLE, OHCI_MIE);
		if ((ctl & OHCI_IR) == 0) {
			aprint_error_dev(sc->sc_dev,
			    "SMM does not respond, resetting\n");
			OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET | rwc);
			goto reset;
		}
#if 0
/* Don't bother trying to reuse the BIOS init, we'll reset it anyway. */
	} else if ((ctl & OHCI_HCFS_MASK) != OHCI_HCFS_RESET) {
		/* BIOS started controller. */
		DPRINTF("BIOS active", 0, 0, 0, 0);
		if ((ctl & OHCI_HCFS_MASK) != OHCI_HCFS_OPERATIONAL) {
			OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_OPERATIONAL | rwc);
			usb_delay_ms(&sc->sc_bus, USB_RESUME_DELAY);
		}
#endif
	} else {
		DPRINTF("cold started", 0 ,0 ,0 ,0);
	reset:
		/* Controller was cold started. */
		usb_delay_ms(&sc->sc_bus, USB_BUS_RESET_DELAY);
	}

	/*
	 * This reset should not be necessary according to the OHCI spec, but
	 * without it some controllers do not start.
	 */
	DPRINTF("sc %#jx: resetting", (uintptr_t)sc, 0, 0, 0);
	OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET | rwc);
	usb_delay_ms(&sc->sc_bus, USB_BUS_RESET_DELAY);

	/* We now own the host controller and the bus has been reset. */

	OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_HCR); /* Reset HC */
	/* Nominal time for a reset is 10 us. */
	for (i = 0; i < 10; i++) {
		delay(10);
		hcr = OREAD4(sc, OHCI_COMMAND_STATUS) & OHCI_HCR;
		if (!hcr)
			break;
	}
	if (hcr) {
		aprint_error_dev(sc->sc_dev, "reset timeout\n");
		err = EIO;
		goto bad5;
	}
#ifdef OHCI_DEBUG
	if (ohcidebug >= 15)
		ohci_dumpregs(sc);
#endif

	/* The controller is now in SUSPEND state, we have 2ms to finish. */

	/* Set up HC registers. */
	OWRITE4(sc, OHCI_HCCA, DMAADDR(&sc->sc_hccadma, 0));
	OWRITE4(sc, OHCI_CONTROL_HEAD_ED, sc->sc_ctrl_head->physaddr);
	OWRITE4(sc, OHCI_BULK_HEAD_ED, sc->sc_bulk_head->physaddr);
	/* disable all interrupts and then switch on all desired interrupts */
	OWRITE4(sc, OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
	/* switch on desired functional features */
	ctl = OREAD4(sc, OHCI_CONTROL);
	ctl &= ~(OHCI_CBSR_MASK | OHCI_LES | OHCI_HCFS_MASK | OHCI_IR);
	ctl |= OHCI_PLE | OHCI_IE | OHCI_CLE | OHCI_BLE |
		OHCI_RATIO_1_4 | OHCI_HCFS_OPERATIONAL | rwc;
	/* And finally start it! */
	OWRITE4(sc, OHCI_CONTROL, ctl);

	/*
	 * The controller is now OPERATIONAL.  Set a some final
	 * registers that should be set earlier, but that the
	 * controller ignores when in the SUSPEND state.
	 */
	ival = OHCI_GET_IVAL(fm);
	fm = (OREAD4(sc, OHCI_FM_INTERVAL) & OHCI_FIT) ^ OHCI_FIT;
	fm |= OHCI_FSMPS(ival) | ival;
	OWRITE4(sc, OHCI_FM_INTERVAL, fm);
	per = OHCI_PERIODIC(ival); /* 90% periodic */
	OWRITE4(sc, OHCI_PERIODIC_START, per);

	if (sc->sc_flags & OHCIF_SUPERIO) {
		/* no overcurrent protection */
		desca |= OHCI_NOCP;
		/*
		 * Clear NoPowerSwitching and PowerOnToPowerGoodTime meaning
		 * that
		 *  - ports are always power switched
		 *  - don't wait for powered root hub port
		 */
		desca &= ~(__SHIFTIN(0xff, OHCI_POTPGT_MASK) | OHCI_NPS);
	}

	/* Fiddle the No OverCurrent Protection bit to avoid chip bug. */
	OWRITE4(sc, OHCI_RH_DESCRIPTOR_A, desca | OHCI_NOCP);
	OWRITE4(sc, OHCI_RH_STATUS, OHCI_LPSC); /* Enable port power */
	usb_delay_ms(&sc->sc_bus, OHCI_ENABLE_POWER_DELAY);
	OWRITE4(sc, OHCI_RH_DESCRIPTOR_A, desca);

	/*
	 * The AMD756 requires a delay before re-reading the register,
	 * otherwise it will occasionally report 0 ports.
	 */
	sc->sc_noport = 0;
	for (i = 0; i < 10 && sc->sc_noport == 0; i++) {
		usb_delay_ms(&sc->sc_bus, OHCI_READ_DESC_DELAY);
		sc->sc_noport = OHCI_GET_NDP(OREAD4(sc, OHCI_RH_DESCRIPTOR_A));
	}

#ifdef OHCI_DEBUG
	if (ohcidebug >= 5)
		ohci_dumpregs(sc);
#endif

	/* Set up the bus struct. */
	sc->sc_bus.ub_methods = &ohci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct ohci_pipe);

	sc->sc_control = sc->sc_intre = 0;

	/* Finally, turn on interrupts. */
	DPRINTF("enabling %#jx", sc->sc_eintrs | OHCI_MIE, 0, 0, 0);
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, sc->sc_eintrs | OHCI_MIE);

	return 0;

 bad5:
	for (i = 0; i < OHCI_NO_EDS; i++)
		ohci_free_sed(sc, sc->sc_eds[i]);
 bad4:
	ohci_free_sed(sc, sc->sc_isoc_head);
 bad3:
	ohci_free_sed(sc, sc->sc_bulk_head);
 bad2:
	ohci_free_sed(sc, sc->sc_ctrl_head);
 bad1:
	usb_freemem(&sc->sc_bus, &sc->sc_hccadma);
	sc->sc_hcca = NULL;
	return err;
}

struct usbd_xfer *
ohci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	ohci_softc_t *sc = OHCI_BUS2SC(bus);
	struct usbd_xfer *xfer;

	xfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct ohci_xfer));

		/* Initialise this always so we can call remove on it. */
		usb_init_task(&xfer->ux_aborttask, ohci_timeout_task, xfer,
		    USB_TASKQ_MPSAFE);
#ifdef DIAGNOSTIC
		xfer->ux_state = XFER_BUSY;
#endif
	}
	return xfer;
}

void
ohci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_BUS2SC(bus);

	KASSERTMSG(xfer->ux_state == XFER_BUSY,
	    "xfer=%p not busy, 0x%08x\n", xfer, xfer->ux_state);
#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_FREE;
#endif
	pool_cache_put(sc->sc_xferpool, xfer);
}

Static void
ohci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	ohci_softc_t *sc = OHCI_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

/*
 * Shut down the controller when the system is going down.
 */
bool
ohci_shutdown(device_t self, int flags)
{
	ohci_softc_t *sc = device_private(self);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTF("stopping the HC", 0, 0, 0, 0);
	OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET);
	return true;
}

bool
ohci_resume(device_t dv, const pmf_qual_t *qual)
{
	ohci_softc_t *sc = device_private(dv);
	uint32_t ctl;

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling++;
	mutex_spin_exit(&sc->sc_intr_lock);

	/* Some broken BIOSes do not recover these values */
	OWRITE4(sc, OHCI_HCCA, DMAADDR(&sc->sc_hccadma, 0));
	OWRITE4(sc, OHCI_CONTROL_HEAD_ED,
	    sc->sc_ctrl_head->physaddr);
	OWRITE4(sc, OHCI_BULK_HEAD_ED,
	    sc->sc_bulk_head->physaddr);
	if (sc->sc_intre)
		OWRITE4(sc, OHCI_INTERRUPT_ENABLE, sc->sc_intre &
		    (OHCI_ALL_INTRS | OHCI_MIE));
	if (sc->sc_control)
		ctl = sc->sc_control;
	else
		ctl = OREAD4(sc, OHCI_CONTROL);
	ctl |= OHCI_HCFS_RESUME;
	OWRITE4(sc, OHCI_CONTROL, ctl);
	usb_delay_ms(&sc->sc_bus, USB_RESUME_DELAY);
	ctl = (ctl & ~OHCI_HCFS_MASK) | OHCI_HCFS_OPERATIONAL;
	OWRITE4(sc, OHCI_CONTROL, ctl);
	usb_delay_ms(&sc->sc_bus, USB_RESUME_RECOVERY);
	sc->sc_control = sc->sc_intre = 0;

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling--;
	mutex_spin_exit(&sc->sc_intr_lock);

	return true;
}

bool
ohci_suspend(device_t dv, const pmf_qual_t *qual)
{
	ohci_softc_t *sc = device_private(dv);
	uint32_t ctl;

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling++;
	mutex_spin_exit(&sc->sc_intr_lock);

	ctl = OREAD4(sc, OHCI_CONTROL) & ~OHCI_HCFS_MASK;
	if (sc->sc_control == 0) {
		/*
		 * Preserve register values, in case that BIOS
		 * does not recover them.
		 */
		sc->sc_control = ctl;
		sc->sc_intre = OREAD4(sc,
		    OHCI_INTERRUPT_ENABLE);
	}
	ctl |= OHCI_HCFS_SUSPEND;
	OWRITE4(sc, OHCI_CONTROL, ctl);
	usb_delay_ms(&sc->sc_bus, USB_RESUME_WAIT);

	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_bus.ub_usepolling--;
	mutex_spin_exit(&sc->sc_intr_lock);

	return true;
}

#ifdef OHCI_DEBUG
void
ohci_dumpregs(ohci_softc_t *sc)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTF("rev=0x%08jx control=0x%08jx command=0x%08jx",
		 OREAD4(sc, OHCI_REVISION),
		 OREAD4(sc, OHCI_CONTROL),
		 OREAD4(sc, OHCI_COMMAND_STATUS), 0);
	DPRINTF("               intrstat=0x%08jx intre=0x%08jx intrd=0x%08jx",
		 OREAD4(sc, OHCI_INTERRUPT_STATUS),
		 OREAD4(sc, OHCI_INTERRUPT_ENABLE),
		 OREAD4(sc, OHCI_INTERRUPT_DISABLE), 0);
	DPRINTF("               hcca=0x%08jx percur=0x%08jx ctrlhd=0x%08jx",
		 OREAD4(sc, OHCI_HCCA),
		 OREAD4(sc, OHCI_PERIOD_CURRENT_ED),
		 OREAD4(sc, OHCI_CONTROL_HEAD_ED), 0);
	DPRINTF("               ctrlcur=0x%08jx bulkhd=0x%08jx bulkcur=0x%08jx",
		 OREAD4(sc, OHCI_CONTROL_CURRENT_ED),
		 OREAD4(sc, OHCI_BULK_HEAD_ED),
		 OREAD4(sc, OHCI_BULK_CURRENT_ED) ,0);
	DPRINTF("               done=0x%08jx fmival=0x%08jx fmrem=0x%08jx",
		 OREAD4(sc, OHCI_DONE_HEAD),
		 OREAD4(sc, OHCI_FM_INTERVAL),
		 OREAD4(sc, OHCI_FM_REMAINING), 0);
	DPRINTF("               fmnum=0x%08jx perst=0x%08jx lsthrs=0x%08jx",
		 OREAD4(sc, OHCI_FM_NUMBER),
		 OREAD4(sc, OHCI_PERIODIC_START),
		 OREAD4(sc, OHCI_LS_THRESHOLD), 0);
	DPRINTF("               desca=0x%08jx descb=0x%08jx stat=0x%08jx",
		 OREAD4(sc, OHCI_RH_DESCRIPTOR_A),
		 OREAD4(sc, OHCI_RH_DESCRIPTOR_B),
		 OREAD4(sc, OHCI_RH_STATUS), 0);
	DPRINTF("               port1=0x%08jx port2=0x%08jx",
		 OREAD4(sc, OHCI_RH_PORT_STATUS(1)),
		 OREAD4(sc, OHCI_RH_PORT_STATUS(2)), 0, 0);
	DPRINTF("         HCCA: frame_number=0x%04jx done_head=0x%08jx",
		 O32TOH(sc->sc_hcca->hcca_frame_number),
		 O32TOH(sc->sc_hcca->hcca_done_head), 0, 0);
}
#endif

Static int ohci_intr1(ohci_softc_t *);

int
ohci_intr(void *p)
{
	ohci_softc_t *sc = p;
	int ret = 0;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	if (sc == NULL)
		return 0;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	/* If we get an interrupt while polling, then just ignore it. */
	if (sc->sc_bus.ub_usepolling) {
		DPRINTFN(16, "ignored interrupt while polling", 0, 0, 0, 0);
		/* for level triggered intrs, should do something to ack */
		OWRITE4(sc, OHCI_INTERRUPT_STATUS,
			OREAD4(sc, OHCI_INTERRUPT_STATUS));

		goto done;
	}

	ret = ohci_intr1(sc);

done:
	mutex_spin_exit(&sc->sc_intr_lock);
	return ret;
}

Static int
ohci_intr1(ohci_softc_t *sc)
{
	uint32_t intrs, eintrs;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	/* In case the interrupt occurs before initialization has completed. */
	if (sc == NULL || sc->sc_hcca == NULL) {
#ifdef DIAGNOSTIC
		printf("ohci_intr: sc->sc_hcca == NULL\n");
#endif
		return 0;
	}

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	intrs = OREAD4(sc, OHCI_INTERRUPT_STATUS);
	if (!intrs)
		return 0;

	/* Acknowledge */
	OWRITE4(sc, OHCI_INTERRUPT_STATUS, intrs & ~(OHCI_MIE|OHCI_WDH));
	eintrs = intrs & sc->sc_eintrs;
	DPRINTFN(7, "sc=%#jx", (uintptr_t)sc, 0, 0, 0);
	DPRINTFN(7, "intrs=%#jx(%#jx) eintrs=%#jx(%#jx)",
	    intrs, OREAD4(sc, OHCI_INTERRUPT_STATUS), eintrs,
	    sc->sc_eintrs);

	if (!eintrs) {
		return 0;
	}

	if (eintrs & OHCI_SO) {
		sc->sc_overrun_cnt++;
		if (usbd_ratecheck(&sc->sc_overrun_ntc)) {
			printf("%s: %u scheduling overruns\n",
			    device_xname(sc->sc_dev), sc->sc_overrun_cnt);
			sc->sc_overrun_cnt = 0;
		}
		/* XXX do what */
		eintrs &= ~OHCI_SO;
	}
	if (eintrs & OHCI_WDH) {
		/*
		 * We block the interrupt below, and reenable it later from
		 * ohci_softintr().
		 */
		usb_schedsoftintr(&sc->sc_bus);
	}
	if (eintrs & OHCI_RD) {
/* 1 argument, but zero format strings */
		DPRINTFN(5, "resume detect", (uintptr_t)sc, 0, 0, 0);
		printf("%s: resume detect\n", device_xname(sc->sc_dev));
		/* XXX process resume detect */
	}
	if (eintrs & OHCI_UE) {
/* 1 argument, but zero format strings */
		DPRINTFN(5, "unrecoverable error", (uintptr_t)sc, 0, 0, 0);
		printf("%s: unrecoverable error, controller halted\n",
		       device_xname(sc->sc_dev));
		OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET);
		/* XXX what else */
	}
	if (eintrs & OHCI_RHSC) {
		/*
		 * We block the interrupt below, and reenable it later from
		 * a timeout.
		 */
		softint_schedule(sc->sc_rhsc_si);
	}

	if (eintrs != 0) {
		/* Block unprocessed interrupts. */
		OWRITE4(sc, OHCI_INTERRUPT_DISABLE, eintrs);
		sc->sc_eintrs &= ~eintrs;
		DPRINTF("sc %#jx blocking intrs 0x%jx", (uintptr_t)sc,
		    eintrs, 0, 0);
	}

	return 1;
}

void
ohci_rhsc_enable(void *v_sc)
{
	ohci_softc_t *sc = v_sc;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("sc %#jx", (uintptr_t)sc, 0, 0, 0);
	mutex_spin_enter(&sc->sc_intr_lock);
	sc->sc_eintrs |= OHCI_RHSC;
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, OHCI_RHSC);
	mutex_spin_exit(&sc->sc_intr_lock);
}

#ifdef OHCI_DEBUG
const char *ohci_cc_strs[] = {
	"NO_ERROR",
	"CRC",
	"BIT_STUFFING",
	"DATA_TOGGLE_MISMATCH",
	"STALL",
	"DEVICE_NOT_RESPONDING",
	"PID_CHECK_FAILURE",
	"UNEXPECTED_PID",
	"DATA_OVERRUN",
	"DATA_UNDERRUN",
	"BUFFER_OVERRUN",
	"BUFFER_UNDERRUN",
	"reserved",
	"reserved",
	"NOT_ACCESSED",
	"NOT_ACCESSED",
};
#endif

void
ohci_softintr(void *v)
{
	struct usbd_bus *bus = v;
	ohci_softc_t *sc = OHCI_BUS2SC(bus);
	ohci_soft_itd_t *sitd, *sidone, *sitdnext;
	ohci_soft_td_t  *std,  *sdone,  *stdnext;
	struct usbd_xfer *xfer;
	struct ohci_pipe *opipe;
	int len, cc;
	int i, j, actlen, iframes, uedir;
	ohci_physaddr_t done;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	usb_syncmem(&sc->sc_hccadma, offsetof(struct ohci_hcca, hcca_done_head),
	    sizeof(sc->sc_hcca->hcca_done_head),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	done = O32TOH(sc->sc_hcca->hcca_done_head) & ~OHCI_DONE_INTRS;
	sc->sc_hcca->hcca_done_head = 0;
	usb_syncmem(&sc->sc_hccadma, offsetof(struct ohci_hcca, hcca_done_head),
	    sizeof(sc->sc_hcca->hcca_done_head),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	OWRITE4(sc, OHCI_INTERRUPT_STATUS, OHCI_WDH);
	sc->sc_eintrs |= OHCI_WDH;
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, OHCI_WDH);

	/* Reverse the done list. */
	for (sdone = NULL, sidone = NULL; done != 0; ) {
		std = ohci_hash_find_td(sc, done);
		if (std != NULL) {
			usb_syncmem(&std->dma, std->offs, sizeof(std->td),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
			std->dnext = sdone;
			done = O32TOH(std->td.td_nexttd);
			sdone = std;
			DPRINTFN(10, "add TD %#jx", (uintptr_t)std, 0, 0, 0);
			continue;
		}
		sitd = ohci_hash_find_itd(sc, done);
		if (sitd != NULL) {
			usb_syncmem(&sitd->dma, sitd->offs, sizeof(sitd->itd),
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
			sitd->dnext = sidone;
			done = O32TOH(sitd->itd.itd_nextitd);
			sidone = sitd;
			DPRINTFN(5, "add ITD %#jx", (uintptr_t)sitd, 0, 0, 0);
			continue;
		}
		DPRINTFN(10, "addr %#jx not found", (uintptr_t)done, 0, 0, 0);
		device_printf(sc->sc_dev, "WARNING: addr 0x%08lx not found\n",
		    (u_long)done);
		break;
	}

	DPRINTFN(10, "sdone=%#jx sidone=%#jx", (uintptr_t)sdone,
	    (uintptr_t)sidone, 0, 0);
	DPRINTFN(10, "--- TD dump start ---", 0, 0, 0, 0);
#ifdef OHCI_DEBUG
	if (ohcidebug >= 10) {
		for (std = sdone; std; std = std->dnext)
			ohci_dump_td(sc, std);
	}
#endif
	DPRINTFN(10, "--- TD dump end ---", 0, 0, 0, 0);

	for (std = sdone; std; std = stdnext) {
		xfer = std->xfer;
		stdnext = std->dnext;
		DPRINTFN(10, "std=%#jx xfer=%#jx hcpriv=%#jx", (uintptr_t)std,
		    (uintptr_t)xfer, (uintptr_t)(xfer ? xfer->ux_hcpriv : 0),
		    0);
		if (xfer == NULL) {
			/*
			 * xfer == NULL: There seems to be no xfer associated
			 * with this TD. It is tailp that happened to end up on
			 * the done queue.
			 * Shouldn't happen, but some chips are broken(?).
			 */
			continue;
		}

		/*
		 * If software has completed it, either by cancellation
		 * or timeout, drop it on the floor.
		 */
		if (xfer->ux_status != USBD_IN_PROGRESS) {
			KASSERT(xfer->ux_status == USBD_CANCELLED ||
			    xfer->ux_status == USBD_TIMEOUT);
			continue;
		}

		/*
		 * Cancel the timeout and the task, which have not yet
		 * run.  If they have already fired, at worst they are
		 * waiting for the lock.  They will see that the xfer
		 * is no longer in progress and give up.
		 */
		callout_stop(&xfer->ux_callout);
		usb_rem_task(xfer->ux_pipe->up_dev, &xfer->ux_aborttask);

		len = std->len;
		if (std->td.td_cbp != 0)
			len -= O32TOH(std->td.td_be) -
			       O32TOH(std->td.td_cbp) + 1;
		DPRINTFN(10, "len=%jd, flags=0x%jx", len, std->flags, 0, 0);
		if (std->flags & OHCI_ADD_LEN)
			xfer->ux_actlen += len;

		cc = OHCI_TD_GET_CC(O32TOH(std->td.td_flags));
		if (cc == OHCI_CC_NO_ERROR) {
			ohci_hash_rem_td(sc, std);
			if (std->flags & OHCI_CALL_DONE) {
				xfer->ux_status = USBD_NORMAL_COMPLETION;
				usb_transfer_complete(xfer);
			}
		} else {
			/*
			 * Endpoint is halted.  First unlink all the TDs
			 * belonging to the failed transfer, and then restart
			 * the endpoint.
			 */
			ohci_soft_td_t *p, *n;
			opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);

			DPRINTFN(10, "error cc=%jd", cc, 0, 0, 0);

			/* remove xfer's TDs from the hash */
			for (p = std; p->xfer == xfer; p = n) {
				n = p->nexttd;
				ohci_hash_rem_td(sc, p);
			}

			ohci_soft_ed_t *sed = opipe->sed;

			/* clear halt and TD chain */
			sed->ed.ed_headp = HTOO32(p->physaddr);
			usb_syncmem(&sed->dma,
			    sed->offs + offsetof(ohci_ed_t, ed_headp),
			    sizeof(sed->ed.ed_headp),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_CLF);

			if (cc == OHCI_CC_DATA_UNDERRUN)
				xfer->ux_status = USBD_NORMAL_COMPLETION;
			else if (cc == OHCI_CC_STALL)
				xfer->ux_status = USBD_STALLED;
			else
				xfer->ux_status = USBD_IOERROR;
			usb_transfer_complete(xfer);
		}
	}
	DPRINTFN(10, "--- ITD dump start ---", 0, 0, 0, 0);
#ifdef OHCI_DEBUG
	if (ohcidebug >= 10) {
		for (sitd = sidone; sitd; sitd = sitd->dnext)
			ohci_dump_itd(sc, sitd);
	}
#endif
	DPRINTFN(10, "--- ITD dump end ---", 0, 0, 0, 0);

	for (sitd = sidone; sitd != NULL; sitd = sitdnext) {
		xfer = sitd->xfer;
		sitdnext = sitd->dnext;
		DPRINTFN(1, "sitd=%#jx xfer=%#jx hcpriv=%#jx", (uintptr_t)sitd,
		    (uintptr_t)xfer, (uintptr_t)(xfer ? xfer->ux_hcpriv : 0),
		    0);
		if (xfer == NULL)
			continue;

		/*
		 * If software has completed it, either by cancellation
		 * or timeout, drop it on the floor.
		 */
		if (xfer->ux_status != USBD_IN_PROGRESS) {
			KASSERT(xfer->ux_status == USBD_CANCELLED ||
			    xfer->ux_status == USBD_TIMEOUT);
			continue;
		}

		/*
		 * Cancel the timeout and the task, which have not yet
		 * run.  If they have already fired, at worst they are
		 * waiting for the lock.  They will see that the xfer
		 * is no longer in progress and give up.
		 */
		callout_stop(&xfer->ux_callout);
		usb_rem_task(xfer->ux_pipe->up_dev, &xfer->ux_aborttask);

		KASSERT(!sitd->isdone);
#ifdef DIAGNOSTIC
		sitd->isdone = true;
#endif
		if (sitd->flags & OHCI_CALL_DONE) {
			ohci_soft_itd_t *next;

			opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
			opipe->isoc.inuse -= xfer->ux_nframes;
			uedir = UE_GET_DIR(xfer->ux_pipe->up_endpoint->ue_edesc->
			    bEndpointAddress);
			xfer->ux_status = USBD_NORMAL_COMPLETION;
			actlen = 0;
			for (i = 0, sitd = xfer->ux_hcpriv;;
			    sitd = next) {
				next = sitd->nextitd;
				if (OHCI_ITD_GET_CC(O32TOH(sitd->
				    itd.itd_flags)) != OHCI_CC_NO_ERROR)
					xfer->ux_status = USBD_IOERROR;
				/* For input, update frlengths with actual */
				/* XXX anything necessary for output? */
				if (uedir == UE_DIR_IN &&
				    xfer->ux_status == USBD_NORMAL_COMPLETION) {
					iframes = OHCI_ITD_GET_FC(O32TOH(
					    sitd->itd.itd_flags));
					for (j = 0; j < iframes; i++, j++) {
						len = O16TOH(sitd->
						    itd.itd_offset[j]);
						if ((OHCI_ITD_PSW_GET_CC(len) &
						    OHCI_CC_NOT_ACCESSED_MASK)
						    == OHCI_CC_NOT_ACCESSED)
							len = 0;
						else
							len = OHCI_ITD_PSW_LENGTH(len);
						xfer->ux_frlengths[i] = len;
						actlen += len;
					}
				}
				if (sitd->flags & OHCI_CALL_DONE)
					break;
				ohci_hash_rem_itd(sc, sitd);

			}
			ohci_hash_rem_itd(sc, sitd);
			if (uedir == UE_DIR_IN &&
			    xfer->ux_status == USBD_NORMAL_COMPLETION)
				xfer->ux_actlen = actlen;
			xfer->ux_hcpriv = NULL;

			usb_transfer_complete(xfer);
		}
	}

	DPRINTFN(10, "done", 0, 0, 0, 0);
	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
}

void
ohci_device_ctrl_done(struct usbd_xfer *xfer)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);
	int len = UGETW(xfer->ux_request.wLength);
	int isread = (xfer->ux_request.bmRequestType & UT_READ);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(10, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	if (len)
		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	usb_syncmem(&opipe->ctrl.reqdma, 0,
	    sizeof(usb_device_request_t),  BUS_DMASYNC_POSTWRITE);
}

void
ohci_device_intr_done(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);
	int isread =
	    (UE_GET_DIR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(10, "xfer=%#jx, actlen=%jd", (uintptr_t)xfer,
	    xfer->ux_actlen, 0, 0);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

void
ohci_device_bulk_done(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);

	int isread =
	    (UE_GET_DIR(xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(10, "xfer=%#jx, actlen=%jd", (uintptr_t)xfer, xfer->ux_actlen,
	    0, 0);
	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

Static void
ohci_rhsc_softint(void *arg)
{
	ohci_softc_t *sc = arg;

	mutex_enter(&sc->sc_lock);

	ohci_rhsc(sc, sc->sc_intrxfer);

	/* Do not allow RHSC interrupts > 1 per second */
	callout_reset(&sc->sc_tmo_rhsc, hz, ohci_rhsc_enable, sc);

	mutex_exit(&sc->sc_lock);
}

void
ohci_rhsc(ohci_softc_t *sc, struct usbd_xfer *xfer)
{
	u_char *p;
	int i, m;
	int hstatus __unused;
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	hstatus = OREAD4(sc, OHCI_RH_STATUS);
	DPRINTF("sc=%#jx xfer=%#jx hstatus=0x%08jx", (uintptr_t)sc,
	    (uintptr_t)xfer, hstatus, 0);

	if (xfer == NULL) {
		/* Just ignore the change. */
		return;
	}

	p = xfer->ux_buf;
	m = min(sc->sc_noport, xfer->ux_length * 8 - 1);
	memset(p, 0, xfer->ux_length);
	for (i = 1; i <= m; i++) {
		/* Pick out CHANGE bits from the status reg. */
		if (OREAD4(sc, OHCI_RH_PORT_STATUS(i)) >> 16)
			p[i/8] |= 1 << (i%8);
	}
	DPRINTF("change=0x%02jx", *p, 0, 0, 0);
	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);
}

void
ohci_root_intr_done(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	KASSERT(sc->sc_intrxfer == xfer);
	sc->sc_intrxfer = NULL;
}

void
ohci_poll(struct usbd_bus *bus)
{
	ohci_softc_t *sc = OHCI_BUS2SC(bus);
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

#ifdef OHCI_DEBUG
	static int last;
	int new;
	new = OREAD4(sc, OHCI_INTERRUPT_STATUS);
	if (new != last) {
		DPRINTFN(10, "intrs=0x%04jx", new, 0, 0, 0);
		last = new;
	}
#endif
	sc->sc_eintrs |= OHCI_WDH;
	if (OREAD4(sc, OHCI_INTERRUPT_STATUS) & sc->sc_eintrs) {
		mutex_spin_enter(&sc->sc_intr_lock);
		ohci_intr1(sc);
		mutex_spin_exit(&sc->sc_intr_lock);
	}
}

/*
 * Add an ED to the schedule.  Called with USB lock held.
 */
Static void
ohci_add_ed(ohci_softc_t *sc, ohci_soft_ed_t *sed, ohci_soft_ed_t *head)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(8, "sed=%#jx head=%#jx", (uintptr_t)sed, (uintptr_t)head, 0,
	    0);

	KASSERT(mutex_owned(&sc->sc_lock));

	usb_syncmem(&head->dma, head->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(head->ed.ed_nexted),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->next = head->next;
	sed->ed.ed_nexted = head->ed.ed_nexted;
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(sed->ed.ed_nexted),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	head->next = sed;
	head->ed.ed_nexted = HTOO32(sed->physaddr);
	usb_syncmem(&head->dma, head->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(head->ed.ed_nexted),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
}

/*
 * Remove an ED from the schedule.  Called with USB lock held.
 */
Static void
ohci_rem_ed(ohci_softc_t *sc, ohci_soft_ed_t *sed, ohci_soft_ed_t *head)
{
	ohci_soft_ed_t *p;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* XXX */
	for (p = head; p != NULL && p->next != sed; p = p->next)
		;
	KASSERT(p != NULL);

	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(sed->ed.ed_nexted),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	p->next = sed->next;
	p->ed.ed_nexted = sed->ed.ed_nexted;
	usb_syncmem(&p->dma, p->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(p->ed.ed_nexted),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
}

/*
 * When a transfer is completed the TD is added to the done queue by
 * the host controller.  This queue is the processed by software.
 * Unfortunately the queue contains the physical address of the TD
 * and we have no simple way to translate this back to a kernel address.
 * To make the translation possible (and fast) we use a hash table of
 * TDs currently in the schedule.  The physical address is used as the
 * hash value.
 */

#define HASH(a) (((a) >> 4) % OHCI_HASH_SIZE)
/* Called with USB lock held. */
void
ohci_hash_add_td(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	int h = HASH(std->physaddr);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	LIST_INSERT_HEAD(&sc->sc_hash_tds[h], std, hnext);
}

/* Called with USB lock held. */
void
ohci_hash_rem_td(ohci_softc_t *sc, ohci_soft_td_t *std)
{

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	LIST_REMOVE(std, hnext);
}

ohci_soft_td_t *
ohci_hash_find_td(ohci_softc_t *sc, ohci_physaddr_t a)
{
	int h = HASH(a);
	ohci_soft_td_t *std;

	for (std = LIST_FIRST(&sc->sc_hash_tds[h]);
	     std != NULL;
	     std = LIST_NEXT(std, hnext))
		if (std->physaddr == a)
			return std;
	return NULL;
}

/* Called with USB lock held. */
void
ohci_hash_add_itd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	int h = HASH(sitd->physaddr);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(10, "sitd=%#jx physaddr=0x%08jx",
	    (uintptr_t)sitd, (u_long)sitd->physaddr, 0, 0);

	LIST_INSERT_HEAD(&sc->sc_hash_itds[h], sitd, hnext);
}

/* Called with USB lock held. */
void
ohci_hash_rem_itd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(10, "sitd=%#jx physaddr=0x%08jx", (uintptr_t)sitd,
	    sitd->physaddr, 0, 0);

	LIST_REMOVE(sitd, hnext);
}

ohci_soft_itd_t *
ohci_hash_find_itd(ohci_softc_t *sc, ohci_physaddr_t a)
{
	int h = HASH(a);
	ohci_soft_itd_t *sitd;

	for (sitd = LIST_FIRST(&sc->sc_hash_itds[h]);
	     sitd != NULL;
	     sitd = LIST_NEXT(sitd, hnext))
		if (sitd->physaddr == a)
			return sitd;
	return NULL;
}

void
ohci_timeout(void *addr)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	struct usbd_xfer *xfer = addr;
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct usbd_device *dev = xfer->ux_pipe->up_dev;

	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	mutex_enter(&sc->sc_lock);
	if (!sc->sc_dying && xfer->ux_status == USBD_IN_PROGRESS)
		usb_add_task(dev, &xfer->ux_aborttask, USB_TASKQ_HC);
	mutex_exit(&sc->sc_lock);
}

void
ohci_timeout_task(void *addr)
{
	struct usbd_xfer *xfer = addr;
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	mutex_enter(&sc->sc_lock);
	ohci_abort_xfer(xfer, USBD_TIMEOUT);
	mutex_exit(&sc->sc_lock);
}

#ifdef OHCI_DEBUG
void
ohci_dump_tds(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	for (; std; std = std->nexttd) {
		ohci_dump_td(sc, std);
		KASSERTMSG(std->nexttd == NULL || std != std->nexttd,
		    "std %p next %p", std, std->nexttd);
	}
}

void
ohci_dump_td(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	usb_syncmem(&std->dma, std->offs, sizeof(std->td),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	uint32_t flags = O32TOH(std->td.td_flags);
	DPRINTF("TD(%#jx) at 0x%08jx:", (uintptr_t)std, std->physaddr, 0, 0);
	DPRINTF("    round=%jd DP=%jx DI=%jx T=%jx",
	    !!(flags & OHCI_TD_R),
	    __SHIFTOUT(flags, OHCI_TD_DP_MASK),
	    OHCI_TD_GET_DI(flags),
	    __SHIFTOUT(flags, OHCI_TD_TOGGLE_MASK));
	DPRINTF("    EC=%jd CC=%jd", OHCI_TD_GET_EC(flags),
	    OHCI_TD_GET_CC(flags), 0, 0);
	DPRINTF("    td_cbp=0x%08jx td_nexttd=0x%08jx td_be=0x%08jx",
	       (u_long)O32TOH(std->td.td_cbp),
	       (u_long)O32TOH(std->td.td_nexttd),
	       (u_long)O32TOH(std->td.td_be), 0);
}

void
ohci_dump_itd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	usb_syncmem(&sitd->dma, sitd->offs, sizeof(sitd->itd),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	uint32_t flags = O32TOH(sitd->itd.itd_flags);
	DPRINTF("ITD(%#jx) at 0x%08jx", (uintptr_t)sitd, sitd->physaddr, 0, 0);
	DPRINTF("    sf=%jd di=%jd fc=%jd cc=%jd",
	    OHCI_ITD_GET_SF(flags), OHCI_ITD_GET_DI(flags),
	    OHCI_ITD_GET_FC(flags), OHCI_ITD_GET_CC(flags));
	DPRINTF("    bp0=0x%08jx next=0x%08jx be=0x%08jx",
	    O32TOH(sitd->itd.itd_bp0),
	    O32TOH(sitd->itd.itd_nextitd),
	    O32TOH(sitd->itd.itd_be), 0);
	CTASSERT(OHCI_ITD_NOFFSET == 8);
	DPRINTF("    offs[0] = 0x%04jx  offs[1] = 0x%04jx  "
	    "offs[2] = 0x%04jx  offs[3] = 0x%04jx",
	    O16TOH(sitd->itd.itd_offset[0]),
	    O16TOH(sitd->itd.itd_offset[1]),
	    O16TOH(sitd->itd.itd_offset[2]),
	    O16TOH(sitd->itd.itd_offset[3]));
	DPRINTF("    offs[4] = 0x%04jx  offs[5] = 0x%04jx  "
	    "offs[6] = 0x%04jx  offs[7] = 0x%04jx",
	    O16TOH(sitd->itd.itd_offset[4]),
	    O16TOH(sitd->itd.itd_offset[5]),
	    O16TOH(sitd->itd.itd_offset[6]),
	    O16TOH(sitd->itd.itd_offset[7]));
}

void
ohci_dump_itds(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	for (; sitd; sitd = sitd->nextitd)
		ohci_dump_itd(sc, sitd);
}

void
ohci_dump_ed(ohci_softc_t *sc, ohci_soft_ed_t *sed)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	uint32_t flags = O32TOH(sed->ed.ed_flags);
	DPRINTF("ED(%#jx) at 0x%08jx:", (uintptr_t)sed, sed->physaddr, 0, 0);
	DPRINTF("    addr=%jd endpt=%jd maxp=%jd",
	    OHCI_ED_GET_FA(flags),
	    OHCI_ED_GET_EN(flags),
	    OHCI_ED_GET_MAXP(flags),
	    0);
	DPRINTF("    dir=%jd speed=%jd skip=%jd iso=%jd",
	   __SHIFTOUT(flags, OHCI_ED_DIR_MASK),
	    !!(flags & OHCI_ED_SPEED),
	    !!(flags & OHCI_ED_SKIP),
	    !!(flags & OHCI_ED_FORMAT_ISO));
	DPRINTF("    tailp=0x%08jx", (u_long)O32TOH(sed->ed.ed_tailp),
	    0, 0, 0);
	DPRINTF("    headp=0x%08jx nexted=0x%08jx halted=%jd carry=%jd",
	    O32TOH(sed->ed.ed_headp), O32TOH(sed->ed.ed_nexted),
	    !!(O32TOH(sed->ed.ed_headp) & OHCI_HALTED),
	    !!(O32TOH(sed->ed.ed_headp) & OHCI_TOGGLECARRY));
}
#endif

usbd_status
ohci_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	struct usbd_bus *bus = dev->ud_bus;
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	uint8_t addr = dev->ud_addr;
	uint8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	ohci_soft_ed_t *sed;
	ohci_soft_td_t *std;
	ohci_soft_itd_t *sitd;
	ohci_physaddr_t tdphys;
	uint32_t fmt;
	usbd_status err = USBD_NOMEM;
	int ival;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(1, "pipe=%#jx, addr=%jd, endpt=%jd (%jd)", (uintptr_t)pipe,
	    addr, ed->bEndpointAddress, bus->ub_rhaddr);

	if (sc->sc_dying) {
		return USBD_IOERROR;
	}

	std = NULL;
	sed = NULL;

	if (addr == bus->ub_rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &ohci_root_intr_methods;
			break;
		default:
			err = USBD_INVAL;
			goto bad;
		}
	} else {
		sed = ohci_alloc_sed(sc);
		if (sed == NULL)
			goto bad;
		opipe->sed = sed;
		if (xfertype == UE_ISOCHRONOUS) {
			sitd = ohci_alloc_sitd(sc);
			if (sitd == NULL)
				goto bad;

			opipe->tail.itd = sitd;
			tdphys = sitd->physaddr;
			fmt = OHCI_ED_FORMAT_ISO;
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
				fmt |= OHCI_ED_DIR_IN;
			else
				fmt |= OHCI_ED_DIR_OUT;
		} else {
			std = ohci_alloc_std(sc);
			if (std == NULL)
				goto bad;

			opipe->tail.td = std;
			tdphys = std->physaddr;
			fmt = OHCI_ED_FORMAT_GEN | OHCI_ED_DIR_TD;
		}
		sed->ed.ed_flags = HTOO32(
			OHCI_ED_SET_FA(addr) |
			OHCI_ED_SET_EN(UE_GET_ADDR(ed->bEndpointAddress)) |
			(dev->ud_speed == USB_SPEED_LOW ? OHCI_ED_SPEED : 0) |
			fmt |
			OHCI_ED_SET_MAXP(UGETW(ed->wMaxPacketSize)));
		sed->ed.ed_headp = HTOO32(tdphys |
		    (pipe->up_endpoint->ue_toggle ? OHCI_TOGGLECARRY : 0));
		sed->ed.ed_tailp = HTOO32(tdphys);
		usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		switch (xfertype) {
		case UE_CONTROL:
			pipe->up_methods = &ohci_device_ctrl_methods;
			err = usb_allocmem(&sc->sc_bus,
				  sizeof(usb_device_request_t),
				  0, &opipe->ctrl.reqdma);
			if (err)
				goto bad;
			mutex_enter(&sc->sc_lock);
			ohci_add_ed(sc, sed, sc->sc_ctrl_head);
			mutex_exit(&sc->sc_lock);
			break;
		case UE_INTERRUPT:
			pipe->up_methods = &ohci_device_intr_methods;
			ival = pipe->up_interval;
			if (ival == USBD_DEFAULT_INTERVAL)
				ival = ed->bInterval;
			err = ohci_device_setintr(sc, opipe, ival);
			if (err)
				goto bad;
			break;
		case UE_ISOCHRONOUS:
			pipe->up_serialise = false;
			pipe->up_methods = &ohci_device_isoc_methods;
			return ohci_setup_isoc(pipe);
		case UE_BULK:
			pipe->up_methods = &ohci_device_bulk_methods;
			mutex_enter(&sc->sc_lock);
			ohci_add_ed(sc, sed, sc->sc_bulk_head);
			mutex_exit(&sc->sc_lock);
			break;
		}
	}

	return USBD_NORMAL_COMPLETION;

 bad:
	if (std != NULL) {
		ohci_free_std(sc, std);
	}
	if (sed != NULL)
		ohci_free_sed(sc, sed);
	return err;

}

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
void
ohci_close_pipe(struct usbd_pipe *pipe, ohci_soft_ed_t *head)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);
	ohci_soft_ed_t *sed = opipe->sed;

	KASSERT(mutex_owned(&sc->sc_lock));

#ifdef DIAGNOSTIC
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
	    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK)) {
		ohci_soft_td_t *std;
		std = ohci_hash_find_td(sc, O32TOH(sed->ed.ed_headp));
		printf("ohci_close_pipe: pipe not empty sed=%p hd=0x%x "
		       "tl=0x%x pipe=%p, std=%p\n", sed,
		       (int)O32TOH(sed->ed.ed_headp),
		       (int)O32TOH(sed->ed.ed_tailp),
		       pipe, std);
#ifdef OHCI_DEBUG
		usbd_dump_pipe(&opipe->pipe);
		ohci_dump_ed(sc, sed);
		if (std)
			ohci_dump_td(sc, std);
#endif
		usb_delay_ms(&sc->sc_bus, 2);
		if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
		    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK))
			printf("ohci_close_pipe: pipe still not empty\n");
	}
#endif
	ohci_rem_ed(sc, sed, head);
	/* Make sure the host controller is not touching this ED */
	usb_delay_ms(&sc->sc_bus, 1);
	pipe->up_endpoint->ue_toggle =
	    (O32TOH(sed->ed.ed_headp) & OHCI_TOGGLECARRY) ? 1 : 0;
	ohci_free_sed_locked(sc, opipe->sed);
}

/*
 * Cancel or timeout a device request.  We have two cases to deal with
 *
 * 1) A driver wants to stop scheduled or inflight transfers
 * 2) A transfer has timed out
 *
 * It's impossible to guarantee that the requested transfer will not
 * have (partially) happened since the hardware runs concurrently.
 *
 * Transfer state is protected by the bus lock and we set the transfer status
 * as soon as either of the above happens (with bus lock held).
 *
 * Then we arrange for the hardware to tells us that it is not still
 * processing the TDs by setting the sKip bit and requesting a SOF interrupt
 *
 * Once we see the SOF interrupt we can check the transfer TDs/iTDs to see if
 * they've been processed and either
 * 	a) if they're unused recover them for later use, or
 *	b) if they've been used allocate new TD/iTDs to replace those
 *         used.  The softint handler will free the old ones.
 */
void
ohci_abort_xfer(struct usbd_xfer *xfer, usbd_status status)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_ed_t *sed = opipe->sed;
	ohci_soft_td_t *p, *n;
	ohci_physaddr_t headp;
	int hit;

	KASSERTMSG((status == USBD_CANCELLED || status == USBD_TIMEOUT),
	    "invalid status for abort: %d", (int)status);

	DPRINTF("xfer=%#jx pipe=%#jx sed=%#jx", (uintptr_t)xfer,
	    (uintptr_t)opipe, (uintptr_t)sed, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	ASSERT_SLEEPABLE();

	if (status == USBD_CANCELLED) {
		/*
		 * We are synchronously aborting.  Try to stop the
		 * callout and task, but if we can't, wait for them to
		 * complete.
		 */
		callout_halt(&xfer->ux_callout, &sc->sc_lock);
		usb_rem_task_wait(xfer->ux_pipe->up_dev, &xfer->ux_aborttask,
		    USB_TASKQ_HC, &sc->sc_lock);
	} else {
		/* Otherwise, we are timing out.  */
		KASSERT(status == USBD_TIMEOUT);
	}

	/*
	 * The xfer cannot have been cancelled already.  It is the
	 * responsibility of the caller of usbd_abort_pipe not to try
	 * to abort a pipe multiple times, whether concurrently or
	 * sequentially.
	 */
	KASSERT(xfer->ux_status != USBD_CANCELLED);

	/* Only the timeout, which runs only once, can time it out.  */
	KASSERT(xfer->ux_status != USBD_TIMEOUT);

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
		DPRINTFN(4, "xfer %#jx dying %ju", (uintptr_t)xfer,
		    xfer->ux_status, 0, 0);
		goto dying;
	}

	/*
	 * HC Step 1: Unless the endpoint is already halted, we set the endpoint
	 * descriptor sKip bit and wait for hardware to complete processing.
	 *
	 * This includes ensuring that any TDs of the transfer that got onto
	 * the done list are also removed.  We ensure this by waiting for
	 * both a WDH and SOF interrupt.
	 */
	DPRINTFN(1, "stop ed=%#jx", (uintptr_t)sed, 0, 0, 0);
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP); /* force hardware skip */
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/*
	 * HC Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.
	 */
	/* Hardware finishes in 1ms */
	usb_delay_ms_locked(opipe->pipe.up_dev->ud_bus, 20, &sc->sc_lock);

	/*
	 * HC Step 3: Remove any vestiges of the xfer from the hardware.
	 * The complication here is that the hardware may have executed
	 * beyond the xfer we're trying to abort.  So as we're scanning
	 * the TDs of this xfer we check if the hardware points to
	 * any of them.
	 */
	p = xfer->ux_hcpriv;
	KASSERT(p);

#ifdef OHCI_DEBUG
	DPRINTF("--- dump start ---", 0, 0, 0, 0);

	if (ohcidebug >= 2) {
		DPRINTF("sed:", 0, 0, 0, 0);
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, p);
	}
	DPRINTF("--- dump end ---", 0, 0, 0, 0);
#endif
	headp = O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK;
	hit = 0;
	for (; p->xfer == xfer; p = n) {
		hit |= headp == p->physaddr;
		n = p->nexttd;
		ohci_hash_rem_td(sc, p);
	}
	/* Zap headp register if hardware pointed inside the xfer. */
	if (hit) {
		DPRINTFN(1, "set hd=0x%08jx, tl=0x%08jx",  (int)p->physaddr,
		    (int)O32TOH(sed->ed.ed_tailp), 0, 0);
		sed->ed.ed_headp = HTOO32(p->physaddr); /* unlink TDs */
		usb_syncmem(&sed->dma,
		    sed->offs + offsetof(ohci_ed_t, ed_headp),
		    sizeof(sed->ed.ed_headp),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	} else {
		DPRINTFN(1, "no hit", 0, 0, 0, 0);
	}

	/*
	 * HC Step 4: Turn on hardware again.
	 */
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP); /* remove hardware skip */
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/*
	 * Final step: Notify completion to waiting xfers.
	 */
dying:
	usb_transfer_complete(xfer);
	DPRINTFN(14, "end", 0, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
}

/*
 * Data structures and routines to emulate the root hub.
 */
Static int
ohci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	ohci_softc_t *sc = OHCI_BUS2SC(bus);
	usb_port_status_t ps;
	uint16_t len, value, index;
	int l, totlen = 0;
	int port, i;
	uint32_t v;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	if (sc->sc_dying)
		return -1;

	DPRINTFN(4, "type=0x%02jx request=%02jx", req->bmRequestType,
	    req->bRequest, 0, 0);

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(8, "wValue=0x%04jx", value, 0, 0, 0);
		if (len == 0)
			break;
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
			totlen = usb_makestrdesc(sd, len, "OHCI root hub");
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
		DPRINTFN(8, "UR_CLEAR_PORT_FEATURE port=%jd feature=%jd",
		    index, value, 0, 0);
		if (index < 1 || index > sc->sc_noport) {
			return -1;
		}
		port = OHCI_RH_PORT_STATUS(index);
		switch(value) {
		case UHF_PORT_ENABLE:
			OWRITE4(sc, port, UPS_CURRENT_CONNECT_STATUS);
			break;
		case UHF_PORT_SUSPEND:
			OWRITE4(sc, port, UPS_OVERCURRENT_INDICATOR);
			break;
		case UHF_PORT_POWER:
			/* Yes, writing to the LOW_SPEED bit clears power. */
			OWRITE4(sc, port, UPS_LOW_SPEED);
			break;
		case UHF_C_PORT_CONNECTION:
			OWRITE4(sc, port, UPS_C_CONNECT_STATUS << 16);
			break;
		case UHF_C_PORT_ENABLE:
			OWRITE4(sc, port, UPS_C_PORT_ENABLED << 16);
			break;
		case UHF_C_PORT_SUSPEND:
			OWRITE4(sc, port, UPS_C_SUSPEND << 16);
			break;
		case UHF_C_PORT_OVER_CURRENT:
			OWRITE4(sc, port, UPS_C_OVERCURRENT_INDICATOR << 16);
			break;
		case UHF_C_PORT_RESET:
			OWRITE4(sc, port, UPS_C_PORT_RESET << 16);
			break;
		default:
			return -1;
		}
		switch(value) {
		case UHF_C_PORT_CONNECTION:
		case UHF_C_PORT_ENABLE:
		case UHF_C_PORT_SUSPEND:
		case UHF_C_PORT_OVER_CURRENT:
		case UHF_C_PORT_RESET:
			/* Enable RHSC interrupt if condition is cleared. */
			if ((OREAD4(sc, port) >> 16) == 0)
				ohci_rhsc_enable(sc);
			break;
		default:
			break;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0) {
			return -1;
		}
		usb_hub_descriptor_t hubd;

		totlen = min(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);

		v = OREAD4(sc, OHCI_RH_DESCRIPTOR_A);
		hubd.bNbrPorts = sc->sc_noport;
		USETW(hubd.wHubCharacteristics,
		      (v & OHCI_NPS ? UHD_PWR_NO_SWITCH :
		       v & OHCI_PSM ? UHD_PWR_GANGED : UHD_PWR_INDIVIDUAL)
		      /* XXX overcurrent */
		      );
		hubd.bPwrOn2PwrGood = OHCI_GET_POTPGT(v);
		v = OREAD4(sc, OHCI_RH_DESCRIPTOR_B);
		for (i = 0, l = sc->sc_noport; l > 0; i++, l -= 8, v >>= 8)
			hubd.DeviceRemovable[i++] = (uint8_t)v;
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + i;
		totlen = min(totlen, hubd.bDescLength);
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
		DPRINTFN(8, "get port status i=%jd", index, 0, 0, 0);
		if (index < 1 || index > sc->sc_noport) {
			return -1;
		}
		if (len != 4) {
			return -1;
		}
		v = OREAD4(sc, OHCI_RH_PORT_STATUS(index));
		DPRINTFN(8, "port status=0x%04jx", v, 0, 0, 0);
		USETW(ps.wPortStatus, v);
		USETW(ps.wPortChange, v >> 16);
		totlen = min(len, sizeof(ps));
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
		port = OHCI_RH_PORT_STATUS(index);
		switch(value) {
		case UHF_PORT_ENABLE:
			OWRITE4(sc, port, UPS_PORT_ENABLED);
			break;
		case UHF_PORT_SUSPEND:
			OWRITE4(sc, port, UPS_SUSPEND);
			break;
		case UHF_PORT_RESET:
			DPRINTFN(5, "reset port %jd", index, 0, 0, 0);
			OWRITE4(sc, port, UPS_RESET);
			for (i = 0; i < 5; i++) {
				usb_delay_ms(&sc->sc_bus,
					     USB_PORT_ROOT_RESET_DELAY);
				if (sc->sc_dying) {
					return -1;
				}
				if ((OREAD4(sc, port) & UPS_RESET) == 0)
					break;
			}
			DPRINTFN(8, "port %jd reset, status = 0x%04jx", index,
			    OREAD4(sc, port), 0, 0);
			break;
		case UHF_PORT_POWER:
			DPRINTFN(2, "set port power %jd", index, 0, 0, 0);
			OWRITE4(sc, port, UPS_PORT_POWER);
			break;
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

Static usbd_status
ohci_root_intr_transfer(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return ohci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ohci_root_intr_start(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	const bool polling = sc->sc_bus.ub_usepolling;

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (!polling)
		mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Abort a root interrupt request. */
Static void
ohci_root_intr_abort(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	sc->sc_intrxfer = NULL;

	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

/* Close the root pipe. */
Static void
ohci_root_intr_close(struct usbd_pipe *pipe)
{
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	sc->sc_intrxfer = NULL;
}

/************************/

int
ohci_device_ctrl_init(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	usb_device_request_t *req = &xfer->ux_request;
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_td_t *stat, *setup;
	int isread = req->bmRequestType & UT_READ;
	int len = xfer->ux_bufsize;
	int err = ENOMEM;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	setup = ohci_alloc_std(sc);
	if (setup == NULL) {
		goto bad1;
	}
	stat = ohci_alloc_std(sc);
	if (stat == NULL) {
		goto bad2;
	}

	ox->ox_setup = setup;
	ox->ox_stat = stat;
	ox->ox_nstd = 0;

	/* Set up data transaction */
	if (len != 0) {
		err = ohci_alloc_std_chain(sc, xfer, len, isread);
		if (err) {
			goto bad3;
		}
	}
	return 0;

 bad3:
	ohci_free_std(sc, stat);
 bad2:
	ohci_free_std(sc, setup);
 bad1:
	return err;
}

void
ohci_device_ctrl_fini(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(8, "xfer %#jx nstd %jd", (uintptr_t)xfer, ox->ox_nstd, 0, 0);

	mutex_enter(&sc->sc_lock);
	if (ox->ox_setup != opipe->tail.td) {
		ohci_free_std_locked(sc, ox->ox_setup);
	}
	for (size_t i = 0; i < ox->ox_nstd; i++) {
		ohci_soft_td_t *std = ox->ox_stds[i];
		if (std == NULL)
			break;
		ohci_free_std_locked(sc, std);
	}
	ohci_free_std_locked(sc, ox->ox_stat);
	mutex_exit(&sc->sc_lock);

	if (ox->ox_nstd) {
		const size_t sz = sizeof(ohci_soft_td_t *) * ox->ox_nstd;
		kmem_free(ox->ox_stds, sz);
	}
}

Static usbd_status
ohci_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return ohci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ohci_device_ctrl_start(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	usb_device_request_t *req = &xfer->ux_request;
	struct usbd_device *dev __diagused = opipe->pipe.up_dev;
	ohci_soft_td_t *setup, *stat, *next, *tail;
	ohci_soft_ed_t *sed;
	int isread;
	int len;
	const bool polling = sc->sc_bus.ub_usepolling;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(xfer->ux_rqflags & URQ_REQUEST);

	isread = req->bmRequestType & UT_READ;
	len = UGETW(req->wLength);

	DPRINTF("xfer=%#jx len=%jd, addr=%jd, endpt=%jd", (uintptr_t)xfer, len,
	    dev->ud_addr, opipe->pipe.up_endpoint->ue_edesc->bEndpointAddress);
	DPRINTF("type=0x%02jx, request=0x%02jx, wValue=0x%04jx, wIndex=0x%04jx",
	    req->bmRequestType, req->bRequest, UGETW(req->wValue),
	    UGETW(req->wIndex));

	/* Need to take lock here for pipe->tail.td */
	if (!polling)
		mutex_enter(&sc->sc_lock);

	/*
	 * Use the pipe "tail" TD as our first and loan our first TD to the
	 * next transfer
	 */
	setup = opipe->tail.td;
	opipe->tail.td = ox->ox_setup;
	ox->ox_setup = setup;

	stat = ox->ox_stat;

	/* point at sentinel */
	tail = opipe->tail.td;
	sed = opipe->sed;

	KASSERTMSG(OHCI_ED_GET_FA(O32TOH(sed->ed.ed_flags)) == dev->ud_addr,
	    "address ED %d pipe %d\n",
	    OHCI_ED_GET_FA(O32TOH(sed->ed.ed_flags)), dev->ud_addr);
	KASSERTMSG(OHCI_ED_GET_MAXP(O32TOH(sed->ed.ed_flags)) ==
	    UGETW(opipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize),
	    "MPL ED %d pipe %d\n",
	    OHCI_ED_GET_MAXP(O32TOH(sed->ed.ed_flags)),
	    UGETW(opipe->pipe.up_endpoint->ue_edesc->wMaxPacketSize));

	/* next will point to data if len != 0 */
	next = stat;

	/* Set up data transaction */
	if (len != 0) {
		ohci_soft_td_t *std;
		ohci_soft_td_t *end;

		next = ox->ox_stds[0];
		ohci_reset_std_chain(sc, xfer, len, isread, next, &end);

		end->td.td_nexttd = HTOO32(stat->physaddr);
		end->nexttd = stat;

		usb_syncmem(&end->dma, end->offs, sizeof(end->td),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
		std = ox->ox_stds[0];
		/* Start toggle at 1 and then use the carried toggle. */
		std->td.td_flags &= HTOO32(~OHCI_TD_TOGGLE_MASK);
		std->td.td_flags |= HTOO32(OHCI_TD_TOGGLE_1);
		usb_syncmem(&std->dma,
		    std->offs + offsetof(ohci_td_t, td_flags),
		    sizeof(std->td.td_flags),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}

	DPRINTFN(8, "setup %#jx data %#jx stat %#jx tail %#jx",
	    (uintptr_t)setup,
	    (uintptr_t)(len != 0 ? ox->ox_stds[0] : NULL), (uintptr_t)stat,
	    (uintptr_t)tail);
	KASSERT(opipe->tail.td == tail);

	memcpy(KERNADDR(&opipe->ctrl.reqdma, 0), req, sizeof(*req));
	usb_syncmem(&opipe->ctrl.reqdma, 0, sizeof(*req), BUS_DMASYNC_PREWRITE);

	setup->td.td_flags = HTOO32(OHCI_TD_SETUP | OHCI_TD_NOCC |
				     OHCI_TD_TOGGLE_0 | OHCI_TD_NOINTR);
	setup->td.td_cbp = HTOO32(DMAADDR(&opipe->ctrl.reqdma, 0));
	setup->td.td_nexttd = HTOO32(next->physaddr);
	setup->td.td_be = HTOO32(O32TOH(setup->td.td_cbp) + sizeof(*req) - 1);
	setup->nexttd = next;
	setup->len = 0;
	setup->xfer = xfer;
	setup->flags = 0;
	ohci_hash_add_td(sc, setup);

	xfer->ux_hcpriv = setup;
	usb_syncmem(&setup->dma, setup->offs, sizeof(setup->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	stat->td.td_flags = HTOO32(
		(isread ? OHCI_TD_OUT : OHCI_TD_IN) |
		OHCI_TD_NOCC | OHCI_TD_TOGGLE_1 | OHCI_TD_SET_DI(1));
	stat->td.td_cbp = 0;
	stat->td.td_nexttd = HTOO32(tail->physaddr);
	stat->td.td_be = 0;
	stat->nexttd = tail;
	stat->flags = OHCI_CALL_DONE;
	stat->len = 0;
	stat->xfer = xfer;
	ohci_hash_add_td(sc, stat);

	usb_syncmem(&stat->dma, stat->offs, sizeof(stat->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	memset(&tail->td, 0, sizeof(tail->td));
	tail->nexttd = NULL;
	tail->xfer = NULL;

	usb_syncmem(&tail->dma, tail->offs, sizeof(tail->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef OHCI_DEBUG
	USBHIST_LOGN(ohcidebug, 5, "--- dump start ---", 0, 0, 0, 0);
	if (ohcidebug >= 5) {
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, setup);
	}
	USBHIST_LOGN(ohcidebug, 5, "--- dump end ---", 0, 0, 0, 0);
#endif

	/* Insert ED in schedule */
	sed->ed.ed_tailp = HTOO32(tail->physaddr);
	usb_syncmem(&sed->dma,
	    sed->offs + offsetof(ohci_ed_t, ed_tailp),
	    sizeof(sed->ed.ed_tailp),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_CLF);
	if (xfer->ux_timeout && !polling) {
		callout_reset(&xfer->ux_callout, mstohz(xfer->ux_timeout),
			    ohci_timeout, xfer);
	}

	DPRINTF("done", 0, 0, 0, 0);

	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Abort a device control request. */
Static void
ohci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);
	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/* Close a device control pipe. */
Static void
ohci_device_ctrl_close(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("pipe=%#jx", (uintptr_t)pipe, 0, 0, 0);
	ohci_close_pipe(pipe, sc->sc_ctrl_head);
	ohci_free_std_locked(sc, opipe->tail.td);
}

/************************/

Static void
ohci_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);

	opipe->sed->ed.ed_headp &= HTOO32(~OHCI_TOGGLECARRY);
}

Static void
ohci_noop(struct usbd_pipe *pipe)
{
}

Static int
ohci_device_bulk_init(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	int len = xfer->ux_bufsize;
	int endpt = xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress;;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int err;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));

	DPRINTFN(4, "xfer=%#jx len=%jd isread=%jd flags=%jd", (uintptr_t)xfer,
	    len, isread, xfer->ux_flags);
	DPRINTFN(4, "endpt=%jd", endpt, 0, 0, 0);

	/* Allocate a chain of new TDs (including a new tail). */
	err = ohci_alloc_std_chain(sc, xfer, len, isread);
	if (err)
		return err;

	return 0;
}

Static void
ohci_device_bulk_fini(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(8, "xfer %#jx nstd %jd", (uintptr_t)xfer, ox->ox_nstd, 0, 0);

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < ox->ox_nstd; i++) {
		ohci_soft_td_t *std = ox->ox_stds[i];
		if (std == NULL)
			break;
		if (std != opipe->tail.td)
			ohci_free_std_locked(sc, std);
	}
	mutex_exit(&sc->sc_lock);

	if (ox->ox_nstd) {
		const size_t sz = sizeof(ohci_soft_td_t *) * ox->ox_nstd;
		kmem_free(ox->ox_stds, sz);
	}
}

Static usbd_status
ohci_device_bulk_transfer(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return ohci_device_bulk_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ohci_device_bulk_start(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_td_t *last;
	ohci_soft_td_t *data, *tail, *tdp;
	ohci_soft_ed_t *sed;
	int len, isread, endpt;
	const bool polling = sc->sc_bus.ub_usepolling;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));

	len = xfer->ux_length;
	endpt = xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sed = opipe->sed;

	DPRINTFN(4, "xfer=%#jx len=%jd isread=%jd flags=%jd", (uintptr_t)xfer,
	    len, isread, xfer->ux_flags);
	DPRINTFN(4, "endpt=%jd", endpt, 0, 0, 0);

	if (!polling)
		mutex_enter(&sc->sc_lock);

	/*
	 * Use the pipe "tail" TD as our first and loan our first TD to the
	 * next transfer
	 */
	data = opipe->tail.td;
	opipe->tail.td = ox->ox_stds[0];
	ox->ox_stds[0] = data;
	ohci_reset_std_chain(sc, xfer, len, isread, data, &last);

	/* point at sentinel */
	tail = opipe->tail.td;
	memset(&tail->td, 0, sizeof(tail->td));
	tail->nexttd = NULL;
	tail->xfer = NULL;
	usb_syncmem(&tail->dma, tail->offs, sizeof(tail->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	xfer->ux_hcpriv = data;

	DPRINTFN(8, "xfer %#jx data %#jx tail %#jx", (uintptr_t)xfer,
	    (uintptr_t)ox->ox_stds[0], (uintptr_t)tail, 0);
	KASSERT(opipe->tail.td == tail);

	/* We want interrupt at the end of the transfer. */
	last->td.td_flags &= HTOO32(~OHCI_TD_INTR_MASK);
	last->td.td_flags |= HTOO32(OHCI_TD_SET_DI(1));
	last->td.td_nexttd = HTOO32(tail->physaddr);
	last->nexttd = tail;
	last->flags |= OHCI_CALL_DONE;
	usb_syncmem(&last->dma, last->offs, sizeof(last->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	DPRINTFN(4, "ed_flags=0x%08jx td_flags=0x%08jx "
		    "td_cbp=0x%08jx td_be=0x%08jx",
		    (int)O32TOH(sed->ed.ed_flags),
		    (int)O32TOH(data->td.td_flags),
		    (int)O32TOH(data->td.td_cbp),
		    (int)O32TOH(data->td.td_be));

#ifdef OHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	if (ohcidebug >= 5) {
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, data);
	}
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	/* Insert ED in schedule */
	for (tdp = data; tdp != tail; tdp = tdp->nexttd) {
		KASSERT(tdp->xfer == xfer);
	}
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_tailp = HTOO32(tail->physaddr);
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_BLF);
	if (xfer->ux_timeout && !sc->sc_bus.ub_usepolling) {
		callout_reset(&xfer->ux_callout, mstohz(xfer->ux_timeout),
			    ohci_timeout, xfer);
	}

	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

Static void
ohci_device_bulk_abort(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTF("xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);
	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/*
 * Close a device bulk pipe.
 */
Static void
ohci_device_bulk_close(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTF("pipe=%#jx", (uintptr_t)pipe, 0, 0, 0);
	ohci_close_pipe(pipe, sc->sc_bulk_head);
	ohci_free_std_locked(sc, opipe->tail.td);
}

/************************/

Static int
ohci_device_intr_init(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	int len = xfer->ux_bufsize;
	int endpt = xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress;;
	int isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	int err;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));
	KASSERT(len != 0);

	DPRINTFN(4, "xfer=%#jx len=%jd isread=%jd flags=%jd", (uintptr_t)xfer,
	    len, isread, xfer->ux_flags);
	DPRINTFN(4, "endpt=%jd", endpt, 0, 0, 0);

	ox->ox_nstd = 0;

	err = ohci_alloc_std_chain(sc, xfer, len, isread);
	if (err) {
		return err;
	}

	return 0;
}

Static void
ohci_device_intr_fini(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(8, "xfer %#jx nstd %jd", (uintptr_t)xfer, ox->ox_nstd, 0, 0);

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < ox->ox_nstd; i++) {
		ohci_soft_td_t *std = ox->ox_stds[i];
		if (std != NULL)
			break;
		if (std != opipe->tail.td)
			ohci_free_std_locked(sc, std);
	}
	mutex_exit(&sc->sc_lock);

	if (ox->ox_nstd) {
		const size_t sz = sizeof(ohci_soft_td_t *) * ox->ox_nstd;
		kmem_free(ox->ox_stds, sz);
	}
}

Static usbd_status
ohci_device_intr_transfer(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	usbd_status err;

	/* Insert last in queue. */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (err)
		return err;

	/* Pipe isn't running, start first */
	return ohci_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

Static usbd_status
ohci_device_intr_start(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_ed_t *sed = opipe->sed;
	ohci_soft_td_t *data, *last, *tail;
	int len, isread, endpt;
	const bool polling = sc->sc_bus.ub_usepolling;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	if (sc->sc_dying)
		return USBD_IOERROR;

	DPRINTFN(3, "xfer=%#jx len=%jd flags=%jd priv=%#jx", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, (uintptr_t)xfer->ux_priv);

	KASSERT(!(xfer->ux_rqflags & URQ_REQUEST));

	len = xfer->ux_length;
	endpt = xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;

	if (!polling)
		mutex_enter(&sc->sc_lock);

	/*
	 * Use the pipe "tail" TD as our first and loan our first TD to the
	 * next transfer.
	 */
	data = opipe->tail.td;
	opipe->tail.td = ox->ox_stds[0];
	ox->ox_stds[0] = data;
	ohci_reset_std_chain(sc, xfer, len, isread, data, &last);

	/* point at sentinel */
	tail = opipe->tail.td;
	memset(&tail->td, 0, sizeof(tail->td));
	tail->nexttd = NULL;
	tail->xfer = NULL;
	usb_syncmem(&tail->dma, tail->offs, sizeof(tail->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	xfer->ux_hcpriv = data;

	DPRINTFN(8, "data %#jx tail %#jx", (uintptr_t)ox->ox_stds[0],
	    (uintptr_t)tail, 0, 0);
	KASSERT(opipe->tail.td == tail);

	/* We want interrupt at the end of the transfer. */
	last->td.td_flags &= HTOO32(~OHCI_TD_INTR_MASK);
	last->td.td_flags |= HTOO32(OHCI_TD_SET_DI(1));

	last->td.td_nexttd = HTOO32(tail->physaddr);
	last->nexttd = tail;
	last->flags |= OHCI_CALL_DONE;
	usb_syncmem(&last->dma, last->offs, sizeof(last->td),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

#ifdef OHCI_DEBUG
	DPRINTFN(5, "--- dump start ---", 0, 0, 0, 0);
	if (ohcidebug >= 5) {
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, data);
	}
	DPRINTFN(5, "--- dump end ---", 0, 0, 0, 0);
#endif

	/* Insert ED in schedule */
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_tailp = HTOO32(tail->physaddr);
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	xfer->ux_status = USBD_IN_PROGRESS;
	if (!polling)
		mutex_exit(&sc->sc_lock);

	return USBD_IN_PROGRESS;
}

/* Abort a device interrupt request. */
Static void
ohci_device_intr_abort(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc __diagused = OHCI_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/* Close a device interrupt pipe. */
Static void
ohci_device_intr_close(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);
	int nslots = opipe->intr.nslots;
	int pos = opipe->intr.pos;
	int j;
	ohci_soft_ed_t *p, *sed = opipe->sed;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	DPRINTFN(1, "pipe=%#jx nslots=%jd pos=%jd", (uintptr_t)pipe, nslots,
	    pos, 0);
	usb_syncmem(&sed->dma, sed->offs,
	    sizeof(sed->ed), BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
	    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK))
		usb_delay_ms_locked(&sc->sc_bus, 2, &sc->sc_lock);

	for (p = sc->sc_eds[pos]; p && p->next != sed; p = p->next)
		continue;
	KASSERT(p);
	p->next = sed->next;
	p->ed.ed_nexted = sed->ed.ed_nexted;
	usb_syncmem(&p->dma, p->offs + offsetof(ohci_ed_t, ed_nexted),
	    sizeof(p->ed.ed_nexted),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	for (j = 0; j < nslots; j++)
		--sc->sc_bws[(pos * nslots + j) % OHCI_NO_INTRS];

	ohci_free_std_locked(sc, opipe->tail.td);
	ohci_free_sed_locked(sc, opipe->sed);
}

Static usbd_status
ohci_device_setintr(ohci_softc_t *sc, struct ohci_pipe *opipe, int ival)
{
	int i, j, best;
	u_int npoll, slow, shigh, nslots;
	u_int bestbw, bw;
	ohci_soft_ed_t *hsed, *sed = opipe->sed;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTFN(2, "pipe=%#jx", (uintptr_t)opipe, 0, 0, 0);
	if (ival == 0) {
		printf("ohci_setintr: 0 interval\n");
		return USBD_INVAL;
	}

	npoll = OHCI_NO_INTRS;
	while (npoll > ival)
		npoll /= 2;
	DPRINTFN(2, "ival=%jd npoll=%jd", ival, npoll, 0, 0);

	/*
	 * We now know which level in the tree the ED must go into.
	 * Figure out which slot has most bandwidth left over.
	 * Slots to examine:
	 * npoll
	 * 1	0
	 * 2	1 2
	 * 4	3 4 5 6
	 * 8	7 8 9 10 11 12 13 14
	 * N    (N-1) .. (N-1+N-1)
	 */
	slow = npoll-1;
	shigh = slow + npoll;
	nslots = OHCI_NO_INTRS / npoll;
	for (best = i = slow, bestbw = ~0; i < shigh; i++) {
		bw = 0;
		for (j = 0; j < nslots; j++)
			bw += sc->sc_bws[(i * nslots + j) % OHCI_NO_INTRS];
		if (bw < bestbw) {
			best = i;
			bestbw = bw;
		}
	}
	DPRINTFN(2, "best=%jd(%jd..%jd) bestbw=%jd", best, slow, shigh, bestbw);

	mutex_enter(&sc->sc_lock);
	hsed = sc->sc_eds[best];
	sed->next = hsed->next;
	usb_syncmem(&hsed->dma, hsed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(hsed->ed.ed_flags),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_nexted = hsed->ed.ed_nexted;
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	hsed->next = sed;
	hsed->ed.ed_nexted = HTOO32(sed->physaddr);
	usb_syncmem(&hsed->dma, hsed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(hsed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	mutex_exit(&sc->sc_lock);

	for (j = 0; j < nslots; j++)
		++sc->sc_bws[(best * nslots + j) % OHCI_NO_INTRS];
	opipe->intr.nslots = nslots;
	opipe->intr.pos = best;

	DPRINTFN(5, "returns %#jx", (uintptr_t)opipe, 0, 0, 0);
	return USBD_NORMAL_COMPLETION;
}

/***********************/

Static int
ohci_device_isoc_init(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_itd_t *sitd;
	size_t i;
	int err;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTFN(1, "xfer %#jx len %jd flags %jd", (uintptr_t)xfer,
	    xfer->ux_length, xfer->ux_flags, 0);

	const size_t nfsitd =
	    (xfer->ux_nframes + OHCI_ITD_NOFFSET - 1) / OHCI_ITD_NOFFSET;
	const size_t nbsitd = xfer->ux_bufsize / OHCI_PAGE_SIZE;
	const size_t nsitd = MAX(nfsitd, nbsitd) + 1;

	ox->ox_sitds = kmem_zalloc(sizeof(ohci_soft_itd_t *) * nsitd,
	    KM_SLEEP);
	ox->ox_nsitd = nsitd;

	for (i = 0; i < nsitd; i++) {
		/* Allocate next ITD */
		sitd = ohci_alloc_sitd(sc);
		if (sitd == NULL) {
			err = ENOMEM;
			goto fail;
		}
		ox->ox_sitds[i] = sitd;
		sitd->xfer = xfer;
		sitd->flags = 0;
	}

	return 0;
fail:
	for (; i > 0;) {
		ohci_free_sitd(sc, ox->ox_sitds[--i]);
	}
	return err;
}

Static void
ohci_device_isoc_fini(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < ox->ox_nsitd; i++) {
		if (ox->ox_sitds[i] != opipe->tail.itd) {
			ohci_free_sitd_locked(sc, ox->ox_sitds[i]);
		}
	}
	mutex_exit(&sc->sc_lock);

	if (ox->ox_nsitd) {
		const size_t sz = sizeof(ohci_soft_itd_t *) * ox->ox_nsitd;
		kmem_free(ox->ox_sitds, sz);
	}
}


usbd_status
ohci_device_isoc_transfer(struct usbd_xfer *xfer)
{
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	usbd_status __diagused err;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();

	DPRINTFN(5, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	/* Put it on our queue, */
	mutex_enter(&sc->sc_lock);
	err = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);

	KASSERT(err == USBD_NORMAL_COMPLETION);

	/* insert into schedule, */
	ohci_device_isoc_enter(xfer);

	/* and start if the pipe wasn't running */
	return USBD_IN_PROGRESS;
}

void
ohci_device_isoc_enter(struct usbd_xfer *xfer)
{
	struct ohci_xfer *ox = OHCI_XFER2OXFER(xfer);
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_ed_t *sed = opipe->sed;
	ohci_soft_itd_t *sitd, *nsitd, *tail;
	ohci_physaddr_t buf, offs, noffs, bp0;
	int i, ncur, nframes;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(5, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	mutex_enter(&sc->sc_lock);

	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return;
	}

	struct isoc *isoc = &opipe->isoc;

	DPRINTFN(1, "used=%jd next=%jd xfer=%#jx nframes=%jd",
	     isoc->inuse, isoc->next, (uintptr_t)xfer, xfer->ux_nframes);

	if (isoc->next == -1) {
		/* Not in use yet, schedule it a few frames ahead. */
		isoc->next = O32TOH(sc->sc_hcca->hcca_frame_number) + 5;
		DPRINTFN(2,"start next=%jd", isoc->next, 0, 0, 0);
	}

	sitd = opipe->tail.itd;
	opipe->tail.itd = ox->ox_sitds[0];
	ox->ox_sitds[0] = sitd;

	buf = DMAADDR(&xfer->ux_dmabuf, 0);
	bp0 = OHCI_PAGE(buf);
	offs = OHCI_PAGE_OFFSET(buf);
	nframes = xfer->ux_nframes;
	xfer->ux_hcpriv = sitd;
	size_t j = 1;
	for (i = ncur = 0; i < nframes; i++, ncur++) {
		noffs = offs + xfer->ux_frlengths[i];
		if (ncur == OHCI_ITD_NOFFSET ||	/* all offsets used */
		    OHCI_PAGE(buf + noffs) > bp0 + OHCI_PAGE_SIZE) { /* too many page crossings */

			/* Allocate next ITD */
			nsitd = ox->ox_sitds[j++];
			KASSERT(nsitd != NULL);
			KASSERT(j < ox->ox_nsitd);

			/* Fill current ITD */
			sitd->itd.itd_flags = HTOO32(
				OHCI_ITD_NOCC |
				OHCI_ITD_SET_SF(isoc->next) |
				OHCI_ITD_SET_DI(6) | /* delay intr a little */
				OHCI_ITD_SET_FC(ncur));
			sitd->itd.itd_bp0 = HTOO32(bp0);
			sitd->itd.itd_nextitd = HTOO32(nsitd->physaddr);
			sitd->itd.itd_be = HTOO32(bp0 + offs - 1);
			sitd->nextitd = nsitd;
			sitd->xfer = xfer;
			sitd->flags = 0;
#ifdef DIAGNOSTIC
			sitd->isdone = false;
#endif
			ohci_hash_add_itd(sc, sitd);
			usb_syncmem(&sitd->dma, sitd->offs, sizeof(sitd->itd),
			    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

			sitd = nsitd;
			isoc->next = isoc->next + ncur;
			bp0 = OHCI_PAGE(buf + offs);
			ncur = 0;
		}
		sitd->itd.itd_offset[ncur] = HTOO16(OHCI_ITD_MK_OFFS(offs));
		/* XXX Sync */
		offs = noffs;
	}
	KASSERT(j <= ox->ox_nsitd);

	/* point at sentinel */
	tail = opipe->tail.itd;
	memset(&tail->itd, 0, sizeof(tail->itd));
	tail->nextitd = NULL;
	tail->xfer = NULL;
	usb_syncmem(&tail->dma, tail->offs, sizeof(tail->itd),
	    BUS_DMASYNC_PREWRITE);

	/* Fixup last used ITD */
	sitd->itd.itd_flags = HTOO32(
		OHCI_ITD_NOCC |
		OHCI_ITD_SET_SF(isoc->next) |
		OHCI_ITD_SET_DI(0) |
		OHCI_ITD_SET_FC(ncur));
	sitd->itd.itd_bp0 = HTOO32(bp0);
	sitd->itd.itd_nextitd = HTOO32(tail->physaddr);
	sitd->itd.itd_be = HTOO32(bp0 + offs - 1);
	sitd->nextitd = tail;
	sitd->xfer = xfer;
	sitd->flags = OHCI_CALL_DONE;
#ifdef DIAGNOSTIC
	sitd->isdone = false;
#endif
	ohci_hash_add_itd(sc, sitd);
	usb_syncmem(&sitd->dma, sitd->offs, sizeof(sitd->itd),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	isoc->next = isoc->next + ncur;
	isoc->inuse += nframes;

	/* XXX pretend we did it all */
	xfer->ux_actlen = offs;
	xfer->ux_status = USBD_IN_PROGRESS;

#ifdef OHCI_DEBUG
	if (ohcidebug >= 5) {
		DPRINTF("frame=%jd", O32TOH(sc->sc_hcca->hcca_frame_number),
		    0, 0, 0);
		ohci_dump_itds(sc, xfer->ux_hcpriv);
		ohci_dump_ed(sc, sed);
	}
#endif

	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_tailp = HTOO32(tail->physaddr);
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	mutex_exit(&sc->sc_lock);
}

void
ohci_device_isoc_abort(struct usbd_xfer *xfer)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(xfer->ux_pipe);
	ohci_softc_t *sc = OHCI_XFER2SC(xfer);
	ohci_soft_ed_t *sed;
	ohci_soft_itd_t *sitd;

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(1, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Transfer is already done. */
	if (xfer->ux_status != USBD_NOT_STARTED &&
	    xfer->ux_status != USBD_IN_PROGRESS) {
		printf("ohci_device_isoc_abort: early return\n");
		goto done;
	}

	/* Give xfer the requested abort code. */
	xfer->ux_status = USBD_CANCELLED;

	sed = opipe->sed;
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP); /* force hardware skip */
	usb_syncmem(&sed->dma, sed->offs + offsetof(ohci_ed_t, ed_flags),
	    sizeof(sed->ed.ed_flags),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	sitd = xfer->ux_hcpriv;
	KASSERT(sitd);

	usb_delay_ms_locked(&sc->sc_bus, OHCI_ITD_NOFFSET, &sc->sc_lock);

	for (; sitd->xfer == xfer; sitd = sitd->nextitd) {
		ohci_hash_rem_itd(sc, sitd);
#ifdef DIAGNOSTIC
		DPRINTFN(1, "abort sets done sitd=%#jx", (uintptr_t)sitd,
		    0, 0, 0);
		sitd->isdone = true;
#endif
	}

	/* Run callback. */
	usb_transfer_complete(xfer);

	sed->ed.ed_headp = HTOO32(sitd->physaddr); /* unlink TDs */
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP); /* remove hardware skip */
	usb_syncmem(&sed->dma, sed->offs, sizeof(sed->ed),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

 done:
	KASSERT(mutex_owned(&sc->sc_lock));
}

void
ohci_device_isoc_done(struct usbd_xfer *xfer)
{
	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTFN(1, "xfer=%#jx", (uintptr_t)xfer, 0, 0, 0);
}

usbd_status
ohci_setup_isoc(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);
	struct isoc *isoc = &opipe->isoc;

	isoc->next = -1;
	isoc->inuse = 0;

	mutex_enter(&sc->sc_lock);
	ohci_add_ed(sc, opipe->sed, sc->sc_isoc_head);
	mutex_exit(&sc->sc_lock);

	return USBD_NORMAL_COMPLETION;
}

void
ohci_device_isoc_close(struct usbd_pipe *pipe)
{
	struct ohci_pipe *opipe = OHCI_PIPE2OPIPE(pipe);
	ohci_softc_t *sc = OHCI_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));

	OHCIHIST_FUNC(); OHCIHIST_CALLED();
	DPRINTF("pipe=%#jx", (uintptr_t)pipe, 0, 0, 0);
	ohci_close_pipe(pipe, sc->sc_isoc_head);
#ifdef DIAGNOSTIC
	opipe->tail.itd->isdone = true;
#endif
	ohci_free_sitd_locked(sc, opipe->tail.itd);
}
