/*	$NetBSD: copy.s,v 1.16 1995/02/08 14:26:10 mycroft Exp $	*/

/*-
 * Copyright (c) 1994, 1995 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/errno.h>
#include <machine/asm.h>

#include "assym.s"

#ifdef sun3
#define	USER_SFC	moveq #FC_USERD,d1;	movec d1,sfc
#define	KERNEL_SFC	moveq #FC_CONTROL,d1;	movec d1,sfc
#define	USER_DFC	moveq #FC_USERD,d1;	movec d1,dfc
#define	KERNEL_DFC	moveq #FC_CONTROL,d1;	movec d1,dfc
#else
#define	USER_SFC
#define	KERNEL_SFC
#define	USER_DFC
#define	KERNEL_DFC
#endif

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 */
ENTRY(copyin)
	movl	sp@(12),d0	/* check count */
	beq	ciabort		/* <= 0, don't do anything */
#ifdef MAPPEDCOPY
	.globl	_mappedcopysize,_mappedcopyin
	cmpl	_mappedcopysize,d0	| size >= mappedcopysize
	bcc	_mappedcopyin		| yes, go do it the new way
#endif
	movl	d2,sp@-
	movl	_curpcb,a0	/* set fault handler */
	movl	#cifault,a0@(PCB_ONFAULT)
	movl	sp@(8),a0	/* src address */
	movl	sp@(12),a1	/* dest address */
	USER_SFC
	movl	a0,d1
	btst	#0,d1		/* src address odd? */
	beq	cieven		/* no, skip alignment */
	movsb	a0@+,d2		/* yes, copy a byte */
	movb	d2,a1@+
	subql	#1,d0		/* adjust count */
	beq	cidone		/* count 0, all done  */
cieven:
	movl	a1,d1
	btst	#0,d1		/* dest address odd? */
	bne	cibytes		/* yes, no hope for alignment, copy bytes */
	movl	d0,d1		/* no, both even */
	lsrl	#2,d1		/* convert count to longword count */
	beq	cibytes		/* count 0, skip longword loop */
	subql	#1,d1		/* predecrement for dbf */
cilloop:
	movsl	a0@+,d2		/* copy a longword */
	movl	d2,a1@+
	dbf	d1,cilloop	/* decrement low word of count */
	subil	#0x10000,d1	/* decrement high word of count */
	bcc	cilloop
	andl	#3,d0		/* what remains */
	beq	cidone		/* nothing, all done */
cibytes:
	subql	#1,d0		/* predecrement for dbf */
cibloop:
	movsb	a0@+,d2		/* copy a byte */
	movb	d2,a1@+
	dbf	d0,cibloop	/* decrement low word of count */
	subil	#0x10000,d0	/* decrement high word of count */
	bcc	cibloop
	clrl	d0		/* no error */
cidone:
	KERNEL_SFC
	movl	_curpcb,a0	/* clear fault handler */
	clrl	a0@(PCB_ONFAULT)
	movl	sp@+,d2
ciabort:
	rts
cifault:
	moveq	#EFAULT,d0
	jra	cidone

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 */
ENTRY(copyout)
	movl	sp@(12),d0	/* check count */
	beq	coabort		/* <= 0, don't do anything */
#ifdef MAPPEDCOPY
	.globl	_mappedcopysize,_mappedcopyout
	cmpl	_mappedcopysize,d0	| size >= mappedcopysize
	bcc	_mappedcopyout		| yes, go do it the new way
#endif
	movl	d2,sp@-
	movl	_curpcb,a0	/* set fault handler */
	movl	#cofault,a0@(PCB_ONFAULT)
	movl	sp@(8),a0	/* src address */
	movl	sp@(12),a1	/* dest address */
	USER_DFC
	movl	a0,d1
	btst	#0,d1		/* src address odd? */
	beq	coeven		/* no, skip alignment */
	movb	a0@+,d2		/* yes, copy a byte */
	movsb	d2,a1@+
	subql	#1,d0		/* adjust count */
	beq	codone		/* count 0, all done  */
