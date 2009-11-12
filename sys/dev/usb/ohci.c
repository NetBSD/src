/*	$NetBSD: ohci.c,v 1.182.12.5 2009/11/12 08:50:05 uebayasi Exp $	*/

/*-
 * Copyright (c) 1998, 2004, 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ohci.c,v 1.182.12.5 2009/11/12 08:50:05 uebayasi Exp $");
/* __FBSDID("$FreeBSD: src/sys/dev/usb/ohci.c,v 1.167 2006/10/19 01:15:58 iedowse Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/device.h>
#include <sys/select.h>
#include <uvm/uvm_extern.h>
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#include <sys/module.h>
#include <sys/bus.h>
#if defined(DIAGNOSTIC) && defined(__i386__) && defined(__FreeBSD__)
#include <machine/cpu.h>
#endif
#endif
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#if defined(__FreeBSD__)
#include <sys/sysctl.h>

#define delay(d)                DELAY(d)
#endif

#if defined(__OpenBSD__)
struct cfdriver ohci_cd = {
	NULL, "ohci", DV_DULL
};
#endif

#ifdef OHCI_DEBUG
#define DPRINTF(x)	if (ohcidebug) logprintf x
#define DPRINTFN(n,x)	if (ohcidebug>(n)) logprintf x
int ohcidebug = 0;
#ifdef __FreeBSD__
SYSCTL_NODE(_hw_usb, OID_AUTO, ohci, CTLFLAG_RW, 0, "USB ohci");
SYSCTL_INT(_hw_usb_ohci, OID_AUTO, debug, CTLFLAG_RW,
	   &ohcidebug, 0, "ohci debug level");
#endif
#ifndef __NetBSD__
#define bitmask_snprintf(q,f,b,l) snprintf((b), (l), "%b", (q), (f))
#endif
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * The OHCI controller is either little endian or host endian,
 * so on big endian machines with little endian controller
 * the data strored in memory needs to be swapped.
 */
#define	O16TOH(val)	(sc->sc_endian ? (val) : le16toh(val))
#define	O32TOH(val)	(sc->sc_endian ? (val) : le32toh(val))
#define	HTOO16(val)	(sc->sc_endian ? (val) : htole16(val))
#define	HTOO32(val)	(sc->sc_endian ? (val) : htole32(val))

struct ohci_pipe;

Static void		ohci_free_desc_chunks(ohci_softc_t *,
			    struct ohci_mdescs *);
Static ohci_soft_ed_t  *ohci_alloc_sed(ohci_softc_t *);
Static void		ohci_free_sed(ohci_softc_t *, ohci_soft_ed_t *);

Static usbd_status	ohci_grow_std(ohci_softc_t *);
Static ohci_soft_td_t  *ohci_alloc_std(ohci_softc_t *);
Static ohci_soft_td_t  *ohci_alloc_std_norsv(ohci_softc_t *);
Static void		ohci_free_std(ohci_softc_t *, ohci_soft_td_t *);
Static void		ohci_free_std_norsv(ohci_softc_t *, ohci_soft_td_t *);

Static usbd_status	ohci_grow_sitd(ohci_softc_t *);
Static ohci_soft_itd_t *ohci_alloc_sitd(ohci_softc_t *);
Static ohci_soft_itd_t *ohci_alloc_sitd_norsv(ohci_softc_t *);
Static void		ohci_free_sitd(ohci_softc_t *,ohci_soft_itd_t *);
Static void		ohci_free_sitd_norsv(ohci_softc_t *,ohci_soft_itd_t *);

Static void		ohci_free_std_chain(ohci_softc_t *, ohci_soft_td_t *,
					    ohci_soft_td_t *);
Static usbd_status	ohci_alloc_std_chain(struct ohci_pipe *,
			    ohci_softc_t *, int, int, usbd_xfer_handle,
			    ohci_soft_td_t *, ohci_soft_td_t **);

Static usbd_status	ohci_open(usbd_pipe_handle);
Static void		ohci_poll(struct usbd_bus *);
Static void		ohci_softintr(void *);
Static void		ohci_waitintr(ohci_softc_t *, usbd_xfer_handle);
Static void		ohci_rhsc(ohci_softc_t *, usbd_xfer_handle);

Static usbd_status	ohci_device_request(usbd_xfer_handle xfer);
Static void		ohci_add_ed(ohci_softc_t *, ohci_soft_ed_t *,
			    ohci_soft_ed_t *);
Static void		ohci_rem_ed(ohci_softc_t *, ohci_soft_ed_t *,
			    ohci_soft_ed_t *);

Static ohci_soft_td_t  *ohci_find_td(ohci_softc_t *, ohci_physaddr_t);
Static ohci_soft_itd_t *ohci_find_itd(ohci_softc_t *, ohci_physaddr_t);

Static usbd_status	ohci_setup_isoc(usbd_pipe_handle pipe);
Static void		ohci_device_isoc_enter(usbd_xfer_handle);

Static usbd_status	ohci_prealloc(struct ohci_softc *,
			    struct ohci_xfer *, size_t, int);
Static usbd_status	ohci_allocm(struct usbd_bus *, usbd_xfer_handle,
			    void *buf, size_t);
Static void		ohci_freem(struct usbd_bus *, usbd_xfer_handle,
			    enum usbd_waitflg);

Static usbd_status	ohci_map_alloc(usbd_xfer_handle);
Static void		ohci_map_free(usbd_xfer_handle);
Static void		ohci_mapm(usbd_xfer_handle, void *, size_t);
Static usbd_status	ohci_mapm_mbuf(usbd_xfer_handle, struct mbuf *);
Static void		ohci_unmapm(usbd_xfer_handle);

Static usbd_xfer_handle	ohci_allocx(struct usbd_bus *, usbd_pipe_handle,
			    enum usbd_waitflg);
Static void		ohci_freex(struct usbd_bus *, usbd_xfer_handle);

Static void		ohci_transfer_complete(usbd_xfer_handle, int);

Static usbd_status	ohci_root_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	ohci_root_ctrl_start(usbd_xfer_handle);
Static void		ohci_root_ctrl_abort(usbd_xfer_handle);
Static void		ohci_root_ctrl_close(usbd_pipe_handle);
Static void		ohci_root_ctrl_done(usbd_xfer_handle);

Static usbd_status	ohci_root_intr_transfer(usbd_xfer_handle);
Static usbd_status	ohci_root_intr_start(usbd_xfer_handle);
Static void		ohci_root_intr_abort(usbd_xfer_handle);
Static void		ohci_root_intr_close(usbd_pipe_handle);
Static void		ohci_root_intr_done(usbd_xfer_handle);

Static usbd_status	ohci_device_ctrl_transfer(usbd_xfer_handle);
Static usbd_status	ohci_device_ctrl_start(usbd_xfer_handle);
Static void		ohci_device_ctrl_abort(usbd_xfer_handle);
Static void		ohci_device_ctrl_close(usbd_pipe_handle);
Static void		ohci_device_ctrl_done(usbd_xfer_handle);

Static usbd_status	ohci_device_bulk_transfer(usbd_xfer_handle);
Static usbd_status	ohci_device_bulk_start(usbd_xfer_handle);
Static void		ohci_device_bulk_abort(usbd_xfer_handle);
Static void		ohci_device_bulk_close(usbd_pipe_handle);
Static void		ohci_device_bulk_done(usbd_xfer_handle);

Static usbd_status	ohci_device_intr_transfer(usbd_xfer_handle);
Static usbd_status	ohci_device_intr_start(usbd_xfer_handle);
Static void		ohci_device_intr_abort(usbd_xfer_handle);
Static void		ohci_device_intr_close(usbd_pipe_handle);
Static void		ohci_device_intr_done(usbd_xfer_handle);

Static usbd_status	ohci_device_isoc_transfer(usbd_xfer_handle);
Static usbd_status	ohci_device_isoc_start(usbd_xfer_handle);
Static void		ohci_device_isoc_abort(usbd_xfer_handle);
Static void		ohci_device_isoc_close(usbd_pipe_handle);
Static void		ohci_device_isoc_done(usbd_xfer_handle);

Static usbd_status	ohci_device_setintr(ohci_softc_t *sc,
			    struct ohci_pipe *pipe, int ival);
Static usbd_status	ohci_device_intr_insert(ohci_softc_t *sc,
			    usbd_xfer_handle xfer);

Static int		ohci_str(usb_string_descriptor_t *, int, const char *);

Static void		ohci_timeout(void *);
Static void		ohci_timeout_task(void *);
Static void		ohci_rhsc_enable(void *);

Static void		ohci_close_pipe(usbd_pipe_handle, ohci_soft_ed_t *);
Static void		ohci_abort_xfer(usbd_xfer_handle, usbd_status);

Static void		ohci_device_clear_toggle(usbd_pipe_handle pipe);
Static void		ohci_noop(usbd_pipe_handle pipe);

Static usbd_status ohci_controller_init(ohci_softc_t *sc);

Static void		ohci_aux_mem_init(struct ohci_aux_mem *);
Static usbd_status	ohci_aux_mem_alloc(ohci_softc_t *,
			    struct ohci_aux_mem *,
			    int /*naux*/, int /*maxp*/);
Static void		ohci_aux_mem_free(ohci_softc_t *,
			    struct ohci_aux_mem *);
Static bus_addr_t	ohci_aux_dma_alloc(struct ohci_aux_mem *,
			    const union usb_bufptr *, int,
			    struct usb_aux_desc *);
Static void		ohci_aux_dma_complete(struct usb_aux_desc *,
			    struct ohci_aux_mem *,
			    int /*is_mbuf*/, int /*isread*/);
Static void		ohci_aux_dma_sync(ohci_softc_t *,
			    struct ohci_aux_mem *, int /*op*/);

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
static __inline uint8_t
OREAD1(ohci_softc_t *sc, bus_size_t r)
{

	OBARR(sc);
	return bus_space_read_1(sc->iot, sc->ioh, r);
}

static __inline uint16_t
OREAD2(ohci_softc_t *sc, bus_size_t r)
{

	OBARR(sc);
	return bus_space_read_2(sc->iot, sc->ioh, r);
}

static __inline uint32_t
OREAD4(ohci_softc_t *sc, bus_size_t r)
{

	OBARR(sc);
	return bus_space_read_4(sc->iot, sc->ioh, r);
}

/* Reverse the bits in a value 0 .. 31 */
Static u_int8_t revbits[OHCI_NO_INTRS] =
  { 0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
    0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
    0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f };

struct ohci_pipe {
	struct usbd_pipe pipe;
	ohci_soft_ed_t *sed;
	u_int32_t aborting;
	union {
		ohci_soft_td_t *td;
		ohci_soft_itd_t *itd;
	} tail;
	/* Info needed for different pipe kinds. */
	union {
		/* Control pipe */
		struct {
			usb_dma_t reqdma;
			u_int length;
			ohci_soft_td_t *setup, *data, *stat;
		} ctl;
		/* Interrupt pipe */
		struct {
			int nslots;
			int pos;
		} intr;
		/* Bulk pipe */
		struct {
			u_int length;
			int isread;
		} bulk;
		/* Iso pipe */
		struct iso {
			int next, inuse;
		} iso;
	} u;
};

#define OHCI_INTR_ENDPT 1

Static const struct usbd_bus_methods ohci_bus_methods = {
	ohci_open,
	ohci_softintr,
	ohci_poll,
	ohci_allocm,
	ohci_freem,
	ohci_map_alloc,
	ohci_map_free,
	ohci_mapm,
	ohci_mapm_mbuf,
	ohci_unmapm,
	ohci_allocx,
	ohci_freex,
};

Static const struct usbd_pipe_methods ohci_root_ctrl_methods = {
	ohci_root_ctrl_transfer,
	ohci_root_ctrl_start,
	ohci_root_ctrl_abort,
	ohci_root_ctrl_close,
	ohci_noop,
	ohci_root_ctrl_done,
};

Static const struct usbd_pipe_methods ohci_root_intr_methods = {
	ohci_root_intr_transfer,
	ohci_root_intr_start,
	ohci_root_intr_abort,
	ohci_root_intr_close,
	ohci_noop,
	ohci_root_intr_done,
};

Static const struct usbd_pipe_methods ohci_device_ctrl_methods = {
	ohci_device_ctrl_transfer,
	ohci_device_ctrl_start,
	ohci_device_ctrl_abort,
	ohci_device_ctrl_close,
	ohci_noop,
	ohci_device_ctrl_done,
};

Static const struct usbd_pipe_methods ohci_device_intr_methods = {
	ohci_device_intr_transfer,
	ohci_device_intr_start,
	ohci_device_intr_abort,
	ohci_device_intr_close,
	ohci_device_clear_toggle,
	ohci_device_intr_done,
};

Static const struct usbd_pipe_methods ohci_device_bulk_methods = {
	ohci_device_bulk_transfer,
	ohci_device_bulk_start,
	ohci_device_bulk_abort,
	ohci_device_bulk_close,
	ohci_device_clear_toggle,
	ohci_device_bulk_done,
};

Static const struct usbd_pipe_methods ohci_device_isoc_methods = {
	ohci_device_isoc_transfer,
	ohci_device_isoc_start,
	ohci_device_isoc_abort,
	ohci_device_isoc_close,
	ohci_noop,
	ohci_device_isoc_done,
};

#if defined(__NetBSD__) || defined(__OpenBSD__)
int
ohci_activate(device_t self, enum devact act)
{
	struct ohci_softc *sc = (struct ohci_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);

	case DVACT_DEACTIVATE:
		if (sc->sc_child != NULL)
			rv = config_deactivate(sc->sc_child);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}
#endif

int
ohci_detach(struct ohci_softc *sc, int flags)
{
	int rv = 0;
	usbd_xfer_handle xfer;

	sc->sc_dying = 1;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);

	if (rv != 0)
		return (rv);
#endif

	if (sc->sc_bus.methods == NULL)
		return (rv);	/* attach has been aborted */

	usb_uncallout(sc->sc_tmo_rhsc, ohci_rhsc_enable, sc);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	powerhook_disestablish(sc->sc_powerhook);
	shutdownhook_disestablish(sc->sc_shutdownhook);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
	/* Don't touch hardware if it has already been gone. */
	if ((flags & DETACH_FORCE) == 0)
#endif
	{
		OWRITE4(sc, OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
		OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET);
	}

	usb_delay_ms(&sc->sc_bus, 300); /* XXX let stray task complete */

#if 0	/* freed by ohci_free_desc_chunks(sc, &sc->sc_sed_chunks) below */
	for (i = 0; i < OHCI_NO_EDS; i++)
		ohci_free_sed(sc, sc->sc_eds[i]);
	ohci_free_sed(sc, sc->sc_isoc_head);
	ohci_free_sed(sc, sc->sc_bulk_head);
	ohci_free_sed(sc, sc->sc_ctrl_head);
#endif
	usb_freemem(&sc->sc_dmatag, &sc->sc_hccadma);

	while ((xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
		usb_clean_buffer_dma(&sc->sc_dmatag, &OXFER(xfer)->dmabuf);
		free(xfer, M_USB);
	}
	ohci_free_desc_chunks(sc, &sc->sc_sed_chunks);
	ohci_free_desc_chunks(sc, &sc->sc_std_chunks);
	ohci_free_desc_chunks(sc, &sc->sc_sitd_chunks);
	usb_dma_tag_finish(&sc->sc_dmatag);

	return (rv);
}

