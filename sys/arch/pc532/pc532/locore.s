/*	$NetBSD: locore.s,v 1.60.2.1 2000/06/22 17:02:10 minoura Exp $	*/

/*
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * locore.s  - -  assembler routines needed for BSD stuff.  (ns32532/pc532)
 *
 * Phil Nelson, Dec 6, 1992
 *
 * Modified by Matthias Pfaller, Jan 1996.
 *
 */

#include "opt_ddb.h"

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/trap.h>
#include <machine/cpufunc.h>
#include <machine/jmpbuf.h>

#define	SET(a, b)	.globl	_C_LABEL(a); .set _C_LABEL(a),b

/*
 * kernel_text is used by libkvm.
 */
	SET(kernel_text,(KERNBASE + 0x2000))

/*
 * Initialization
 */

	DATA_D(esym,		0)	/* pointer to end of symbols */
	DATA_D(_have_fpu,	0)	/* Have we an FPU installed? */

ASENTRY(start)
	ints_off			/* make sure interrupts are off. */
	bicpsrw	PSL_US			/* make sure we are using sp0. */
	lprd    sb,0			/* gcc expects this. */

	/*
	 * Zero the bss segment.
	 */
	addr	_C_LABEL(end)(pc),r0	/* setup to zero the bss segment. */
	addr	_C_LABEL(edata)(pc),r1
	subd	r1,r0			/* compute _end - _edata */
	movd	r0,tos			/* push length */
	movqd	0,tos			/* push zero */
	addr	r1,tos			/* push address */
	bsr	_C_LABEL(memset)		/* zero the bss segment */

	/*
	 * The boot program provides us a magic in r3,
	 * esym in r4, bootdev in r6 and boothowto in r7.
	 * Set the kernel variables if the magic matches.
	 */
	cmpd	0xc1e86394,r3
	bne	1f
	movd	r4,_C_LABEL(esym)(pc)
	movd	r6,_C_LABEL(bootdev)(pc)
	movd	r7,_C_LABEL(boothowto)(pc)
1:	/*
	 * Finish machine initialization and start main.
	 */
	br	_C_LABEL(init532)

ENTRY_NOPROFILE(proc_trampoline)
#if !defined(MRTD)
	movd	r4,0(sp)	/* There was curproc on the stack */
	jsr	0(r3)
	cmpqd	0,tos
	br	rei
#else
	movd	r4,tos
	jsr	0(r3)
	br	rei
#endif

/****************************************************************************/

/*
 * Burn N microseconds in a delay loop.
 */
KENTRY(delay, 4)		/* bsr  2 cycles;  80 ns */
	cmpqd	0,S_ARG0	/*	2 cycles;  80 ns */
	beq	2f		/*	2 cycles;  80 ns */
	nop; nop; nop; nop	/*	8 cycles; 320 ns */
#ifdef CPU30MHZ
	nop; nop
#endif
	movd	S_ARG0,r0	/* 	2 cycles;  80 ns */
	acbd	-1,r0,1f	/*      5 cycles; 200 ns */
				/*                ====== */
				/*                840 ns */
	ret	ARGS		/*	4 cycles; 160 ns */
				/*                ====== */
				/*		 1000 ns */
				/*		  840 ns */
1:	nop; nop; nop; nop; nop	/*     10 cycles; 400 ns */
	nop; nop; nop; nop; nop	/*     10 cycles; 400 ns */
#ifdef CPU30MHZ
	nop; nop
#endif
	acbd	-1,r0,1b	/* 	5 cycles; 200 ns */
2:	ret	ARGS		/* 	4 cycles; 160 ns */

/****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */

ENTRY_NOPROFILE(sigcode)
	jsr	0(SIGF_HANDLER(sp))
	addr	SIGF_SC(sp),tos		/* scp (the call may have clobbered */
					/* the copy at SIGF_SCP(sp)). */
	movqd	0,tos			/* Push a fake return address. */
	movd	SYS___sigreturn14,r0
	svc
	movd	0,0			/* Illegal instruction. */
GLOBAL(esigcode)

#if defined(PROF) || defined(GPROF) || defined(KGDB) || defined(DDB)
	/* Just a gap between esigcode and the next label */
	.long	0
#endif

/****************************************************************************/

/*
 * The following primitives are used to copy data in and out of the user's
 * address space.
 */

/*
 * int copyout(caddr_t from, caddr_t to, size_t len);
 *
 * Copy len bytes into the user's address space.
 */
