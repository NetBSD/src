/*	$NetBSD: SYS.h,v 1.2 2003/03/24 14:29:34 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <sys/syscall.h>

/*
 * Note: NetBSD/sh5 uses a TRAPA code of 0x80 to indicate a system
 * call. (This is the same code used by the SH3/SH4 ports.)
 */

#ifdef __STDC__
#define	SYSTRAP(x)	movi SYS_ ## x ##, r0; movi 128, r1; trapa r1
#else
#define	SYSTRAP(x)	movi SYS_/**/x, r0; movi 128, r1; trapa r1
#endif

#define	_SYSCALL_NOERROR(x,y)						\
	ENTRY(x) ;							\
	SYSTRAP(y)

#ifdef PIC
#define	_SYSCALL_CERROR	99b
#define	_SYSCALL(x,y)							\
	_TEXT_SECTION ;							\
	_ALIGN_TEXT ;							\
	99: PIC_PROLOGUE ;						\
	    PIC_PTAL(_C_LABEL(__cerror), r0, tr0) ;			\
	    blink tr0, r63 ;						\
	_SYSCALL_NOERROR(x,y)
#else
#define	_SYSCALL_CERROR	_C_LABEL(__cerror)
#define	_SYSCALL(x,y)							\
	_SYSCALL_NOERROR(x,y)
#endif

#define	PSEUDO_NOERROR(x,y)						\
	_SYSCALL_NOERROR(x,y) ;						\
	ptabs/l r18, tr0 ;						\
	blink tr0, r63

#define	PSEUDO(x,y)							\
	_SYSCALL(x,y) ;							\
	ptabs/l r18, tr0 ;						\
	beq/l r0, r63, tr0 ;						\
	pta/l _SYSCALL_CERROR, tr0 ;					\
	blink tr0, r63

#define RSYSCALL_NOERROR(x)						\
	PSEUDO_NOERROR(x,x)

#define	RSYSCALL(x)							\
	PSEUDO(x,x)

#define	WSYSCALL(weak,strong)						\
	WEAK_ALIAS(weak,strong)	;					\
	PSEUDO(strong,weak)

