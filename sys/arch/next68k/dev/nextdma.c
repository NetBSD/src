/*	$NetBSD: nextdma.c,v 1.15.2.1 2000/01/21 00:31:32 he Exp $	*/
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

#define _GENERIC_BUS_DMA_PRIVATE
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

  /* @@@ for debugging */
struct nextdma_config *debugernd;
struct nextdma_config *debugexnd;

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
		t->_get_tag = NULL;           /* lose */
		t->_dmamap_create = _bus_dmamap_create;
		t->_dmamap_destroy = _bus_dmamap_destroy;
		t->_dmamap_load = _bus_dmamap_load_direct;
		t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf_direct;
		t->_dmamap_load_uio = _bus_dmamap_load_uio_direct;
		t->_dmamap_load_raw = _bus_dmamap_load_raw_direct;
		t->_dmamap_unload = _bus_dmamap_unload;
		t->_dmamap_sync = next_dmamap_sync;
  
		t->_dmamem_alloc = _bus_dmamem_alloc;
		t->_dmamem_free = _bus_dmamem_free;
		t->_dmamem_map = _bus_dmamem_map;
		t->_dmamem_unmap = _bus_dmamem_unmap;
		t->_dmamem_mmap = _bus_dmamem_mmap;

		nd->nd_dmat = t;
	}

  /* @@@ for debugging */
	if (nd->nd_intr == NEXT_I_ENETR_DMA) {
		debugernd = nd;
	}
	if (nd->nd_intr == NEXT_I_ENETX_DMA) {
		debugexnd = nd;
	}

	nextdma_init(nd);

	isrlink_autovec(nextdma_intr, nd, NEXT_I_IPL(nd->nd_intr), 10);
	INTR_ENABLE(nd->nd_intr);
}

void
nextdma_init(nd)
	struct nextdma_config *nd;
{
  DPRINTF(("DMA init ipl (%ld) intr(0x%b)\n",
			NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

	/* @@@ should probably check and free these maps */
	nd->_nd_map = NULL;
	nd->_nd_idx = 0;
	nd->_nd_map_cont = NULL;
	nd->_nd_idx_cont = 0;

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
			DMACSR_INITBUF | DMACSR_CLRCOMPLETE | DMACSR_RESET);

	next_dma_setup_curr_regs(nd);
	next_dma_setup_cont_regs(nd);

#if 0 && defined(DIAGNOSTIC)
	/* Today, my computer (mourning) appears to fail this test.
	 * yesterday, another NeXT (milo) didn't have this problem
	 * Darrin B. Jewell <jewell@mit.edu>  Mon May 25 07:53:05 1998
	 */
	{
		u_long state;
		state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);
		state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);
    state &= (DMACSR_BUSEXC | DMACSR_COMPLETE | 
              DMACSR_SUPDATE | DMACSR_ENABLE);

		if (state) {
			next_dma_print(nd);
			panic("DMA did not reset\n");
		}
	}
#endif
}


void
nextdma_reset(nd)
	struct nextdma_config *nd;
{
	int s;
	s = spldma();									/* @@@ should this be splimp()? */

	DPRINTF(("DMA reset\n"));

#if (defined(ND_DEBUG))
	if (nextdma_debug) next_dma_print(nd);
#endif

	nextdma_init(nd);
	splx(s);
}

/****************************************************************/

/* If the next had multiple busses, this should probably
 * go elsewhere, but it is here anyway */
