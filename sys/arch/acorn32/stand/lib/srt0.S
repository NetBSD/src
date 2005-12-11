/*	$NetBSD: srt0.S,v 1.4 2005/12/11 12:16:08 christos Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 *
 * Modified by Reinoud Zandijk to get the program high water mark into the
 * HIMEM variable as normally seen in RISC OS. This is the max address+1 of
 * the current `WIMPSlot'. It used to be a #define. The HIMEM var needs to be
 * initialised though so it doesn't get zero'd when the BSS is later cleared.
 * (see srt1.c)
 */

/* Minimum desired free memory */

#ifndef BOOT_MIN_AVAIL_MEMORY
#define BOOT_MIN_AVAIL_MEMORY	10 * 1024 * 1024
#endif

/* If enough above BOOT_MIN_AVAIL_MEMORY, leave BOOT_LEAVE_MEMORY unused */
/* Avoids sparkfs failing on memory if we try to read a kernel from it */

#ifndef BOOT_LEAVE_MEMORY
#define BOOT_LEAVE_MEMORY	5 * 1024 * 1024
#endif

/*
 * Both BOOT_LEAVE_MEMORY and BOOT_MIN_AVAIL_MEMORY + BOOT_LEAVE_MEMORY
 * must fit into an immediate ATM constant - 8 bits with arbitrary rotation
 */

#include <machine/asm.h>
#include <riscoscalls.h>

ENTRY(_start)
#ifdef BOOT_MAXIMISE_MEMORY
	mvn	r0, #0
	mvn	r1, #0
	swi	Wimp_SlotSize
	add	r0, r0, r2
#if (BOOT_LEAVE_MEMORY > 0)
	cmp	r0, #BOOT_MIN_AVAIL_MEMORY + BOOT_LEAVE_MEMORY
	subhi	r0, r0, #BOOT_LEAVE_MEMORY
#endif 
	mvn	r1, #0
	swi	Wimp_SlotSize
#endif
	swi	OS_GetEnv
	ldr	r6, Lhimem
	str	r1, [r6]
	mov	sp, r1
	mov	fp, #0
	b	__start

Lhimem:
	.word	HIMEM

