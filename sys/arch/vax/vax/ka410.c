/*	$NetBSD: ka410.c,v 1.14 1999/01/01 21:43:19 ragge Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
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
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/mtpr.h>
#include <machine/sid.h>
#include <machine/pmap.h>
#include <machine/nexus.h>
#include <machine/uvax.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/clock.h>
#include <machine/vsbus.h>

#include "smg.h"
#include "ry.h"
#include "ncr.h"

static	void	ka410_conf __P((struct device*, struct device*, void*));
static	void	ka410_steal_pages __P((void));
static	void	ka410_memerr __P((void));
static	int	ka410_mchk __P((caddr_t));
static	void	ka410_halt __P((void));
static	void	ka410_reboot __P((int));
static	void	ka41_cache_enable __P((void));

static	caddr_t	l2cache;	/* mapped in address */
static	long 	*cacr;		/* l2csche ctlr reg */

extern  short *clk_page;

/* 
 * Declaration of 410-specific calls.
 */
struct	cpu_dep ka410_calls = {
	ka410_steal_pages,
	no_nicr_clock,
	ka410_mchk,
	ka410_memerr, 
	ka410_conf,
	chip_clkread,
	chip_clkwrite,
	1,      /* ~VUPS */
	ka410_halt,
	ka410_reboot,
};


void
ka410_conf(parent, self, aux)
	struct	device *parent, *self;
	void	*aux;
{
	switch (vax_cputype) {
	case VAX_TYP_UV2:
		printf(": KA410\n");
		break;

	case VAX_TYP_CVAX:
		printf(": KA41/42\n");
		printf("%s: Enabling primary cache, ", self->dv_xname);
mtpr(KA420_CADR_S2E|KA420_CADR_S1E|KA420_CADR_ISE|KA420_CADR_DSE, PR_CADR);
		if (vax_confdata & KA420_CFG_CACHPR) {
			printf("secondary cache\n");
			ka41_cache_enable();
		} else
			printf("no secondary cache present\n");
	}
}

void
ka41_cache_enable()
{
	*cacr = KA420_CACR_TPE; 	/* Clear any error, disable cache */
	bzero(l2cache, KA420_CH2_SIZE); /* Clear whole cache */
	*cacr = KA420_CACR_CEN;		/* Enable cache */
}

void
ka410_memerr()
{
	printf("Memory err!\n");
}

int
ka410_mchk(addr)
	caddr_t addr;
{
	panic("Machine check");
	return 0;
}

