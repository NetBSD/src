/*-
 * Copyright (c) 1993 Charles Hannum.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *
 *	from: @(#)isa.c	7.2 (Berkeley) 5/13/91
 *	$Id: isadma.c,v 1.4 1993/10/22 20:24:14 mycroft Exp $
 */

/*
 * code to deal with ISA DMA
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isa.h>
#include <i386/isa/isavar.h>
#include <i386/isa/ic/i8237.h>

/*
**  Register definitions for DMA controller 1 (channels 0..3):
*/
#define	DMA1_CHN(c)	(IO_DMA1 + 1*(2*(c)))	/* addr reg for channel c */
#define	DMA1_SR		(IO_DMA1 + 1*8)		/* status register */
#define	DMA1_SMSK	(IO_DMA1 + 1*10)	/* single mask register */
#define	DMA1_MODE	(IO_DMA1 + 1*11)	/* mode register */
#define	DMA1_FFC	(IO_DMA1 + 1*12)	/* clear first/last FF */

/*
**  Register definitions for DMA controller 2 (channels 4..7):
*/
#define	DMA2_CHN(c)	(IO_DMA2 + 2*(2*(c)))	/* addr reg for channel c */
#define	DMA2_SR		(IO_DMA2 + 2*8)		/* status register */
#define	DMA2_SMSK	(IO_DMA2 + 2*10)	/* single mask register */
#define	DMA2_MODE	(IO_DMA2 + 2*11)	/* mode register */
#define	DMA2_FFC	(IO_DMA2 + 2*12)	/* clear first/last FF */

/* region of physical memory known to be contiguous */
caddr_t isaphysmem;
static caddr_t bouncebuf[8];		/* XXX */
static caddr_t bounced[8];		/* XXX */
static vm_size_t bouncesize[8];		/* XXX */

/* high byte of address is stored in this port for i-th dma channel */
static u_short dmapageport[8] =
	{ 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };

/*
 * at_setup_dmachan(): allocate bounce buffer and check for conflicts.
 *
 * XXX
 * This sucks.  We should be able to bounce more than NBPG bytes, but I
 * don't feel like writing the code to do contiguous allocation right now.
 */
void
at_setup_dmachan(chan, max)
	int chan;
	u_long max;
{

#ifdef DIAGNOSTIC
	if (chan > 7 || chan < 0)
		panic("at_setup_dmachan: impossible request"); 
	if (max > NBPG)
		panic("at_setup_dmachan: what a lose");
#endif

	/* XXX check for drq conflict? */

	bouncebuf[chan] = (caddr_t) isaphysmem + NBPG*chan;
}

/*
 * at_dma_cascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
void
at_dma_cascade(chan)
	int chan;
{

#ifdef DIAGNOSTIC
	if (chan > 7 || chan < 0)
		panic("at_dma_cascade: impossible request"); 
#endif

	/* XXX check for drq conflict? */

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		outb(DMA1_MODE, DMA37MD_CASCADE | chan);
		outb(DMA1_SMSK, chan);
	} else {
		outb(DMA2_MODE, DMA37MD_CASCADE | (chan & 3));
		outb(DMA2_SMSK, chan & 3);
	}
}

/*
 * at_dma(): program 8237 DMA controller channel, avoid page alignment
 * problems by using a bounce buffer.
 */
