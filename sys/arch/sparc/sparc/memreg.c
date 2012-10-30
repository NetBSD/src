/*	$NetBSD: memreg.c,v 1.44.2.1 2012/10/30 17:20:22 yamt Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)memreg.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: memreg.c,v 1.44.2.1 2012/10/30 17:20:22 yamt Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/ctlreg.h>

#include <sparc/sparc/memreg.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>

#include <machine/pte.h>
#include <machine/reg.h>	/* for trapframe */
#include <machine/trap.h>	/* for trap types */

volatile u_int *par_err_reg;

static int	memregmatch_mainbus(device_t, cfdata_t, void *);
static int	memregmatch_obio(device_t, cfdata_t, void *);
static void	memregattach_mainbus(device_t, device_t, void *);
static void	memregattach_obio(device_t, device_t, void *);

CFATTACH_DECL_NEW(memreg_mainbus, 0,
    memregmatch_mainbus, memregattach_mainbus, NULL, NULL);

CFATTACH_DECL_NEW(memreg_obio, 0,
    memregmatch_obio, memregattach_obio, NULL, NULL);

#if defined(SUN4M)
static void hardmemerr4m(unsigned, u_int, u_int, u_int, u_int);
#endif

/*
 * The OPENPROM calls this "memory-error".
 */
static int
memregmatch_mainbus(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("memory-error", ma->ma_name) == 0);
}

static int
memregmatch_obio(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("memory-error", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("memregmatch_obio: attach args mixed up\n");
		return (0);
	}

	return (1);
}

/* ARGSUSED */
static void
memregattach_mainbus(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	printf("\n");
	if (ma->ma_promvaddr != 0) {
		par_err_reg = (volatile int *)ma->ma_promvaddr;
		return;
	}

	if (bus_space_map(ma->ma_bustag,
			   ma->ma_paddr,
			   sizeof(par_err_reg),
			   BUS_SPACE_MAP_LINEAR,
			   &bh) != 0) {
		printf("memregattach_mainbus: can't map register\n");
		return;
	}
	par_err_reg = (volatile int *)bh;
}

/* ARGSUSED */
static void
memregattach_obio(device_t parent, device_t self, void *aux)
{
	union obio_attach_args *uoba = aux;
	bus_space_handle_t bh;

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		if (sa->sa_promvaddr != 0) {
			par_err_reg = (volatile int *)sa->sa_promvaddr;
			return;
		}

		if (sbus_bus_map(sa->sa_bustag,
				 sa->sa_slot, sa->sa_offset,
				 sizeof(par_err_reg),
				 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			printf("memregattach_obio: can't map register\n");
			return;
		}
		par_err_reg = (volatile int *)bh;
	}

	/* On sun4, `par_err_reg' has already been mapped in autoconf.c */
	if (par_err_reg == NULL)
		panic("memregattach");

	printf("\n");
}

/*
 * Synchronous and asynchronous memory error handler.
 * (This is the level 15 interrupt, which is not vectored.)
 * Should kill the process that got its bits clobbered,
 * and take the page out of the page pool, but for now...
 */

void
memerr4_4c(unsigned int issync,
	   u_int ser, u_int sva, u_int aer, u_int ava,
	   struct trapframe *tf) /* XXX - unused/invalid */
{
	char bits[64];
	u_int pte;

	snprintb(bits, sizeof(bits), SER_BITS, ser);
	printf("%ssync mem arr: ser=%s sva=0x%x ",
	    issync ? "" : "a", bits, sva);
	snprintb(bits, sizeof(bits), AER_BITS, aer & 0xff);
	printf("aer=%s ava=0x%x\n", bits, ava);

	pte = getpte4(sva);
	if ((pte & PG_V) != 0 && (pte & PG_TYPE) == PG_OBMEM) {
		u_int pa = (pte & PG_PFNUM) << PGSHIFT;
		printf(" spa=0x%x, module location: %s\n", pa,
			prom_pa_location(pa, 0));
	}

	pte = getpte4(ava);
	if ((pte & PG_V) != 0 && (pte & PG_TYPE) == PG_OBMEM) {
		u_int pa = (pte & PG_PFNUM) << PGSHIFT;
		printf(" apa=0x%x, module location: %s\n", pa,
			prom_pa_location(pa, 0));
	}

	if (par_err_reg) {
		snprintb(bits, sizeof(bits), PER_BITS, *par_err_reg);
		printf("parity error register = %s\n", bits);
	}
	panic("memory error");		/* XXX */
}


