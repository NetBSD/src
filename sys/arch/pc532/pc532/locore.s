/*	$NetBSD: locore.s,v 1.31 1996/01/31 21:33:56 phil Exp $	*/

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
 *
 *	locore.s
 *
 *	locore.s,v 1.2 1993/09/13 07:26:47 phil Exp
 */

/*
 * locore.s  - -  assembler routines needed for BSD stuff.  (ns32532/pc532)
 *
 * Phil Nelson, Dec 6, 1992
 *
 * Modified by Matthias Pfaller, Jan 1996.
 *
 */

/*
 * Tell include files that we don't want any C stuff.
 */
#define LOCORE

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>
#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/trap.h>
#include <machine/cpufunc.h>

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 */
	.globl	_PTmap, _PTD, _PTDpde, _Sysmap
	.set	_PTmap,(PTDPTDI << PDSHIFT)
	.set	_PTD,(_PTmap + PTDPTDI * NBPG)
	.set	_PTDpde,(_PTD + PTDPTDI * 4)		# XXX 4 == sizeof pde
	.set	_Sysmap,(_PTmap + KPTDI * NBPG)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
	.globl	_APTmap, _APTD, _APTDpde
	.set	_APTmap,(APTDPTDI << PDSHIFT)
	.set	_APTD,(_APTmap + APTDPTDI * NBPG)
	.set	_APTDpde,(_PTD + APTDPTDI * 4)		# XXX 4 == sizeof pde

	.globl	_proc0paddr, _PTDpaddr
_proc0paddr:	.long	0
_PTDpaddr:	.long	0	# paddr of PTD, for libkvm

/*
 * Initialization
 */
	.data
	.globl	_cold, _esym, _bootdev, _boothowto, _inttab
	.globl	__save_sp, __save_fp, __old_intbase, __have_fpu
_cold:		.long	1	/* cold till we are not */
_esym:		.long	0	/* pointer to end of symbols */
__save_sp:	.long	0	/* Monitor stack pointer */
__save_fp:	.long	0	/* Monitor frame pointer */
__old_intbase:	.long	0	/* Monitor intbase */
__have_fpu:	.long	0	/* Have we an FPU installed? */

	.text
	.globl start
start:	ints_off			# make sure interrupts are off.
	bicpsrw	PSL_US			# make sure we are using sp0.
	lprd    sb,0			# gcc expects this.

	/*
	 * Zero the bss segment.
	 */
	addr	_end(pc),r0		# setup to zero the bss segment.
	addr	_edata(pc),r1
	subd	r1,r0			# compute _end - _edata
	movd	r0,tos			# push length
	addr	_edata(pc),tos		# push address
	bsr	_bzero			# zero the bss segment

	/*
	 * Save monitor's sp, fp and intbase.
	 */
	sprd	sp,__save_sp(pc)
	sprd	fp,__save_fp(pc)
	sprd	intbase,__old_intbase(pc)

	/*
	 * The boot program provides us a magic in r3,
	 * esym in r4, bootdev in r6 and boothowto in r7.
	 * Set the kernel variables if the magic matches.
	 */
	cmpd	0xc1e86394,r3
	bne	1f
	movd	r4,_esym(pc)
	movd	r6,_bootdev(pc)
	movd	r7,_boothowto(pc)
1:	/*
	 * Finish machine initialization and start main.
	 */
	br	_init532

ENTRY(proc_trampoline)
	movd	r4,tos
	jsr	0(r3)
	cmpqd	0,tos
	br	rei

/*****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */

ENTRY(sigcode)
	jsr	0(SIGF_HANDLER(sp))
	addr	SIGF_SC(sp),tos		/* scp (the call may have clobbered */
					/* the copy at SIGF_SCP(sp)). */
	movqd	0,tos			/* Push a fake return address. */
	movd	SYS_sigreturn,r0
	svc
	movd	0,0			/* Illegal instruction. */
	.globl	_esigcode
_esigcode:

/*****************************************************************************/

/*
 * The following primitives are used to fill and copy regions of memory.
 */

