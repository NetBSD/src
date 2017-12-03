/*	$NetBSD: nextdma.c,v 1.47.18.3 2017/12/03 11:36:33 jdolecek Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nextdma.c,v 1.47.18.3 2017/12/03 11:36:33 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#define _M68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <m68k/cacheops.h>

#include <next68k/next68k/isr.h>
#include <next68k/next68k/nextrom.h>

#include <next68k/dev/intiovar.h>

#include "nextdmareg.h"
#include "nextdmavar.h"

#include "esp.h"
#include "xe.h"

#if DEBUG
#define ND_DEBUG
#endif

extern int turbo;

#define panic		__asm volatile("trap  #15"); printf

#define NEXTDMA_DEBUG nextdma_debug
/* (nsc->sc_chan->nd_intr == NEXT_I_SCSI_DMA) && nextdma_debug */
#if defined(ND_DEBUG)
int nextdma_debug = 0;
#define DPRINTF(x) if (NEXTDMA_DEBUG) printf x;
int ndtrace_show = 0;
char ndtrace_buf[8192+100];
size_t ndtrace_len = 0;
#define NDTRACEIF(x) if (10) do {x;} while (0)
#else
#define DPRINTF(x)
#define NDTRACEIF(x)
#endif
#define PRINTF(x) printf x

void
ndtrace_printf(const char *fmt, ...) {
#ifdef ND_DEBUG
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(ndtrace_buf + ndtrace_len, sizeof(ndtrace_buf)
	    - ndtrace_len, fmt, ap);
	va_end(ap);
	ndtrace_len += len;
#endif
}

int
ndtrace_empty(void) {
#ifdef ND_DEBUG
	return ndtrace_len == 0;
#else
	return 1;
#endif
}

void
ndtrace_reset(void) {
#ifdef ND_DEBUG
	ndtrace_len = 0;
#endif
}

void
ndtrace_addc(int c) {
#ifdef ND_DEBUG
	if (ndtrace_len < sizeof(ndtrace_buf) - 1) {
		ndtrace_buf[ndtrace_len++] = c;
		ndtrace_buf[ndtrace_len] = '\0';
	}
#endif
}

const char *
ndtrace_get(void) {
#ifdef ND_DEBUG
	return ndtrace_buf;
#else
	return NULL;
#endif
}


#if defined(ND_DEBUG)
int nextdma_debug_enetr_idx = 0;
unsigned int nextdma_debug_enetr_state[100] = { 0 };
int nextdma_debug_scsi_idx = 0;
unsigned int nextdma_debug_scsi_state[100] = { 0 };

void nextdma_debug_initstate(struct nextdma_softc *);
void nextdma_debug_savestate(struct nextdma_softc *, unsigned int);
void nextdma_debug_scsi_dumpstate(void);
void nextdma_debug_enetr_dumpstate(void);
#endif


int	nextdma_match(device_t, cfdata_t, void *);
void	nextdma_attach(device_t, device_t, void *);

void nextdmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t, bus_size_t, int);
int nextdma_continue(struct nextdma_softc *);
void nextdma_rotate(struct nextdma_softc *);

void nextdma_setup_cont_regs(struct nextdma_softc *);
void nextdma_setup_curr_regs(struct nextdma_softc *);

#if NESP > 0
static int nextdma_esp_intr(void *);
#endif
#if NXE > 0
static int nextdma_enet_intr(void *);
#endif

#define nd_bsr4(reg) \
	bus_space_read_4(nsc->sc_bst, nsc->sc_bsh, (reg))
