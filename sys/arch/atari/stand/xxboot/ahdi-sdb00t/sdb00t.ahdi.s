/*	$NetBSD: sdb00t.ahdi.s,v 1.3 2001/09/05 19:48:12 thomas Exp $	*/

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
	.globl	_start, main, fill, end

	.text

_start:	bras	main
#else
	.globl	start, main, fill, end

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
	cmpl	#0x444d4172,%d3		| SCSI bootdev?
	beqs	0f
	movq	#-1,%d4			| no, ACSI
	movq	#0,%d5
0:	movb	%d5,%d1			| NVRAM bootpref
	bnes	1f

	| The Hades bios does not provide a bootprev. In case
	| of doubt, we fetch it ourselves.
	movb	#BOOTPREF,rtcrnr:w
	movb	rtcdat:w,%d1
	bnes	1f
	movq	#-8,%d1	 		| bootpref = any

1:	movb	%a0@,%d2			| bootflags
	btst	#0,%d2
	beqs	2f
	andb	%d1,%d2
	bnes	boot
2:	lea	%a0@(12),%a0
	dbra	%d0,1b

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
 * in:  d4/d7 (target) d5 (count), d6 (offset), a4 (buffer)
 * out: d0 (<= 0)
 * mod: d0, d1, d2, a0, a1, a5, a6
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

fill:	.space	46

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
