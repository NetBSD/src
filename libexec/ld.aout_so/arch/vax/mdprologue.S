/*	$NetBSD: mdprologue.S,v 1.11 2002/02/24 01:06:21 matt Exp $	*/

/*
 * Copyright (c) 1998 Matt Thomas <matt@3am-software.com>
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
 *    derived from this software without specific prior written permission
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
 * i386 run-time link editor entry points.
 */

	.text
	.globl	_binder, _binder_entry
	.globl	_rtl
	.type	_rtl,@function

/*
 *	_rtl(int version, struct crt_ldso *crtp)
 */

	.align	2
_rtl:					/* crt0 calls us here */
	.word	0			/* no registers to save */
	movl	8(%ap),%r0		/* load crtp into %r0 */
					/* setup arguments for rtld() */
	/*
	 * Add the 1st entry in the GOT (e.g. __DYNAMIC) to the base
	 * address of ld.so and pushd that onto the stack.
	 */
	addl3	__GLOBAL_OFFSET_TABLE_,(%r0),-(%sp)
	pushl	%r0			/* 2nd arg == crtp */
	pushl	4(%ap)			/* 1st arg == version */
	calls	$3,_rtld		/* _rtld(version, crtp, DYNAMIC) */
	movpsl	-(%sp)			/* flush the instruction cache */
	pushab	1f			/*   by issuing an */
	rei				/*   rei. */
1:	ret

/*
 * First call to a procedure generally comes through here for binding.
 * We got here via JSB so now (%sp) is inside our jmpslot_t.  So we
 * simply preserve our registers, push the address of jmpslot_t for
 * _binder.  Save the address we are supported to call (which was 
 * returned in R0) in the stack location that the JSB used to store
 * its return address (which we don't care about anymore).
 *
 * Check to see the function has a zero entry mask.  Since we
 * were called with a zero entry, if the function we need to call
 * also has a zero entry mask, we can let it use our call frame
 * and jump inside of it to its first instruction.  Otherwise we'll
 * call the actual function the slow way and return.
 *
 * The above optimization is required in order for vfork(2) to
 * work properly in shared libraries.  DO NOT REMOVE IT.
 */
	.align	1
	.type	_binder_entry,@label
_binder_entry:
	pushr	$0x3f		/* save %r0 to %r5 */
#ifdef DEBUG
	pushl	$29
	pushab	LC1
	pushl	$2
	calls	$3,_write
#endif
	subl3	$8, 24(%sp), -(%sp) /* point to beginning of jmpslot */
	bicl2	$3, (%sp)
	calls	$1, _binder	/* _binder(jsp) */
	movpsl	-(%sp)		/* flush the instruction cache */
	pushab	1f		/*   by issuing an */
	rei			/*   rei. */
1:	movl	%r0, 24(%sp)	/* save return address onto stack */
	bicw3	6(%fp),(%r0),%r0 /* does the entry mask save any additional regs */
	popr	$0x3f		/* restore %r0 to %r5 (cond flags aren't modified) */
	bneq	2f		/* yes? do it the hard way */
	addl2	$2,(%sp)	/* no? skip past the mask */
	rsb			/*    and jump to it */
2:	callg	(%ap), *(%sp)+	/* return value from _binder() == actual */
	ret
#ifdef DEBUG
LC1:	.asciz	"ld.so: entered _binder_entry\n"
#endif
