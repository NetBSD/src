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
 * $Id: mac68k_init.c,v 1.1 1994/06/26 13:20:19 briggs Exp $
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

volatile unsigned char	*Via1Base = (volatile unsigned char *) INTIOBASE;
unsigned long		NuBusBase = NBBASE;
unsigned long		IOBase    = INTIOBASE;
int			has5380scsi = 0;	/* Set in setmachdep() */
int			has53c96scsi = 0;	/* Set in setmachdep() */

extern volatile unsigned char	*sccA;
extern unsigned char		*ASCBase;
extern unsigned long		videoaddr;

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
	u_int	Sysseg_pa, Sysseg1_pa, Sysptmap_pa, umap_pa;
	u_int	pt, ptpa, ptextra, ptsize;
	u_int	p0_ptpa, p0_u_area_pa, i;
	u_int	sg_proto, pg_proto;
	u_int	*sg, *pg, *pg2;

	/* init "tracking" values */
	vend   = get_top_of_ram();
	avail  = vend;
	vstart = mac68k_round_page(esym);
	pstart = vstart + load_addr;
	pend   = vend   + load_addr;
strprintf("vstart", vstart);
strprintf("vend", vend);
strprintf("pstart", pstart);
strprintf("pend", pend);
strprintf("avail", avail);
	avail -= vstart;
strprintf("avail", avail);

	/*
	 * Allocate the kernel 1st level segment table.
	 */
	Sysseg1_pa = pstart;
	Sysseg1 = vstart;
	vstart += NBPG;
	pstart += NBPG;
	avail  -= NBPG;
strprintf("avail", avail);

	/*
	 * Allocate the kernel segment table.
	 */
	Sysseg_pa = pstart;
	Sysseg = vstart;
	vstart += NBPG * 16; /* Amiga used 8 instead of 16 */
	pstart += NBPG * 16; /* Amiga used 8 instead of 16 */
	avail  -= NBPG * 16; /* Amiga used 8 instead of 16 */
strprintf("avail", avail);

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
strprintf("avail", avail);

	/*
	 * Allocate kernel page table map.
	 */
	Sysptmap = vstart;
	Sysptmap_pa = pstart;
	vstart += NBPG*Sysptsize;
	pstart += NBPG*Sysptsize;
	avail  -= NBPG*Sysptsize;
strprintf("avail", avail);

	/*
	 * Set Sysmap; mapped after page table pages.
	 */
???????	Sysmap = (struct pte *) (ptsize << 11);

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
		sg_proto += MAC_040RTSIZE;
	}
	sg_proto = ptpa | SG_RW | SG_V;
	pg_proto = ptpa | PG_RW | PG_CI | PG_V;
	/*
	 * Map so many segs.
	 */
strprintf("avail", avail);
	sg = (u_int *) Sysseg_pa;
	pg = (u_int *) Sysptmap_pa;
	while (sg_proto < pstart) {
		*sg++ = sg_proto;
		if (pg_proto < pstart)
			*pg++ = pg_proto;
		else if (pg < (u_int *) (Sysptmap_pa + NBPG))
			*pg++ = PG_NV;
		sg_proto += MAC_040PTSIZE;
		pg_proto += NBPG;
	}
	/*
	 * Invalidate remainder of table.
	 */
	do {
		*sg++ = SG_NV;
		if (pg < (u_int *) (Sysptmap_pa + NBPG))
			*pg++ = PG_NV;
	} while (sg < (u_int *) (Sysseg_pa + NBPG * 16)); /* amiga had 8 */

strprintf("avail", avail);
	/*
	 * The end of the last segment (0xFFFC 0000) of KVA space is
	 * used to map the u-area of the current process (u + kernel stack)
	 */
	umap_pa = pstart;
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
	pg = (u_int *) (Sysptmap_pa + 1024); /****** Fix constant ******/
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
strprintf("Record... avail", avail);

	/*
	 * Record KVA at which to access current u-area PTE(s).
	 */
	Umap = (u_int) Sysmap + MAC_MAX_PTSIZE - UPAGES * 4;

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
	
strprintf("iospace... avail", avail);
	/*
	 * Go back and validate I/O space.
	 */
	pg      -= ptextra;
	pg_proto = INTIOBASE | PG_RW | PG_CI | PG_V;
	while (pg_proto < INTIOTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}

strprintf("Nubus... avail", avail);
	/*
	 * Go validate NuBus space.
	 */
	pg_proto = NBBASE | PG_RW | PG_CI | PG_V; /* Need CI, here? (akb) */
	while (pg_proto < NBTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
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
strprintf("p0_u_area... avail", avail);

	p0_u_area_pa = pstart;	/* Base of u-area and end of PT */

	/*
	 * Invalidate entire page table.
	 */
	for (pg = (u_int *) p0_ptpa ; pg < (u_int *) p0_u_area_pa ; )
		*pg++ = PG_NV;
	
	/*
	 * Now go back and validate u-area PTE(s) in PT and in Umap.
	 */
	pg -= UPAGES;
	pg2 = (u_int *) (umap_pa + 4*(NPTEPG - UPAGES));
	pg_proto  = p0_u_area_pa | PG_RW | PG_V;
	pg_proto |= PG_CCB;
	for (i=0 ; i < UPAGES ; i++, pg_proto += NBPG) {
		*pg++  = pg_proto;
		*pg2++ = pg_proto;
	}
	bzero ((u_char *) p0_u_area_pa, UPAGES * NBPG);

strprintf("save kva... avail", avail);
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
strprintf("maxmem", maxmem);
	physmem = (mach_memsize*1024*1024) >> PGSHIFT;
strprintf("physmem", physmem);

	/*
	 * Get the pmap module in sync with reality.
	 */
	pmap_bootstrap(pstart, load_addr);

	/*
	 * Record base KVA of I/O and NuBus spaces.
	 */
	IOBase = (u_int) Sysmap - ptextra * NBPG;
	NuBusBase = IOBase + IIOMAPSIZE * NBPG;
strprintf("IOBase", IOBase);
strprintf("NuBusBase", NuBusBase);
	
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

	videoaddr = videoaddr - NBBASE + NuBusBase;
	NewScreenAddress();
strprintf("videoaddr", videoaddr);

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
	sccA = (volatile unsigned char *) ((u_int) sccA - INTIOBASE + IOBase);
	Via1Base = (volatile unsigned char *)
			((u_int) Via1Base - INTIOBASE + IOBase);
	ASCBase  = (unsigned char *) ((u_int) ASCBase - INTIOBASE + IOBase);
}
