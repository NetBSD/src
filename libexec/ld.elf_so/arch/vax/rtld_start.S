/*	$NetBSD: rtld_start.S,v 1.19 2014/03/19 15:34:30 matt Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Portions copyright 2002, 2003 Charles M. Hannum <root@ihack.net>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <machine/asm.h>

/* R9 contains the address of PS_STRINGS and since its caller saved,
 * we can just use it.  R6 has a backup copy of the stack pointer which
 * we can use as well.
 */
ENTRY(_rtld_start, 0)
	/* Allocate space on the stack for the cleanup and obj_main
	 * entries that _rtld() will provide for us.
	 */
	clrl	%fp
	subl2	$8,%sp

	movab	_DYNAMIC,%r0
	subl3	_GLOBAL_OFFSET_TABLE_,%r0,%r10
	pushl	%r10		/* relocbase */
	pushl	%r0		/* &_DYNAMIC */
	calls	$2,_rtld_relocate_nonplt_self

	pushl	%r10		/* relocbase */
	pushal	4(%sp)		/* sp */
	calls	$2,_rtld	/* entry = _rtld(sp, relocbase) */

	movq	(%sp)+,%r7	/* grab cleanup and obj_main into %r7/%r8 */
	jmp	2(%r0)		/* jump to entry point + 2 */
END(_rtld_start)

/*
 * Lazy binding entry point, called via PLT via JMP into pltgot[1].
 * SP+0: obj entry points
 * SP+4: address to relocation index
 *
 * Note: Some functions rely on there not being an additional call frame;
 * hence the `optimization' to avoid the callg opportunistically.
 */
ALTENTRY(_rtld_bind_start)
	movab	-64(%sp),%sp	/* reserve some space */
	pushr	$0x3f		/* save R0-R5 */
	movq	-8(%fp),%r0	/* get addresses of plt.got & reloc index */
	pushl	(%r1)		/* push relocation index */
	pushl	%r0		/* push address of obj entry */
	calls	$2,_rtld_bind

	addl3	$2,%r0,%r3		/* save routine address */
	extzv	$0,$12,(%r0),%r1	/* get entry mask */
	extzv	$0,$12,6(%fp),%r2	/* get saved mask */
	cmpw	%r1,%r2		/* compare them */
	bneq	12f		/* if they are different, rebuild */
	movl	%r0,-4(%fp)	/* save routine address */
	popr	$0x3f		/* pop registers */
	movab	68(%sp),%sp	/* restore sp */
	rsb			/* and jump to it */

	/*
	 * We need to rebuild the callframe.  Save the current one in case
	 * we might overwrite it.
	 */
12:	movq	4(%fp),-(%sp)	/* save PSW and AP */
	movq	12(%fp),-(%sp)	/* save FP and return address */
	/*
	 * Find out where this this call frame ends.
	 */
	movl	%ap,%r0		/* get past callframe and registers */
	bbs	$29,4(%fp),22f	/* calls is easy, it's where AP is */
	/*
	 * Callg not so much
	 */
	movab	20(%fp),%r0	/* past fixed callframe */
	tstw	%r2		/* no saved registers? */
	beql	22f		/*    none, so we are done. */
	movl	$11,%r4		/* start with register 11 */
20:	bbc	%r4,%r2,21f	/* save this register? */
	addl2	$4,%r0		/*   yes, adjust for saved register */
21:	sobgeq	%r4,20b		/* try next register */

22:
	/*
	 * First "push" the caller saved registers (if there any that
	 * need to saved.)
	 */
	tstw	%r1		/* if there are no registers to save */
	beql	1f		/* just push the callframe */
	cmpw	%r1,$63		/* if there are no caller-saved registers */
	blequ	5f		/* skip them */
	bbc	$11,%r1,10f	/* does it need to be saved? */
	movl	%r11,-(%r0)
10:	bbc	$10,%r1,9f	/* does it need to be saved? */
	movl	%r10,-(%r0)
9:	bbc	$9,%r1,8f	/* does it need to be saved? */
	movl	%r9,-(%r0)
8:	bbc	$8,%r1,7f	/* does it need to be saved? */
	movl	%r8,-(%r0)
7:	bbc	$7,%r1,6f	/* does it need to be saved? */
	movl	%r7,-(%r0)
6:	bbc	$6,%r1,5f	/* does it need to be saved? */
	movl	%r6,-(%r0)
5:	
	/*
	 * r0-r5 are not normally preserved so we should be done.
	 */
	cmpw	%r1,$63
	bgtru	1f
	/*
	 * For some reason, we have to preserve these.
	 */
	movab	16(%sp),%r2
	bbc	$5,%r1,4f	/* does it need to be saved? */
	movl	20(%r2),-(%r0)
4:	bbc	$4,%r1,3f	/* does it need to be saved? */
	movl	16(%r2),-(%r0)
3:	bbc	$3,%r1,2f	/* does it need to be saved? */
	movl	12(%r2),-(%r0)
2:	bbc	$2,%r1,1f	/* does it need to be saved? */
	movl	8(%r2),-(%r0)

	/*
	 * Now we save the fixed part of the callframe.
	 */
1:	clrl	%r4		/* clear condition handler slot */
	movq	(%sp)+,-(%r0)	/* move FP and PC into place */
	movq	(%sp)+,-(%r0)	/* move PSW/save-mask/etc + AP into place */
	movq	%r3,-(%r0)	/* move routine address + cond handle slot */
	addl3	$4,%r0,%fp	/* get start of new callframe */
	insv	%r1,$0,$12,6(%fp) /* insert new saved mask */
	popr	$0x3f		/* restore R0-R5 (cond flags not modified) */
	subl3	$4,%fp,%sp	/* sp needs to be equal to fp */
	rsb			/* and jmp to the routine */
END(_rtld_bind_start)
