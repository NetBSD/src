/*-
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

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 *	- use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(copyin)
	movl	sp@(12),d0	/* check count */
	jle	ciabort		/* <= 0, don't do anything */
#ifdef MAPPEDCOPY
	.globl	_mappedcopysize,_mappedcopyin
	cmpl	_mappedcopysize,d0	| size >= mappedcopysize
	jcc	_mappedcopyin		| yes, go do it the new way
#endif
	movl	d2,sp@-
	movl	_curpcb,a0	/* set fault handler */
	movl	#cifault,a0@(PCB_ONFAULT)
	movl	sp@(8),a0	/* src address */
	movl	sp@(12),a1	/* dest address */
	movl	a0,d1
	btst	#0,d1		/* src address odd? */
	jeq	cieven		/* no, skip alignment */
	movsb	a0@+,d2		/* yes, copy a byte */
	movb	d2,a1@+
	subql	#1,d0		/* adjust count */
	jeq	cidone		/* count 0, all done  */
cieven:
	movl	a1,d1
	btst	#0,d1		/* dest address odd? */
	jne	cibloop		/* yes, no hope for alignment, copy bytes */
	movl	d0,d1		/* no, both even */
	lsrl	#2,d1		/* convert count to longword count */
	jeq	cibloop		/* count 0, skip longword loop */
cilloop:
	movsl	a0@+,d2		/* copy a longword */
	movl	d2,a1@+
	subql	#1,d1		/* adjust count */
	jne	cilloop		/* still more, keep copying */
	andl	#3,d0		/* what remains */
	jeq	cidone		/* nothing, all done */
cibloop:
	movsb	a0@+,d2		/* copy a byte */
	movb	d2,a1@+
	subql	#1,d0		/* adjust count */
	jne	cibloop		/* still more, keep going */
cidone:
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
 *	- use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(copyout)
	movl	sp@(12),d0	/* check count */
	jle	coabort		/* <= 0, don't do anything */
#ifdef MAPPEDCOPY
	.globl	_mappedcopysize,_mappedcopyout
	cmpl	_mappedcopysize,d0	| size >= mappedcopysize
	jcc	_mappedcopyout		| yes, go do it the new way
#endif
	movl	d2,sp@-
	movl	_curpcb,a0	/* set fault handler */
	movl	#cofault,a0@(PCB_ONFAULT)
	movl	sp@(8),a0	/* src address */
	movl	sp@(12),a1	/* dest address */
	movl	a0,d1
	btst	#0,d1		/* src address odd? */
	jeq	coeven		/* no, skip alignment */
	movb	a0@+,d2		/* yes, copy a byte */
	movsb	d2,a1@+
	subql	#1,d0		/* adjust count */
	jeq	codone		/* count 0, all done  */
coeven:
	movl	a1,d1
	btst	#0,d1		/* dest address odd? */
	jne	cobloop		/* yes, no hope for alignment, copy bytes */
	movl	d0,d1		/* no, both even */
	lsrl	#2,d1		/* convert count to longword count */
	jeq	cobloop		/* count 0, skip longword loop */
colloop:
	movl	a0@+,d2		/* copy a longword */
	movsl	d2,a1@+
	subql	#1,d1		/* adjust count */
	jne	colloop		/* still more, keep copying */
	andl	#3,d0		/* what remains */
	jeq	codone		/* nothing, all done */
cobloop:
	movb	a0@+,d2		/* copy a byte */
	movsb	d2,a1@+
	subql	#1,d0		/* adjust count */
	jne	cobloop		/* still more, keep going */
codone:
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
	jeq	csdone		/* nothing to do */
csloop:
	movb	a0@+,a1@+	/* copy a byte */
	jeq	csdone		/* copied null, exit */
	subql	#1,d1		/* adjust count */
	jne	csloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
csdone:
	tstl	sp@(16)		/* length desired? */
	jeq	csexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
csexit:
	rts
csfault:
	moveq	#EFAULT,d0
	jra	csdone

ENTRY(copyinstr)
	movl	_curpcb,a0	/* set fault handler */
	movl	#cisfault,a0@(PCB_ONFAULT)
	movl	sp@(4),a0	/* a0 = fromaddr */
	movl	sp@(8),a1	/* a1 = toaddr */
	clrl	d0
	movl	sp@(12),d1	/* count */
	jeq	cisdone		/* nothing to do */
cisloop:
	movsb	a0@+,d0		/* copy a byte */
	movb	d0,a1@+
	jeq	cisdone		/* copied null, exit */
	subql	#1,d1		/* adjust count */
	jne	cisloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
cisdone:
	tstl	sp@(16)		/* length desired? */
	jeq	cisexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
cisexit:
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
	clrl	d0
	movl	sp@(12),d1	/* count */
	jeq	cosdone		/* nothing to do */
cosloop:
	movb	a0@+,d0		/* copy a byte */
	movsb	d0,a1@+
	jeq	cosdone		/* copied null, exit */
	subql	#1,d1		/* adjust count */
	jne	cosloop		/* more room, keep going */
	moveq	#ENAMETOOLONG,d0 /* ran out of space */
cosdone:
	tstl	sp@(16)		/* length desired? */
	jeq	cosexit
	subl	sp@(4),a0	/* yes, calculate length copied */
	movl	sp@(16),a1	/* return location */
	movl	a0,a1@
cosexit:
	movl	_curpcb,a0	/* clear fault handler */
	clrl	a0@(PCB_ONFAULT)
	rts
cosfault:
	moveq	#EFAULT,d0
	jra	cosdone