/*
 * bzero (void *b, size_t len)
 *	write len zero bytes to the string b.
 */

ENTRY(bzero)
	enter	[r3],0

	movd	B_ARG0,r1		/* b */
	movd	B_ARG1,r2		/* len */
	cmpd	19,r2
	bhs	6f			/* Not worth the trouble. */

	/*
	 * Is address aligned?
	 */
	movd	r1,r0
	andd	3,r0			/* r0 = b & 3 */
	cmpqd	0,r0
	beq	0f

	/*
	 * Align address (if necessary).
	 */
	movqd	0,0(r1)
	addr	-4(r0)[r2:b],r2		/* len = len + (r0 - 4) */
	negd	r0,r0
	addr	4(r0)[r1:b],r1		/* b = b + (-r0 + 4) */

0:	/*
	 * Compute loop start address.
	 */
	movd	r2,r0
	addr	60(r2),r3
	andd	60,r0			/* r0 = len & 60 */
	lshd	-6,r3			/* r3 = (len + 60) >> 6 */
	andd	3,r2			/* len &= 3 */

	cmpqd	0,r0
	beq	1f

	addr	-64(r1)[r0:b],r1	/* b = b - 64 + r0 */
	lshd	-2,r0
	addr	0(r0)[r0:w],r0
	negd	r0,r0			/* r0 = -3 * r0 / 4 */

	jump	2f(pc)[r0:b]		/* Now enter the loop */

	/*
	 * Zero 64 bytes per loop iteration.
	 */
	.align	2
1:	movqd	0,0(r1)
	movqd	0,4(r1)
	movqd	0,8(r1)
	movqd	0,12(r1)
	movqd	0,16(r1)
	movqd	0,20(r1)
	movqd	0,24(r1)
	movqd	0,28(r1)
	movqd	0,32(r1)
	movqd	0,36(r1)
	movqd	0,40(r1)
	movqd	0,44(r1)
	movqd	0,48(r1)
	movqd	0,52(r1)
	movqd	0,56(r1)
	movqd	0,60(r1)
2:	addd	64,r1
	acbd	-1,r3,1b

3:	cmpqd	0,r2
	beq	5f

	/*
	 * Zero out blocks shorter then four bytes.
	 */
4:	movqb	0,-1(r1)[r2:b]
	acbd	-1,r2,4b

5:	exit	[r3]
	ret	0

	/*
	 * For blocks smaller then 20 bytes
	 * this is faster.
	 */
	.align	2
6:	cmpqd	3,r2
	bhs	3b

	movd	r2,r0
	andd	3,r2
	lshd	-2,r0

7:	movqd	0,0(r1)
	addqd	4,r1
	acbd	-1,r0,7b
	br	3b

/*****************************************************************************/

/*
 * The following primitives are used to copy data in and out of the user's
 * address space.
 */

/*
 * copyout(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes into the user's address space.
 */
ENTRY(copyout)
	enter	[r3,r4],0
	movd	_curpcb(pc),r4
	addr	_copy_fault(pc),PCB_ONFAULT(r4)

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
	ret	0

/*
 * copyin(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes from the user's address space.
 */
ENTRY(copyin)
	enter	[r3,r4],0
	movd	_curpcb(pc),r4
	addr	_copy_fault(pc),PCB_ONFAULT(r4)

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
	bhi	_copy_fault
	cmpd	r1,r3
	bhs	_copy_fault		/* check for overflow. */

	/* And now do the copy. */
	lshd	-2,r0
	movsd
1:	movd	B_ARG2,r0
	andd	3,r0
	movsb				/* This also sets r0 to zero. */
9:	movd	r0,PCB_ONFAULT(r4)
	exit	[r3,r4]
	ret	0

ENTRY(copy_fault)
	movqd	0,PCB_ONFAULT(r4)
	movd	EFAULT,r0
	exit	[r3,r4]
	ret	0

