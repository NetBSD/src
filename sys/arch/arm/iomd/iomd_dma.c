/* 	$NetBSD: iomd_dma.c,v 1.2.2.5 2002/06/23 17:34:52 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Scott Stevens
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
 *	This product includes software developed by Scott Stevens.
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
 *
 * RiscBSD kernel project
 *
 * dma.c
 *
 * Created      : 15/03/97
 */

#define DMA_DEBUG
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/pmap.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/iomd_dma.h>


/*
 * Only for non ARM7500 machines but the kernel could be booted on a different machine
 */

static struct dma_ctrl ctrl[6];

void dma_dumpdc __P((struct dma_ctrl *));

void
dma_go(dp)
	struct dma_ctrl *dp;
{
#ifdef DMA_DEBUG
	printf("dma_go()\n");
#endif
	if(dp->dc_flags & DMA_FL_READY) {
		dp->dc_flags = DMA_FL_ACTIVE;
		enable_irq(IRQ_DMACH0 + dp->dc_channel);
	}
	else
		panic("dma not ready");
}

int
dma_reset(dp)
	struct dma_ctrl *dp;
{
#ifdef DMA_DEBUG
	printf("dma_reset()\n");
	dma_dumpdc(dp);
#endif
	*dp->dc_cr = DMA_CR_CLEAR;
	dp->dc_flags = 0;
	disable_irq(IRQ_DMACH0 + dp->dc_channel);
	return(0);
}

/*
 * Setup dma transfer, prior to the dma_go call
 */
int
dma_setup(dp, start, len, readp)
	struct dma_ctrl *dp;
	int readp;
	u_char *start;
	int len;
{
#ifdef DMA_DEBUG
	printf("dma_setup(start = %p, len = 0x%08x, readp = %d\n", start, len, readp);
#endif
	if(((u_int)start & (dp->dc_dmasize - 1)) ||
	   (len & (dp->dc_dmasize - 1))) {
		printf("dma_setup: unaligned DMA, %p (0x%08x)\n",
			start, len);
	}
	*dp->dc_cr = DMA_CR_CLEAR | DMA_CR_ENABLE | (readp?DMA_CR_DIR:0) | dp->dc_dmasize;
	*dp->dc_cr = DMA_CR_ENABLE | (readp?DMA_CR_DIR:0) | dp->dc_dmasize;

	dp->dc_nextaddr = start;
	dp->dc_len = len;

	dp->dc_flags = DMA_FL_READY;
	return(0);
}

/*
 * return true if DMA is active
 */
int
dma_isactive(dp)
	struct dma_ctrl *dp;
{
	return(dp->dc_flags & DMA_FL_ACTIVE);
}

/*
 * return true if interrupt pending
 */
int
dma_isintr(dp)
	struct dma_ctrl *dp;
{
#ifdef DMA_DEBUG
/*	printf("dma_isintr() returning %d\n", *dp->dc_st & DMA_ST_INT);*/
#endif
	return(*dp->dc_st & DMA_ST_INT);
}

int
dma_intr(cookie)
	void *cookie;
{
	struct dma_ctrl *dp = cookie;
	u_char status = (*dp->dc_st) & DMA_ST_MASK;
	paddr_t cur;
	int len;
	int bufap = 0;

#ifdef DMA_DEBUG
	printf("dma_intr() status = 0x%02x\n", status);
#endif
	
	if(!(dp->dc_flags & DMA_FL_ACTIVE)) {
		/* interrupt whilst not active */
		/* ie. last buffer programmed */
		dma_reset(dp);
		return(0);
	}

	switch(status) {
	case DMA_ST_OVER | DMA_ST_INT:
	case DMA_ST_OVER | DMA_ST_INT | DMA_ST_CHAN:
		/* idle, either first buffer or finished */
		if(status & DMA_ST_CHAN) {
			/* fill buffer B */
			bufap = 0;
			goto fill;
		}
		else {
			/* fill buffer A */
			bufap = 1;
			goto fill;
		}
		break;
	case DMA_ST_INT:
	case DMA_ST_INT | DMA_ST_CHAN:
		/* buffer ready */
		if(status & DMA_ST_CHAN) {
			/* fill buffer A */
			bufap = 1;
			goto fill;
		}
		else {
			/* fill buffer B */
			bufap = 0;
			goto fill;
		}
		break;
	default:
		/* Shouldn't be here */
#ifdef DMA_DEBUG
		printf("DMA ch %d bad status [%x]\n", dp->dc_channel, status);
		dma_dumpdc(dp);
#endif
		break;
	}
		
/*	return(0);*/
/* XXX */
#define	PHYS(x, y)	pmap_extract(pmap_kernel(), (vaddr_t)x, (paddr_t *)(y))
fill:
#ifdef DMA_DEBUG
	printf("fill:\n");
#endif
	if (dp->dc_len == 0) goto done;
	PHYS(dp->dc_nextaddr, &cur);
	len = NBPG - (cur & PGOFSET);
	if (len > dp->dc_len) {
		/* Last buffer */
		len = dp->dc_len;
	}

#ifdef DMA_DEBUG
	dma_dumpdc(dp);
/*	ptsc_dump_mem(dp->dc_nextaddr, len);*/
#endif
/*
 * Flush the cache for this address
 */
	cpu_dcache_wbinv_range((vaddr_t)dp->dc_nextaddr, len);

	dp->dc_nextaddr += len;
	dp->dc_len -= len;

	if(bufap) {
		*dp->dc_cura = (u_int)cur;
		*dp->dc_enda = ((u_int)cur + len - dp->dc_dmasize) |
				(dp->dc_len == 0 ? DMA_END_STOP : 0);
		if (dp->dc_len == 0) {
			/* Last buffer, fill other buffer with garbage */
			*dp->dc_endb = (u_int)cur;
		}
	}
	else {
		*dp->dc_curb = (u_int)cur;
		*dp->dc_endb = ((u_int)cur + len - dp->dc_dmasize) |
				(dp->dc_len == 0 ? DMA_END_STOP : 0);
		if (dp->dc_len == 0) {
			/* Last buffer, fill other buffer with garbage */
			*dp->dc_enda = (u_int)cur;
		}
	}
#ifdef DMA_DEBUG
	dma_dumpdc(dp);
/*	ptsc_dump_mem(dp->dc_nextaddr - len, len);*/
	printf("about to return\n");
#endif
	return(1);
done:
#ifdef DMA_DEBUG
	printf("done:\n");
#endif
	dp->dc_flags = 0;
	*dp->dc_cr = 0;
	disable_irq(IRQ_DMACH0 + dp->dc_channel);
#ifdef DMA_DEBUG
	printf("about to return\n");
#endif
	return(1);
}