coeven:
	movl	a1,d1
	btst	#0,d1		/* dest address odd? */
	bne	cobytes		/* yes, no hope for alignment, copy bytes */
	movl	d0,d1		/* no, both even */
	lsrl	#2,d1		/* convert count to longword count */
	beq	cobytes		/* count 0, skip longword loop */
	subql	#1,d1		/* predecrement for dbf */
colloop:
	movl	a0@+,d2		/* copy a longword */
	movsl	d2,a1@+
	dbf	d1,colloop	/* decrement low word of count */
	subil	#0x10000,d1	/* decrement high word of count */
	bcc	colloop
	andl	#3,d0		/* what remains */
	beq	codone		/* nothing, all done */
cobytes:
	subql	#1,d0		/* predecrement for dbf */
cobloop:
	movb	a0@+,d2		/* copy a byte */
	movsb	d2,a1@+
	dbf	d0,cobloop	/* decrement low word of count */
	subil	#0x10000,d0	/* decrement high word of count */
	bcc	cobloop
	clrl	d0		/* no error */
codone:
	KERNEL_DFC
	movl	_curpcb,a0	/* clear fault handler */
	clrl	a0@(PCB_ONFAULT)
	movl	sp@+,d2
coabort:
	rts
cofault:
	moveq	#EFAULT,d0
	jra	codone

ENTRY(copystr)
	movl	sp@(4),a0	/* a0 = fromaddr */
	movl	sp@(8),a1	/* a1 = toaddr */
	clrl	d0
	movl	sp@(12),d1	/* count */
	beq	csdone		/* nothing to do */
	subql	#1,d1		/* predecrement for dbeq */
csloop:
	movb	a0@+,a1@+	/* copy a byte */
	dbeq	d1,csloop	/* decrement low word of count */
	beq	csdone		/* copied null, exit */
	subil	#0x10000,d1	/* decrement high word of count */
	bcc	csloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
csdone:
	tstl	sp@(16)		/* length desired? */
	beq	csexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
csexit:
	rts

ENTRY(copyinstr)
	movl	_curpcb,a0	/* set fault handler */
	movl	#cisfault,a0@(PCB_ONFAULT)
	movl	sp@(4),a0	/* a0 = fromaddr */
	movl	sp@(8),a1	/* a1 = toaddr */
	USER_SFC
	clrl	d0
	movl	sp@(12),d1	/* count */
	beq	cisdone		/* nothing to do */
	subql	#1,d1		/* predecrement for dbeq */
cisloop:
	movsb	a0@+,d0		/* copy a byte */
	movb	d0,a1@+
	dbeq	d1,cisloop	/* decrement low word of count */
	beq	cisdone		/* copied null, exit */
	subil	#0x10000,d1	/* decrement high word of count */
	bcc	cisloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
cisdone:
	tstl	sp@(16)		/* length desired? */
	beq	cisexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
cisexit:
	KERNEL_SFC
	movl	_curpcb,a0	/* clear fault handler */
	clrl	a0@(PCB_ONFAULT)
	rts
cisfault:
	moveq	#EFAULT,d0
	jra	cisdone

ENTRY(copyoutstr)
	movl	_curpcb,a0	/* set fault handler */
	movl	#cosfault,a0@(PCB_ONFAULT)
	movl	sp@(4),a0	/* a0 = fromaddr */
	movl	sp@(8),a1	/* a1 = toaddr */
	USER_DFC
	clrl	d0
	movl	sp@(12),d1	/* count */
	beq	cosdone		/* nothing to do */
	subql	#1,d1		/* predecrement for dbeq */
cosloop:
	movb	a0@+,d0		/* copy a byte */
	movsb	d0,a1@+
	dbeq	d1,cosloop	/* decrement low word of count */
	beq	cosdone		/* copied null, exit */
	subil	#0x10000,d1	/* decrement high word of count */
	bcc	cosloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
cosdone:
	tstl	sp@(16)		/* length desired? */
	beq	cosexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
cosexit:
	KERNEL_DFC
	movl	_curpcb,a0	/* clear fault handler */
	clrl	a0@(PCB_ONFAULT)
	rts
cosfault:
	moveq	#EFAULT,d0
	jra	cosdone