#define nd_bsw4(reg,val) \
	bus_space_write_4(nsc->sc_bst, nsc->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(nextdma, sizeof(struct nextdma_softc),
    nextdma_match, nextdma_attach, NULL, NULL);

static struct nextdma_channel nextdma_channel[] = {
#if NESP > 0
	{ "scsi", NEXT_P_SCSI_CSR, DD_SIZE, NEXT_I_SCSI_DMA, &nextdma_esp_intr },
#endif
#if NXE > 0
	{ "enetx", NEXT_P_ENETX_CSR, DD_SIZE, NEXT_I_ENETX_DMA, &nextdma_enet_intr },
	{ "enetr", NEXT_P_ENETR_CSR, DD_SIZE, NEXT_I_ENETR_DMA, &nextdma_enet_intr },
#endif
};
static int nnextdma_channels = (sizeof(nextdma_channel)/sizeof(nextdma_channel[0]));

static int attached = 0;

struct nextdma_softc *
nextdma_findchannel(const char *name)
{
	device_t dev;
	deviter_t di;

	for (dev = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     dev != NULL;
	     dev = deviter_next(&di)) {
		if (strncmp(device_xname(dev), "nextdma", 7) == 0) {
			struct nextdma_softc *nsc = device_private(dev);
			if (strcmp(nsc->sc_chan->nd_name, name) == 0)
				break;
		}
	}
	deviter_release(&di);
	if (dev == NULL)
		return NULL;
	return device_private(dev);
}

int
nextdma_match(device_t parent, cfdata_t match, void *aux)
{
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;

	if (attached >= nnextdma_channels)
		return (0);

	ia->ia_addr = (void *)nextdma_channel[attached].nd_base;

	return (1);
}

void
nextdma_attach(device_t parent, device_t self, void *aux)
{
	struct nextdma_softc *nsc = device_private(self);
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;

	if (attached >= nnextdma_channels)
		return;

	nsc->sc_dev = self;
	nsc->sc_chan = &nextdma_channel[attached];

	nsc->sc_dmat = ia->ia_dmat;
	nsc->sc_bst = ia->ia_bst;

	if (bus_space_map(nsc->sc_bst, nsc->sc_chan->nd_base,
			  nsc->sc_chan->nd_size, 0, &nsc->sc_bsh)) {
		panic("%s: can't map DMA registers for channel %s",
		      device_xname(self), nsc->sc_chan->nd_name);
	}

	nextdma_init (nsc);

	isrlink_autovec(nsc->sc_chan->nd_intrfunc, nsc,
			NEXT_I_IPL(nsc->sc_chan->nd_intr), 10, NULL);
	INTR_ENABLE(nsc->sc_chan->nd_intr);

	printf (": channel %d (%s)\n", attached, 
		nsc->sc_chan->nd_name);
	attached++;

	return;
}

void
nextdma_init(struct nextdma_softc *nsc)
{
#ifdef ND_DEBUG
	if (NEXTDMA_DEBUG) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS,
		    NEXT_I_BIT(nsc->sc_chan->nd_intr));
		printf("DMA init ipl (%ld) intr(%s)\n",
			NEXT_I_IPL(nsc->sc_chan->nd_intr), sbuf);
	}
#endif

	nsc->sc_stat.nd_map = NULL;
	nsc->sc_stat.nd_idx = 0;
	nsc->sc_stat.nd_map_cont = NULL;
	nsc->sc_stat.nd_idx_cont = 0;
	nsc->sc_stat.nd_exception = 0;

	nd_bsw4 (DD_CSR, DMACSR_RESET | DMACSR_CLRCOMPLETE);
	nd_bsw4 (DD_CSR, 0);

#if 01
	nextdma_setup_curr_regs(nsc);
	nextdma_setup_cont_regs(nsc);
#endif

#if defined(DIAGNOSTIC)
	{
		u_long state;
		state = nd_bsr4 (DD_CSR);

#if 1
		/* mourning (a 25 MHz 68040 mono slab) appears to set BUSEXC
		 * milo (a 25 MHz 68040 mono cube) didn't have this problem
		 * Darrin B. Jewell <jewell@mit.edu>  Mon May 25 07:53:05 1998
		 */
		state &= (DMACSR_COMPLETE | DMACSR_SUPDATE | DMACSR_ENABLE);
#else
		state &= (DMACSR_BUSEXC | DMACSR_COMPLETE | 
			  DMACSR_SUPDATE | DMACSR_ENABLE);
#endif
		if (state) {
			nextdma_print(nsc);
			panic("DMA did not reset");
		}
	}
#endif
}

