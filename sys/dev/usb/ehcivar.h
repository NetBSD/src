/*	$NetBSD: ehcivar.h,v 1.42.14.18 2015/10/20 08:33:17 skrll Exp $ */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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

#ifndef _EHCIVAR_H_
#define _EHCIVAR_H_

#include <sys/pool.h>

typedef struct ehci_soft_qtd {
	ehci_qtd_t qtd;
	struct ehci_soft_qtd *nextqtd;	/* mirrors nextqtd in TD */
	ehci_physaddr_t physaddr;	/* qTD's physical address */
	usb_dma_t dma;			/* qTD's DMA infos */
	int offs;			/* qTD's offset in usb_dma_t */
	struct usbd_xfer *xfer;		/* xfer back pointer */
	uint16_t len;
} ehci_soft_qtd_t;
#define EHCI_SQTD_ALIGN	MAX(EHCI_QTD_ALIGN, CACHE_LINE_SIZE)
#define EHCI_SQTD_SIZE ((sizeof(struct ehci_soft_qtd) + EHCI_SQTD_ALIGN - 1) & -EHCI_SQTD_ALIGN)
#define EHCI_SQTD_CHUNK (EHCI_PAGE_SIZE / EHCI_SQTD_SIZE)

typedef struct ehci_soft_qh {
	ehci_qh_t qh;
	struct ehci_soft_qh *next;
	struct ehci_soft_qtd *sqtd;
	ehci_physaddr_t physaddr;
	usb_dma_t dma;			/* QH's DMA infos */
	int offs;			/* QH's offset in usb_dma_t */
	int islot;
} ehci_soft_qh_t;
#define EHCI_SQH_SIZE ((sizeof(struct ehci_soft_qh) + EHCI_QH_ALIGN - 1) / EHCI_QH_ALIGN * EHCI_QH_ALIGN)
#define EHCI_SQH_CHUNK (EHCI_PAGE_SIZE / EHCI_SQH_SIZE)

typedef struct ehci_soft_itd {
	union {
		ehci_itd_t itd;
		ehci_sitd_t sitd;
	};
	union {
		struct {
			/* soft_itds links in a periodic frame */
			struct ehci_soft_itd *next;
			struct ehci_soft_itd *prev;
		} frame_list;
		/* circular list of free itds */
		LIST_ENTRY(ehci_soft_itd) free_list;
	};
	struct ehci_soft_itd *xfer_next; /* Next soft_itd in xfer */
	ehci_physaddr_t physaddr;
	usb_dma_t dma;
	int offs;
	int slot;
	struct timeval t; /* store free time */
} ehci_soft_itd_t;
#define EHCI_ITD_SIZE ((sizeof(struct ehci_soft_itd) + EHCI_QH_ALIGN - 1) / EHCI_ITD_ALIGN * EHCI_ITD_ALIGN)
#define EHCI_ITD_CHUNK (EHCI_PAGE_SIZE / EHCI_ITD_SIZE)

#define ehci_soft_sitd_t ehci_soft_itd_t
#define ehci_soft_sitd ehci_soft_itd
#define sc_softsitds sc_softitds
#define EHCI_SITD_SIZE ((sizeof(struct ehci_soft_sitd) + EHCI_QH_ALIGN - 1) / EHCI_SITD_ALIGN * EHCI_SITD_ALIGN)
#define EHCI_SITD_CHUNK (EHCI_PAGE_SIZE / EHCI_SITD_SIZE)

struct ehci_xfer {
	struct usbd_xfer ex_xfer;
	struct usb_task	ex_aborttask;
	TAILQ_ENTRY(ehci_xfer) ex_next; /* list of active xfers */
	union {
		/* ctrl/bulk/intr */
		struct {
			ehci_soft_qtd_t *ex_sqtdstart;
			ehci_soft_qtd_t *ex_sqtdend;
		};
		/* isoc */
		struct {
			ehci_soft_itd_t *ex_itdstart;
			ehci_soft_itd_t *ex_itdend;
		};
		/* split isoc */
		struct {
			ehci_soft_sitd_t *ex_sitdstart;
			ehci_soft_sitd_t *ex_sitdend;
		};
	};
	bool ex_isdone;	/* used only when DIAGNOSTIC is defined */
};

#define EHCI_BUS2SC(bus)	((bus)->ub_hcpriv)
#define EHCI_PIPE2SC(pipe)	EHCI_BUS2SC((pipe)->up_dev->ud_bus)
#define EHCI_XFER2SC(xfer)	EHCI_PIPE2SC((xfer)->ux_pipe)
#define EHCI_EPIPE2SC(epipe)	EHCI_BUS2SC((epipe)->pipe.up_dev->ud_bus)

#define EHCI_XFER2EXFER(xfer)	((struct ehci_xfer *)(xfer))

#define EHCI_XFER2EPIPE(xfer)	((struct ehci_pipe *)((xfer)->ux_pipe))
#define EHCI_PIPE2EPIPE(pipe)	((struct ehci_pipe *)(pipe))