void
next_dmamap_sync(t, map, offset, len, ops)
     bus_dma_tag_t t;
     bus_dmamap_t map;
     bus_addr_t offset;
     bus_size_t len;
     int ops;
{
	/* flush/purge the cache.
	 * assumes pointers are aligned
	 * @@@ should probably be fixed to use offset and len args.
	 * should also optimize this to work on pages for larger regions?
	 */

	if ((ops & BUS_DMASYNC_PREWRITE) ||
			(ops & BUS_DMASYNC_PREREAD)) {
		int i;
		for(i=0;i<map->dm_nsegs;i++) {
			bus_addr_t p = map->dm_segs[i].ds_addr;
			bus_addr_t e = p+map->dm_segs[i].ds_len;
#ifdef DIAGNOSTIC
			if ((p % 16) || (e % 16)) {
				panic("unaligned address in next_dmamap_sync while flushing.\n"
						"address=0x%08x, length=0x%08x, ops=0x%x",
						p,e,ops);
			}
#endif
			while(p<e) {
				DCFL(p);								/* flush */
				p += 16;								/* cache line length */
			}
		}
	}

	if ((ops & BUS_DMASYNC_POSTREAD) ||
			(ops & BUS_DMASYNC_POSTWRITE)) {
		int i;
		for(i=0;i<map->dm_nsegs;i++) {
			bus_addr_t p = map->dm_segs[i].ds_addr;
			bus_addr_t e = p+map->dm_segs[i].ds_len;
#ifdef DIAGNOSTIC
			/* We don't check the end address for alignment since if the
			 * dma operation stops short, the end address may be modified.
			 */
			if (p % 16) {
				panic("unaligned address in next_dmamap_sync while purging.\n"
						"address=0x%08x, length=0x%08x, ops=0x%x",
						p,e,ops);
			}
#endif
			while(p<e) {
				DCPL(p);								/* purge */
				p += 16;								/* cache line length */
			}
		}
	}
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

	/* If we've reached the end of the current map, then inform
	 * that we've completed that map.
	 */
	if (nd->_nd_map && ((nd->_nd_idx+1) == nd->_nd_map->dm_nsegs)) {
		if (nd->nd_completed_cb) 
			(*nd->nd_completed_cb)(nd->_nd_map, nd->nd_cb_arg);
	}

	/* Rotate the continue map into the current map */
	nd->_nd_map = nd->_nd_map_cont;
	nd->_nd_idx = nd->_nd_idx_cont;

	if ((!nd->_nd_map_cont) ||
			((nd->_nd_map_cont) &&
					(++nd->_nd_idx_cont >= nd->_nd_map_cont->dm_nsegs))) {
		if (nd->nd_continue_cb) {
			nd->_nd_map_cont = (*nd->nd_continue_cb)(nd->nd_cb_arg);
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
	DPRINTF(("DMA next_dma_setup_regs()\n"));

	if (nd->_nd_map_cont) {

		if (nd->nd_intr == NEXT_I_ENETX_DMA) {
			/* Ethernet transmit needs secret magic */

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_START,
					nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_STOP,
					((nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr +
							nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len)
							+ 0x0) | 0x80000000);

		}
#ifdef NEXTDMA_SCSI_HACK
		else if ((nd->nd_intr == NEXT_I_SCSI_DMA) && (nd->_nd_dmadir == DMACSR_WRITE)) {

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_START,
					nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr);

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_STOP,
					((nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr +
							nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len)
							+ 0x20));
    }
#endif
		else {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_START,
					nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_STOP,
					nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr +
					nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len);
		}

	} else {

		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_START, 0xdeadbeef);
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_STOP, 0xdeadbeef);
	}

#if 1 /* 0xfeedbeef in these registers leads to instability.  it will
			 * panic after a short while with 0xfeedbeef in the DD_START and DD_STOP
			 * registers.  I suspect that an unexpected hardware restart
			 * is cycling the bogus values into the active registers.  Until
			 * that is understood, we seed these with the same as DD_START and DD_STOP
			 */
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START, 
			bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_START));
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP,
			bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_STOP));
#else
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START, 0xfeedbeef);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP, 0xfeedbeef);
#endif

}

void
next_dma_setup_curr_regs(nd)
	struct nextdma_config *nd;
{
	DPRINTF(("DMA next_dma_setup_curr_regs()\n"));


