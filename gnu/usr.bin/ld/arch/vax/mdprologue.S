/*	$NetBSD: mdprologue.S,v 1.7 1998/10/19 03:09:33 matt Exp $	*/

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

#include <sys/syscall.h>

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
	movl	8(ap),r0		/* load crtp into r0 */
					/* setup arguments for rtld() */
	/*
	 * Add the 1st entry in the GOT (e.g. __DYNAMIC) to the base
	 * address of ld.so and pushd that onto the stack.
	 */
	addl3	__GLOBAL_OFFSET_TABLE_,(r0),-(sp)
	pushl	r0			/* 2nd arg == crtp */
	pushl	4(ap)			/* 1st arg == version */
	calls	$3,_rtld		/* _rtld(version, crtp, DYNAMIC) */
	movpsl	-(sp)			/* flush the instruction cache */
	pushab	1f			/*   by issuing an */
	rei				/*   rei. */
1:	ret

/*
 * First call to a procedure generally comes through here for binding.
 * We got here via JSB so now (sp) is pointing to the reloc_index
 * inside our jmpslot_t.  So we simply preserve our registers,
 * pushd the address again for _binder.  Save R0 on the stack that
 * the JSB used.  Call the actual function and return.
 */
	.align	1
	.type	_binder_entry,@label
_binder_entry:
	pushr	$0x3f		/* save r0 to r5 */
#ifdef DEBUG
	pushl	$29
	pushab	LC1
	pushl	$2
	calls	$3,_write
#endif
	subl3	$9, 24(sp), -(sp) /* point to beginning of jmpslot */
	calls	$1, _binder	/* _binder(jsp) */
	movl	r0, 24(sp)
	popr	$0x3f		/* restore r0 to r5 */
	callg	(ap), *(sp)+	/* return value from _binder() == actual */
	ret
#ifdef DEBUG
LC1:	.asciz	"ld.so: entered _binder_entry\n"
#endif
