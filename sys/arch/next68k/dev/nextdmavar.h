/*	$NetBSD: nextdmavar.h,v 1.3 1998/12/19 09:31:44 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


struct nextdma_config {
	bus_space_tag_t nd_bst;				/* bus space tag */
	bus_space_handle_t nd_bsh;		/* bus space handle for device */

  u_long nd_intr;               /* NEXT_I_ values from cpu.h */

	bus_dma_tag_t nd_dmat;

	/* This is called to get another map to dma */
	bus_dmamap_t (*nd_continue_cb) __P((void *));
	/* This is called when a map has completed dma */
	void (*nd_completed_cb) __P((bus_dmamap_t, void *));
	/* This is called when dma shuts down */
	void (*nd_shutdown_cb)  __P((void *));

	void *nd_cb_arg;							/* callback argument */

	struct generic_bus_dma_tag _nd_dmat; /* should probably be elsewhere */
	bus_dmamap_t _nd_map;					/* map currently in dd_next */
	int          _nd_idx;					/* idx of segment currently in dd_next */
	bus_dmamap_t _nd_map_cont;    /* map needed to continue DMA */
	int          _nd_idx_cont;		/* segment index to continue DMA */
};


/* Configure the interface & initialize private structure vars */
void nextdma_config __P((struct nextdma_config *));
void nextdma_init __P((struct nextdma_config *));
void nextdma_start __P((struct nextdma_config *, u_long));

/* query to see if nextdma is finished */
int nextdma_finished __P((	struct nextdma_config *));
void nextdma_reset __P((struct nextdma_config *));
