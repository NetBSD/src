/*	$NetBSD: ohcivar.h,v 1.39.40.1 2007/05/22 14:57:39 itohy Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/ohcivar.h,v 1.45 2006/09/07 00:06:41 imp Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

struct ohci_mem_desc {
	caddr_t		om_top;
	ohci_physaddr_t	om_topdma;
	usb_dma_t	om_dma;
	SIMPLEQ_ENTRY(ohci_mem_desc) om_next;
};

typedef struct ohci_soft_ed {
	ohci_ed_t ed;
	struct ohci_soft_ed *next;
	struct ohci_mem_desc *oe_mdesc;
} ohci_soft_ed_t;
#define OHCI_SED_SIZE ((sizeof (struct ohci_soft_ed) + OHCI_ED_ALIGN - 1) / OHCI_ED_ALIGN * OHCI_ED_ALIGN)
#define OHCI_SED_CHUNK	((PAGE_SIZE - sizeof(struct ohci_mem_desc)) / OHCI_SED_SIZE)
#define OHCI_SED_DMAADDR(d)	\
	((d)->oe_mdesc->om_topdma + ((caddr_t)(d) - (d)->oe_mdesc->om_top))
#define OHCI_SED_SYNC(sc, d, ops)	\
	USB_MEM_SYNC2(&(sc)->sc_dmatag, &(d)->oe_mdesc->om_dma, (caddr_t)(d) - (d)->oe_mdesc->om_top , sizeof(ohci_ed_t), (ops))

typedef struct ohci_soft_td {
	ohci_td_t td;
	struct ohci_soft_td *nexttd; /* mirrors nexttd in TD */
	struct ohci_soft_td *dnext; /* next in done list */
	struct ohci_mem_desc *ot_mdesc;
	LIST_ENTRY(ohci_soft_td) hnext;
	usbd_xfer_handle xfer;
	u_int16_t len;
	u_int16_t flags;
#define OHCI_CALL_DONE	0x0001
#define OHCI_ADD_LEN	0x0002
#define OHCI_TD_FREE	0x8000
} ohci_soft_td_t;
#define OHCI_STD_SIZE ((sizeof (struct ohci_soft_td) + OHCI_TD_ALIGN - 1) / OHCI_TD_ALIGN * OHCI_TD_ALIGN)
#define OHCI_STD_CHUNK ((PAGE_SIZE - sizeof(struct ohci_mem_desc)) / OHCI_STD_SIZE)
#define OHCI_STD_DMAADDR(d)	\
	((d)->ot_mdesc->om_topdma + ((caddr_t)(d) - (d)->ot_mdesc->om_top))
#define OHCI_STD_SYNC(sc, d, ops)	\
	USB_MEM_SYNC2(&(sc)->sc_dmatag, &(d)->ot_mdesc->om_dma, (caddr_t)(d) - (d)->ot_mdesc->om_top, sizeof(ohci_td_t), (ops))

typedef struct ohci_soft_itd {
	ohci_itd_t itd;
	struct ohci_soft_itd *nextitd; /* mirrors nexttd in ITD */
	struct ohci_soft_itd *dnext; /* next in done list */
	struct ohci_mem_desc *oit_mdesc;
	LIST_ENTRY(ohci_soft_itd) hnext;
	usbd_xfer_handle xfer;
	u_int16_t flags;
#define	OHCI_ITD_ACTIVE	0x0010		/* Hardware op in progress */
#define	OHCI_ITD_INTFIN	0x0020		/* Hw completion interrupt seen.*/
#define	OHCI_ITD_FREE	0x8000		/* In free list */
#ifdef DIAGNOSTIC
	char isdone;
#endif
} ohci_soft_itd_t;
#define OHCI_SITD_SIZE ((sizeof (struct ohci_soft_itd) + OHCI_ITD_ALIGN - 1) / OHCI_ITD_ALIGN * OHCI_ITD_ALIGN)
#define OHCI_SITD_CHUNK ((PAGE_SIZE - sizeof(struct ohci_mem_desc)) / OHCI_SITD_SIZE)
#define OHCI_SITD_DMAADDR(d)	\
	((d)->oit_mdesc->om_topdma + ((caddr_t)(d) - (d)->oit_mdesc->om_top))