KENTRY(copyout, 12)
	enter	[r3,r4],0
	movd	_C_LABEL(curpcb)(pc),r4
	addr	_ASM_LABEL(copy_fault)(pc),PCB_ONFAULT(r4)

	movd	B_ARG0,r1		/* from */
	movd	B_ARG1,r2		/* to */
	movd	B_ARG2,r0		/* len */
	cmpqd	0,r0
	beq	9f			/* anything to do? */

	/*
	 * We check that each page of the destination buffer is writable
	 * with one movsub per page.
	 */
	/* Compute number of pages. */
	movd	r2,r3
	andd	PGOFSET,r3
	addd	r0,r3
	addqd	-1,r3
	lshd	-PGSHIFT,r3

	/* Do an user-write-access for first page. */
	movsub	0(r1),0(r2)

	/* More to do? */
	cmpqd	0,r3
	beq	2f

	/* Bump address to start of next page. */
	addd	NBPG,r2
	andd	~PGOFSET,r2

	/* Do an user-write-acess for all remaining pages. */
1:	movsub	0(r1),0(r2)
	addd	NBPG,r2
	acbd	-1,r3,1b

	/* Reload to argument. */
	movd	B_ARG1,r2

2:	/* And now do the copy. */
	lshd	-2,r0
	movsd
	movd	B_ARG2,r0
	andd	3,r0
	movsb				/* This also sets r0 to zero. */
9:	movd	r0,PCB_ONFAULT(r4)
	exit	[r3,r4]
	ret	ARGS

/*
 * int copyin(caddr_t from, caddr_t to, size_t len);
 *
 * Copy len bytes from the user's address space.
 */
KENTRY(copyin, 12)
	enter	[r3,r4],0
	movd	_C_LABEL(curpcb)(pc),r4
	addr	_ASM_LABEL(copy_fault)(pc),PCB_ONFAULT(r4)

	movd	B_ARG0,r1		/* from */
	movd	B_ARG1,r2		/* to */
	movd	B_ARG2,r0		/* len */
	cmpqd	0,r0
	beq	9f			/* anything to do? */

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is readable, and the CPU will do that for us.
	 */
	movd	r1,r3
	addd	r0,r3
	cmpd	r3,VM_MAXUSER_ADDRESS
	bhi	_ASM_LABEL(copy_fault)
	cmpd	r1,r3
	bhs	_ASM_LABEL(copy_fault)	/* check for overflow. */

	/* And now do the copy. */
	lshd	-2,r0
	movsd
1:	movd	B_ARG2,r0
	andd	3,r0
	movsb				/* This also sets r0 to zero. */
9:	movd	r0,PCB_ONFAULT(r4)
	exit	[r3,r4]
	ret	ARGS

ASLOCAL(copy_fault)
	movqd	0,PCB_ONFAULT(r4)
	movd	EFAULT,r0
	exit	[r3,r4]
	/*
	 * copyin and copyout have the same number of arguments.
	 */
	ret	ARGS

/*
 * int copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 *
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
KENTRY(copyoutstr, 16)
	enter	[r3],0
	movd	_C_LABEL(curpcb)(pc),r3
	addr	_ASM_LABEL(copystr_fault)(pc),PCB_ONFAULT(r3)
	movd	B_ARG0,r0		/* from */
	movd	B_ARG1,r1		/* to */
	movd	B_ARG2,r2		/* maxlen */
	cmpqd	0,r2
	beq	2f			/* anything to do? */

1:	movsub	0(r0),0(r1)
	cmpqb	0,0(r0)
	beq	3f
	addqd	1,r0
	addqd	1,r1
	acbd	-1,r2,1b
2:	movd	ENAMETOOLONG,r0
	br	_ASM_LABEL(copystr_return)
3:	addqd	-1,r2
	movqd	0,r0
	br	_ASM_LABEL(copystr_return)

/*
 * int copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 *
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
KENTRY(copyinstr, 16)
	enter	[r3],0
	movd	_C_LABEL(curpcb)(pc),r3
	addr	_ASM_LABEL(copystr_fault)(pc),PCB_ONFAULT(r3)
	movd	B_ARG0,r0		/* from */
	movd	B_ARG1,r1		/* to */
	movd	B_ARG2,r2		/* maxlen */
	cmpqd	0,r2
	beq	2f			/* anything to do? */

1:	movusb	0(r0),0(r1)
	cmpqb	0,0(r1)
	beq	3f
	addqd	1,r0
	addqd	1,r1
	acbd	-1,r2,1b
2:	movd	ENAMETOOLONG,r0
	br	_ASM_LABEL(copystr_return)
3:	addqd	-1,r2
	movqd	0,r0
	br	_ASM_LABEL(copystr_return)

ASLOCAL(copystr_fault)
	movd	EFAULT,r0