void
at_dma(read, addr, nbytes, chan)
	int read;
	caddr_t addr;
	vm_size_t nbytes;
	int chan;
{
	vm_offset_t phys;
	int waport;
	caddr_t newaddr;

#ifdef DIAGNOSTIC
	if (chan > 7 || chan < 0 ||
	    (chan < 4 && nbytes > (1<<16)) ||
	    (chan >= 4 && (nbytes > (1<<17) || nbytes & 1 || (u_int)addr & 1)))
		panic("at_dma: impossible request"); 
#endif

	if (at_dma_rangecheck((vm_offset_t)addr, nbytes, chan)) {
		if (bouncebuf[chan] == 0)
			/* some twit forgot to call at_setup_dmachan() */
			panic("at_dma: no bounce buffer");
		/* XXX totally braindead; NBPG is not enough */
		if (nbytes > NBPG)
			panic("at_dma: transfer too large");
		bouncesize[chan] = nbytes;
		newaddr = bouncebuf[chan];
		/* copy bounce buffer on write */
		if (!read) {
			bcopy(addr, newaddr, nbytes);
			bounced[chan] = 0;
		} else
			bounced[chan] = addr;
		addr = newaddr;
	}

	/* translate to physical */
	phys = pmap_extract(pmap_kernel(), (vm_offset_t)addr);

	if ((chan & 4) == 0) {
		/*
		 * Program one of DMA channels 0..3.  These are
		 * byte mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		if (read)
			outb(DMA1_MODE, DMA37MD_SINGLE|DMA37MD_WRITE|chan);
		else
			outb(DMA1_MODE, DMA37MD_SINGLE|DMA37MD_READ|chan);
		outb(DMA1_FFC, 0);

		/* send start address */
		waport =  DMA1_CHN(chan);
		outb(waport, phys);
		outb(waport, phys>>8);
		outb(dmapageport[chan], phys>>16);

		/* send count */
		outb(waport + 1, --nbytes);
		outb(waport + 1, nbytes>>8);

		/* unmask channel */
		outb(DMA1_SMSK, DMA37SM_CLEAR | chan);
	} else {
		/*
		 * Program one of DMA channels 4..7.  These are
		 * word mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		if (read)
			outb(DMA2_MODE, DMA37MD_SINGLE|DMA37MD_WRITE|(chan&3));
		else
			outb(DMA2_MODE, DMA37MD_SINGLE|DMA37MD_READ|(chan&3));
		outb(DMA2_FFC, 0);

		/* send start address */
		waport = DMA2_CHN(chan & 3);
		outb(waport, phys>>1);
		outb(waport, phys>>9);
		outb(dmapageport[chan], phys>>16);

		/* send count */
		nbytes >>= 1;
		outb(waport + 2, --nbytes);
		outb(waport + 2, nbytes>>8);

		/* unmask channel */
		outb(DMA2_SMSK, DMA37SM_CLEAR | (chan & 3));
	}
}

/*
 * Abort a DMA request, clearing the bounce buffer, if any.
 */
void
at_dma_abort(chan)
	int chan;
{

#ifdef DIAGNOSTIC
	if (chan > 7 || chan < 0)
		panic("at_dma_abort: impossible request");
#endif

	bounced[chan] = 0;

	/* mask channel */
	if ((chan & 4) == 0)
		outb(DMA1_SMSK, DMA37SM_SET | chan);
	else
		outb(DMA2_SMSK, DMA37SM_SET | (chan & 3));
}

/*
 * End a DMA request, copying data from the bounce buffer, if any,
 * when reading.
 */
void
at_dma_terminate(chan)
	unsigned chan;
{
	u_char tc;

#ifdef DIAGNOSTIC
	if (chan > 7 || chan < 0)
		panic("at_dma_terminate: impossible request");
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
	if (bounced[chan]) {
		bcopy(bouncebuf[chan], bounced[chan], bouncesize[chan]);
		bounced[chan] = 0;
	}

	/* mask channel */
	if ((chan & 4) == 0)
		outb(DMA1_SMSK, DMA37SM_SET | chan);
	else
		outb(DMA2_SMSK, DMA37SM_SET | (chan & 3));
}

/*
 * Check for problems with the address range of a DMA transfer
 * (non-contiguous physical pages, outside of bus address space,
 * crossing DMA page boundaries).
 * Return true if special handling needed.
 */
int
at_dma_rangecheck(va, length, chan)
	vm_offset_t va;
	u_long length;
	int chan;
{
	vm_offset_t phys, priorpage = 0, endva;
	u_int dma_pgmsk = (chan&4) ?  ~(128*1024-1) : ~(64*1024-1);

	endva = round_page(va + length);
	for (; va < endva ; va += NBPG) {
		phys = trunc_page(pmap_extract(pmap_kernel(), va));
		if (phys == 0)
			panic("at_dma_rangecheck: no physical page present");
		if (phys >= (1<<24))
			return 1;
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
