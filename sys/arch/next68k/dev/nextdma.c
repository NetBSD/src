/*	$NetBSD: nextdma.c,v 1.27 2001/04/17 03:42:25 dbj Exp $	*/
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <m68k/cacheops.h>

#include <next68k/next68k/isr.h>

#define _NEXT68K_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include "nextdmareg.h"
#include "nextdmavar.h"

#if 1
#define ND_DEBUG
#endif

#if defined(ND_DEBUG)
int nextdma_debug = 0;
#define DPRINTF(x) if (nextdma_debug) printf x;
#else
#define DPRINTF(x)
#endif

#if defined(ND_DEBUG)
int nextdma_debug_enetr_idx = 0;
unsigned int nextdma_debug_enetr_state[100] = { 0 };
int nextdma_debug_scsi_idx = 0;
unsigned int nextdma_debug_scsi_state[100] = { 0 };

void nextdma_debug_initstate(struct nextdma_config *nd);
void nextdma_debug_savestate(struct nextdma_config *nd, unsigned int state);
void nextdma_debug_scsi_dumpstate(void);
void nextdma_debug_enetr_dumpstate(void);

void
nextdma_debug_initstate(struct nextdma_config *nd)
{
	switch(nd->nd_intr) {
	case NEXT_I_ENETR_DMA:
		memset(nextdma_debug_enetr_state,0,sizeof(nextdma_debug_enetr_state));
		break;
	case NEXT_I_SCSI_DMA:
		memset(nextdma_debug_scsi_state,0,sizeof(nextdma_debug_scsi_state));
		break;
	}
}

void
nextdma_debug_savestate(struct nextdma_config *nd, unsigned int state)
{
	switch(nd->nd_intr) {
	case NEXT_I_ENETR_DMA:
		nextdma_debug_enetr_state[nextdma_debug_enetr_idx++] = state;
		nextdma_debug_enetr_idx %= (sizeof(nextdma_debug_enetr_state)/sizeof(unsigned int));
		break;
	case NEXT_I_SCSI_DMA:
		nextdma_debug_scsi_state[nextdma_debug_scsi_idx++] = state;
		nextdma_debug_scsi_idx %= (sizeof(nextdma_debug_scsi_state)/sizeof(unsigned int));
		break;
	}
}

void
nextdma_debug_enetr_dumpstate(void)
{
	int i;
	int s;
	s = spldma();
	i = nextdma_debug_enetr_idx;
	do {
		char sbuf[256];
		if (nextdma_debug_enetr_state[i]) {
			bitmask_snprintf(nextdma_debug_enetr_state[i], DMACSR_BITS, sbuf, sizeof(sbuf));
			printf("DMA: 0x%02x state 0x%s\n",i,sbuf);
		}
		i++;
		i %= (sizeof(nextdma_debug_enetr_state)/sizeof(unsigned int));
	} while (i != nextdma_debug_enetr_idx);
	splx(s);
}

void
nextdma_debug_scsi_dumpstate(void)
{
	int i;
	int s;
	s = spldma();
	i = nextdma_debug_scsi_idx;
	do {
		char sbuf[256];
		if (nextdma_debug_scsi_state[i]) {
			bitmask_snprintf(nextdma_debug_scsi_state[i], DMACSR_BITS, sbuf, sizeof(sbuf));
			printf("DMA: 0x%02x state 0x%s\n",i,sbuf);
		}
		i++;
		i %= (sizeof(nextdma_debug_scsi_state)/sizeof(unsigned int));
	} while (i != nextdma_debug_scsi_idx);
	splx(s);
}
#endif


void next_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
                       bus_size_t, int));
int next_dma_continue __P((struct nextdma_config *));
void next_dma_rotate __P((struct nextdma_config *));

void next_dma_setup_cont_regs __P((struct nextdma_config *));
void next_dma_setup_curr_regs __P((struct nextdma_config *));

