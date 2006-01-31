/* $NetBSD: alloc.s,v 1.9 2006/01/31 14:58:28 is Exp $ */

/*-
 * Copyright (c) 1996,2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Memory allocation through exec library.
 */

#include <machine/asm.h>

ENTRY_NOPROFILE(alloc)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%d0
	movl	#0x50001,%d1	| MEMF_CLEAR|MEMF_REVERSE|MEMF_PUBLIC for now.
	jsr	%a6@(-0xc6)	| AllocMem
	movl	%sp@+,%a6
	movl	%d0,%a0		| Comply with ELF ABI
	rts

ENTRY_NOPROFILE(dealloc)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	movl	%sp@(12),%d0
	jsr	%a6@(-0xd2)	| FreeMem
	movl	%sp@+,%a6
	rts

