/*	$NetBSD: md.c,v 1.10 1998/12/17 20:14:44 pk Exp $	*/

/*
 * Copyright (C) 1997 Mark Brinicombe
 * Copyright (C) 1997 Causality Limited
 * Copyright (C) 1996 Wolfgang Solfrank
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
 *	This product includes software developed by Wolfgang Solfrank.
 *	This product includes software developed by Causality Limited.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Second cut for arm32 (used to be a simple copy of i386 code) */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <a.out.h>
#include <stab.h>
#include <string.h>
#ifdef RTLD
#include <machine/sysarch.h>
#include <sys/syscall.h>
#endif	/* RTLD */

#include "ld.h"
#ifndef RTLD
/* Pull in the ld(1) bits as well */
#include "ld_i.h"
#endif

#ifdef RTLD
/*
 * Flush the instruction cache of the specified address
 * Some processors have separate instruction caches and
 * as such may need a flush following a jump slot fixup.
 */
__inline void
iflush(addr, len)
	void *addr;
	int len;
{
	struct arm32_sync_icache_args p;

	/*
	 * This is not an efficient way to flush a chunk of memory
	 * if we need to flush lots of small chunks i.e. a jmpslot
	 * per function call.
	 */
	p.addr = (u_int)addr;
	p.len = len;

	__asm __volatile("mov r0, %0; mov r1, %1; swi %2"
	 : : "I" (ARM32_SYNC_ICACHE), "r" (&p), "J" (SYS_sysarch));
}
#endif	/* RTLD */


/*
 * Get relocation addend corresponding to relocation record RP
 * from address ADDR
 */
long
md_get_addend(rp, addr)
	struct relocation_info	*rp;
	unsigned char		*addr;
{
	long rel;
	
	switch (rp->r_length) {
	case 0:
		rel = get_byte(addr);
		break;
	case 1:
		rel = get_short(addr);
		break;
	case 2:
		rel = get_long(addr);
		break;
	case 3:			/* looks like a special hack for b & bl */
		rel = (((long)get_long(addr) & 0xffffff) << 8) >> 6;
		/*
		 * XXX
		 * Address the addend to be relative to the start of the file
		 * The implecation of doing this is that this adjustment is
		 * done every time the reloc goes through ld.
		 * This means that the adjustment can be applied multiple
		 * times if ld -r is used to do a partial link.
		 *
		 * Solution:
		 * 1. Put a hack in md_relocate so that PC relative
		 *    relocations are not modified if
		 *    relocatable_output == 1
		 * 2. Modify the assembler to apply this adjustment.
		 */
		rel -= rp->r_address;
		break;
	default:
		errx(1, "Unsupported relocation size: %x",
		    rp->r_length);
	}
	return rp->r_neg ? -rel : rel; /* Hack to make r_neg work */
}

/*
 * Put RELOCATION at ADDR according to relocation record RP.
 */
void
md_relocate(rp, relocation, addr, relocatable_output)
struct relocation_info	*rp;
	long		relocation;
	unsigned char	*addr;
	int		relocatable_output;
{
	/*
	 * XXX
	 * See comments above in md_get_addend
	 */
#ifndef RTLD
	if (RELOC_PCREL_P(rp) && relocatable_output)
		relocation += (rp->r_address + pc_relocation);
	if (rp->r_pcrel && rp->r_length == 2 && relocatable_output)
		relocation -= rp->r_address;
#endif

	if (rp->r_neg)		/* Not sure, whether this works in all cases XXX */
		relocation = -relocation;
	
	switch (rp->r_length) {
	case 0:
		put_byte(addr, relocation);
		break;
	case 1:
		put_short(addr, relocation);
		break;
	case 2:
		put_long(addr, relocation);
		break;
	case 3:
		put_long(addr,
			 (get_long(addr)&0xff000000)
			 | ((relocation>>2)&0xffffff));
		break;
	default:
		errx(1, "Unsupported relocation size: %x",
		    rp->r_length);
	}
}