Static void
ohci_free_desc_chunks(ohci_softc_t *sc, struct ohci_mdescs *c)
{
	struct ohci_mem_desc *om;

	while ((om = SIMPLEQ_FIRST(c)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(c, om_next);
		usb_freemem(&sc->sc_dmatag, &om->om_dma);
	}
}

ohci_soft_ed_t *
ohci_alloc_sed(ohci_softc_t *sc)
{
	ohci_soft_ed_t *sed;
	usbd_status err;
	int i, offs;
	usb_dma_t dma;
	struct ohci_mem_desc *om;

	if (sc->sc_freeeds == NULL) {
		DPRINTFN(2, ("ohci_alloc_sed: allocating chunk\n"));
		err = usb_allocmem(&sc->sc_dmatag,
		    OHCI_SED_SIZE*OHCI_SED_CHUNK + sizeof(struct ohci_mem_desc),
		    OHCI_ED_ALIGN, &dma);
		if (err)
			return (NULL);
		om = KERNADDR(&dma, OHCI_SED_SIZE * OHCI_SED_CHUNK);
		om->om_top = KERNADDR(&dma, 0);
		om->om_topdma = DMAADDR(&dma, 0);
		om->om_dma = dma;
		SIMPLEQ_INSERT_HEAD(&sc->sc_sed_chunks, om, om_next);
		for(i = 0; i < OHCI_SED_CHUNK; i++) {
			offs = i * OHCI_SED_SIZE;
			sed = KERNADDR(&dma, offs);
			sed->oe_mdesc = om;
			sed->next = sc->sc_freeeds;
			sc->sc_freeeds = sed;
		}
	}
	sed = sc->sc_freeeds;
	sc->sc_freeeds = sed->next;
	memset(&sed->ed, 0, sizeof(ohci_ed_t));
	sed->next = 0;
	return (sed);
}

void
ohci_free_sed(ohci_softc_t *sc, ohci_soft_ed_t *sed)
{
	sed->next = sc->sc_freeeds;
	sc->sc_freeeds = sed;
}

Static usbd_status
ohci_grow_std(ohci_softc_t *sc)
{
	usb_dma_t dma;
	struct ohci_mem_desc *om;
	ohci_soft_td_t *std;
	usbd_status err;
	int i, s, offs;

	DPRINTFN(2, ("ohci_grow_std: allocating chunk\n"));
	err = usb_allocmem(&sc->sc_dmatag,
	    OHCI_STD_SIZE*OHCI_STD_CHUNK + sizeof(struct ohci_mem_desc),
	    OHCI_TD_ALIGN, &dma);
	if (err)
		return (err);
	om = KERNADDR(&dma, OHCI_STD_SIZE * OHCI_STD_CHUNK);
	om->om_top = KERNADDR(&dma, 0);
	om->om_topdma = DMAADDR(&dma, 0);
	om->om_dma = dma;
	s = splusb();
	SIMPLEQ_INSERT_HEAD(&sc->sc_std_chunks, om, om_next);
	for(i = 0; i < OHCI_STD_CHUNK; i++) {
		offs = i * OHCI_STD_SIZE;
		std = KERNADDR(&dma, offs);
		std->ot_mdesc = om;
		std->nexttd = sc->sc_freetds;
		std->ad.aux_len = 0;
		sc->sc_freetds = std;
		sc->sc_nfreetds++;
	}
	splx(s);

	return (USBD_NORMAL_COMPLETION);
}

ohci_soft_td_t *
ohci_alloc_std(ohci_softc_t *sc)
{
	ohci_soft_td_t *std;
	int s;

	s = splusb();

#ifdef DIAGNOSTIC
	if (sc->sc_freetds == NULL)
		panic("ohci_alloc_std: %d", sc->sc_nfreetds);
#endif
	std = sc->sc_freetds;
	sc->sc_freetds = std->nexttd;
	splx(s);
	memset(&std->td, 0, sizeof(ohci_td_t));
	std->nexttd = NULL;
	std->xfer = NULL;
	std->flags = 0;

	return (std);
}

Static ohci_soft_td_t *
ohci_alloc_std_norsv(ohci_softc_t *sc)
{
	int s;

	s = splusb();
	if (sc->sc_nfreetds < 1)
		if (ohci_grow_std(sc))
			return (NULL);
	sc->sc_nfreetds--;
	splx(s);
	return (ohci_alloc_std(sc));
}

void
ohci_free_std(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	int s;

	s = splusb();
	std->flags = OHCI_TD_FREE;
	std->nexttd = sc->sc_freetds;
	sc->sc_freetds = std;
	splx(s);
}

Static void
ohci_free_std_norsv(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	int s;

	ohci_free_std(sc, std);
	s = splusb();
	sc->sc_nfreetds++;
	splx(s);
}

usbd_status
ohci_alloc_std_chain(struct ohci_pipe *opipe, ohci_softc_t *sc,
		     int alen, int rd, usbd_xfer_handle xfer,
		     ohci_soft_td_t *sp, ohci_soft_td_t **ep)
{
	ohci_soft_td_t *next, *cur, *end;
	ohci_physaddr_t tddma, segdmaadr, dmaend;
	u_int32_t tdflags;
	int len, maxp, curlen, seg, seglen, seglen1;
	struct usb_buffer_dma *ub = &OXFER(xfer)->dmabuf;
	bus_dma_segment_t *segs = USB_BUFFER_SEGS(ub);
	int nsegs = USB_BUFFER_NSEGS(ub);
	u_int16_t flags = xfer->flags;
	usb_endpoint_descriptor_t *ed;
	int needaux;
	union usb_bufptr bufptr;
	bus_addr_t auxdma;

	DPRINTFN(alen < 4096,("ohci_alloc_std_chain: start len=%d\n", alen));

	len = alen;
	cur = sp;
	end = NULL;

	ed = opipe->pipe.endpoint->edesc;
	maxp = UE_MAXPKTSZ(ed);
	tdflags = HTOO32(
	    (rd ? OHCI_TD_IN : OHCI_TD_OUT) |
	    (flags & USBD_SHORT_XFER_OK ? OHCI_TD_R : 0) |
	    OHCI_TD_NOCC | OHCI_TD_TOGGLE_CARRY | OHCI_TD_SET_DI(6));
	/*
	 * aux memory is possibly required if
	 *	buffer has more than one segments,
	 *	the transfer direction is OUT,
	 *	and
	 *		buffer is an mbuf chain, or
	 *		buffer is not maxp aligned
	 */
	needaux =
	    nsegs > 1 &&
	    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
	    ((xfer->rqflags & URQ_DEV_MAP_MBUF) ||
	     (maxp & (maxp - 1)) != 0 /* maxp is not a power of 2 */ ||
	     (segs[0].ds_addr & (maxp - 1)) != 0 /* buffer is unaligned */);

	if (needaux) {
		usb_bufptr_init(&bufptr, xfer);
	}

	seg = 0;
	seglen = 0;
	segdmaadr = 0;	/* XXX -Wuninitialized */
	while (len > 0) {
		/* sync last entry */
		if (end)
			OHCI_STD_SYNC(sc, end,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		next = ohci_alloc_std(sc);
		if (next == NULL)
			goto nomem;

		/*
		 * The OHCI hardware can handle at most one 4K crossing.
		 * The OHCI spec says: If during the data transfer the buffer
		 * address contained in the HC's working copy of
		 * CurrentBufferPointer crosses a 4K boundary, the upper 20
		 * bits of Buffer End are copied to the working value of
		 * CurrentBufferPointer causing the next buffer address to
		 * be the 0th byte in the same 4K page that contains the
		 * last byte of the buffer (the 4K boundary crossing may
		 * occur within a data packet transfer.)
		 */

		/*
		 * 1. skip handled segments
		 * 2. gather segments that are continuous in DMA address
		 *    (Does this make any sense?)
		 */
		if (seglen <= 0) {
			USB_KASSERT2(seg < nsegs,
			    ("ohci_alloc_std_chain: overrun"));
			do {
				seglen1 = seglen;
				seglen += segs[seg].ds_len;
				if (seglen1 <= 0 && seglen > 0)
					segdmaadr = segs[seg].ds_addr - seglen1;
			} while (++seg < nsegs && (seglen <= 0 ||
			    segdmaadr + seglen == segs[seg].ds_addr));

			if (seglen > len)
				seglen = len;
		}

		if (needaux && seglen < len && seglen < maxp) {
			/* need aux -- a packet is not contiguous */
			curlen = len < maxp ? len : maxp;
			USB_KASSERT(seglen < curlen);
			auxdma = ohci_aux_dma_alloc(&OXFER(xfer)->aux, &bufptr,
			    curlen, &cur->ad);

			/* prepare aux DMA */
			usb_bufptr_wr(&bufptr, cur->ad.aux_kern,
			    curlen, xfer->rqflags & URQ_DEV_MAP_MBUF);
			cur->td.td_cbp = HTOO32(auxdma);
			cur->td.td_be = HTOO32(auxdma + curlen - 1);

			/* handled segments will be skipped above */
		} else {
#ifdef DIAGNOSTIC
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT)
				USB_KASSERT(seglen >= maxp || seglen >= len);
#endif
			curlen = seglen;
			dmaend = segdmaadr + curlen;
			if ((sc->sc_flags & OHCI_FLAG_QUIRK_2ND_4KB) == 0)
				dmaend--;
			dmaend = OHCI_PAGE(dmaend);

			if (OHCI_PAGE(segdmaadr) != dmaend &&
			    OHCI_PAGE(segdmaadr) + OHCI_PAGE_SIZE != dmaend) {
				/* must use multiple TDs, fill as much as possible. */
				curlen = 2 * OHCI_PAGE_SIZE -
					 (segdmaadr & (OHCI_PAGE_SIZE-1));

				if (sc->sc_flags & OHCI_FLAG_QUIRK_2ND_4KB)
					curlen--;
			}

			/* the length must be a multiple of the max size */
			if (curlen < len)
				curlen -= curlen % maxp;
			USB_KASSERT(curlen);
			USB_KASSERT2(curlen <= seglen,
			    ("ohci_alloc_std_chain: curlen %d > seglen %d",
			    curlen, len));

			DPRINTFN(4,("ohci_alloc_std_chain: segdmaadr=0x%08x "
				    "dmaend=0x%08x len=%d seglen=%d curlen=%d\n",
				    segdmaadr, segdmaadr + curlen - 1,
				    len, seglen, curlen));
			cur->td.td_cbp = HTOO32(segdmaadr);
			cur->td.td_be = HTOO32(segdmaadr + curlen - 1);
		}
		len -= curlen;

		cur->td.td_flags = tdflags;
		cur->nexttd = next;
		tddma = OHCI_STD_DMAADDR(next);
		cur->td.td_nexttd = HTOO32(tddma);
		cur->len = curlen;
		cur->flags = OHCI_ADD_LEN;
		cur->xfer = xfer;
		DPRINTFN(10,("ohci_alloc_std_chain: cbp=0x%08x be=0x%08x",
			    O32TOH(cur->td.td_cbp), O32TOH(cur->td.td_be)));
		seglen -= curlen;
		segdmaadr += curlen;
		DPRINTFN(10,("%s\n", seglen < 0 ? " (aux)" : ""));

		if (needaux)
			usb_bufptr_advance(&bufptr, curlen,
			    xfer->rqflags & URQ_DEV_MAP_MBUF);

		end = cur;
		cur = next;
	}
	if (((flags & USBD_FORCE_SHORT_XFER) || alen == 0) &&
	    alen % maxp == 0) {
		if (end)
			OHCI_STD_SYNC(sc, end,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Force a 0 length transfer at the end. */
		next = ohci_alloc_std(sc);
		if (next == NULL)
			goto nomem;

		cur->td.td_flags = tdflags;
		cur->td.td_cbp = 0; /* indicate 0 length packet */
		cur->nexttd = next;
		tddma = OHCI_STD_DMAADDR(next);
		cur->td.td_nexttd = HTOO32(tddma);
		cur->td.td_be = ~0;
		cur->len = 0;
		cur->flags = 0;
		cur->xfer = xfer;
		DPRINTFN(2,("ohci_alloc_std_chain: add 0 xfer\n"));
		end = cur;
	}
	*ep = end;

	if (needaux)
		ohci_aux_dma_sync(sc, &OXFER(xfer)->aux, BUS_DMASYNC_PREWRITE);

	return (USBD_NORMAL_COMPLETION);

 nomem:
	/* free chain */
	if (end)
		ohci_free_std_chain(sc, sp->nexttd, end);
	return (USBD_NOMEM);
}

Static void
ohci_free_std_chain(ohci_softc_t *sc, ohci_soft_td_t *std,
		    ohci_soft_td_t *stdend)
{
	ohci_soft_td_t *p;

	for (; std != stdend; std = p) {
		p = std->nexttd;
		ohci_free_std(sc, std);
	}
	ohci_free_std(sc, stdend);
}

Static usbd_status
ohci_grow_sitd(ohci_softc_t *sc)
{
	usb_dma_t dma;
	struct ohci_mem_desc *om;
	ohci_soft_itd_t *sitd;
	usbd_status err;
	int i, s, offs;

	DPRINTFN(2, ("ohci_alloc_sitd: allocating chunk\n"));
	err = usb_allocmem(&sc->sc_dmatag,
	    OHCI_SITD_SIZE*OHCI_SITD_CHUNK + sizeof(struct ohci_mem_desc),
	    OHCI_ITD_ALIGN, &dma);
	if (err)
		return (err);
	om = KERNADDR(&dma, OHCI_SITD_SIZE * OHCI_SITD_CHUNK);
	om->om_top = KERNADDR(&dma, 0);
	om->om_topdma = DMAADDR(&dma, 0);
	om->om_dma = dma;
	s = splusb();
	SIMPLEQ_INSERT_HEAD(&sc->sc_sitd_chunks, om, om_next);
	for(i = 0; i < OHCI_SITD_CHUNK; i++) {
		offs = i * OHCI_SITD_SIZE;
		sitd = KERNADDR(&dma, offs);
		sitd->oit_mdesc = om;
		sitd->nextitd = sc->sc_freeitds;
		sitd->ad.aux_len = 0;
		sc->sc_freeitds = sitd;
		sc->sc_nfreeitds++;
	}
	splx(s);

	return (USBD_NORMAL_COMPLETION);
}

ohci_soft_itd_t *
ohci_alloc_sitd(ohci_softc_t *sc)
{
	ohci_soft_itd_t *sitd;
	int s;

#ifdef DIAGNOSTIC
	if (sc->sc_freeitds == NULL)
		panic("ohci_alloc_sitd: %d", sc->sc_nfreeitds);
#endif

	s = splusb();
	sitd = sc->sc_freeitds;
	sc->sc_freeitds = sitd->nextitd;
	memset(&sitd->itd, 0, sizeof(ohci_itd_t));
	sitd->nextitd = NULL;
	sitd->xfer = NULL;
	sitd->flags = 0;
	splx(s);

#ifdef DIAGNOSTIC
	sitd->isdone = 0;
#endif

	return (sitd);
}

Static ohci_soft_itd_t *
ohci_alloc_sitd_norsv(ohci_softc_t *sc)
{
	int s;

	s = splusb();
	if (sc->sc_nfreeitds < 1)
		if (ohci_grow_sitd(sc))
			return (NULL);
	sc->sc_nfreeitds--;
	splx(s);
	return (ohci_alloc_sitd(sc));
}

void
ohci_free_sitd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	int s;

	DPRINTFN(10,("ohci_free_sitd: sitd=%p\n", sitd));

#ifdef DIAGNOSTIC
	if (!sitd->isdone) {
		panic("ohci_free_sitd: sitd=%p not done", sitd);
		return;
	}
	/* Warn double free */
	sitd->isdone = 0;
#endif

	s = splusb();
	sitd->flags = OHCI_ITD_FREE;
	sitd->nextitd = sc->sc_freeitds;
	sc->sc_freeitds = sitd;
	splx(s);
}

Static void
ohci_free_sitd_norsv(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	int s;

	ohci_free_sitd(sc, sitd);
	s = splusb();
	sc->sc_nfreeitds++;
	splx(s);
}

usbd_status
ohci_init(ohci_softc_t *sc)
{
	ohci_soft_ed_t *sed, *psed;
	ohci_physaddr_t eddma;
	usbd_status err;
	int i;
	u_int32_t rev;

	DPRINTF(("ohci_init: start\n"));
#if defined(__OpenBSD__)
	printf(",");
#else
	printf("%s:", USBDEVNAME(sc->sc_bus.bdev));
#endif
	rev = OREAD4(sc, OHCI_REVISION);
	printf(" OHCI version %d.%d%s\n", OHCI_REV_HI(rev), OHCI_REV_LO(rev),
	       OHCI_REV_LEGACY(rev) ? ", legacy support" : "");

	if (OHCI_REV_HI(rev) != 1 || OHCI_REV_LO(rev) != 0) {
		printf("%s: unsupported OHCI revision\n",
		       USBDEVNAME(sc->sc_bus.bdev));
		sc->sc_bus.usbrev = USBREV_UNKNOWN;
		return (USBD_INVAL);
	}
	sc->sc_bus.usbrev = USBREV_1_0;

	SIMPLEQ_INIT(&sc->sc_free_xfers);
	SIMPLEQ_INIT(&sc->sc_sed_chunks);
	SIMPLEQ_INIT(&sc->sc_std_chunks);
	SIMPLEQ_INIT(&sc->sc_sitd_chunks);

	usb_dma_tag_init(&sc->sc_dmatag);

	/* XXX determine alignment by R/W */
	/* Allocate the HCCA area. */
	err = usb_allocmem(&sc->sc_dmatag, OHCI_HCCA_SIZE,
			 OHCI_HCCA_ALIGN, &sc->sc_hccadma);
	if (err)
		goto bad1;
	sc->sc_hcca = KERNADDR(&sc->sc_hccadma, 0);
	memset(sc->sc_hcca, 0, OHCI_HCCA_SIZE);

	sc->sc_eintrs = OHCI_NORMAL_INTRS;

	/* Allocate dummy ED that starts the control list. */
	sc->sc_ctrl_head = ohci_alloc_sed(sc);
	if (sc->sc_ctrl_head == NULL) {
		err = USBD_NOMEM;
		goto bad2;
	}
	sc->sc_ctrl_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);

	/* Allocate dummy ED that starts the bulk list. */
	sc->sc_bulk_head = ohci_alloc_sed(sc);
	if (sc->sc_bulk_head == NULL) {
		err = USBD_NOMEM;
		goto bad3;
	}
	sc->sc_bulk_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);

	/* Allocate dummy ED that starts the isochronous list. */
	sc->sc_isoc_head = ohci_alloc_sed(sc);
	if (sc->sc_isoc_head == NULL) {
		err = USBD_NOMEM;
		goto bad3;
	}
	sc->sc_isoc_head->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);

	/* Allocate all the dummy EDs that make up the interrupt tree. */
	for (i = 0; i < OHCI_NO_EDS; i++) {
		sed = ohci_alloc_sed(sc);
		if (sed == NULL) {
			while (--i >= 0)
				ohci_free_sed(sc, sc->sc_eds[i]);
			err = USBD_NOMEM;
			goto bad3;
		}
		/* All ED fields are set to 0. */
		sc->sc_eds[i] = sed;
		sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
		if (i != 0)
			psed = sc->sc_eds[(i-1) / 2];
		else
			psed = sc->sc_isoc_head;
		sed->next = psed;
		eddma = OHCI_SED_DMAADDR(psed);
		sed->ed.ed_nexted = HTOO32(eddma);
	}
	/*
	 * Fill HCCA interrupt table.  The bit reversal is to get
	 * the tree set up properly to spread the interrupts.
	 */
	for (i = 0; i < OHCI_NO_INTRS; i++) {
		eddma = OHCI_SED_DMAADDR(sc->sc_eds[OHCI_NO_EDS-OHCI_NO_INTRS+i]);
		sc->sc_hcca->hcca_interrupt_table[revbits[i]] = HTOO32(eddma);
	}
	USB_MEM_SYNC(&sc->sc_dmatag, &sc->sc_hccadma,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

#ifdef OHCI_DEBUG
	if (ohcidebug > 15) {
		for (i = 0; i < OHCI_NO_EDS; i++) {
			printf("ed#%d ", i);
			ohci_dump_ed(sc, sc->sc_eds[i]);
		}
		printf("iso ");
		ohci_dump_ed(sc, sc->sc_isoc_head);
	}
#endif

	err = ohci_controller_init(sc);
	if (err != USBD_NORMAL_COMPLETION)
		goto bad3;

	/* Set up the bus struct. */
	sc->sc_bus.methods = &ohci_bus_methods;
	sc->sc_bus.pipe_size = sizeof(struct ohci_pipe);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	sc->sc_control = sc->sc_intre = 0;
	sc->sc_powerhook = powerhook_establish(USBDEVNAME(sc->sc_bus.bdev),
	    ohci_power, sc);
	sc->sc_shutdownhook = shutdownhook_establish(ohci_shutdown, sc);
#endif

	usb_callout_init(sc->sc_tmo_rhsc);

	/* Finally, turn on interrupts. */
	DPRINTFN(1,("ohci_init: enabling\n"));
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, sc->sc_eintrs | OHCI_MIE);

	return (USBD_NORMAL_COMPLETION);

 bad3:
	ohci_free_desc_chunks(sc, &sc->sc_sed_chunks);
 bad2:
	usb_freemem(&sc->sc_dmatag, &sc->sc_hccadma);
 bad1:
	usb_dma_tag_finish(&sc->sc_dmatag);
	return (err);
}

