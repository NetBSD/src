/*	$NetBSD: mdprologue.S,v 1.1 1997/10/17 21:25:59 mark Exp $	*/

/*
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
 * arm run-time link editor entry points.
 */

#include <machine/asm.h>
#include <sys/syscall.h>

	.text

/*
 *	_rtl(int version, struct crt_ldso *crtp)
 */

	.align	0
	.type	_rtl,#function
_rtl:					@ crt0 calls us here
	mov	ip, sp			@ Allocate stack frame
	stmfd	sp!, {sl, fp, ip, lr, pc}
	sub	fp, ip, #4

	ldr	r3, [r1, #0]		@ base address of ld.so (first field)
					@ setup arguments for rtld()
	ldr	sl, GOTaddr		@ get pc-rel offset for GOT
	add	sl, pc, sl		@ here is the GOT
L1:
	ldr	r2, DYNAMICaddr
	ldr	r2, [sl, r2]		@ 1st entry in GOT is our __DYNAMIC
	add	r2, r2, r3		@   add load address
	ldr	lr, rtldaddr		@ get GOT offset for function
	ldr	lr, [sl, lr]		@ get function address
	add	r3, lr, r3		@   and relocate
	mov	lr, pc
	mov	pc, r3			@ _rtld(version, crtp, DYNAMIC)

	ldmea	fp, {sl, fp, ip, pc}

	.align	0
GOTaddr:
	.word	__GLOBAL_OFFSET_TABLE_ + (. - (L1+4) )
DYNAMICaddr:
	.word	__DYNAMIC
rtldaddr:
	.word	_rtld

 @ First call to a procedure generally comes through here for
 @ binding.

ENTRY(binder_entry)
	stmfd	sp!, {r0-fp, ip, lr, pc} @ save all regs in standard frame
					@   (ip and lr to be fixed later)
	add	fp, sp, #(15-1)*4	@ get new fp
	add	r0, fp, #4		@ compute old sp
	str	r0, [fp, #-8]		@ fixup saved ip

	sub	r0, ip, #(8+12)		@ make it point to the jmpslot

	bl	_binder
	str	r0, [fp]		@ return value from _binder()
					@   == actual address of function
	ldmfa	fp, {r0-fp, sp, lr, pc}	@ restore regs