void
dma_dumpdc(dc)
	struct dma_ctrl *dc;
{
	printf("\ndc:\t%p\n"
		"dc_channel:\t%p=0x%08x\tdc_flags:\t%p=0x%08x\n"
		"dc_cura:\t%p=0x%08x\tdc_enda:\t%p=0x%08x\n"
		"dc_curb:\t%p=0x%08x\tdc_endb:\t%p=0x%08x\n"
		"dc_cr:\t%p=0x%02x\t\tdc_st:\t%p=0x%02x\n"
		"dc_nextaddr:\t%p=0x%08x\tdc_len:\t%p=0x%08x\n",
		dc,
		&dc->dc_channel, (int)dc->dc_channel,
		&dc->dc_flags, (int)dc->dc_flags,
		dc->dc_cura, (int)*dc->dc_cura,
		dc->dc_enda, (int)*dc->dc_enda,
		dc->dc_curb, (int)*dc->dc_curb,
		dc->dc_endb, (int)*dc->dc_endb,
		dc->dc_cr, (int)*dc->dc_cr,
		dc->dc_st, (int)(*dc->dc_st) & DMA_ST_MASK,
		&dc->dc_nextaddr, (int)dc->dc_nextaddr,
		&dc->dc_len, dc->dc_len);
}

struct dma_ctrl *
dma_init(ch, extp, dmasize, ipl)
	int ch;
	int extp;
	int dmasize;
	int ipl;
{
	struct dma_ctrl *dp = &ctrl[ch];
	int offset = ch * 0x20;
	volatile u_char *dmaext = (volatile u_char *)(IOMD_ADDRESS(IOMD_DMAEXT));

	printf("Initialising DMA channel %d\n", ch);

	dp->dc_channel = ch;
	dp->dc_flags = 0;
	dp->dc_dmasize = dmasize;
	dp->dc_cura = (volatile u_int *)(IOMD_ADDRESS(IOMD_IO0CURA) + offset);
	dp->dc_enda = (volatile u_int *)(IOMD_ADDRESS(IOMD_IO0ENDA) + offset);
	dp->dc_curb = (volatile u_int *)(IOMD_ADDRESS(IOMD_IO0CURB) + offset);
	dp->dc_endb = (volatile u_int *)(IOMD_ADDRESS(IOMD_IO0ENDB) + offset);
	dp->dc_cr = (volatile u_char *)(IOMD_ADDRESS(IOMD_IO0CR) + offset);
	dp->dc_st = (volatile u_char *)(IOMD_ADDRESS(IOMD_IO0ST) + offset);

	if (extp)
		*dmaext |= (1 << ch);

	printf("about to claim interrupt\n");

	dp->dc_ih.ih_func = dma_intr;
	dp->dc_ih.ih_arg = dp;
	dp->dc_ih.ih_level = ipl;
	dp->dc_ih.ih_name = "dma";
	dp->dc_ih.ih_maskaddr = (u_int) IOMD_ADDRESS(IOMD_DMARQ);
	dp->dc_ih.ih_maskbits = (1 << ch);

	if (irq_claim(IRQ_DMACH0 + ch, &dp->dc_ih))
		panic("Cannot install DMA IRQ handler\n");

	return(dp);
}