/* Information about an entry in the interrupt list. */
struct ehci_soft_islot {
	ehci_soft_qh_t *sqh;	/* Queue Head. */
};

#define EHCI_FRAMELIST_MAXCOUNT	1024
#define EHCI_IPOLLRATES		8 /* Poll rates (1ms, 2, 4, 8 .. 128) */
#define EHCI_INTRQHS		((1 << EHCI_IPOLLRATES) - 1)
#define EHCI_MAX_POLLRATE	(1 << (EHCI_IPOLLRATES - 1))
#define EHCI_IQHIDX(lev, pos) \
	((((pos) & ((1 << (lev)) - 1)) | (1 << (lev))) - 1)
#define EHCI_ILEV_IVAL(lev)	(1 << (lev))


#define EHCI_HASH_SIZE 128
#define EHCI_COMPANION_MAX 8

#define EHCI_FREE_LIST_INTERVAL 100

typedef struct ehci_softc {
	device_t sc_dev;
	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_doorbell;
	void *sc_doorbell_si;
	void *sc_pcd_si;
	struct usbd_bus sc_bus;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t sc_size;
	u_int sc_offs;			/* offset to operational regs */
	int sc_flags;			/* misc flags */
#define EHCIF_DROPPED_INTR_WORKAROUND	0x01
#define EHCIF_ETTF			0x02 /* Emb. Transaction Translater func. */

	char sc_vendor[32];		/* vendor string for root hub */
	int sc_id_vendor;		/* vendor ID for root hub */

	uint32_t sc_cmd;		/* shadow of cmd reg during suspend */

	u_int sc_ncomp;
	u_int sc_npcomp;
	device_t sc_comps[EHCI_COMPANION_MAX];

	usb_dma_t sc_fldma;
	ehci_link_t *sc_flist;
	u_int sc_flsize;
	u_int sc_rand;			/* XXX need proper intr scheduling */

	struct ehci_soft_islot sc_islots[EHCI_INTRQHS];

	/*
	 * an array matching sc_flist, but with software pointers,
	 * not hardware address pointers
	 */
	struct ehci_soft_itd **sc_softitds;

	TAILQ_HEAD(, ehci_xfer) sc_intrhead;

	ehci_soft_qh_t *sc_freeqhs;
	ehci_soft_qtd_t *sc_freeqtds;
	LIST_HEAD(sc_freeitds, ehci_soft_itd) sc_freeitds;
	LIST_HEAD(sc_freesitds, ehci_soft_sitd) sc_freesitds;

	int sc_noport;
	uint8_t sc_hasppc;		/* has Port Power Control */
	struct usbd_xfer *sc_intrxfer;
	char sc_isreset[EHCI_MAX_PORTS];
	char sc_softwake;
	kcondvar_t sc_softwake_cv;

	uint32_t sc_eintrs;
	ehci_soft_qh_t *sc_async_head;

	pool_cache_t sc_xferpool;	/* free xfer pool */

	struct callout sc_tmo_intrlist;

	device_t sc_child; /* /dev/usb# device */
	char sc_dying;

	void (*sc_vendor_init)(struct ehci_softc *);
	int (*sc_vendor_port_status)(struct ehci_softc *, uint32_t, int);
} ehci_softc_t;

#define EREAD1(sc, a) bus_space_read_1((sc)->iot, (sc)->ioh, (a))
#define EREAD2(sc, a) bus_space_read_2((sc)->iot, (sc)->ioh, (a))
#define EREAD4(sc, a) bus_space_read_4((sc)->iot, (sc)->ioh, (a))
#define EWRITE1(sc, a, x) bus_space_write_1((sc)->iot, (sc)->ioh, (a), (x))
#define EWRITE2(sc, a, x) bus_space_write_2((sc)->iot, (sc)->ioh, (a), (x))
#define EWRITE4(sc, a, x) bus_space_write_4((sc)->iot, (sc)->ioh, (a), (x))
#define EOREAD1(sc, a) bus_space_read_1((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a))
#define EOREAD2(sc, a) bus_space_read_2((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a))
#define EOREAD4(sc, a) bus_space_read_4((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a))
#define EOWRITE1(sc, a, x) bus_space_write_1((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a), (x))
#define EOWRITE2(sc, a, x) bus_space_write_2((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a), (x))
#define EOWRITE4(sc, a, x) bus_space_write_4((sc)->iot, (sc)->ioh, (sc)->sc_offs+(a), (x))

int		ehci_init(ehci_softc_t *);
int		ehci_intr(void *);
int		ehci_detach(ehci_softc_t *, int);
int		ehci_activate(device_t, enum devact);
void		ehci_childdet(device_t, device_t);
bool		ehci_suspend(device_t, const pmf_qual_t *);
bool		ehci_resume(device_t, const pmf_qual_t *);
bool		ehci_shutdown(device_t, int);

#endif /* _EHCIVAR_H_ */
