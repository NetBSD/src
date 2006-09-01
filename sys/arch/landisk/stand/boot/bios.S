/*	$NetBSD: bios.S,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

/*-
 * Copyright (c) 2005 NONAKA Kimihiro
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
 */

#include <machine/asm.h>

/*
 * int bioscons_getc(void);
 */
ENTRY(bioscons_getc)
	mov	#0, r4
	mov	#1, r0
	trapa	#0x3f
	rts
	 nop

/*
 * void bioscons_putc(int c);
 */
ENTRY(bioscons_putc)
	mov	#31, r0
	trapa	#0x3f
	rts
	 nop

/*
 * int biosdisk_read(int dev, uint32_t blkaddr, void *buf, size_t nblks);
 */
ENTRY(biosdisk_read)
	mov	#2, r0
	trapa	#0x3f
	rts
	 nop

/*
 * void cache_enable(void);
 * void cache_flush(void);
 */
ENTRY(cache_enable)
ENTRY(cache_flush)
	mov     #0, r4
	mov     #6, r0
	trapa   #0x3f
	rts
	 nop

/*
 * void cache_disable(void);
 */
ENTRY(cache_disable)
	mov     #1, r4
	mov     #6, r0
	trapa   #0x3f
	rts
	 nop