#if defined(SUN4M)
/*
 * hardmemerr4m: called upon fatal memory error. Print a message and panic.
 */
static void
hardmemerr4m(unsigned type, u_int sfsr, u_int sfva, u_int afsr, u_int afva)
{
	char bits[64];

	printf("memory fault: type %d", type);
	snprintb(bits, sizeof(bits), SFSR_BITS, sfsr);
	printf("sfsr=%s sfva=0x%x\n", bits, sfva);

	if (afsr != 0) {
		snprintb(bits, sizeof(bits), AFSR_BITS, afsr);
		printf("; afsr=%s afva=0x%x%x\n", bits,
			(afsr & AFSR_AFA) >> AFSR_AFA_RSHIFT, afva);
	}

	if ((sfsr & SFSR_FT) == SFSR_FT_NONE  && (afsr & AFSR_AFO) == 0)
		return;

	panic("hard memory error");
}

/*
 * Memerr4m: handle a non-trivial memory fault. These include HyperSPARC
 * asynchronous faults, SuperSPARC store-buffer copyback failures, and
 * data faults without a valid faulting VA. We try to retry the operation
 * once, and then fail if we get called again.
 */

/* XXXSMP */
static int addrold = (int) 0xdeadbeef; /* We pick an unlikely address */
static int addroldtop = (int) 0xdeadbeef;
static int oldtype = -1;
/* XXXSMP */

void
hypersparc_memerr(unsigned type, u_int sfsr, u_int sfva, struct trapframe *tf)
{
	u_int afsr;
	u_int afva;

	KERNEL_LOCK(1, NULL);

	(*cpuinfo.get_asyncflt)(&afsr, &afva);
	if ((afsr & AFSR_AFO) != 0) {	/* HS async fault! */

		printf("HyperSPARC async cache memory failure at phys 0x%x%x\n",
		       (afsr & AFSR_AFA) >> AFSR_AFA_RSHIFT, afva);

		if (afva == addrold && (afsr & AFSR_AFA) == addroldtop)
			goto hard;

		oldtype = -1;
		addrold = afva;
		addroldtop = afsr & AFSR_AFA;
	}
out:
	KERNEL_UNLOCK_ONE(NULL);
	return;

hard:
	hardmemerr4m(type, sfsr, sfva, afsr, afva);
	goto out;
}

void
viking_memerr(unsigned type, u_int sfsr, u_int sfva, struct trapframe *tf)
{
	u_int afsr=0;	/* No Async fault registers on the viking */
	u_int afva=0;

	KERNEL_LOCK(1, NULL);

	if (type == T_STOREBUFFAULT) {

		/*
		 * On Supersparc, we try to re-enable the store buffers
		 * to force a retry.
		 */
		printf("store buffer copy-back failure at 0x%x. Retrying...\n",
		       sfva);

		if (oldtype == T_STOREBUFFAULT || addrold == sfva)
			goto hard;

		oldtype = T_STOREBUFFAULT;
		addrold = sfva;

		/* re-enable store buffer */
		sta(SRMMU_PCR, ASI_SRMMU,
		    lda(SRMMU_PCR, ASI_SRMMU) | VIKING_PCR_SB);

	} else if (type == T_DATAFAULT && (sfsr & SFSR_FAV) == 0) {
		/*
		 * bizarre.
		 * XXX: Should handle better. See SuperSPARC manual pg. 9-35
		 */
		printf("warning: got data fault with no faulting address."
		       " Ignoring.\n");

		if (oldtype == T_DATAFAULT)
			goto hard;
		oldtype = T_DATAFAULT;
	}

out:
	KERNEL_UNLOCK_ONE(NULL);
	return;

hard:
	hardmemerr4m(type, sfsr, sfva, afsr, afva);
	goto out;
}

void
memerr4m(unsigned type, u_int sfsr, u_int sfva, struct trapframe *tf)
{
	u_int afsr;
	u_int afva;

	KERNEL_LOCK(1, NULL);

	/*
	 * No known special cases.
	 * Just get async registers, if any, and report the unhandled case.
	 */
	if ((*cpuinfo.get_asyncflt)(&afsr, &afva) != 0)
		afsr = afva = 0;

	hardmemerr4m(type, sfsr, sfva, afsr, afva);
	KERNEL_UNLOCK_ONE(NULL);
}
#endif /* SUN4M */
