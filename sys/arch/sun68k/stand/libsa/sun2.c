/*	$NetBSD: sun2.c,v 1.1 2001/06/14 12:57:15 fredette Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Matthew Fredette.
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
 * Standalone functions specific to the Sun2.
 */

/* Need to avoid conflicts on these: */
#define get_pte sun2_get_pte
#define set_pte sun2_set_pte
#define get_segmap sun2_get_segmap
#define set_segmap sun2_set_segmap

#include <sys/param.h>
#include <machine/idprom.h>
#include <machine/mon.h>

#include <arch/sun2/include/pte.h>
#include <arch/sun2/sun2/control.h>
#ifdef notyet
#include <arch/sun3/sun3/vme.h>
#else
#define VME16_BASE MBIO_BASE
#define VME16_MASK MBIO_MASK
#endif
#include <arch/sun2/sun2/mbmem.h>
#include <arch/sun2/sun2/mbio.h>

#include <stand.h>

#include "libsa.h"
#include "dvma.h"
#include "saio.h"	/* enum MAPTYPES */

#define OBIO_MASK 0xFFFFFF

u_int	get_pte __P((vm_offset_t va));
void	set_pte __P((vm_offset_t va, u_int pte));
char *	dvma2_alloc  __P((int len));
void	dvma2_free  __P((char *dvma, int len));
char *	dvma2_mapin  __P((char *pkt, int len));
void	dvma2_mapout  __P((char *dmabuf, int len));
char *	dev2_mapin  __P((int type, u_long addr, int len));

struct mapinfo {
	int maptype;
	int pgtype;
	u_int base;
	u_int mask;
};

#ifdef	notyet
struct mapinfo
sun2_mapinfo[MAP__NTYPES] = {
	/* On-board memory, I/O */
	{ MAP_MAINMEM,   PGT_OBMEM,   0,          ~0 },
	{ MAP_OBIO,      PGT_OBIO,    0,          OBIO_MASK },
	/* Multibus memory, I/O */
	{ MAP_MBMEM,     PGT_MBMEM, MBMEM_BASE, MBMEM_MASK },
	{ MAP_MBIO,      PGT_MBIO,  MBIO_BASE, MBIO_MASK },
	/* VME A16 */
	{ MAP_VME16A16D, PGT_VME_D16, VME16_BASE, VME16_MASK },
	{ MAP_VME16A32D, 0, 0, 0 },
	/* VME A24 */
	{ MAP_VME24A16D, 0, 0, 0 },
	{ MAP_VME24A32D, 0, 0, 0 },
	/* VME A32 */
	{ MAP_VME32A16D, 0, 0, 0 },
	{ MAP_VME32A32D, 0, 0, 0 },
};
#endif

/* The virtual address we will use for PROM device mappings. */
int sun2_devmap = SUN3_MONSHORTSEG;

char *
dev2_mapin(maptype, physaddr, length)
	int maptype;
	u_long physaddr;
	int length;
{
#ifdef	notyet
	u_int i, pa, pte, pgva, va;

	if ((sun2_devmap + length) > SUN3_MONSHORTPAGE)
		panic("dev2_mapin: length=%d\n", length);

	for (i = 0; i < MAP__NTYPES; i++)
		if (sun2_mapinfo[i].maptype == maptype)
			goto found;
	panic("dev2_mapin: bad maptype");
found:

	if (physaddr & ~(sun2_mapinfo[i].mask))
		panic("dev2_mapin: bad address");
	pa = sun2_mapinfo[i].base += physaddr;

	pte = PA_PGNUM(pa) | PG_PERM |
		sun2_mapinfo[i].pgtype;

	va = pgva = sun2_devmap;
	do {
		set_pte(pgva, pte);
		pgva += NBPG;
		pte += 1;
		length -= NBPG;
	} while (length > 0);
	sun2_devmap = pgva;
	va += (physaddr & PGOFSET);

#ifdef	DEBUG_PROM
	if (debug)
		printf("dev2_mapin: va=0x%x pte=0x%x\n",
			   va, get_pte(va));
#endif
	return ((char*)va);
#else
	panic("dev2_mapin");
	return(NULL);
#endif
}

/*****************************************************************
 * DVMA support
 */

/*
 * The easiest way to deal with the need for DVMA mappings is to
 * create a DVMA alias mapping of the entire address range used by
 * the boot program.  That way, dvma_mapin can just compute the
 * DVMA alias address, and dvma_mapout does nothing.
 *
 * Note that this assumes that standalone programs will do I/O
 * operations only within range (SA_MIN_VA .. SA_MAX_VA) checked.
 */

#define DVMA_BASE 0x00f00000
#define DVMA_MAPLEN  0x38000	/* 256K - 32K (save MONSHORTSEG) */

#define SA_MIN_VA	0x220000
#define SA_MAX_VA	(SA_MIN_VA + DVMA_MAPLEN)

/* This points to the end of the free DVMA space. */
u_int dvma2_end = DVMA_BASE + DVMA_MAPLEN;

void
dvma2_init()
{
	int segva, dmava, sme;

	segva = SA_MIN_VA;
	dmava = DVMA_BASE;

	while (segva < SA_MAX_VA) {
		sme = get_segmap(segva);
		set_segmap(dmava, sme);
		segva += NBSG;
		dmava += NBSG;
	}
}