void
nextdma_config(nd)
	struct nextdma_config *nd;
{
	/* Initialize the dma_tag. As a hack, we currently
	 * put the dma tag in the structure itself.  It shouldn't be there.
	 */

	{
		bus_dma_tag_t t;
		t = &nd->_nd_dmat;
		t->_cookie = nd;
		t->_dmamap_create = _bus_dmamap_create;
		t->_dmamap_destroy = _bus_dmamap_destroy;
		t->_dmamap_load = _bus_dmamap_load_direct;
		t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf_direct;
		t->_dmamap_load_uio = _bus_dmamap_load_uio_direct;
		t->_dmamap_load_raw = _bus_dmamap_load_raw_direct;
		t->_dmamap_unload = _bus_dmamap_unload;
		t->_dmamap_sync = _bus_dmamap_sync;
  
		t->_dmamem_alloc = _bus_dmamem_alloc;
		t->_dmamem_free = _bus_dmamem_free;
		t->_dmamem_map = _bus_dmamem_map;
		t->_dmamem_unmap = _bus_dmamem_unmap;
		t->_dmamem_mmap = _bus_dmamem_mmap;

		nd->nd_dmat = t;
	}

	nextdma_init(nd);

	isrlink_autovec(nextdma_intr, nd, NEXT_I_IPL(nd->nd_intr), 10);
	INTR_ENABLE(nd->nd_intr);
}

void
nextdma_init(nd)
	struct nextdma_config *nd;
{
#ifdef ND_DEBUG
	if (nextdma_debug) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		printf("DMA init ipl (%ld) intr(0x%s)\n",
			NEXT_I_IPL(nd->nd_intr), sbuf);
	}
#endif

	nd->_nd_map = NULL;
	nd->_nd_idx = 0;
	nd->_nd_map_cont = NULL;
	nd->_nd_idx_cont = 0;

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
			DMACSR_RESET | DMACSR_INITBUF);

	next_dma_setup_curr_regs(nd);
	next_dma_setup_cont_regs(nd);

#if defined(DIAGNOSTIC)
	{
		u_long state;
		state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);

#if 1
	/* mourning (a 25Mhz 68040 mono slab) appears to set BUSEXC
	 * milo (a 25Mhz 68040 mono cube) didn't have this problem
	 * Darrin B. Jewell <jewell@mit.edu>  Mon May 25 07:53:05 1998
	 */
    state &= (DMACSR_COMPLETE | DMACSR_SUPDATE | DMACSR_ENABLE);
#else
    state &= (DMACSR_BUSEXC | DMACSR_COMPLETE | 
              DMACSR_SUPDATE | DMACSR_ENABLE);
#endif
		if (state) {
			next_dma_print(nd);
			panic("DMA did not reset");
		}
	}
#endif
}


void
nextdma_reset(nd)
	struct nextdma_config *nd;
{
	int s;
	s = spldma();

	DPRINTF(("DMA reset\n"));

#if (defined(ND_DEBUG))
	if (nextdma_debug) next_dma_print(nd);
#endif

	if ((nd->_nd_map) || (nd->_nd_map_cont)) {
		/* @@@ clean up dma maps */
		panic("DMA abort not implemented\n");
	}

	nextdma_init(nd);
	splx(s);
}

/****************************************************************/


/* Call the completed and continue callbacks to try to fill
 * in the dma continue buffers.
 */
void
next_dma_rotate(nd)
	struct nextdma_config *nd;
{

	DPRINTF(("DMA next_dma_rotate()\n"));

	/* Rotate the continue map into the current map */
	nd->_nd_map = nd->_nd_map_cont;
	nd->_nd_idx = nd->_nd_idx_cont;

