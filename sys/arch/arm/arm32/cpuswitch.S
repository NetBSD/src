/*	$NetBSD: cpuswitch.S,v 1.18 2002/08/31 03:07:32 thorpej Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpuswitch.S
 *
 * cpu switching functions
 *
 * Created      : 15/10/94
 */

#include "opt_armfpe.h"

#include "assym.h"
#include <machine/param.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/asm.h>

#undef IRQdisable
#undef IRQenable

/*
 * New experimental definitions of IRQdisable and IRQenable
 * These keep FIQ's enabled since FIQ's are special.
 */

#define IRQdisable \
	mrs	r14, cpsr ; \
	orr	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14 ; \

#define IRQenable \
	mrs	r14, cpsr ; \
	bic	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14 ; \

/*
 * setrunqueue() and remrunqueue()
 *
 * Functions to add and remove a process for the run queue.
 */

	.text

.Lwhichqs:
	.word	_C_LABEL(sched_whichqs)

.Lqs:
	.word	_C_LABEL(sched_qs)

/*
 * On entry
 *	r0 = process
 */

ENTRY(setrunqueue)
	/*
	 * Local register usage
	 * 	r0 = process
	 * 	r1 = queue
	 * 	r2 = &qs[queue] and temp
	 * 	r3 = temp
	 *	r12 = whichqs
	 */
