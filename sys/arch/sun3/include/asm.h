/*-
 * Copyright (c) 1993 Adam Glass 
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This version based on the old asm.h which is ifdef notdefed at the end,
 * and jolitz's i386 asm.h mess.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	from: @(#)asm.h	5.5 (Berkeley) 5/7/91
 *	asm.h,v 1.1 1993/06/16 21:42:47 mycroft Exp
 */

#ifndef _MACHINE_ASM_H_
#define _MACHINE_ASM_H_

/*
 * XXX assumes that arguments are not passed in %eax
 */


#ifdef STDC
# define _C_FUNC(x)	_ ## x
#else
# define _C_FUNC(x)	_/**/x
#endif
#define	_ASM_FUNC(x)	x

#define ALIGN_TEXT .align 2
#ifdef GPROF
# define _BEGIN_ENTRY	
# define _END_ENTRY	link a6,#0; jbsr mcount; unlk a6 ; 
# define _ENTER_FUNC(x) jra _C_FUNC(x)+12
#else
# define _BEGIN_ENTRY	
# define _END_ENTRY
# define _ENTER_FUNC(x) ;
#endif

#define _ENTRY(x)	.globl x; ALIGN_TEXT; x:

#define	ENTRY(y)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(y)); _END_ENTRY
#define	TWOENTRY(y,z)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(z)); _END_ENTRY \
   _ENTER_FUNC(y); \
   _ENTRY(_C_FUNC(y)); _END_ENTRY
#define	ASENTRY(y)	_BEGIN_ENTRY; _ENTRY(_ASM_FUNC(y)); _END_ENTRY

#define	ASMSTR		.asciz

#endif /* !_MACHINE_ASM_H_ */
