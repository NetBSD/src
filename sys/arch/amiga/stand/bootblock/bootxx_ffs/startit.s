/*	$NetBSD: startit.s,v 1.1.2.3 2002/06/23 17:34:34 jdolecek Exp $	*/

/*
 * Copyright (c) 1996 Ignatios Souvatzis
 * Copyright (c) 1994 Michael L. Hitch
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
 *      This product includes software developed by Michael L. Hitch.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 * From: $NetBSD: startit.s,v 1.1.2.3 2002/06/23 17:34:34 jdolecek Exp $
 */
#include "machine/asm.h"

	.set	ABSEXECBASE,4

	.text

ENTRY_NOPROFILE(startit)
	movel	4:w,%a6			| SysBase
	movel	%sp@(4),%a0		| Boot loader address
	movel	%sp@(8),%a1		| IOR
	movel	%sp@(12),%a5		| Console data
/*
 * Installboot can modify the default command in the bootblock loader,
 * but boot.amiga uses the default command in boot.amiga.  Copy the
 * possibly modified default command before entering the boot loader.
 */
	lea	%pc@(_C_LABEL(default_command)),%a2
	lea	%a0@(16),%a3
	moveq	#(32/4)-1,%d0
Lcommand:
	movel	%a2@+,%a3@+
	dbra	%d0,Lcommand

	jsr	%a0@(12)
	rts
