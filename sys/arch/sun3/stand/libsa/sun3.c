/*	$NetBSD: sun3.c,v 1.4 2001/09/05 13:34:54 tsutsui Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 * Standalone functions specific to the Sun3.
 */

#define _SUN3_ XXX

/* Need to avoid conflicts on these: */
#define get_pte sun3_get_pte
#define set_pte sun3_set_pte
#define get_segmap sun3_get_segmap
#define set_segmap sun3_set_segmap

#include <sys/param.h>
#include <machine/idprom.h>
#include <machine/mon.h>
#include <machine/pte.h>

#include <arch/sun3/sun3/control.h>
#include <arch/sun3/sun3/vme.h>

#include <stand.h>

#include "libsa.h"
#include "dvma.h"
#include "saio.h"	/* enum MAPTYPES */

#define OBIO_MASK 0xFFFFFF

char *	dvma3_alloc  __P((int len));
void	dvma3_free  __P((char *dvma, int len));
char *	dvma3_mapin  __P((char *pkt, int len));
void	dvma3_mapout  __P((char *dmabuf, int len));
char *	dev3_mapin  __P((int type, u_long addr, int len));

struct mapinfo {
	int maptype;
	int pgtype;
	u_int base;
	u_int mask;
};

struct mapinfo
sun3_mapinfo[MAP__NTYPES] = {
	/* On-board memory, I/O */
	{ MAP_MAINMEM,   PGT_OBMEM,   0,          ~0 },
	{ MAP_OBIO,      PGT_OBIO,    0,          OBIO_MASK },
	/* Multibus adapter (A24,A16) */
	{ MAP_MBMEM,     PGT_VME_D16, VME24_BASE, VME24_MASK },
	{ MAP_MBIO,      PGT_VME_D16, VME16_BASE, VME16_MASK },
	/* VME A16 */
	{ MAP_VME16A16D, PGT_VME_D16, VME16_BASE, VME16_MASK },
	{ MAP_VME16A32D, PGT_VME_D32, VME16_BASE, VME16_MASK },
	/* VME A24 */
	{ MAP_VME24A16D, PGT_VME_D16, VME24_BASE, VME24_MASK },
	{ MAP_VME24A32D, PGT_VME_D32, VME24_BASE, VME24_MASK },
	/* VME A32 */
	{ MAP_VME32A16D, PGT_VME_D16, VME32_BASE, VME32_MASK },
	{ MAP_VME32A32D, PGT_VME_D32, VME32_BASE, VME32_MASK },
};

/* The virtual address we will use for PROM device mappings. */
int sun3_devmap = SUN3_MONSHORTSEG;

char *
dev3_mapin(maptype, physaddr, length)
	int maptype;
	u_long physaddr;
	int length;
{
	u_int i, pa, pte, pgva, va;

	if ((sun3_devmap + length) > SUN3_MONSHORTPAGE)
		panic("dev3_mapin: length=%d\n", length);

	for (i = 0; i < MAP__NTYPES; i++)
		if (sun3_mapinfo[i].maptype == maptype)
			goto found;
	panic("dev3_mapin: bad maptype");
found:

	if (physaddr & ~(sun3_mapinfo[i].mask))
		panic("dev3_mapin: bad address");
	pa = sun3_mapinfo[i].base += physaddr;

	pte = PA_PGNUM(pa) | PG_PERM |
		sun3_mapinfo[i].pgtype;

	va = pgva = sun3_devmap;
	do {
		set_pte(pgva, pte);
		pgva += NBPG;
		pte += 1;
		length -= NBPG;
	} while (length > 0);
	sun3_devmap = pgva;
	va += (physaddr & PGOFSET);

#ifdef	DEBUG_PROM
	if (debug)
		printf("dev3_mapin: va=0x%x pte=0x%x\n",
			   va, get_pte(va));
#endif
	return ((char*)va);
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

#define	DVMA_BASE 0xFFf00000
#define DVMA_MAPLEN  0xE0000	/* 1 MB - 128K (save MONSHORTSEG) */

#define SA_MIN_VA	0x200000
#define SA_MAX_VA	(SA_MIN_VA + DVMA_MAPLEN)

/* This points to the end of the free DVMA space. */
u_int dvma3_end = DVMA_BASE + DVMA_MAPLEN;

void
dvma3_init()
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
dvma3_mapin(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < SA_MIN_VA) || (va >= SA_MAX_VA))
		panic("dvma3_mapin");

	va -= SA_MIN_VA;
	va += DVMA_BASE;

	return ((char *) va);
}

/* Destroy a DVMA address alias. */
void
dvma3_mapout(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < DVMA_BASE) || (va >= (DVMA_BASE + DVMA_MAPLEN)))
		panic("dvma3_mapout");
}

char *
dvma3_alloc(int len)
{
	len = m68k_round_page(len);
	dvma3_end -= len;
	return((char*)dvma3_end);
}

void
dvma3_free(char *dvma, int len)
{
	/* not worth the trouble */
}

/*****************************************************************
 * Control space stuff...
 */

u_int
get_pte(va)
	vaddr_t va;
{
	va = CONTROL_ADDR_BUILD(PGMAP_BASE, va);
	return (get_control_word(va));
}

void
set_pte(va, pte)
	vaddr_t va;
	u_int pte;
{
	va = CONTROL_ADDR_BUILD(PGMAP_BASE, va);
	set_control_word(va, pte);
}

int
get_segmap(va)
	vaddr_t va;
{
	va = CONTROL_ADDR_BUILD(SEGMAP_BASE, va);
	return (get_control_byte(va));
}

void
set_segmap(va, sme)
	vaddr_t va;
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
sun3_getidprom(u_char *dst)
{
	vaddr_t src;		/* control space address */
	int len, x;

	src = IDPROM_BASE;
	len = sizeof(struct idprom);
	do {
		x = get_control_byte(src++);
		*dst++ = x;
	} while (--len > 0);
}

/*****************************************************************
 * Init our function pointers, etc.
 */

void
sun3_init()
{

	/* Set the function pointers. */
	dev_mapin_p   = dev3_mapin;
	dvma_alloc_p  = dvma3_alloc;
	dvma_free_p   = dvma3_free;
	dvma_mapin_p  = dvma3_mapin;
	dvma_mapout_p = dvma3_mapout;

	/* Prepare DVMA segment. */
	dvma3_init();
}
