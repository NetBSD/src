/*
 * Copyright (C) 1994	Allen K. Briggs
 * All rights reserved.
 *
 * Large parts of this file are copied from the amiga_init.c file from
 * the NetBSD/Amiga port as of the summer of 1994.  The following fine
 * authors are credited in that file:
 *	Markus Wild, Bryan Ford, Niklas Hallqvist, and Michael L. Hitch
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: mac68k_init.c,v 1.4.2.1 1994/07/24 01:23:35 cgd Exp $
 *
 */

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/cpu.h>

/* Let us map a 68040 system at (close to) boot time. */

extern u_int	Sysptmap, Sysptsize, Sysseg, Umap, proc0paddr;
extern u_int	Sysseg1;

extern u_long	esym;	/* Set in machdep.c:getenvvars() */

static u_int	Sysseg1_pa;

extern volatile unsigned char	*sccA;
extern volatile unsigned char	*Via1Base;
extern unsigned char		*ASCBase;
extern unsigned long		videoaddr;

static void
debug_translate(u_int val)
{
	u_int	*p, f;

	strprintf("translate", val);
	p = (u_int *) Sysseg1_pa;
	/* Get root index */
	f = (val&SG_IMASK1) >> SG_ISHIFT1;
	strprintf("  root index", f);

	p = (u_int *) (p[f] & ~0xf);
	/* Get segment index */
	f = (val&SG_IMASK2) >> SG_040ISHIFT;
	strprintf("  segment index", f);
	if (p[f]) {
		p = (u_int *) (p[f] & ~0xf);
		/* Get page index */
		f = (val & SG_040PMASK) >> SG_PSHIFT;
		strprintf("  page index", f);
		if (p[f]) {
			f = p[f] & ~0xf;
			strprintf("     into", f + (val & 0xfff));
			return;
		}
	}
	strprintf("     into nothing", 0);
}
	
/*
 * We only have a minimal stack when this is called.  Assuming that
 * we're dealing with what I will call a Quadra 700-class machine,
 * the main memory is currently mapped pretty close to logical==physical,
 * the MMU is enabled, and the current page tables are at the end of
 * physical RAM.
 *
 * Several of the techniques were scammed from the Amiga code.
 *
 * On the list of "to-do"s is a clean-up of this code to allow for
 * more 040-like action--right now, the 040 is pretty much emulating
 * an 851/030 mmu style.
 */