Static usbd_status
ohci_controller_init(ohci_softc_t *sc)
{
	int i;
	u_int32_t s, ctl, rwc, ival, hcr, fm, per, desca, descb;

	/* Preserve values programmed by SMM/BIOS but lost over reset. */
	ctl = OREAD4(sc, OHCI_CONTROL);
	rwc = ctl & OHCI_RWC;
	fm = OREAD4(sc, OHCI_FM_INTERVAL);
	desca = OREAD4(sc, OHCI_RH_DESCRIPTOR_A);
	descb = OREAD4(sc, OHCI_RH_DESCRIPTOR_B);

	/* Determine in what context we are running. */
	if (ctl & OHCI_IR) {
		/* SMM active, request change */
		DPRINTF(("ohci_init: SMM active, request owner change\n"));
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
			printf("%s: SMM does not respond, resetting\n",
			       USBDEVNAME(sc->sc_bus.bdev));
			OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET | rwc);
			goto reset;
		}
#if 0
/* Don't bother trying to reuse the BIOS init, we'll reset it anyway. */
	} else if ((ctl & OHCI_HCFS_MASK) != OHCI_HCFS_RESET) {
		/* BIOS started controller. */
		DPRINTF(("ohci_init: BIOS active\n"));
		if ((ctl & OHCI_HCFS_MASK) != OHCI_HCFS_OPERATIONAL) {
			OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_OPERATIONAL | rwc);
			usb_delay_ms(&sc->sc_bus, USB_RESUME_DELAY);
		}
#endif
	} else {
		DPRINTF(("ohci_init: cold started\n"));
	reset:
		/* Controller was cold started. */
		usb_delay_ms(&sc->sc_bus, USB_BUS_RESET_DELAY);
	}

	/*
	 * This reset should not be necessary according to the OHCI spec, but
	 * without it some controllers do not start.
	 */
	DPRINTF(("%s: resetting\n", USBDEVNAME(sc->sc_bus.bdev)));
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
		printf("%s: reset timeout\n", USBDEVNAME(sc->sc_bus.bdev));
		return (USBD_IOERROR);
	}
#ifdef OHCI_DEBUG
	if (ohcidebug > 15)
		ohci_dumpregs(sc);
#endif

	/* The controller is now in SUSPEND state, we have 2ms to finish. */

	/* Set up HC registers. */
	OWRITE4(sc, OHCI_HCCA, DMAADDR(&sc->sc_hccadma, 0));
	OWRITE4(sc, OHCI_CONTROL_HEAD_ED, OHCI_SED_DMAADDR(sc->sc_ctrl_head));
	OWRITE4(sc, OHCI_BULK_HEAD_ED, OHCI_SED_DMAADDR(sc->sc_bulk_head));
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
	if (ohcidebug > 5)
		ohci_dumpregs(sc);
#endif
	return (USBD_NORMAL_COMPLETION);
}

/*
 * Allocate a physically contiguous buffer to handle cases where OHCI
 * cannot handle a packet because it is not physically contiguous.
 */
Static void
ohci_aux_mem_init(struct ohci_aux_mem *aux)
{

	aux->aux_curchunk = aux->aux_chunkoff = aux->aux_naux = 0;
}

Static usbd_status
ohci_aux_mem_alloc(ohci_softc_t *sc, struct ohci_aux_mem *aux,
	int naux, int maxp)
{
	int nchunk, i, j;
	usbd_status err;

	USB_KASSERT(aux->aux_nchunk == 0);

	nchunk = OHCI_NCHUNK(naux, maxp);
	for (i = 0; i < nchunk; i++) {
		err = usb_allocmem(&sc->sc_dmatag, OHCI_AUX_CHUNK_SIZE,
		    OHCI_AUX_CHUNK_SIZE, &aux->aux_chunk_dma[i]);
		if (err) {
			for (j = 0; j < i; j++)
				usb_freemem(&sc->sc_dmatag,
				    &aux->aux_chunk_dma[j]);
			return (err);
		}
	}

	aux->aux_nchunk = nchunk;
	ohci_aux_mem_init(aux);

	return (USBD_NORMAL_COMPLETION);
}

Static void
ohci_aux_mem_free(ohci_softc_t *sc, struct ohci_aux_mem *aux)
{
	int i;

	for (i = 0; i < aux->aux_nchunk; i++)
		usb_freemem(&sc->sc_dmatag, &aux->aux_chunk_dma[i]);

	aux->aux_nchunk = 0;
}

Static bus_addr_t
ohci_aux_dma_alloc(struct ohci_aux_mem *aux,
	const union usb_bufptr *bufptr, int len,
	struct usb_aux_desc *ad)
{
	bus_addr_t auxdma;

	if (aux->aux_chunkoff + len > OHCI_AUX_CHUNK_SIZE) {
		aux->aux_curchunk++;
		aux->aux_chunkoff = 0;
	}
	USB_KASSERT(aux->aux_curchunk < aux->aux_nchunk);

	auxdma =
	    DMAADDR(&aux->aux_chunk_dma[aux->aux_curchunk], aux->aux_chunkoff);
	ad->aux_kern =
	    KERNADDR(&aux->aux_chunk_dma[aux->aux_curchunk], aux->aux_chunkoff);
	ad->aux_ptr = *bufptr;
	ad->aux_len = len;

	aux->aux_chunkoff += len;
	aux->aux_naux++;

	return auxdma;
}

Static void
ohci_aux_dma_complete(struct usb_aux_desc *ad, struct ohci_aux_mem *aux,
	int is_mbuf, int isread)
{

	if (isread) {
		usb_bufptr_rd(&ad->aux_ptr, ad->aux_kern, ad->aux_len,
		    is_mbuf);
	}
	ad->aux_len = 0;
	USB_KASSERT(aux->aux_naux > 0);
	if (--aux->aux_naux == 0)
		ohci_aux_mem_init(aux);
}

Static void
ohci_aux_dma_sync(ohci_softc_t *sc, struct ohci_aux_mem *aux, int op)
{
	int i;

	for (i = 0; i < aux->aux_curchunk; i++)
		USB_MEM_SYNC(&sc->sc_dmatag, &aux->aux_chunk_dma[i], op);
	if (aux->aux_chunkoff)
		USB_MEM_SYNC2(&sc->sc_dmatag, &aux->aux_chunk_dma[i],
		    0, aux->aux_chunkoff, op);
}

Static usbd_status
ohci_prealloc(struct ohci_softc *sc, struct ohci_xfer *oxfer,
	size_t bufsize, int nseg)
{
	usb_endpoint_descriptor_t *ed;
	int maxseg, maxp, naux;
	int seglen, ntd;
	int s;
	int err;

	ed = oxfer->xfer.pipe->endpoint->edesc;
	maxp = UE_MAXPKTSZ(ed);

	if (maxp == 0)
		return (USBD_INVAL);

	/* (over) estimate needed number of TDs */
	maxseg = (sc->sc_flags & OHCI_FLAG_QUIRK_2ND_4KB) ? 4096 : 8192;
	seglen = maxseg - (maxseg % maxp);
	ntd = bufsize / seglen + nseg;

	/* allocate aux buffer for non-isoc OUT or isoc transfer */
	naux = 0;
	if (nseg > 1 &&
	    ((ed->bmAttributes & UE_XFERTYPE) == UE_ISOCHRONOUS ||
	     UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT)) {
		/* estimate needed aux segments */
		naux = nseg - 1;

		/* pre-allocate aux memory */
		err = ohci_aux_mem_alloc(sc, &oxfer->aux, naux, maxp);
		if (err)
			return err;

		/* an aux segment will consume a TD */
		ntd += naux;
	}

	s = splusb();
	if ((ed->bmAttributes & UE_XFERTYPE) == UE_ISOCHRONOUS) {
		/* pre-allocate ITDs */
		while (sc->sc_nfreeitds < ntd) {
			DPRINTF(("%s: ohci_prealloc: need %d ITD (%d cur)\n",
			    USBDEVNAME(sc->sc_bus.bdev),
			    ntd, sc->sc_nfreeitds));
			if ((err = ohci_grow_sitd(sc))
			    != USBD_NORMAL_COMPLETION) {
				splx(s);
				ohci_aux_mem_free(sc, &oxfer->aux);
				return (err);
			}
		}
		sc->sc_nfreeitds -= ntd;
	} else {
		/* pre-allocate TDs */
		while (sc->sc_nfreetds < ntd) {
			DPRINTF(("%s: ohci_prealloc: need %d TD (%d cur)\n",
			    USBDEVNAME(sc->sc_bus.bdev),
			    ntd, sc->sc_nfreetds));
			if ((err = ohci_grow_std(sc))
			    != USBD_NORMAL_COMPLETION) {
				splx(s);
				ohci_aux_mem_free(sc, &oxfer->aux);
				return (err);
			}
		}
		sc->sc_nfreetds -= ntd;
	}
	splx(s);

	oxfer->rsvd_tds = ntd;

	return (USBD_NORMAL_COMPLETION);
}

usbd_status
ohci_allocm(struct usbd_bus *bus, usbd_xfer_handle xfer, void *buf, size_t size)
{
	struct ohci_softc *sc = (struct ohci_softc *)bus;
	struct ohci_xfer *oxfer = OXFER(xfer);
	usbd_status err;

	if ((err = usb_alloc_buffer_dma(&sc->sc_dmatag, &oxfer->dmabuf,
	    buf, size, &xfer->hcbuffer)) == USBD_NORMAL_COMPLETION) {
		if ((xfer->rqflags & URQ_DEV_MAP_PREPARED) == 0 &&
		    (err = ohci_prealloc(sc, oxfer, size,
		    USB_BUFFER_NSEGS(&oxfer->dmabuf)))
		    != USBD_NORMAL_COMPLETION) {
			usb_free_buffer_dma(&sc->sc_dmatag, &oxfer->dmabuf,
			    U_WAITOK);
		}
	}

	return err;
}

void
ohci_freem(struct usbd_bus *bus, usbd_xfer_handle xfer,
	enum usbd_waitflg waitflg)
{
	struct ohci_softc *sc = (struct ohci_softc *)bus;
	struct ohci_xfer *oxfer = OXFER(xfer);
	int s;

	usb_free_buffer_dma(&sc->sc_dmatag, &oxfer->dmabuf, waitflg);

	if ((xfer->rqflags & URQ_DEV_MAP_PREPARED) == 0) {
		s = splusb();
		if ((xfer->pipe->endpoint->edesc->bmAttributes & UE_XFERTYPE) ==
		    UE_ISOCHRONOUS) {
			sc->sc_nfreeitds += oxfer->rsvd_tds;
		} else {
			sc->sc_nfreetds += oxfer->rsvd_tds;
		}
		splx(s);
		ohci_aux_mem_free(sc, &oxfer->aux);
		oxfer->rsvd_tds = 0;
	}
}

Static usbd_status
ohci_map_alloc(usbd_xfer_handle xfer)
{
	struct ohci_softc *sc = (struct ohci_softc *)xfer->device->bus;
	struct ohci_xfer *oxfer = OXFER(xfer);
	usbd_status st;

	st = usb_alloc_dma_resources(&sc->sc_dmatag, &oxfer->dmabuf);
	if (st)
		return st;

	if ((st = ohci_prealloc(sc, oxfer, MAXPHYS, USB_DMA_NSEG))) {
		usb_free_dma_resources(&sc->sc_dmatag, &oxfer->dmabuf);
	}

	return st;
}

Static void
ohci_map_free(usbd_xfer_handle xfer)
{
	struct ohci_softc *sc = (struct ohci_softc *)xfer->device->bus;
	struct ohci_xfer *oxfer = OXFER(xfer);
	int s;

	USB_KASSERT(xfer->rqflags & URQ_DEV_MAP_PREPARED);

	usb_free_dma_resources(&sc->sc_dmatag, &oxfer->dmabuf);

	s = splusb();
	if ((xfer->pipe->endpoint->edesc->bmAttributes & UE_XFERTYPE) ==
	    UE_ISOCHRONOUS) {
		sc->sc_nfreeitds += oxfer->rsvd_tds;
	} else {
		sc->sc_nfreetds += oxfer->rsvd_tds;
	}
	splx(s);
	ohci_aux_mem_free(sc, &oxfer->aux);
	oxfer->rsvd_tds = 0;
}

Static void
ohci_mapm(usbd_xfer_handle xfer, void *buf, size_t size)
{
	struct ohci_softc *sc = (struct ohci_softc *)xfer->device->bus;
	struct ohci_xfer *oxfer = OXFER(xfer);

	usb_map_dma(&sc->sc_dmatag, &oxfer->dmabuf, buf, size);
}

Static usbd_status
ohci_mapm_mbuf(usbd_xfer_handle xfer, struct mbuf *chain)
{
	struct ohci_softc *sc = (struct ohci_softc *)xfer->device->bus;
	struct ohci_xfer *oxfer = OXFER(xfer);

	return (usb_map_mbuf_dma(&sc->sc_dmatag, &oxfer->dmabuf, chain));
}

Static void
ohci_unmapm(usbd_xfer_handle xfer)
{
	struct ohci_softc *sc = (struct ohci_softc *)xfer->device->bus;
	struct ohci_xfer *oxfer = OXFER(xfer);

	usb_unmap_dma(&sc->sc_dmatag, &oxfer->dmabuf);
}

usbd_xfer_handle
ohci_allocx(struct usbd_bus *bus, usbd_pipe_handle pipe,
	enum usbd_waitflg waitflg)
{
	struct ohci_softc *sc = (struct ohci_softc *)bus;
	usbd_xfer_handle xfer;

	xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers);
	if (xfer != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
#ifdef DIAGNOSTIC
		if (xfer->busy_free != XFER_FREE) {
			printf("ohci_allocx: xfer=%p not free, 0x%08x\n", xfer,
			       xfer->busy_free);
		}
#endif
	} else {
		xfer = malloc(sizeof(struct ohci_xfer), M_USB,
		    waitflg == U_WAITOK ? M_WAITOK : M_NOWAIT);
	}
	if (xfer != NULL) {
		memset(xfer, 0, sizeof (struct ohci_xfer));
		usb_init_task(&OXFER(xfer)->abort_task, ohci_timeout_task,
		    xfer);
		OXFER(xfer)->ohci_xfer_flags = 0;
		OXFER(xfer)->rsvd_tds = 0;
#ifdef DIAGNOSTIC
		xfer->busy_free = XFER_BUSY;
#endif
	}
	return (xfer);
}

void
ohci_freex(struct usbd_bus *bus, usbd_xfer_handle xfer)
{
	struct ohci_softc *sc = (struct ohci_softc *)bus;

#ifdef DIAGNOSTIC
	if (xfer->busy_free != XFER_BUSY) {
		panic("ohci_freex: xfer=%p not busy, 0x%08x\n", xfer,
		       xfer->busy_free);
	}
	xfer->busy_free = XFER_FREE;
#endif
	SIMPLEQ_INSERT_HEAD(&sc->sc_free_xfers, xfer, next);
}

/*
 * Perform any OHCI-specific transfer completion operations, then
 * call usb_transfer_complete().
 */