void
ka410_steal_pages()
{
	extern	vm_offset_t avail_start, virtual_avail;
        extern  int clk_adrshift, clk_tweak;
	int	junk, parctl = 0;

	/* Interrupt vector number in interrupt mask table */
	inr_ni = VS3100_NI;
	inr_sr = VS3100_SR;
	inr_st = VS3100_ST;
	inr_vf = VS3100_VF;
	/* 
	 * SCB is already copied/initialized at addr avail_start
	 * by pmap_bootstrap(), but it's not yet mapped. Thus we use
	 * the MAPPHYS() macro to reserve these two pages and to
	 * perform the mapping. The mapped address is assigned to junk.
	 */
	MAPPHYS(junk, 2, VM_PROT_READ|VM_PROT_WRITE);

	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	clk_adrshift = 1;       /* Addressed at long's... */
	clk_tweak = 2;          /* ...and shift two */
	MAPVIRT(clk_page, 1);
	pmap_map((vm_offset_t)clk_page, (vm_offset_t)KA410_WAT_BASE,
	    (vm_offset_t)KA410_WAT_BASE + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);

	/* LANCE CSR & DMA memory */
	MAPVIRT(lance_csr, 1);
	pmap_map((vm_offset_t)lance_csr, (vm_offset_t)NI_BASE,
	    (vm_offset_t)NI_BASE + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);

	MAPVIRT(vs_cpu, 1);
	pmap_map((vm_offset_t)vs_cpu, (vm_offset_t)VS_REGS,
	    (vm_offset_t)VS_REGS + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);

	MAPVIRT(dz_regs, 2);
	pmap_map((vm_offset_t)dz_regs, (vm_offset_t)DZ_CSR,
	    (vm_offset_t)DZ_CSR + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);

	MAPVIRT(lance_addr, 1);
	pmap_map((vm_offset_t)lance_addr, (vm_offset_t)NI_ADDR,
	    (vm_offset_t)NI_ADDR + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);

	MAPPHYS(le_iomem, (NI_IOSIZE/VAX_NBPG), VM_PROT_READ|VM_PROT_WRITE);

	if (((int)le_iomem & ~KERNBASE) > 0xffffff)
		parctl = PARCTL_DMA;

#if NSMG > 0
	if ((vax_confdata & KA420_CFG_MULTU) == 0) {
		MAPVIRT(sm_addr, (SMSIZE / VAX_NBPG));
		pmap_map((vm_offset_t)sm_addr, (vm_offset_t)SMADDR,
		    (vm_offset_t)SMADDR + SMSIZE, VM_PROT_READ|VM_PROT_WRITE);
		((struct vs_cpu *)VS_REGS)->vc_vdcorg = 0;
	}
#endif
#if NRD || NRX || NNCR
	MAPVIRT(sca_regs, 1);
	pmap_map((vm_offset_t)sca_regs, (vm_offset_t)KA410_DKC_BASE,
	    (vm_offset_t)KA410_DKC_BASE + VAX_NBPG,
	    VM_PROT_READ|VM_PROT_WRITE);

	if (vax_boardtype == VAX_BTYP_410) {
		MAPVIRT(dma_area, (KA410_DMA_SIZE / VAX_NBPG));
		pmap_map((vm_offset_t)dma_area, (vm_offset_t)KA410_DMA_BASE,
		    (vm_offset_t)KA410_DMA_BASE + KA410_DMA_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE);
	} else {
		MAPVIRT(dma_area, (KA420_DMA_SIZE / VAX_NBPG));
		pmap_map((vm_offset_t)dma_area, (vm_offset_t)KA420_DMA_BASE,
		    (vm_offset_t)KA420_DMA_BASE + KA420_DMA_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE);
	}
#endif
	if ((vax_cputype == VAX_TYP_CVAX) &&
	    (vax_confdata & KA420_CFG_CACHPR)) {
		MAPVIRT(l2cache, (KA420_CH2_SIZE / VAX_NBPG));
		pmap_map((vm_offset_t)l2cache, (vm_offset_t)KA420_CH2_BASE,
		    (vm_offset_t)KA420_CH2_BASE + KA420_CH2_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE);
		MAPVIRT(cacr, 1);
		pmap_map((vm_offset_t)cacr, (vm_offset_t)KA420_CACR,
		    (vm_offset_t)KA420_CACR + VAX_NBPG, VM_PROT_READ|VM_PROT_WRITE);
	}
		
	/*
	 * Clear restart and boot in progress flags
	 * in the CPMBX. (ie. clear bits 4 and 5)
	 */
	KA410_WAT_BASE->cpmbx = (KA410_WAT_BASE->cpmbx & ~0x30);

	/*
	 * Enable memory parity error detection and clear error bits.
	 */
        switch (vax_cputype) {
        case VAX_TYP_UV2:
		KA410_CPU_BASE->ka410_mser = 1;
                break;

        case VAX_TYP_CVAX:
		((struct vs_cpu *)VS_REGS)->vc_parctl =
		    parctl | PARCTL_CPEN | PARCTL_DPEN ;
		break;
        }
}

static void
ka410_halt()
{
	asm("movl $0xc, (%0)"::"r"((int)clk_page + 0x38)); /* Don't ask */
	asm("halt");
}

static void
ka410_reboot(arg)
	int arg;
{
	asm("movl $0xc, (%0)"::"r"((int)clk_page + 0x38)); /* Don't ask */
	asm("halt");
}