	if ((!nd->_nd_map_cont) ||
			((nd->_nd_map_cont) &&
					(++nd->_nd_idx_cont >= nd->_nd_map_cont->dm_nsegs))) {
		if (nd->nd_continue_cb) {
			nd->_nd_map_cont = (*nd->nd_continue_cb)(nd->nd_cb_arg);
			if (nd->_nd_map_cont) {
				nd->_nd_map_cont->dm_xfer_len = 0;
			}
		} else {
			nd->_nd_map_cont = 0;
		}
		nd->_nd_idx_cont = 0;
	}

#ifdef DIAGNOSTIC
	if (nd->_nd_map_cont) {
		if (!DMA_BEGINALIGNED(nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr)) {
			next_dma_print(nd);
			panic("DMA request unaligned at start\n");
		}
		if (!DMA_ENDALIGNED(nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr + 
				nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len)) {
			next_dma_print(nd);
			panic("DMA request unaligned at end\n");
		}
	}
#endif

}

void
next_dma_setup_cont_regs(nd)
	struct nextdma_config *nd;
{
	bus_addr_t dd_start;
	bus_addr_t dd_stop;
	bus_addr_t dd_saved_start;
	bus_addr_t dd_saved_stop;

	DPRINTF(("DMA next_dma_setup_regs()\n"));

	if (nd->_nd_map_cont) {
		dd_start = nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr;
		dd_stop  = (nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr +
				nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len);

		if (nd->nd_intr == NEXT_I_ENETX_DMA) {
			dd_stop |= 0x80000000;		/* Ethernet transmit needs secret magic */
		}
	} else {
		dd_start = 0xdeadbeef;
		dd_stop = 0xdeadbeef;
	}

	dd_saved_start = dd_start;
	dd_saved_stop  = dd_stop;

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_START, dd_start);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_STOP, dd_stop);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START, dd_saved_start);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP, dd_saved_stop);

#ifdef DIAGNOSTIC
	if (   (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_START) != dd_start) 
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_STOP) != dd_stop) 
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START) != dd_saved_start) 
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP) != dd_saved_stop)
			) {
		next_dma_print(nd);
		panic("DMA failure writing to continue regs");
	}
#endif
}

void
next_dma_setup_curr_regs(nd)
	struct nextdma_config *nd;
{
	bus_addr_t dd_next;
	bus_addr_t dd_limit;
	bus_addr_t dd_saved_next;
	bus_addr_t dd_saved_limit;

	DPRINTF(("DMA next_dma_setup_curr_regs()\n"));


	if (nd->_nd_map) {
		dd_next = nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr;
		dd_limit = (nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
				nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);

		if (nd->nd_intr == NEXT_I_ENETX_DMA) {
			dd_limit |= 0x80000000; /* Ethernet transmit needs secret magic */
		}
	} else {
		dd_next = 0xdeadbeef;
		dd_limit = 0xdeadbeef;
	}

	dd_saved_next = dd_next;
	dd_saved_limit = dd_limit;

	if (nd->nd_intr == NEXT_I_ENETX_DMA) {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF, dd_next);
	} else {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT, dd_next);
	}
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT, dd_limit);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT, dd_saved_next);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT, dd_saved_limit);

#ifdef DIAGNOSTIC
	if (   (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF) != dd_next)
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT) != dd_next)
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT) != dd_limit)
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT) != dd_saved_next)
			|| (bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT) != dd_saved_limit)
			) {
		next_dma_print(nd);
		panic("DMA failure writing to current regs");
	}
#endif
}


/* This routine is used for debugging */

void
next_dma_print(nd)
	struct nextdma_config *nd;
{
	u_long dd_csr;
	u_long dd_next;
	u_long dd_next_initbuf;
	u_long dd_limit;
	u_long dd_start;
	u_long dd_stop;
	u_long dd_saved_next;
	u_long dd_saved_limit;
	u_long dd_saved_start;
	u_long dd_saved_stop;
	char sbuf[256];

