/*	$NetBSD: fwohcivar.h,v 1.21 2003/07/08 10:06:31 itojun Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of the 3am Software Foundry.
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

#ifndef _DEV_IEEE1394_FWOHCIVAR_H_
#define	_DEV_IEEE1394_FWOHCIVAR_H_

#include <sys/callout.h>
#include <sys/queue.h>

#include <machine/bus.h>

#define	OHCI_PAGE_SIZE		0x0800
#define	OHCI_BUF_ARRQ_CNT	16
#define	OHCI_BUF_ARRS_CNT	8
#define	OHCI_BUF_ATRQ_CNT	(8*8)
#define	OHCI_BUF_ATRS_CNT	(8*8)
#define	OHCI_BUF_IR_CNT		8
#define	OHCI_BUF_CNT							\
	(OHCI_BUF_ARRQ_CNT + OHCI_BUF_ARRS_CNT + OHCI_BUF_ATRQ_CNT +	\
	    OHCI_BUF_ATRS_CNT + OHCI_BUF_IR_CNT + 1 + 1)

#define	OHCI_LOOP		1000
#define	OHCI_SELFID_TIMEOUT	(hz * 3)
#define OHCI_ASYNC_STREAM	0x100

struct fwohci_softc;
struct fwohci_pkt;
struct mbuf;

struct fwohci_buf {
	TAILQ_ENTRY(fwohci_buf) fb_list;
	bus_dma_segment_t fb_seg;
	int fb_nseg;
	bus_dmamap_t fb_dmamap;		/* DMA map of the buffer */
	caddr_t fb_buf;			/* kernel virtual addr of the buffer */
	struct fwohci_desc *fb_desc;	/* kernel virtual addr of descriptor */
	bus_addr_t fb_daddr;		/* physical addr of the descriptor */
	int fb_off;
	struct mbuf *fb_m;
	void *fb_statusarg;
	void (*fb_callback)(struct device *, struct mbuf *);
	int (*fb_statuscb)(struct fwohci_softc *, void *, struct fwohci_pkt *);
};

struct fwohci_pkt {
	int	fp_tcode;
	int	fp_hlen;
	int	fp_dlen;
	u_int32_t fp_hdr[4];
	struct uio fp_uio;
	struct iovec fp_iov[6];
	u_int32_t *fp_trail;
	struct mbuf *fp_m;
	u_int16_t fp_status;
	void *fp_statusarg;
	int (*fp_statuscb)(struct fwohci_softc *, void *, struct fwohci_pkt *);
	void (*fp_callback)(struct device *, struct mbuf *);
};

struct fwohci_handler {
	LIST_ENTRY(fwohci_handler) fh_list;
	u_int32_t	fh_tcode;	/* ARRQ   / ARRS   / IR   */
	u_int32_t	fh_key1;	/* addrhi / srcid  / chan */
	u_int32_t	fh_key2;	/* addrlo / tlabel / tag  */
	u_int32_t	fh_key3;	/* for addr's a possible range. */
	int		(*fh_handler)(struct fwohci_softc *, void *,
	    struct fwohci_pkt *);
	void		*fh_handarg;
};

struct fwohci_ctx {
	int	fc_ctx;
	int	fc_type;	/* FWOHCI_CTX_(ASYNC|ISO_SINGLE|ISO_MULTI) */
	int	fc_bufcnt;
	u_int32_t	*fc_branch;
	TAILQ_HEAD(fwohci_buf_s, fwohci_buf) fc_buf;
	struct fwohci_buf_s fc_buf2; /* for iso */
	LIST_HEAD(, fwohci_handler) fc_handler;
	struct fwohci_buf *fc_buffers;
};



struct fwohci_ir_ctx {
	struct fwohci_softc *irc_sc;

	int irc_num;		/* context number */
	int irc_flags;		/* IEEE1394_IR_* */
	int irc_status;
#define IRC_STATUS_READY		0x0001
#define IRC_STATUS_RUN			0x0002
#define IRC_STATUS_SLEEPING		0x0004
#define IRC_STATUS_RECEIVE		0x0008

	int irc_pktcount;

	int irc_channel;	/* channel number */
	int irc_tagbm;		/* tag bitmap */
	int irc_maxsize;	/* maxmum data size for a packet */

	int irc_maxqueuelen;	/* for debug purpose */
	int irc_maxqueuepos;

	struct fwohci_desc *irc_readtop;	/* where data start */
	struct fwohci_desc *irc_writeend;	/* where branch addr is 0 */
	u_int32_t irc_savedbranch;

	struct fwohci_iso_buf *irc_buf_ptr;

	/* data for descriptor */
	bus_dma_segment_t irc_desc_seg;
	bus_dmamap_t irc_desc_dmamap;
	int irc_desc_num;	/* number of descriptors */
	int irc_desc_size;	/* actual size in byte */
	struct fwohci_desc *irc_desc_map; /* Do not change */
	int irc_desc_nsegs;

