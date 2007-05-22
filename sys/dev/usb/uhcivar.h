/*	$NetBSD: uhcivar.h,v 1.40.40.1 2007/05/22 14:57:42 itohy Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/uhcivar.h,v 1.43 2006/09/07 00:06:41 imp Exp $	*/

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

/*
 * To avoid having 1024 TDs for each isochronous transfer we introduce
 * a virtual frame list.  Every UHCI_VFRAMELIST_COUNT entries in the real
 * frame list points to a non-active TD.  These, in turn, form the
 * starts of the virtual frame list.  This also has the advantage that it
 * simplifies linking in/out of TDs/QHs in the schedule.
 * Furthermore, initially each of the inactive TDs point to an inactive
 * QH that forms the start of the interrupt traffic for that slot.
 * Each of these QHs point to the same QH that is the start of control
 * traffic.  This QH points at another QH which is the start of the
 * bulk traffic.
 *
 * UHCI_VFRAMELIST_COUNT should be a power of 2 and <= UHCI_FRAMELIST_COUNT.
 */
#define UHCI_VFRAMELIST_COUNT 128

typedef struct uhci_soft_qh uhci_soft_qh_t;
typedef struct uhci_soft_td uhci_soft_td_t;

typedef union {
	struct uhci_soft_qh *sqh;
	struct uhci_soft_td *std;
} uhci_soft_td_qh_t;

/*
 * An interrupt info struct contains the information needed to
 * execute a requested routine when the controller generates an
 * interrupt.  Since we cannot know which transfer generated
 * the interrupt all structs are linked together so they can be
 * searched at interrupt time.
 */
typedef struct uhci_intr_info {
	struct uhci_softc *sc;
	usbd_xfer_handle xfer;
	uhci_soft_td_t *stdstart;
	uhci_soft_td_t *stdend;
	LIST_ENTRY(uhci_intr_info) list;
#ifdef DIAGNOSTIC
	int isdone;
#endif
} uhci_intr_info_t;

/* for aux memory */
#define UHCI_AUX_CHUNK_SIZE		4096
#define UHCI_MAX_PKT_SIZE		1023	/* USB 1.1 specification */
#define UHCI_AUX_PER_CHUNK(maxp)	(UHCI_AUX_CHUNK_SIZE/(maxp))
#define UHCI_NCHUNK(naux, maxp)	\
	(((naux) + UHCI_AUX_PER_CHUNK(maxp) - 1) / UHCI_AUX_PER_CHUNK(maxp))
struct uhci_aux_mem {
	usb_dma_t aux_chunk_dma[UHCI_NCHUNK(USB_DMA_NSEG-1, UHCI_MAX_PKT_SIZE)];
	int	aux_nchunk;	/* number of allocated chunk */
	int	aux_curchunk;	/* current chunk */
	int	aux_chunkoff;	/* offset in current chunk */
	int	aux_naux;	/* number of aux */
};

struct uhci_xfer {
	struct usbd_xfer xfer;
	uhci_intr_info_t iinfo;
	struct usb_task	abort_task;
	int curframe;
	u_int32_t uhci_xfer_flags;
	struct usb_buffer_dma dmabuf;
	int rsvd_tds;
	struct uhci_aux_mem aux;
};

#define UHCI_XFER_ABORTING	0x0001	/* xfer is aborting. */
#define UHCI_XFER_ABORTWAIT	0x0002	/* abort completion is being awaited. */

#if 1	/* make sure the argument is actually an xfer pointer */
#define UXFER(xfer) ((void)&(xfer)->hcpriv, (struct uhci_xfer *)(xfer))
#else
#define UXFER(xfer) ((struct uhci_xfer *)(xfer))
#endif

struct uhci_mem_desc {
	caddr_t		um_top;
	uhci_physaddr_t	um_topdma;
	usb_dma_t	um_dma;
	SIMPLEQ_ENTRY(uhci_mem_desc) um_next;
};

/*
 * Extra information that we need for a TD.
 */
struct uhci_soft_td {
	uhci_td_t td;			/* The real TD, must be first */
	uhci_soft_td_qh_t link; 	/* soft version of the td_link field */
	struct uhci_mem_desc *ut_mdesc;	/* DMA memory desc */
	uhci_physaddr_t aux_dma;	/* Auxillary storage if needed. */
	void *aux_kern;
	void *aux_data;			/* Original aux data virtual address. */
	int aux_len;			/* Auxillary storage size. */
};
/*
 * Make the size such that it is a multiple of UHCI_TD_ALIGN.  This way
 * we can pack a number of soft TD together and have the real TD well
 * aligned.
 * NOTE: Minimum size is 32 bytes.
 */
#define UHCI_STD_SIZE ((sizeof (struct uhci_soft_td) + UHCI_TD_ALIGN - 1) / UHCI_TD_ALIGN * UHCI_TD_ALIGN)
#define UHCI_STD_CHUNK ((PAGE_SIZE - sizeof(struct uhci_mem_desc)) / UHCI_STD_SIZE)
#define UHCI_STD_DMAADDR(d)	\
	((d)->ut_mdesc->um_topdma + ((caddr_t)(d) - (d)->ut_mdesc->um_top))