ASLOCAL(copystr_return)
	/* Set *lencopied and return r0. */
	movqd	0,PCB_ONFAULT(r3)
	movd	B_ARG2,r1
	subd	r2,r1
	movd	B_ARG3,r2
	cmpqd	0,r2
	beq	1f
	movd	r1,0(r2)
1:	exit	[r3]
	/*
	 * copyinstr and copyoutstr have the same number of arguments.
	 */
	ret	ARGS

/*
 * int copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 *
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
KENTRY(copystr, 16)
	enter	[r4],0
	movd	B_ARG0,r1		/* from */
	movd	B_ARG1,r2		/* to */
	movd	B_ARG2,r0		/* maxlen */
	cmpqd	0,r0
	beq	0f			/* anything to do? */

	movqd	0,r4			/* Set match value. */
	movsb	u
	movd	r0,r1			/* Save count. */
	bfs	1f

	/*
	 * Terminated due to limit count.
	 * Return ENAMETOOLONG. 
	 */
0:	movd	ENAMETOOLONG,r0
	br	2f

1:	/*
	 * Terminated due to match. Adjust 
	 * count and transfer final element.
	 */
	addqd	-1,r1
	movqd	0,r0
	movb	r0,0(r2)

2:	/* Set *lencopied and return r0. */
	movd	B_ARG2,r2
	subd	r1,r2
	movd	B_ARG3,r1
	cmpqd	0,r1
	beq	3f
	movd	r2,0(r1)
3:	exit	[r4]
	ret	ARGS

/*
 * int kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 *
 * We can't use memcpy because the state of the stack at fault time must
 * be known. So we duplicate the memcpy code here. Sigh.
 */

KENTRY(kcopyret, 12)
0:	movd	_C_LABEL(curpcb)(pc),r4
	movd	tos,PCB_ONFAULT(r4)
	movd	EFAULT,r0
	exit	[r3,r4]
	ret	ARGS

KENTRY(kcopy, 12)
	enter	[r3,r4],0
	movd	_C_LABEL(curpcb)(pc),r4
	movd	PCB_ONFAULT(r4),tos
	addr	0b(pc),PCB_ONFAULT(r4)

	movd	B_ARG2,r0
	movd	B_ARG0,r1
	movd	B_ARG1,r2
	cmpd	r2,r1
	bls	0f
	movd	r1,r3
	addd	r0,r3
	cmpd	r2,r3
	bls	2f
0:	cmpqd	4,r0
	bhi	1f

	/*
	 * Align destination address.
	 */
	movd	3,r3
	andd	r2,r3
	movd	0(r1),0(r2)
	negd	r3,r3
	addqd	4,r3
	addd	r3,r1
	addd	r3,r2
	subd	r3,r0

	movqd	3,r3
	andd	r0,r3
	lshd	-2,r0
	movsd

	movd	r3,r0
1:	movsb
	movd	tos,PCB_ONFAULT(r4)
	movqd	0,r0
	exit	[r3,r4]
	ret	ARGS

2:	addd	r0,r1
	addd	r0,r2
	addqd	-1,r1
	addqd	-1,r2
	cmpqd	4,r0
	bhi	0f

	/*
	 * Align destination address.
	 */
	movd	r0,r3
	movqd	1,r0
	addd	r2,r0
	andd	3,r0
	subd	r0,r3
	movsb	b
	movd	r3,r0
	andd	3,r3

	addqd	-3,r1
	addqd	-3,r2
	lshd	-2,r0
	movsd	b

	movd	r3,r0
	addqd	3,r1
	addqd	3,r2
0:	movsb	b
	movd	tos,PCB_ONFAULT(r4)
	movqd	0,r0
	exit	[r3,r4]
	ret	ARGS

/*
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */
KENTRY(fuword, 4)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(fufault)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	/*
	 * MACH's locore.s code says that
	 * due to cpu bugs the destination
	 * of movusi can't be a register or tos.
	 */
	movusd	0(r0),S_ARG0
	movd	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 */
KENTRY(fuswintr, 4)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(fubail)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movusw	0(r0),S_ARG0
	movqd	0,r0
	movw	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
KENTRY(fubyte, 4)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(fufault)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movusb	0(r0),S_ARG0
	movqd	0,r0
	movb	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
