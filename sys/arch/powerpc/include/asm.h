/*	$NetBSD: asm.h,v 1.3 1997/04/16 22:52:50 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PPC_ASM_H_
#define _PPC_ASM_H_

#ifdef PIC
#define PIC_PROLOGUE	XXX
#define PIC_EPILOGUE	XXX
#ifdef	__STDC__
#define PIC_PLT(x)	XXX
#define PIC_GOT(x)	XXX
#define PIC_GOTOFF(x)	XXX
#else	/* not __STDC__ */
#define PIC_PLT(x)	XXX
#define PIC_GOT(x)	XXX
#define PIC_GOTOFF(x)	XXX
#endif	/* __STDC__ */
#else
#define PIC_PROLOGUE
#define PIC_EPILOGUE
#define PIC_PLT(x)	x
#define PIC_GOT(x)	x
#define PIC_GOTOFF(x)	x
#endif

#define	_C_LABEL(x)	x
#define	_ASM_LABEL(x)	x

#define	_GLOBAL(x) \
	.data; .align 2; .globl x; x:

#define _ENTRY(x) \
	.text; .align 2; .globl x; .type x,@function; x:

#ifdef GPROF
# define _PROF_PROLOGUE	XXX
#else
# define _PROF_PROLOGUE
#endif

#define	ENTRY(y)	_ENTRY(_C_LABEL(y)); _PROF_PROLOGUE
#define	ASENTRY(y)	_ENTRY(_ASM_LABEL(y)); _PROF_PROLOGUE
#define	GLOBAL(y)	_GLOBAL(_C_LABEL(y))

#define	ASMSTR		.asciz

#define RCSID(x)	.text; .asciz x

#endif /* !_PPC_ASM_H_ */
