/*	$NetBSD: wdb00t.ahdi.s,v 1.4.6.2 2002/04/17 00:02:46 nathanw Exp $	*/

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

_start:	bras	main
#else
	.globl	start, main, fill

	.text

start:	bras	main
#endif
	bra	rds0

main:	bclr	#2,(_drvbits+3):w
	clrl	pun_ptr:w
	movml	%d3/%d5,%sp@-
	movw	#-1,%sp@-
	movw	#Kbshift,%sp@-
	trap	#BIOS
	addql	#4,%sp
	btst	#3,%d0			| Alternate?
	bnes	exit
	movq	#3,%d0
	lea	%pc@(p0_dsc),%a0
	movb	%d5,%d1			| NVRAM bootpref
	bnes	0f

	| The Hades bios does not provide a bootprev. In case
	| of doubt, we fetch it ourselves.
	movb	#BOOTPREF,rtcrnr:w
	movb	rtcdat:w,%d1
	bnes	0f
	movq	#-8,%d1	 		| bootpref = any

0:	movb	%a0@,%d2			| bootflags
	btst	#0,%d2
	beqs	1f
	andb	%d1,%d2
	bnes	boot
1:	lea	%a0@(12),%a0
	dbra	%d0,0b

exit:	movml	%sp@+,%d3/%d5
tostst:	clrw	_bootdev:w
	movl	_sysbase:w,%a0
	movl	%a0@(24),%d0
	swap	%d0
	cmpl	#0x19870422,%d0		| old TOS?
	bccs	0f			| no
	movw	#0xe0,%d7
0:	rts

boot:	movl	%a0@(4),%d6
	movq	#1,%d5
	lea	%pc@(end),%a4
	bsrs	rds0
	tstw	%d0
	bnes	exit
	movl	%a4,%a0
	movw	#0xff,%d0
	movq	#0,%d1
0:	addw	%a0@+,%d1
	dbra	%d0,0b
	cmpw	#0x1234,%d1
	bnes	exit
	lea	%pc@(rds0),%a3
	lea	%pc@(tostst),%a5
	movml	%sp@+,%d3/%d5
	jmp	%a4@			| start bootsector code
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

fill:	.space	52

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
