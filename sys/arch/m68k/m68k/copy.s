/*	$NetBSD: copy.s,v 1.43.18.1 2014/08/20 00:03:11 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
 * 3. Neither the name of the University nor the names of its contributors
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
 * This file contains the functions for user-space access:
 * copyin/copyout, fuword/suword, etc.
 */

#include "opt_multiprocessor.h"
#ifdef MULTIPROCESSOR
#error need to write MP support for ucas_* functions
#endif

#include <sys/errno.h>
#include <machine/asm.h>

#include "assym.h"

	.file	"copy.s"
	.text

#ifdef CI_CURPCB
#define	GETCURPCB(r)	movl	_C_LABEL(cpu_info_store)+CI_CURPCB,r
#else
#define	GETCURPCB(r)	movl	_C_LABEL(curpcb),r
#endif

#ifdef	DIAGNOSTIC
/*
 * The following routines all use the "moves" instruction to access
 * memory with "user" privilege while running in supervisor mode.
 * The "function code" registers actually determine what type of
 * access "moves" does, and the kernel arranges to leave them set
 * for "user data" access when these functions are called.
 *
 * The diagnostics:  CHECK_SFC,  CHECK_DFC
 * will verify that the sfc/dfc register values are correct.
 */
.Lbadfc:
	PANIC("copy.s: bad sfc or dfc")
	jra	.Lbadfc
#define	CHECK_SFC	movec %sfc,%d0; subql #FC_USERD,%d0; bne .Lbadfc
#define	CHECK_DFC	movec %dfc,%d0; subql #FC_USERD,%d0; bne .Lbadfc
#else	/* DIAGNOSTIC */
#define	CHECK_SFC
#define	CHECK_DFC
#endif	/* DIAGNOSTIC */

/*
 * copyin(void *from, void *to, size_t len);
 * Copy len bytes from the user's address space.
 *
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 */
ENTRY(copyin)
	CHECK_SFC
	movl	12(%sp),%d0		| check count
	jeq	.Lciret			| == 0, don't do anything
#ifdef MAPPEDCOPY
	cmpl	_C_LABEL(mappedcopysize),%d0 | size >= mappedcopysize
	jcc	_C_LABEL(mappedcopyin)	| yes, go do it the new way
#endif
	movl	%d2,-(%sp)		| save scratch register
	GETCURPCB(%a0)			| set fault handler
	movl	#.Lcifault,PCB_ONFAULT(%a0)
	movl	8(%sp),%a0		| src address
	movl	12(%sp),%a1		| dest address
	movl	%a0,%d1
	btst	#0,%d1			| src address odd?
	jeq	.Lcieven		| no, skip alignment
	movsb	(%a0)+,%d2		| yes, copy a byte
	movb	%d2,(%a1)+
	subql	#1,%d0			| adjust count
	jeq	.Lcidone		| count 0, all done
.Lcieven:
	movl	%a1,%d1
	btst	#0,%d1			| dest address odd?
	jne	.Lcibytes		| yes, must copy bytes
	movl	%d0,%d1			| OK, both even.  Get count
	lsrl	#2,%d1			|   and convert to longwords
	jeq	.Lcibytes		| count 0, skip longword loop
	subql	#1,%d1			| predecrement for dbf
.Lcilloop:
	movsl	(%a0)+,%d2		| copy a longword
	movl	%d2,(%a1)+
	dbf	%d1,.Lcilloop		| decrement low word of count
	subil	#0x10000,%d1		| decrement high word of count
	jcc	.Lcilloop
	andl	#3,%d0			| what remains
	jeq	.Lcidone		| nothing, all done
.Lcibytes:
	subql	#1,%d0			| predecrement for dbf
.Lcibloop:
	movsb	(%a0)+,%d2		| copy a byte
	movb	%d2,(%a1)+
	dbf	%d0,.Lcibloop		| decrement low word of count
	subil	#0x10000,%d0		| decrement high word of count
	jcc	.Lcibloop
	clrl	%d0			| no error
.Lcidone:
	GETCURPCB(%a0)			| clear fault handler
	clrl	PCB_ONFAULT(%a0)
	movl	(%sp)+,%d2		| restore scratch register
.Lciret:
	rts
.Lcifault:
	jra	.Lcidone

