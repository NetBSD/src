/*	$NetBSD: mips.c,v 1.1 1999/02/09 18:16:33 tv Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley. Modified by Ralph Campbell for mips.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * From: sparc.c 5.1 (Berkeley) 7/7/92
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)mips.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: mips.c,v 1.1 1999/02/09 18:16:33 tv Exp $");
#endif
#endif /* not lint */

#include "gprof.h"
#include "cg_arcs.h"   
#include "core.h"
#include "hist.h"
#include "symtab.h"

static Sym indirectchild;

void
findcall(parentp, p_lowpc, p_highpc)
	Sym	*parentp;
	bfd_vma	p_lowpc;
	bfd_vma	p_highpc;
{
	bfd_vma pc;
	Sym *childp;
	bfd_vma destpc;
	unsigned int op;
	int off;
	static bool inited = FALSE;

	if (!inited) {
		inited = TRUE;
		sym_init (&indirectchild);
		indirectchild.cg.prop.fract = 1.0;
		indirectchild.cg.cyc.head = &indirectchild;
	}

	if (core_text_space == 0)
		return;
	if (p_lowpc < s_lowpc)
		p_lowpc = s_lowpc;
	if (p_highpc > s_highpc)
		p_highpc = s_highpc;

	for (pc = p_lowpc; pc < p_highpc; pc += 4) {
		off = pc - s_lowpc;
		op = *(unsigned int *)&(((const char *)core_text_space)[off]);
		if ((op & 0xfc000000) == 0x0c000000) {
			/*
			 * a jal insn -- check that this
			 * is the address of a function.
			 */
			off = (op & 0x03ffffff) << 2;
			destpc = (pc & 0xf0000000) | off;
			if (destpc >= s_lowpc && destpc <= s_highpc) {
				childp = sym_lookup(&symtab, destpc);
				if (childp != 0 && childp->addr == destpc)
					addarc(parentp, childp, 0L);
			}
		} else if ((op & 0xfc00f83f) == 0x0000f809)
			/*
			 * A jalr -- an indirect call.
			 */
			addarc(parentp, &indirectchild, 0L);
	}
}
