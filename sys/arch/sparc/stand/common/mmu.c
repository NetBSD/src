/*	$NetBSD: mmu.c,v 1.1 2003/03/01 13:01:56 pk Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <lib/libsa/stand.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <machine/pte.h>
#include <sparc/sparc/asm.h>
#include <sparc/stand/common/promdev.h>

static int sun4_mmu3l;	/* sun4 3-lvl MMU */
static int rcookie;	/* sun4 3-lvl MMU region resource */
static int scookie;	/* sun4/sun4c MMU segment resource */
static int obmem;	/* SRMMU on-board memory space */

static int pmap_map4(vaddr_t va, paddr_t pa, psize_t size);
static int pmap_extract4(vaddr_t va, paddr_t *ppa);
static int pmap_map_srmmu(vaddr_t va, paddr_t pa, psize_t size);
static int pmap_extract_srmmu(vaddr_t va, paddr_t *ppa);

int (*pmap_map)(vaddr_t va, paddr_t pa, psize_t size);
int (*pmap_extract)(vaddr_t va, paddr_t *ppa);

extern int boothowto;

int mmu_init(void)
{
	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		pmap_map = pmap_map4;
		pmap_extract = pmap_extract4;
		if (CPU_ISSUN4) {
			/* Find out if we're on a 3-lvl sun4 MMU. */
			struct idprom *idp = prom_getidprom();
			if (idp->id_machine == ID_SUN4_400)
				sun4_mmu3l = 1;
		}
		/*
		 * XXX - we just guess which MMU cookies are free:
		 *	 region 0 maps [0-16MB]
		 *	 segment 0-16 map [0-4MB]
		 *	 the PROM mappings use high-valued cookies.
		 */
		rcookie = 2;
		scookie = 17;
	} else if (CPU_ISSUN4M || CPU_ISSUN4D) {
		char buf[32];
		pmap_map = pmap_map_srmmu;
		pmap_extract = pmap_extract_srmmu;
		sprintf(buf, "obmem %lx L!", (u_long)&obmem);
		prom_interpret(buf);
	} else
		return (ENOTSUP);

	return (0);
}


/*
 * SUN4 & SUN4C VM routines.
 * Limited functionality wrt. MMU resource management.
 */
#define	setregmap(va, smeg)	stha((va)+2, ASI_REGMAP, (smeg << 8))
#define	setsegmap(va, pmeg)	stba(va, ASI_SEGMAP, pmeg)

int pmap_map4(vaddr_t va, paddr_t pa, psize_t size)
{
	u_int n = size >> PGSHIFT;
	u_int pte;

	if (sun4_mmu3l)
		setregmap((va & -NBPRG), rcookie++);

	if (boothowto & AB_VERBOSE)
		printf("Mapping %lx -> %lx (%d pages)\n", va, pa, n);
	setsegmap((va & -NBPSG), ++scookie);
	while (n--) {
		pte = PG_S | PG_V | PG_W | PG_NC | ((pa >> PGSHIFT) & PG_PFNUM);
		setpte4(va, pte);
		va += NBPG;
		pa += NBPG;
		if ((va & (NBPSG - 1)) == 0) {
			if (boothowto & AB_VERBOSE)
				printf("%d ", scookie);
			setsegmap(va, ++scookie);
		}
	}
	if (boothowto & AB_VERBOSE)
		printf("\n", scookie);
	return (0);
}

int pmap_extract4(vaddr_t va, paddr_t *ppa)
{
	u_int pte;

	va &= -NBPG;
	pte = getpte4(va);
	if ((pte & PG_V) == 0)
		return (EFAULT);

	*ppa = (pte & PG_PFNUM) << PGSHIFT;
	return (0);
}


/*
 * SRMMU VM routines.
 * We use the PROM's Forth services to do all the hard work.
 */
int pmap_map_srmmu(vaddr_t va, paddr_t pa, psize_t size)
{
	char buf[64];
	u_int n = size >> PGSHIFT;

	if (boothowto & AB_VERBOSE)
		printf("Mapping %lx -> %lx (%d pages)\n", va, pa, n);
	while (n--) {
		sprintf(buf, "%lx %x %lx map-page", pa, obmem, va);
		prom_interpret(buf);
		va += NBPG;
		pa += NBPG;
		if (boothowto & AB_VERBOSE && (va & (NBPSG - 1)) == 0)
			printf("%d ", n);
	}
	if (boothowto & AB_VERBOSE)
		printf("\n", scookie);
	return (0);
}

int pmap_extract_srmmu(vaddr_t va, paddr_t *ppa)
{
	char buf[32];
	u_int pte;

	va &= -NBPG;
	sprintf(buf, "%lx pgmap@ %lx L!", va, (u_long)&pte);
	prom_interpret(buf);
	if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE)
		return (EFAULT);

	*ppa = (pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT;
	return (0);
}