	volatile void *irc_waitchan;	/* wait channel */
	struct selinfo irc_sel;

	/* data for buffer */
	bus_dma_segment_t irc_buf_segs[16];
	bus_dmamap_t irc_buf_dmamap;
	int irc_buf_totalsize;
	int irc_buf_nsegs;
	u_int8_t *irc_buf;

	/* for debug purpose */
#ifdef FWOHCI_WAIT_DEBUG
	u_int16_t irc_cycle[3];	/* 0 for wait time, 1 for intr time */
#endif
};



/*
 * Context dedicated for isochronous transmit.  Two data structure are
 * defined.
 */
struct fwohci_it_ctx;

#define IEEE1394_IT_PKTHDR	0x0001

struct fwohci_it_dmabuf {
	struct fwohci_it_ctx *itd_ctx;
	int itd_num;
	int itd_flags;
#define ITD_FLAGS_LOCK		0x0001
#define ITD_FLAGS_UNLOCK	0x0000
#define ITD_FLAGS_LOCK_MASK	0x0001

	/* memory for descriptor */
	struct fwohci_desc *itd_desc;	/* top of descriptor */
	bus_addr_t itd_desc_phys;	/* physical addr of 1st descriptor */
	int itd_descsize;		/* number of total descriptors */
	struct fwohci_desc *itd_lastdesc;	/* last valid descriptor */

	int itd_maxpacket;		/* maximum packets for the buffer */
	int itd_npacket;		/* number of valid packets */
	int itd_maxsize;		/* maximum packet size */

	/* DMA buffer */
#define FWOHCI_MAX_ITDATASEG	8
	bus_dma_segment_t itd_seg[FWOHCI_MAX_ITDATASEG];
	bus_dmamap_t itd_dmamap;
	int itd_size;			/* count in byte */
	u_int8_t *itd_buf;
	int itd_nsegs;

	/* header store descriptor */
	struct fwohci_desc *itd_store;
	bus_addr_t itd_store_phys;

	u_int32_t itd_savedbranch;

#if 0
	int fwohci_itd_construct(struct fwohci_it_ctx *,
	    struct fwohci_it_dmabuf *, int, struct fwohci_desc *, int,
	    int, paddr_t);
	void fwohci_itd_destruct(struct fwohci_it_dmabuf *);
	int fwohci_itd_writedata(struct fwohci_it_dmabuf *, int,
	    struct ieee1394_it_datalist *);
	int fwohci_itd_link(struct fwohci_it_dmabuf *,
	    struct fwohci_it_dmabuf *);
	bus_addr_t fwohci_itd_list_head(struct fwohci_it_dmabuf *);
	void fwohci_itd_clean(struct fwohci_it_dmabuf *);
	int fwohci_itd_isfilled(struct fwohci_it_dmabuf *);
	int fwohci_itd_hasdata(struct fwohci_it_dmabuf *);
	int fwohci_itd_isfull(struct fwohci_it_dmabuf *);
#endif
#define fwohci_itd_list_head(itd)	(itd)->itd_desc_phys
#define fwohci_itd_hasdata(itd)		(itd)->itd_npacket
#define fwohci_itd_isfull(itd)						\
		((itd)->itd_npacket == (itd)->itd_maxpacket)
#define fwohci_itd_islocked(itd)					\
		((itd)->itd_flags & ITD_FLAGS_LOCK)
};


struct fwohci_it_ctx {
	struct fwohci_softc *itc_sc;

	int itc_num;		/* context number */

	volatile int itc_flags;	/* flags */
#define ITC_FLAGS_RUN		0x0001

	int itc_channel;	/* channel number */
	int itc_tag;		/* tag */
	int itc_maxsize;	/* maxmum data size for a packet */
	int itc_speed;		/* speed */

	struct fwohci_it_dmabuf *itc_buf; /* array for fwohci_it_dmabuf */
	int itc_bufnum;		/* const: num of elements in itc_buf array */

#if 1
	volatile struct fwohci_it_dmabuf *itc_buf_start;
	struct fwohci_it_dmabuf *itc_buf_end;
	struct fwohci_it_dmabuf *itc_buf_linkend;
#endif
	volatile int16_t itc_buf_cnt;	/* # buffers which contain data */
#if 0
	int16_t itc_bufidx_start;
	int16_t itc_bufidx_end;
	int16_t itc_bufidx_linkend;
#endif

	/* data for descriptor */
	bus_dma_segment_t itc_dseg;
	bus_dmamap_t itc_ddmamap;
	int itc_descsize;	/* count in byte */
	u_int8_t *itc_descmap;
	int itc_dnsegs;

