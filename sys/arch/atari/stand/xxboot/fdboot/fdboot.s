/*	$NetBSD: fdboot.s,v 1.4 2001/09/05 19:48:13 thomas Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
/*
 * Fake gemdos-fs bootsector, to keep TOS away.
 */
	.ascii	"NetBSD"	| oem
	.byte	0,0,0		| serial
	.word	0		| bps
	.byte	0		| spc
	.word	0		| res
	.byte	0		| fat
	.word	0		| ndirs
	.word	0		| sec
	.byte	0		| media
	.word	0		| spf
	.word	0		| spt
	.word	0		| side
	.word	0		| hid
	.even
/*
 * ROM loader does not save any register!
 */
main:	movml	%d1-%d7/%a0-%a6,%sp@-

	lea	%pc@(m_bot),%a3
	movl	_membot:w,%d3
	lea	MAXBOT,%a6
	cmpl	%a6,%d3
	bhis	exit			| membot > MAXBOT

	lea	%pc@(m_top),%a3
	movl	_memtop:w,%d3
	cmpl	#MINTOP,%d3
	blts	exit			| memtop < MINTOP

	andw	#-4,%d3
	movl	%d3,%a0
	movl	%sp,%a0@-
	movl	%a0,%sp			| set new stack
/*
 * Determine the number of sectors per cylinder.
 */
	movq	#0,%d3
0:	movl	%a6,%a3			| buffer
	movq	#1,%d0			| count
	movq	#0,%d1			| side
	movl	%d1,%d2			| track
	addw	%d0,%d3			| sector
	bsr	rds0
	beqs	0b
	subqw	#1,%d3
	addw	%d3,%d3
	lea	%pc@(secpercyl),%a0
	movw	%d3,%a0@
/*
 * Load secondary boot loader and disklabel.
 */
	movq	#NSEC,%d5		| # of sectors
	movq	#1,%d4			| first sector
	movl	%a6,%a3			| load address
	bsr	rds1
	lea	%pc@(m_rds),%a3
	bnes	0f			| I/O error
/*
 * int bootxx(readsector, disklabel, autoboot)
 */
	clrl	%sp@-			| no autoboot
	pea	%a6@(LBLST-MAXBOT)	| disklabel
	pea	%pc@(rds2)		| readsector
	jsr	%a6@(BXXST-MAXBOT)
	lea	%pc@(m_sbl),%a3		| NetBSD not booted
	lea	%sp@(12),%sp
0:	movl	%sp@,%sp			| restore BIOS stack
	movl	%d0,%d3

exit:	bsrs	puts			| display error
	lea	%pc@(m_key),%a3
	bsrs	puts			| wait for key
0:	movml	%sp@+,%d1-%d7/%a0-%a6
	movq	#0,%d0
	rts
/*
 * puts (in: a3, d3)
 */
0:	cmpw	#35,%d0		| '#'
	bnes	1f
	bsrs	puti
	bras	puts
1:	cmpw	#64,%d0		| '@'
	bnes	2f
	movw	#2,%sp@-
	movw	#Bconin,%sp@-
	trap	#BIOS
	addql	#4,%sp
	bras	puts
2:	bsrs	putc
puts:	movq	#0,%d0
	movb	%a3@+,%d0
	bnes	0b
	rts

puti:	swap	%d3
	bsrs	0f
	swap	%d3
0:	rorw	#8,%d3
	bsrs	1f
	rorw	#8,%d3
1:	rorb	#4,%d3
	bsrs	2f
	rorb	#4,%d3
2:	movw	%d3,%d0
	andw	#15,%d0
	addw	#48,%d0
	cmpw	#58,%d0
	bcss	putc
	addw	#39,%d0
putc:	movw	%d0,%sp@-
	movw	#2,%sp@-
	movw	#Bconout,%sp@-
	trap	#BIOS
	addql	#6,%sp
	rts
/*
 * int readsec (void *buffer, u_int offset, u_int count);
 */
rds2:	movml	%d2-%d5/%a2-%a3,%sp@-
	movl	%sp@(28),%a3		| buffer
	movl	%sp@(32),%d4		| offset
	movl	%sp@(36),%d5		| count
	bsrs	rds1
	movml	%sp@+,%d2-%d5/%a2-%a3
	rts

rds1:	bsrs	1f
	bnes	0f
	tstl	%d5
	bnes	rds1
0:	rts

1:	movq	#0,%d0
	movw	%pc@(secpercyl),%d0
	movl	%d4,%d3
	divuw	%d0,%d3
	movw	%d3,%d2		| track
	clrw	%d3
	swap	%d3
	lsrw	#1,%d0
	divuw	%d0,%d3
	movw	%d3,%d1		| side
	swap	%d3
	subw	%d3,%d0
	addqw	#1,%d3		| sector
	cmpl	%d0,%d5
	bccs	rds0
	movw	%d5,%d0

rds0:	movw	%d0,%sp@-		| count
	movw	%d1,%sp@-		| side
	movw	%d2,%sp@-		| track
	movw	%d3,%sp@-		| sector
	movw	_bootdev:w,%sp@-	| device
	clrl	%sp@-		| filler
	movl	%a3,%sp@-		| buffer
	addl	%d0,%d4
	subl	%d0,%d5
	lsll	#8,%d0
	addl	%d0,%d0
	addl	%d0,%a3
	movw	#Floprd,%sp@-
	trap	#XBIOS
	lea	%sp@(20),%sp
	tstl	%d0
	rts

secpercyl:
	.word	0

m_bot:	.asciz	"fdboot: membot == 0x#\r\n"
m_top:	.asciz	"fdboot: memtop == 0x#\r\n"
m_sbl:	.asciz	"fdboot: bootxx => 0x#\r\n"
m_rds:	.asciz	"fdboot: Floprd => 0x#\r\n"
m_key:	.asciz	"\007\r\npress any key... @\r\n"

fill:	.space	22		| 510-(fill-start)
	.word	0		| checksum
end:
