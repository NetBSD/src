/*	$NetBSD: start.s,v 1.1 1995/03/29 21:24:16 ragge Exp $ */
/*
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by
 * Bertram Barth.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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
 */

 /* All bugs are subject to removal without further notice */
		


#include "DEFS.h"		
#include "bootdefs.h"

/* ---------------------------------------------------------------------- */
_start:	.globl _start		# this is the symbolic name for the start
				# of code to be relocated. We can use this
				# to get the actual/real adress (pc-rel)
				# or to get the relocated address (abs).
LBN0:	
/* ---------------------------------------------------------------------- */
.org	0x00			# uVAX booted from TK50 starts here
	brb	from_0x00	# continue behind dispatch-block

.org	0x02			# information used by uVAX-ROM
	.byte  	IAREA_OFFSET	# offset in words to identification area 
	.byte	1		# this byte must be 1(defined by DEC)
	.word	0		# logical block number (word swapped) 
	.word	0		# of the secondary image

.org	0x08			#
	brb	from_0x08	# skip ...

.org	0x0A			# uVAX booted from disk starts here
	brb	from_0x0A	# skip ...

.org	0x0C			# 11/750 starts here
	brw	cont_750	# skip ...

/* -------------------- */
from_0x00:			# uVAX from TK50 
	xorl2	$4, bootinfo	# this variable tells the difference
from_0x0A:			# uVAX from disk
	pushr	regmask		# save registers ...
	xorl2	$1, bootinfo	# mark where we came from
	brw	cont_uvax	# all(?) uVAXen continue there

from_0x08:			#
	pushr	regmask		#
        xorl2   $1, bootinfo		/* bertram: ??? */
	cmpb	102(r11), $35		/* check if device-type == TK50 */
	bneq	1f
	xorl2   $4, bootinfo		/* yes, it's a TK50 */
1:	brw	cont_tape

.org	LABELSTART - 6
regmask: 	.word 0x0fff	# using a variable saves 3 bytes !!!
bootinfo:	.long 0x0	# another 3 bytes if within byte-offset
/* -------------------- */
.org	LABELSTART

.org	LABELEND

/* -------------------- */
.org	IAREA_OFFSET * 2
	.byte	0x18		/* must be 0x18 */
	.byte	0x00		/* must be 0x00 (MBZ) */
	.byte	0x00		/* any value */
	.byte	0xFF - (0x18 + 0x00 + 0x00)	
		/* 4th byte holds 1s' complement of sum of previous 3 bytes */

	.byte	0x00		/* must be 0x00 (MBZ) */
	.byte	VOLINFO
	.byte	0x00		/* any value */
	.byte	0x00		/* any value */

	.long	SISIZE		/* size in blocks of secondary image */
	.long	SILOAD		/* load offset (usually 0) */
	.long 	SIOFF		/* byte offset into secondary image */
	.long	(SISIZE + SILOAD + SIOFF) 	/* sum of previous 3 */


/* -------------------- */
cont_tape:				# uVAXen from tape /* ??? */
	movl	52(r11), r7		# load iovec into r7 
	addl3	(r7), r7, funcaddr	# store address of qio-entry 
	moval	LBN1, bufaddr;
					# start pushing arguments for qio: 
	pushl	r11				# base of RPB 
	pushl	$0				# virtual flag 
	pushl	$33				# read-logical-block 
	pushl	$1				# LBN to start reading
	pushl	$(512 * LOADSIZE)		# bytes to read 
	pushl	bufaddr				# buffer-address 
	calls	$6, *funcaddr		# and call the qio-routine 
	blbs	r0, cont_uvax		# if successful, goto next stage 
	halt				# if not: FATAL !!!

/* -------------------- */
cont_uvax:
	brw	start_uvax

/* -------------------- */
.align 2
cont_750:
	/*
	 * After bootblock (LBN-0) has been loaded into the first page 
	 * and of good memory by 11/750's ROM-code (transfer address
	 * of bootblock-code is: base of good memory + 0x0C) registers
	 * are initialized as:
	 * 	R0:	type of boot-device
	 *			0:	Massbus device
	 *			1:	RK06/RK07
	 *			2:	RL02
	 *			17:	UDA50
	 *			35:	TK50
	 *			64:	TU58
	 *	R1:	(UBA) address of UNIBUS I/O-page
	 *		(MBA) address of boot device's adapter
	 * 	R2:	(UBA) address of the boot device's CSR
	 *		(MBA) controller number of boot device
	 *	R6:	address of driver subroutine in ROM
	 */
        movl    r0,r10
        movl    r5,r11 /* No conversion of bdev in block 0 */
        clrl    r5
        clrl    r4
        movl    $0xa0000,sp
1:      incl    r4
        movl    r4,r8
        addl2   $0x200,r5
        cmpl    $16,r4
        beql    2f
        pushl   r5
        jsb     (r6)
        blbs    r0,1b
2:      movl	r10,r0
	movl	r11,r5

	brw	start_750

.org	512 - 8 			/* make room for variables */

funcaddr:	.long 0x11111111
bufaddr:	.long 0x33333333

.org	512				/* might be paranoid ... */
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
LBN1:
/* ---------------------------------------------------------------------- */
.org 	512

/* -------------------- */
start_uvax:
	popr	regmask		# restore contents of boot-regs
	jsb	saveregs	# and save them
	movzbl	102(r11), r10	# rpb->devtype describes device	(bdev)
	movl	r5, r11		# r5 holds boot-control-flags 	(howto)
	brb	start_all

/* -------------------- */
start_750:
	jsb 	saveregs	# and save them ...
	movl	r0, r10		# r0 holds type of boot-device	(bdev)
	movl	r5, r11		# r5 holds boot-control-flags 	(howto)
	brb	start_all

/* -------------------- */
start_all:
	movl	$_start, sp	# new stack directly beyond reloc
	subl2	$48, sp		# don\'t overwrite saved boot-registers
	pushr	$0xfff		#
	subl3	$_start, $_end, r0	# calculate size of code
	moval	_start, r1	# get actual base-address of code
	movl	$_start, r2	# get relocated base-address of code
	movc3	r0, (r1), (r2)	# copy the code to new location
	popr	$0xfff
	jsb	L1		   # jump to some subroutine where 
L1:	movl	$relocated, (sp)   # return-address on top of stack 
	rsb			   # can be replaced with new address
relocated:			# now relocation is done !!!
	calls	$0, _main	# call main() which is 
	halt			# not intended to return ...

/* ---------------------------------------------------------------------- */
ENTRY(hoppabort, 0)
        movl    4(ap),r6
        movl    8(ap),r11
        movl    0xc(ap),r10
        calls   $0,(r6)
	halt

/* ---------------------------------------------------------------------- */
save_sp: .long
saveregs:
	movl	sp, save_sp		# save stack
	movl	$_start, sp		# make new/dummy stack
	pushr	$0xfff			# push registers r0 - r11
	movl	save_sp, sp		# and restore stack
	rsb	
/* ---------------------------------------------------------------------- */
/*
 * int relocate (int base, int size, int top)
 */
ENTRY(relocate, 0)
	subl2	8(ap), 12(ap)		# decrease top-address by size
	bicl2	$1023, 12(ap)		# make the address page-aligned
	movc3	8(ap), *4(ap), *12(ap)	# copy the area/data
	movl	12(ap), r0		# return the new base-address
	ret
/*----------------------------------------------------------------------*/
/*
 * EOF
 */
