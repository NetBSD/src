/*	$NetBSD: md-static-funcs.c,v 1.1 1997/10/17 21:25:42 mark Exp $	*/

/*
 * Copyright (C) 1997 Mark Brinicombe
 * Copyright (C) 1997 Causality Limited
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

/*
 * Called by ld.so when onanating.
 * This *must* be a static function, so it is not called through a jmpslot.
 */

#define RELOC_32	0x04
#define RELOC_JMPSLOT	0x06	/* See note below */
#define RELOC_GOT32	0x24

/*
 * The value is RELOC_JMPSLOT is special. Really it should be 0x46 i.e.
 * have the r_jmptable bit set however due to the usage of the
 * relocation bits outside the linker the r_jmptable is only used
 * internal to the linker and thus this bit get lost when the
 * relocations are written out.
 */

static void
md_relocate_simple(r, relocation, addr)
	struct relocation_info	*r;
	long			relocation;
	char			*addr;
{
	int index;

	index = r->r_pcrel | (r->r_length << 1) | (r->r_extern << 3)
	    | (r->r_neg << 4) | (r->r_baserel << 5);

	if (index == RELOC_JMPSLOT)
		*(long *)addr += relocation;
	else if (index == RELOC_GOT32)
		*(long *)addr += relocation;
	else if (index == RELOC_32)
		*(long *)addr += relocation;

/*	if (r->r_relative)
		*(long *)addr += relocation;*/
}
