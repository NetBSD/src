/*	$NetBSD: md.c,v 1.5 1998/12/17 20:14:44 pk Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <a.out.h>
#include <stab.h>
#include <string.h>

#include "ld.h"
#ifndef RTLD
/* Pull in the ld(1) bits as well */
#include "ld_i.h"
#endif

static int reloc_target_rightshift[] = {
	-1,  0,  2,  0,  0, 16, 16,  2,
	-1, -1,  2,  2, -1, -1,  0,  0,
	16, 16,  2, -1, -1, -1, -1, -1,
	-1, -1,  0,  0,  0,  0, 16, 16
};

static int reloc_target_bitpos[] = {
	-1,  0,  2,  0,  0,  0,  0,  2,
	-1, -1,  2,  2, -1, -1,  0,  0,
	 0,  0,  2, -1, -1, -1, -1, -1,
	-1, -1,  0,  0,  0,  0,  0,  0
};

static int reloc_target_size[] = {
	-1,  4,  4,  2,  2,  2,  2,  4,
	-1, -1,  4,  4, -1, -1,  2,  2,
	 2,  2,  4, -1, -1, -1, -1, -1,
	-1, -1,  4,  4,  4,  2,  2,  2
};

static int reloc_target_bitsize[] = {
	-1, 32, 24, 16, 16, 16, 16, 14,
	-1, -1, 24, 14, -1, -1, 16, 16,
	16, 16, 24, -1, -1, -1, -1, -1,
	-1, -1, 32, 32, 32, 16, 16, 16
};

/*
 * Get relocation addend corresponding to relocation record RP
 * from address ADDR
 */
long
md_get_addend(rp, addr)
	struct relocation_info	*rp;
	unsigned char		*addr;
{
	return rp->r_addend;
}

/*
 * Put RELOCATION at ADDR according to relocation record RP.
 */
void
md_relocate(rp, relocation, addr, relocatable_output)
	struct relocation_info	*rp;
	long			relocation;
	unsigned char		*addr;
	int			relocatable_output;
{
	register unsigned long mask;
	int ha_adj = 0;
	
	if (relocatable_output) {
		/*
		 * Store relocation where the next link-edit run
		 * will look for it.
		 */
		rp->r_addend = relocation;
		return;
	}
	if (rp->r_type == RELOC_16_HA
	    || rp->r_type == RELOC_GOT16_HA
	    || rp->r_type == RELOC_PLT16_HA)
		relocation += (relocation & 0x8000) << 1;
	
	relocation >>= RELOC_VALUE_RIGHTSHIFT(rp);

	/* Unshifted mask for relocation */
	mask = 1 << RELOC_TARGET_BITSIZE(rp) - 1;
	mask |= mask - 1;
	relocation &= mask;

	/* Shift everything up to where it's going to be used */
	relocation <<= RELOC_TARGET_BITPOS(rp);
	mask <<= RELOC_TARGET_BITPOS(rp);

	switch (RELOC_TARGET_SIZE(rp)) {
	case 4:
		*(u_long *)addr &= ~mask;
		*(u_long *)addr |= relocation;
		break;
	case 2:
		*(u_short *)addr &= ~mask;
		*(u_short *)addr |= relocation;
		break;
	default:
		errx(1, "Unknown relocation type %d", rp->r_type);
	}
}

/*
 * Set up a "direct" transfer (ie. not through the run-time binder) from
 * jmpslot at OFFSET to ADDR. Used by `ld' when the SYMBOLIC flag is on,
 * and by `ld.so' after resolving the symbol.
 */
void
md_fix_jmpslot(sp, offset, addr, first)
	jmpslot_t	*sp;
	long		offset;
	u_long		addr;
	int		first;
{
	errx(1, "md_fix_jmpslot unimplemented");
}

void
md_set_breakpoint(where, savep)
	long	where;
	long	*savep;
{
	*savep = *(long *)where;
	*(char *)where = TRAP;
}



#ifndef	RTLD
/*
 * Machine dependent part of claim_rrs_reloc().
 * Set RRS relocation type.
 */
int
md_make_reloc(rp, r, type)
	struct relocation_info	*rp, *r;
	int			type;
{
	r->r_type = rp->r_type;
	r->r_addend = rp->r_addend;

	if (RELOC_PCREL_P(rp))
		r->r_addend -= pc_relocation;

	return 1;
}

/*
 * Set up a transfer from jmpslot at OFFSET (relative to the PLT table)
 * to the binder slot (which is at offset 0 of the PLT).
 */
void
md_make_jmpslot(sp, offset, index)
	jmpslot_t	*sp;
	long		offset;
	long		index;
{
	errx(1, "md_make_jmpslot unimplemented");
}

/*
 * Update the relocation record for a RRS jmpslot.
 */
void
md_make_jmpreloc(rp, r, type)
	struct relocation_info	*rp, *r;
	int			type;
{
	errx(1, "md_make_jmpreloc unimplemented");
}

/*
 * Set relocation type for a RRS GOT relocation.
 */
void
md_make_gotreloc(rp, r, type)
	struct relocation_info	*rp, *r;
	int			type;
{
	errx(1, "md_make_gotreloc unimplemented");
}

/*
 * Set relocation type for a RRS copy operation.
 */
void
md_make_cpyreloc(rp, r)
	struct relocation_info	*rp, *r;
{
	errx(1, "md_make_cpyreloc unimplemented");
}

/*
 * Initialize (output) exec header such that useful values are
 * obtained from subsequent N_*() macro evaluations.
 */
void
md_init_header(hp, magic, flags)
	struct exec	*hp;
	int		magic, flags;
{
	N_SETMAGIC((*hp), magic, MID_MACHINE, flags);

	/* TEXT_START depends on the value of outheader.a_entry.  */
	if (!(link_mode & SHAREABLE))
		hp->a_entry = PAGSIZ;
}
#endif /* RTLD */


#ifdef NEED_SWAP
/*
 * Byte swap routines for cross-linking.
 */

void
md_swapin_exec_hdr(h)
	struct exec *h;
{
	swap_longs((long *)h + 1, sizeof(*h)/sizeof(long) - skip);
}

void
md_swapout_exec_hdr(h)
	struct exec *h;
{
	swap_longs((long *)h + 1, sizeof(*h)/sizeof(long) - skip);
}


void
md_swapin_reloc(r, n)
	struct relocation_info *r;
	int n;
{
	errx(1, "md_swapin_reloc unimplemented");
}

void
md_swapout_reloc(r, n)
	struct relocation_info *r;
	int n;
{
	errx(1, "md_swapout_reloc unimplemented");
}

void
md_swapout_jmpslot(j, n)
	jmpslot_t	*j;
	int		n;
{
	errx(1, "md_swapout_jmpslot unimplemented");
}
#endif /* NEED_SWAP */
