/* $NetBSD: cpu.h,v 1.2 1996/02/01 22:29:34 mycroft Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
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
 * Last updated : 01/07/95
 *
 * Based on kate/katelib/arm6.h
 *
 *    $Id: cpu.h,v 1.2 1996/02/01 22:29:34 mycroft Exp $
 */

#ifndef _ARM_CPU_H
#define _ARM_CPU_H

#ifndef _LOCORE
#  include <machine/frame.h>
#  include <machine/psl.h>
#endif


/*
 * If we are not an ARM6 then we MUST use late aborts as only the ARM6
 * supports early aborts.
 * For the ARM6 we will use early abort unless otherwise configured
 * This reduces the overheads of LDR/STR aborts and no correction is required.
 */

#ifndef CPU_ARM6
#  define CPU_LATE_ABORTS
#endif

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

#define I32_bit (1 << 7)
#define F32_bit (1 << 6)

#define PSR_MODE	0x0000001f
#define PSR_USR32_MODE	0x00000010
#define PSR_FIQ32_MODE	0x00000011
#define PSR_IRQ32_MODE	0x00000012
#define PSR_SVC32_MODE	0x00000013
#define PSR_ABT32_MODE	0x00000017
#define PSR_UND32_MODE	0x0000001b

#define CPU_ID_DESIGNER_MASK	0xff000000
#define CPU_ID_ARM_LTD		0x41000000
#define CPU_ID_MAKER_MASK	0x00ff0000
#define CPU_ID_GPS		0x00560000
#define CPU_ID_VLSI		0x00000000
#define CPU_ID_CPU_MASK		0x0000fff0
#define ID_ARM610		0x00000610
#define ID_ARM700		0x00007000
#define ID_ARM710		0x00007100
#define CPU_ID_REVISION_MASK	0x0000000f

#define CPU_CONTROL_MMU_ENABLE  0x0001
#define CPU_CONTROL_AFLT_ENABLE 0x0002
#define CPU_CONTROL_IDC_ENABLE  0x0004
#define CPU_CONTROL_WBUF_ENABLE 0x0008
#define CPU_CONTROL_32BP_ENABLE 0x0010
#define CPU_CONTROL_32BD_ENABLE 0x0020
#define CPU_CONTROL_LABT_ENABLE 0x0040
#define CPU_CONTROL_BEND_ENABLE 0x0080
#define CPU_CONTROL_SYST_ENABLE 0x0100
#define CPU_CONTROL_ROM_ENABLE	0x0200
#define CPU_CONTROL_CPCLK	0x0400

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
	orr	r0, r0, #(I32_bit | F32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}

#define IRQenable \
	stmfd	sp!, {r0} ; \
	mrs	r0, cpsr_all ; \
	bic	r0, r0, #(I32_bit | F32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}		

#else
#  define IRQdisable SetCPSR(I32_bit | F32_bit, I32_bit | F32_bit);
#  define IRQenable SetCPSR(I32_bit | F32_bit, 0);
#endif

/*
 * Return TRUE/FALSE (1/0) depending on whether the frame came from USR
 * mode or not.
 */
 
#define CLKF_USERMODE(frame) ((frame->if_spsr & PSR_MODE) == PSR_USR32_MODE)

#define CLKF_BASEPRI(frame) (1)

#define CLKF_PC(frame) (frame->if_pc)

#define CLKF_INTR(frame) ((frame->if_spsr & PSR_MODE) == PSR_IRQ32_MODE)

/* Hack to monitor FPE this requires sys/user.h to be included in kern/kern_clock.h */
/*#define CLKF_INTR(frame) (p && (p->p_addr->u_pcb.pcb_fpstate.fp_flags & 2) != 0)*/

#define cpu_set_init_frame(p, frame) (p->p_md.md_regs = frame)

#define DELAY(x)	delay(x)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#define cpu_wait(p)	/* nothing */

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define signotify(p)            setsoftast()
    
#endif

/* End of cpu.h */