Static void
ohci_transfer_complete(usbd_xfer_handle xfer, int clear_halt)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	struct usb_buffer_dma *ub = &OXFER(xfer)->dmabuf;
	ohci_soft_itd_t *sitd, *sitdnext;
	ohci_soft_td_t  *std,  *stdnext;
	ohci_physaddr_t dmaadr;
	int isread, is_mbuf;
	int s;

	isread = usbd_xfer_isread(xfer);
	is_mbuf = xfer->rqflags & URQ_DEV_MAP_MBUF;

	if (ub)
		usb_sync_buffer_dma(&sc->sc_dmatag, ub,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);

	if (OXFER(xfer)->aux.aux_naux) {
		/* sync aux */
		ohci_aux_dma_sync(sc, &OXFER(xfer)->aux,
		    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}

	/* Copy back from any auxillary buffers after a read operation. */
	if (xfer->hcpriv == NULL) {
		DPRINTFN(-1, ("ohci_transfer_complete: hcpriv == NULL\n"));
	} else if (xfer->nframes == 0) {
		/* Bulk/Interrupt transfer */
		/* Remove all TDs belonging to this xfer. */
		for (std = xfer->hcpriv; std->xfer == xfer; std = stdnext) {
			stdnext = std->nexttd;
			if (std->ad.aux_len)
				ohci_aux_dma_complete(&std->ad,
				    &OXFER(xfer)->aux, is_mbuf, isread);
			ohci_free_std(sc, std);
		}
		if (clear_halt) {
			/* clear halt */
			OHCI_SED_SYNC_POST(sc, opipe->sed);
			dmaadr = OHCI_STD_DMAADDR(std);
			opipe->sed->ed.ed_headp = HTOO32(dmaadr);
			OHCI_SED_SYNC2(sc, opipe->sed,
			    offsetof(ohci_ed_t, ed_headp),
			    sizeof(ohci_physaddr_t),
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_CLF);
		}
	} else {
		/* Isoc transfer */
		/* Remove all ITDs belonging to this xfer. */
		for (sitd = xfer->hcpriv; sitd->xfer == xfer;
		    sitd = sitdnext) {
			sitdnext = sitd->nextitd;
			if (sitd->ad.aux_len)
				ohci_aux_dma_complete(&sitd->ad,
				    &OXFER(xfer)->aux, is_mbuf, isread);
			ohci_free_sitd(sc, sitd);
		}
	}

	USB_KASSERT(OXFER(xfer)->aux.aux_naux == 0);
	xfer->hcpriv = NULL;

	s = splusb();
	usb_transfer_complete(xfer);
	splx(s);
}

/*
 * Shut down the controller when the system is going down.
 */
void
ohci_shutdown(void *v)
{
	ohci_softc_t *sc = v;

	DPRINTF(("ohci_shutdown: stopping the HC\n"));
	OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET);
}

/*
 * Handle suspend/resume.
 *
 * We need to switch to polling mode here, because this routine is
 * called from an interupt context.  This is all right since we
 * are almost suspended anyway.
 */
void
ohci_power(int why, void *v)
{
	ohci_softc_t *sc = v;
	u_int32_t ctl;
	int s;

#ifdef OHCI_DEBUG
	DPRINTF(("ohci_power: sc=%p, why=%d\n", sc, why));
	ohci_dumpregs(sc);
#endif

	s = splhardusb();
	switch (why) {
	USB_PWR_CASE_SUSPEND:
		sc->sc_bus.use_polling++;
		ctl = OREAD4(sc, OHCI_CONTROL) & ~OHCI_HCFS_MASK;
		if (sc->sc_control == 0) {
			/*
			 * Preserve register values, in case that APM BIOS
			 * does not recover them.
			 */
			sc->sc_control = ctl;
			sc->sc_intre = OREAD4(sc, OHCI_INTERRUPT_ENABLE);
		}
		ctl |= OHCI_HCFS_SUSPEND;
		OWRITE4(sc, OHCI_CONTROL, ctl);
		usb_delay_ms(&sc->sc_bus, USB_RESUME_WAIT);
		sc->sc_bus.use_polling--;
		break;

	USB_PWR_CASE_RESUME:
		sc->sc_bus.use_polling++;

		/* Some broken BIOSes never initialize Controller chip */
		ohci_controller_init(sc);

		if (sc->sc_intre)
			OWRITE4(sc, OHCI_INTERRUPT_ENABLE,
				sc->sc_intre & (OHCI_ALL_INTRS | OHCI_MIE));
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
		sc->sc_bus.use_polling--;
		break;

	default:
		break;
	}
	splx(s);
}

#ifdef OHCI_DEBUG
void
ohci_dumpregs(ohci_softc_t *sc)
{
	DPRINTF(("ohci_dumpregs: rev=0x%08x control=0x%08x command=0x%08x\n",
		 OREAD4(sc, OHCI_REVISION),
		 OREAD4(sc, OHCI_CONTROL),
		 OREAD4(sc, OHCI_COMMAND_STATUS)));
	DPRINTF(("               intrstat=0x%08x intre=0x%08x intrd=0x%08x\n",
		 OREAD4(sc, OHCI_INTERRUPT_STATUS),
		 OREAD4(sc, OHCI_INTERRUPT_ENABLE),
		 OREAD4(sc, OHCI_INTERRUPT_DISABLE)));
	DPRINTF(("               hcca=0x%08x percur=0x%08x ctrlhd=0x%08x\n",
		 OREAD4(sc, OHCI_HCCA),
		 OREAD4(sc, OHCI_PERIOD_CURRENT_ED),
		 OREAD4(sc, OHCI_CONTROL_HEAD_ED)));
	DPRINTF(("               ctrlcur=0x%08x bulkhd=0x%08x bulkcur=0x%08x\n",
		 OREAD4(sc, OHCI_CONTROL_CURRENT_ED),
		 OREAD4(sc, OHCI_BULK_HEAD_ED),
		 OREAD4(sc, OHCI_BULK_CURRENT_ED)));
	DPRINTF(("               done=0x%08x fmival=0x%08x fmrem=0x%08x\n",
		 OREAD4(sc, OHCI_DONE_HEAD),
		 OREAD4(sc, OHCI_FM_INTERVAL),
		 OREAD4(sc, OHCI_FM_REMAINING)));
	DPRINTF(("               fmnum=0x%08x perst=0x%08x lsthrs=0x%08x\n",
		 OREAD4(sc, OHCI_FM_NUMBER),
		 OREAD4(sc, OHCI_PERIODIC_START),
		 OREAD4(sc, OHCI_LS_THRESHOLD)));
	DPRINTF(("               desca=0x%08x descb=0x%08x stat=0x%08x\n",
		 OREAD4(sc, OHCI_RH_DESCRIPTOR_A),
		 OREAD4(sc, OHCI_RH_DESCRIPTOR_B),
		 OREAD4(sc, OHCI_RH_STATUS)));
	DPRINTF(("               port1=0x%08x port2=0x%08x\n",
		 OREAD4(sc, OHCI_RH_PORT_STATUS(1)),
		 OREAD4(sc, OHCI_RH_PORT_STATUS(2))));
	DPRINTF(("         HCCA: frame_number=0x%04x done_head=0x%08x\n",
		 O32TOH(sc->sc_hcca->hcca_frame_number),
		 O32TOH(sc->sc_hcca->hcca_done_head)));
}
#endif

Static int ohci_intr1(ohci_softc_t *);

#if defined(__NetBSD__) || defined(__OpenBSD__)
int
#elif defined(__FreeBSD__)
void
#endif
ohci_intr(void *p)
{
	ohci_softc_t *sc = p;

	if (sc == NULL || sc->sc_dying)
#if defined(__NetBSD__) || defined(__OpenBSD__)
		return (0);
#elif defined(__FreeBSD__)
		return;
#endif

	/* If we get an interrupt while polling, then just ignore it. */
	if (sc->sc_bus.use_polling) {
#ifdef DIAGNOSTIC
		DPRINTFN(16, ("ohci_intr: ignored interrupt while polling\n"));
#endif
		/* for level triggered intrs, should do something to ack */
		OWRITE4(sc, OHCI_INTERRUPT_STATUS,
			OREAD4(sc, OHCI_INTERRUPT_STATUS));

#if defined(__NetBSD__) || defined(__OpenBSD__)
		return (0);
#elif defined(__FreeBSD__)
		return;
#endif
	}

#if defined(__NetBSD__) || defined(__OpenBSD__)
	return
#endif
	ohci_intr1(sc);
}

Static int
ohci_intr1(ohci_softc_t *sc)
{
	u_int32_t intrs, eintrs;

	DPRINTFN(14,("ohci_intr1: enter\n"));

	/* In case the interrupt occurs before initialization has completed. */
	if (sc == NULL || sc->sc_hcca == NULL) {
#ifdef DIAGNOSTIC
		printf("ohci_intr: sc->sc_hcca == NULL\n");
#endif
		return (0);
	}

	intrs = OREAD4(sc, OHCI_INTERRUPT_STATUS);

	if (intrs == 0)		/* nothing to be done (PCI shared interrupt) */
		return (0);

	/* Acknowledge */
	OWRITE4(sc, OHCI_INTERRUPT_STATUS, intrs & ~(OHCI_MIE|OHCI_WDH));

	eintrs = intrs & sc->sc_eintrs;
	if (!eintrs)
		return (0);

	sc->sc_bus.intr_context++;
	sc->sc_bus.no_intrs++;
	DPRINTFN(7, ("ohci_intr: sc=%p intrs=0x%x(0x%x) eintrs=0x%x\n",
		     sc, (u_int)intrs, OREAD4(sc, OHCI_INTERRUPT_STATUS),
		     (u_int)eintrs));

	if (eintrs & OHCI_SO) {
		sc->sc_overrun_cnt++;
		if (usbd_ratecheck(&sc->sc_overrun_ntc)) {
			printf("%s: %u scheduling overruns\n",
			    USBDEVNAME(sc->sc_bus.bdev), sc->sc_overrun_cnt);
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
		printf("%s: resume detect\n", USBDEVNAME(sc->sc_bus.bdev));
		/* XXX process resume detect */
	}
	if (eintrs & OHCI_UE) {
		printf("%s: unrecoverable error, controller halted\n",
		       USBDEVNAME(sc->sc_bus.bdev));
		OWRITE4(sc, OHCI_CONTROL, OHCI_HCFS_RESET);
		/* XXX what else */
	}
	if (eintrs & OHCI_RHSC) {
		/*
		 * We block the interrupt below, and reenable it later from
		 * a timeout.
		 */
		ohci_rhsc(sc, sc->sc_intrxfer);
		/* Do not allow RHSC interrupts > 1 per second */
		usb_callout(sc->sc_tmo_rhsc, hz, ohci_rhsc_enable, sc);
	}

	sc->sc_bus.intr_context--;

	if (eintrs != 0) {
		/* Block unprocessed interrupts. */
		OWRITE4(sc, OHCI_INTERRUPT_DISABLE, eintrs);
		sc->sc_eintrs &= ~eintrs;
		DPRINTFN(1, ("%s: blocking intrs 0x%x\n",
		    USBDEVNAME(sc->sc_bus.bdev), eintrs));
	}

	return (1);
}

void
ohci_rhsc_enable(void *v_sc)
{
	ohci_softc_t *sc = v_sc;
	int s;

	s = splhardusb();
	sc->sc_eintrs |= OHCI_RHSC;
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, OHCI_RHSC);
	splx(s);
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
	"NOT_ACCESSED"
};
#endif