	/* Read all of the registers before we print anything out,
	 * in case something changes
	 */
	dd_csr          = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);
	dd_next         = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT);
	dd_next_initbuf = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF);
	dd_limit        = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT);
	dd_start        = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_START);
	dd_stop         = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_STOP);
	dd_saved_next   = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT);
	dd_saved_limit  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);
	dd_saved_start  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START);
	dd_saved_stop   = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP);

	bitmask_snprintf((*(volatile u_long *)IIOV(NEXT_P_INTRSTAT)),
			 NEXT_INTR_BITS, sbuf, sizeof(sbuf));
	printf("NDMAP: *intrstat = 0x%s\n", sbuf);

	bitmask_snprintf((*(volatile u_long *)IIOV(NEXT_P_INTRMASK)),
			 NEXT_INTR_BITS, sbuf, sizeof(sbuf));
	printf("NDMAP: *intrmask = 0x%s\n", sbuf);

	/* NDMAP is Next DMA Print (really!) */

	if (nd->_nd_map) {
		printf("NDMAP: nd->_nd_map->dm_mapsize = %d\n",
				nd->_nd_map->dm_mapsize);
		printf("NDMAP: nd->_nd_map->dm_nsegs = %d\n",
				nd->_nd_map->dm_nsegs);
		printf("NDMAP: nd->_nd_map->dm_xfer_len = %d\n",
				nd->_nd_map->dm_xfer_len);
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_len = %d\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);
		{
			int i;
			printf("NDMAP: Entire map;\n");
			for(i=0;i<nd->_nd_map->dm_nsegs;i++) {
				printf("NDMAP:   nd->_nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
						i,nd->_nd_map->dm_segs[i].ds_addr);
				printf("NDMAP:   nd->_nd_map->dm_segs[%d].ds_len = %d\n",
						i,nd->_nd_map->dm_segs[i].ds_len);
			}
		}
	} else {
		printf("NDMAP: nd->_nd_map = NULL\n");
	}
	if (nd->_nd_map_cont) {
		printf("NDMAP: nd->_nd_map_cont->dm_mapsize = %d\n",
				nd->_nd_map_cont->dm_mapsize);
		printf("NDMAP: nd->_nd_map_cont->dm_nsegs = %d\n",
				nd->_nd_map_cont->dm_nsegs);
		printf("NDMAP: nd->_nd_map_cont->dm_xfer_len = %d\n",
				nd->_nd_map_cont->dm_xfer_len);
		printf("NDMAP: nd->_nd_map_cont->dm_segs[%d].ds_addr = 0x%08lx\n",
				nd->_nd_idx_cont,nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr);
		printf("NDMAP: nd->_nd_map_cont->dm_segs[%d].ds_len = %d\n",
				nd->_nd_idx_cont,nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len);
		if (nd->_nd_map_cont != nd->_nd_map) {
			int i;
			printf("NDMAP: Entire map;\n");
			for(i=0;i<nd->_nd_map_cont->dm_nsegs;i++) {
				printf("NDMAP:   nd->_nd_map_cont->dm_segs[%d].ds_addr = 0x%08lx\n",
						i,nd->_nd_map_cont->dm_segs[i].ds_addr);
				printf("NDMAP:   nd->_nd_map_cont->dm_segs[%d].ds_len = %d\n",
						i,nd->_nd_map_cont->dm_segs[i].ds_len);
			}
		}
	} else {
		printf("NDMAP: nd->_nd_map_cont = NULL\n");
	}

	bitmask_snprintf(dd_csr, DMACSR_BITS, sbuf, sizeof(sbuf));
	printf("NDMAP: dd->dd_csr          = 0x%s\n",   sbuf);

	printf("NDMAP: dd->dd_saved_next   = 0x%08x\n", dd_saved_next);
	printf("NDMAP: dd->dd_saved_limit  = 0x%08x\n", dd_saved_limit);
	printf("NDMAP: dd->dd_saved_start  = 0x%08x\n", dd_saved_start);
	printf("NDMAP: dd->dd_saved_stop   = 0x%08x\n", dd_saved_stop);
	printf("NDMAP: dd->dd_next         = 0x%08x\n", dd_next);
	printf("NDMAP: dd->dd_next_initbuf = 0x%08x\n", dd_next_initbuf);
	printf("NDMAP: dd->dd_limit        = 0x%08x\n", dd_limit);
	printf("NDMAP: dd->dd_start        = 0x%08x\n", dd_start);
	printf("NDMAP: dd->dd_stop         = 0x%08x\n", dd_stop);

	bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
			 sbuf, sizeof(sbuf));
	printf("NDMAP: interrupt ipl (%ld) intr(0x%s)\n",
			NEXT_I_IPL(nd->nd_intr), sbuf);
}