/*
 * copyout(void *from, void *to, size_t len);
 * Copy len bytes into the user's address space.
 *
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 */
ENTRY(copyout)
	CHECK_DFC
	movl	12(%sp),%d0		| check count
	jeq	.Lcoret			| == 0, don't do anything
#ifdef MAPPEDCOPY
	cmpl	_C_LABEL(mappedcopysize),%d0 | size >= mappedcopysize
	jcc	_C_LABEL(mappedcopyout)	| yes, go do it the new way
#endif
	movl	%d2,-(%sp)		| save scratch register
	GETCURPCB(%a0)			| set fault handler
	movl	#.Lcofault,PCB_ONFAULT(%a0)
	movl	8(%sp),%a0		| src address
	movl	12(%sp),%a1		| dest address
	movl	%a0,%d1
	btst	#0,%d1			| src address odd?
	jeq	.Lcoeven		| no, skip alignment
	movb	(%a0)+,%d2		| yes, copy a byte
	movsb	%d2,(%a1)+
	subql	#1,%d0			| adjust count
	jeq	.Lcodone		| count 0, all done
.Lcoeven:
	movl	%a1,%d1
	btst	#0,%d1			| dest address odd?
	jne	.Lcobytes		| yes, must copy bytes
	movl	%d0,%d1			| OK, both even.  Get count
	lsrl	#2,%d1			|   and convert to longwords
	jeq	.Lcobytes		| count 0, skip longword loop
	subql	#1,%d1			| predecrement for dbf
.Lcolloop:
	movl	(%a0)+,%d2		| copy a longword
	movsl	%d2,(%a1)+
	dbf	%d1,.Lcolloop		| decrement low word of count
	subil	#0x10000,%d1		| decrement high word of count
	jcc	.Lcolloop
	andl	#3,%d0			| what remains
	jeq	.Lcodone		| nothing, all done
.Lcobytes:
	subql	#1,%d0			| predecrement for dbf
.Lcobloop:
	movb	(%a0)+,%d2		| copy a byte
	movsb	%d2,(%a1)+
	dbf	%d0,.Lcobloop		| decrement low word of count
	subil	#0x10000,%d0		| decrement high word of count
	jcc	.Lcobloop
	clrl	%d0			| no error
.Lcodone:
	GETCURPCB(%a0)			| clear fault handler
	clrl	PCB_ONFAULT(%a0)
	movl	(%sp)+,%d2		| restore scratch register
.Lcoret:
	rts
.Lcofault:
	jra	.Lcodone