KENTRY(suword, 8)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(sufault)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsud	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 */
KENTRY(suswintr, 8)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(subail)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsuw	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */
KENTRY(subyte, 8)
	movd	_C_LABEL(curpcb)(pc),r2
	addr	_C_LABEL(sufault)(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsub	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	ARGS

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
KENTRY(fufault, 4)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	ARGS

KENTRY(sufault, 8)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	ARGS

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
KENTRY(fubail, 4)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	ARGS

KENTRY(subail, 8)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	ARGS

/***************************************************************************/

/*
 * setjmp(label_t *);
 * longjmp(label_t *);
 *
 * The kernel versions of setjmp and longjmp.
 * r0-r2 and sb are not saved.
 */

KENTRY(setjmp, 4)
	movd	S_ARG0,r0

	sprd	sp,0(r0)		/* save stackpointer */
	sprd	fp,4(r0)		/* save framepointer */
	movd	0(sp),8(r0)		/* save return address */

	movd	r3,12(r0)		/* save registers r3-r7 */
	movd	r4,16(r0)
	movd	r5,20(r0)
	movd	r6,24(r0)
	movd	r7,28(r0)

	movqd	0,r0			/* return(0) */
	ret	ARGS

KENTRY(longjmp, 4)
	movd	S_ARG0,r0

	lprd	sp,0(r0)		/* restore stackpointer */
	lprd	fp,4(r0)		/* restore framepointer */
	movd	8(r0),0(sp)		/* modify return address */

	movd	12(r0),r3		/* restore registers r3-r7 */
	movd	16(r0),r4
	movd	20(r0),r5
	movd	24(r0),r6
	movd	28(r0),r7

	movqd	1,r0			/* return(1) */
	ret	ARGS

/****************************************************************************/

/*
 * The following primitives manipulate the run queues.
 * whichqs tells which of the 32 queues qs have processes
 * in them.  Setrunqueue puts processes into queues, remrunqueue
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */

/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
KENTRY(setrunqueue, 4)
	movd	S_ARG0,r0
#ifdef DIAGNOSTIC
	cmpqd	0,P_BACK(r0)		/* should not be on q already */
	bne	1f
	cmpqd	0,P_WCHAN(r0)
	bne	1f
	cmpb	SRUN,P_STAT(r0)
	bne	1f
#endif
	movzbd	P_PRIORITY(r0),r1
	lshd	-2,r1
	sbitd	r1,_C_LABEL(sched_whichqs)(pc) /* set queue full bit */
	addr	_C_LABEL(sched_qs)(pc)[r1:q],r1 /* locate q hdr */
	movd	P_BACK(r1),r2		/* locate q tail */
	movd	r1,P_FORW(r0)		/* set p->p_forw */
	movd	r0,P_BACK(r1)		/* update q's p_back */
	movd	r0,P_FORW(r2)		/* update tail's p_forw */
	movd    r2,P_BACK(r0)		/* set p->p_back */
	ret	ARGS
#ifdef DIAGNOSTIC
1:	PANIC("setrunqueue")		/* Was on the list! */
#endif

/*
 * remrunqueue(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
KENTRY(remrunqueue, 4)
	movd	S_ARG0,r1
	movzbd	P_PRIORITY(r1),r0
#ifdef DIAGNOSTIC
	lshd	-2,r0
	tbitd	r0,_C_LABEL(sched_whichqs)(pc)
	bfc	1f
#endif
	movd	P_BACK(r1),r2		/* Address of prev. item */
	movqd	0,P_BACK(r1)		/* Clear reverse link */
	movd	P_FORW(r1),r1		/* Addr of next item. */
	movd	r1,P_FORW(r2)		/* Unlink item. */
	movd	r2,P_BACK(r1)
	cmpd	r1,r2			/* r1 = r2 => empty queue */
	bne	2f
#ifndef DIAGNOSTIC
	lshd	-2,r0
#endif
	cbitd	r0,_C_LABEL(sched_whichqs)(pc) /* mark q as empty */
2:	ret	ARGS
#ifdef DIAGNOSTIC
1:	PANIC("remrunqueue")		/* No queue entry! */
#endif

/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 * Note1: this is really a part of cpu_switch() but defined here for kernel
 *        profiling.
 * Note2: cpu_switch jumps into the idle loop with "bfs 0b". This makes it
 *        possible to insert a fake "enter" after the _idle label for the
 *        benefit of kgdb and ddb.
 */
ENTRY_NOPROFILE(idle)
#if defined(DDB) || defined(KGDB)
	enter	[r3,r4,r5,r6,r7],0
#endif
0:	ints_off
	ffsd	_C_LABEL(sched_whichqs)(pc),r0
	bfc	sw1
	ints_on				/* We may lose a tick here ... */
	wait				/* Wait for interrupt. */
	br	0b

/*
 * void cpu_switch(struct proc *)
 * Find a runnable process and switch to it.  Wait if necessary.
 */
KENTRY(cpu_switch, 4)
	enter	[r3,r4,r5,r6,r7],0

	movd	_C_LABEL(curproc)(pc),r4

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spl0()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movqd	0,_C_LABEL(curproc)(pc)

	movd	_C_LABEL(imask)(pc),tos
	bsr	_C_LABEL(splx)		/* spl0 - process pending interrupts */
#if !defined(MRTD)
# if defined(DDB) || defined(KGDB)
	cmpqd	0,tos
	movd	r0,tos
# else
	movd	r0,0(sp)
# endif
#else
	movd	r0,tos
#endif

	/*
	 * First phase: find new process.
	 *
	 * Registers:
	 *   r0 - queue number
	 *   r1 - queue head
	 *   r2 - new process
	 *   r3 - next process in queue
	 *   r4 - old process
	 */
	ints_off

	movqd	0,r0
	ffsd	_C_LABEL(sched_whichqs)(pc),r0 /* find a full q */
	bfs	0b			/* if none, idle */

sw1:	/* Get the process and unlink it from the queue. */
	addr	_C_LABEL(sched_qs)(pc)[r0:q],r1 /* address of qs entry! */

	movd	P_FORW(r1),r2		/* unlink from front of process q */
#ifdef	DIAGNOSTIC
	cmpd	r2,r1			/* linked to self (i.e. nothing queued? */
	beq	_C_LABEL(switch_error)	/* not possible */
#endif
	movd	P_FORW(r2),r3
	movd	r3,P_FORW(r1)
	movd	r1,P_BACK(r3)

	cmpd	r1,r3			/* q empty? */
	bne	3f

	cbitd	r0,_C_LABEL(sched_whichqs)(pc) /* queue is empty, turn off whichqs. */

3:	movqd	0,_C_LABEL(want_resched)(pc) /* We did a resched! */

#ifdef	DIAGNOSTIC
	cmpqd	0,P_WCHAN(r2)		/* Waiting for something? */
	bne	_C_LABEL(switch_error)	/* Yes; shouldn't be queued. */
	cmpb	SRUN,P_STAT(r2)		/* In run state? */
	bne	_C_LABEL(switch_error)	/* No; shouldn't be queued. */
#endif

	/* Isolate process. XXX Is this necessary? */
	movqd	0,P_BACK(r2)

	/* p->p_cpu initialized in fork1() for single-processor */

	/* Record new process. */
	movb	SONPROC,P_STAT(r2)	/* p->p_stat = SONPROC */
	movd	r2,_C_LABEL(curproc)(pc)

	/* It's okay to take interrupts here. */
	ints_on

	/* Skip context switch if same process. */
	cmpd	r2,r4
	beq	_ASM_LABEL(switch_return)

	/* If old process exited, don't bother. */
	cmpqd	0,r4
	beq	_ASM_LABEL(switch_exited)

	/*
	 * Second phase: save old context.
	 *
	 * Registers:
	 *   r4 - old process, then old pcb
	 *   r2 - new process
	 */

	movd	P_ADDR(r4),r4

	/* save stack and frame pointer registers. */
	sprd	sp,PCB_KSP(r4)
	sprd	fp,PCB_KFP(r4)

ASLOCAL(switch_exited)
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   r1 - new pcb
	 *   r2 - new process
	 */

	/* No interrupts while loading new state. */
	ints_off
	movd	P_ADDR(r2),r1

	/* Switch address space. */
	lmr	ptb0,PCB_PTB(r1)
	lmr	ptb1,PCB_PTB(r1)

	/* Restore stack and frame pointer registers. */
	lprd	sp,PCB_KSP(r1)
	lprd	fp,PCB_KFP(r1)

	/* Record new pcb. */
	movd	r1,_C_LABEL(curpcb)(pc)

	/*
	 * Disable the FPU.
	 */
	sprw	cfg,r0
	andw	~CFG_F,r0
	lprw	cfg,r0

	/* Interrupts are okay again. */
	ints_on

switch_return:
	/*
	 * Restore old priority level from stack.
	 */
	bsr	_C_LABEL(splx)
#if !defined(MRTD)
	cmpqd	0,tos
#endif

	exit	[r3,r4,r5,r6,r7]
	ret	ARGS

#ifdef	DIAGNOSTIC
ENTRY(switch_error)
	PANIC("cpu_switch")
#endif

/****************************************************************************/

/*
 * FPU handling.
 * Normally the FPU state is saved and restored in trap.
 */

/*
 * Check if we have a FPU installed.
 */
#ifdef NS381
# define FPU_INSTALLED
#else
# define FPU_INSTALLED	cmpqd 0,_C_LABEL(_have_fpu)(pc); beq 9f
#endif

/*
 * void save_fpu_context(struct pcb *p);
 * Save FPU context.
 */
KENTRY(save_fpu_context, 4)
	FPU_INSTALLED
	movd	S_ARG0,r0
	sprw	cfg,r1
	orw	CFG_F,r1
	lprw	cfg,r1
	sfsr	PCB_FSR(r0)
	movl	f0,PCB_F0(r0)
	movl	f1,PCB_F1(r0)
	movl	f2,PCB_F2(r0)
	movl	f3,PCB_F3(r0)
	movl	f4,PCB_F4(r0)
	movl	f5,PCB_F5(r0)
	movl	f6,PCB_F6(r0)
	movl	f7,PCB_F7(r0)
9:	ret	ARGS

/*
 * void restore_fpu_context(struct pcb *p);
 * Restore FPU context.
 */
KENTRY(restore_fpu_context, 4)
	FPU_INSTALLED
	movd	S_ARG0,r0
	sprw	cfg,r1
	orw	CFG_F,r1
	lprw	cfg,r1
	lfsr	PCB_FSR(r0)
	movl	PCB_F0(r0),f0
	movl	PCB_F1(r0),f1
	movl	PCB_F2(r0),f2
	movl	PCB_F3(r0),f3
	movl	PCB_F4(r0),f4
	movl	PCB_F5(r0),f5
	movl	PCB_F6(r0),f6
	movl	PCB_F7(r0),f7
9:	ret	ARGS

/****************************************************************************/

/*
 * Trap and fault vector routines
 *
 * On exit from the kernel to user mode, we always need to check for ASTs.  In
 * addition, we need to do this atomically; otherwise an interrupt may occur
 * which causes an AST, but it won't get processed until the next kernel entry
 * (possibly the next clock tick).  Thus, we disable interrupt before checking,
 * and only enable them again on the final `rett' or before calling the AST
 * handler.
 */

/*
 * First some macro definitions.
 */

/*
 * Enter the kernel. Save r0-r7, usp and sb
 */
#define	KENTER \
	enter	[r0,r1,r2,r3,r4,r5,r6,r7],8; \
	sprd	usp,REGS_USP(sp); \
	sprd	sb,REGS_SB(sp)

/*
 * Exit the kernel. Restore sb, sp and r0-r7.
 */
#define KEXIT \
	lprd	sb,REGS_SB(sp); \
	lprd	usp,REGS_USP(sp); \
	exit	[r0,r1,r2,r3,r4,r5,r6,r7]; \
	rett	0

/*
 * Check for AST.
 */
#define	CHECKAST \
	tbitw	8,REGS_PSR(sp); \
	bfc	9f; \
	7: \
	ints_off; \
	cmpqd	0,_C_LABEL(astpending)(pc); \
	beq	9f; \
	movqd	0,_C_LABEL(astpending)(pc); \
	ints_on; \
	movd	T_AST,tos; \
	movqd	0,tos; \
	movqd	0,tos; \
	bsr	_C_LABEL(trap); \
	adjspd	-12; \
	br	7b; \
	9:

#define	TRAP(label, code) \
	_ALIGN_TEXT; CAT(_ASM_LABEL(label),:); KENTER; \
	movd	code,tos; \
	br	_C_LABEL(handle_trap)

TRAP(trap_nmi,	    T_NMI)	/*  1 non-maskable interrupt */
TRAP(trap_abt,	    T_ABT)	/*  2 abort */
TRAP(trap_slave,    T_SLAVE)	/*  3 coprocessor trap */
TRAP(trap_ill,	    T_ILL)	/*  4 illegal operation in user mode */
TRAP(trap_dvz,	    T_DVZ)	/*  6 divide by zero */
TRAP(trap_bpt,	    T_BPT)	/*  8 breakpoint instruction */
TRAP(trap_trc,	    T_TRC)	/*  9 trace trap */
TRAP(trap_und,	    T_UND)	/* 10 undefined instruction */
TRAP(trap_rbe,	    T_RBE)	/* 11 restartable bus error */
TRAP(trap_nbe,	    T_NBE)	/* 12 non-restartable bus error */
TRAP(trap_ovf,	    T_OVF)	/* 13 integer overflow trap */
TRAP(trap_dbg,	    T_DBG)	/* 14 debug trap */
TRAP(trap_reserved, T_RESERVED)	/* 15 reserved */

/*
 * The following handles all synchronous traps and non maskable interupts.
 */
ENTRY_NOPROFILE(handle_trap)
	lprd    sb,0			/* Kernel code expects sb to be 0 */
	/*
	 * Store the mmu status.
	 * This is needed for abort traps.
	 */
	smr 	tear,tos
	smr	msr,tos
	bsr	_C_LABEL(trap)
	adjspd	-12			/* Pop off software part of frame. */
	CHECKAST
	KEXIT

/*
 * We abuse the flag trap to flush the instruction cache.
 * r0 contains the start address and r1 the len of the
 * region to flush. The start address of the handler is
 * cache line aligned.
 */
	.align	4
ASENTRY_NOPROFILE(flush_icache)
#ifndef CINVSMALL
	cinv	ia,r0
	addqd	1,tos
	rett	0
#else
	addqd	1,tos			/* Increment return address. */
	addd	r0,r1
	movd	r1,tos			/* Save address of second line. */
	sprw	psr,tos			/* Push psr. */
	movqw	0,tos			/* Push mod. */
	bsr	1f			/* Invalidate first line. */
	/*
	 * Restore address of second cachline and
	 * fall through to do the invalidation.
	 */
	movd	tos,r0
1:	movd	r0,r1
	andd	PGOFSET,r0
	lshd	-PGSHIFT,r1
GLOBAL(cinvstart)
	movd	@_PTmap[r1:d],r1
	andd	~PGOFSET,r1
	addd	r0,r1
	cinv	i,r1
GLOBAL(cinvend)
	rett	0
#endif

/*
 * The system call trap handler.
 */
ASENTRY_NOPROFILE(svc_handler)
	KENTER
	lprd	sb,0			/* Kernel code expects sb to be 0 */
	bsr	_C_LABEL(syscall)
rei:	CHECKAST
	KEXIT

/*
 * The handler for all asynchronous interrupts.
 */
ASENTRY_NOPROFILE(interrupt)
	KENTER
	lprd    sb,0			/* Kernel code expects sb to be 0 */
	movd	_C_LABEL(Cur_pl)(pc),tos
	movb	@ICU_ADR+SVCT,r0	/* Fetch vector */
	andd	0x0f,r0
	movd	r0,tos
	/*
	 * Disable software interrupts and the current
	 * interrupt by setting the corresponding bits
	 * in the ICU's IMSK registern and in Cur_pl.
	 */
	movd	r0,r1
	muld	IV_SIZE,r0
	movd	_C_LABEL(ivt)+IV_MASK(r0),r2
	orw	r2,_C_LABEL(Cur_pl)(pc)
	orw	r2,@ICU_ADR+IMSK
	movb	@ICU_ADR+HVCT,r2	/* Acknowledge Interrupt */
	/* 
	 * Flush pending writes and then enable CPU interrupts. 
	 */
	ints_off ; ints_on
	/* 
	 * Increment interrupt counters.
	 */
	addqd	1,_C_LABEL(intrcnt)(pc)[r1:d]
	addqd	1,_C_LABEL(uvmexp)+V_INTR(pc)
	addqd	1,_C_LABEL(ivt)+IV_CNT(r0)

	movd	_C_LABEL(ivt)+IV_ARG(r0),r1 /* Get argument */
	cmpqd	0,r1
	bne	1f
	addr	0(sp),r1		/* NULL -> push frame address */
1:
	movd	r1,tos
	movd	_C_LABEL(ivt)+IV_VEC(r0),r0 /* Call the handler */
	jsr	0(r0)
#if !defined(MRTD)
	adjspd	-8			/* Remove arg and vec from stack */
#else
	adjspd	-4			/* Remove vec from stack */
#endif
	/*
	 * If we are switching back to spl0 and there
	 * are pending software interrupts, switch to
	 * splsoft and call check_sir now.
	 */
	movd	tos,r3
	cmpd	r3,_C_LABEL(imask)(pc)
	bne	1f
	cmpqd	0,_C_LABEL(sirpending)(pc)
	beq	1f
	/*
	 * r3 contains imask[IPL_ZERO] and IR_SOFT is
	 * not set in imask[IPL_ZERO]. So the following
	 * instruction just does a
	 * 	r0 = r3 | (1 << IR_SOFT).
	 */
	addr	1 << IR_SOFT(r3),r0
	movd	r0,_C_LABEL(Cur_pl)(pc)
	movw	r0,@ICU_ADR+IMSK
#if defined(MRTD)
	movqd	0,tos
#endif
	bsr	_C_LABEL(check_sir)

1:	CHECKAST
	ints_off
	movd	r3,_C_LABEL(Cur_pl)(pc)
	movw	r3,@ICU_ADR+IMSK	/* Restore Cur_pl */
	KEXIT

/*
 * Finally the interrupt vector table.
 */
	_ALIGN_TEXT
GLOBAL(inttab)
	ASVECTOR(interrupt)
	ASVECTOR(trap_nmi)
	ASVECTOR(trap_abt)
	ASVECTOR(trap_slave)
	ASVECTOR(trap_ill)
	ASVECTOR(svc_handler)
	ASVECTOR(trap_dvz)
	ASVECTOR(flush_icache)
	ASVECTOR(trap_bpt)
	ASVECTOR(trap_trc)
	ASVECTOR(trap_und)
	ASVECTOR(trap_rbe)
	ASVECTOR(trap_nbe)
	ASVECTOR(trap_ovf)
	ASVECTOR(trap_dbg)
	ASVECTOR(trap_reserved)

/****************************************************************************/

/*
 * void *ram_size(void *start);
 * Determine RAM size.
 * 
 * First attempt: write-and-read-back (WRB) each page from start
 * until WRB fails or get a parity error.  This didn't work because
 * address decoding wraps around.
 *
 * New algorithm:
 *
 *	ret = round-up-page (start);
 *  loop:
 *	if (!WRB or parity or wrap) return ret;
 *	ret += pagesz;	(* check end of RAM at powers of two *)
 *	goto loop;
 *
 * Several things make this tricky.  First, the value read from
 * an address will be the same value written to the address if
 * the cache is on -- regardless of whether RAM is located at
 * the address.  Hence the cache must be disabled.  Second,
 * reading an unpopulated RAM address is likely to produce a
 * parity error.  Third, a value written to an unpopulated address
 * can be held by capacitance on the bus and can be correctly
 * read back if there is no intervening bus cycle.  Hence,
 * read and write two patterns.
 * 
 * Registers:
 *   r0 - current page, return value
 *   r1 - old config register
 *   r2 - temp config register
 *   r3 - pattern0
 *   r4 - pattern1
 *   r5 - old nmi vector
 *   r6 - save word at @0
 *   r7 - save word at @4
 *   sb - pointer to intbase
 */

KENTRY(ram_size, 4)
	enter	[r3,r4,r5,r6,r7],0
	sprd	sb,tos
	sprd	intbase,r0
	lprd	sb,r0			/* load intbase into sb */

	/*
	 * Initialize things.
	 */
	movd	@0,r6			/* save 8 bytes of first page */
	movd	@4,r7
	movd	0,@0			/* zero 8 bytes of first page */
	movd	0,@4
	sprw	cfg,r1			/* turn off data cache */
	movw	r1,r2			/* r1 = old config */
	andw	~CFG_DC,r2
	lprw	cfg,r2
	movd	4(sb),r5		/* save old NMI vector */
	addr	2f(pc),4(sb)		/* tmp NMI vector */
	cinv	ia,r0			/* Vector reads go through the icache? */
	movd	8(fp),r0		/* r0 = start */
	addr	PGOFSET(r0),r0 		/* round up to page */
	andd	~PGOFSET,r0
	movd	0xa5a5a5a5,r3
	comd	r3,r4

0:	movd	r3,0(r0)		/* write 8 bytes */
	movd	r4,4(r0)
	lprw	cfg,r2			/* flush write buffer */
	cmpd	r3,0(r0)		/* read back and compare */
	bne	1f
	cmpd	r4,4(r0)
	bne	1f
	cmpqd	0,@0			/* check for address wrap */
	bne	1f
	cmpqd	0,@4			/* check for address wrap */
	bne	1f
	addr	NBPG(r0),r0		/* next page */
	br	0b

1:	movd	r6,@0			/* restore 8 bytes of first page */
	movd	r7,@4
	lprw	cfg,r1			/* turn data cache back on */
	movd	r5,4(sb)		/* restore NMI vector */
	cinv	ia,r0			/* Vector reads go through the icache? */
	movd	PARCLU_PHYS,r2
	movb	0(r2),r2		/* clear parity status */
	lprd	sb,tos
	exit	[r3,r4,r5,r6,r7]
	ret	ARGS

2:					/* come here if parity error */
	addr	1b(pc),0(sp)		/* modify return addr to exit */
	rett	0

/****************************************************************************/

/*
 * vmstat -i uses the following labels and interrupt even increments the
 * counters. This information is also availiable from ivt[n].iv_use
 * and ivt[n].iv_cnt in much better form.
 */
	.text
GLOBAL(intrnames)
	ASMSTR "int  0"
	ASMSTR "int  1"
	ASMSTR "int  2"
	ASMSTR "int  3"
	ASMSTR "int  4"
	ASMSTR "int  5"
	ASMSTR "int  6"
	ASMSTR "int  7"
	ASMSTR "int  8"
	ASMSTR "int  9"
	ASMSTR "int 10"
	ASMSTR "int 11"
	ASMSTR "int 12"
	ASMSTR "int 13"
	ASMSTR "int 14"
	ASMSTR "int 15"
GLOBAL(eintrnames)

	.data
GLOBAL(intrcnt)
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
GLOBAL(eintrcnt)