void
ohci_softintr(void *v)
{
	ohci_softc_t *sc = v;
	ohci_soft_itd_t *sitd, *sidone, *sitdnext;
	ohci_soft_td_t  *std,  *sdone,  *stdnext, *p;
	usbd_xfer_handle xfer;
	struct ohci_pipe *opipe;
	int len, cc, s;
	int i, j, iframes;
	ohci_physaddr_t done;

	DPRINTFN(10,("ohci_softintr: enter\n"));

	sc->sc_bus.intr_context++;

	s = splhardusb();
	USB_MEM_SYNC2(&sc->sc_dmatag, &sc->sc_hccadma,
	    offsetof(struct ohci_hcca, hcca_done_head),
	    sizeof(sc->sc_hcca->hcca_done_head),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	done = O32TOH(sc->sc_hcca->hcca_done_head) & ~OHCI_DONE_INTRS;
	sc->sc_hcca->hcca_done_head = 0;
	USB_MEM_SYNC2(&sc->sc_dmatag, &sc->sc_hccadma,
	    offsetof(struct ohci_hcca, hcca_done_head),
	    sizeof(sc->sc_hcca->hcca_done_head),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	OWRITE4(sc, OHCI_INTERRUPT_STATUS, OHCI_WDH);
	sc->sc_eintrs |= OHCI_WDH;
	OWRITE4(sc, OHCI_INTERRUPT_ENABLE, OHCI_WDH);
	splx(s);

	/* Reverse the done list. */
	for (sdone = NULL, sidone = NULL; done != 0; ) {
		std = ohci_find_td(sc, done);
		if (std != NULL) {
			OHCI_STD_SYNC(sc, std,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			std->dnext = sdone;
			done = O32TOH(std->td.td_nexttd);
			sdone = std;
			DPRINTFN(10,("add TD %p\n", std));
			continue;
		}
		sitd = ohci_find_itd(sc, done);
		if (sitd != NULL) {
			OHCI_SITD_SYNC(sc, sitd,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			sitd->dnext = sidone;
			done = O32TOH(sitd->itd.itd_nextitd);
			sidone = sitd;
			DPRINTFN(5,("add ITD %p\n", sitd));
			continue;
		}
		panic("ohci_softintr: addr 0x%08lx not found", (u_long)done);
	}

	DPRINTFN(10,("ohci_softintr: sdone=%p sidone=%p\n", sdone, sidone));

#ifdef OHCI_DEBUG
	if (ohcidebug > 10) {
		DPRINTF(("ohci_process_done: TD done:\n"));
		ohci_dump_tds(sc, sdone);
	}
#endif

	for (std = sdone; std; std = stdnext) {
		xfer = std->xfer;
		stdnext = std->dnext;
		DPRINTFN(10, ("ohci_process_done: std=%p xfer=%p hcpriv=%p\n",
				std, xfer, (xfer ? xfer->hcpriv : NULL)));
		if (xfer == NULL) {
			/*
			 * xfer == NULL: There seems to be no xfer associated
			 * with this TD. It is tailp that happened to end up on
			 * the done queue.
			 */
			continue;
		}
		if (xfer->status == USBD_CANCELLED ||
		    xfer->status == USBD_TIMEOUT) {
			DPRINTF(("ohci_process_done: cancel/timeout %p\n",
				 xfer));
			/* Handled by abort routine. */
			continue;
		}

		len = std->len;
		if (std->td.td_cbp != 0)
			len -= O32TOH(std->td.td_be) -
			       O32TOH(std->td.td_cbp) + 1;
		DPRINTFN(10, ("ohci_process_done: len=%d, flags=0x%x\n", len,
		    std->flags));
		if (std->flags & OHCI_ADD_LEN)
			xfer->actlen += len;

		cc = OHCI_TD_GET_CC(O32TOH(std->td.td_flags));
		if (cc != OHCI_CC_NO_ERROR) {
			/*
			 * Endpoint is halted.  First unlink all the TDs
			 * belonging to the failed transfer, and then restart
			 * the endpoint.
			 */
			opipe = (struct ohci_pipe *)xfer->pipe;

			DPRINTFN(15,("ohci_process_done: error cc=%d (%s)\n",
			  OHCI_TD_GET_CC(O32TOH(std->td.td_flags)),
			  ohci_cc_strs[OHCI_TD_GET_CC(O32TOH(std->td.td_flags))]));
			usb_uncallout(xfer->timeout_handle, ohci_timeout, xfer);
			usb_rem_task(OXFER(xfer)->xfer.pipe->device,
			    &OXFER(xfer)->abort_task);

			/* Remove all this xfer's TDs from the done queue. */
			for (p = std; p->dnext != NULL; p = p->dnext) {
				if (p->dnext->xfer != xfer)
					continue;
				p->dnext = p->dnext->dnext;
			}
			/* The next TD may have been removed. */
			stdnext = std->dnext;

			if (cc == OHCI_CC_STALL)
				xfer->status = USBD_STALLED;
			else
				xfer->status = USBD_IOERROR;
			ohci_transfer_complete(xfer, 1);
			continue;
		}
		/*
		 * Skip intermediate TDs. They remain linked from
		 * xfer->hcpriv and we free them when the transfer completes.
		 */
		if ((std->flags & OHCI_CALL_DONE) == 0)
			continue;

		/* Normal transfer completion */
		usb_uncallout(xfer->timeout_handle, ohci_timeout, xfer);
		usb_rem_task(OXFER(xfer)->xfer.pipe->device,
		    &OXFER(xfer)->abort_task);
		xfer->status = USBD_NORMAL_COMPLETION;
		ohci_transfer_complete(xfer, 0);
	}

#ifdef OHCI_DEBUG
	if (ohcidebug > 10) {
		DPRINTF(("ohci_softintr: ITD done:\n"));
		ohci_dump_itds(sc, sidone);
	}
#endif

	for (sitd = sidone; sitd != NULL; sitd = sitdnext) {
		xfer = sitd->xfer;
		sitdnext = sitd->dnext;
		sitd->flags |= OHCI_ITD_INTFIN;
		DPRINTFN(1, ("ohci_process_done: sitd=%p xfer=%p hcpriv=%p\n",
			     sitd, xfer, xfer ? xfer->hcpriv : 0));
		if (xfer == NULL)
			continue;
		if (xfer->status == USBD_CANCELLED ||
		    xfer->status == USBD_TIMEOUT) {
			DPRINTF(("ohci_process_done: cancel/timeout %p\n",
				 xfer));
			/* Handled by abort routine. */
			continue;
		}
		if (xfer->pipe)
			if (xfer->pipe->aborting)
				continue; /*Ignore.*/
#ifdef DIAGNOSTIC
		if (sitd->isdone)
			printf("ohci_softintr: sitd=%p is done\n", sitd);
		sitd->isdone = 1;
#endif
		opipe = (struct ohci_pipe *)xfer->pipe;
		if (opipe->aborting)
			continue;

		if (sitd->flags & OHCI_CALL_DONE) {
			ohci_soft_itd_t *next;
			int isread = opipe->pipe.endpoint->edesc->bEndpointAddress & UE_DIR_IN;

			opipe->u.iso.inuse -= xfer->nframes;
			xfer->status = USBD_NORMAL_COMPLETION;
			for (i = 0, sitd = xfer->hcpriv;;sitd = next) {
				next = sitd->nextitd;
				cc = OHCI_ITD_GET_CC(O32TOH(sitd->itd.itd_flags));
				if (cc != OHCI_CC_NO_ERROR) {
					xfer->status = USBD_IOERROR;
				}

				{
					iframes = OHCI_ITD_GET_FC(O32TOH(sitd->itd.itd_flags));
					for (j = 0; j < iframes; i++, j++) {
						len = O16TOH(sitd->itd.itd_offset[j]);
						cc = OHCI_ITD_PSW_GET_CC(len);
						switch (cc &
						    OHCI_CC_NOT_ACCESSED_MASK) {
						case OHCI_CC_NOT_ACCESSED:
							len = 0;
							break;
						case OHCI_CC_BUFFER_OVERRUN:
							/* or BUFFER_UNDERRUN */
							xfer->status = USBD_IOERROR;
							/* FALLTHROUGH */
						default:
							len = OHCI_ITD_PSW_LENGTH(len);
							break;
						}
						if (isread)
							xfer->frlengths[i] = len;
					}
				}
				if (sitd->flags & OHCI_CALL_DONE)
					break;
			}
			if (xfer->status != USBD_NORMAL_COMPLETION) {
				/* resync frame */
				opipe->u.iso.next = -1;
			}
#if 1
			/* XXX? */
			xfer->status = USBD_NORMAL_COMPLETION;
#endif

			ohci_transfer_complete(xfer, 0);
		}
	}

#ifdef USB_USE_SOFTINTR
	if (sc->sc_softwake) {
		sc->sc_softwake = 0;
		wakeup(&sc->sc_softwake);
	}
#endif /* USB_USE_SOFTINTR */

	sc->sc_bus.intr_context--;
	DPRINTFN(10,("ohci_softintr: done:\n"));
}

void
ohci_device_ctrl_done(usbd_xfer_handle xfer)
{
	DPRINTFN(10,("ohci_device_ctrl_done: xfer=%p\n", xfer));

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST)) {
		panic("ohci_device_ctrl_done: not a request");
	}
#endif
}

void
ohci_device_intr_done(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	usbd_status err;

	DPRINTFN(10,("ohci_device_intr_done: xfer=%p, actlen=%d\n",
		     xfer, xfer->actlen));

	if (xfer->pipe->repeat) {
		err = ohci_device_intr_insert(sc, xfer);
		if (err) {
			xfer->status = err;
			return;
		}
	}
}

void
ohci_device_bulk_done(usbd_xfer_handle xfer)
{
	DPRINTFN(10,("ohci_device_bulk_done: xfer=%p, actlen=%d\n",
		     xfer, xfer->actlen));
}

void
ohci_rhsc(ohci_softc_t *sc, usbd_xfer_handle xfer)
{
	usbd_pipe_handle pipe;
	u_char *p;
	int i, m;
	int hstatus;

	hstatus = OREAD4(sc, OHCI_RH_STATUS);
	DPRINTF(("ohci_rhsc: sc=%p xfer=%p hstatus=0x%08x\n",
		 sc, xfer, hstatus));

	if (xfer == NULL) {
		/* Just ignore the change. */
		return;
	}

	pipe = xfer->pipe;

	p = xfer->hcbuffer;
	m = min(sc->sc_noport, xfer->length * 8 - 1);
	memset(p, 0, xfer->length);
	for (i = 1; i <= m; i++) {
		/* Pick out CHANGE bits from the status reg. */
		if (OREAD4(sc, OHCI_RH_PORT_STATUS(i)) >> 16)
			p[i/8] |= 1 << (i%8);
	}
	DPRINTF(("ohci_rhsc: change=0x%02x\n", *p));
	xfer->actlen = xfer->length;
	xfer->status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);
}

void
ohci_root_intr_done(usbd_xfer_handle xfer)
{
}

void
ohci_root_ctrl_done(usbd_xfer_handle xfer)
{
}

/*
 * Wait here until controller claims to have an interrupt.
 * Then call ohci_intr and return.  Use timeout to avoid waiting
 * too long.
 */
void
ohci_waitintr(ohci_softc_t *sc, usbd_xfer_handle xfer)
{
	int timo;
	u_int32_t intrs;

	xfer->status = USBD_IN_PROGRESS;
	for (timo = xfer->timeout; timo >= 0; timo--) {
		usb_delay_ms(&sc->sc_bus, 1);
		if (sc->sc_dying)
			break;
		intrs = OREAD4(sc, OHCI_INTERRUPT_STATUS) & sc->sc_eintrs;
		DPRINTFN(15,("ohci_waitintr: 0x%04x\n", intrs));
#ifdef OHCI_DEBUG
		if (ohcidebug > 15)
			ohci_dumpregs(sc);
#endif
		if (intrs) {
			ohci_intr1(sc);
			if (xfer->status != USBD_IN_PROGRESS)
				return;
		}
	}

	/* Timeout */
	DPRINTF(("ohci_waitintr: timeout\n"));
	xfer->status = USBD_TIMEOUT;
	ohci_transfer_complete(xfer, 0);
}

void
ohci_poll(struct usbd_bus *bus)
{
	ohci_softc_t *sc = (ohci_softc_t *)bus;
#ifdef OHCI_DEBUG
	static int last;
	int new;
	new = OREAD4(sc, OHCI_INTERRUPT_STATUS);
	if (new != last) {
		DPRINTFN(10,("ohci_poll: intrs=0x%04x\n", new));
		last = new;
	}
#endif

	if (OREAD4(sc, OHCI_INTERRUPT_STATUS) & sc->sc_eintrs)
		ohci_intr1(sc);
}

usbd_status
ohci_device_request(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	usb_device_request_t *req = &xfer->request;
	usbd_device_handle dev = opipe->pipe.device;
	ohci_softc_t *sc = (ohci_softc_t *)dev->bus;
	ohci_soft_td_t *setup, *stat, *next, *tail;
	ohci_soft_ed_t *sed;
	int isread;
	int len;
	usbd_status err;
	int s;
	ohci_physaddr_t dmaadr;

	isread = req->bmRequestType & UT_READ;
	len = UGETW(req->wLength);

	DPRINTFN(3,("ohci_device_request: type=0x%02x, request=0x%02x, "
		    "wValue=0x%04x, wIndex=0x%04x len=%d, addr=%d, endpt=%d\n",
		    req->bmRequestType, req->bRequest, UGETW(req->wValue),
		    UGETW(req->wIndex), len, dev->address,
		    opipe->pipe.endpoint->edesc->bEndpointAddress));

	setup = opipe->tail.td;
	stat = ohci_alloc_std(sc);
	if (stat == NULL) {
		err = USBD_NOMEM;
		goto bad1;
	}
	tail = ohci_alloc_std(sc);
	if (tail == NULL) {
		err = USBD_NOMEM;
		goto bad2;
	}
	tail->xfer = NULL;

	sed = opipe->sed;
	opipe->u.ctl.length = len;
	next = stat;

	/* Set up data transaction */
	if (len != 0) {
		ohci_soft_td_t *std = stat;

		err = ohci_alloc_std_chain(opipe, sc, len, isread, xfer,
			  std, &stat);
		if (err)
			goto bad3;
		OHCI_STD_SYNC(sc, stat,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		stat = stat->nexttd; /* point at free TD */
		/* Start toggle at 1 and then use the carried toggle. */
		std->td.td_flags &= HTOO32(~OHCI_TD_TOGGLE_MASK);
		std->td.td_flags |= HTOO32(OHCI_TD_TOGGLE_1);
	}

	memcpy(KERNADDR(&opipe->u.ctl.reqdma, 0), req, sizeof *req);

	setup->td.td_flags = HTOO32(OHCI_TD_SETUP | OHCI_TD_NOCC |
				     OHCI_TD_TOGGLE_0 | OHCI_TD_SET_DI(6));
	setup->td.td_cbp = HTOO32(DMAADDR(&opipe->u.ctl.reqdma, 0));
	setup->nexttd = next;
	dmaadr = OHCI_STD_DMAADDR(next);
	setup->td.td_nexttd = HTOO32(dmaadr);
	setup->td.td_be = HTOO32(O32TOH(setup->td.td_cbp) + sizeof *req - 1);
	setup->len = 0;
	setup->xfer = xfer;
	setup->flags = 0;
	xfer->hcpriv = setup;
	OHCI_STD_SYNC(sc, setup, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	stat->td.td_flags = HTOO32(
		(isread ? OHCI_TD_OUT : OHCI_TD_IN) |
		OHCI_TD_NOCC | OHCI_TD_TOGGLE_1 | OHCI_TD_SET_DI(1));
	stat->td.td_cbp = 0;
	stat->nexttd = tail;
	dmaadr = OHCI_STD_DMAADDR(tail);
	stat->td.td_nexttd = HTOO32(dmaadr);
	stat->td.td_be = 0;
	stat->flags = OHCI_CALL_DONE;
	stat->len = 0;
	stat->xfer = xfer;
	OHCI_STD_SYNC(sc, stat, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

#ifdef OHCI_DEBUG
	if (ohcidebug > 5) {
		DPRINTF(("ohci_device_request:\n"));
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, setup);
	}
#endif

	/* Insert ED in schedule */
	s = splusb();
	dmaadr = OHCI_STD_DMAADDR(tail);
	sed->ed.ed_tailp = HTOO32(dmaadr);
	OHCI_SED_SYNC(sc, sed, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	opipe->tail.td = tail;
	OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_CLF);
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		usb_callout(xfer->timeout_handle, MS_TO_TICKS(xfer->timeout),
			    ohci_timeout, xfer);
	}
	splx(s);

#ifdef OHCI_DEBUG
	if (ohcidebug > 20) {
		delay(10000);
		DPRINTF(("ohci_device_request: status=%x\n",
			 OREAD4(sc, OHCI_COMMAND_STATUS)));
		ohci_dumpregs(sc);
		printf("ctrl head:\n");
		ohci_dump_ed(sc, sc->sc_ctrl_head);
		printf("sed:\n");
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, setup);
	}
#endif

	return (USBD_NORMAL_COMPLETION);

 bad3:
	ohci_free_std(sc, tail);
 bad2:
	ohci_free_std(sc, stat);
 bad1:
	return (err);
}

/*
 * Add an ED to the schedule.  Called at splusb().
 */
void
ohci_add_ed(ohci_softc_t *sc, ohci_soft_ed_t *sed, ohci_soft_ed_t *head)
{
	ohci_physaddr_t dmaadr;

	DPRINTFN(8,("ohci_add_ed: sed=%p head=%p\n", sed, head));

	SPLUSBCHECK;
	sed->next = head->next;
	sed->ed.ed_nexted = head->ed.ed_nexted;
	OHCI_SED_SYNC2(sc, sed,
	    offsetof(ohci_ed_t, ed_nexted), sizeof(ohci_physaddr_t),
	    BUS_DMASYNC_PREWRITE);
	head->next = sed;
	dmaadr = OHCI_SED_DMAADDR(sed);
	head->ed.ed_nexted = HTOO32(dmaadr);
	OHCI_SED_SYNC2(sc, head,
	    offsetof(ohci_ed_t, ed_nexted), sizeof(ohci_physaddr_t),
	    BUS_DMASYNC_PREWRITE);
}

/*
 * Remove an ED from the schedule.  Called at splusb().
 */
void
ohci_rem_ed(ohci_softc_t *sc, ohci_soft_ed_t *sed, ohci_soft_ed_t *head)
{
	ohci_soft_ed_t *p;

	SPLUSBCHECK;

	/* XXX */
	for (p = head; p != NULL && p->next != sed; p = p->next)
		;
	if (p == NULL)
		panic("ohci_rem_ed: ED not found");
	p->next = sed->next;
	p->ed.ed_nexted = sed->ed.ed_nexted;
	OHCI_SED_SYNC2(sc, p,
	    offsetof(ohci_ed_t, ed_nexted), sizeof(ohci_physaddr_t),
	    BUS_DMASYNC_PREWRITE);
}

/*
 * When a transfer is completed the TD is added to the done queue by
 * the host controller.  This queue is the processed by software.
 * Unfortunately the queue contains the physical address of the TD
 * and we have no simple way to translate this back to a kernel address.
 * To make the translation possible (and fast) we use the chunk of
 * TDs used for allocation.
 */

ohci_soft_td_t *
ohci_find_td(ohci_softc_t *sc, ohci_physaddr_t a)
{
	struct ohci_mem_desc *om;
	ohci_soft_td_t *std;
	size_t off;

	/* if these are present they should be masked out at an earlier
	 * stage.
	 */
	USB_KASSERT2((a&~OHCI_HEADMASK) == 0, ("%s: 0x%b has lower bits set\n",
				      USBDEVNAME(sc->sc_bus.bdev),
				      (int) a, "\20\1HALT\2TOGGLE"));

	SIMPLEQ_FOREACH(om, &sc->sc_std_chunks, om_next) {
		if (a >= om->om_topdma &&
		    a < om->om_topdma + OHCI_STD_SIZE * OHCI_STD_CHUNK) {
			off = a - om->om_topdma;
			if (off % OHCI_STD_SIZE) {
#ifdef DIAGNOSTIC
				printf("ohci_find_td: 0x%x bad address\n",
				    (unsigned)a);
#endif
				break;
			}
			std = (ohci_soft_td_t *)(om->om_top + off);
			if (std->flags & OHCI_TD_FREE) {
#ifdef DIAGNOSTIC
				printf("ohci_find_td: 0x%x: free td\n",
				    (unsigned)OHCI_STD_DMAADDR(std));
#endif
				break;
			}
			return std;
		}
	}

	return (NULL);
}

ohci_soft_itd_t *
ohci_find_itd(ohci_softc_t *sc, ohci_physaddr_t a)
{
	struct ohci_mem_desc *om;
	ohci_soft_itd_t *sitd;
	size_t off;

	SIMPLEQ_FOREACH(om, &sc->sc_sitd_chunks, om_next) {
		if (a >= om->om_topdma &&
		    a < om->om_topdma + OHCI_SITD_SIZE * OHCI_SITD_CHUNK) {
			off = a - om->om_topdma;
			if (off % OHCI_SITD_SIZE) {
#ifdef DIAGNOSTIC
				printf("ohci_find_itd: 0x%x bad address\n",
				    (unsigned)a);
#endif
				break;
			}
			sitd = (ohci_soft_itd_t *)(om->om_top + off);
			if (sitd->flags & OHCI_ITD_FREE) {
#ifdef DIAGNOSTIC
				printf("ohci_find_itd: 0x%x: free itd\n",
				    (unsigned)OHCI_SITD_DMAADDR(sitd));
#endif
				break;
			}
			return sitd;
		}
	}
	return (NULL);
}

void
ohci_timeout(void *addr)
{
	struct ohci_xfer *oxfer = addr;
	struct ohci_pipe *opipe = (struct ohci_pipe *)oxfer->xfer.pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;

	DPRINTF(("ohci_timeout: oxfer=%p\n", oxfer));

	if (sc->sc_dying) {
		ohci_abort_xfer(&oxfer->xfer, USBD_TIMEOUT);
		return;
	}

	/* Execute the abort in a process context. */
	usb_add_task(oxfer->xfer.pipe->device, &oxfer->abort_task,
	    USB_TASKQ_HC);
}

void
ohci_timeout_task(void *addr)
{
	usbd_xfer_handle xfer = addr;
	int s;

	DPRINTF(("ohci_timeout_task: xfer=%p\n", xfer));

	s = splusb();
	ohci_abort_xfer(xfer, USBD_TIMEOUT);
	splx(s);
}

#ifdef OHCI_DEBUG
void
ohci_dump_tds(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	for (; std; std = std->nexttd)
		ohci_dump_td(sc, std);
}

void
ohci_dump_td(ohci_softc_t *sc, ohci_soft_td_t *std)
{
	char sbuf[128];

	bitmask_snprintf((u_int32_t)O32TOH(std->td.td_flags),
			 "\20\23R\24OUT\25IN\31TOG1\32SETTOGGLE",
			 sbuf, sizeof(sbuf));

	printf("TD(%p) at %08lx: %s delay=%d ec=%d cc=%d\ncbp=0x%08lx "
	       "nexttd=0x%08lx be=0x%08lx\n",
	       std, (u_long)OHCI_STD_DMAADDR(std), sbuf,
	       OHCI_TD_GET_DI(O32TOH(std->td.td_flags)),
	       OHCI_TD_GET_EC(O32TOH(std->td.td_flags)),
	       OHCI_TD_GET_CC(O32TOH(std->td.td_flags)),
	       (u_long)O32TOH(std->td.td_cbp),
	       (u_long)O32TOH(std->td.td_nexttd),
	       (u_long)O32TOH(std->td.td_be));
}

void
ohci_dump_itd(ohci_softc_t *sc, ohci_soft_itd_t *sitd)
{
	int i;

	printf("ITD(%p) at %08lx: sf=%d di=%d fc=%d cc=%d\n"
	       "bp0=0x%08lx next=0x%08lx be=0x%08lx\n",
	       sitd, (u_long)OHCI_SITD_DMAADDR(sitd),
	       OHCI_ITD_GET_SF(O32TOH(sitd->itd.itd_flags)),
	       OHCI_ITD_GET_DI(O32TOH(sitd->itd.itd_flags)),
	       OHCI_ITD_GET_FC(O32TOH(sitd->itd.itd_flags)),
	       OHCI_ITD_GET_CC(O32TOH(sitd->itd.itd_flags)),
	       (u_long)O32TOH(sitd->itd.itd_bp0),
	       (u_long)O32TOH(sitd->itd.itd_nextitd),
	       (u_long)O32TOH(sitd->itd.itd_be));
	for (i = 0; i < OHCI_ITD_NOFFSET; i++)
		printf("offs[%d]=0x%04x ", i,
		       (u_int)O16TOH(sitd->itd.itd_offset[i]));
	printf("\n");
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
	char sbuf[128], sbuf2[128];

	bitmask_snprintf((u_int32_t)O32TOH(sed->ed.ed_flags),
			 "\20\14OUT\15IN\16LOWSPEED\17SKIP\20ISO",
			 sbuf, sizeof(sbuf));
	bitmask_snprintf((u_int32_t)O32TOH(sed->ed.ed_headp),
			 "\20\1HALT\2CARRY", sbuf2, sizeof(sbuf2));

	printf("ED(%p) at 0x%08lx: addr=%d endpt=%d maxp=%d flags=%s\ntailp=0x%08lx "
		 "headflags=%s headp=0x%08lx nexted=0x%08lx\n",
		 sed, (u_long)OHCI_SED_DMAADDR(sed),
		 OHCI_ED_GET_FA(O32TOH(sed->ed.ed_flags)),
		 OHCI_ED_GET_EN(O32TOH(sed->ed.ed_flags)),
		 OHCI_ED_GET_MAXP(O32TOH(sed->ed.ed_flags)), sbuf,
		 (u_long)O32TOH(sed->ed.ed_tailp), sbuf2,
		 (u_long)O32TOH(sed->ed.ed_headp),
		 (u_long)O32TOH(sed->ed.ed_nexted));
}
#endif

usbd_status
ohci_open(usbd_pipe_handle pipe)
{
	usbd_device_handle dev = pipe->device;
	ohci_softc_t *sc = (ohci_softc_t *)dev->bus;
	usb_endpoint_descriptor_t *ed = pipe->endpoint->edesc;
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	u_int8_t addr = dev->address;
	u_int8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	ohci_soft_ed_t *sed;
	ohci_soft_td_t *std;
	ohci_soft_itd_t *sitd;
	ohci_physaddr_t tdphys;
	u_int32_t fmt;
	usbd_status err;
	int s;
	int ival;

	DPRINTFN(1, ("ohci_open: pipe=%p, addr=%d, endpt=%d (%d)\n",
		     pipe, addr, ed->bEndpointAddress, sc->sc_addr));

	if (sc->sc_dying)
		return (USBD_IOERROR);

	std = NULL;
	sed = NULL;

	if (addr == sc->sc_addr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->methods = &ohci_root_ctrl_methods;
			break;
		case UE_DIR_IN | OHCI_INTR_ENDPT:
			pipe->methods = &ohci_root_intr_methods;
			break;
		default:
			return (USBD_INVAL);
		}
	} else {
		sed = ohci_alloc_sed(sc);
		if (sed == NULL)
			goto bad0;
		opipe->sed = sed;
		if (xfertype == UE_ISOCHRONOUS) {
			sitd = ohci_alloc_sitd_norsv(sc);
			if (sitd == NULL)
				goto bad1;
			opipe->tail.itd = sitd;
			opipe->aborting = 0;
			tdphys = OHCI_SITD_DMAADDR(sitd);
			fmt = OHCI_ED_FORMAT_ISO;
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
				fmt |= OHCI_ED_DIR_IN;
			else
				fmt |= OHCI_ED_DIR_OUT;
		} else {
			std = ohci_alloc_std_norsv(sc);
			if (std == NULL)
				goto bad1;
			opipe->tail.td = std;
			tdphys = OHCI_STD_DMAADDR(std);
			fmt = OHCI_ED_FORMAT_GEN | OHCI_ED_DIR_TD;
		}
		sed->ed.ed_flags = HTOO32(
			OHCI_ED_SET_FA(addr) |
			OHCI_ED_SET_EN(UE_GET_ADDR(ed->bEndpointAddress)) |
			(dev->speed == USB_SPEED_LOW ? OHCI_ED_SPEED : 0) |
			fmt |
			OHCI_ED_SET_MAXP(UE_MAXPKTSZ(ed)));
		sed->ed.ed_headp = HTOO32(tdphys |
		    (pipe->endpoint->savedtoggle ? OHCI_TOGGLECARRY : 0));
		sed->ed.ed_tailp = HTOO32(tdphys);

		switch (xfertype) {
		case UE_CONTROL:
			pipe->methods = &ohci_device_ctrl_methods;
			err = usb_allocmem(&sc->sc_dmatag,
				  sizeof(usb_device_request_t),
				  0, &opipe->u.ctl.reqdma);
			if (err)
				goto bad;
			s = splusb();
			ohci_add_ed(sc, sed, sc->sc_ctrl_head);
			splx(s);
			break;
		case UE_INTERRUPT:
			pipe->methods = &ohci_device_intr_methods;
			ival = pipe->interval;
			if (ival == USBD_DEFAULT_INTERVAL)
				ival = ed->bInterval;
			return (ohci_device_setintr(sc, opipe, ival));
		case UE_ISOCHRONOUS:
			pipe->methods = &ohci_device_isoc_methods;
			return (ohci_setup_isoc(pipe));
		case UE_BULK:
			pipe->methods = &ohci_device_bulk_methods;
			s = splusb();
			ohci_add_ed(sc, sed, sc->sc_bulk_head);
			splx(s);
			break;
		}
	}
	return (USBD_NORMAL_COMPLETION);

 bad:
	if (std != NULL)
		ohci_free_std(sc, std);
 bad1:
	if (sed != NULL)
		ohci_free_sed(sc, sed);
 bad0:
	return (USBD_NOMEM);

}

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
void
ohci_close_pipe(usbd_pipe_handle pipe, ohci_soft_ed_t *head)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;
	ohci_soft_ed_t *sed = opipe->sed;
	int s;

	s = splusb();
#ifdef DIAGNOSTIC
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
	    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK)) {
		ohci_soft_td_t *std;
		std = ohci_find_td(sc,
		    O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK);
		printf("ohci_close_pipe: pipe not empty sed=%p hd=0x%x "
		       "tl=0x%x pipe=%p, std=%p\n", sed,
		       (int)O32TOH(sed->ed.ed_headp),
		       (int)O32TOH(sed->ed.ed_tailp),
		       pipe, std);
#ifdef USB_DEBUG
		usbd_dump_pipe(&opipe->pipe);
#endif
#ifdef OHCI_DEBUG
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
	splx(s);
	pipe->endpoint->savedtoggle =
	    (O32TOH(sed->ed.ed_headp) & OHCI_TOGGLECARRY) ? 1 : 0;
	ohci_free_sed(sc, sed);
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
 */
void
ohci_abort_xfer(usbd_xfer_handle xfer, usbd_status status)
{
	struct ohci_xfer *oxfer = OXFER(xfer);
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	ohci_soft_ed_t *sed = opipe->sed;
	ohci_soft_td_t *p, *n;
	ohci_physaddr_t headp;
	ohci_physaddr_t dmaadr;
	int s, hit;
	int wake;
	int isread, is_mbuf;

	DPRINTF(("ohci_abort_xfer: xfer=%p pipe=%p sed=%p\n", xfer, opipe,sed));

	if (sc->sc_dying) {
		/* If we're dying, just do the software part. */
		s = splusb();
		xfer->status = status;	/* make software ignore it */
		usb_uncallout(xfer->timeout_handle, ohci_timeout, xfer);
		usb_rem_task(xfer->pipe->device, &oxfer->abort_task);
		ohci_transfer_complete(xfer, 0);
		splx(s);
		return;
	}

	if (xfer->device->bus->intr_context || !curproc)
		panic("ohci_abort_xfer: not in process context");

	/*
	 * If an abort is already in progress then just wait for it to
	 * complete and return.
	 */
	if (oxfer->ohci_xfer_flags & OHCI_XFER_ABORTING) {
		DPRINTFN(2, ("ohci_abort_xfer: already aborting\n"));
		/* No need to wait if we're aborting from a timeout. */
		if (status == USBD_TIMEOUT)
			return;
		/* Override the status which might be USBD_TIMEOUT. */
		xfer->status = status;
		DPRINTFN(2, ("ohci_abort_xfer: waiting for abort to finish\n"));
		oxfer->ohci_xfer_flags |= OHCI_XFER_ABORTWAIT;
		while (oxfer->ohci_xfer_flags & OHCI_XFER_ABORTING)
			tsleep(&oxfer->ohci_xfer_flags, PZERO, "ohciaw", 0);
		return;
	}

	/*
	 * Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	s = splusb();
	oxfer->ohci_xfer_flags |= OHCI_XFER_ABORTING;
	xfer->status = status;	/* make software ignore it */
	usb_uncallout(xfer->timeout_handle, ohci_timeout, xfer);
	usb_rem_task(xfer->pipe->device, &oxfer->abort_task);
	splx(s);
	DPRINTFN(1,("ohci_abort_xfer: stop ed=%p\n", sed));
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP); /* force hardware skip */

	/*
	 * Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.  Also make sure the soft interrupt routine
	 * has run.
	 */
	usb_delay_ms(opipe->pipe.device->bus, 20); /* Hardware finishes in 1ms */
	s = splusb();
#ifdef USB_USE_SOFTINTR
	sc->sc_softwake = 1;
#endif /* USB_USE_SOFTINTR */
	usb_schedsoftintr(&sc->sc_bus);
#ifdef USB_USE_SOFTINTR
	tsleep(&sc->sc_softwake, PZERO, "ohciab", 0);
#endif /* USB_USE_SOFTINTR */
	splx(s);

	/*
	 * Step 3: Remove any vestiges of the xfer from the hardware.
	 * The complication here is that the hardware may have executed
	 * beyond the xfer we're trying to abort.  So as we're scanning
	 * the TDs of this xfer we check if the hardware points to
	 * any of them.
	 */
	s = splusb();		/* XXX why? */
	p = xfer->hcpriv;
#ifdef DIAGNOSTIC
	if (p == NULL) {
		oxfer->ohci_xfer_flags &= ~OHCI_XFER_ABORTING; /* XXX */
		splx(s);
		printf("ohci_abort_xfer: hcpriv is NULL\n");
		return;
	}
#endif
#ifdef OHCI_DEBUG
	if (ohcidebug > 1) {
		DPRINTF(("ohci_abort_xfer: sed=\n"));
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, p);
	}
#endif
	isread = usbd_xfer_isread(xfer);
	is_mbuf = xfer->rqflags & URQ_DEV_MAP_MBUF;
	headp = O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK;
	hit = 0;
	for (; p->xfer == xfer; p = n) {
		hit |= headp == OHCI_STD_DMAADDR(p);
		n = p->nexttd;
	}
	/* Zap headp register if hardware pointed inside the xfer. */
	if (hit) {
		DPRINTFN(1,("ohci_abort_xfer: set hd=0x%08x, tl=0x%08x\n",
			    (int)OHCI_STD_DMAADDR(p), (int)O32TOH(sed->ed.ed_tailp)));
		dmaadr = OHCI_STD_DMAADDR(p);
		sed->ed.ed_headp = HTOO32(dmaadr); /* unlink TDs */
	} else {
		DPRINTFN(1,("ohci_abort_xfer: no hit\n"));
	}

	/*
	 * Step 4: Turn on hardware again.
	 */
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP); /* remove hardware skip */

	/*
	 * Step 5: Execute callback.
	 */
	/* Do the wakeup first to avoid touching the xfer after the callback. */
	wake = oxfer->ohci_xfer_flags & OHCI_XFER_ABORTWAIT;
	oxfer->ohci_xfer_flags &= ~(OHCI_XFER_ABORTING | OHCI_XFER_ABORTWAIT);
	ohci_transfer_complete(xfer, 0);
	if (wake)
		wakeup(&oxfer->ohci_xfer_flags);

	splx(s);
}

/*
 * Data structures and routines to emulate the root hub.
 */
Static usb_device_descriptor_t ohci_devd = {
	USB_DEVICE_DESCRIPTOR_SIZE,
	UDESC_DEVICE,		/* type */
	{0x00, 0x01},		/* USB version */
	UDCLASS_HUB,		/* class */
	UDSUBCLASS_HUB,		/* subclass */
	UDPROTO_FSHUB,		/* protocol */
	64,			/* max packet */
	{0},{0},{0x00,0x01},	/* device id */
	1,2,0,			/* string indicies */
	1			/* # of configurations */
};

Static usb_config_descriptor_t ohci_confd = {
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

Static usb_interface_descriptor_t ohci_ifcd = {
	USB_INTERFACE_DESCRIPTOR_SIZE,
	UDESC_INTERFACE,
	0,
	0,
	1,
	UICLASS_HUB,
	UISUBCLASS_HUB,
	UIPROTO_FSHUB,
	0
};

Static usb_endpoint_descriptor_t ohci_endpd = {
	.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
	.bDescriptorType = UDESC_ENDPOINT,
	.bEndpointAddress = UE_DIR_IN | OHCI_INTR_ENDPT,
	.bmAttributes = UE_INTERRUPT,
	.wMaxPacketSize = {8, 0},			/* max packet */
	.bInterval = 255,
};

Static usb_hub_descriptor_t ohci_hubd = {
	.bDescLength = USB_HUB_DESCRIPTOR_SIZE,
	.bDescriptorType = UDESC_HUB,
};

Static int
ohci_str(usb_string_descriptor_t *p, int l, const char *s)
{
	int i;

	if (l == 0)
		return (0);
	p->bLength = 2 * strlen(s) + 2;
	if (l == 1)
		return (1);
	p->bDescriptorType = UDESC_STRING;
	l -= 2;
	for (i = 0; s[i] && l > 1; i++, l -= 2)
		USETW2(p->bString[i], 0, s[i]);
	return (2*i+2);
}

/*
 * Simulate a hardware hub by handling all the necessary requests.
 */
Static usbd_status
ohci_root_ctrl_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	/* Insert last in queue. */
#if 0	/* root ctrl doesn't do DMA */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);
#else
	err = usb_insert_transfer(xfer);
#endif
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ohci_root_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ohci_root_ctrl_start(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usb_device_request_t *req;
	void *buf = NULL;
	int port, i;
	int s, len, value, index, l, totlen = 0;
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

	DPRINTFN(4,("ohci_root_ctrl_start: type=0x%02x request=%02x\n",
		    req->bmRequestType, req->bRequest));

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	if (len != 0) {
		/* mbuf transfer is not supported */
		if (xfer->rqflags & URQ_DEV_MAP_MBUF)
			return (USBD_INVAL);
		buf = xfer->hcbuffer;
	}

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
		DPRINTFN(8,("ohci_root_ctrl_control wValue=0x%04x\n", value));
		if (len == 0)
			break;
		switch(value >> 8) {
		case UDESC_DEVICE:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			USETW(ohci_devd.idVendor, sc->sc_id_vendor);
			memcpy(buf, &ohci_devd, l);
			break;
		case UDESC_CONFIG:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_CONFIG_DESCRIPTOR_SIZE);
			memcpy(buf, &ohci_confd, l);
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_INTERFACE_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &ohci_ifcd, l);
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_ENDPOINT_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &ohci_endpd, l);
			break;
		case UDESC_STRING:
			*(u_int8_t *)buf = 0;
			totlen = 1;
			switch (value & 0xff) {
			case 0: /* Language table */
				if (len > 0)
					*(u_int8_t *)buf = 4;
				if (len >=  4) {
		USETW(((usb_string_descriptor_t *)buf)->bString[0], 0x0409);
					totlen = 4;
				}
				break;
			case 1: /* Vendor */
				totlen = ohci_str(buf, len, sc->sc_vendor);
				break;
			case 2: /* Product */
				totlen = ohci_str(buf, len, "OHCI root hub");
				break;
			}
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
		DPRINTFN(8, ("ohci_root_ctrl_start: UR_CLEAR_PORT_FEATURE "
			     "port=%d feature=%d\n",
			     index, value));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
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
			err = USBD_IOERROR;
			goto ret;
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
			err = USBD_IOERROR;
			goto ret;
		}
		v = OREAD4(sc, OHCI_RH_DESCRIPTOR_A);
		hubd = ohci_hubd;
		hubd.bNbrPorts = sc->sc_noport;
		USETW(hubd.wHubCharacteristics,
		      (v & OHCI_NPS ? UHD_PWR_NO_SWITCH :
		       v & OHCI_PSM ? UHD_PWR_GANGED : UHD_PWR_INDIVIDUAL)
		      /* XXX overcurrent */
		      );
		hubd.bPwrOn2PwrGood = OHCI_GET_POTPGT(v);
		v = OREAD4(sc, OHCI_RH_DESCRIPTOR_B);
		for (i = 0, l = sc->sc_noport; l > 0; i++, l -= 8, v >>= 8)
			hubd.DeviceRemovable[i++] = (u_int8_t)v;
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
		DPRINTFN(8,("ohci_root_ctrl_transfer: get port status i=%d\n",
			    index));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		if (len != 4) {
			err = USBD_IOERROR;
			goto ret;
		}
		v = OREAD4(sc, OHCI_RH_PORT_STATUS(index));
		DPRINTFN(8,("ohci_root_ctrl_transfer: port status=0x%04x\n",
			    v));
		USETW(ps.wPortStatus, v);
		USETW(ps.wPortChange, v >> 16);
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
		port = OHCI_RH_PORT_STATUS(index);
		switch(value) {
		case UHF_PORT_ENABLE:
			OWRITE4(sc, port, UPS_PORT_ENABLED);
			break;
		case UHF_PORT_SUSPEND:
			OWRITE4(sc, port, UPS_SUSPEND);
			break;
		case UHF_PORT_RESET:
			DPRINTFN(5,("ohci_root_ctrl_transfer: reset port %d\n",
				    index));
			OWRITE4(sc, port, UPS_RESET);
			for (i = 0; i < 5; i++) {
				usb_delay_ms(&sc->sc_bus,
					     USB_PORT_ROOT_RESET_DELAY);
				if (sc->sc_dying) {
					err = USBD_IOERROR;
					goto ret;
				}
				if ((OREAD4(sc, port) & UPS_RESET) == 0)
					break;
			}
			DPRINTFN(8,("ohci port %d reset, status = 0x%04x\n",
				    index, OREAD4(sc, port)));
			break;
		case UHF_PORT_POWER:
			DPRINTFN(2,("ohci_root_ctrl_transfer: set port power "
				    "%d\n", index));
			OWRITE4(sc, port, UPS_PORT_POWER);
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
		break;
	default:
		err = USBD_IOERROR;
		goto ret;
	}
	xfer->actlen = totlen;
	err = USBD_NORMAL_COMPLETION;
 ret:
	xfer->status = err;
#if 0	/* root ctrl doesn't do DMA */
	ohci_transfer_complete(xfer, 0);
#else
	s = splusb();
	usb_transfer_complete(xfer);
	splx(s);
#endif
	return (USBD_IN_PROGRESS);
}

/* Abort a root control request. */
Static void
ohci_root_ctrl_abort(usbd_xfer_handle xfer)
{
	/* Nothing to do, all transfers are synchronous. */
}

/* Close the root pipe. */
Static void
ohci_root_ctrl_close(usbd_pipe_handle pipe)
{
	DPRINTF(("ohci_root_ctrl_close\n"));
	/* Nothing to do. */
}

Static usbd_status
ohci_root_intr_transfer(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	/* Insert last in queue. */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ohci_root_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ohci_root_intr_start(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;

	if (sc->sc_dying)
		return (USBD_IOERROR);
	if (xfer->rqflags & URQ_DEV_MAP_MBUF)
		return (USBD_INVAL);	/* mbuf transfer is not supported */

	sc->sc_intrxfer = xfer;

	return (USBD_IN_PROGRESS);
}

/* Abort a root interrupt request. */
Static void
ohci_root_intr_abort(usbd_xfer_handle xfer)
{
	usbd_pipe_handle pipe = xfer->pipe;

	if (pipe->intrxfer == xfer) {
		DPRINTF(("ohci_root_intr_abort: remove\n"));
		pipe->intrxfer = NULL;
	}
	xfer->status = USBD_CANCELLED;
	ohci_transfer_complete(xfer, 0);
}

/* Close the root pipe. */
Static void
ohci_root_intr_close(usbd_pipe_handle pipe)
{
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;

	DPRINTF(("ohci_root_intr_close\n"));

	sc->sc_intrxfer = NULL;
}

/************************/

Static usbd_status
ohci_device_ctrl_transfer(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	/* Insert last in queue. */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ohci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ohci_device_ctrl_start(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST)) {
		/* XXX panic */
		printf("ohci_device_ctrl_transfer: not a request\n");
		return (USBD_INVAL);
	}
#endif

	err = ohci_device_request(xfer);
	if (err)
		return (err);

	if (sc->sc_bus.use_polling)
		ohci_waitintr(sc, xfer);
	return (USBD_IN_PROGRESS);
}

/* Abort a device control request. */
Static void
ohci_device_ctrl_abort(usbd_xfer_handle xfer)
{
	DPRINTF(("ohci_device_ctrl_abort: xfer=%p\n", xfer));
	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/* Close a device control pipe. */
Static void
ohci_device_ctrl_close(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;

	DPRINTF(("ohci_device_ctrl_close: pipe=%p\n", pipe));
	ohci_close_pipe(pipe, sc->sc_ctrl_head);
	ohci_free_std_norsv(sc, opipe->tail.td);
	usb_freemem(&sc->sc_dmatag, &opipe->u.ctl.reqdma);
}

/************************/

Static void
ohci_device_clear_toggle(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;

	opipe->sed->ed.ed_headp &= HTOO32(~OHCI_TOGGLECARRY);
}

Static void
ohci_noop(usbd_pipe_handle pipe)
{
}

Static usbd_status
ohci_device_bulk_transfer(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	/* Insert last in queue. */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ohci_device_bulk_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ohci_device_bulk_start(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	usbd_device_handle dev = opipe->pipe.device;
	ohci_softc_t *sc = (ohci_softc_t *)dev->bus;
	int addr = dev->address;
	ohci_soft_td_t *data, *tail, *tdp;
	ohci_soft_ed_t *sed;
	ohci_physaddr_t dmaadr;
	int s, len, isread, endpt;
	usbd_status err;

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (xfer->rqflags & URQ_REQUEST) {
		/* XXX panic */
		printf("ohci_device_bulk_start: a request\n");
		return (USBD_INVAL);
	}
#endif

	len = xfer->length;
	endpt = xfer->pipe->endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	sed = opipe->sed;

	DPRINTFN(4,("ohci_device_bulk_start: xfer=%p len=%d isread=%d "
		    "flags=%d endpt=%d\n", xfer, len, isread, xfer->flags,
		    endpt));

	opipe->u.bulk.isread = isread;
	opipe->u.bulk.length = len;

	/* Update device address */
	sed->ed.ed_flags = HTOO32(
		(O32TOH(sed->ed.ed_flags) & ~OHCI_ED_ADDRMASK) |
		OHCI_ED_SET_FA(addr));

	/* Allocate a chain of new TDs (including a new tail). */
	data = opipe->tail.td;
	err = ohci_alloc_std_chain(opipe, sc, len, isread, xfer,
		  data, &tail);
	/* We want interrupt at the end of the transfer. */
	tail->td.td_flags &= HTOO32(~OHCI_TD_INTR_MASK);
	tail->td.td_flags |= HTOO32(OHCI_TD_SET_DI(1));
	tail->flags |= OHCI_CALL_DONE;
	OHCI_STD_SYNC(sc, tail, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	tail = tail->nexttd;	/* point at sentinel */
	if (err)
		return (err);

	tail->xfer = NULL;
	xfer->hcpriv = data;

	DPRINTFN(4,("ohci_device_bulk_start: ed_flags=0x%08x td_flags=0x%08x "
		    "td_cbp=0x%08x td_be=0x%08x\n",
		    (int)O32TOH(sed->ed.ed_flags),
		    (int)O32TOH(data->td.td_flags),
		    (int)O32TOH(data->td.td_cbp),
		    (int)O32TOH(data->td.td_be)));

#ifdef OHCI_DEBUG
	if (ohcidebug > 5) {
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, data);
	}
#endif

	/* Insert ED in schedule */
	s = splusb();
	for (tdp = data; tdp != tail; tdp = tdp->nexttd) {
		tdp->xfer = xfer;
	}
	dmaadr = OHCI_STD_DMAADDR(tail);
	sed->ed.ed_tailp = HTOO32(dmaadr);
	opipe->tail.td = tail;
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
	OHCI_SED_SYNC(sc, sed, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	OWRITE4(sc, OHCI_COMMAND_STATUS, OHCI_BLF);
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		usb_callout(xfer->timeout_handle, MS_TO_TICKS(xfer->timeout),
			    ohci_timeout, xfer);
	}

#if 0
/* This goes wrong if we are too slow. */
	if (ohcidebug > 10) {
		delay(10000);
		DPRINTF(("ohci_device_intr_transfer: status=%x\n",
			 OREAD4(sc, OHCI_COMMAND_STATUS)));
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, data);
	}
#endif

	splx(s);

	if (sc->sc_bus.use_polling)
		ohci_waitintr(sc, xfer);

	return (USBD_IN_PROGRESS);
}

Static void
ohci_device_bulk_abort(usbd_xfer_handle xfer)
{
	DPRINTF(("ohci_device_bulk_abort: xfer=%p\n", xfer));
	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/*
 * Close a device bulk pipe.
 */
Static void
ohci_device_bulk_close(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;

	DPRINTF(("ohci_device_bulk_close: pipe=%p\n", pipe));
	ohci_close_pipe(pipe, sc->sc_bulk_head);
	ohci_free_std_norsv(sc, opipe->tail.td);
}

/************************/

Static usbd_status
ohci_device_intr_transfer(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	/* Insert last in queue. */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (ohci_device_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

Static usbd_status
ohci_device_intr_start(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	ohci_soft_ed_t *sed = opipe->sed;
	usbd_status err;

	if (sc->sc_dying)
		return (USBD_IOERROR);

	DPRINTFN(3, ("ohci_device_intr_start: xfer=%p len=%d "
		     "flags=%d priv=%p\n",
		     xfer, xfer->length, xfer->flags, xfer->priv));

#ifdef DIAGNOSTIC
	if (xfer->rqflags & URQ_REQUEST)
		panic("ohci_device_intr_start: a request");
#endif

	err = ohci_device_intr_insert(sc, xfer);
	if (err)
		return (err);

	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);

	return (USBD_IN_PROGRESS);
}

/*
 * Insert an interrupt transfer into an endpoint descriptor list
 */
Static usbd_status
ohci_device_intr_insert(ohci_softc_t *sc, usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_soft_ed_t *sed = opipe->sed;
	ohci_soft_td_t *data, *tail;
	ohci_physaddr_t dataphys, physend;
	ohci_physaddr_t dmaadr;
	int s, isread, endpt;
	struct usb_buffer_dma *ub = &OXFER(xfer)->dmabuf;
	bus_dma_segment_t *segs = USB_BUFFER_SEGS(ub);
	int nsegs = USB_BUFFER_NSEGS(ub);

	DPRINTFN(4, ("ohci_device_intr_insert: xfer=%p", xfer));

	endpt = xfer->pipe->endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;

	data = opipe->tail.td;
	tail = ohci_alloc_std(sc);
	if (tail == NULL)
		return (USBD_NOMEM);
	tail->xfer = NULL;

	data->td.td_flags = HTOO32(
		isread ? OHCI_TD_IN : OHCI_TD_OUT |
		OHCI_TD_NOCC |
		OHCI_TD_SET_DI(1) | OHCI_TD_TOGGLE_CARRY);
	if (xfer->flags & USBD_SHORT_XFER_OK)
		data->td.td_flags |= HTOO32(OHCI_TD_R);
	/*
	 * Assume a short mapping with no complications, which
	 * should always be true for <= 4k buffers in contiguous
	 * virtual memory. The data can take the following forms:
	 *	1 segment in 1 OHCI page
	 *	1 segment in 2 OHCI pages
	 *	2 segments in 2 OHCI pages
	 * (see comment in ohci_alloc_std_chain() for details)
	 */
	USB_KASSERT2(xfer->length > 0 && xfer->length <= OHCI_PAGE_SIZE,
	    ("ohci_device_intr_insert: bad length %d", xfer->length));
	dataphys = segs[0].ds_addr;
	physend = dataphys + xfer->length - 1;
	if (nsegs == 2) {
		USB_KASSERT2(OHCI_PAGE_OFFSET(dataphys +
		    segs[0].ds_len) == 0,
		    ("ohci_device_intr_insert: bad seg 0 termination"));
		physend = segs[1].ds_addr + xfer->length -
		    segs[0].ds_len - 1;
	} else {
		USB_KASSERT2(nsegs == 1,
		    ("ohci_device_intr_insert: bad seg count %d",
		    (u_int)nsegs));
	}
	data->td.td_cbp = HTOO32(dataphys);
	data->nexttd = tail;
	dmaadr = OHCI_STD_DMAADDR(tail);
	data->td.td_nexttd = HTOO32(dmaadr);
	data->td.td_be = HTOO32(physend);
	data->len = xfer->length;
	data->xfer = xfer;
	data->flags = OHCI_CALL_DONE | OHCI_ADD_LEN;
	xfer->hcpriv = data;
	xfer->actlen = 0;
	OHCI_STD_SYNC(sc, data, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

#ifdef OHCI_DEBUG
	if (ohcidebug > 5) {
		DPRINTF(("ohci_device_intr_insert:\n"));
		ohci_dump_ed(sc, sed);
		ohci_dump_tds(sc, data);
	}
#endif

	/* Insert ED in schedule */
	s = splusb();
	dmaadr = OHCI_STD_DMAADDR(tail);
	sed->ed.ed_tailp = HTOO32(dmaadr);
	OHCI_SED_SYNC(sc, sed, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	opipe->tail.td = tail;
	splx(s);

	return (USBD_NORMAL_COMPLETION);
}

/* Abort a device control request. */
Static void
ohci_device_intr_abort(usbd_xfer_handle xfer)
{
	if (xfer->pipe->intrxfer == xfer) {
		DPRINTF(("ohci_device_intr_abort: remove\n"));
		xfer->pipe->intrxfer = NULL;
	}
	ohci_abort_xfer(xfer, USBD_CANCELLED);
}

/* Close a device interrupt pipe. */
Static void
ohci_device_intr_close(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;
	int nslots = opipe->u.intr.nslots;
	int pos = opipe->u.intr.pos;
	int j;
	ohci_soft_ed_t *p, *sed = opipe->sed;
	int s;

	DPRINTFN(1,("ohci_device_intr_close: pipe=%p nslots=%d pos=%d\n",
		    pipe, nslots, pos));
	s = splusb();
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
	if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
	    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK))
		usb_delay_ms(&sc->sc_bus, 2);
#ifdef DIAGNOSTIC
	if ((O32TOH(sed->ed.ed_tailp) & OHCI_HEADMASK) !=
	    (O32TOH(sed->ed.ed_headp) & OHCI_HEADMASK))
		panic("%s: Intr pipe %p still has TDs queued",
			USBDEVNAME(sc->sc_bus.bdev), pipe);
#endif

	for (p = sc->sc_eds[pos]; p && p->next != sed; p = p->next)
		;
#ifdef DIAGNOSTIC
	if (p == NULL)
		panic("ohci_device_intr_close: ED not found");
#endif
	p->next = sed->next;
	p->ed.ed_nexted = sed->ed.ed_nexted;
	splx(s);

	for (j = 0; j < nslots; j++)
		--sc->sc_bws[(pos * nslots + j) % OHCI_NO_INTRS];

	ohci_free_std_norsv(sc, opipe->tail.td);
	ohci_free_sed(sc, opipe->sed);
}

Static usbd_status
ohci_device_setintr(ohci_softc_t *sc, struct ohci_pipe *opipe, int ival)
{
	int i, j, s, best;
	u_int npoll, slow, shigh, nslots;
	u_int bestbw, bw;
	ohci_soft_ed_t *hsed, *sed = opipe->sed;
	ohci_physaddr_t dmaadr;

	DPRINTFN(2, ("ohci_setintr: pipe=%p\n", opipe));
	if (ival == 0) {
		printf("ohci_setintr: 0 interval\n");
		return (USBD_INVAL);
	}

	npoll = OHCI_NO_INTRS;
	while (npoll > ival)
		npoll /= 2;
	DPRINTFN(2, ("ohci_setintr: ival=%d npoll=%d\n", ival, npoll));

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
	DPRINTFN(2, ("ohci_setintr: best=%d(%d..%d) bestbw=%d\n",
		     best, slow, shigh, bestbw));

	s = splusb();
	hsed = sc->sc_eds[best];
	sed->next = hsed->next;
	sed->ed.ed_nexted = hsed->ed.ed_nexted;
	hsed->next = sed;
	dmaadr = OHCI_SED_DMAADDR(sed);
	hsed->ed.ed_nexted = HTOO32(dmaadr);
	splx(s);

	for (j = 0; j < nslots; j++)
		++sc->sc_bws[(best * nslots + j) % OHCI_NO_INTRS];
	opipe->u.intr.nslots = nslots;
	opipe->u.intr.pos = best;

	DPRINTFN(5, ("ohci_setintr: returns %p\n", opipe));
	return (USBD_NORMAL_COMPLETION);
}

/***********************/

usbd_status
ohci_device_isoc_transfer(usbd_xfer_handle xfer)
{
	ohci_softc_t *sc = (ohci_softc_t *)xfer->pipe->device->bus;
	usbd_status err;

	DPRINTFN(5,("ohci_device_isoc_transfer: xfer=%p\n", xfer));

	/* Put it on our queue, */
	err = usb_insert_transfer_dma(xfer, &sc->sc_dmatag,
	    &OXFER(xfer)->dmabuf);

	/* bail out on error, */
	if (err && err != USBD_IN_PROGRESS)
		return (err);

	/* XXX should check inuse here */

	/* insert into schedule, */
	ohci_device_isoc_enter(xfer);

	/* and start if the pipe wasn't running */
	if (!err)
		ohci_device_isoc_start(SIMPLEQ_FIRST(&xfer->pipe->queue));

	return (err);
}

void
ohci_device_isoc_enter(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	usbd_device_handle dev = opipe->pipe.device;
	ohci_softc_t *sc = (ohci_softc_t *)dev->bus;
	ohci_soft_ed_t *sed = opipe->sed;
	struct iso *iso = &opipe->u.iso;
	struct usb_buffer_dma *ub = &OXFER(xfer)->dmabuf;
	bus_dma_segment_t *segs = USB_BUFFER_SEGS(ub);
	int nsegs = USB_BUFFER_NSEGS(ub);
	ohci_soft_itd_t *sitd, *nsitd;
	ohci_physaddr_t dataphys, physend, prevphysend;
	ohci_physaddr_t segdmaadr;
	ohci_physaddr_t dmaadr;
	int i, len, ncur, nframes, seg, seglen, seglen1;
	int useaux, npagecross;
	int s;
#if 1
	union usb_bufptr bufptr;
	int endpt, isread, is_mbuf;
	bus_addr_t auxdma;
#endif
#if 1
	uint32_t sitdo, tailo;
#endif

	DPRINTFN(1,("ohci_device_isoc_enter: used=%d next=%d xfer=%p "
		    "nframes=%d\n",
		    iso->inuse, iso->next, xfer, xfer->nframes));

	if (sc->sc_dying)
		return;

	if (iso->next == -1) {
		/* Not in use yet, schedule it a few frames ahead. */
		USB_MEM_SYNC2(&sc->sc_dmatag, &sc->sc_hccadma,
		    offsetof(struct ohci_hcca, hcca_frame_number),
		    sizeof(sc->sc_hcca->hcca_frame_number),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		iso->next = (uint16_t)(O32TOH(sc->sc_hcca->hcca_frame_number) + 5);
		DPRINTFN(2,("ohci_device_isoc_enter: start next=%d\n",
			    iso->next));

#if 1
		OHCI_SED_SYNC_POST(sc, sed);
		if (sed->ed.ed_headp != sed->ed.ed_tailp) {
			/* set SKIP first */
			sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP);
			OHCI_SED_SYNC2(sc, sed,
			    offsetof(ohci_ed_t, ed_flags), sizeof(u_int32_t),
			    BUS_DMASYNC_PREWRITE);

			/* wait for SKIP to take effect */
			delay(2100);

			OHCI_SED_SYNC_POST(sc, sed);

			tailo = sed->ed.ed_tailp;
			for (sitdo = sed->ed.ed_headp; sitdo != tailo;
			    sitdo = sitd->itd.itd_nextitd) {
				sitd = ohci_find_itd(sc, O32TOH(sitdo));
				*(uint16_t *)&sitd->itd.itd_flags =
				    HTOO16(iso->next);
				iso->next = (uint16_t)(iso->next +
				  OHCI_ITD_GET_FC(O32TOH(sitd->itd.itd_flags)));
				OHCI_SITD_SYNC(sc, sitd,
				    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			}

			/* clear SKIP */
			sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
			OHCI_SED_SYNC2(sc, sed,
			    offsetof(ohci_ed_t, ed_flags), sizeof(u_int32_t),
			    BUS_DMASYNC_PREWRITE);
		}
#endif
	}

	usb_bufptr_init(&bufptr, xfer);

	sitd = opipe->tail.itd;
	nframes = xfer->nframes;
	xfer->hcpriv = sitd;
	endpt = xfer->pipe->endpoint->edesc->bEndpointAddress;
	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	is_mbuf = xfer->rqflags & URQ_DEV_MAP_MBUF;

#ifdef DIAGNOSTIC
	segdmaadr = 0xbbbbbbbb;		/* XXX -Wuninitialized */
	dataphys = 0x77777777;		/* XXX -Wuninitialized */
	physend = 0xeeeeeeee;		/* XXX -Wuninitialized */
	prevphysend = 0x99999999;	/* XXX -Wuninitialized */
#else
	segdmaadr = 0;		/* XXX -Wuninitialized */
	dataphys = 0;		/* XXX -Wuninitialized */
	physend = 0;		/* XXX -Wuninitialized */
	prevphysend = 0;	/* XXX -Wuninitialized */
#endif

	seg = 0;
	seglen = 0;
	useaux = 0;
	i = 0;
	while (i < nframes) {
		ncur = 0;
		npagecross = 0;
		do {
			/*
			 * Fill in as many ITD frames as possible.
			 */

			/* Find the frame start and end physical addresses. */
			len = xfer->frlengths[i];

			if (useaux == 0) {
				/* skip handled segments */
				if (seglen <= 0) {
					USB_KASSERT2(seg < nsegs,
					  ("ohci_device_isoc_enter: overrun"));
					do {
						seglen1 = seglen;
						seglen += segs[seg].ds_len;
						if (seglen1 <= 0 && seglen > 0)
							segdmaadr =
							    segs[seg].ds_addr
							    - seglen1;
					} while (++seg < nsegs &&
					    (seglen <= 0 ||
					     segdmaadr + seglen ==
						segs[seg].ds_addr));
				}

				/* DMA start and end addresses of the frame */
				if (len <= seglen) {
					/* the frame fits in the segment */
					physend = segdmaadr + len - 1;
					dataphys = segdmaadr;
				} else if (/* len > seglen */
				    (OHCI_PAGE_OFFSET(segdmaadr + seglen) ||
				     OHCI_PAGE_OFFSET(segs[seg].ds_addr) ||
				     len > seglen + segs[seg].ds_len)) {

					DPRINTF(("ohci_device_isoc_enter: need aux len %#x dma %#x seglen %#x nextseg %#x\n", len, segdmaadr, seglen, (unsigned)segs[seg].ds_addr));
					/* need aux memory */
					useaux = 1;
					if (ncur > 0) {
						/* doesn't fit for now */
						break;
					} else {
						goto doaux;	/* shortcut */
					}
				} else { /* len > seglen */
					/* the frame ends in the next segment */
					physend = segs[seg].ds_addr +
					    (len - seglen - 1);
					dataphys = segdmaadr;
				}
			}

			if (useaux) {
			doaux:
				USB_KASSERT(ncur == 0);	/* for simplicity */

				/* need aux memory */
				auxdma = ohci_aux_dma_alloc(&OXFER(xfer)->aux,
				    &bufptr, len, &sitd->ad);

				/* prepare aux DMA */
				if (!isread)
					usb_bufptr_wr(&bufptr,
					    sitd->ad.aux_kern, len, is_mbuf);
				dataphys = auxdma;
				physend = dataphys + len - 1;

				useaux = 0;
			}

			if (ncur == 0) {
				sitd->itd.itd_bp0 = HTOO32(dataphys);
			} else {
				/* check start of new page */
				if (OHCI_PAGE_OFFSET(dataphys) == 0)
					npagecross++;
			}

			sitd->itd.itd_offset[ncur] =
			    HTOO16(OHCI_ITD_MK_OFFS(npagecross, dataphys));

			if (OHCI_PAGE(dataphys) != OHCI_PAGE(physend))
				npagecross++;
			if (npagecross > 1 ||
			    ((sc->sc_flags & OHCI_FLAG_QUIRK_2ND_4KB) &&
			     npagecross == 1 &&
			     OHCI_PAGE_OFFSET(physend) == OHCI_PAGE_SIZE - 1)) {
				/* the frame doesn't fit in the ITD */
				physend = prevphysend;
				break;
			}

			/* the frame is handled */
			seglen -= len;
			segdmaadr += len;
			usb_bufptr_advance(&bufptr, len, is_mbuf);
			prevphysend = physend;

			i++;
			ncur++;

		} while (ncur < OHCI_ITD_NOFFSET && i < nframes);

		/* Allocate next ITD */
		nsitd = ohci_alloc_sitd(sc);
		if (nsitd == NULL) {
			/* XXX what now? */
			printf("%s: isoc TD alloc failed\n",
			       USBDEVNAME(sc->sc_bus.bdev));
			return;
		}

		/* Fill out remaining fields of current ITD */
		sitd->itd.itd_be = HTOO32(physend);
		sitd->nextitd = nsitd;
		dmaadr = OHCI_SITD_DMAADDR(nsitd);
		sitd->itd.itd_nextitd = HTOO32(dmaadr);
		sitd->xfer = xfer;

		if (i < nframes) {
			sitd->itd.itd_flags = HTOO32(
				OHCI_ITD_NOCC |
				OHCI_ITD_SET_SF(iso->next) |
				OHCI_ITD_SET_DI(6) | /* delay intr a little */
				OHCI_ITD_SET_FC(ncur));
			sitd->flags = OHCI_ITD_ACTIVE;
		} else {
			sitd->itd.itd_flags = HTOO32(
				OHCI_ITD_NOCC |
				OHCI_ITD_SET_SF(iso->next) |
				OHCI_ITD_SET_DI(0) |
				OHCI_ITD_SET_FC(ncur));
			sitd->flags = OHCI_CALL_DONE | OHCI_ITD_ACTIVE;
		}
		iso->next = (uint16_t)(iso->next + ncur);

		OHCI_SITD_SYNC(sc, sitd,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		sitd = nsitd;
	}

	iso->inuse += nframes;

	/* XXX pretend we did it all */
	xfer->actlen = 0;
	for (i = 0; i < nframes; i++)
		xfer->actlen += xfer->frlengths[i];

	xfer->status = USBD_IN_PROGRESS;

#ifdef OHCI_DEBUG
	if (ohcidebug > 5) {
		USB_MEM_SYNC2(&sc->sc_dmatag, &sc->sc_hccadma,
		    offsetof(struct ohci_hcca, hcca_frame_number),
		    sizeof(sc->sc_hcca->hcca_frame_number),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DPRINTF(("ohci_device_isoc_enter: frame=%d\n",
			 O32TOH(sc->sc_hcca->hcca_frame_number)));
		ohci_dump_itds(sc, xfer->hcpriv);
		ohci_dump_ed(sc, sed);
	}
#endif

	/* sync aux */
	ohci_aux_dma_sync(sc, &OXFER(xfer)->aux,
	    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	s = splusb();
	opipe->tail.itd = sitd;
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);
	dmaadr = OHCI_SITD_DMAADDR(sitd);
	sed->ed.ed_tailp = HTOO32(dmaadr);
	OHCI_SED_SYNC(sc, sed, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	splx(s);

#ifdef OHCI_DEBUG
	if (ohcidebug > 5) {
		delay(150000);
		USB_MEM_SYNC2(&sc->sc_dmatag, &sc->sc_hccadma,
		    offsetof(struct ohci_hcca, hcca_frame_number),
		    sizeof(sc->sc_hcca->hcca_frame_number),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DPRINTF(("ohci_device_isoc_enter: after frame=%d\n",
			 O32TOH(sc->sc_hcca->hcca_frame_number)));
		ohci_dump_itds(sc, xfer->hcpriv);
		ohci_dump_ed(sc, sed);
	}
#endif
}

usbd_status
ohci_device_isoc_start(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	ohci_soft_ed_t *sed;
	int s;

	DPRINTFN(5,("ohci_device_isoc_start: xfer=%p\n", xfer));

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (xfer->status != USBD_IN_PROGRESS)
		printf("ohci_device_isoc_start: not in progress %p\n", xfer);
#endif

	/* XXX anything to do? */

	s = splusb();
	sed = opipe->sed;  /*  Turn off ED skip-bit to start processing */
	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP);    /* ED's ITD list.*/
	splx(s);

	return (USBD_IN_PROGRESS);
}

void
ohci_device_isoc_abort(usbd_xfer_handle xfer)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)xfer->pipe;
	ohci_softc_t *sc = (ohci_softc_t *)opipe->pipe.device->bus;
	ohci_soft_ed_t *sed;
	ohci_soft_itd_t *sitd, *tmp_sitd;
	ohci_physaddr_t dmaadr;
	int s,undone,num_sitds;

	s = splusb();
	opipe->aborting = 1;

	DPRINTFN(1,("ohci_device_isoc_abort: xfer=%p\n", xfer));

	/* Transfer is already done. */
	if (xfer->status != USBD_NOT_STARTED &&
	    xfer->status != USBD_IN_PROGRESS) {
		splx(s);
		printf("ohci_device_isoc_abort: early return\n");
		return;
	}

	/* Give xfer the requested abort code. */
	xfer->status = USBD_CANCELLED;

	sed = opipe->sed;
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP); /* force hardware skip */
	OHCI_SED_SYNC2(sc, sed,
	    offsetof(ohci_ed_t, ed_flags), sizeof(u_int32_t),
	    BUS_DMASYNC_PREWRITE);

	num_sitds = 0;
	sitd = xfer->hcpriv;
#ifdef DIAGNOSTIC
	if (sitd == NULL) {
		splx(s);
		printf("ohci_device_isoc_abort: hcpriv==0\n");
		return;
	}
#endif
	for (; sitd != NULL && sitd->xfer == xfer; sitd = sitd->nextitd) {
		num_sitds++;
#ifdef DIAGNOSTIC
		DPRINTFN(1,("abort sets done sitd=%p\n", sitd));
		sitd->isdone = 1;
#endif
	}

	splx(s);

	/*
	 * Each sitd has up to OHCI_ITD_NOFFSET transfers, each can
	 * take a usb 1ms cycle. Conservatively wait for it to drain.
	 * Even with DMA done, it can take awhile for the "batch"
	 * delivery of completion interrupts to occur thru the controller.
	 */

	do {
		usb_delay_ms(&sc->sc_bus, 2*(num_sitds*OHCI_ITD_NOFFSET));

		undone   = 0;
		tmp_sitd = xfer->hcpriv;
		for (; tmp_sitd != NULL && tmp_sitd->xfer == xfer;
		    tmp_sitd = tmp_sitd->nextitd) {
			OHCI_SITD_SYNC(sc, tmp_sitd,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			if (OHCI_CC_NO_ERROR ==
			    OHCI_ITD_GET_CC(O32TOH(tmp_sitd->itd.itd_flags)) &&
			    tmp_sitd->flags & OHCI_ITD_ACTIVE &&
			    (tmp_sitd->flags & OHCI_ITD_INTFIN) == 0)
				undone++;
		}
	} while (undone != 0);

	s = splusb();

	/* Run callback. */
	ohci_transfer_complete(xfer, 0);

	/* There is always a `next' sitd so link it up. */
	dmaadr = OHCI_SITD_DMAADDR(sitd);
	sed->ed.ed_headp = HTOO32(dmaadr);

	sed->ed.ed_flags &= HTOO32(~OHCI_ED_SKIP); /* remove hardware skip */
	OHCI_SED_SYNC(sc, sed, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
#if 0	/* above should be like this? */
	OHCI_SED_SYNC2(sc, sed,
	    offsetof(ohci_ed_t, ed_flags), sizeof(u_int32_t),
	    BUS_DMASYNC_PREWRITE);
#endif

	splx(s);
}

void
ohci_device_isoc_done(usbd_xfer_handle xfer)
{
	/* This null routine corresponds to non-isoc "done()" routines
	 * that free the stds associated with an xfer after a completed
	 * xfer interrupt. However, in the case of isoc transfers, the
	 * sitds associated with the transfer have already been processed
	 * and reallocated for the next iteration by
	 * "ohci_device_isoc_transfer()".
	 *
	 * Routine "usb_transfer_complete()" is called at the end of every
	 * relevant usb interrupt. "usb_transfer_complete()" indirectly
	 * calls 1) "ohci_device_isoc_transfer()" (which keeps pumping the
	 * pipeline by setting up the next transfer iteration) and 2) then
	 * calls "ohci_device_isoc_done()". Isoc transfers have not been
	 * working for the ohci usb because this routine was trashing the
	 * xfer set up for the next iteration (thus, only the first
	 * UGEN_NISOREQS xfers outstanding on an open would work). Perhaps
	 * this could all be re-factored, but that's another pass...
	 */
}

usbd_status
ohci_setup_isoc(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;
	struct iso *iso = &opipe->u.iso;
	int s;

	iso->next = -1;
	iso->inuse = 0;

	s = splusb();
	ohci_add_ed(sc, opipe->sed, sc->sc_isoc_head);
	splx(s);

	return (USBD_NORMAL_COMPLETION);
}

void
ohci_device_isoc_close(usbd_pipe_handle pipe)
{
	struct ohci_pipe *opipe = (struct ohci_pipe *)pipe;
	ohci_softc_t *sc = (ohci_softc_t *)pipe->device->bus;
	ohci_soft_ed_t *sed;

	DPRINTF(("ohci_device_isoc_close: pipe=%p\n", pipe));

	sed = opipe->sed;
	sed->ed.ed_flags |= HTOO32(OHCI_ED_SKIP); /* Stop device. */
	OHCI_SED_SYNC2(sc, sed,
	    offsetof(ohci_ed_t, ed_flags), sizeof(u_int32_t),
	    BUS_DMASYNC_PREWRITE);

	ohci_close_pipe(pipe, sc->sc_isoc_head); /* Stop isoc list, free ED.*/

	/* up to NISOREQs xfers still outstanding. */

#ifdef DIAGNOSTIC
	opipe->tail.itd->isdone = 1;
#endif
	ohci_free_sitd_norsv(sc, opipe->tail.itd); /* Next `avail free' sitd.*/
}