/****************************************************************/

int
nextdma_intr(arg)
     void *arg;
{
  /* @@@ This is bogus, we can't be certain of arg's type
	 * unless the interrupt is for us.  For now we successfully
	 * cheat because DMA interrupts are the only things invoked
	 * at this interrupt level.
	 */
  struct nextdma_config *nd = arg;

  if (!INTR_OCCURRED(nd->nd_intr)) return 0;
  /* Handle dma interrupts */

#ifdef ND_DEBUG
	if (nextdma_debug) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		printf("DMA interrupt ipl (%ld) intr(0x%s)\n",
			NEXT_I_IPL(nd->nd_intr), sbuf);
	}
#endif

#ifdef DIAGNOSTIC
	if (!nd->_nd_map) {
		next_dma_print(nd);
		panic("DMA missing current map in interrupt!\n");
	}
#endif

  {
    unsigned int state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);

#if defined(ND_DEBUG)
		nextdma_debug_savestate(nd,state);
#endif

#ifdef DIAGNOSTIC
		if (!(state & DMACSR_COMPLETE)) {
			char sbuf[256];
			next_dma_print(nd);
			bitmask_snprintf(state, DMACSR_BITS, sbuf, sizeof(sbuf));
			printf("DMA: state 0x%s\n",sbuf);
			panic("DMA complete not set in interrupt\n");
		}
#endif

		{
			bus_addr_t onext;
			bus_addr_t olimit;
			bus_addr_t slimit;

			DPRINTF(("DMA: finishing xfer\n"));
			
			onext = nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr;
			olimit = onext + nd->_nd_map->dm_segs[nd->_nd_idx].ds_len;

			{
				int result = 0;
				if (state & DMACSR_ENABLE) {
					/* enable bit was set */
					result |= 0x01;
				}
				if (state & DMACSR_SUPDATE) {
					/* supdate bit was set */
					result |= 0x02;
				}
				if (nd->_nd_map_cont == NULL) {
					KASSERT(nd->_nd_idx+1 == nd->_nd_map->dm_nsegs);
					/* Expecting a shutdown, didn't SETSUPDATE last turn */
					result |= 0x04;
				}
				if (state & DMACSR_BUSEXC) {
					/* bus exception bit was set */
					result |= 0x08;
				}
				switch (result) {
				case 0x00: /* !BUSEXC && !expecting && !SUPDATE && !ENABLE */
				case 0x08: /* BUSEXC && !expecting && !SUPDATE && !ENABLE */
					if (nd->nd_intr == NEXT_I_SCSI_DMA) {
						slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT);
					} else {
						slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);
					}
					break;
				case 0x01: /* !BUSEXC && !expecting && !SUPDATE && ENABLE */
				case 0x09: /* BUSEXC && !expecting && !SUPDATE && ENABLE */
					if (nd->nd_intr == NEXT_I_SCSI_DMA) {
						bus_addr_t snext;
						snext = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT);
						if (snext != onext) {
							slimit = olimit;
						} else {
							slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);
						}
					} else {
						slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);
					}
					break;
				case 0x02: /* !BUSEXC && !expecting && SUPDATE && !ENABLE */
				case 0x0a: /* BUSEXC && !expecting && SUPDATE && !ENABLE */ 
					slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT);
					break;
				case 0x04:  /* !BUSEXC && expecting && !SUPDATE && !ENABLE */
				case 0x0c: /* BUSEXC && expecting && !SUPDATE && !ENABLE */ 
					slimit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT);
					break;
				default:
#ifdef DIAGNOSTIC
					{
						char sbuf[256];
						printf("DMA: please send this output to port-next68k-maintainer@netbsd.org:\n");
						bitmask_snprintf(state, DMACSR_BITS, sbuf, sizeof(sbuf));
						printf("DMA: state 0x%s\n",sbuf);
						next_dma_print(nd);
						panic("DMA: condition 0x%02x not yet documented to occur\n",result);
					}
#endif
					slimit = olimit;
					break;
				}
			}

			if (nd->nd_intr == NEXT_I_ENETX_DMA) {
				slimit &= ~0x80000000;
			}

#ifdef DIAGNOSTIC
			if ((slimit < onext) || (slimit > olimit)) {
				char sbuf[256];
				bitmask_snprintf(state, DMACSR_BITS, sbuf, sizeof(sbuf));
				printf("DMA: state 0x%s\n",sbuf);
				next_dma_print(nd);
				panic("DMA: Unexpected limit register (0x%08x) in finish_xfer\n",slimit);
			}
#endif

#ifdef DIAGNOSTIC
			if ((state & DMACSR_ENABLE) && ((nd->_nd_idx+1) != nd->_nd_map->dm_nsegs)) {
				if (slimit != olimit) {
					char sbuf[256];
					bitmask_snprintf(state, DMACSR_BITS, sbuf, sizeof(sbuf));
					printf("DMA: state 0x%s\n",sbuf);
					next_dma_print(nd);
					panic("DMA: short limit register (0x%08x) w/o finishing map.\n",slimit);
				}
			}
#endif

#if (defined(ND_DEBUG))
			if (nextdma_debug > 2) next_dma_print(nd);
#endif

			nd->_nd_map->dm_xfer_len += slimit-onext;

			/* If we've reached the end of the current map, then inform
			 * that we've completed that map.
			 */
			if ((nd->_nd_idx+1) == nd->_nd_map->dm_nsegs) {
				if (nd->nd_completed_cb) 
					(*nd->nd_completed_cb)(nd->_nd_map, nd->nd_cb_arg);
			} else {
				KASSERT(nd->_nd_map == nd->_nd_map_cont);
				KASSERT(nd->_nd_idx+1 == nd->_nd_idx_cont);
			}
			nd->_nd_map = 0;
			nd->_nd_idx = 0;
		}

		if (state & DMACSR_ENABLE) {

			next_dma_rotate(nd);
			next_dma_setup_cont_regs(nd);

			{
				u_long dmadir;								/* 	DMACSR_SETREAD or DMACSR_SETWRITE */
				
				if (state & DMACSR_READ) {
					dmadir = DMACSR_SETREAD;
				} else {
					dmadir = DMACSR_SETWRITE;
				}

				if (nd->_nd_map_cont == NULL) {
					KASSERT(nd->_nd_idx+1 == nd->_nd_map->dm_nsegs);
					bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
							DMACSR_CLRCOMPLETE | dmadir);
				} else {
					bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
							DMACSR_CLRCOMPLETE | dmadir | DMACSR_SETSUPDATE);
				}
			}
			
		} else {

			DPRINTF(("DMA: a shutdown occurred\n"));
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, DMACSR_CLRCOMPLETE | DMACSR_RESET);

			/* Cleanup more incomplete transfers */
#if 1
			/* cleanup continue map */
			if (nd->_nd_map_cont) {
				DPRINTF(("DMA: shutting down with non null continue map\n"));
				if (nd->nd_completed_cb) 
					(*nd->nd_completed_cb)(nd->_nd_map_cont, nd->nd_cb_arg);
								
				nd->_nd_map_cont = 0;
				nd->_nd_idx_cont = 0;
			}