void
nextdma_reset(struct nextdma_softc *nsc)
{
	int s;
	struct nextdma_status *stat = &nsc->sc_stat;

	s = spldma();

	DPRINTF(("DMA reset\n"));

#if (defined(ND_DEBUG))
	if (NEXTDMA_DEBUG > 1) nextdma_print(nsc);
#endif

	nd_bsw4 (DD_CSR, DMACSR_CLRCOMPLETE | DMACSR_RESET);
	if ((stat->nd_map) || (stat->nd_map_cont)) {
		if (stat->nd_map_cont) {
			DPRINTF(("DMA: resetting with non null continue map\n"));
			if (nsc->sc_conf.nd_completed_cb) 
				(*nsc->sc_conf.nd_completed_cb)
					(stat->nd_map_cont, nsc->sc_conf.nd_cb_arg);
			
			stat->nd_map_cont = 0;
			stat->nd_idx_cont = 0;
		}
		if (nsc->sc_conf.nd_shutdown_cb)
			(*nsc->sc_conf.nd_shutdown_cb)(nsc->sc_conf.nd_cb_arg);
		stat->nd_map = 0;
		stat->nd_idx = 0;
	}

	splx(s);
}

/****************************************************************/


/* Call the completed and continue callbacks to try to fill
 * in the dma continue buffers.
 */
void
nextdma_rotate(struct nextdma_softc *nsc)
{
	struct nextdma_status *stat = &nsc->sc_stat;

	NDTRACEIF (ndtrace_addc('r'));
	DPRINTF(("DMA nextdma_rotate()\n"));

	/* Rotate the continue map into the current map */
	stat->nd_map = stat->nd_map_cont;
	stat->nd_idx = stat->nd_idx_cont;

	if ((!stat->nd_map_cont) ||
	    ((++stat->nd_idx_cont >= stat->nd_map_cont->dm_nsegs))) {
		if (nsc->sc_conf.nd_continue_cb) {
			stat->nd_map_cont = (*nsc->sc_conf.nd_continue_cb)
				(nsc->sc_conf.nd_cb_arg);
			if (stat->nd_map_cont) {
				stat->nd_map_cont->dm_xfer_len = 0;
			}
		} else {
			stat->nd_map_cont = 0;
		}
		stat->nd_idx_cont = 0;
	}

#if defined(DIAGNOSTIC) && 0
	if (stat->nd_map_cont) {
		if (!DMA_BEGINALIGNED(stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_addr)) {
			nextdma_print(nsc);
			panic("DMA request unaligned at start");
		}
		if (!DMA_ENDALIGNED(stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_addr + 
				stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_len)) {
			nextdma_print(nsc);
			panic("DMA request unaligned at end");
		}
	}
#endif

}

void
nextdma_setup_curr_regs(struct nextdma_softc *nsc)
{
	bus_addr_t dd_next;
	bus_addr_t dd_limit;
	bus_addr_t dd_saved_next;
	bus_addr_t dd_saved_limit;
	struct nextdma_status *stat = &nsc->sc_stat;

	NDTRACEIF (ndtrace_addc('C'));
	DPRINTF(("DMA nextdma_setup_curr_regs()\n"));

	if (stat->nd_map) {
		dd_next = stat->nd_map->dm_segs[stat->nd_idx].ds_addr;
		dd_limit = (stat->nd_map->dm_segs[stat->nd_idx].ds_addr +
			    stat->nd_map->dm_segs[stat->nd_idx].ds_len);

		if (!turbo && nsc->sc_chan->nd_intr == NEXT_I_ENETX_DMA) {
			dd_limit |= 0x80000000; /* Ethernet transmit needs secret magic */
			dd_limit += 15;
		}
	} else {
		dd_next = turbo ? 0 : 0xdeadbeef;
		dd_limit = turbo ? 0 : 0xdeadbeef;
	}

	dd_saved_next = dd_next;
	dd_saved_limit = dd_limit;

	NDTRACEIF (if (stat->nd_map) {
		ndtrace_printf("%ld", stat->nd_map->dm_segs[stat->nd_idx].ds_len);
	});

	if (!turbo && (nsc->sc_chan->nd_intr == NEXT_I_ENETX_DMA)) {
		nd_bsw4 (DD_NEXT_INITBUF, dd_next);
	} else {
		nd_bsw4 (DD_NEXT, dd_next);
	}
	nd_bsw4 (DD_LIMIT, dd_limit);
	if (!turbo) nd_bsw4 (DD_SAVED_NEXT, dd_saved_next);
	if (!turbo) nd_bsw4 (DD_SAVED_LIMIT, dd_saved_limit);

#ifdef DIAGNOSTIC
	if ((nd_bsr4 (DD_NEXT_INITBUF) != dd_next)
	    || (nd_bsr4 (DD_NEXT) != dd_next)
	    || (nd_bsr4 (DD_LIMIT) != dd_limit)
	    || (!turbo && (nd_bsr4 (DD_SAVED_NEXT) != dd_saved_next))
	    || (!turbo && (nd_bsr4 (DD_SAVED_LIMIT) != dd_saved_limit))
		) {
		nextdma_print(nsc);
		panic("DMA failure writing to current regs");
	}
#endif
}

void
nextdma_setup_cont_regs(struct nextdma_softc *nsc)
{
	bus_addr_t dd_start;
	bus_addr_t dd_stop;
	bus_addr_t dd_saved_start;
	bus_addr_t dd_saved_stop;
	struct nextdma_status *stat = &nsc->sc_stat;

	NDTRACEIF (ndtrace_addc('c'));
	DPRINTF(("DMA nextdma_setup_regs()\n"));

	if (stat->nd_map_cont) {
		dd_start = stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_addr;
		dd_stop  = (stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_addr +
			    stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_len);

		if (!turbo && nsc->sc_chan->nd_intr == NEXT_I_ENETX_DMA) {
			dd_stop |= 0x80000000; /* Ethernet transmit needs secret magic */
			dd_stop += 15;
		}
	} else {
		dd_start = turbo ? nd_bsr4 (DD_NEXT) : 0xdeadbee0;
		dd_stop = turbo ? 0 : 0xdeadbee0;
	}

	dd_saved_start = dd_start;
	dd_saved_stop  = dd_stop;

	NDTRACEIF (if (stat->nd_map_cont) {
		ndtrace_printf("%ld", stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_len);
	});

	nd_bsw4 (DD_START, dd_start);
	nd_bsw4 (DD_STOP, dd_stop);
	if (!turbo) nd_bsw4 (DD_SAVED_START, dd_saved_start);
	if (!turbo) nd_bsw4 (DD_SAVED_STOP, dd_saved_stop);
	if (turbo && nsc->sc_chan->nd_intr == NEXT_I_ENETR_DMA)
		nd_bsw4 (DD_STOP - 0x40, dd_start);

#ifdef DIAGNOSTIC
	if ((nd_bsr4 (DD_START) != dd_start)
	    || (dd_stop && (nd_bsr4 (DD_STOP) != dd_stop))
	    || (!turbo && (nd_bsr4 (DD_SAVED_START) != dd_saved_start))
	    || (!turbo && (nd_bsr4 (DD_SAVED_STOP) != dd_saved_stop))
		) {
		nextdma_print(nsc);
		panic("DMA failure writing to continue regs");
	}
#endif
}

/****************************************************************/

#if NESP > 0
static int
nextdma_esp_intr(void *arg)
{
	/* @@@ This is bogus, we can't be certain of arg's type
	 * unless the interrupt is for us.  For now we successfully
	 * cheat because DMA interrupts are the only things invoked
	 * at this interrupt level.
	 */
	struct nextdma_softc *nsc = arg;
	int esp_dma_int(void *); /* XXX */
		
	if (!INTR_OCCURRED(nsc->sc_chan->nd_intr))
		return 0;
	/* Handle dma interrupts */

	return esp_dma_int (nsc->sc_conf.nd_cb_arg);

}
#endif

#if NXE > 0
static int
nextdma_enet_intr(void *arg)
{
	/* @@@ This is bogus, we can't be certain of arg's type
	 * unless the interrupt is for us.  For now we successfully
	 * cheat because DMA interrupts are the only things invoked
	 * at this interrupt level.
	 */
	struct nextdma_softc *nsc = arg;
	unsigned int state;
	bus_addr_t onext;
	bus_addr_t olimit;
	bus_addr_t slimit;
	int result;
	struct nextdma_status *stat = &nsc->sc_stat;

	if (!INTR_OCCURRED(nsc->sc_chan->nd_intr))
		return 0;
	/* Handle dma interrupts */

	NDTRACEIF (ndtrace_addc('D'));
#ifdef ND_DEBUG
	if (NEXTDMA_DEBUG) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS,
		    NEXT_I_BIT(nsc->sc_chan->nd_intr));
		printf("DMA interrupt ipl (%ld) intr(%s)\n",
		       NEXT_I_IPL(nsc->sc_chan->nd_intr), sbuf);
	}
#endif

#ifdef DIAGNOSTIC
	if (!stat->nd_map) {
		nextdma_print(nsc);
		panic("DMA missing current map in interrupt!");
	}
#endif

	state = nd_bsr4 (DD_CSR);

#if defined(ND_DEBUG)
	nextdma_debug_savestate(nsc, state);
#endif

#ifdef DIAGNOSTIC
	if (/* (state & DMACSR_READ) || */ !(state & DMACSR_COMPLETE)) {
		char sbuf[256];
		nextdma_print(nsc);
		snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, state);
		printf("DMA: state %s\n",sbuf);
		panic("DMA complete not set in interrupt");
	}
#endif

	DPRINTF(("DMA: finishing xfer\n"));
			
	onext = stat->nd_map->dm_segs[stat->nd_idx].ds_addr;
	olimit = onext + stat->nd_map->dm_segs[stat->nd_idx].ds_len;

	result = 0;
	if (state & DMACSR_ENABLE) {
		/* enable bit was set */
		result |= 0x01;
	}
	if (state & DMACSR_SUPDATE) {
		/* supdate bit was set */
		result |= 0x02;
	}
	if (stat->nd_map_cont == NULL) {
		KASSERT(stat->nd_idx+1 == stat->nd_map->dm_nsegs);
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
		if (turbo) {
			volatile u_int *limit = (volatile u_int *)IIOV(0x2000050+0x4000);
			slimit = *limit;
		} else {
			slimit = nd_bsr4 (DD_SAVED_LIMIT);
		}
		break;
	case 0x01: /* !BUSEXC && !expecting && !SUPDATE && ENABLE */
	case 0x09: /* BUSEXC && !expecting && !SUPDATE && ENABLE */
		if (turbo) {
			volatile u_int *limit = (volatile u_int *)IIOV(0x2000050+0x4000);
			slimit = *limit;
		} else {
			slimit = nd_bsr4 (DD_SAVED_LIMIT);
		}
		break;
	case 0x02: /* !BUSEXC && !expecting && SUPDATE && !ENABLE */
	case 0x0a: /* BUSEXC && !expecting && SUPDATE && !ENABLE */ 
		slimit = nd_bsr4 (DD_NEXT);
		break;
	case 0x04:  /* !BUSEXC && expecting && !SUPDATE && !ENABLE */
	case 0x0c: /* BUSEXC && expecting && !SUPDATE && !ENABLE */ 
		slimit = nd_bsr4 (DD_LIMIT);
		break;
	default:
#ifdef DIAGNOSTIC
	{
		char sbuf[256];
		printf("DMA: please send this output to port-next68k-maintainer@NetBSD.org:\n");
		snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, state);
		printf("DMA: state %s\n",sbuf);
		nextdma_print(nsc);
		panic("DMA: condition 0x%02x not yet documented to occur",result);
	}
#endif
	slimit = olimit;
	break;
	}

	if (!turbo && nsc->sc_chan->nd_intr == NEXT_I_ENETX_DMA) {
		slimit &= ~0x80000000;
		slimit -= 15;
	}

#ifdef DIAGNOSTIC
	if ((state & DMACSR_READ))
		DPRINTF (("limits: 0x%08lx <= 0x%08lx <= 0x%08lx %s\n", onext, slimit, olimit,
			  (state & DMACSR_READ) ? "read" : "write"));
	if ((slimit < onext) || (slimit > olimit)) {
		char sbuf[256];
		snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, state);
		printf("DMA: state %s\n",sbuf);
		nextdma_print(nsc);
		panic("DMA: Unexpected limit register (0x%08lx) in finish_xfer",slimit);
	}
#endif

#ifdef DIAGNOSTIC
	if ((state & DMACSR_ENABLE) && ((stat->nd_idx+1) != stat->nd_map->dm_nsegs)) {
		if (slimit != olimit) {
			char sbuf[256];
			snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, state);
			printf("DMA: state %s\n",sbuf);
			nextdma_print(nsc);
			panic("DMA: short limit register (0x%08lx) w/o finishing map.",slimit);
		}
	}
#endif

#if (defined(ND_DEBUG))
	if (NEXTDMA_DEBUG > 2) nextdma_print(nsc);
#endif

	stat->nd_map->dm_xfer_len += slimit-onext;

	/* If we've reached the end of the current map, then inform
	 * that we've completed that map.
	 */
	if ((stat->nd_idx+1) == stat->nd_map->dm_nsegs) {
		if (nsc->sc_conf.nd_completed_cb) 
			(*nsc->sc_conf.nd_completed_cb)
				(stat->nd_map, nsc->sc_conf.nd_cb_arg);
	} else {
		KASSERT(stat->nd_map == stat->nd_map_cont);
		KASSERT(stat->nd_idx+1 == stat->nd_idx_cont);
	}
	stat->nd_map = 0;
	stat->nd_idx = 0;

#if (defined(ND_DEBUG))
	if (NEXTDMA_DEBUG) {
		char sbuf[256];
		snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, state);
		printf("CLNDMAP: dd->dd_csr          = %s\n",   sbuf);
	}
#endif
	if (state & DMACSR_ENABLE) {
		u_long dmadir;		/* DMACSR_SETREAD or DMACSR_SETWRITE */

		nextdma_rotate(nsc);
		nextdma_setup_cont_regs(nsc);
		
		if (state & DMACSR_READ) {
			dmadir = DMACSR_SETREAD;
		} else {
			dmadir = DMACSR_SETWRITE;
		}

		if (stat->nd_map_cont == NULL) {
			KASSERT(stat->nd_idx+1 == stat->nd_map->dm_nsegs);
			nd_bsw4 (DD_CSR, DMACSR_CLRCOMPLETE | dmadir);
			NDTRACEIF (ndtrace_addc('g'));
		} else {
			nd_bsw4 (DD_CSR, DMACSR_CLRCOMPLETE | dmadir | DMACSR_SETSUPDATE);
			NDTRACEIF (ndtrace_addc('G'));
		}
	} else {
		DPRINTF(("DMA: a shutdown occurred\n"));
		nd_bsw4 (DD_CSR, DMACSR_CLRCOMPLETE | DMACSR_RESET);
		
		/* Cleanup more incomplete transfers */
		/* cleanup continue map */
		if (stat->nd_map_cont) {
			DPRINTF(("DMA: shutting down with non null continue map\n"));
			if (nsc->sc_conf.nd_completed_cb) 
				(*nsc->sc_conf.nd_completed_cb)
					(stat->nd_map_cont, nsc->sc_conf.nd_cb_arg);
								
			stat->nd_map_cont = 0;
			stat->nd_idx_cont = 0;
		}
		if (nsc->sc_conf.nd_shutdown_cb) 
			(*nsc->sc_conf.nd_shutdown_cb)(nsc->sc_conf.nd_cb_arg);
	}
	
#ifdef ND_DEBUG
	if (NEXTDMA_DEBUG) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf),
		    NEXT_INTR_BITS, NEXT_I_BIT(nsc->sc_chan->nd_intr));
		printf("DMA exiting interrupt ipl (%ld) intr(%s)\n",
		       NEXT_I_IPL(nsc->sc_chan->nd_intr), sbuf);
	}
#endif
	
	return(1);
}
#endif

/*
 * Check to see if dma has finished for a channel */
int
nextdma_finished(struct nextdma_softc *nsc)
{
	int r;
	int s;
	struct nextdma_status *stat = &nsc->sc_stat;

	s = spldma();
	r = (stat->nd_map == NULL) && (stat->nd_map_cont == NULL);
	splx(s);

	return(r);
}

void
nextdma_start(struct nextdma_softc *nsc, u_long dmadir)
{
	struct nextdma_status *stat = &nsc->sc_stat;

	NDTRACEIF (ndtrace_addc('n'));
#ifdef DIAGNOSTIC
	if (!nextdma_finished(nsc)) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf),
		    NEXT_INTR_BITS, NEXT_I_BIT(nsc->sc_chan->nd_intr));
		panic("DMA trying to start before previous finished on intr(%s)", sbuf);
	}
#endif

#ifdef ND_DEBUG
	if (NEXTDMA_DEBUG) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf),
		    NEXT_INTR_BITS, NEXT_I_BIT(nsc->sc_chan->nd_intr));
		printf("DMA start (%ld) intr(%s)\n",
		       NEXT_I_IPL(nsc->sc_chan->nd_intr), sbuf);
	}
#endif

#ifdef DIAGNOSTIC
	if (stat->nd_map) {
		nextdma_print(nsc);
		panic("DMA: nextdma_start() with non null map");
	}
	if (stat->nd_map_cont) {
		nextdma_print(nsc);
		panic("DMA: nextdma_start() with non null continue map");
	}
#endif

#ifdef DIAGNOSTIC
	if ((dmadir != DMACSR_SETREAD) && (dmadir != DMACSR_SETWRITE)) {
		panic("DMA: nextdma_start(), dmadir arg must be DMACSR_SETREAD or DMACSR_SETWRITE");
	}
#endif

#if defined(ND_DEBUG)
	nextdma_debug_initstate(nsc);
#endif

	/* preload both the current and the continue maps */
	nextdma_rotate(nsc);

#ifdef DIAGNOSTIC
	if (!stat->nd_map_cont) {
		panic("No map available in nextdma_start()");
	}
#endif

	nextdma_rotate(nsc);

#ifdef ND_DEBUG
	if (NEXTDMA_DEBUG) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf),
		    NEXT_INTR_BITS, NEXT_I_BIT(nsc->sc_chan->nd_intr));
		printf("DMA initiating DMA %s of %d segments on intr(%s)\n",
		       (dmadir == DMACSR_SETREAD ? "read" : "write"), stat->nd_map->dm_nsegs, sbuf);
	}
#endif

	nd_bsw4 (DD_CSR, (turbo ? DMACSR_INITBUFTURBO : DMACSR_INITBUF) |
		 DMACSR_RESET | dmadir);
	nd_bsw4 (DD_CSR, 0);

	nextdma_setup_curr_regs(nsc);
	nextdma_setup_cont_regs(nsc);

#if (defined(ND_DEBUG))
	if (NEXTDMA_DEBUG > 2) nextdma_print(nsc);
#endif

	if (stat->nd_map_cont == NULL) {
		nd_bsw4 (DD_CSR, DMACSR_SETENABLE | dmadir);
	} else {
		nd_bsw4 (DD_CSR, DMACSR_SETSUPDATE | DMACSR_SETENABLE | dmadir);
	}
}

/* This routine is used for debugging */
void
nextdma_print(struct nextdma_softc *nsc)
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
	struct nextdma_status *stat = &nsc->sc_stat;

	/* Read all of the registers before we print anything out,
	 * in case something changes
	 */
	dd_csr          = nd_bsr4 (DD_CSR);
	dd_next         = nd_bsr4 (DD_NEXT);
	dd_next_initbuf = nd_bsr4 (DD_NEXT_INITBUF);
	dd_limit        = nd_bsr4 (DD_LIMIT);
	dd_start        = nd_bsr4 (DD_START);
	dd_stop         = nd_bsr4 (DD_STOP);
	dd_saved_next   = nd_bsr4 (DD_SAVED_NEXT);
	dd_saved_limit  = nd_bsr4 (DD_SAVED_LIMIT);
	dd_saved_start  = nd_bsr4 (DD_SAVED_START);
	dd_saved_stop   = nd_bsr4 (DD_SAVED_STOP);

	snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS,
	    *(volatile u_long *)IIOV(NEXT_P_INTRSTAT));
	printf("NDMAP: *intrstat = %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS,
	    *(volatile u_long *)IIOV(NEXT_P_INTRMASK));
	printf("NDMAP: *intrmask = %s\n", sbuf);

	/* NDMAP is Next DMA Print (really!) */

	if (stat->nd_map) {
		int i;

		printf("NDMAP: nd_map->dm_mapsize = %ld\n",
		       stat->nd_map->dm_mapsize);
		printf("NDMAP: nd_map->dm_nsegs = %d\n",
		       stat->nd_map->dm_nsegs);
		printf("NDMAP: nd_map->dm_xfer_len = %ld\n",
		       stat->nd_map->dm_xfer_len);
		printf("NDMAP: nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
		       stat->nd_idx, stat->nd_map->dm_segs[stat->nd_idx].ds_addr);
		printf("NDMAP: nd_map->dm_segs[%d].ds_len = %ld\n",
		       stat->nd_idx, stat->nd_map->dm_segs[stat->nd_idx].ds_len);

		printf("NDMAP: Entire map;\n");
		for(i=0;i<stat->nd_map->dm_nsegs;i++) {
			printf("NDMAP:   nd_map->dm_segs[%d].ds_addr = 0x%08lx\n",
			       i,stat->nd_map->dm_segs[i].ds_addr);
			printf("NDMAP:   nd_map->dm_segs[%d].ds_len = %ld\n",
			       i,stat->nd_map->dm_segs[i].ds_len);
		}
	} else {
		printf("NDMAP: nd_map = NULL\n");
	}
	if (stat->nd_map_cont) {
		printf("NDMAP: nd_map_cont->dm_mapsize = %ld\n",
		       stat->nd_map_cont->dm_mapsize);
		printf("NDMAP: nd_map_cont->dm_nsegs = %d\n",
		       stat->nd_map_cont->dm_nsegs);
		printf("NDMAP: nd_map_cont->dm_xfer_len = %ld\n",
		       stat->nd_map_cont->dm_xfer_len);
		printf("NDMAP: nd_map_cont->dm_segs[%d].ds_addr = 0x%08lx\n",
		       stat->nd_idx_cont,stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_addr);
		printf("NDMAP: nd_map_cont->dm_segs[%d].ds_len = %ld\n",
		       stat->nd_idx_cont,stat->nd_map_cont->dm_segs[stat->nd_idx_cont].ds_len);
		if (stat->nd_map_cont != stat->nd_map) {
			int i;
			printf("NDMAP: Entire map;\n");
			for(i=0;i<stat->nd_map_cont->dm_nsegs;i++) {
				printf("NDMAP:   nd_map_cont->dm_segs[%d].ds_addr = 0x%08lx\n",
				       i,stat->nd_map_cont->dm_segs[i].ds_addr);
				printf("NDMAP:   nd_map_cont->dm_segs[%d].ds_len = %ld\n",
				       i,stat->nd_map_cont->dm_segs[i].ds_len);
			}
		}
	} else {
		printf("NDMAP: nd_map_cont = NULL\n");
	}

	snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, dd_csr);
	printf("NDMAP: dd->dd_csr          = %s\n",   sbuf);

	printf("NDMAP: dd->dd_saved_next   = 0x%08lx\n", dd_saved_next);
	printf("NDMAP: dd->dd_saved_limit  = 0x%08lx\n", dd_saved_limit);
	printf("NDMAP: dd->dd_saved_start  = 0x%08lx\n", dd_saved_start);
	printf("NDMAP: dd->dd_saved_stop   = 0x%08lx\n", dd_saved_stop);
	printf("NDMAP: dd->dd_next         = 0x%08lx\n", dd_next);
	printf("NDMAP: dd->dd_next_initbuf = 0x%08lx\n", dd_next_initbuf);
	printf("NDMAP: dd->dd_limit        = 0x%08lx\n", dd_limit);
	printf("NDMAP: dd->dd_start        = 0x%08lx\n", dd_start);
	printf("NDMAP: dd->dd_stop         = 0x%08lx\n", dd_stop);

	snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS,
	    NEXT_I_BIT(nsc->sc_chan->nd_intr));
	printf("NDMAP: interrupt ipl (%ld) intr(%s)\n",
			NEXT_I_IPL(nsc->sc_chan->nd_intr), sbuf);
}

#if defined(ND_DEBUG)
void
nextdma_debug_initstate(struct nextdma_softc *nsc)
{
	switch(nsc->sc_chan->nd_intr) {
	case NEXT_I_ENETR_DMA:
		memset(nextdma_debug_enetr_state,0,sizeof(nextdma_debug_enetr_state));
		break;
	case NEXT_I_SCSI_DMA:
		memset(nextdma_debug_scsi_state,0,sizeof(nextdma_debug_scsi_state));
		break;
	}
}

void
nextdma_debug_savestate(struct nextdma_softc *nsc, unsigned int state)
{
	switch(nsc->sc_chan->nd_intr) {
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
			snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, nextdma_debug_enetr_state[i]);
			printf("DMA: 0x%02x state %s\n",i,sbuf);
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
			snprintb(sbuf, sizeof(sbuf), DMACSR_BITS, nextdma_debug_scsi_state[i]);
			printf("DMA: 0x%02x state %s\n",i,sbuf);
		}
		i++;
		i %= (sizeof(nextdma_debug_scsi_state)/sizeof(unsigned int));
	} while (i != nextdma_debug_scsi_idx);
	splx(s);
}
#endif