/*
 * copystr(void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
ENTRY(copystr)
	movl	4(%sp),%a0		| a0 = fromaddr
	movl	8(%sp),%a1		| a1 = toaddr
	clrl	%d0
	movl	12(%sp),%d1		| count
	jeq	.Lcstoolong		| nothing to copy
	subql	#1,%d1			| predecrement for dbeq
.Lcsloop:
	movb	(%a0)+,(%a1)+		| copy a byte
	dbeq	%d1,.Lcsloop		| decrement low word of count
	jeq	.Lcsdone		| copied null, exit
	subil	#0x10000,%d1		| decrement high word of count
	jcc	.Lcsloop		| more room, keep going
.Lcstoolong:
	moveq	#ENAMETOOLONG,%d0	| ran out of space
.Lcsdone:
	tstl	16(%sp)			| length desired?
	jeq	.Lcsret
	subl	4(%sp),%a0		| yes, calculate length copied
	movl	16(%sp),%a1		| store at return location
	movl	%a0,(%a1)
.Lcsret:
	rts

/*
 * copyinstr(void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
ENTRY(copyinstr)
	CHECK_SFC
	GETCURPCB(%a0)			| set fault handler
	movl	#.Lcisfault,PCB_ONFAULT(%a0)
	movl	4(%sp),%a0		| a0 = fromaddr
	movl	8(%sp),%a1		| a1 = toaddr
	clrl	%d0
	movl	12(%sp),%d1		| count
	jeq	.Lcistoolong		| nothing to copy
	subql	#1,%d1			| predecrement for dbeq
.Lcisloop:
	movsb	(%a0)+,%d0		| copy a byte
	movb	%d0,(%a1)+
	dbeq	%d1,.Lcisloop		| decrement low word of count
	jeq	.Lcisdone		| copied null, exit
	subil	#0x10000,%d1		| decrement high word of count
	jcc	.Lcisloop		| more room, keep going
.Lcistoolong:
	moveq	#ENAMETOOLONG,%d0	| ran out of space
.Lcisdone:
	tstl	16(%sp)		| length desired?
	jeq	.Lcisexit
	subl	4(%sp),%a0		| yes, calculate length copied
	movl	16(%sp),%a1		| store at return location
	movl	%a0,(%a1)
.Lcisexit:
	GETCURPCB(%a0)			| clear fault handler
	clrl	PCB_ONFAULT(%a0)
	rts
.Lcisfault:
	jra	.Lcisdone

/*
 * copyoutstr(void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
ENTRY(copyoutstr)
	CHECK_DFC
	GETCURPCB(%a0)			| set fault handler
	movl	#.Lcosfault,PCB_ONFAULT(%a0)
	movl	4(%sp),%a0		| a0 = fromaddr
	movl	8(%sp),%a1		| a1 = toaddr
	clrl	%d0
	movl	12(%sp),%d1		| count
	jeq	.Lcostoolong		| nothing to copy
	subql	#1,%d1			| predecrement for dbeq
.Lcosloop:
	movb	(%a0)+,%d0		| copy a byte
	movsb	%d0,(%a1)+
	dbeq	%d1,.Lcosloop		| decrement low word of count
	jeq	.Lcosdone		| copied null, exit
	subil	#0x10000,%d1		| decrement high word of count
	jcc	.Lcosloop		| more room, keep going
.Lcostoolong:
	moveq	#ENAMETOOLONG,%d0	| ran out of space
.Lcosdone:
	tstl	16(%sp)		| length desired?
	jeq	.Lcosexit
	subl	4(%sp),%a0		| yes, calculate length copied
	movl	16(%sp),%a1		| store at return location
	movl	%a0,(%a1)
.Lcosexit:
	GETCURPCB(%a0)			| clear fault handler
	clrl	PCB_ONFAULT(%a0)
	rts
.Lcosfault:
	jra	.Lcosdone

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
ENTRY(kcopy)
	link	%a6,#-4
	GETCURPCB(%a0)			| set fault handler
	movl	PCB_ONFAULT(%a0),-4(%a6) | save old handler first
	movl	#.Lkcfault,PCB_ONFAULT(%a0)
	movl	16(%a6),-(%sp)		| push len
	movl	8(%a6),-(%sp)		| push src
	movl	12(%a6),-(%sp)		| push dst
	jbsr	_C_LABEL(memcpy)	| copy it
	addl	#12,%sp			| pop args
	clrl	%d0			| success!
.Lkcdone:
	GETCURPCB(%a0)			| restore fault handler
	movl	-4(%a6),PCB_ONFAULT(%a0)
	unlk	%a6
	rts
.Lkcfault:
	addl	#16,%sp			| pop args and return address
	jra	.Lkcdone

/*
 * fuword(void *uaddr);
 * Fetch an int from the user's address space.
 */
ENTRY(fuword)
	CHECK_SFC
	movl	4(%sp),%a0		| address to read
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lferr,PCB_ONFAULT(%a1)
	movsl	(%a0),%d0		| do read from user space
	jra	.Lfdone

/*
 * fusword(void *uaddr);
 * Fetch a short from the user's address space.
 */
ENTRY(fusword)
	CHECK_SFC
	movl	4(%sp),%a0		| address to read
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lferr,PCB_ONFAULT(%a1)
	moveq	#0,%d0
	movsw	(%a0),%d0		| do read from user space
	jra	.Lfdone

/*
 * fuswintr(void *uaddr);
 * Fetch a short from the user's address space.
 * Can be called during an interrupt.
 */
ENTRY(fuswintr)
	CHECK_SFC
	movl	4(%sp),%a0		| address to read
	GETCURPCB(%a1)			| set fault handler
	movl	#_C_LABEL(fubail),PCB_ONFAULT(%a1)
	moveq	#0,%d0
	movsw	(%a0),%d0		| do read from user space
	jra	.Lfdone

/*
 * fubyte(void *uaddr);
 * Fetch a byte from the user's address space.
 */
ENTRY(fubyte)
	CHECK_SFC
	movl	4(%sp),%a0		| address to read
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lferr,PCB_ONFAULT(%a1)
	moveq	#0,%d0
	movsb	(%a0),%d0		| do read from user space
	jra	.Lfdone