/*
 * copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
ENTRY(copyoutstr)
	enter	[r3],0
	movd	_curpcb(pc),r3
	addr	_copystr_fault(pc),PCB_ONFAULT(r3)
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
	br	copystr_return
3:	addqd	-1,r2
	movqd	0,r0
	br	copystr_return

/*
 * copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
ENTRY(copyinstr)
	enter	[r3],0
	movd	_curpcb(pc),r3
	addr	_copystr_fault(pc),PCB_ONFAULT(r3)
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
	br	copystr_return
3:	addqd	-1,r2
	movqd	0,r0
	br	copystr_return

ENTRY(copystr_fault)
	movd	EFAULT,r0

copystr_return:
	/* Set *lencopied and return r0. */
	movqd	0,PCB_ONFAULT(r3)
	movd	B_ARG2,r1
	subd	r2,r1
	movd	B_ARG3,r2
	cmpqd	0,r2
	beq	1f
	movd	r1,0(r2)
1:	exit	[r3]
	ret	0

/*
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
ENTRY(copystr)
	enter	[r4],0
	movd	B_ARG0,r1		/* from */
	movd	B_ARG1,r2		/* to */
	movd	B_ARG2,r0		/* maxlen */
	cmpqd	0,r0
	beq	2f			/* anything to do? */

	movqd	0,r4			/* Set match value. */
	movsb	u
	movd	r0,r1			/* Save count. */
	bfs	1f

	/*
	 * Terminated due to limit count.
	 * Return ENAMETOOLONG. 
	 */
	movd	ENAMETOOLONG,r0
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
	ret	0