	if (nd->_nd_map) {

		if (nd->nd_intr == NEXT_I_ENETX_DMA) {
			/* Ethernet transmit needs secret magic */

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT,
					((nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
							nd->_nd_map->dm_segs[nd->_nd_idx].ds_len)
							+ 0x0) | 0x80000000);

		}
#ifdef NEXTDMA_SCSI_HACK
		else if ((nd->nd_intr == NEXT_I_SCSI_DMA) && (nd->_nd_dmadir == DMACSR_WRITE)) {
			/* SCSI needs secret magic */

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT,
					((nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
							nd->_nd_map->dm_segs[nd->_nd_idx].ds_len)
							+ 0x20));

		}
#endif
		else {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);
		}

	} else {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,0xdeadbeef);
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT, 0xdeadbeef);
	}

#if 1  /* See comment in next_dma_setup_cont_regs() above */
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT, 
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF));
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT,
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT));
#else
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT, 0xfeedbeef);
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT, 0xfeedbeef);
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

	/* NDMAP is Next DMA Print (really!) */

	printf("NDMAP: nd->_nd_dmadir = 0x%08x\n",nd->_nd_dmadir);

	if (nd->_nd_map) {
		printf("NDMAP: nd->_nd_map->dm_mapsize = %d\n",
				nd->_nd_map->dm_mapsize);
		printf("NDMAP: nd->_nd_map->dm_nsegs = %d\n",
				nd->_nd_map->dm_nsegs);
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_len = %d\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);
	} else {
		printf("NDMAP: nd->_nd_map = NULL\n");
	}
	if (nd->_nd_map_cont) {
		printf("NDMAP: nd->_nd_map_cont->dm_mapsize = %d\n",
				nd->_nd_map_cont->dm_mapsize);
		printf("NDMAP: nd->_nd_map_cont->dm_nsegs = %d\n",
				nd->_nd_map_cont->dm_nsegs);
		printf("NDMAP: nd->_nd_map_cont->dm_segs[%d].ds_addr = 0x%08lx\n",
				nd->_nd_idx_cont,nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_addr);
		printf("NDMAP: nd->_nd_map_cont->dm_segs[%d].ds_len = %d\n",
				nd->_nd_idx_cont,nd->_nd_map_cont->dm_segs[nd->_nd_idx_cont].ds_len);
	} else {
		printf("NDMAP: nd->_nd_map_cont = NULL\n");
	}

	printf("NDMAP: dd->dd_csr          = 0x%b\n",   dd_csr,   DMACSR_BITS);
	printf("NDMAP: dd->dd_saved_next   = 0x%08x\n", dd_saved_next);
	printf("NDMAP: dd->dd_saved_limit  = 0x%08x\n", dd_saved_limit);
	printf("NDMAP: dd->dd_saved_start  = 0x%08x\n", dd_saved_start);
	printf("NDMAP: dd->dd_saved_stop   = 0x%08x\n", dd_saved_stop);
	printf("NDMAP: dd->dd_next         = 0x%08x\n", dd_next);
	printf("NDMAP: dd->dd_next_initbuf = 0x%08x\n", dd_next_initbuf);
	printf("NDMAP: dd->dd_limit        = 0x%08x\n", dd_limit);
	printf("NDMAP: dd->dd_start        = 0x%08x\n", dd_start);
	printf("NDMAP: dd->dd_stop         = 0x%08x\n", dd_stop);

	printf("NDMAP: interrupt ipl (%ld) intr(0x%b)\n",
			NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
}

/****************************************************************/

