/*	$NetBSD: exec_image.S,v 1.6 2026/11/25 11:01:38 isaki Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto.
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

#include <machine/asm.h>

#ifndef SRAM_MEMSIZE
#define SRAM_MEMSIZE	(0x00ed0008)
#endif

	.text
	.even

ENTRY_NOPROFILE(exec_image)
||	void exec_image(entry, size, bootdev, boothowto)
||	unsigned int entry;
||	int size;
||	int bootdev;
||	int boothowto;
	addql		#4,%sp			| throw away the return address
	moveml		%sp@+,%d4-%d7
	moval		%d4,%a0			| entry

	| These three stack parameters are currently not used by secondary
	| bootloader.  Only for the backward (and future?) compatibility.
	movel		%d5,%sp@-		| push last arg (esym)
	movel		SRAM_MEMSIZE,%sp@-	| second arg (physsize)
	pea		%a0@			| first arg (dummy firstpa)

	movq		#0,%d0			| reg arg (unused)
	movq		#0,%d1			| reg arg (unused)
	movq		#0,%d2			| reg arg (unused)
	movq		#0,%d3			| reg arg (unused)
	movq		#0,%d4			| reg arg (unused)
	movq		#0,%d5			| reg arg (unused)
						| bootdev(%d6) is already set
						| boothowto(%d7) is already set
						| entry(%a0) is already set
	moval		%d0,%a1			| reg arg (unused)
	moval		%d0,%a2			| reg arg (unused)
	moval		%d0,%a3			| reg arg (unused)
	moval		%d0,%a4			| reg arg (unused)
	moval		%d0,%a5			| reg arg (unused)
	moval		%d0,%a6			| reg arg (unused)

	jsr		%a0@			| here we go!

	/* NOTREACHED */
