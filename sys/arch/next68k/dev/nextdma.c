/*	$NetBSD: nextdma.c,v 1.6 1998/12/08 09:35:07 dbj Exp $	*/
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

#if 0
#define ND_DEBUG
#endif

#if defined(ND_DEBUG)
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

  /* @@@ for debugging */
struct nextdma_config *debugernd;
struct nextdma_config *debugexnd;

int nextdma_intr __P((void *));
void next_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
                       bus_size_t, int));
int next_dma_continue __P((struct nextdma_config *));
void next_dma_rotate __P((struct nextdma_config *));

void next_dma_setup_cont_regs __P((struct nextdma_config *));
void next_dma_setup_curr_regs __P((struct nextdma_config *));

void next_dma_print __P((struct nextdma_config *));

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
	if (ops & BUS_DMASYNC_PREWRITE) {
		int i;
		for(i=0;i<map->dm_nsegs;i++) {
			bus_addr_t p = map->dm_segs[i].ds_addr;
			bus_addr_t e = p+map->dm_segs[i].ds_len;
			while(p<e) {
				DCFL(p);								/* flush */
				p += 16;								/* cache line length */
			}
		}
	}

	if (ops & BUS_DMASYNC_POSTREAD) {
		int i;
		for(i=0;i<map->dm_nsegs;i++) {
			bus_addr_t p = map->dm_segs[i].ds_addr;
			bus_addr_t e = p+map->dm_segs[i].ds_len;
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
		} else {
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

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_START, 
			bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_START));
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_STOP,
			bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_STOP));

}

void
next_dma_setup_curr_regs(nd)
	struct nextdma_config *nd;
{
	DPRINTF(("DMA next_dma_setup_curr_regs()\n"));

	if (nd->nd_intr == NEXT_I_ENETX_DMA) {
			/* Ethernet transmit needs secret magic */

		if (nd->_nd_map) {

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT,
					((nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
							nd->_nd_map->dm_segs[nd->_nd_idx].ds_len)
							+ 0x0) | 0x80000000);
		} else {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF,0xdeadbeef);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT, 0xdeadbeef);

		}
			
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT, 
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT_INITBUF));
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT,
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT));
		
	} else {

		if (nd->_nd_map) {

			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT,
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr +
					nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);
		} else {
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_NEXT, 0xdeadbeef);
			bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT, 0xdeadbeef);

		}

		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT, 
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_NEXT));
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT,
				bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_LIMIT));

	}

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

	if (nd->_nd_map) {
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr);
		printf("NDMAP: nd->_nd_map->dm_segs[%d].ds_len = %d\n",
				nd->_nd_idx,nd->_nd_map->dm_segs[nd->_nd_idx].ds_len);
	} else {
		printf("NDMAP: nd->_nd_map = NULL\n");
	}
	if (nd->_nd_map_cont) {
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

  {
    int state = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_CSR);

    state &= (DMACSR_BUSEXC | DMACSR_COMPLETE | 
              DMACSR_SUPDATE | DMACSR_ENABLE);

    if (state & DMACSR_BUSEXC) {
#if 0 /* This bit seems to get set periodically and I don't know why */
			next_dma_print(nd);
      panic("Bus exception in DMA ipl (%ld) intr(0x%b)\n",
             NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
#endif
    }

#ifdef DIAGNOSTIC
		if (!(state & DMACSR_COMPLETE)) {
			next_dma_print(nd);
#if 0 /* This bit doesn't seem to get set every once in a while,
			 * and I don't know why.  Let's try treating it as a spurious
			 * interrupt.  ie. report it and ignore the interrupt.
			 */
			printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
			panic("DMA  ipl (%ld) intr(0x%b), DMACSR_COMPLETE not set in intr\n",
					NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
#else
			printf("DMA  ipl (%ld) intr(0x%b), DMACSR_COMPLETE not set in intr\n",
					NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
			return(1);
#endif
		}
#endif

		/* Set the length of the segment to match actual length. 
		 * @@@ is it okay to resize dma segments here?
		 * i should probably ask jason about this.
		 */
		if (nd->_nd_map) {

			bus_addr_t next;
			bus_addr_t limit;

#if 0
			if (state & DMACSR_ENABLE) {
				next  = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_NEXT);
			} else {
				next  = nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr;
			}
#else
			next  = nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr;
#endif
			limit = bus_space_read_4(nd->nd_bst, nd->nd_bsh, DD_SAVED_LIMIT);

			if (nd->nd_intr == NEXT_I_ENETX_DMA) {
				limit &= ~0x80000000;
			}

#ifdef DIAGNOSTIC
			if (next != nd->_nd_map->dm_segs[nd->_nd_idx].ds_addr) {
				next_dma_print(nd);
				printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);

				panic("DMA  ipl (%ld) intr(0x%b), unexpected completed address\n",
						NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS);
			}
#endif

			/* @@@ I observed a case where DMACSR_ENABLE wasn't set and
			 * DD_SAVED_LIMIT didn't contain the expected limit value.  This
			 * should be tested, fixed, and removed.  */

			if (((limit-next) > nd->_nd_map->dm_segs[nd->_nd_idx].ds_len)
					|| (limit-next < 0)) {
#if 0
				next_dma_print(nd);
				printf("DEBUG: state = 0x%b\n", state,DMACSR_BITS);
				panic("DMA packlen: next = 0x%08x limit = 0x%08x\n",next,limit);
#else
				DPRINTF(("DMA packlen: next = 0x%08x limit = 0x%08x",next,limit));
#endif
				
			} else {
				nd->_nd_map->dm_segs[nd->_nd_idx].ds_len = limit - next;
			}
		}


		if ((state & DMACSR_ENABLE) == 0) {

			/* Non chaining interrupts shutdown immediately */
			if (!nd->nd_chaining_flag) {
				nd->_nd_map = nd->_nd_map_cont;
				nd->_nd_idx = nd->_nd_idx_cont;
				nd->_nd_map_cont = 0;
				nd->_nd_idx_cont = 0;
			}

			/* Call the completed callback for the last packet */
			if (nd->_nd_map && ((nd->_nd_idx+1) == nd->_nd_map->dm_nsegs)) {
				if (nd->nd_completed_cb) 
					(*nd->nd_completed_cb)(nd->_nd_map, nd->nd_cb_arg);
			}
			nd->_nd_map = 0;
			nd->_nd_idx = 0;

			if (nd->_nd_map_cont) {
				DPRINTF(("DMA  ipl (%ld) intr(0x%b), restarting\n",
					NEXT_I_IPL(nd->nd_intr), NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
						DMACSR_SETSUPDATE | DMACSR_SETENABLE);

			} else {
				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
						DMACSR_CLRCOMPLETE | DMACSR_RESET);
				DPRINTF(("DMA: enable not set w/o continue map, shutting down dma\n"));
				if (nd->nd_shutdown_cb) (*nd->nd_shutdown_cb)(nd->nd_cb_arg);
			}

		} else {
			next_dma_rotate(nd);
			next_dma_setup_cont_regs(nd);

			if (nd->_nd_map_cont) {
				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
						DMACSR_SETSUPDATE | DMACSR_CLRCOMPLETE);
			} else {
				bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
						DMACSR_CLRCOMPLETE);
			}

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

	next_dma_rotate(nd);

#ifdef DIAGNOSTIC
	if (!nd->_nd_map_cont) {
		panic("No map available in nextdma_start()");
	}
	if (!DMA_BEGINALIGNED(nd->_nd_map_cont->dm_segs[nd->_nd_idx].ds_addr)) {
		panic("unaligned begin dma at start\n");
	}
	if (!DMA_ENDALIGNED(nd->_nd_map_cont->dm_segs[nd->_nd_idx].ds_addr + 
			nd->_nd_map_cont->dm_segs[nd->_nd_idx].ds_len)) {
		panic("unaligned end dma at start\n");
	}
#endif

	DPRINTF(("DMA initiating DMA %s of %d segments on intr(0x%b)\n",
			(dmadir == DMACSR_READ ? "read" : "write"), nd->_nd_map_cont->dm_nsegs,
			NEXT_I_BIT(nd->nd_intr),NEXT_INTR_BITS));

	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 0);
	bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
			DMACSR_INITBUF | DMACSR_RESET | dmadir);

	next_dma_setup_cont_regs(nd);

	/* When starting DMA, we must put the continue map
	 * into the current register.  We reset the nd->_nd_map
	 * pointer here to avoid duplicated completed callbacks
	 * for the first buffer.
	 */
	nd->_nd_map = nd->_nd_map_cont;
	nd->_nd_idx = nd->_nd_idx_cont;
	next_dma_setup_curr_regs(nd);
	nd->_nd_map = 0;
	nd->_nd_idx = 0;


#if (defined(ND_DEBUG))
	next_dma_print(nd);
#endif

	if (nd->nd_chaining_flag) {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR,
				DMACSR_SETSUPDATE | DMACSR_SETENABLE);
	} else {
		bus_space_write_4(nd->nd_bst, nd->nd_bsh, DD_CSR, 
				DMACSR_SETENABLE);
	}

}
