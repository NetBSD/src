/*	$NetBSD: gtidmavar.h,v 1.1 2003/03/05 22:08:21 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
 * All rights reserved.
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
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * idmavar.h -- defines for GT-64260 IDMA driver
 *
 * creation	Wed Aug 15 00:48:10 PDT 2001	cliff
 */

#ifndef _IDMAVAR_H
#define _IDMAVAR_H

/*
 * IDMA Overview:
 *
 *	A driver allocates an IDMA channel, registering for callbacks.
 *	The channel includes descriptors and a pending queue
 *
 *	The driver allocates & details descriptors, then requests start
 *
 *		... time passes ...
 *
 *	On completion the callback is made, passing the opaque "arg" 
 *	(e.g. a softc), a descriptor handle, and completion status
 *	The driver callback completes the transaction and frees
 *	or recycles the descriptor.
 *
 *	If the channel is no longer needed the driver may free it,
 *	but we expect drivers to hold channels long term
 *	(i.e till shutdown).
 *
 * Descriptors:
 *
 *	Hardware descriptors are coupled 1:1 with descriptor handles.
 *	The descriptors themselves are as defined by GT-64260 spec;
 *	descriptor handles control descriptor use.  They are separate
 *	to allow efficient packing of descriptors in DMA mapped memory.
 */
 
/*
 * NOTE:
 *	interrupt priority IPL_IDMA is determined by worst case client driver
 *	since each IDMA IRQ is shared across 4 channels
 *	so adjust as needed
 */
#define IPL_IDMA	IPL_NET
#define splidma()	splraise(IPL_IDMA)

#define IDMA_DMSEG_MAX 1
typedef struct idma_dmamem {
	bus_dmamap_t idm_map;		/* dmamem'ed memory */
	caddr_t idm_kva;		/* kva of dmamem memory */
	int idm_nsegs;			/* # of segment in idm_segs */
	int idm_maxsegs;		/* maximum # of segments allowed */
	size_t idm_size;		/* size of memory region */
	bus_dma_segment_t idm_segs[IDMA_DMSEG_MAX];
} idma_dmamem_t;

/*
 * descriptor handle
 */
typedef enum {
	IDH_FREE,
	IDH_ALLOC,
	IDH_QWAIT,
	IDH_PENDING,
	IDH_RETRY,
	IDH_DONE,
	IDH_CANCEL,
	IDH_ABORT
} idma_desch_state_t;
typedef enum {
	IDS_OK,
	IDS_FAIL
} idma_sts_t;
struct idma_chan;
typedef struct idma_desch {
	idma_desch_state_t idh_state;
	struct idma_desch *idh_next;
	struct idma_chan *idh_chan;
	u_int32_t idh_hold;
	SIMPLEQ_ENTRY(idma_desch) idh_q;
	struct idma_desc *idh_desc_va;
	struct idma_desc *idh_desc_pa;
	void *idh_aux;
	u_int64_t tb;
} idma_desch_t;


/*
 * descriptor handle queue head
 */
typedef unsigned int idma_chan_state_t;
#define	IDC_FREE	0
#define	IDC_ALLOC	1
#define	IDC_IDLE	2
#define	IDC_STOPPED	4
#define	IDC_QFULL	8
typedef struct idma_q {
	unsigned int idq_depth;
	SIMPLEQ_HEAD(, idma_desch) idq_q;
} idma_q_t;

/*
 * IDMA channel control
 */
struct idma_softc;
typedef struct idma_chan {
	idma_chan_state_t idc_state;
	struct idma_softc *idc_sc;
	unsigned int idc_chan;			/* channel number */
	int (*idc_callback) __P((void *, struct idma_desch *, u_int32_t));
						/* completion callback */
	void *idc_arg;				/* completion callback arg */
	idma_q_t idc_q;				/* pending transactions */
	unsigned int idc_ndesch;		/* # descriptor handles */
	idma_desch_t *idc_active;		/* active transaction */
	idma_desch_t *idc_desch_free;		/* allocation ptr */
	idma_desch_t *idc_desch_done;		/* completion ptr */
	idma_desch_t *idc_desch;		/* descriptor handles */
	idma_dmamem_t idc_desc_mem;		/* descriptor dmamem */
	unsigned long idc_done_count;		/* running done count */
	unsigned long idc_abort_count;		/* running abort count */
} idma_chan_t;


/*
 * IDMA driver softc
 */
typedef unsigned int idma_state_t;
#define IDMA_ATTACHED	1
typedef struct idma_softc {
	struct device idma_dev;
	struct gt_softc *idma_gt;
	bus_space_tag_t	idma_bustag;
	bus_dma_tag_t idma_dmatag;
	bus_space_handle_t idma_bushandle;
	bus_addr_t idma_reg_base;
	bus_size_t idma_reg_size;
	u_int32_t idma_ien;
	struct callout idma_callout;
	unsigned int idma_callout_state;
	unsigned int idma_burst_size;
	idma_state_t idma_state;
	idma_chan_t idma_chan[NIDMA_CHANS];
	void *idma_ih[4];
} idma_softc_t;

/*
 * IDMA external function prototypes
 */
extern void idma_chan_free __P((idma_chan_t *));
extern idma_chan_t *idma_chan_alloc
	__P((unsigned int, int (*) __P((void *, struct idma_desch *, u_int32_t)), void *));

extern void idma_desch_free __P((idma_desch_t *));
extern idma_desch_t *idma_desch_alloc __P((idma_chan_t *));
extern void idma_desch_list_free __P((idma_desch_t *));

extern void idma_desc_list_free __P((idma_desch_t *));
extern idma_desch_t *idma_desch_list_alloc __P((idma_chan_t *, unsigned int));

extern void idma_intr_enb __P((idma_chan_t *));
extern void idma_intr_dis __P((idma_chan_t *));

extern int idma_start __P((idma_desch_t *));
extern void idma_qflush __P((idma_chan_t *));

extern void idma_abort __P((idma_desch_t *, unsigned int, const char *));

/*
 * flags for idma_abort()
 */
#define IDMA_ABORT_CANCEL	1	/* don't atempt completion or retry */


#endif /* _IDMAVAR_H */