/*
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */
ENTRY(fuword)
	movd	_curpcb(pc),r2
	addr	_fusufault(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	/*
	 * MACH's locore.s code says that
	 * due to cpu bugs the destination
	 * of movusi can't be a register or tos.
	 */
	movusd	0(r0),S_ARG0
	movd	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	0

/*
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 */
ENTRY(fuswintr)
	movd	_curpcb(pc),r2
	addr	_fusubail(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movusw	0(r0),S_ARG0
	movqd	0,r0
	movw	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	0

/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
ENTRY(fubyte)
	movd	_curpcb(pc),r2
	addr	_fusufault(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movusb	0(r0),S_ARG0
	movqd	0,r0
	movb	S_ARG0,r0
	movqd	0,PCB_ONFAULT(r2)
	ret	0

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
ENTRY(suword)
	movd	_curpcb(pc),r2
	addr	_fusufault(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsud	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	0

/*
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 */
ENTRY(suswintr)
	movd	_curpcb(pc),r2
	addr	_fusubail(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsuw	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	0

/*
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */
ENTRY(subyte)
	movd	_curpcb(pc),r2
	addr	_fusufault(pc),PCB_ONFAULT(r2)
	movd	S_ARG0,r0
	movsub	S_ARG1,0(r0)
	movqd	0,r0
	movd	r0,PCB_ONFAULT(r2)
	ret	0

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
ENTRY(fusufault)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	0

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
ENTRY(fusubail)
	movqd	0,PCB_ONFAULT(r2)
	movqd	-1,r0
	ret	0

/*****************************************************************************/

/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */
	.globl	_whichqs, _qs, _cnt, _panic

/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
ENTRY(setrunqueue)
	movd	S_ARG0,r0
#ifdef DIAGNOSTIC
	cmpqd	0,P_BACK(r0)		/* should not be on q already */
	bne	1f
	cmpqd	0,P_WCHAN(r0)
	bne	1f
	cmpb	SRUN,P_STAT(r0)
	bne	1f
#endif /* DIAGNOSTIC */
	movzbd	P_PRIORITY(r0),r1
	lshd	-2,r1
	sbitd	r1,_whichqs(pc)		/* set queue full bit */
	addr	_qs(pc)[r1:q],r1	/* locate q hdr */
	movd	P_BACK(r1),r2		/* locate q tail */
	movd	r1,P_FORW(r0)		/* set p->p_forw */
	movd	r0,P_BACK(r1)		/* update q's p_back */
	movd	r0,P_FORW(r2)		/* update tail's p_forw */
	movd    r2,P_BACK(r0)		/* set p->p_back */
	ret	0
#ifdef DIAGNOSTIC
1:	addr	3f(pc),tos		/* Was on the list! */
	bsr	_panic
3:	.asciz "setrunqueue"
#endif

/*
 * remrq(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
ENTRY(remrq)
	movd	S_ARG0,r1
	movzbd	P_PRIORITY(r1),r0
#ifdef DIAGNOSTIC
	lshd	-2,r0
	tbitd	r0,_whichqs(pc)
	bfc	1f
#endif /* DIAGNOSTIC */
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
	cbitd	r0,_whichqs(pc)		/* mark q as empty */
2:	ret	0
#ifdef DIAGNOSTIC
1:	addr	3f(pc),tos		/* No queue entry! */
	bsr	_panic
3:	.asciz "remrq"
#endif

/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */
ENTRY(idle)
	ints_off
	cmpqd	0,_whichqs(pc)
	bne	sw1
	ints_on				/* We may lose a tick here ... */
	wait				/* Wait for interrupt. */
	br	_idle

#ifdef	DIAGNOSTIC
ENTRY(switch_error)
	addr	1f(pc),tos
	bsr	_panic
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * cpu_switch(void);
 * Find a runnable process and switch to it.  Wait if necessary.
 */
ENTRY(cpu_switch)
	enter	[r3,r4,r5,r6,r7],0

	movd	_curproc(pc),r4

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spl0()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movqd	0,_curproc(pc)

	movd	_imask(pc),tos
	bsr	_splx			/* spl0 - process pending interrupts */

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

sw1:	movqd	0,r0
	ffsd	_whichqs(pc),r0		/* find a full q */
	bfs	_idle			/* if none, idle */

	/* Get the process and unlink it from the queue. */
	addr	_qs(pc)[r0:q],r1	/* address of qs entry! */

	movd	P_FORW(r1),r2		/* unlink from front of process q */
#ifdef	DIAGNOSTIC
	cmpd	r2,r1			/* linked to self (i.e. nothing queued? */
	beq	_switch_error		/* not possible */
#endif /* DIAGNOSTIC */
	movd	P_FORW(r2),r3
	movd	r3,P_FORW(r1)
	movd	r1,P_BACK(r3)

	cmpd	r1,r3			/* q empty? */
	bne	3f

	cbitd	r0,_whichqs(pc)		/* queue is empty, turn off whichqs. */

3:	movqd	0,_want_resched(pc)	/* We did a resched! */

#ifdef	DIAGNOSTIC
	cmpqd	0,P_WCHAN(r2)		/* Waiting for something? */
	bne	_switch_error		/* Yes; shouldn't be queued. */
	cmpb	SRUN,P_STAT(r2)		/* In run state? */
	bne	_switch_error		/* No; shouldn't be queued. */
#endif /* DIAGNOSTIC */
	/* Isolate process. XXX Is this necessary? */
	movqd	0,P_BACK(r2)

	/* Record new process. */
	movd	r2,_curproc(pc)

	/* It's okay to take interrupts here. */
	ints_on

	/* Skip context switch if same process. */
	cmpd	r2,r4
	beq	switch_return

	/* If old process exited, don't bother. */
	cmpqd	0,r4
	beq	switch_exited

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

switch_exited:
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
	movd	r1,_curpcb(pc)

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
	bsr	_splx
	cmpqd	0,tos

	movd	r2,r0			/* return(p); */
	exit	[r3,r4,r5,r6,r7]
	ret	0

/*****************************************************************************/

/*
 * FPU handling.
 * Normally the FPU state is saved and restored in trap.
 */

/*
 * Check if we have a FPU installed.
 */
#ifdef NS381
#define FPU_INSTALLED
#else
#define FPU_INSTALLED	cmpqd 0,__have_fpu(pc); beq 9f
#endif

/*
 * void save_fpu_context(struct pcb *p);
 * Save FPU context.
 */
ENTRY(save_fpu_context)
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
9:	ret	0

/*
 * void restore_fpu_context(struct pcb *p);
 * Restore FPU context.
 */
ENTRY(restore_fpu_context)
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
9:	ret	0

/*****************************************************************************/

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
 * Check for AST. CPU interrupts have to be disabled.
 */
#define	CHECKAST \
	tbitw	8,REGS_PSR(sp); \
	bfc	9f; \
	cmpqd	0,_astpending(pc); \
	beq	9f; \
	movqd	0,_astpending(pc); \
	ints_on; \
	movd	T_AST,tos; \
	movqd	0,tos; \
	movqd	0,tos; \
	bsr	_trap; \
	adjspd	-12; 9:

#define	TRAP(label, code) \
	.align 2; CAT(label,:); KENTER; \
	movd	code,tos; \
	br	handle_trap

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
	.align	2
handle_trap:
	lprd    sb,0			/* Kernel code expects sb to be 0 */
	/*
	 * Store the mmu status.
	 * This is needed for abort traps.
	 */
	smr 	tear,tos
	smr	msr,tos
	bsr	_trap
	adjspd	-12			/* Pop off software part of frame. */
	ints_off
	CHECKAST
	KEXIT

/*
 * We abuse the flag trap to flush the instruction cache.
 * r0 contains the start address and r1 the len of the
 * region to flush. The start address of the handler is
 * cache line aligned.
 */
	.align	4
#ifndef CINVSMALL
trap_flg:
	cinv	ia,r0
	addqd	1,tos
	rett	0
#else
	.globl	_cinvstart, _cinvend
trap_flg:
	addqd	1,tos			/* Increment return address */
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
_cinvstart:
	movd	@_PTmap[r1:d],r1
	andd	~PGOFSET,r1
	addd	r0,r1
	cinv	i,r1
_cinvend:
	rett	0
#endif

/*
 * The system call trap handler.
 */
	.align	2
trap_svc:
	KENTER
	lprd	sb,0			/* Kernel code expects sb to be 0 */
	bsr	_syscall
rei:	ints_off
	CHECKAST
	KEXIT

/*
 * The handler for all asynchronous interrupts.
 */
	.align	2
trap_int:
	KENTER
	lprd    sb,0			/* Kernel code expects sb to be 0 */
	movd	_Cur_pl(pc),tos
	movb	@ICU_ADR+HVCT,r0	/* fetch vector */
	andd	0x0f,r0
	movd	r0,tos
	movqd	1,r1
	lshd	r0,r1
	orw	r1,_Cur_pl(pc)		/* or bit to Cur_pl */
	orw	r1,@ICU_ADR+IMSK	/* and to IMSK */
					/* bits set by idisabled in IMSK */
					/* have to be preserved */
	ints_off			/* flush pending writes */
	ints_on				/* and now turn ints on */
	addqd	1,_intrcnt(pc)[r0:d]
	lshd	4,r0
	addqd	1,_cnt+V_INTR(pc)
	addqd	1,_ivt+IV_CNT(r0)	/* increment counters */
	movd	_ivt+IV_ARG(r0),r1	/* get argument */
	cmpqd	0,r1
	bne	1f
	addr	0(sp),r1		/* NULL -> push frame address */
1:	movd	r1,tos
	movd	_ivt+IV_VEC(r0),r0	/* call the handler */
	jsr	0(r0)

	adjspd	-8			/* Remove arg and vec from stack */
	bsr	_splx_di		/* Restore Cur_pl */
	cmpqd	0,tos
	CHECKAST
	KEXIT

/*
 * Finally the interrupt vector table.
 */
	.align 2
_inttab:
	.long trap_int
	.long trap_nmi
	.long trap_abt
	.long trap_slave
	.long trap_ill
	.long trap_svc
	.long trap_dvz
	.long trap_flg
	.long trap_bpt
	.long trap_trc
	.long trap_und
	.long trap_rbe
	.long trap_nbe
	.long trap_ovf
	.long trap_dbg
	.long trap_reserved

/*****************************************************************************/

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

pattern0	= 0xa5a5a5a5
pattern1	= 0x5a5a5a5a
parity_clr	= 0x28000050

ENTRY(ram_size)
	enter	[r1,r2,r3,r4,r5,r6,r7],0
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
	addr	tmp_nmi(pc),4(sb)	/* tmp NMI vector */
	cinv	ia,r0			/* Vector reads go through the icache */
	movd	8(fp),r0		/* r0 = start */
	addr	PGOFSET(r0),r0 		/* round up to page */
	andd	~PGOFSET,r0
	movd	pattern0,r3
	movd	pattern1,r4
rz_loop:
	movd	r3,0(r0)		/* write 8 bytes */
	movd	r4,4(r0)
	lprw	cfg,r2			/* flush write buffer */
	cmpd	r3,0(r0)		/* read back and compare */
	bne	rz_exit
	cmpd	r4,4(r0)
	bne	rz_exit
	cmpqd	0,@0			/* check for address wrap */
	bne	rz_exit
	cmpqd	0,@4			/* check for address wrap */
	bne	rz_exit
	addr	NBPG(r0),r0		/* next page */
	br	rz_loop
rz_exit:
	movd	r6,@0			/* restore 8 bytes of first page */
	movd	r7,@4
	lprw	cfg,r1			/* turn data cache back on */
	movd	r5,4(sb)		/* restore NMI vector */
	cinv	ia,r0			/* Vector reads go through the icache */
	movd	parity_clr,r2
	movb	0(r2),r2		/* clear parity status */
	lprd	sb,tos
	exit	[r1,r2,r3,r4,r5,r6,r7]
	ret	0

tmp_nmi:				/* come here if parity error */
	addr	rz_exit(pc),0(sp)	/* modify return addr to exit */
	rett	0

/* To get back to the rom monitor .... */
ENTRY(bpt_to_monitor)

/* Switch to monitor's stack. */
	ints_off
	bicpsrw	PSL_US			/* make sure we are using sp0. */
	sprd	psr, tos		/* Push the current psl. */
	save	[r1,r2,r3,r4]
	sprd	sp, r1  		/* save kernel's sp */
	sprd	fp, r2  		/* save kernel's fp */
	sprd	intbase, r3		/* Save current intbase. */
	smr	ptb0, r4		/* Save current ptd! */

/* Change to low addresses */
	lmr	ptb0, _PTDpaddr(pc)	/* Load the idle ptd */
	addr	low(pc), r0
	andd	~KERNBASE, r0
	movd	r0, tos
	ret	0

low:
/* Turn off mapping. */
	smr	mcr, r0
	lmr	mcr, 0
	lprd	sp, __save_sp(pc)	/* restore monitors sp */
	lprd	fp, __save_fp(pc)	/* restore monitors fp */
	lprd	intbase, __old_intbase(pc)	/* restore monitors intbase */
	bpt

/* Reload kernel stack AND return. */
	lprd	intbase, r3		/* restore kernel's intbase */
	lprd	fp, r2			/* restore kernel's fp */
	lprd	sp, r1			/* restore kernel's sp */
	lmr	mcr, r0
	addr	highagain(pc), r0
	ord  	KERNBASE, r0
	jump 	0(r0)
highagain:
	lmr	ptb0, r4		/* Get the last ptd! */
	restore	[r1,r2,r3,r4]
	lprd	psr, tos		/* restore psl */
	ints_on
	ret 0

/* Include all other .s files. */
#include "bcopy.s"

/*****************************************************************************/

/*
 * vmstat -i uses the following labels and trap_int even increments the
 * counters. This information is also availiable from ivt[n].iv_use
 * and ivt[n].iv_cnt in much better form.
 */
	.globl	_intrnames, _eintrnames, _intrcnt, _eintrcnt
	.text
_intrnames:
	.asciz "int  0"
	.asciz "int  1"
	.asciz "int  2"
	.asciz "int  3"
	.asciz "int  4"
	.asciz "int  5"
	.asciz "int  6"
	.asciz "int  7"
	.asciz "int  8"
	.asciz "int  9"
	.asciz "int 10"
	.asciz "int 11"
	.asciz "int 12"
	.asciz "int 13"
	.asciz "int 14"
	.asciz "int 15"
_eintrnames:

	.data
_intrcnt:
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
_eintrcnt:
