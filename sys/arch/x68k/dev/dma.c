/*	$NetBSD: dma.c,v 1.2 1998/05/23 20:51:13 is Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_m68kcpu.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <vm/vm.h>

#include <machine/cpu.h>

#include <x68k/x68k/iodevice.h>
#include <x68k/dev/dmavar.h>

#define NDMA 4

/* region of physical memory known to be contiguous */
caddr_t dma_dataaddr[NDMA];
caddr_t dma_bouncebuf[NDMA];
vm_size_t dma_bouncebytes[NDMA];
char dma_bounced[NDMA];

/*
 * Check for problems with the address range of a DMA transfer
 * (non-contiguous physical pages, outside of bus address space,
 * crossing DMA page boundaries).
 * Return true if special handling needed.
 */
int
dmarangecheck(va, length)
	vm_offset_t va;
	u_long length;
{
	vm_offset_t phys, priorpage = 0, endva;
	u_int dma_pgmsk = ~PGOFSET;

	endva = round_page(va + length);
	for (; va < endva ; va += NBPG) {
		phys = trunc_page(pmap_extract(pmap_kernel(), va));
		if (phys == 0)
			panic("dmacheck: no physical page present");
		if (phys >= (1<<24)) 
			return 1;		/* XXX */
		if (priorpage) {
			if (priorpage + NBPG != phys)
				return 1;
			/* check if crossing a DMA page boundary */
			if ((priorpage ^ phys) & dma_pgmsk)
				return 1;
		}
		priorpage = phys;
	}
	return 0;
}

/*
 * program HD63450 DMAC channel.
 */
void
x68k_dmastart(flag, addr, nbytes, chan)
	int flag;
	caddr_t addr;
	int nbytes;
	int chan;
{
	volatile struct dmac *dmac = &IODEVbase->io_dma[chan];

	if (dmarangecheck((vm_offset_t)addr, nbytes)) {
		dma_bouncebytes[chan] = nbytes;
		dma_dataaddr[chan] = addr;
		if (!(flag)) {
			bcopy(addr, dma_bouncebuf[chan], nbytes);
			dma_bounced[chan] = DMA_BWR;
		} else {
			dma_bounced[chan] = DMA_BRD;
		}
		addr = dma_bouncebuf[chan];
	} else {
		dma_bounced[chan] = 0;
	}
	dmac->csr = 0xff;
	dmac->ocr = flag ? 0xb2 : 0x32;
	dmac->mtc = (unsigned short)nbytes;
	asm("nop");
	asm("nop");
	dmac->mar = (unsigned long)kvtop(addr);
#if defined(M68040)
		/*
		 * Push back dirty cache lines
		 */
		if (mmutype == MMU_68040)
			DCFP(kvtop(addr));
#endif
	dmac->ccr = 0x88;
}
#if 0
void
isa_dmadone(flags, addr, nbytes, chan)
	int flags;
	caddr_t addr;
	vm_size_t nbytes;
	int chan;
{
	u_char tc;

#ifdef DIAGNOSTIC
	if (chan < 0 || chan > 7)
		panic("isa_dmadone: impossible request");
#endif

	/* check that the terminal count was reached */
	if ((chan & 4) == 0)
		tc = inb(DMA1_SR) & (1 << chan);
	else
		tc = inb(DMA2_SR) & (1 << (chan & 3));
	if (tc == 0)
		/* XXX probably should panic or something */
		log(LOG_ERR, "dma channel %d not finished\n", chan);

	/* copy bounce buffer on read */
	if (dma_bounced[chan]) {
		bcopy(dma_bounce[chan], addr, nbytes);
		dma_bounced[chan] = 0;
	}

	/* mask channel */
	if ((chan & 4) == 0)
		outb(DMA1_SMSK, DMA37SM_SET | chan);
	else
		outb(DMA2_SMSK, DMA37SM_SET | (chan & 3));
}

/* head of queue waiting for physmem to become available */
struct buf isa_physmemq;

/* blocked waiting for resource to become free for exclusive use */
static isaphysmemflag;
/* if waited for and call requested when free (B_CALL) */
static void (*isaphysmemunblock)(); /* needs to be a list */

/*
 * Allocate contiguous physical memory for transfer, returning
 * a *virtual* address to region. May block waiting for resource.
 * (assumed to be called at splbio())
 */
caddr_t
isa_allocphysmem(caddr_t va, unsigned length, void (*func)()) {
	
	isaphysmemunblock = func;
	while (isaphysmemflag & B_BUSY) {
		isaphysmemflag |= B_WANTED;
		sleep((caddr_t)&isaphysmemflag, PRIBIO);
	}
	isaphysmemflag |= B_BUSY;

	return((caddr_t)isaphysmem);
}

/*
 * Free contiguous physical memory used for transfer.
 * (assumed to be called at splbio())
 */
void
isa_freephysmem(caddr_t va, unsigned length) {

	isaphysmemflag &= ~B_BUSY;
	if (isaphysmemflag & B_WANTED) {
		isaphysmemflag &= B_WANTED;
		wakeup((caddr_t)&isaphysmemflag);
		if (isaphysmemunblock)
			(*isaphysmemunblock)();
	}
}

#endif