	volatile u_int32_t *itc_scratch; /* descriptor decoder will write */
	u_int32_t itc_scratch_paddr;

	volatile void *itc_waitchan;	/* wait channel */

	int itc_outpkt;		/* only for debugging */

#if 0
	struct fwohci_it_ctx *fwohci_it_ctx_construct(int);
	void fwohci_it_ctx_destruct(struct fwohci_it_ctx *);
	void fwohci_it_ctx_intr(struct fwohci_it_ctx *);
	int fwohci_it_ctx_writedata(ieee1394_it_tag_t, int,
	    struct ieee1394_it_datalist *);
private:
	void fwohci_it_ctx_run(struct fwohci_it_ctx *);
	void fwohci_it_intr(struct fwohci_softc *, struct fwohci_it_ctx *);
#endif
#define INC_BUF(itc, buf)						\
	do {								\
		if (++buf == (itc)->itc_buf + (itc)->itc_bufnum) {	\
			buf = &(itc)->itc_buf[0];			\
		}							\
	} while (0)
};

struct fwohci_uidtbl {
	int		fu_valid;
	u_int8_t	fu_uid[8];
};

/*
 * Needed to keep track of outstanding packets during a read op. Since the
 * packet stream is asynch it's possible to parse a response packet before the
 * ack bits are processed. In this case something needs to track whether the
 * abuf is still valid before possibly attempting to use items from within it.
 */

struct fwohci_cb {
	struct ieee1394_abuf *ab;
	int count;
	int abuf_valid;
};

struct fwohci_softc {
	struct ieee1394_softc sc_sc1394;
	struct evcnt sc_intrcnt;
	struct evcnt sc_isocnt;
	struct evcnt sc_ascnt;
	struct evcnt sc_itintrcnt;

	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_dma_tag_t sc_dmat;
	bus_size_t sc_memsize;
#if 0

/* Mandatory structures to get the link enabled
 */
	bus_dmamap_t sc_configrom_map;
	bus_dmamap_t sc_selfid_map;
	u_int32_t *sc_selfid_buf;
	u_int32_t *sc_configrom;
#endif

	bus_dma_segment_t sc_dseg;
	int sc_dnseg;
	bus_dmamap_t sc_ddmamap;
	struct fwohci_desc *sc_desc;
	u_int8_t *sc_descmap;
	int sc_descsize;
	int sc_isoctx;
	int sc_itctx;

	void *sc_shutdownhook;
	void *sc_powerhook;
	struct callout sc_selfid_callout;
	int sc_selfid_fail;

	struct fwohci_ctx *sc_ctx_arrq;
	struct fwohci_ctx *sc_ctx_arrs;
	struct fwohci_ctx *sc_ctx_atrq;
	struct fwohci_ctx *sc_ctx_atrs;
	struct fwohci_ctx **sc_ctx_as; /* previously sc_ctx_ir */
	struct fwohci_buf sc_buf_cnfrom;
	struct fwohci_buf sc_buf_selfid;

	struct fwohci_ir_ctx **sc_ctx_ir;
	struct fwohci_it_ctx **sc_ctx_it;

	struct proc *sc_event_thread;

	int sc_dying;
	u_int32_t sc_intmask;
	u_int32_t sc_iso;
    
	u_int8_t sc_csr[CSR_SB_END];

	struct fwohci_uidtbl *sc_uidtbl;
	u_int16_t sc_nodeid;			/* Full Node ID of this node */
	u_int8_t sc_rootid;			/* Phy ID of Root */
	u_int8_t sc_irmid;			/* Phy ID of IRM */
	u_int8_t sc_tlabel;			/* Transaction Label */
	
	LIST_HEAD(, ieee1394_softc) sc_nodelist;
};

int fwohci_init (struct fwohci_softc *, const struct evcnt *);
int fwohci_intr (void *);
int fwohci_print (void *, const char *);
int fwohci_detach(struct fwohci_softc *, int);
int fwohci_activate(struct device *, enum devact);

/* Macros to read and write the OHCI registers
 */
#define	OHCI_CSR_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, reg, htole32(val))
#define	OHCI_CSR_READ(sc, reg) \
	le32toh(bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, reg))

#define FWOHCI_CTX_ASYNC	0
#define FWOHCI_CTX_ISO_SINGLE	1	/* for async stream */
#define FWOHCI_CTX_ISO_MULTI	2	/* for isochronous */

/* Locators. */

#include "locators.h"

#define fwbuscf_idhi cf_loc[FWBUSCF_IDHI]
#define FWBUS_UNK_IDHI FWBUSCF_IDHI_DEFAULT

#define fwbuscf_idlo cf_loc[FWBUSCF_IDLO]
#define FWBUS_UNK_IDLO FWBUSCF_IDLO_DEFAULT

#endif	/* _DEV_IEEE1394_FWOHCIVAR_H_ */