#define UHCI_STD_SYNC(sc, d, ops)	\
	USB_MEM_SYNC2(&(sc)->sc_dmatag, &(d)->ut_mdesc->um_dma, (caddr_t)(d) - (d)->ut_mdesc->um_top , sizeof(uhci_td_t), (ops))

/*
 * Extra information that we need for a QH.
 */
struct uhci_soft_qh {
	uhci_qh_t qh;			/* The real QH, must be first */
	uhci_soft_qh_t *hlink;		/* soft version of qh_hlink */
	uhci_soft_td_t *elink;		/* soft version of qh_elink */
	struct uhci_mem_desc *uq_mdesc;	/* DMA memory desc */
	int pos;			/* Timeslot position */
};
/* See comment about UHCI_STD_SIZE. */
#define UHCI_SQH_SIZE ((sizeof (struct uhci_soft_qh) + UHCI_QH_ALIGN - 1) / UHCI_QH_ALIGN * UHCI_QH_ALIGN)
#define UHCI_SQH_CHUNK ((PAGE_SIZE - sizeof(struct uhci_mem_desc)) / UHCI_SQH_SIZE)
#define UHCI_SQH_DMAADDR(d)	\
	((d)->uq_mdesc->um_topdma + ((caddr_t)(d) - (d)->uq_mdesc->um_top))
#define UHCI_SQH_SYNC(sc, d, ops)	\
	USB_MEM_SYNC2(&(sc)->sc_dmatag, &(d)->uq_mdesc->um_dma, (caddr_t)(d) - (d)->uq_mdesc->um_top , sizeof(uhci_qh_t), (ops))

/*
 * Information about an entry in the virtual frame list.
 */
struct uhci_vframe {
	uhci_soft_td_t *htd;		/* pointer to dummy TD */
	uhci_soft_td_t *etd;		/* pointer to last TD */
	uhci_soft_qh_t *hqh;		/* pointer to dummy QH */
	uhci_soft_qh_t *eqh;		/* pointer to last QH */
	u_int bandwidth;		/* max bandwidth used by this frame */
};

#define UHCI_SCFLG_DONEINIT	0x0001	/* uhci_init() done */

typedef struct uhci_softc {
	struct usbd_bus sc_bus;		/* base device */
	int sc_flags;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t sc_size;
#if defined(__FreeBSD__)
	void *ih;

	struct resource *io_res;
	struct resource *irq_res;
#endif

	usb_dma_tag_t sc_dmatag;
	uhci_physaddr_t *sc_pframes;
	usb_dma_t sc_dma;
	struct uhci_vframe sc_vframes[UHCI_VFRAMELIST_COUNT];

	uhci_soft_qh_t *sc_lctl_start;	/* dummy QH for low speed control */
	uhci_soft_qh_t *sc_lctl_end;	/* last control QH */
	uhci_soft_qh_t *sc_hctl_start;	/* dummy QH for high speed control */
	uhci_soft_qh_t *sc_hctl_end;	/* last control QH */
	uhci_soft_qh_t *sc_bulk_start;	/* dummy QH for bulk */
	uhci_soft_qh_t *sc_bulk_end;	/* last bulk transfer */
	uhci_soft_qh_t *sc_last_qh;	/* dummy QH at the end */
	u_int32_t sc_loops;		/* number of QHs that wants looping */

	uhci_soft_td_t *sc_freetds;	/* TD free list */
	uhci_soft_qh_t *sc_freeqhs;	/* QH free list */
	int sc_nfreetds;

	SIMPLEQ_HEAD(uhci_mdescs, uhci_mem_desc)
		sc_std_chunks, sc_sqh_chunks;

	SIMPLEQ_HEAD(, usbd_xfer) sc_free_xfers; /* free xfers */

	u_int8_t sc_addr;		/* device address */
	u_int8_t sc_conf;		/* device configuration */

	u_int8_t sc_saved_sof;
	u_int16_t sc_saved_frnum;

#ifdef USB_USE_SOFTINTR
	char sc_softwake;
#endif /* USB_USE_SOFTINTR */

	char sc_isreset;
	char sc_suspend;
	char sc_dying;

	LIST_HEAD(, uhci_intr_info) sc_intrhead;

	/* Info for the root hub interrupt channel. */
	int sc_ival;			/* time between root hub intrs */
	usbd_xfer_handle sc_intr_xfer;	/* root hub interrupt transfer */
	usb_callout_t sc_poll_handle;

	char sc_vendor[32];		/* vendor string for root hub */
	int sc_id_vendor;		/* vendor ID for root hub */

#if defined(__NetBSD__)
	void *sc_powerhook;		/* cookie from power hook */
	void *sc_shutdownhook;		/* cookie from shutdown hook */
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
	device_t sc_child;		/* /dev/usb# device */
#endif
} uhci_softc_t;

usbd_status	uhci_init(uhci_softc_t *);
int		uhci_intr(void *);
int		uhci_detach(uhci_softc_t *, int);
#if defined(__NetBSD__) || defined(__OpenBSD__)
int		uhci_activate(device_t, enum devact);
#endif

void		uhci_shutdown(void *v);
void		uhci_power(int state, void *priv);