/* Convert a local address to a DVMA address. */
char *
dvma2_mapin(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < SA_MIN_VA) || (va >= SA_MAX_VA))
		panic("dvma2_mapin: 0x%x outside 0x%x..0x%x\n", 
		    va, SA_MIN_VA, SA_MAX_VA);

	va -= SA_MIN_VA;
	va += DVMA_BASE;

	return ((char *) va);
}

/* Destroy a DVMA address alias. */
void
dvma2_mapout(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < DVMA_BASE) || (va >= (DVMA_BASE + DVMA_MAPLEN)))
		panic("dvma2_mapout");
}

char *
dvma2_alloc(int len)
{
	len = m68k_round_page(len);
	dvma2_end -= len;
	return((char*)dvma2_end);
}

void
dvma2_free(char *dvma, int len)
{
	/* not worth the trouble */
}

/*****************************************************************
 * Control space stuff...
 */

u_int
get_pte(va)
	vm_offset_t va;
{
	u_int pte;

	pte = get_control_word(CONTROL_ADDR_BUILD(PGMAP_BASE, va));
	if (pte & PG_VALID) {
		/* 
		 * This clears bit 30 (the kernel readable bit, which
		 * should always be set), bit 28 (which should always
		 * be set) and bit 26 (the user writable bit, which we
		 * always have tracking the kernel writable bit).  In
		 * the protection, this leaves bit 29 (the kernel
		 * writable bit) and bit 27 (the user readable bit).
		 * See pte2.h for more about this hack.
		 */
		pte &= ~(0x54000000);
		/*
		 * Flip bit 27 (the user readable bit) to become bit
		 * 27 (the PG_SYSTEM bit).
		 */
		pte ^= (PG_SYSTEM);
	}
	return (pte);
}

void
set_pte(va, pte)
	vm_offset_t va;
	u_int pte;
{
	if (pte & PG_VALID) {
		/* Clear bit 26 (the user writable bit).  */
		pte &= (~0x04000000);
		/*
		 * Flip bit 27 (the PG_SYSTEM bit) to become bit 27
		 * (the user readable bit).
		 */
		pte ^= (PG_SYSTEM);
		/*
		 * Always set bits 30 (the kernel readable bit) and
		 * bit 28, and set bit 26 (the user writable bit) iff
		 * bit 29 (the kernel writable bit) is set *and* bit
		 * 27 (the user readable bit) is set.  This latter bit
		 * of logic is expressed in the bizarre second term
		 * below, chosen because it needs no branches.
		 */
#if (PG_WRITE >> 2) != PG_SYSTEM
#error	"PG_WRITE and PG_SYSTEM definitions don't match!"
#endif
		pte |= 0x50000000
		    | ((((pte & PG_WRITE) >> 2) & pte) >> 1);
	}
	set_control_word(CONTROL_ADDR_BUILD(PGMAP_BASE, va), pte);
}

int
get_segmap(va)
	vm_offset_t va;
{
	va = CONTROL_ADDR_BUILD(SEGMAP_BASE, va);
	return (get_control_byte(va));
}

void
set_segmap(va, sme)
	vm_offset_t va;
	int sme;
{
	va = CONTROL_ADDR_BUILD(SEGMAP_BASE, va);
	set_control_byte(va, sme);
}

/*
 * Copy the IDPROM contents into the passed buffer.
 * The caller (idprom.c) will do the checksum.
 */
void
sun2_getidprom(u_char *dst)
{
	vm_offset_t src;	/* control space address */
	int len, x;

	src = IDPROM_BASE;
	len = sizeof(struct idprom);
	do {
		x = get_control_byte(src);
		src += NBPG;
		*dst++ = x;
	} while (--len > 0);
}

/*****************************************************************
 * Init our function pointers, etc.
 */

void
sun2_init()
{
	vm_offset_t va;
	u_int pte;
	u_int pte_zero;

	/* Set the function pointers. */
	dev_mapin_p   = dev2_mapin;
	dvma_alloc_p  = dvma2_alloc;
	dvma_free_p   = dvma2_free;
	dvma_mapin_p  = dvma2_mapin;
	dvma_mapout_p = dvma2_mapout;
       
	/* Prepare DVMA segment. */
	dvma2_init();

	/*
	 * XXX fredette - the PROM in my Sun 2/120 doesn't map a whole
	 * lot of main memory contiguously from zero, only
	 * 768KB. (0x0c0000).  This probably has something to do with
	 * the PROM using physical addresses 0x0c0000 - 0x0fffff for
	 * its DVMA space.  We'll need more contiguous memory to bring
	 * in the kernel, so we make sure that the first 1.25MB
	 * (0x140000) of main memory is mapped contiguously.  This value
	 * was chosen because my 2/120 PROM maps mbmem space at VA
	 * 0x140000, and I don't want to mess with it, and 1.25MB will
	 * be enough for my early kernels. We use the PTE for page 0 as 
	 * a template.
	 */
#define MAINMEM_MAP_BYTES (0x140000)
	pte_zero = (get_pte(0) & ~PG_FRAME);
	for(va = NBPG; va < MAINMEM_MAP_BYTES; va += NBPG) {
		pte = pte_zero | PA_PGNUM(va);
		set_pte(va, pte);
	}
}
