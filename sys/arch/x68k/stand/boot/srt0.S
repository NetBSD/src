/*	$NetBSD: srt0.S,v 1.1 2001/09/27 10:14:49 minoura Exp $	*/

/*-
 * Copyright (c) 2001 Minoura Makoto
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <m68k/asm.h>	

ASENTRY(start)
	bra		start0
	.ascii		BOOT
	.byte		0x20
	.asciz		BOOT_VERS
	.even

/*
 * via stack:	(firstpa, physsize, esym)
 * via regs:	%d6:	bootdev
 *		%d7:	boothowto
 *  for /boot, only %d6 is used.
 */
start0:
	moveml		%sp@+,%d0-%d2
	lea		%pc@(start),%sp

	movl		%d6,%sp@-
	jbsr		_C_LABEL(bootmain)

ENTRY_NOPROFILE(exit)
	pea		%pc@(reboot_msg)
	jbsr		_C_LABEL(printf)
	jbsr		_C_LABEL(getchar)
	trap		#10

reboot_msg:	.asciz	"\n[Hit key to reboot]"
		.even

/*
 * Detect MPU type.  From locore.s.
 * Note we've already confirmed it is not 68000/010.
 */
ENTRY_NOPROFILE(detectmpu)
	movl	#0x200,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040/68060
	moveq	#3,%d0
	jra	Lgo			| no, we have 68030
Lnot68030:
	bset	#31,%d0			| data cache enable bit
	movc	%d0,%cacr		|   only exists on 68040/68060
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lis68020		| yes, we have 68020
	moveq	#0,%d0			| now turn it back off
	movec	%d0,%cacr		|   before we access any data
	.word	0xf4d8			| cinva bc - invalidate caches XXX
	bset	#30,%d0			| data cache no allocate mode bit
	movc	%d0,%cacr		|   only exists on 68060
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lis68040		| yes, we have 68040
	moveq	#6,%d0
	jra	Lgo
Lis68040:
	moveq	#4,%d0
	jra	Lgo
Lis68020:
	movl	#2,%d0			| and a 68020 CPU
Lgo:
	rts

/*
 * Check if the given address is valid for byte read.
 * From boot_ufs/boot.S.
 */
ENTRY_NOPROFILE(badbaddr)
		lea	0x0008:W,%a1		| MPU Bus Error vector
		moveq	#1,%d0
		lea	%pc@(badr1),%a0
		movew	%sr,%sp@-
		oriw	#0x0700,%sr		| keep out interrupts
		movel	%a1@,%sp@-
		movel	%a0,%a1@		| set bus error vector
		movel	%sp,%d1			| save sp
		moveal	%sp@(10),%a0
		tstb	%a0@			| try read...
		moveq	#0,%d0			| this is skipped on bus error
badr1:		moveal	%d1,%sp			| restore sp
		movel	%sp@+,%a1@
		movew	%sp@+,%sr
		rts