void
map040(void)
{
extern void	etext(); /* Okaaaaay... */
	u_int	vstart, vend, pstart, pend, avail;
	u_int	Sysseg_pa, /*Sysseg1_pa,*/ Sysptmap_pa, umap_pa;
	u_int	pt, ptpa, ptextra, ptsize;
	u_int	p0_ptpa, p0_u_area_pa, i;
	u_int	sg_proto, pg_proto;
	u_int	*sg, *pg, *pg2;
	u_int	oldIOBase, oldNBBase;

	oldIOBase = IOBase;
	oldNBBase = NuBusBase;
	/* init "tracking" values */
	vend   = get_top_of_ram();
	avail  = vend;
	vstart = mac68k_round_page(esym);
	pstart = vstart + load_addr;
	pend   = vend   + load_addr;
	avail -= vstart;

	/*
	 * Allocate the kernel 1st level segment table.
	 */
	Sysseg1_pa = pstart;
	Sysseg1 = vstart;
	vstart += NBPG;
	pstart += NBPG;
	avail  -= NBPG;

	/*
	 * Allocate the kernel segment table.
	 */
	Sysseg_pa = pstart;
	Sysseg = vstart;
	vstart += NBPG * 16;
	pstart += NBPG * 16;
	avail  -= NBPG * 16;

	/*
	 * Allocate initial page table pages.
	 */
	pt = vstart;
	ptpa = pstart;
	ptextra = IIOMAPSIZE + NBMAPSIZE;
	ptsize = (Sysptsize + howmany(ptextra, NPTEPG)) << PGSHIFT;
	vstart += ptsize;
	pstart += ptsize;
	avail  -= ptsize;

	/*
	 * Allocate kernel page table map.
	 */
	Sysptmap = vstart;
	Sysptmap_pa = pstart;
	vstart += NBPG * Sysptsize;
	pstart += NBPG * Sysptsize;
	avail  -= NBPG * Sysptsize;

	/*
	 * Set Sysmap; mapped after page table pages.
	 */
	Sysmap = (struct pte *) (ptsize << (12));

	/*
	 * Initialize segment table and page table map.
	 */
	sg_proto = Sysseg_pa | SG_RW | SG_V;

	/*
	 * Map all level 1 entries to the segment table.
	 */
	sg = (u_int *) Sysseg1_pa;
	while (sg_proto < ptpa) {
		*sg++ = sg_proto;
		sg_proto += MAC_040STSIZE;
	}
	/*
	 * Clear remainder of root table.
	 */
	while (sg < (u_int *) Sysseg_pa)
		*sg++ = SG_NV;

	sg_proto = ptpa | SG_RW | SG_V;
	pg_proto = ptpa | PG_RW | PG_CI | PG_V;
	/*
	 * Map so many segs.
	 */
	sg = (u_int *) Sysseg_pa;
	pg = (u_int *) Sysptmap_pa;
	while (sg_proto < pstart) {
		*sg++ = sg_proto;
		if (pg_proto < pstart)
			*pg++ = pg_proto;
		else if (pg < (u_int *) (Sysptmap_pa + NBPG * Sysptsize))
			*pg++ = PG_NV;
		sg_proto += MAC_040PTSIZE;
		pg_proto += NBPG;
	}
	/*
	 * Invalidate remainder of table.
	 */
	do {
		*sg++ = SG_NV;
		if (pg < (u_int *) (Sysptmap_pa + NBPG * Sysptsize))
			*pg++ = PG_NV;
	} while (sg < (u_int *) (Sysseg_pa + NBPG * 16));

	/*
	 * Portions of the last segment of KVA space (0xFFF0 0000 -
	 * 0xFFFF FFFF are mapped for the current process u-area.
	 * Specifically, 0xFFFF C000 - 0xFFFF FFFF is mapped.  This
	 * translates to (u + kernel stack).
	 */
	/*
	 * Use next available slot.
	 */
	sg_proto = (pstart + NBPG - MAC_040PTSIZE) | SG_RW | SG_V;
	umap_pa  = pstart; /* Remember for later map entry. */
	/*
	 * Enter the page into the level 2 segment table.
	 */
	sg = (u_int *) (Sysseg_pa + NBPG * 16);
	while (sg_proto > pstart) {
		*--sg = sg_proto;
		sg_proto -= MAC_040PTSIZE;
	}
	/*
	 * Enter the page into the page table map.
	 */
	pg_proto = pstart | PG_RW | PG_CI | PG_V;
	pg = (u_int *) (Sysptmap_pa + 1024);
	*--pg = pg_proto;
	/*
	 * Invalidate all pte's (will validate u-area afterwards)
	 */
	for (pg = (u_int *) pstart ; pg < (u_int *) (pstart + NBPG) ; )
		*pg++ = PG_NV;
	/*
	 * Account for the allocated page.
	 */
	vstart += NBPG;
	pstart += NBPG;
	avail  -= NBPG;

	/*
	 * Record KVA at which to access current u-area PTE(s).
	 */
	Umap = (u_int) Sysmap + NPTEPG*NBPG - HIGHPAGES * 4;

	/*
	 * Initialize kernel page table page(s) (assume load at VA 0)
	 */
	pg_proto = load_addr | PG_RO | PG_V;
	pg       = (u_int *) ptpa;
	for (i = 0 ; i < (u_int) etext ; i+=NBPG , pg_proto+=NBPG )
		*pg++ = pg_proto;
	
	/*
	 * Data, BSS and dynamic tables are read/write.
	 */
	pg_proto = (pg_proto&PG_FRAME) | PG_RW | PG_V;
	pg_proto |= PG_CCB;
	/*
	 * Go until end of data allocated so far plus proc0 PT/u-area
	 * (to be allocated below)
	 */
	for ( ; i < vstart + (UPAGES + 1) * NBPG ; i+=NBPG , pg_proto += NBPG )
		*pg++ = pg_proto;
	/*
	 * Invalidate remainder of kernel PT.
	 */
	while (pg < (u_int *) (ptpa + ptsize))
		*pg++ = PG_NV;
	
	/*
	 * Go back and validate I/O space.
	 */
	pg      -= ptextra;
	pg2      = pg;
	pg_proto = (IOBase & PG_FRAME) | PG_RW | PG_CI | PG_V;
	while (pg_proto < INTIOTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}

	/*
	 * Go validate NuBus space.
	 */
	pg_proto = (NBBASE & PG_FRAME) | PG_RW | PG_CI | PG_V;
					/* Need CI, here? (akb) */
	while (pg_proto < NBTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}

	/*
	 * Record base KVA of I/O and NuBus spaces.
	 */
	IOBase = (u_int) Sysmap - ptextra * NBPG;
	NuBusBase = IOBase + IIOMAPSIZE * NBPG;

	/*
	 * Make proper segment table entries for these, now.
	 */
	sg_proto = ((u_int)pg2) | SG_RW | SG_V;
	i =(((u_int) IOBase) & SG_IMASK1) >> SG_ISHIFT1;
	sg = (u_int *) ((((u_int *) Sysseg1)[i]) & ~0x7f);
	sg += (((u_int) IOBase) & SG_IMASK2) >> SG_040ISHIFT;
	while (sg_proto < (u_int) pg) {
		*sg++ = sg_proto;
		sg_proto += MAC_040PTSIZE;
	}

	/*
	 * This is bogus..  This happens automatically
	 * on the Amiga, I think...
	 */
	sg_proto = Sysptmap_pa | SG_RW | SG_V;
	i = (((u_int) Sysmap) & SG_IMASK1) >> SG_ISHIFT1;
	sg = (u_int *) ((((u_int *) Sysseg1)[i]) & ~0x7f);
	sg += (((u_int) Sysmap) & SG_IMASK2) >> SG_040ISHIFT;
	while (sg_proto < Sysptmap_pa + Sysptsize * NBPG) {
		*sg++ = sg_proto;
		sg_proto += MAC_040PTSIZE;
	}

	/*
	 * Setup page table for process 0.
	 * We set up page table access for the kernel via Usrptmap (usrpt)
	 * [no longer used?] and access the u-area itself via Umap (u).
	 * First available page (vstart/pstart) is used for proc0 page table.
	 * Next UPAGES (page(s)) following are for u-area.
	 */

	p0_ptpa = pstart;
	vstart += NBPG;
	pstart += NBPG;
	avail  -= NBPG;

	p0_u_area_pa = pstart;	/* Base of u-area and end of PT */

	/*
	 * Invalidate entire page table.
	 */
	for (pg = (u_int *) p0_ptpa ; pg < (u_int *) p0_u_area_pa ; )
		*pg++ = PG_NV;
	
	/*
	 * Now go back and validate u-area PTE(s) in PT and in Umap.
	 */
	pg -= HIGHPAGES;
	pg2 = (u_int *) (umap_pa + 4*(NPTEPG - UPAGES));
	pg_proto  = p0_u_area_pa | PG_RW | PG_V;
	pg_proto |= PG_CCB;
	for (i=0 ; i < UPAGES ; i++, pg_proto += NBPG) {
		*pg++  = pg_proto;
		*pg2++ = pg_proto;
	}
	bzero ((u_char *) p0_u_area_pa, UPAGES * NBPG);

	/*
	 * Save KVA of proc0 u-area.
	 */
	proc0paddr = vstart;
	vstart += UPAGES * NBPG;
	pstart += UPAGES * NBPG;
	avail  -= UPAGES * NBPG;

	/*
	 * Init mem sizes.
	 */
	maxmem  = pend >> PGSHIFT;
	physmem = (mac68k_machine.mach_memsize*1024*1024) >> PGSHIFT;

	/*
	 * Get the pmap module in sync with reality.
	 */
	pmap_bootstrap(pstart, load_addr);

	/*
	 * Prepare to enable the MMU.
	 * [Amiga copies kernel over right before this.  This might be
	 *  necessary on the IIci/si/vx/etc., but is not necessary on
	 *  any 040 boxes that I know of (yet). ]
	 */
	/*
	 * movel Sysseg1_pa, a0;
	 * movec a0, SRP;
	 * pflusha;
	 * movel #0x8000, d0;
	 * movec d0, TC;
	 */
	asm volatile ("movel %0, a0; .word 0x4e7b,0x8807"
			: : "a" (Sysseg1_pa));
	asm volatile (".word 0xf518" : : );
	asm volatile ("movel #0x8000,d0; .word 0x4e7b,0x0003" : : );
	TBIA();

	/*
	 * (akb) I think that this is
	 * Designed to force proc0paddr to be set despite gcc's optimizer.
	 */
	i =      *(int *) proc0paddr;
	*(volatile int *) proc0paddr = i;

	/*
	 * Reset pointers that will be wrong, now.
	 * They are set this way to avoid blowing up the working MacII, et al.
	 */
	Via1Base = (volatile unsigned char *)
			((u_int) Via1Base - oldIOBase + IOBase);
	ASCBase  = (unsigned char *) ((u_int) ASCBase - oldIOBase + IOBase);
	videoaddr = videoaddr - oldNBBase + NuBusBase;
}
