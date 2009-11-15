/*	$NetBSD: startit.s,v 1.9 2009/11/15 20:38:36 snj Exp $	*/

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
 * From: $NetBSD: startit.s,v 1.9 2009/11/15 20:38:36 snj Exp $
 */
#include <machine/asm.h>

	.set	ABSEXECBASE,4

	.text

ENTRY_NOPROFILE(startit)
#if TESTONAMIGA
	movew	#0x999,0xdff180		| gray
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#31,0x200003c9
	moveb	#31,0x200003c9
	moveb	#31,0x200003c9
#endif
	movel	%sp,%a3
	movel	4:w,%a6
	lea	%pc@(start_super:w),%a5
	jmp	%a6@(-0x1e)		| supervisor-call

start_super:
#if TESTONAMIGA
	movew	#0x900,0xdff180		| dark red
#endif
	movew	#0x2700,%sr

	| the BSD kernel wants values into the following registers:
	| %a0:  fastmem-start
	| %d0:  fastmem-size
	| %d1:  chipmem-size
	| %d3:  Amiga specific flags
	| %d4:  E clock frequency
	| %d5:  AttnFlags (cpuid)
	| %d6:  boot partition offset
	| %d7:  boothowto
	| %a4:  esym location
	| %a2:  Inhibit sync flags
	| All other registers zeroed for possible future requirements.

	lea	%pc@(_C_LABEL(startit):w),%sp	| make sure we have a good stack ***

	movel	%a3@(4),%a1		| loaded kernel
	movel	%a3@(8),%d2		| length of loaded kernel
|	movel	%a3@(12),%sp		| entry point in stack pointer
	movel	%a3@(12),%a6		| entry point		***
	movel	%a3@(16),%a0		| fastmem-start
	movel	%a3@(20),%d0		| fastmem-size
	movel	%a3@(24),%d1		| chipmem-size
	movel	%a3@(28),%d7		| boothowto
	movel	%a3@(32),%a4		| esym
	movel	%a3@(36),%d5		| cpuid
	movel	%a3@(40),%d4		| E clock frequency
	movel	%a3@(44),%d3		| Amiga flags
	movel	%a3@(48),%a2		| Inhibit sync flags
	movel	%a3@(52),%d6		| boot partition offset

	cmpb	#0x7D,%a3@(36)		| is it DraCo?
	movel	%a3@(56),%a3		| Load to fastmem flag
	jeq	nott			| yes, switch off MMU later

					| no, it is an Amiga:

#if TESTONAMIGA
	movew	#0xf00,0xdff180		|red
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#63,0x200003c9
	moveb	#0,0x200003c9
	moveb	#0,0x200003c9
#endif

	movew	#(1<<9),0xdff096	| disable DMA on Amigas.

| ------ mmu off start -----

	btst	#3,%d5			| AFB_68040,SysBase->AttnFlags
	jeq	not040

| Turn off 68040/060 MMU

	subl	%a5,%a5
	.word 0x4e7b,0xd003		| movec %a5,tc
	.word 0x4e7b,0xd806		| movec %a5,urp
	.word 0x4e7b,0xd807		| movec %a5,srp
	.word 0x4e7b,0xd004		| movec %a5,itt0
	.word 0x4e7b,0xd005		| movec %a5,itt1
	.word 0x4e7b,0xd006		| movec %a5,dtt0
	.word 0x4e7b,0xd007		| movec %a5,dtt1
	jra	nott

not040:
	lea	%pc@(zero:w),%a5
	pmove	%a5@,%tc		| Turn off MMU
	lea	%pc@(nullrp:w),%a5
	pmove	%a5@,%crp		| Turn off MMU some more
	pmove	%a5@,%srp		| Really, really, turn off MMU

| Turn off 68030 TT registers

	btst	#2,%d5			| AFB_68030,SysBase->AttnFlags
	jeq	nott			| Skip TT registers if not 68030
	lea	%pc@(zero:w),%a5
	.word 0xf015,0x0800		| pmove %a5@,tt0 (gas only knows about 68851 ops..)
	.word 0xf015,0x0c00		| pmove %a5@,tt1 (gas only knows about 68851 ops..)

nott:
| ---- mmu off end ----
#if TESTONAMIGA
	movew	#0xf60,0xdff180		| orange
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#63,0x200003c9
	moveb	#24,0x200003c9
	moveb	#0,0x200003c9
#endif


