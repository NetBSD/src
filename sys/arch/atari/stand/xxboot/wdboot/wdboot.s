/*	$NetBSD: wdboot.s,v 1.5.6.2 2002/04/17 00:02:48 nathanw Exp $	*/

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

	tstb	%d5
	beqs	0f			| no boot preference
	cmpb	#0x20,%d5
	bnes	exit			| bootpref != NetBSD

0:	btst	#3,%d0			| Alternate?
	bnes	exit
	subql	#1,%d0
	movl	%d0,%a3			| autoboot flag
	
	movl	_membot:w,%d3
	lea	MAXBOT,%a4
	cmpl	%a4,%d3
	bhis	exit			| membot > MAXBOT

	movl	_memtop:w,%d3
	cmpl	#MINTOP,%d3
	blts	exit			| memtop < MINTOP

	andw	#-4,%d3
	movl	%d3,%a0
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
 * in:  d4 (target) d5 (count), d6 (offset), a4 (buffer)
 * out: d0 (<= 0)
 * mod: d0, d1, d2, a0, a1, a5, a6
 */
rds0:	lea	%pc@(dpar),%a6
	tstb	%a6@
	bnes	0f
	movb	%d4,%d0
	andb	#1,%d0
	aslb	#4,%d0
	orb	#0xa0,%d0
	movb	%d0,idesdh:l
	movl	%a4,%a0
	movq	#0,%d1
	movb	#0,idedor:l
	movb	#0xec,idecr:l		| IDENTIFY DRIVE
	bsrs	wait
	bnes	err
	movb	%a4@(7),%a6@		| tracks/cylinder
	movb	%a4@(13),%a6@(1)		| sectors/track
0:	movl	%d6,%d1
	movq	#0,%d0
	movb	%a6@(1),%d0
	movq	#0,%d2
	movb	%a6@,%d2
	mulu	%d0,%d2
	divu	%d2,%d1
	movb	%d1,idecl:l
	lsrl	#8,%d1
	movb	%d1,idech:l
	lsrl	#8,%d1
	divu	%d0,%d1
	movb	%d4,%d0
	andb	#1,%d0
	aslb	#4,%d0
	orb	%d0,%d1
	orb	#0xa0,%d1
	movb	%d1,idesdh:l
	swap	%d1
	addqw	#1,%d1
	movb	%d1,idesn:l
	movl	%a4,%a0
	movb	%d5,idesc:l
	movw	%d5,%d1
	subqw	#1,%d1
	movb	#0,idedor:l
	movb	#0x20,idecr:l
wait:	movl	#0x7d0,%d0
	addl	_hz_200:w,%d0
2:	btst	#5,gpip:w
	beqs	3f
	cmpl	_hz_200:w,%d0
	bhis	2b
err:	movq	#-1,%d0
	rts
3:	movb	idesr:l,%d0
	btst	#0,%d0
	bnes	err
	btst	#3,%d0
	beqs	err
	movq	#63,%d0
	lea	idedr:l,%a1
4:	movw	%a1@,%a0@+
	movw	%a1@,%a0@+
	movw	%a1@,%a0@+
	movw	%a1@,%a0@+
	dbra	%d0,4b
	dbra	%d1,wait
	movq	#0,%d0
	rts

regsav:	.long	0

fill:	.space	30

dpar:	.byte	0			| tracks/cylinder
	.byte	0			| sectors/track

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
