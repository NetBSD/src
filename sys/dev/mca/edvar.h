/*	$NetBSD: edvar.h,v 1.4 2001/05/04 12:58:34 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct edc_mca_softc;
#define DASD_MAX_CMD_RES_LEN	8

struct ed_softc {
	struct device sc_dev;

	/* General disk infos */
	struct disk sc_dk;
	struct buf_queue sc_q;
	struct simplelock sc_q_lock;
	struct callout sc_edstart;

	void *sc_data;		/* pointer to data for tranfer */ 
	long sc_bcount;		/* bytes available in buffer */
	daddr_t sc_rawblkno;	/* starting blkno of transfer */
	int sc_read;		/* Read Transfer ? */

	struct edc_mca_softc *edc_softc;   /* pointer to our parent */

	int sc_flags;	  
#define WDF_WLABEL	0x001 /* label is writable */
#define WDF_LABELLING   0x002 /* writing label */
#define WDF_LOADED	0x004 /* parameters loaded */
#define WDF_KLABEL	0x008 /* retain label after 'full' close */
#define EDF_BOUNCEBUF		0x010	/* use bounce buffer */
#define EDF_DMAMAP_LOADED	0x020	/* dmamap_xfer loaded */
#define EDF_PROCESS_QUEUE	0x040
#define EDF_DK_BUSY		0x080	/* disk_busy() called */
#define EDF_INIT		0x100	/* disk initialized */
	int sc_capacity;
	struct lock sc_lock;	/* drive lock */

	/* actual drive parameters */
	int sc_devno;		/* DASD device number */
	u_int16_t cyl;
	u_int8_t heads;
	u_int8_t sectors;
	u_int8_t spares;	/* spares per cylinder */
	u_int32_t rba;
	u_int8_t drv_flags;

	u_int16_t sc_status_block[DASD_MAX_CMD_RES_LEN]; /* CMD status block */

	daddr_t sc_badsect[127];	/* 126 plus trailing -1 marker */

	/* Info needed for DMA */
	bus_dma_tag_t sc_dmat;		/* DMA tag as passed by parent */
	bus_dmamap_t dmamap_xfer;	/* transfer dma map */
#define ED_NSEGS	1
	bus_dma_segment_t sc_dmaseg[ED_NSEGS];	/* DMA segment array */
	bus_dma_segment_t sc_dmam[1];	/* Allocated DMA-safe memory */
	vsize_t		sc_dmam_sz;	/* Size of allocated DMA memory */ 
	caddr_t		sc_dmamkva;	/* Mapped kva of the memory */
	
	void *sc_sdhook;		/* our shutdown hook */

#if NRND > 0
	rndsource_element_t	rnd_source;
#endif

	struct proc *sc_worker;		/* Worker thread */
	volatile int sc_error;
};