#define OHCI_SITD_SYNC(sc, d, ops)	\
	USB_MEM_SYNC2(&(sc)->sc_dmatag, &(d)->oit_mdesc->om_dma, (caddr_t)(d) - (d)->oit_mdesc->om_top, sizeof(ohci_itd_t), (ops))

#define OHCI_NO_EDS (2*OHCI_NO_INTRS-1)

#define OHCI_HASH_SIZE 128

#define OHCI_SCFLG_DONEINIT	0x0001	/* ohci_init() done. */

typedef struct ohci_softc {
	struct usbd_bus sc_bus;		/* base device */
	int sc_flags;
#define OHCI_FLAG_QUIRK_2ND_4KB	0x0001	/* can't touch 2nd 4KB boundary */

	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t sc_size;

#if defined(__FreeBSD__)
	void *ih;

	struct resource *io_res;
	struct resource *irq_res;
#endif

	usb_dma_tag_t sc_dmatag;
	usb_dma_t sc_hccadma;
	struct ohci_hcca *sc_hcca;
	ohci_soft_ed_t *sc_eds[OHCI_NO_EDS];
	u_int sc_bws[OHCI_NO_INTRS];

	u_int32_t sc_eintrs;		/* enabled interrupts */

	ohci_soft_ed_t *sc_isoc_head;
	ohci_soft_ed_t *sc_ctrl_head;
	ohci_soft_ed_t *sc_bulk_head;

	SIMPLEQ_HEAD(ohci_mdescs, ohci_mem_desc)
		sc_sed_chunks, sc_std_chunks, sc_sitd_chunks;

	int sc_noport;
	u_int8_t sc_addr;		/* device address */
	u_int8_t sc_conf;		/* device configuration */

	int sc_endian;
#define	OHCI_LITTLE_ENDIAN	0	/* typical (uninitialized default) */
#define	OHCI_HOST_ENDIAN	1	/* if OHCI always matches CPU */

#ifdef USB_USE_SOFTINTR
	char sc_softwake;
#endif /* USB_USE_SOFTINTR */

	ohci_soft_ed_t *sc_freeeds;
	ohci_soft_td_t *sc_freetds;
	ohci_soft_itd_t *sc_freeitds;
	int sc_nfreetds, sc_nfreeitds;

	SIMPLEQ_HEAD(, usbd_xfer) sc_free_xfers; /* free xfers */

	usbd_xfer_handle sc_intrxfer;

	char sc_vendor[16];
	int sc_id_vendor;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	void *sc_powerhook;		/* cookie from power hook */
	void *sc_shutdownhook;		/* cookie from shutdown hook */
#endif
	u_int32_t sc_control;		/* Preserved during suspend/standby */
	u_int32_t sc_intre;

	u_int sc_overrun_cnt;
	struct timeval sc_overrun_ntc;

	usb_callout_t sc_tmo_rhsc;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	device_t sc_child;
#endif
	char sc_dying;
} ohci_softc_t;

struct ohci_xfer {
	struct usbd_xfer xfer;
	struct usb_task	abort_task;
	u_int32_t ohci_xfer_flags;
	struct usb_buffer_dma dmabuf;
	int rsvd_tds;
};
#define OHCI_XFER_ABORTING	0x01	/* xfer is aborting. */
#define OHCI_XFER_ABORTWAIT	0x02	/* abort completion is being awaited. */

#if 1	/* make sure the argument is actually an xfer pointer */
#define OXFER(xfer) ((void)&(xfer)->hcpriv, (struct ohci_xfer *)(xfer))
#else
#define OXFER(xfer) ((struct ohci_xfer *)(xfer))
#endif

usbd_status	ohci_init(ohci_softc_t *);
#if defined(__NetBSD__) || defined(__OpenBSD__)
int
#elif defined(__FreeBSD__)
void
#endif
		ohci_intr(void *);
int		ohci_detach(ohci_softc_t *, int);
#if defined(__NetBSD__) || defined(__OpenBSD__)
int		ohci_activate(device_t, enum devact);
#endif

#define MS_TO_TICKS(ms) ((ms) * hz / 1000)

void		ohci_shutdown(void *v);
void		ohci_power(int state, void *priv);
