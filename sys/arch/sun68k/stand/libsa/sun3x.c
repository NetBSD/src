/*	$NetBSD: sun3x.c,v 1.2.8.1 2002/05/30 15:36:26 gehenna Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper and Gordon Ross
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

/*
 * Standalone functions specific to the Sun3X.
 */

#define _SUN3X_ XXX

/* Avoid conflicts on these: */
#define get_pte sun3x_get_pte
#define set_pte sun3x_set_pte

/*
 * We need to get the sun3x NBSG definition, even if we're
 * building this with a different sun68k target.
 */
#include <arch/sun3/include/param3x.h>

#include <sys/param.h>
#include <machine/mon.h>

#include <stand.h>

#include "libsa.h"
#include "dvma.h"
#include "saio.h"	/* enum MAPTYPES */

#include <arch/sun3/include/pte3x.h>
#include <arch/sun3/sun3x/iommu.h>
#include <arch/sun3/sun3x/vme.h>

/* Names, names... */
#define	MON_LOMEM_BASE	0
#define	MON_LOMEM_SIZE	0x400000
#define MON_LOMEM_END	(MON_LOMEM_BASE+MON_LOMEM_SIZE)
#define MON_KDB_BASE	SUN3X_MON_KDB_BASE
#define MON_KDB_SIZE	SUN3X_MON_KDB_SIZE
#define MON_KDB_END 	(MON_KDB_BASE+MON_KDB_SIZE)
#define MON_DVMA_BASE	SUN3X_MON_DVMA_BASE
#define MON_DVMA_SIZE	SUN3X_MON_DVMA_SIZE

void mmu_atc_flush (u_int va);
void set_iommupte(u_int va, u_int pa);

u_int	get_pte __P((vaddr_t va));
void	set_pte __P((vaddr_t va, u_int pte));
char *	dvma3x_alloc __P((int len));
void	dvma3x_free __P((char *dvma, int len));
char *	dvma3x_mapin __P((char *pkt, int len));
void	dvma3x_mapout __P((char *dmabuf, int len));
char *	dev3x_mapin __P((int type, u_long addr, int len));

struct mapinfo {
	int maptype;
	u_int base;
	u_int mask;
};

struct mapinfo
sun3x_mapinfo[MAP__NTYPES] = {
	/* On-board memory, I/O */
	{ MAP_MAINMEM,   0, ~0 },
	{ MAP_OBIO,      0, ~0 },
	/* Multibus adapter (A24,A16) */
	{ MAP_MBMEM,     VME24D16_BASE, VME24_MASK },
	{ MAP_MBIO,      VME16D16_BASE, VME16_MASK },
	/* VME A16 */
	{ MAP_VME16A16D, VME16D16_BASE, VME16_MASK },
	{ MAP_VME16A32D, VME16D32_BASE, VME16_MASK },
	/* VME A24 */
	{ MAP_VME24A16D, VME24D16_BASE, VME24_MASK },
	{ MAP_VME24A32D, VME24D32_BASE, VME24_MASK },
	/* VME A32 */
	{ MAP_VME32A16D, VME32D16_BASE, VME32_MASK },
	{ MAP_VME32A32D, VME32D32_BASE, VME32_MASK },
};

/* The virtual address we will use for PROM device mappings. */
u_int sun3x_devmap = MON_KDB_BASE;

char *
dev3x_mapin(maptype, physaddr, length)
	int maptype;
	u_long physaddr;
	int length;
{
	u_int i, pa, pte, pgva, va;

	if ((sun3x_devmap + length) > (MON_KDB_BASE + MON_KDB_SIZE))
		panic("dev3x_mapin: length=%d\n", length);

	for (i = 0; i < MAP__NTYPES; i++)
		if (sun3x_mapinfo[i].maptype == maptype)
			goto found;
	panic("dev3x_mapin: bad maptype");
found:

	if (physaddr & ~(sun3x_mapinfo[i].mask))
		panic("dev3x_mapin: bad address");
	pa = sun3x_mapinfo[i].base + physaddr;

	pte = pa | MMU_DT_PAGE | MMU_SHORT_PTE_CI;

	va = pgva = sun3x_devmap;
	do {
		set_pte(pgva, pte);
		pgva += NBPG;
		pte += NBPG;
		length -= NBPG;
	} while (length > 0);
	sun3x_devmap = pgva;
	va += (physaddr & PGOFSET);

#ifdef	DEBUG_PROM
	if (debug)
		printf("dev3x_mapin: va=0x%x pte=0x%x\n",
			   va, get_pte(va));
#endif
	return ((char*)va);
}

/*****************************************************************
 * DVMA support
 */

#define SA_MIN_VA	0x200000
#define SA_MAX_VA	(SA_MIN_VA + MON_DVMA_SIZE - (8 * NBPG))

#define	MON_DVMA_MAPLEN	(MON_DVMA_SIZE - NBPG)

/* This points to the end of the free DVMA space. */
u_int dvma3x_end = MON_DVMA_BASE + MON_DVMA_MAPLEN;

void
dvma3x_init()
{
	u_int va, pa;

	pa = SA_MIN_VA;
	va = MON_DVMA_BASE;

	while (pa < SA_MAX_VA) {
		set_pte(va, pa | MMU_DT_PAGE | MMU_SHORT_PTE_CI);
		set_iommupte(va, pa | IOMMU_PDE_DT_VALID | IOMMU_PDE_CI);
		va += NBPG;
		pa += NBPG;
	}
}

/* Convert a local address to a DVMA address. */
char *
dvma3x_mapin(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < SA_MIN_VA) || (va >= SA_MAX_VA))
		panic("dvma3x_mapin");

	va -= SA_MIN_VA;
	va += MON_DVMA_BASE;

	return ((char *) va);
}

/* Convert a DVMA address to a local address. */
void
dvma3x_mapout(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < MON_DVMA_BASE) ||
		(va >= (MON_DVMA_BASE + MON_DVMA_MAPLEN)))
		panic("dvma3x_mapout");
}

char *
dvma3x_alloc(int len)
{
	len = m68k_round_page(len);
	dvma3x_end -= len;
	return((char*)dvma3x_end);
}

void
dvma3x_free(char *dvma, int len)
{
	/* not worth the trouble */
}

/*****************************************************************
 * MMU (and I/O MMU) support
 */

u_int
get_pte(va)
	vaddr_t va;	/* virt. address */
{
	u_int	pn;
	mmu_short_pte_t *tbl;

	if (va >= MON_LOMEM_BASE && va < MON_LOMEM_END) {
		tbl = (mmu_short_pte_t *) *romVectorPtr->lomemptaddr;
	} else if (va >= MON_KDB_BASE && va < MON_KDB_END) {
		va -= MON_KDB_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->monptaddr;
	} else if (va >= MON_DVMA_BASE) {
		va -= MON_DVMA_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->shadowpteaddr;
	} else {
		return 0;
	}

	/* Calculate the page number within the selected table. */
	pn = (va >> MMU_PAGE_SHIFT);
	/* Extract the PTE from the table. */
	return tbl[pn].attr.raw;
}

void
set_pte(va, pa)
	vaddr_t va;	/* virt. address */
	u_int pa;	/* phys. address */
{
	u_int	pn;
	mmu_short_pte_t *tbl;

	if (va >= MON_LOMEM_BASE && va < (MON_LOMEM_BASE + MON_LOMEM_SIZE)) {
		/*
		 * Main memory range.
		 */
		tbl = (mmu_short_pte_t *) *romVectorPtr->lomemptaddr;
	} else if (va >= MON_KDB_BASE && va < (MON_KDB_BASE + MON_KDB_SIZE)) {
		/*
		 * Kernel Debugger range.
		 */
		va -= MON_KDB_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->monptaddr;
	} else if (va >= MON_DVMA_BASE) {
		/*
		 * DVMA range.
		 */
		va -= MON_DVMA_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->shadowpteaddr;
	} else {
		/* invalid range */
		return;
	}

	/* Calculate the page number within the selected table. */
	pn = (va >> MMU_PAGE_SHIFT);
	/* Enter the PTE into the table. */
	tbl[pn].attr.raw = pa;
	/* Flush the ATC of any cached entries for the va. */
	mmu_atc_flush(va);
}

void
mmu_atc_flush(va)
	u_int va;
{

	__asm __volatile ("pflush	#0,#0,%0@" : : "a" (va));
}

void
set_iommupte(va, pa)
	u_int va;	/* virt. address */
	u_int pa;	/* phys. address */
{
	iommu_pde_t *iommu_va;
	int pn;

	iommu_va = (iommu_pde_t *) *romVectorPtr->dvmaptaddr;

	/* Adjust the virtual address into an offset within the DVMA map. */
	va -= MON_DVMA_BASE;

	/* Convert the slave address into a page index. */
	pn = IOMMU_BTOP(va);

	iommu_va[pn].addr.raw = pa;
}

/*****************************************************************
 * Init our function pointers, etc.
 */

void
sun3x_init()
{

	/* Set the function pointers. */
	dev_mapin_p   = dev3x_mapin;
	dvma_alloc_p  = dvma3x_alloc;
	dvma_free_p   = dvma3x_free;
	dvma_mapin_p  = dvma3x_mapin;
	dvma_mapout_p = dvma3x_mapout;

	dvma3x_init();
}