/*
 * Set up a "direct" transfer (ie. not through the run-time binder) from
 * jmpslot at OFFSET to ADDR. Used by `ld' when the SYMBOLIC flag is on,
 * and by `ld.so' after resolving the symbol.
 */

#ifdef RTLD
extern       void		binder_entry __P((void));
#endif

void
md_fix_jmpslot(sp, offset, addr, first)
	jmpslot_t	*sp;
	long		offset;
	u_long		addr;
	int		first;
{
	/*
	 * For jmpslot 0 (the binder)
	 * generate
	 *	sub	ip, pc, ip
	 *	ldr	pc, [pc, #-4]
	 *	.word	addr
	 *	<unused>
	 *
	 * For jump slots generated by the linker (i.e. -Bsymbolic)
	 * build a direct jump to the absolute address
	 * i.e.
	 *	ldr	pc, [pc]
	 *	<unused>
	 *	.word	new_addr
	 *	<unused>
	 *
	 * For jump slots generated by ld.so at run time,
	 * just modify the address offset since the slot
	 * will have been created with md_make_jmpslot().
	 * i.e.
	 *	ldr	ip, [pc]
	 *	add	pc, pc, ip
	 *	.word	new_rel_addr
	 *	<unused>
	 */
	if (first) {
		/* Build binder jump slot */
		sp->opcode1 = GETSLOTADDR;
		sp->opcode2 = LDRPCADDR;
		sp->address = addr;
#ifdef RTLD
		iflush(sp, sizeof(jmpslot_t));
#endif	/* RTLD */
	} else {
#ifdef RTLD
		/*
		 * Change the relative offset to the binder
		 * into a relative offset to the function
		 */
		sp->address = (addr - (long)sp - 12);
#else
		/*
		 * Build a direct transfer jump slot
		 * as we not doing a run time fixup.
		 */
		sp->opcode1 = JUMP;
		sp->address = addr;
#endif	/* RTLD */
	}
}

void
md_set_breakpoint(where, savep)
	long	where;
	long	*savep;
{
	*savep = *(long *)where;
	*(long *)where = TRAP;
#ifdef RTLD
	iflush((long *)where, sizeof(long));
#endif	/* RTLD */	
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
	/* Copy most attributes */
	r->r_pcrel = rp->r_pcrel;
	r->r_length = rp->r_length;
	r->r_neg = rp->r_neg;
	r->r_baserel = rp->r_baserel;
	r->r_jmptable = rp->r_jmptable;
	r->r_relative = rp->r_relative;

	return 0;
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
	/*
	 * Build the jump slot as follows
	 *
	 *	ldr	ip, [pc]
	 *	add	pc, pc, ip
	 *	.word	rel_addr
	 *	.word	reloc_index
	 */
	sp->opcode1 = GETRELADDR;
	sp->opcode2 = ADDPC;
	sp->address = - (offset + 12);
	sp->reloc_index = index;
}


/*
 * Update the relocation record for a RRS jmpslot.
 */
void
md_make_jmpreloc(rp, r, type)
	struct relocation_info	*rp, *r;
	int			type;
{
	r->r_address += 8;
	r->r_pcrel = 0;
	r->r_length = 2;
	r->r_neg = 0;
	r->r_baserel = 0;
	r->r_jmptable = 1;
	r->r_relative = 0;
}

/*
 * Set relocation type for a RRS GOT relocation.
 */
void
md_make_gotreloc(rp, r, type)
	struct relocation_info	*rp, *r;
	int			type;
{
	r->r_pcrel = 0;
	r->r_length = 2;
	r->r_neg = 0;
	r->r_baserel = 1;
	r->r_jmptable = 0;
	r->r_relative = 0;

}

/*
 * Set relocation type for a RRS copy operation.
 */
void
md_make_cpyreloc(rp, r)
	struct relocation_info	*rp, *r;
{
	r->r_pcrel = 0;
	r->r_length = 2;
	r->r_neg = 0;
	r->r_baserel = 0;
	r->r_jmptable = 0;
	r->r_relative = 0;
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
	N_SETMAGIC((*hp), magic, MID_ARM6, flags);

	/* TEXT_START depends on the value of outheader.a_entry.  */
	if (!(link_mode & SHAREABLE))
		hp->a_entry = PAGSIZ;
}
#endif	/* RTLD */


#ifdef NEED_SWAP
/*
 * Byte swap routines for cross-linking.
 */

void
md_swapin_exec_hdr(h)
	struct exec *h;
{
	/* NetBSD: Always leave magic alone */
	int skip = 1;

	swap_longs((long *)h + skip, sizeof(*h)/sizeof(long) - skip);
}

void
md_swapout_exec_hdr(h)
	struct exec *h;
{
	/* NetBSD: Always leave magic alone */
	int skip = 1;

	swap_longs((long *)h + skip, sizeof(*h)/sizeof(long) - skip);
}

void
md_swapout_jmpslot(j, n)
	jmpslot_t	*j;
	int		n;
{
	for (; n; n--, j++) {
		j->opcode1 = md_swap_long(j->opcode1);
		j->opcode2 = md_swap_long(j->opcode2);
		j->address = md_swap_long(j->address);
		j->reloc_index = md_swap_long(j->reloc_index);
	}
}

#endif /* NEED_SWAP */

/*
 * md_swapin_reloc()
 *
 * As well as provide bit swapping for cross compiling with different
 * endianness we need to munge some of the reloc bits.
 * This munging is due to the assemble packing all the PIC related
 * relocs so that only 1 extra bit in the reloc structure is needed
 * The result is that jmpslot branches are packed as a baserel branch
 * Spot this case and internally use a jmptable bit.
 */

void
md_swapin_reloc(r, n)
	struct relocation_info *r;
	int n;
{
	int	bits;

	for (; n; n--, r++) {
#ifdef NEED_SWAP
		r->r_address = md_swap_long(r->r_address);
		bits = ((int *)r)[1];
		r->r_symbolnum = md_swap_long(bits) & 0x00ffffff;
		r->r_pcrel = (bits & 1);
		r->r_length = (bits >> 1) & 3;
		r->r_extern = (bits >> 3) & 1;
		r->r_neg = (bits >> 4) & 1;
		r->r_baserel = (bits >> 5) & 1;
		r->r_jmptable = (bits >> 6) & 1;
		r->r_relative = (bits >> 7) & 1;
#endif
		/* Look for PIC relocation */
		if (r->r_baserel) {
			/* Look for baserel branch */
			if (r->r_length == 3 && r->r_pcrel == 0) {
				r->r_jmptable = 1;
			}
			/* Look for GOTPC reloc */
			if (r->r_length == 2 && r->r_pcrel == 1)
				r->r_baserel = 0;
		}
	}
}

void
md_swapout_reloc(r, n)
	struct relocation_info *r;
	int n;
{
	int	bits;

	for (; n; n--, r++) {
		/* Look for jmptable relocation */
		if (r->r_jmptable && r->r_pcrel == 0 && r->r_length == 3) {
			r->r_jmptable = 0;
			r->r_baserel = 1;
		}
#ifdef NEED_SWAP
		r->r_address = md_swap_long(r->r_address);
		bits = md_swap_long(r->r_symbolnum) & 0xffffff00;
		bits |= (r->r_pcrel & 1);
		bits |= (r->r_length & 3) << 1;
		bits |= (r->r_extern & 1) << 3;
		bits |= (r->r_neg & 1) << 4;
		bits |= (r->r_baserel & 1) << 5;
		bits |= (r->r_jmptable & 1) << 6;
		bits |= (r->r_relative & 1) << 7;
		((int *)r)[1] = bits;
#endif
	}
}