#else
			/* Do an automatic dma restart */
			if (nd->_nd_map_cont) {
				u_long dmadir;								/* 	DMACSR_SETREAD or DMACSR_SETWRITE */

				next_dma_rotate(nd);

				if (state & DMACSR_READ) {
					dmadir = DMACSR_SETREAD;
				} else {
					dmadir = DMACSR_SETWRITE;
				}

				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
						DMACSR_INITBUF | DMACSR_RESET | dmadir);

				next_dma_setup_curr_regs(nd);
				next_dma_setup_cont_regs(nd);

				if (nd->_nd_map_cont == NULL) {
					KASSERT(nd->_nd_idx+1 == nd->_nd_map->dm_nsegs);
					bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
							DMACSR_SETENABLE | dmadir);
				} else {
					bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
							DMACSR_SETSUPDATE | DMACSR_SETENABLE | dmadir);
				}
				return 1;
			}
#endif
			if (nd->nd_shutdown_cb) (*nd->nd_shutdown_cb)(nd->nd_cb_arg);
		}
	}

#ifdef ND_DEBUG
	if (nextdma_debug) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		printf("DMA exiting interrupt ipl (%ld) intr(0x%s)\n",
			NEXT_I_IPL(nd->nd_intr), sbuf);
	}
#endif

  return(1);
}

/*
 * Check to see if dma has finished for a channel */
int
nextdma_finished(nd)
	struct nextdma_config *nd;
{
	int r;
	int s;
	s = spldma();									/* @@@ should this be splimp()? */
	r = (nd->_nd_map == NULL) && (nd->_nd_map_cont == NULL);
	splx(s);
	return(r);
}

void
nextdma_start(nd, dmadir)
	struct nextdma_config *nd;
	u_long dmadir;								/* 	DMACSR_SETREAD or DMACSR_SETWRITE */
{

#ifdef DIAGNOSTIC
	if (!nextdma_finished(nd)) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		panic("DMA trying to start before previous finished on intr(0x%s)\n", sbuf);
	}
#endif

#ifdef ND_DEBUG
	if (nextdma_debug) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		printf("DMA start (%ld) intr(0x%s)\n",
			NEXT_I_IPL(nd->nd_intr), sbuf);
	}
#endif

#ifdef DIAGNOSTIC
	if (nd->_nd_map) {
		next_dma_print(nd);
		panic("DMA: nextdma_start() with non null map\n");
	}
	if (nd->_nd_map_cont) {
		next_dma_print(nd);
		panic("DMA: nextdma_start() with non null continue map\n");
	}
#endif

#ifdef DIAGNOSTIC
	if ((dmadir != DMACSR_SETREAD) && (dmadir != DMACSR_SETWRITE)) {
		panic("DMA: nextdma_start(), dmadir arg must be DMACSR_SETREAD or DMACSR_SETWRITE\n");
	}
#endif

#if defined(ND_DEBUG)
	nextdma_debug_initstate(nd);
#endif

	/* preload both the current and the continue maps */
	next_dma_rotate(nd);

#ifdef DIAGNOSTIC
	if (!nd->_nd_map_cont) {
		panic("No map available in nextdma_start()");
	}
#endif

	next_dma_rotate(nd);

#ifdef ND_DEBUG
	if (nextdma_debug) {
		char sbuf[256];

		bitmask_snprintf(NEXT_I_BIT(nd->nd_intr), NEXT_INTR_BITS,
				 sbuf, sizeof(sbuf));
		printf("DMA initiating DMA %s of %d segments on intr(0x%s)\n",
			(dmadir == DMACSR_SETREAD ? "read" : "write"), nd->_nd_map->dm_nsegs, sbuf);
	}
#endif

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
			DMACSR_INITBUF | DMACSR_RESET | dmadir);

	next_dma_setup_curr_regs(nd);
	next_dma_setup_cont_regs(nd);

#if (defined(ND_DEBUG))
	if (nextdma_debug > 2) next_dma_print(nd);
#endif

	if (nd->_nd_map_cont == NULL) {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
				DMACSR_SETENABLE | dmadir);
	} else {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
				DMACSR_SETSUPDATE | DMACSR_SETENABLE | dmadir);
	}
}
