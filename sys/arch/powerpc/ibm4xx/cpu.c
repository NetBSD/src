/*	$NetBSD: cpu.c,v 1.2.2.2 2002/03/16 15:59:15 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/dcr.h>

struct cputab {
	int version;
	char *name;
};
static struct cputab models[] = {
	{ PVR_401A1 >> 16, "401A1" },
	{ PVR_401B2 >> 16, "401B21" },
	{ PVR_401C2 >> 16, "401C2" },
	{ PVR_401D2 >> 16, "401D2" },
	{ PVR_401E2 >> 16, "401E2" },
	{ PVR_401F2 >> 16, "401F2" },
	{ PVR_401G2 >> 16, "401G2" },
	{ PVR_403 >> 16, "403" },
	{ PVR_405GP >> 16, "405GP" },
	{ 0,		    NULL }
};

static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

/*
 * Arguably the ECC stuff belongs somewhere else....
 */
int intr_ecc(void *);

u_quad_t		intr_ecc_tb;
u_quad_t		intr_ecc_iv;	 /* Interval */
u_int32_t		intr_ecc_cnt;

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

int ncpus;

struct cpu_info cpu_info_store;

int cpufound = 0;

static int
cpumatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	/* make sure that we're looking for a CPU */
	if (strcmp(maa->mb_name, cf->cf_driver->cd_name) != 0)
                return (0);

	return !cpufound;
}

static void
cpuattach(struct device *parent, struct device *self, void *aux)
{
	int pvr, cpu;
	int own, pcf, cas, pcl, aid;
	struct cputab *cp = models;

	cpufound++;
	ncpus++;

	asm ("mfpvr %0" : "=r"(pvr));
	cpu = pvr >> 16;

	/* Break PVR up into separate fields and print them out. */
	own = (pvr >> 20) & 0xfff;
	pcf = (pvr >> 16) & 0xf;
	cas = (pvr >> 10) & 0x3f;
	pcl = (pvr >> 6) & 0xf;
	aid = pvr & 0x3f;

	while (cp->name) {
		if (cp->version == cpu)
			break;
		cp++;
	}
	if (cp->name)
		strcpy(cpu_model, cp->name);
	else
		sprintf(cpu_model, "Version 0x%x", cpu);
	sprintf(cpu_model + strlen(cpu_model), " (Revision %d.%d)",
		(pvr >> 8) & 0xff, pvr & 0xff);

#if 1
	printf(": %dMHz %s\n", board_data.processor_speed / 1000 / 1000,
	    cpu_model);
#endif

	cpu_probe_cache();

	printf("Instruction cache size %d line size %d\n", 
		curcpu()->ci_ci.icache_size, curcpu()->ci_ci.icache_line_size);
	printf("Data cache size %d line size %d\n", 
		curcpu()->ci_ci.dcache_size, curcpu()->ci_ci.dcache_line_size);

#ifdef DEBUG
	/* It sux that the cache info here is useless. */
	printf("PVR: owner %x core family %x cache %x version %x asic %x\n",
		own, pcf, cas, pcl, aid);
#endif

	/* Initialize ECC error-logging handler.  This is always enabled,
	 * but it will never be called on systems that do not have ECC
	 * enabled by POST code in the bootloader.
         */

	printf("Enabling ecc handler\n");
	intr_ecc_tb = 0;
	intr_ecc_iv = board_data.processor_speed; /* Set interval */
	intr_ecc_cnt = 0;

	intr_establish(16, IST_LEVEL, IPL_SERIAL, intr_ecc, NULL);
}

/*
 * This routine must be explicitly called to initialize the 
 * CPU cache information so cache flushe and memcpy operation 
 * work.
 */
void
cpu_probe_cache()
{
	int version;

	/*
	 * First we need to identify the cpu and determine the 
	 * cache line size, or things like memset/memcpy may lose
	 * badly.
	 */
	__asm __volatile("mfpvr %0" : "=r" (version));
	switch (version & 0xffff0000) {
	case PVR_401A1:
		curcpu()->ci_ci.dcache_size = 1024;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 2848;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401B2:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 16384;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401C2:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 0;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401D2:
		curcpu()->ci_ci.dcache_size = 2848;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 4096;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401E2:
		curcpu()->ci_ci.dcache_size = 0;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 0;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401F2:
		curcpu()->ci_ci.dcache_size = 2048;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 2848;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401G2:
		curcpu()->ci_ci.dcache_size = 2848;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 8192;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_403:
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_405GP:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 32;
		curcpu()->ci_ci.icache_size = 8192;
		curcpu()->ci_ci.icache_line_size = 32;
		break;
	default:
		/* 
		 * Unknown CPU type.  For safety we'll specify a 
		 * cache with a 4-byte line size.  That way cache 
		 * flush routines won't miss any lines.
		 */
		curcpu()->ci_ci.dcache_line_size = 4;
		curcpu()->ci_ci.icache_line_size = 4;
		break;
	}

}

/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */

void
dcache_flush_page(vaddr_t va)
{
	int i;

	if (curcpu()->ci_ci.dcache_line_size)
		for (i = 0; i < NBPG; i += curcpu()->ci_ci.dcache_line_size)
			asm volatile("dcbf %0,%1" : : "r" (va), "r" (i));
	asm volatile("sync;isync" : : );
}

void
icache_flush_page(vaddr_t va)
{
	int i;

	if (curcpu()->ci_ci.icache_line_size)
		for (i = 0; i < NBPG; i += curcpu()->ci_ci.icache_line_size)
			asm volatile("icbi %0,%1" : : "r" (va), "r" (i));
	asm volatile("sync;isync" : : );
}

void
dcache_flush(vaddr_t va, vsize_t len)
{
	int i;

	if (len == 0)
		return;

	/* Make sure we flush all cache lines */
	len += va & (curcpu()->ci_ci.dcache_line_size-1);
	if (curcpu()->ci_ci.dcache_line_size)
		for (i = 0; i < len; i += curcpu()->ci_ci.dcache_line_size)
			asm volatile("dcbf %0,%1" : : "r" (va), "r" (i));
	asm volatile("sync;isync" : : );
}

void
icache_flush(vaddr_t va, vsize_t len)
{
	int i;

	if (len == 0)
		return;

	/* Make sure we flush all cache lines */
	len += va & (curcpu()->ci_ci.icache_line_size-1);
	if (curcpu()->ci_ci.icache_line_size)
		for (i = 0; i < len; i += curcpu()->ci_ci.icache_line_size)
			asm volatile("icbi %0,%1" : : "r" (va), "r" (i));
	asm volatile("sync;isync" : : );
}

/*
 * ECC fault handler.
 */
