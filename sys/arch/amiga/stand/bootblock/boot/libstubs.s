/* $NetBSD: libstubs.s,v 1.3.8.1 2001/03/12 13:27:12 bouyer Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 * Exec.library functions.
 */
#include <machine/asm.h>
	.comm _C_LABEL(SysBase),4

ENTRY_NOPROFILE(OpenLibrary)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	movl	%sp@(12),%d0
	jsr	%a6@(-0x228)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
#ifdef notyet
ENTRY_NOPROFILE(CloseLibrary)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x19e)
	movl	%sp@+,%a6
	rts
#endif
ENTRY_NOPROFILE(CreateIORequest)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%d0
	jsr	%a6@(-0x28e)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts

ENTRY_NOPROFILE(CreateMsgPort)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	jsr	%a6@(-0x29a)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
	
#ifdef notyet
ENTRY_NOPROFILE(DeleteMsgPort)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	jsr	%a6@(-0x2a0)
	movl	%sp@+,%a6
	rts
	
ENTRY_NOPROFILE(DeleteIORequest)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	jsr	%a6@(-0x294)
	movl	%sp@+,%a6
	rts
#endif
	
ENTRY_NOPROFILE(OpenDevice)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%d0
	movl	%sp@(16),%a1
	movl	%sp@(20),%d1
	jsr	%a6@(-0x1bc)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(DoIO)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1c8)
	movl	%sp@+,%a6
	rts
#ifdef nomore
ENTRY_NOPROFILE(CheckIO)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1d4)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
#endif
ENTRY_NOPROFILE(WaitIO)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1da)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(SendIO)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1ce)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(AbortIO)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1e0)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(WaitPort)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	jsr	%a6@(-0x180)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts

#ifndef DOINLINES
ENTRY_NOPROFILE(CacheClearU)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	jsr	%a6@(-0x27c)
	movl	%sp@+,%a6
	rts
#endif
ENTRY_NOPROFILE(CachePreDMA)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%a1
	movl	%sp@(16),%d0
	jsr	%a6@(-0x2fa)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(FindResident)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x60)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts

ENTRY_NOPROFILE(OpenResource)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):w),%a6
	movl	%sp@(8),%a1
	jsr	%a6@(-0x1f2)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
#ifdef notyet
ENTRY_NOPROFILE(Forbid)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):W),%a6
	jsr	%a6@(-0x84)
	movl	%sp@+,%a6
	rts

ENTRY_NOPROFILE(Permit)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(SysBase):W),%a6
	jsr	%a6@(-0x8a)
	movl	%sp@+,%a6
	rts
#endif

/*
 * Intuition.library functions.
 */

	.comm _C_LABEL(IntuitionBase),4

ENTRY_NOPROFILE(OpenScreenTagList)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(IntuitionBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%a1
	jsr	%a6@(-0x264)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts

ENTRY_NOPROFILE(OpenWindowTagList)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(IntuitionBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%a1
	jsr	%a6@(-0x25e)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
#ifdef nomore
ENTRY_NOPROFILE(mytime)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(IntuitionBase):w),%a6
	subql	#8,%sp
	movl	%sp,%a0
	lea	%sp@(4),%a1
	jsr	%a6@(-0x54)
	movl	%sp@+,%d0
	addql	#4,%sp
	movl	%sp@+,%a6
	rts
#endif
	.comm _C_LABEL(ExpansionBase),4
ENTRY_NOPROFILE(FindConfigDev)
	movl	%a6,%sp@-
	movl	%pc@(_C_LABEL(ExpansionBase):w),%a6
	movl	%sp@(8),%a0
	movl	%sp@(12),%d0
	movl	%sp@(16),%d1
	jsr	%a6@(-0x48)
	movl	%sp@+,%a6
	movl	%d0,%a0			| Comply with ELF ABI
	rts