#ifdef DIAGNOSTIC
	ldr	r1, [r0, #(P_BACK)]
	teq	r1, #0x00000000
	bne	.Lsetrunqueue_erg

	ldr	r1, [r0, #(P_WCHAN)]
	teq	r1, #0x00000000
	bne	.Lsetrunqueue_erg
#endif

	/* Get the priority of the queue */
	ldrb	r1, [r0, #(P_PRIORITY)]

	/* Indicate that there is a process on this queue */
	ldr	r12, .Lwhichqs
	mov	r1, r1, lsr #2
	ldr	r2, [r12]
	mov	r3, #0x00000001
	mov	r3, r3, lsl r1
	orr	r2, r2, r3
	str	r2, [r12]

	/* Get the address of the queue */
	ldr	r2, .Lqs
	add	r1, r2, r1, lsl # 3

	/* Hook the process in */
	str	r1, [r0, #(P_FORW)]
	ldr	r2, [r1, #(P_BACK)]

	str	r0, [r1, #(P_BACK)]
#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	.Lsetrunqueue_erg
#endif
	str	r0, [r2, #(P_FORW)]
	str	r2, [r0, #(P_BACK)]

	mov	pc, lr

#ifdef DIAGNOSTIC
.Lsetrunqueue_erg:
	mov	r2, r1
	mov	r1, r0
	add	r0, pc, #.Ltext1 - . - 8
	bl	_C_LABEL(printf)

	ldr	r2, .Lqs
	ldr	r1, [r2]
	add	r0, pc, #.Ltext2 - . - 8
	b	_C_LABEL(panic)	

.Ltext1:
	.asciz	"setrunqueue : %08x %08x\n"
.Ltext2:
	.asciz	"setrunqueue : [qs]=%08x qs=%08x\n"
	.align	0
#endif

/*
 * On entry
 *	r0 = process
 */

ENTRY(remrunqueue)
	/*
	 * Local register usage
	 *	r0 = oldproc
	 * 	r1 = queue
	 * 	r2 = &qs[queue] and scratch
	 *	r3 = scratch
	 *	r12 = whichqs
	 */

	/* Get the priority of the queue */
	ldrb	r1, [r0, #(P_PRIORITY)]
	mov	r1, r1, lsr #2

	/* Unhook the process */
	ldr	r2, [r0, #(P_FORW)]
	ldr	r3, [r0, #(P_BACK)]

	str	r3, [r2, #(P_BACK)]
	str	r2, [r3, #(P_FORW)]

	/* If the queue is now empty clear the queue not empty flag */
	teq	r2, r3

	/* This could be reworked to avoid the use of r4 */
	ldreq	r12, .Lwhichqs
	moveq	r3, #0x00000001
	ldreq	r2, [r12]
	moveq	r3, r3, lsl r1
	biceq	r2, r2, r3
	streq	r2, [r12]

	/* Remove the back pointer for the process */
	mov	r1, #0x00000000
	str	r1, [r0, #(P_BACK)]

	mov	pc, lr


/*
 * cpuswitch()
 *
 * preforms a process context switch.
 * This function has several entry points
 */

.Lcurproc:
	.word	_C_LABEL(curproc)

.Lcurpcb:
	.word	_C_LABEL(curpcb)

.Lwant_resched:
	.word	_C_LABEL(want_resched)

.Lcpufuncs:	
	.word	_C_LABEL(cpufuncs)
	
	.data
	.global	_C_LABEL(curpcb)
_C_LABEL(curpcb):
	.word	0x00000000
	.text

.Lblock_userspace_access:
	.word	_C_LABEL(block_userspace_access)

.Lcpu_do_powersave:
	.word	_C_LABEL(cpu_do_powersave)

/*
 * Idle loop, exercised while waiting for a process to wake up.
 *
 * NOTE: When we jump back to .Lswitch_search, we must have a
 * pointer to whichqs in r7, which is what it is when we arrive
 * here.
 */
/* LINTSTUB: Ignore */
ASENTRY_NP(idle)
#if defined(LOCKDEBUG)
	bl	_C_LABEL(sched_unlock_idle)
#endif
	ldr	r3, .Lcpu_do_powersave

	/* Enable interrupts */
	IRQenable

	/* If we don't want to sleep, use a simpler loop. */
	ldr	r3, [r3]		/* r3 = cpu_do_powersave */
	teq	r3, #0
	bne	2f

	/* Non-powersave idle. */
1:	/* should maybe do uvm pageidlezero stuff here */
	ldr	r3, [r7]		/* r3 = whichqs */
	teq	r3, #0x00000000
	bne	.Lswitch_search
	b	1b

2:	/* Powersave idle. */
	ldr	r4, .Lcpufuncs
3:	ldr	r3, [r7]		/* r3 = whichqs */
	teq	r3, #0x00000000
	bne	.Lswitch_search

	/* if saving power, don't want to pageidlezero */
	mov	r0, #0
	add	lr, pc, #3b - . - 8
	ldr	pc, [r4, #(CF_SLEEP)]
	/* loops back around */


/*
 * Find a new process to run, save the current context and
 * load the new context
 */

ENTRY(cpu_switch)
/*
 * Local register usage. Some of these registers are out of date.
 * r1 = oldproc
 * r2 = spl level
 * r3 = whichqs
 * r4 = queue
 * r5 = &qs[queue]
 * r6 = newproc
 * r7 = scratch
 */
	stmfd	sp!, {r4-r7, lr}

	/*
	 * Get the current process and indicate that there is no longer
	 * a valid process (curproc = 0).  Zero the current PCB pointer
	 * while we're at it.
	 */
	ldr	r7, .Lcurproc
	ldr	r6, .Lcurpcb
	mov	r0, #0x00000000
	ldr	r1, [r7]		/* r1 = curproc */
	str	r0, [r7]		/* curproc = NULL */
	str	r0, [r6]		/* curpcb = NULL */

	/* stash the old proc while we call functions */
	mov	r5, r1

#if defined(LOCKDEBUG)
	/* release the sched_lock before handling interrupts */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	/* Lower the spl level to spl0 and get the current spl level. */
#ifdef __NEWINTR
	mov	r0, #(IPL_NONE)
	bl	_C_LABEL(_spllower)
#else /* ! __NEWINTR */
#ifdef spl0
	mov	r0, #(_SPL_0)
	bl	_C_LABEL(splx)
#else
	bl	_C_LABEL(spl0)
#endif /* spl0 */
#endif /* __NEWINTR */

	/* Push the old spl level onto the stack */
	str	r0, [sp, #-0x0004]!

	/* First phase : find a new process */

	ldr	r7, .Lwhichqs

	/* rem: r5 = old proc */
	/* rem: r7 = &whichqs */

.Lswitch_search:
	IRQdisable
#if defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

	/* Do we have any active queues  */
	ldr	r3, [r7]

	/* If not we must idle until we do. */
	teq	r3, #0x00000000
	beq	_ASM_LABEL(idle)

	/* put old proc back in r1 */
	mov	r1, r5

	/* rem: r1 = old proc */
	/* rem: r3 = whichqs */
	/* rem: interrupts are disabled */

	/*
	 * We have found an active queue. Currently we do not know which queue
	 * is active just that one of them is.
	 */
	/* this is the ffs algorithm devised by d.seal and posted to
	 * comp.sys.arm on 16 Feb 1994.
	 */
 	rsb	r5, r3, #0
 	ands	r0, r3, r5
	
	adr	r5, .Lcpu_switch_ffs_table
	
				    /* X = R0 */
	orr	r4, r0, r0, lsl #4  /* r4 = X * 0x11 */ 
	orr	r4, r4, r4, lsl #6  /* r4 = X * 0x451 */
	rsb	r4, r4, r4, lsl #16 /* r4 = X * 0x0450fbaf */
	
	/* used further down, saves SA stall */
	ldr	r6, .Lqs
              
	/* now lookup in table indexed on top 6 bits of a4 */
	ldrb	r4, [ r5, r4, lsr #26 ]

	/* rem: r0 = bit mask of chosen queue (1 << r4) */
	/* rem: r1 = old proc */
	/* rem: r3 = whichqs */
	/* rem: r4 = queue number */
	/* rem: interrupts are disabled */

	/* Get the address of the queue (&qs[queue]) */
	add	r5, r6, r4, lsl #3

	/*
	 * Get the process from the queue and place the next process in
	 * the queue at the head. This basically unlinks the process at
	 * the head of the queue.
	 */
	ldr	r6, [r5, #(P_FORW)]

	/* rem: r6 = new process */
	ldr	r7, [r6, #(P_FORW)]
	str	r7, [r5, #(P_FORW)]	

	/*
	 * Test to see if the queue is now empty. If the head of the queue
	 * points to the queue itself then there are no more processes in
	 * the queue. We can therefore clear the queue not empty flag held
	 * in r3.
	 */

	teq	r5, r7
	biceq	r3, r3, r0

	/* rem: r0 = bit mask of chosen queue (1 << r4) - NOT NEEDED AN MORE */

	/* Fix the back pointer for the process now at the head of the queue. */
	ldr	r0, [r6, #(P_BACK)]
	str	r0, [r7, #(P_BACK)]

	/* Update the RAM copy of the queue not empty flags word. */
	ldr	r7, .Lwhichqs
	str	r3, [r7]

	/* rem: r1 = old proc */
	/* rem: r3 = whichqs - NOT NEEDED ANY MORE */
	/* rem: r4 = queue number - NOT NEEDED ANY MORE */
	/* rem: r6 = new process */
	/* rem: interrupts are disabled */

	/* Clear the want_resched flag */
	ldr	r7, .Lwant_resched
	mov	r0, #0x00000000
	str	r0, [r7]

	/*
	 * Clear the back pointer of the process we have removed from
	 * the head of the queue. The new process is isolated now.
	 */
	str	r0, [r6, #(P_BACK)]

#if defined(LOCKDEBUG)
	/*
	 * unlock the sched_lock, but leave interrupts off, for now.
	 */
	mov	r7, r1
	bl	_C_LABEL(sched_unlock_idle)
	mov	r1, r7
#endif

	/* p->p_cpu initialized in fork1() for single-processor */

	/* Process is now on a processor. */
	mov	r0, #SONPROC			/* p->p_stat = SONPROC */
	strb	r0, [r6, #(P_STAT)]

	/* We have a new curproc now so make a note it */
	ldr	r7, .Lcurproc
	str	r6, [r7]

	/* Hook in a new pcb */
	ldr	r7, .Lcurpcb
	ldr	r0, [r6, #(P_ADDR)]
	str	r0, [r7]

	/* At this point we can allow IRQ's again. */
	IRQenable

	/* rem: r1 = old proc */
	/* rem: r6 = new process */
	/* rem: interrupts are enabled */

	/*
	 * If the new process is the same as the process that called
	 * cpu_switch() then we do not need to save and restore any
	 * contexts. This means we can make a quick exit.
	 * The test is simple if curproc on entry (now in r1) is the
	 * same as the proc removed from the queue we can jump to the exit.
	 */
	teq	r1, r6
	beq	.Lswitch_return

	/* Remember the old process in r0 */
	mov	r0, r1

	/*
	 * If the curproc on entry to cpu_switch was zero then the
	 * process that called it was exiting. This means that we do
	 * not need to save the current context. Instead we can jump
	 * straight to restoring the context for the new process.
	 */
	teq	r0, #0x00000000
	beq	.Lswitch_exited

	/* rem: r0 = old proc */
	/* rem: r6 = new process */
	/* rem: interrupts are enabled */

	/* Stage two : Save old context */

	/* Get the user structure for the old process. */
	ldr	r1, [r0, #(P_ADDR)]

	/* Save all the registers in the old process's pcb */
	add	r7, r1, #(PCB_R8)
	stmia	r7, {r8-r13}

	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
        mrs	r3, cpsr
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE | I32_bit)
        msr	cpsr_c, r2

	str	sp, [r1, #(PCB_UND_SP)]

        msr	cpsr_c, r3		/* Restore the old mode */

	/* rem: r0 = old proc */
	/* rem: r1 = old pcb */
	/* rem: r6 = new process */
	/* rem: interrupts are enabled */

	/* What else needs to be saved  Only FPA stuff when that is supported */

	/* r1 now free! */

	/* Third phase : restore saved context */

	/* rem: r0 = old proc */
	/* rem: r6 = new process */
	/* rem: interrupts are enabled */

	/*
	 * Don't allow user space access between the purge and the switch.
	 */
	ldr	r3, .Lblock_userspace_access
	mov	r1, #0x00000001
	mov	r2, #0x00000000
	str	r1, [r3]

	stmfd	sp!, {r0-r3}
	ldr	r1, .Lcpufuncs
	add	lr, pc, #.Lcs_cache_purged - . - 8
	ldr	pc, [r1, #CF_IDCACHE_WBINV_ALL]

.Lcs_cache_purged:
	ldmfd	sp!, {r0-r3}

.Lcs_cache_purge_skipped:
	/* At this point we need to kill IRQ's again. */
	IRQdisable

	/*
	 * Interrupts are disabled so we can allow user space accesses again
	 * as none will occur until interrupts are re-enabled after the
	 * switch.
	 */
	str	r2, [r3]

	/* Get the user structure for the new process in r1 */
	ldr	r1, [r6, #(P_ADDR)]

	/* Get the pagedir physical address for the process. */
	ldr	r0, [r1, #(PCB_PAGEDIR)]

	/* Switch the memory to the new process */
	ldr	r3, .Lcpufuncs
	add	lr, pc, #.Lcs_context_switched - . - 8
	ldr	pc, [r3, #CF_CONTEXT_SWITCH]
       
.Lcs_context_switched:
	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
        mrs	r3, cpsr
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE)
        msr	cpsr_c, r2

	ldr	sp, [r1, #(PCB_UND_SP)]

        msr	cpsr_c, r3		/* Restore the old mode */

	/* Restore all the save registers */
	add	r7, r1, #PCB_R8
	ldmia	r7, {r8-r13}

	mov	r7, r1			/* preserve PCB pointer */

#ifdef ARMFPE
	add	r0, r1, #(USER_SIZE) & 0x00ff
	add	r0, r0, #(USER_SIZE) & 0xff00 
	bl	_C_LABEL(arm_fpe_core_changecontext)
#endif

	/* We can enable interrupts again */
	IRQenable

	/* rem: r6 = new proc */
	/* rem: r7 = new PCB */

	/* 
	 * Check for restartable atomic sequences (RAS).
	 */

	ldr	r2, [r6, #(P_NRAS)]
	ldr	r4, [r7, #(PCB_TF)]	/* r4 = trapframe (used below) */
	teq	r2, #0			/* p->p_nras == 0? */
	bne	.Lswitch_do_ras		/* no, check for one */

.Lswitch_return:

	/* Get the spl level from the stack and update the current spl level */
	ldr	r0, [sp], #0x0004
	bl	_C_LABEL(splx)

	/* cpu_switch returns the proc it switched to. */
	mov	r0, r6

	/*
	 * Pull the registers that got pushed when either savectx() or
	 * cpu_switch() was called and return.
	 */
	ldmfd	sp!, {r4-r7, pc}

.Lswitch_do_ras:
	ldr	r1, [r4, #(TF_PC)]	/* second ras_lookup() arg */
	mov	r0, r6			/* first ras_lookup() arg */
	bl	_C_LABEL(ras_lookup)
	cmn	r0, #1			/* -1 means "not in a RAS" */
	strne	r0, [r4, #(TF_PC)]
	b	.Lswitch_return

.Lswitch_exited:
	/*
	 * We skip the cache purge because switch_exit() already did
	 * it.  Load up registers the way Lcs_cache_purge_skipped
	 * expects.  Userspace access already blocked in switch_exit().
	 */
	ldr	r3, .Lblock_userspace_access
	mov	r2, #0x00000000
	b	.Lcs_cache_purge_skipped

/*
 * void switch_exit(struct proc *p, struct proc *p0);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */

/* LINTSTUB: Func: void switch_exit(struct proc *p, struct proc *p0) */
ENTRY(switch_exit)
	/*
	 * r0 = proc
	 * r1 = proc0
	 */

	mov	r3, r0

	/* In case we fault */
	ldr	r0, .Lcurproc
	mov	r2, #0x00000000
	str	r2, [r0]

/*	ldr	r0, .Lcurpcb
	str	r2, [r0]*/

	/*
	 * Don't allow user space access between the purge and the switch.
	 */
	ldr	r0, .Lblock_userspace_access
	mov	r2, #0x00000001
	str	r2, [r0]

	/* Switch to proc0 context */

	stmfd	sp!, {r0-r3}

	ldr	r0, .Lcpufuncs
	add	lr, pc, #.Lse_cache_purged - . - 8
	ldr	pc, [r0, #CF_IDCACHE_WBINV_ALL]

.Lse_cache_purged:
	ldmfd	sp!, {r0-r3}

	IRQdisable

	ldr	r2, [r1, #(P_ADDR)]
	ldr	r0, [r2, #(PCB_PAGEDIR)]

	/* Switch the memory to the new process */
	ldr	r4, .Lcpufuncs
	add	lr, pc, #.Lse_context_switched - . - 8
	ldr	pc, [r4, #CF_CONTEXT_SWITCH]
       
.Lse_context_switched:
	/* Restore all the save registers */
	add	r7, r2, #PCB_R8
	ldmia	r7, {r8-r13}

	/* This is not really needed ! */
	/* Yes it is for the su and fu routines */
	ldr	r0, .Lcurpcb
	str	r2, [r0]

	IRQenable

/*	str	r3, [sp, #-0x0004]!*/

	/*
	 * Schedule the vmspace and stack to be freed.
	 */
	mov	r0, r3			/* exit2(p) */
	bl	_C_LABEL(exit2)

	/* Paranoia */
	ldr	r1, .Lcurproc
	mov	r0, #0x00000000
	str	r0, [r1]

	ldr	r7, .Lwhichqs		/* r7 = &whichqs */
	mov	r5, #0x00000000		/* r5 = old proc = NULL */
	b	.Lswitch_search

/* LINTSTUB: Func: void savectx(struct pcb *pcb) */
ENTRY(savectx)
	/*
	 * r0 = pcb
	 */

	/* Push registers.*/
	stmfd	sp!, {r4-r7, lr}

	/* Store all the registers in the process's pcb */
	add	r2, r0, #(PCB_R8)
	stmia	r2, {r8-r13}

	/* Pull the regs of the stack */
	ldmfd	sp!, {r4-r7, pc}

ENTRY(proc_trampoline)
	add	lr, pc, #(.Ltrampoline_return - . - 8)
	mov	r0, r5
	mov	r1, sp
	mov	pc, r4

.Ltrampoline_return:
	/* Kill irq's */
        mrs     r0, cpsr
        orr     r0, r0, #(I32_bit)
        msr     cpsr_c, r0

	PULLFRAME

	movs	pc, lr			/* Exit */

	.type .Lcpu_switch_ffs_table, _ASM_TYPE_OBJECT;
.Lcpu_switch_ffs_table:
/* same as ffs table but all nums are -1 from that */
/*               0   1   2   3   4   5   6   7           */
	.byte	 0,  0,  1, 12,  2,  6,  0, 13  /*  0- 7 */
	.byte	 3,  0,  7,  0,  0,  0,  0, 14  /*  8-15 */
	.byte	10,  4,  0,  0,  8,  0,  0, 25  /* 16-23 */
	.byte	 0,  0,  0,  0,  0, 21, 27, 15  /* 24-31 */
	.byte	31, 11,  5,  0,  0,  0,  0,  0	/* 32-39 */
	.byte	 9,  0,  0, 24,  0,  0, 20, 26  /* 40-47 */
	.byte	30,  0,  0,  0,  0, 23,  0, 19  /* 48-55 */
	.byte   29,  0, 22, 18, 28, 17, 16,  0  /* 56-63 */

/* End of cpuswitch.S */
