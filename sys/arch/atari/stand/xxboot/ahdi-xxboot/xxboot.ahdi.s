/*	$NetBSD: xxboot.ahdi.s,v 1.2 1996/03/18 21:06:19 leo Exp $	*/

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

	.globl	start, main, fill, end

	.text

start:	bras	main
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
 * AHDI rootsector code passes:
 * d3: SCSI/IDE <-> ACSI flag	d4: SCSI/IDE target (or -1)
 * d5: boot preference		d6: offset of boot block
 * d7: ACSI target		a3: pointer to readsector()
 * a4: pointer to `start'	a5: pointer to back-to-ROM
 */
main:	movml	d3-d7/a3-a5,sp@-
	lea	pc@(regsav),a0
	movl	sp,a0@

	movw	#-1,sp@-
	movw	#Kbshift,sp@-
	trap	#BIOS
	addql	#4,sp
	subql	#1,d0
	movl	d0,a5			| autoboot flag

	lea	pc@(m_bot),a6
	movl	_membot:w,d3
	lea	MAXBOT,a4
	cmpl	a4,d3
	bhis	exit			| membot > MAXBOT

	lea	pc@(m_top),a6
	movl	_memtop:w,d3
	movl	_v_bas_ad:w,d0
	cmpl	d0,d3
	blts	0f			| memtop < v_bas_ad
	movl	d0,d3
0:	cmpl	#MINTOP,d3
	blts	exit			| memtop < MINTOP

	andw	#-4,d3
	movl	d3,a0
	movl	sp,a0@-
	movl	a0,sp			| set new stack
/*
 * Load secondary boot loader and disklabel.
 */
	movml	a4/a5,sp@-
	movq	#NSEC,d3		| # of sectors
	addql	#1,d6			| first sector
	bsr	rds1
	lea	pc@(m_rds),a6
	movml	sp@+,a4/a5
	bnes	0f			| I/O error
/*
 * int bootxx(readsector, disklabel, autoboot)
 */
	pea	a5@			| autoboot
	pea	a4@(LBLST-MAXBOT)	| disklabel
	pea	pc@(rds2)		| readsector
	jsr	a4@(BXXST-MAXBOT)
	lea	pc@(m_sbl),a6		| NetBSD not booted
	lea	sp@(12),sp

0:	movl	sp@,sp			| restore BIOS stack
	movl	d0,d3
	bmis	exit
	movml	sp@+,d3-d7/a3-a5
	movl	d0,d5			| new boot preference
	jmp	a4@(-0x200)

exit:	bsrs	puts			| display error
	lea	pc@(m_key),a6
	bsrs	puts			| wait for key
	movml	sp@+,d3-d7/a3-a5
	jmp	a5@

/*
 * puts (in: a6, d3)
 */
0:	cmpw	#35,d0		| '#'
	bnes	1f
	bsrs	puti
	bras	puts
1:	cmpw	#64,d0		| '@'
	bnes	2f
	movw	#2,sp@-
	movw	#Bconin,sp@-
	trap	#BIOS
	addql	#4,sp
	bras	puts
2:	bsrs	putc
puts:	movq	#0,d0
	movb	a6@+,d0
	bnes	0b
	rts

puti:	swap	d3
	bsrs	0f
	swap	d3
0:	rorw	#8,d3
	bsrs	1f
	rorw	#8,d3
1:	rorb	#4,d3
	bsrs	2f
	rorb	#4,d3
2:	movw	d3,d0
	andw	#15,d0
	addw	#48,d0
	cmpw	#58,d0
	bcss	putc
	addw	#39,d0
putc:	movw	d0,sp@-
	movw	#2,sp@-
	movw	#Bconout,sp@-
	trap	#BIOS
	addql	#6,sp
	rts
/*
 * int readsec (void *buffer, u_int offset, u_int count);
 */
rds2:	movml	d2-d7/a2-a6,sp@-
	movl	pc@(regsav),a0
	movml	a0@,d3-d7/a3-a5
	movl	sp@(48),a4		| buffer
	movl	sp@(52),d6		| offset
	movl	sp@(56),d3		| count
	bsrs	rds1
	movml	sp@+,d2-d7/a2-a6
	rts

rds1:	movl	#255,d5
	cmpl	d5,d3
	bccs	0f
	movl	d3,d5
0:	jsr	a3@
	tstl	d0
	bnes	1f
	addl	#(255*512),a4
	addl	d5,d6
	subl	d5,d3
	bnes	rds1
1:	rts

m_bot:	.asciz	"xxboot: membot == 0x#\r\n"
m_top:	.asciz	"xxboot: memtop == 0x#\r\n"
m_sbl:	.asciz	"xxboot: bootxx => 0x#\r\n"
m_rds:	.asciz	"xxboot: Floprd => 0x#\r\n"
m_key:	.asciz	"\007\r\npress any key... @\r\n"

regsav:	.long	0

fill:	.space	44		| 510-(fill-start)
	.word	0		| checksum
end:
