/*	$NetBSD: sdboot.s,v 1.4.6.2 2002/04/17 00:02:47 nathanw Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens
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
 *        This product includes software developed by Waldi Ravens.
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

#include "xxboot.h"

#ifdef __ELF__
	.globl	_start, main, fill

	.text
_start:
#else
	.globl	start, main, fill

	.text
start:
#endif
/*
 * in: d3 ('DMAr' flag), d4 (SCSI target), d5 (boot pref), d7 (ACSI target)
 */
main:	lea	%pc@(regsav),%a0
	movml	%d3-%d5/%d7,%sp@-
	movl	%sp,%a0@

	movw	#-1,%sp@-
	movw	#Kbshift,%sp@-
	trap	#BIOS
	addql	#4,%sp

	cmpl	#0x444d4172,%d3		| SCSI bootdev?
	beqs	0f
	movq	#-1,%d4			| no, ACSI
	bras	1f

0:	tstb	%d5
	beqs	1f			| no boot preference
	cmpb	#0x20,%d5
	bnes	exit			| bootpref != NetBSD

1:	btst	#3,%d0			| Alternate?
	bnes	exit
	subql	#1,%d0
	movl	%d0,%a3			| autoboot flag

	movl	_membot:w,%d0
	lea	MAXBOT,%a4
	cmpl	%a4,%d0
	bhis	exit			| membot > MAXBOT

	movl	_memtop:w,%d0
	cmpl	#MINTOP,%d0
	blts	exit			| memtop < MINTOP

	andw	#-4,%d0
	movl	%d0,%a0
	movl	%sp,%a0@-
	movl	%a0,%sp			| set new stack

	movq	#NSEC,%d5		| sector count
	movq	#1,%d6			| first sector
	bsrs	rds0
	tstl	%d0
	bnes	0f
/*
 * loader (readsector, disklabel, autoboot)
 */
	pea	%a3@			| autoboot
	pea	%a4@(LBLST-MAXBOT)	| disklabel
	pea	%pc@(rds1)		| readsector
	jsr	%a4@(BXXST-MAXBOT)
	lea	%sp@(12),%sp		| NetBSD not booted

0:	movl	%sp@,%sp			| restore BIOS stack
	tstl	%d0
	bmis	exit
	movl	%d0,%sp@(8)		| new boot preference

exit:	movml	%sp@+,%d3-%d5/%d7
	rts

/*
 * int readsec (void *buffer, u_int offset, u_int count);
 */
rds1:	movml	%d2-%d7/%a2-%a6,%sp@-
	movl	%pc@(regsav),%a0
	movml	%a0@,%d3-%d5/%d7
	movl	%sp@(48),%a4		| buffer
	movl	%sp@(52),%d6		| offset
	movl	%sp@(56),%d3		| count
0:	movl	#255,%d5
	cmpl	%d5,%d3
	bccs	1f
	movl	%d3,%d5
1:	bsrs	rds0
	tstl	%d0
	bnes	2f
	addl	#(255*512),%a4
	addl	%d5,%d6
	subl	%d5,%d3
	bnes	0b
2:	movml	%sp@+,%d2-%d7/%a2-%a6
	rts
/*
 * in:  d4/d7 (target) d5 (count), d6 (offset), a4 (buffer)
 * out: d0 (<= 0)
 * mod: d0, d1, d2, a0, a5, a6
 */
rds0:	tstl	%d4
	bmis	0f
	movw	%d4,%sp@-			| device
	pea	%a4@			| buffer
	movw	%d5,%sp@-			| count
	movl	%d6,%sp@-			| offset
	movw	#DMAread,%sp@-
	trap	#XBIOS
	lea	%sp@(14),%sp
	rts

0:	st	flock:w
	movl	_hz_200:w,%d0
	addql	#2,%d0
1:	cmpl	_hz_200:w,%d0
	bccs	1b
	movml	%d6/%a4,%sp@-
	lea	dmahi:w,%a6
	movb	%sp@(7),%a6@(4)
	movb	%sp@(6),%a6@(2)
	movb	%sp@(5),%a6@
	lea	%pc@(r0com),%a6
	movb	%sp@(1),%a6@(1)
	movb	%sp@(2),%a6@(5)
	movb	%sp@(3),%a6@(9)
	movb	%d5,%a6@(13)
	addql	#8,%sp
	lea	dmodus:w,%a6
	lea	daccess:w,%a5
	movw	#0x198,%a6@
	movw	#0x098,%a6@
	movw	%d5,%a5@
	movw	#0x88,%a6@
	movq	#0,%d0
	movb	%d7,%d0
	orb	#0x08,%d0
	swap	%d0
	movw	#0x8a,%d0
	bsrs	shake
	lea	%pc@(r0com),%a0
	movq	#3,%d2
2:	movl	%a0@+,%d0
	bsrs	shake
	dbra	%d2,2b
	movq	#0x0a,%d0
	movl	%d0,%a5@
	movl	#0x190,%d1
	bsrs	wait
	movw	#0x8a,%a6@
	movw	%a5@,%d0
	andw	#0xff,%d0
	beqs	0f
r0err:	movq	#-1,%d0
0:	movw	#0x80,%a6@
	clrb	flock:w
r0ret:	rts

shake:	movl	%d0,%a5@
	movq	#0x0a,%d1
wait:	addl	_hz_200:w,%d1
0:	btst	#5,gpip:w
	beqs	r0ret
	cmpl	_hz_200:w,%d1
	bccs	0b
	addql	#4,%sp
	bras	r0err

r0com:	.long	0x0000008a
	.long	0x0000008a
	.long	0x0000008a
	.long	0x0001008a

regsav:	.long	0

fill:	.space	24

	.ascii	"NetBSD"
hd_siz:	.long	0
p0_dsc:	.long	0, 0, 0
p1_dsc:	.long	0, 0, 0
p2_dsc:	.long	0, 0, 0
p3_dsc:	.long	0, 0, 0
bsl_st:	.long	0
bsl_sz:	.long	0
	.word	0
end:
