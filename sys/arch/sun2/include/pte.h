/*	$NetBSD: pte.h,v 1.1 2001/03/29 04:58:52 fredette Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef	_MACHINE_PTE_H
#define	_MACHINE_PTE_H

#define NCONTEXT 8
#define NPMEG	256
#define SEGINV	(NPMEG-1)
#define NPAGSEG 16
#define NSEGMAP 512

/*
 * In our zeal to use the sun3 pmap with as few changes as possible,
 * we pretend that sun2 page table entries work more like their sun3
 * counterparts.  Namely, we pretend that they simply have PG_WRITE
 * and PG_SYSTEM bits, and we use get_pte and set_pte to translate
 * entries between the two styles.  
 *
 * All known valid protections in a real sun2 PTE are given in 
 * (disabled) defines below, and are displayed as bitmaps here:
 *
 * 3 2 2 2 2
 * 0 9 8 7 6   meaning
 * -------------------
 * 1 1 1 0 0   PG_KW => a read/write kernel-only page.
 * 1 0 1 0 0   PG_KR => a read-only kernel-only page.
 * 1 1 1 1 1   PG_UW => a read/write kernel/user page.
 * 1 0 1 1 0   PG_URKR => a read-only kernel/user page.
 *
 * The sun3 PTE protections we want to emulate are:
 *
 * PG_SYSTEM | PG_WRITE => a read/write kernel-only page.
 * PG_SYSTEM            => a read-only kernel-only page.
 *             PG_WRITE => a read/write kernel/user page.
 *                      => a read-only kernel/user page.
 * 
 * We want to assign values to PG_SYSTEM and PG_WRITE, and
 * craft get_pte and set_pte to do a translation from and to the real
 * hardware protections.
 *
 * We begin by noting that bits 30 and 28 are set in all known valid
 * sun2 protections.  Since we assume that the kernel can always read
 * all pages in the system, we might as well call one of them the
 * "kernel readable" bit, and say that the other is just always on.
 * We deem bit 30 the "kernel readable" bit.  There is some evidence
 * that bit 28 may mean "not a device" (the PROM makes PTEs for its
 * device mappings with bit 28 clear), but I'm not sure enough about
 * this to do anything about it.  So, set_pte will always set these
 * bits when it loads a valid PTE, and get_pte will always clear them
 * when it unloads a valid PTE.
 *
 * Bit 25, which SunOS calles the "fill on demand" bit, also needs
 * to be set on all valid PTEs.  Dunno any more about this bit.
 *
 * Next, we see that bit 27 is set for all pages the user can access,
 * and clear otherwise.  This bit has the opposite meaning of the sun3
 * PG_SYSTEM bit, but that's OK - we will just define PG_SYSTEM to be
 * bit 27, and set_pte and get_pte will invert it when loading or
 * unloading a valid PTE.
 * 
 * Bit 29 is set for all pages the kernel can write to.  We define
 * PG_WRITE to be bit 29.  No inverting is done.
 *
 * That leaves us to take care of bit 26.  This bit, and bit 27, need
 * to be set for all pages the user can write to.  On the sun3, all
 * user-accessible pages that the kernel can write to, the user can
 * also write to.  We can use this fact to make set_pte set bit 26 iff
 * the kernel can write to the page (PG_WRITE is set), and the user
 * can also access the page (bit 27 is set, i.e., PG_SYSTEM was clear
 * before set_pte inverted it).
 *
 * This is what makes set_pte tricky.  It begins by clearing bit 26
 * (this is paranoia, if all is working well, this bit should never be
 * set in our pseudo-sun3 PTEs).  It then flips PG_SYSTEM to become
 * the user-accessible bit.  Lastly, as the tricky part, it sets bits
 * 30 and 28, *and* sets bit 26 by shifting the expression (pte &
 * PG_WRITE) right by two to move the resulting "single bit" into the
 * bit 27 position, ANDing that with bit 27 in the PTE (the
 * user-accessible bit), shifting that right once more to line up with
 * the target bit 26 in the PTE, and ORing it in.  This will result in
 * bit 26 being set if the pseudo-sun3 protection was simply PG_WRITE.
 *
 * This could be expressed with if .. else.. logic, but the bit
 * shifts should compile into something that needs no branching.
 *
 * get_pte's job is easier.  All it has to do is clear the always-set
 * bits 30, 28, and 25, *and* clear bit 26, and flip PG_SYSTEM.  It can
 * clear bit 26 because the value that was there can always be derived
 * from the resulting pseudo-sun3 PG_SYSTEM and PG_WRITE combination.
 *
 * And that's how we reuse the sun3 pmap.
 */
#define PG_VALID   0x80000000
#define PG_WRITE   0x20000000
#define PG_NC      0x00000000
#define PG_SYSTEM  0x08000000
#if 0
#define PG_KW      0x70000000
#define PG_KR      0x50000000
#define PG_UW      0x7C000000
#define PG_URKR    0x58000000
#endif
#define PG_TYPE    0x00C00000
#define PG_REF     0x00200000
#define PG_MOD     0x00100000

#define PG_SPECIAL (PG_VALID|PG_WRITE|PG_SYSTEM|PG_NC|PG_REF|PG_MOD)
#define PG_PERM    (PG_VALID|PG_WRITE|PG_SYSTEM|PG_NC)
#define	PG_MODREF  (PG_REF|PG_MOD)
#define PG_FRAME   0x00000FFF

#define PG_MOD_SHIFT 20

#define OBMEM	0
#define OBIO 	1
#define MBMEM	2	/* on the 2/120, VME_D16 on the 2/50 */
#define VME_D16	2
#define MBIO	3	/* on the 2/120, ??? on the 2/50 */
#define PG_TYPE_SHIFT 22

#define PG_INVAL   0x0

#define MAKE_PGTYPE(x) ((x) << PG_TYPE_SHIFT)
#define PG_PFNUM(pte) (pte & PG_FRAME)
#define PG_PA(pte) (PG_PFNUM(pte) << PGSHIFT)

#define	PGT_MASK	MAKE_PGTYPE(3)
#define	PGT_OBMEM	MAKE_PGTYPE(OBMEM)		/* onboard memory */
#define	PGT_OBIO	MAKE_PGTYPE(OBIO)		/* onboard I/O */
#define	PGT_MBMEM	MAKE_PGTYPE(MBMEM)	/* Multibus memory on the 2/120, VME_D16 on the 2/50 */
#define	PGT_VME_D16	MAKE_PGTYPE(VME_D16)	/* VMEbus 16-bit data */
#define	PGT_MBIO	MAKE_PGTYPE(MBIO)	/* Multibus I/O on the 2/120, ??? on the 2/50 */

#define VA_SEGNUM(x)	((u_int)(x) >> SEGSHIFT)

#define VA_PTE_NUM_SHIFT  PGSHIFT
#define VA_PTE_NUM_MASK   (((1 << SEGSHIFT) - 1) ^ ((1 << PGSHIFT) - 1))
#define VA_PTE_NUM(va) ((va & VA_PTE_NUM_MASK) >> VA_PTE_NUM_SHIFT)

#define PA_PGNUM(pa) ((unsigned)pa >> PGSHIFT)

#if defined(_KERNEL) || defined(_STANDALONE)
u_int get_pte __P((vm_offset_t va));
void  set_pte __P((vm_offset_t va, u_int pte));
#endif	/* _KERNEL */

#endif	/* _MACHINE_PTE_H */
