/*	$NetBSD: cpu.h,v 1.23 2000/12/12 05:21:03 mycroft Exp $	*/

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
 * cpu.h
 *
 * CPU specific symbols
 *
 * Created      : 18/09/94
 *
 * Based on kate/katelib/arm6.h
 */

#ifndef _ARM32_CPU_H_
#define _ARM32_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_cputypes.h"
#include "opt_lockdebug.h"
#endif

#ifndef _LOCORE
#include <machine/frame.h>
#endif	/* !_LOCORE */
#include <machine/psl.h>

#if defined(CPU_ARM7500) && !defined(CPU_ARM7)
#error "option CPU_ARM7 is required with CPU_ARM7500"
#endif

#ifdef CPU_ARM7500
#ifdef CPU_ARM6
#error "CPU options CPU_ARM6 and CPU_ARM7500 are not compatible"
#endif
#ifdef CPU_ARM8
#error "CPU options CPU_ARM8 and CPU_ARM7500 are not compatible"
#endif
#ifdef CPU_SA110
#error "CPU options CPU_SA110 and CPU_ARM7500 are not compatible"
#endif
#endif /* CPU_ARM7500 */

#define COPY_SIGCODE    /* copy sigcode above user stack in exec */
 
/*
 * ARM Process Status Register
 *
 * The picture in the ARM manuals looks like this:
 *       3 3 2 2 2                              
 *       1 0 9 8 7                                     8 7 6 5 4       0
 *      +-------+---------------------------------------+-+-+-+---------+
 *      | flags |                  reserved             |I|F| |M M M M M|
 *      |n z c v|                                       | | | |4 3 2 1 0|
 *      +-------+---------------------------------------+-+-+-+---------+
 */

#define	PSR_FLAGS 0xf0000000	/* flags */
#define PSR_N_bit (1 << 31)	/* negative */
#define PSR_Z_bit (1 << 30)	/* zero */
#define PSR_C_bit (1 << 29)	/* carry */
#define PSR_V_bit (1 << 28)	/* overflow */

#define I32_bit (1 << 7)	/* IRQ disable */
#define F32_bit (1 << 6)	/* FIQ disable */

#define PSR_MODE	0x0000001f	/* mode mask */
#define PSR_USR32_MODE	0x00000010
#define PSR_FIQ32_MODE	0x00000011
#define PSR_IRQ32_MODE	0x00000012
#define PSR_SVC32_MODE	0x00000013
#define PSR_ABT32_MODE	0x00000017
#define PSR_UND32_MODE	0x0000001b
#define PSR_32_MODE	0x00000010

#define PSR_IN_USR_MODE(psr)	(!((psr) & 3))		/* XXX */
#define PSR_IN_32_MODE(psr)	((psr) & PSR_32_MODE)

/*
 * ARM Instructions
 *
 *       3 3 2 2 2                              
 *       1 0 9 8 7                                                     0
 *      +-------+-------------------------------------------------------+
 *      | cond  |              instruction dependant                    |
 *      |c c c c|                                                       |
 *      +-------+-------------------------------------------------------+
 */

#define INSN_SIZE		4		/* Always 4 bytes */
#define INSN_COND_MASK		0xf0000000	/* Condition mask */
#define INSN_COND_AL		0xe0000000	/* Always condition */

/* Some of the definitions below need cleaning up for V3/V4 architectures */

#define CPU_ID_DESIGNER_MASK	0xff000000
#define CPU_ID_ARM_LTD		0x41000000
#define CPU_ID_DEC		0x44000000
#define CPU_ID_TYPE_MASK	0x00ff0000
#define CPU_ID_ARM		0x00560000
#define CPU_ID_ARM7500		0x00020000
#define CPU_ID_CPU_MASK		0x0000fff0
#define ID_ARM610		0x00000610
#define ID_ARM700		0x00007000
#define ID_ARM710		0x00007100
#define ID_ARM810		0x00008100
#define ID_SA110		0x0000a100
#define CPU_ID_REVISION_MASK	0x0000000f

#define CPU_CONTROL_MMU_ENABLE	0x0001
#define CPU_CONTROL_AFLT_ENABLE	0x0002
#define CPU_CONTROL_DC_ENABLE	0x0004
#define CPU_CONTROL_WBUF_ENABLE 0x0008
#define CPU_CONTROL_32BP_ENABLE 0x0010
#define CPU_CONTROL_32BD_ENABLE 0x0020
#define CPU_CONTROL_LABT_ENABLE 0x0040
#define CPU_CONTROL_BEND_ENABLE 0x0080
#define CPU_CONTROL_SYST_ENABLE 0x0100
#define CPU_CONTROL_ROM_ENABLE	0x0200
#define CPU_CONTROL_CPCLK	0x0400
#define CPU_CONTROL_BPRD_ENABLE 0x0800
#define CPU_CONTROL_IC_ENABLE   0x1000

#define CPU_CONTROL_IDC_ENABLE	CPU_CONTROL_DC_ENABLE

/* Fault status register definitions */

#define FAULT_TYPE_MASK 0x0f
#define FAULT_USER      0x10

#define FAULT_WRTBUF_0  0x00
#define FAULT_WRTBUF_1  0x02
#define FAULT_BUSERR_0  0x04
#define FAULT_BUSERR_1  0x06
#define FAULT_BUSERR_2  0x08
#define FAULT_BUSERR_3  0x0a
#define FAULT_ALIGN_0   0x01 
#define FAULT_ALIGN_1   0x03 
#define FAULT_BUSTRNL1  0x0c
#define FAULT_BUSTRNL2  0x0e
#define FAULT_TRANS_S   0x05
#define FAULT_TRANS_P   0x07
#define FAULT_DOMAIN_S  0x09
#define FAULT_DOMAIN_P  0x0b
#define FAULT_PERM_S    0x0d
#define FAULT_PERM_P    0x0f

#ifdef _LOCORE
#define IRQdisable \
	stmfd	sp!, {r0} ; \
	mrs	r0, cpsr_all ; \
	orr	r0, r0, #(I32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}

#define IRQenable \
	stmfd	sp!, {r0} ; \
	mrs	r0, cpsr_all ; \
	bic	r0, r0, #(I32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}		

#else
#define IRQdisable SetCPSR(I32_bit, I32_bit);
#define IRQenable SetCPSR(I32_bit, 0);
#endif	/* _LOCORE */

/*
 * Return TRUE/FALSE (1/0) depending on whether the frame came from USR
 * mode or not.
 */
 
#define CLKF_USERMODE(frame) ((frame->if_spsr & PSR_MODE) == PSR_USR32_MODE)

/*
 * This needs straighening, prob is the frame does not have info on the priority
 * a guess that needs trying is (current_spl_level == SPL0)
 */

#define CLKF_BASEPRI(frame) ((frame->if_spsr & PSR_MODE) == PSR_USR32_MODE)

#define CLKF_PC(frame) (frame->if_pc)

/*#define CLKF_INTR(frame) (current_intr_depth > 1)*/

/* Hack to treat FPE time as interrupt time so we can measure it */
#define CLKF_INTR(frame) ((current_intr_depth > 1) || (frame->if_spsr & PSR_MODE) == PSR_UND32_MODE)

#define	PROC_PC(p)	((p)->p_md.md_regs->tf_pc)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#ifndef _LOCORE
#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};
#ifdef _KERNEL
extern struct cpu_info cpu_info_store;
#define	curcpu()	(&cpu_info_store)
#endif /* _KERNEL */
#endif /* ! _LOCORE */

#define cpu_wait(p)	/* nothing */
#define	cpu_number()	0

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define signotify(p)            setsoftast()


#if defined(_KERNEL) && !defined(_LOCORE)
extern int current_intr_depth;

/* stubs.c */
void need_resched	__P((struct cpu_info *));
void need_proftick	__P((struct proc *p));

/* locore.S */
void atomic_set_bit	__P((u_int *address, u_int setmask));
void atomic_clear_bit	__P((u_int *address, u_int clearmask));

/* cpuswitch.S */
struct pcb;
void	savectx		__P((struct pcb *pcb));

/* ast.c */
void userret		__P((register struct proc *p));

/* machdep.h */
void bootsync		__P((void));

/* strstr.c */
char *strstr		__P((const char *s1, const char *s2));

/* syscall.c */
void child_return	__P((void *));

#endif	/* _KERNEL && !_LOCORE */

/* 
 * CTL_MACHDEP definitions.
 */
#define	CPU_DEBUG		1	/* int: misc kernel debug control */
#define	CPU_BOOTED_DEVICE	2	/* string: device we booted from */
#define	CPU_BOOTED_KERNEL	3	/* string: kernel we booted */
#define	CPU_CONSDEV		4	/* struct: dev_t of our console */
#define	CPU_MAXID		5	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "debug", CTLTYPE_INT }, \
	{ "booted_device", CTLTYPE_STRING }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}    

#endif /* !_ARM32_CPU_H_ */

/* End of cpu.h */