int
intr_ecc(void * arg)
{
	u_int32_t		esr, ear;
	int			ce, ue;
	u_quad_t		tb;
	u_long			tmp, msr, dat;
	
	/* This code needs to be improved to handle double-bit errors */
	/* in some intelligent fashion. */

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
	esr = mfdcr(DCR_SDRAM0_CFGDATA);

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_BEAR);
	ear = mfdcr(DCR_SDRAM0_CFGDATA);

	/* Always clear the error to stop the intr ASAP. */	

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
	mtdcr(DCR_SDRAM0_CFGDATA, 0xffffffff);

	if (esr == 0x00) { 	
		/* No current error.  Could happen due to intr. nesting */
		return(1);
	};

	/* Only report errors every once per second max. Do this using the TB, */
	/* because the system time (via microtime) may be adjusted when the date is set */
        /* and can't reliably be used to measure intervals. */
	
	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b" 
		: "=r"(tb), "=r"(tmp));
	intr_ecc_cnt++;

	if ((tb - intr_ecc_tb) < intr_ecc_iv) {
		return(1);
	};		

	ce = (esr & SDRAM0_ECCESR_CE) != 0x00;
	ue = (esr & SDRAM0_ECCESR_UE) != 0x00;

	printf("ECC: Error CNT=%d ESR=%x EAR=%x %s BKNE=%d%d%d%d "
		"BLCE=%d%d%d%d CBE=%d%d.\n",
		intr_ecc_cnt, esr, ear,
		(ue) ? "Uncorrectable" : "Correctable", 
		((esr & SDRAM0_ECCESR_BKEN(0)) != 0x00), 
		((esr & SDRAM0_ECCESR_BKEN(1)) != 0x00),
		((esr & SDRAM0_ECCESR_BKEN(2)) != 0x00), 
		((esr & SDRAM0_ECCESR_BKEN(3)) != 0x00), 
		((esr & SDRAM0_ECCESR_BLCEN(0)) != 0x00), 
		((esr & SDRAM0_ECCESR_BLCEN(1)) != 0x00), 
		((esr & SDRAM0_ECCESR_BLCEN(2)) != 0x00), 
		((esr & SDRAM0_ECCESR_BLCEN(3)) != 0x00),
		((esr & SDRAM0_ECCESR_CBEN(0)) != 0x00),
		((esr & SDRAM0_ECCESR_CBEN(1)) != 0x00));

	/* Should check for uncorrectable errors and panic... */

	if (intr_ecc_cnt > 1000) {
		printf("ECC: Too many errors, recycling entire "
			"SDRAM (size = %d).\n", board_data.mem_size);

		/* Can this code be changed to run without disabling data MMU and disabling intrs? */
		/* Does kernel always map all of physical RAM VA=PA? If so, just loop over lowmem. */

		asm volatile(
			"mfmsr 	%0;"
			"li	%1, 0x00;"
			"ori	%1, %1, 0x8010;"
			"andc	%1, %0, %1;"
			"mtmsr	%1;"
			"sync;isync;"
			"li	%1, 0x00;"
			"1:"
			"dcbt	0, %1;"
			"sync;isync;"
			"lwz	%2, 0(%1);"
			"stw	%2, 0(%1);"
			"sync;isync;"
			"dcbf	0, %1;"
			"sync;isync;"
			"addi	%1, %1, 0x20;"
			"addic.	%3, %3, -0x20;"
			"bge 	1b;"
			"mtmsr %0;"
			"sync;isync;"
		: "=&r" (msr), "=&r" (tmp), "=&r" (dat)
		: "r" (board_data.mem_size) : "0" );

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		esr = mfdcr(DCR_SDRAM0_CFGDATA);

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		mtdcr(DCR_SDRAM0_CFGDATA, 0xffffffff);

		/* Correctable errors here are OK, mem should be clean now. */
		/* Should check for uncorrectable errors and panic... */
		printf("ECC: Recycling complete, ESR=%x. "
			"Checking for persistent errors.\n", esr);
	
		asm volatile(
			"mfmsr 	%0;"
			"li	%1, 0x00;"
			"ori	%1, %1, 0x8010;"
			"andc	%1, %0, %1;"
			"mtmsr	%1;"
			"sync;isync;"
			"li	%1, 0x00;"
			"1:"
			"dcbt	0, %1;"
			"sync;isync;"
			"lwz	%2, 0(%1);"
			"stw	%2, 0(%1);"
			"sync;isync;"
			"dcbf	0, %1;"
			"sync;isync;"
			"addi	%1, %1, 0x20;"
			"addic.	%3, %3, -0x20;"
			"bge 	1b;"
			"mtmsr %0;"
			"sync;isync;"
		: "=&r" (msr), "=&r" (tmp), "=&r" (dat)
		: "r" (board_data.mem_size) : "0" );

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		esr = mfdcr(DCR_SDRAM0_CFGDATA);

		/* If esr is non zero here, we're screwed.  Should check this and panic. */
		printf("ECC: Persistent error check complete, "
			"final ESR=%x.\n", esr);
	};

	intr_ecc_tb = tb;
	intr_ecc_cnt = 0;

	return(1);
};
