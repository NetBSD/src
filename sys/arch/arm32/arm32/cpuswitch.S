/* $NetBSD: cpuswitch.S,v 1.3 1996/03/08 20:03:12 mark Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
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
 * cpu.S
 *
 * cpu switching functions
 *
 * Created      : 15/10/94
 */

#include "assym.h"
#include <machine/param.h>
#include <machine/cpu.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

.text


Lwhichqs:
	.word	_whichqs

Lqs:
	.word	_qs

	.global	_setrunqueue

_setrunqueue:
/*
 * r0 = proc
 * r1 = queue
 * r2 = &qs[queue]
 * r3 = temp
 */
	stmfd	sp!, {r4}

/* diags */

#ifdef DIAGNOSTIC
	ldr	r1, [r0, #(P_BACK)]
	teq	r1, #0x00000000
	bne	Lsetrunqueue_erg

	ldr	r1, [r0, #(P_WCHAN)]
	teq	r1, #0x00000000
	bne	Lsetrunqueue_erg
#endif

/* Get the priority of the queue */

	ldrb	r1, [r0, #(P_PRIORITY)]
	mov	r1, r1, lsr #2

/* Indicate that there is a process on this queue */

	ldr	r4, Lwhichqs
	ldr	r2, [r4]
	mov	r3, #0x00000001
	mov	r3, r3, lsl r1
	orr	r2, r2, r3
	str	r2, [r4]

/* Get the address of the queue */

	ldr	r2, Lqs
	add	r1, r2, r1, lsl # 3

/* Hook the process in */
	str	r1, [r0, #(P_FORW)]
	ldr	r2, [r1, #(P_BACK)]

	str	r0, [r1, #(P_BACK)]
#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	Lsetrunqueue_erg
#endif
	str	r0, [r2, #(P_FORW)]
	str	r2, [r0, #(P_BACK)]

	ldmfd	sp!, {r4}

	mov	pc, lr

#ifdef DIAGNOSTIC
Lsetrunqueue_erg:
	mov	r2, r1
	mov	r1, r0
	add	r0, pc, #Ltext1 - . - 8
	bl	_printf

	ldr	r2, Lqs
	ldr	r1, [r2]
	add	r0, pc, #Ltext2 - . - 8
	b	_panic	

Ltext1:
	.asciz	"setrunqueue : %08x %08x\n"
Ltext2:
	.asciz	"setrunqueue : [qs]=%08x qs=%08x\n"
	.align	0
#endif

	.global	_remrq
_remrq:

/*
 * r0 = oldproc
 * r1 = queue
 * r2 = &qs[queue]
 */
	stmfd	sp!, {r4}

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

	ldreq	r4, Lwhichqs
	ldreq	r2, [r4]
	moveq	r3, #0x00000001
	moveq	r3, r3, lsl r1
	biceq	r2, r2, r3
	streq	r2, [r4]

/* Remove the back pointer for the process */

	mov	r1, #0x00000000
	str	r1, [r0, #(P_BACK)]

	ldmfd	sp!, {r4}

	mov	pc, lr

Lcurproc:
	.word	_curproc

	.global	_curpcb

_curpcb:
	.word	0x00000000

Lcurpcb:
	.word	_curpcb

Lwant_resched:
	.word	_want_resched

	.global	_idle
_idle:
idle:

/* Enable IRQ's and set the spl level to 0 to allow all interrupts */

	IRQenable

idleloop:

#ifdef CPU_ARM7500
/* ARM7500 has a suspend mode so use it ! */

	mov	r7, #(IOMD_BASE)
	orr	r7, r7, #(IOMD_SUSPEND - IOMD_BASE)
	mov	r3, #0x00000001
	strb	r3, [r7]
#endif

/* Disable interrupts while we check for an active queue */

	IRQdisable
	ldr	r7, Lwhichqs
	ldr	r3, [r7]
	teq	r3, #0x00000000
	bne	sw1

/* Nothing active yet. Handle IRQ's again and loop back */

  	IRQenable
	b	idleloop
	
	.global	_cpu_switch

_cpu_switch:
/*
 * r1 = oldproc
 * r2 = spl level
 * r3 = whichqs
 * r4 = queue
 * r5 = &qs[queue]
 * r6 = newproc
 * r7 = scratch
 */
	stmfd	sp!, {r4-r7, r8-r12, lr}

/*
 * Get the current process and indicate that there is no longer a valid
 * process (curproc = 0)
 */

	ldr	r7, Lcurproc
	ldr	r1, [r7]
	mov	r0, #0x00000000
	str	r0, [r7]

/* Zero the pcb */

	ldr	r7, Lcurpcb
	str	r0, [r7]

/* Lower the spl level to spl0 and get the current spl level. */

	mov	r7, r1

#ifdef spl0
	mov	r0, #(SPL_0)
	bl	_splx
#else
	bl	_spl0
#endif

	str	r0, [sp, #-0x0004]!

	mov	r1, r7

/* First phase : find a new process */

switch_search:
	IRQdisable

/* Do we have any active queues  */

	ldr	r7, Lwhichqs
	ldr	r3, [r7]

/* If not we must idle until we do. */

	teq	r3, #0x00000000
	beq	idle

sw1:
/*
 * Paranoid debug time ....
 * Is this overkill ? If we are not in SVC mode then things are
 * very sick and will probably have already died.
 */

#ifdef DIAGNOSTIC
	mrs	r4, cpsr_all
	and	r4, r4, #(PSR_MODE)
	teq	r4, #(PSR_SVC32_MODE)
	beq	switchmodeok

	add	r0, pc, #switchpanic - . - 8
	mrs	r4, cpsr_all
	bl	_panic

switchpanic:
	.asciz	"Yikes! In cpu_switch() but not in SVC mode (%08x)\n"
	.align	0

switchmodeok:
#endif

/*
 * We have found an active queue. Currently we do not know which queue
 * is active just that one of them is.
 * We must check each queue to find the active one.
 * r3 contains a bit for each of the 32 queues. A one indicates that
 * that the queue has something in it.
 */

	mov	r4, #0x00000000

findqueue:
	mov	r0, #0x00000001
	mov	r0, r0, lsl r4

	tst	r3, r0
	addeq	r4, r4, #0x00000001
	beq	findqueue

/*
 * Ok we have found the active queue. The above code can never fail as
 * to get to it IRQ's must be disabled and whichqs != 0.
 * r4 contains the number of the first queue found with a process in it.
 */

/* Get the address of the queue (&qs[queue]) */

	ldr	r5, Lqs
	add	r5, r5, r4, lsl #3

/*
 * Get the process from the queue and place the next process in the queue
 * at the head. This basically unlinks the process at the head of the queue.
 */
	ldr	r6, [r5, #(P_FORW)]

	ldr	r7, [r6, #(P_FORW)]
	str	r7, [r5, #(P_FORW)]	

/*
 * Test to see if the queue is now empty. If the head of the queue points
 * to the queue itself then there are no more processes in the queue.
 * We can therefore clear the queue not empty flag held in r3.
 */

	teq	r5, r7
	biceq	r3, r3, r0

/* Fix the back pointer for the process now at the head of the queue. */

	ldr	r0, [r6, #(P_BACK)]
	str	r0, [r7, #(P_BACK)]

/* Update the RAM copy of the queue not empty flags word. */

	ldr	r7, Lwhichqs
	str	r3, [r7]

/* Clear the want_resched flag */

	mov	r0, #0x00000000
	ldr	r7, Lwant_resched
	str	r0, [r7]

/*
 * Clear the back pointer of the process we have removed from the head
 * of the queue. The new process is isolated now.
 */

	mov	r0, #0x00000000
	str	r0, [r6, #(P_BACK)]

/* We have a new curproc now so make a note it */

	ldr	r7, Lcurproc
	str	r6, [r7]

/* Hook in a new pcb */

	ldr	r7, Lcurpcb
	ldr	r0, [r6, #(P_ADDR)]
	str	r0, [r7]

/* At this point we can allow IRQ's again. */

/*
	IRQenable
	IRQdisable
*/

/*
 * If the new process is the same as the process that called cpu_switch
 * Then we do not need to save and restore any contexts. This means
 * we can make a quick exit.
 * The test is simple if curproc on entry (now in r1) is the same as the
 * proc removed from the queue we can jump to the exit.
 */

	teq	r1, r6
	beq	switch_return

/*
 * If the curproc on entry to cpu_switch was zero then the process that
 * called it was exiting. This means that we do not need to save the current
 * context. Instead we can jump straight to restoring the context for
 * the new process.
 */

	teq	r1, #0x00000000
	beq	switch_exited

/* Stage two : Save old context */

/* Remember the old process in r0 */

	mov	r0, r1

/* Get the user structure for the old process. */

	ldr	r1, [r1, #(P_ADDR)]

/* Save all the registers in the old process's pcb */

	add	r7, r1, #(PCB_R0)
	stmia	r7, {r0-r15}

/* Don't really need to do this. as the stmia above does it */

	str	sp, [r1, #(PCB_SP)]	

/*
 * This can be optimised... We know we want to go from SVC32 mode to UND32
 * mode
 */

        mrs	r3, cpsr_all
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE)
        msr	cpsr_all, r2

	str	sp, [r1, #(PCB_UND_SP)]

        msr	cpsr_all, r3		/* Restore the old mode */

/* What else needs to be saved  Only FPA stuff when that is supported */

/* Third phase : restore saved context */

switch_exited:

/* At this point we need to kill IRQ's again. */

	mrs	r0, cpsr_all
	orr	r0, r0, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

/*	IRQdisable*/

/* Get the user structure for the new process in r1 */

	ldr	r1, [r6, #(P_ADDR)]

/* Get the pagedir physical address for the process. */

	ldr	r0, [r1, #(PCB_PAGEDIR)]

/* Switch the memory to the new process */

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Write the TTB */
        mcr     15, 0, r0, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c5, c0, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0

/*
 * This can be optimised... We know we want to go from SVC32 mode to UND32
 * mode
 */

        mrs	r3, cpsr_all
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE)
        msr	cpsr_all, r2

	ldr	sp, [r1, #(PCB_UND_SP)]

        msr	cpsr_all, r3		/* Restore the old mode */

/* Restore all the save registers */

	add	r7, r1, #PCB_R8
	ldmia	r7, {r8-r14}

/* This is not really needed ! */

	ldr	sp, [r1, #(PCB_SP)]

/* Remember the pcb currently in use */

	ldr	r7, Lcurpcb
	str	r1, [r7]

/* We can enable interrupts again */

/*
	IRQenable
*/

#ifdef ARMFPE
	add	r0, r1, #(USER_SIZE) & 0x00ff
	add	r0, r0, #(USER_SIZE) & 0xff00 
	bl	_arm_fpe_core_changecontext
#endif

switch_return:

/* We have a new curproc now so make a note it */

/*
	ldr	r7, Lcurproc
	str	r6, [r7]
*/

/* Get the spl level from the stack and update the current spl level */

	ldr	r0, [sp], #0x0004
	bl	_splx

/*	IRQenable*/

/* cpu_switch returns the proc it switched to. */

	mov	r0, r6

/*
 * Pull the registers that got pushed when either savectx or cpu_switch
 * was called.
 */
	ldmfd	sp!, {r4-r7, r8-r12}

/* Return to the new process */

	ldmfd	sp!, {pc}

Ltext30:
	.asciz	"nkt: cpu_switch: sp=%08x pcb=%08x\n"
	.align	0

Ltext31:
	.asciz	"nkt: process facing firing squad\ncpu_switch: sp=%08x saved sp=%08x\n"
	.align	0

Ltext32:
	.asciz	"nkt: process checksum failure was %08x instead of %08x\n"
	.align	0

Lproc0:
	.word	_proc0

Lkernel_map:
	.word	_kernel_map

Lkstack:
	.word	_kstack


	.global	_switch_exit

_switch_exit:

/*
 * r0 = proc
 * r1 = proc0
 */

 	ldr	r1, Lproc0

/* In case we fault */

	mov	r2, #0x00000000
	ldr	r3, Lcurproc
	str	r2, [r3]

/*	ldr	r3, Lcurpcb
	str	r2, [r3]*/

/* Switch to proc0 context */

	IRQdisable

	ldr	r2, [r1, #(P_ADDR)]
	ldr	r3, [r2, #(PCB_PAGEDIR)]

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Write the TTB */
        mcr     15, 0, r3, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c5, c0, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0

/* Restore all the save registers */

	add	r7, r2, #PCB_R8
	ldmia	r7, {r8-r14}

/* This is not really needed ! */
/* Yes it is for the su and fu routines */

	ldr	sp, [r2, #(PCB_SP)]
	ldr	r3, Lcurpcb
	str	r2, [r3]

/*	IRQenable*/

	str	r0, [sp, #-0x0004]!

/* Thoroughly nuke the old process's resources. */

/* This has to be done here, before we lose the pmap */

	mov	r1, #0x00000000
	add	r2, r1, #NBPG
	ldr	r0, [r0, #(P_VMSPACE)]
	add	r0, r0, #(VM_PMAP)
	bl	_pmap_remove
	ldr	r0, [sp]

/*
 * Have top wait until we have switched to proc0 as the pmap gets released
 * in vmspace_free()
 */

	ldr	r0, [r0, #(P_VMSPACE)]
	bl	_vmspace_free

/* This has to be done here */

	mov	r2, #(UPAGES << PGSHIFT)
	ldr	r0, [sp], #0x0004
	ldr	r1, [r0, #(P_ADDR)]
	ldr	r0, Lkernel_map
	ldr	r0, [r0]
	bl	_kmem_free

/* Paranoia */

	mov	r0, #0x00000000
	ldr	r1, Lcurproc
	str	r0, [r1]

        ldr     r1, Lproc0
	b	switch_search


Lcurrent_spl_level:
	.word	_current_spl_level
 
	.global	_savectx

_savectx:
/*
 * r0 = p2->p_addr
 * r1 = altreturn
 */

/* Push registers. These must match those pushed by cpu_switch */

	stmfd	sp!, {r4-r7, r8-r12, lr}

/* Push the current spl level onto the stack */

	ldr	r2, Lcurrent_spl_level
	ldr	r2, [r2] 

	str	r2, [sp, #-0x0004]!

/* Store all the registers in the process's pcb */

	add	r2, r0, #(PCB_R0)
	stmia	r2, {r0-r15}

/*
 * Calulcate the offset of the stack pointer from the beginning of the
 * user area.
 */

	mov	r2, sp
	ldr	r3, Lkstack
	ldr	r3, [r3]

	sub	r3, r2, r3
	str	r2, [r0, #(PCB_SP)]

	add	r2, r0, r3

	teq	r1, #0x00000000
	beq	savectx_returnasparent

/*
 * If we are clever we will only copy the used bit of the stack
 * r3 contains the offset from the start of p_addr
 */

/*
 * r3 = offset from p->p_addr to bottom of stack
 * r0 = p2->p_addr
 * r2 = p2 stack pointer
 * sp = current stack pointer 
 * We dont have to worry about a zero length copy as this routine
 * pushes stuff on the stack !
 */

/* We copy from sp to r2 for ((UPAGES * NBPG) - r3) */

/* This needs to change - should be USPACE_SVC_STACK_TOP not UPAGES * NBPG */

	mov	r1, #(UPAGES * NBPG)
	sub	r3, r1, r3

	mov	r1, #0x00000000

savectxloop:
	ldr	r0, [sp, r1]
	str	r0, [r2, r1]
	add	r1, r1, #0x00000004
	subs	r3, r3, #0x00000004
	bne	savectxloop

savectx_returnasparent:
	mov	r0, #0x00000000

/* Skip the pushed spl mask */

	add	sp, sp, #0x0000004

/* Pull the regs of the stack */

	ldmfd	sp!, {r4-r7, r8-r12}
	ldmfd	sp!, {pc}