/*
 * Error routine for fuswintr.  The fault handler in trap.c
 * checks for pcb_onfault set to this fault handler and
 * "bails out" before calling the VM fault handler.
 * (We can not call VM code from interrupt level.)
 * Same code as Lferr but must have a different address.
 */
ENTRY(fubail)
	nop
.Lferr:
	moveq	#-1,%d0			| error indicator
.Lfdone:
	clrl	PCB_ONFAULT(%a1) 	| clear fault handler
	rts

/*
 * suword(void *uaddr, int x);
 * Store an int in the user's address space.
 */
ENTRY(suword)
	CHECK_DFC
	movl	4(%sp),%a0		| address to write
	movl	8(%sp),%d0		| value to put there
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lserr,PCB_ONFAULT(%a1)
	movsl	%d0,(%a0)		| do write to user space
	moveq	#0,%d0			| indicate no fault
	jra	.Lsdone

/*
 * susword(void *uaddr, short x);
 * Store a short in the user's address space.
 */
ENTRY(susword)
	CHECK_DFC
	movl	4(%sp),%a0		| address to write
	movw	10(%sp),%d0		| value to put there
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lserr,PCB_ONFAULT(%a1)
	movsw	%d0,(%a0)		| do write to user space
	moveq	#0,%d0			| indicate no fault
	jra	.Lsdone

/*
 * suswintr(void *uaddr, short x);
 * Store a short in the user's address space.
 * Can be called during an interrupt.
 */
ENTRY(suswintr)
	CHECK_DFC
	movl	4(%sp),%a0		| address to write
	movw	10(%sp),%d0		| value to put there
	GETCURPCB(%a1)			| set fault handler
	movl	#_C_LABEL(subail),PCB_ONFAULT(%a1)
	movsw	%d0,(%a0)		| do write to user space
	moveq	#0,%d0			| indicate no fault
	jra	.Lsdone

/*
 * subyte(void *uaddr, char x);
 * Store a byte in the user's address space.
 */
ENTRY(subyte)
	CHECK_DFC
	movl	4(%sp),%a0		| address to write
	movb	11(%sp),%d0		| value to put there
	GETCURPCB(%a1)			| set fault handler
	movl	#.Lserr,PCB_ONFAULT(%a1)
	movsb	%d0,(%a0)		| do write to user space
	moveq	#0,%d0			| indicate no fault
	jra	.Lsdone

/*
 * Error routine for suswintr.  The fault handler in trap.c
 * checks for pcb_onfault set to this fault handler and
 * "bails out" before calling the VM fault handler.
 * (We can not call VM code from interrupt level.)
 * Same code as Lserr but must have a different address.
 */
ENTRY(subail)
	nop
.Lserr:
	moveq	#-1,%d0			| error indicator
.Lsdone:
	clrl	PCB_ONFAULT(%a1) 	| clear fault handler
	rts

/*
 * int ucas_32(volatile int32_t *uptr, int32_t old, int32_t new, int32_t *ret);
 * Atomically compare-and-swap an int32_t in user space.
 */
	.globl		_C_LABEL(ucas_32_ras_start)
	.globl		_C_LABEL(ucas_32_ras_end)
ENTRY(ucas_32)
	CHECK_SFC
	CHECK_DFC
	GETCURPCB(%a1)
	movl	#.Lucasfault,PCB_ONFAULT(%a1)	| set fault handler
	movl	4(%sp),%a0		| a0 = uptr
_C_LABEL(ucas_32_ras_start):
	movl	8(%sp),%d0		| d0 = old
	movsl	(%a0),%d1		| d1 = *uptr
	cmpl	%d0,%d1			| does *uptr == old?
	jne	.Lucasdiff		| if not, don't change it
	movl	12(%sp),%d0		| d0 = new
	movsl	%d0,(%a0)		| *uptr = new
	nop				| pipeline sync
_C_LABEL(ucas_32_ras_end):
.Lucasdiff:
	movl	16(%sp),%a0		| a0 = ret
	movl	%d1,(%a0)		| *ret = d1 (old *uptr)
	clrl	%d0			| return 0

.Lucasfault:
	clrl	PCB_ONFAULT(%a1)	| clear fault handler
	rts

STRONG_ALIAS(ucas_int,ucas_32)
STRONG_ALIAS(ucas_ptr,ucas_32)