int
nextdma_intr(arg)
     void *arg;
{
  struct nextdma_config *nd = arg;

  /* @@@ This is bogus, we can't be certain of arg's type
	 * unless the interrupt is for us
	 */

  if (!INTR_OCCURRED(nd->nd_intr)) return 0;
  /* Handle dma interrupts */

#ifdef DIAGNOSTIC
	if (nd->nd_intr == NEXT_I_ENETR_DMA) {
		if (debugernd != nd) {
			panic("DMA incorrect handling of rx nd->nd_intr");
		}
	}
	if (nd->nd_intr == NEXT_I_ENETX_DMA) {
		if (debugexnd != nd) {
			panic("DMA incorrect handling of tx nd->nd_intr");
		}
	}
#endif

  DPRINTF(("DMA interrupt ipl (%ld) intr(0x%b)\n",
          NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

#ifdef DIAGNOSTIC
	if (!nd->_nd_map) {
		next_dma_print(nd);
		panic("DMA missing current map in interrupt!\n");
	}
#endif

  {
    int state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);

#ifdef DIAGNOSTIC
		if (!(state & DMACSR_COMPLETE)) {
			next_dma_print(nd);
			printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
			panic("DMA  ipl (%ld) intr(0x%b), DMACSR_COMPLETE not set in intr\n",
					NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
		}
#endif

#if 0 /* This bit gets set sometimes & I don't know why. */
#ifdef DIAGNOSTIC
		if (state & DMACSR_BUSEXC) {
			next_dma_print(nd);
			printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
			panic("DMA  ipl (%ld) intr(0x%b), DMACSR_COMPLETE not set in intr\n",
					NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
		}
#endif
#endif

		/* Check to see if we are expecting dma to shut down */
		if (!nd->_nd_map_cont) {

#ifdef DIAGNOSTIC
#if 1 /* Sometimes the DMA registers have totally bogus values when read.
			 * Until that's understood, we skip this check
			 */

			/* Verify that the registers are laid out as expected */
			{
				bus_addr_t next;
				bus_addr_t limit;
				bus_addr_t expected_limit;
				expected_limit = 
						nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
						nd->_nd_map->dm_segs[nd->_nd_idx].ds_len;

				if (nd->nd_intr == NEXT_I_ENETX_DMA) {
					next  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF);
					limit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT) & ~0x80000000;
				}
#ifdef NEXTDMA_SCSI_HACK
				else if ((nd->nd_intr == NEXT_I_SCSI_DMA) && (nd->_nd_dmadir == DMACSR_WRITE)) {
					next  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT) - 0x20;
					limit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT) - 0x20;
				}
#endif
				else {
					next  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT);
					limit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT);
				}

				if ((next != limit) || (limit != expected_limit)) {
					next_dma_print(nd);
					printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
					panic("unexpected DMA limit at shutdown 0x%08x, 0x%08x, 0x%08x",
							next,limit,expected_limit);
				}
			}
#endif
#endif

#if 1
#ifdef DIAGNOSTIC
			if (state & (DMACSR_SUPDATE|DMACSR_ENABLE)) {
				next_dma_print(nd);
				panic("DMA: unexpected bits set in DMA state at shutdown (0x%b)\n", 
						state,DMACSR_BITS);
			}
#endif
#endif

			if ((nd->_nd_idx+1) == nd->_nd_map->dm_nsegs) {
				if (nd->nd_completed_cb) 
					(*nd->nd_completed_cb)(nd->_nd_map, nd->nd_cb_arg);
			}
			nd->_nd_map = 0;
			nd->_nd_idx = 0;

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
					DMACSR_CLRCOMPLETE | DMACSR_RESET);
			
			DPRINTF(("DMA: a normal and expected shutdown occurred\n"));
			if (nd->nd_shutdown_cb) (*nd->nd_shutdown_cb)(nd->nd_cb_arg);

			return(1);
		}

#if 0
#ifdef DIAGNOSTIC
		if (!(state & DMACSR_SUPDATE)) {
			next_dma_print(nd);
			printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
			panic("SUPDATE not set with continuing DMA");
		}
#endif
#endif

		/* Check that the buffer we are interrupted for is the one we expect.
		 * Shorten the buffer if the dma completed with a short buffer
		 */
		{
			bus_addr_t next;
			bus_addr_t limit;
			bus_addr_t expected_next;
			bus_addr_t expected_limit;
			
			expected_next = nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr;
			expected_limit = expected_next + nd->_nd_map->dm_segs[nd->_nd_idx].ds_len;

#if 0 /* for some unknown reason, somtimes DD_SAVED_NEXT has value from
			 * nd->_nd_map and sometimes it has value from nd->_nd_map_cont.
			 * Somtimes, it has a completely different unknown value.
			 * Until that's understood, we won't sanity check the expected_next value.
			 */
			next  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT);