| ---- copy kernel start ----

| removed Z flag
|	tstl	%a3			| Can we load to fastmem?
|	jeq	L0			| No, leave destination at 0
	movl	%a0,%a3			| Move to start of fastmem chunk
	addl	%a0,%a6			| relocate kernel entry point

	addl	#3,%d2
	andl	#0xfffffffc,%d2		| round up.

	| determine if the kernel need be copied upwards or downwards

	cmpl	%a1,%a3			| %a3-a1
	bcs	above			| source is above

	movl	%a0,%sp
	addl	%d0,%sp			| move the stack to the end of segment

	| copy from below upwards requires copying from end to start.

	addl	%d2,%a3			| one long word past
	addl	%d2,%a1			| one long word past

	subl	#4,%sp			| alloc space
	movl	%a1,%sp@-			| save source
	movl	%a3,%sp@-			| save destination

	| copy copier to end of segment

	movl	%sp,%a3
	subl	#256,%a3			| end of segment save our stack

	lea	%pc@(_C_LABEL(startit_end):w),%a1
	movl	%a0,%sp@-			| save segment start
	lea	%pc@(below:w),%a0

L0:	movw	%a1@-,%a3@-
	cmpl	%a0,%a1
	bne	L0
	movl	%sp@,%a0			| restore segment start
	movl	%a3,%sp@			| address of relocated below
	addl	#(ckend - below),%a3
	movl	%a3,%sp@(12)		| address of ckend for later
| ---- switch off cache ----
	bra	Lchoff			| and to relocated below


below:	movl	%sp@+,%a3			| recover destination
	movl	%sp@+,%a1			| recover source

L1:	movl	%a1@-,%a3@-		| copy kernel
	subl	#4,%d2
	bne	L1

| ---- switch off cache ----
	bra	Lchoff			| and to relocated ckend

above:	movl	%a1@+,%a3@+
	subl	#4,%d2
	bne	above

	lea	%pc@(ckend:w),%a1
	movl	%a3,%sp@-
	pea	%pc@(_C_LABEL(startit_end):w)
L2:
	movl	%a1@+,%a3@+
	cmpl	%sp@,%a1
	bcs	L2
	addql	#4,%sp

#if TESTONAMIGA
	movew	#0xFF0,0xdff180		| yellow
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#63,0x200003c9
	moveb	#63,0x200003c9
	moveb	#0,0x200003c9
#endif

| ---- switch off cache ----
Lchoff:	btst	#3,%d5
	jeq	L3c
	.word	0xf4f8
L3c:	movl	%d2,%sp@-			| save %d2
	movql	#0,%d2			| switch off cache to ensure we use
	movec	%d2,%cacr			| valid kernel data
	movl	%sp@+,%d2			| restore %d2
	rts

| ---- copy kernel end ----

ckend:
#if TESTONAMIGA
	movew	#0x0ff,0xdff180		| petrol
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#0,0x200003c9
	moveb	#63,0x200003c9
	moveb	#63,0x200003c9
#endif

	movl	%d5,%d2
	roll	#8,%d2
	cmpb	#0x7D,%d2
	jne	noDraCo

| DraCo: switch off MMU now:

	subl	%a5,%a5
	.word 0x4e7b,0xd003		| movec %a5,tc
	.word 0x4e7b,0xd806		| movec %a5,urp
	.word 0x4e7b,0xd807		| movec %a5,srp
	.word 0x4e7b,0xd004		| movec %a5,itt0
	.word 0x4e7b,0xd005		| movec %a5,itt1
	.word 0x4e7b,0xd006		| movec %a5,dtt0
	.word 0x4e7b,0xd007		| movec %a5,dtt1
	
noDraCo:
	moveq	#0,%d2			| zero out unused registers
	movel	%d2,%a1			| (might make future compatibility
	movel	%d2,%a3			|  would have known contents)
	movel	%d2,%a5
	movel	%a6,%sp			| entry point into stack pointer
	movel	%d2,%a6

#if TESTONAMIGA
	movew	#0x0F0,0xdff180		| green
#endif
#if TESTONDRACO
	moveb	#0,0x200003c8
	moveb	#0,0x200003c9
	moveb	#63,0x200003c9
	moveb	#0,0x200003c9
#endif

	jmp	%sp@			| jump to kernel entry point


| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x7fff0001
zero:	.long	0

ENTRY_NOPROFILE(startit_end)
