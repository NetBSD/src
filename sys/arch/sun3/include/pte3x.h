/*	$NetBSD: pte3x.h,v 1.4 1997/03/13 17:40:34 gwr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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
 * This file should contain the machine-dependent details about
 * Page Table Entries (PTEs) and related things.  For example,
 * things that depend on the MMU configuration (number of levels
 * in the translation structure) should go here.
 */

#ifndef _MACHINE_PTE_H
#define _MACHINE_PTE_H

#include <machine/mc68851.h>

#define OBMEM	0
#define OBIO 	1
#define VME_D16	2
#define VME_D32	3
#define PG_TYPE_SHIFT 26

#define MAKE_PGTYPE(x) ((x) << PG_TYPE_SHIFT)
#define PG_PGNUM(pte) (pte & PG_FRAME)
#define PG_PA(pte) ((pte & PG_FRAME) <<PGSHIFT)

#define	PGT_MASK	MAKE_PGTYPE(3)
#define	PGT_OBMEM	MAKE_PGTYPE(OBMEM)		/* onboard memory */
#define	PGT_OBIO	MAKE_PGTYPE(OBIO)		/* onboard I/O */
#define	PGT_VME_D16	MAKE_PGTYPE(VME_D16)	/* VMEbus 16-bit data */
#define	PGT_VME_D32	MAKE_PGTYPE(VME_D32)	/* VMEbus 32-bit data */

#define VA_SEGNUM(x)	((u_int)(x) >> SEGSHIFT)

#define VA_PTE_NUM_SHIFT  13
#define VA_PTE_NUM_MASK (0xF << VA_PTE_NUM_SHIFT)
#define VA_PTE_NUM(va) ((va & VA_PTE_NUM_MASK) >> VA_PTE_NUM_SHIFT)

#define PA_PGNUM(pa) ((unsigned)pa >> PGSHIFT)

/*
 * Mach derived conversion macros
 */
#define _round_seg(x)	((((unsigned)(x)) + SEGOFSET) & ~SEGOFSET)
#define _trunc_seg(x)	((unsigned)(x) & ~SEGOFSET)
#define _btos(x)	((unsigned)(x) >> SEGSHIFT)
#define _stob(x)	((unsigned)(x) << SEGSHIFT)

#define _round_page(x)	((((unsigned)(x)) + PGOFSET) & ~PGOFSET)
#define _trunc_page(x)	((unsigned)(x) & ~PGOFSET)
#define _btop(x)	((unsigned)(x) >> PGSHIFT)
#define _ptob(x)	((unsigned)(x) << PGSHIFT)

#ifdef	_KERNEL

/* defined in pmap.c */
vm_offset_t get_pte __P((vm_offset_t va));
void set_pte __P((vm_offset_t, vm_offset_t));

#endif	/* _KERNEL */
#endif	/* !_MACHINE_PTE_H*/
