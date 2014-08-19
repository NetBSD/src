/*	$NetBSD: compat_13_sigreturn13.s,v 1.5.14.1 2014/08/20 00:03:11 tls Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * NOTICE: This file is included by <m68k/m68k/sigreturn.s>.
 */

/*
 * The compat_13_sigreturn13() syscall comes here.  It requires special
 * handling because we must open a hole in the stack to fill in the
 * (possibly much larger) original stack frame.
 */
ENTRY_NOPROFILE(m68k_compat_13_sigreturn13_stub)
	lea	-84(%sp),%sp		| leave enough space for largest frame
	movl	84(%sp),(%sp)		| move up current 8 byte frame
	movl	88(%sp),4(%sp)
	movl	#84,-(%sp)		| default: adjust by 84 bytes
	moveml	#0xFFFF,-(%sp)		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,FR_SP(%sp)		|   in the savearea
	movl	#SYS_compat_13_sigreturn13,-(%sp)	| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall#
	movl	FR_SP(%sp),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	lea	FR_HW(%sp),%a1		| pointer to HW frame
	movw	FR_ADJ(%sp),%d0	| do we need to adjust the stack?
	jeq	.Lc13sigr1		| no, just continue
	moveq	#92,%d1			| total size
	subw	%d0,%d1			|  - hole size = frame size
	lea	92(%a1),%a0		| destination
	addw	%d1,%a1			| source
	lsrw	#1,%d1			| convert to word count
	subqw	#1,%d1			| minus 1 for dbf
.Lc13sigrlp:
	movw	-(%a1),-(%a0)		| copy a word
	dbf	%d1,.Lc13sigrlp		| continue
	movl	%a0,%a1			| new HW frame base
.Lc13sigr1:
	movl	%a1,FR_SP(%sp)		| new SP value
	moveml	(%sp)+,#0x7FFF		| restore user registers
	movl	(%sp),%sp		| and our SP
	jra	_ASM_LABEL(rei)		| all done