#else
			next  = expected_next;
#endif
			limit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);

			if (nd->nd_intr == NEXT_I_ENETX_DMA) {
				limit &= ~0x80000000;
			}
#ifdef NEXTDMA_SCSI_HACK
			else if ((nd->nd_intr == NEXT_I_SCSI_DMA) && (nd->_nd_dmadir == DMACSR_WRITE)) {
				limit -= 0x20;
			}
#endif
			
			if ((limit-next < 0) || 
					(limit-next >= expected_limit-expected_next)) {
#ifdef DIAGNOSTIC
#if 0 /* Sometimes, (under load I think) even DD_SAVED_LIMIT has
			 * a bogus value.  Until that's understood, we don't panic
			 * here.
			 */
				next_dma_print(nd);
				printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
				panic("Unexpected saved registers values.");
#endif
#endif
			} else {
				/* Set the length of the segment to match actual length. 
				 * @@@ is it okay to resize dma segments here?
				 * i should probably ask jason about this.
				 */
				nd->_nd_map->dm_segs[nd->_nd_idx].ds_len = limit-next;
				expected_limit = expected_next + nd->_nd_map->dm_segs[nd->_nd_idx].ds_len;
			}

#if 0 /* these checks are turned off until the above mentioned weirdness is fixed. */
#ifdef DIAGNOSTIC
			if (next != expected_next) {
				next_dma_print(nd);
				printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
				panic("unexpected DMA next buffer in interrupt (found 0x%08x, expected 0x%08x)",
						next,expected_next);
			}
			if (limit != expected_limit) {
				next_dma_print(nd);
				printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
				panic("unexpected DMA limit buffer in interrupt (found 0x%08x, expected 0x%08x)",
						limit,expected_limit);
			}
#endif
#endif
		}

		next_dma_rotate(nd);
		next_dma_setup_cont_regs(nd);

		if (nd->_nd_map_cont) {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
				DMACSR_SETSUPDATE | DMACSR_CLRCOMPLETE | DMACSR_SETENABLE | nd->_nd_dmadir);
		} else {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
				DMACSR_CLRCOMPLETE | DMACSR_SETENABLE | nd->_nd_dmadir);
		}

	}

  DPRINTF(("DMA exiting interrupt ipl (%ld) intr(0x%b)\n",
          NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

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
	u_long dmadir;								/* 	DMACSR_READ or DMACSR_WRITE */
{

#ifdef DIAGNOSTIC
	if (!nextdma_finished(nd)) {
		panic("DMA trying to start before previous finished on intr(0x%b)\n",
				NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
	}
#endif

  DPRINTF(("DMA start (%ld) intr(0x%b)\n",
          NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

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
	if ((dmadir != DMACSR_READ) && (dmadir != DMACSR_WRITE)) {
		panic("DMA: nextdma_start(), dmadir arg must be DMACSR_READ or DMACSR_WRITE\n");
	}
#endif

	nd->_nd_dmadir = dmadir;

	/* preload both the current and the continue maps */
	next_dma_rotate(nd);

#ifdef DIAGNOSTIC
	if (!nd->_nd_map_cont) {
		panic("No map available in nextdma_start()");
	}
#endif

	next_dma_rotate(nd);

	DPRINTF(("DMA initiating DMA %s of %d segments on intr(0x%b)\n",
			(nd->_nd_dmadir == DMACSR_READ ? "read" : "write"), nd->_nd_map->dm_nsegs,
			NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
			DMACSR_INITBUF | DMACSR_RESET | nd->_nd_dmadir);

	next_dma_setup_curr_regs(nd);
	next_dma_setup_cont_regs(nd);

#if (defined(ND_DEBUG))
	if (nextdma_debug) next_dma_print(nd);
#endif

	if (nd->_nd_map_cont) {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
				DMACSR_SETSUPDATE | DMACSR_SETENABLE | nd->_nd_dmadir);
	} else {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
				DMACSR_SETENABLE | nd->_nd_dmadir);
	}

}
