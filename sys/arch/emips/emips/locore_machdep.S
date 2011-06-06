/*	$NetBSD: locore_machdep.S,v 1.1.8.2 2011/06/06 09:05:17 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

/*
 * Access to the eMIPS special registers.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	noreorder
	.set	mips32

#define MIPS_COP_0_EXT_CTL  _(22)
#define MIPS_COP_0_EXT_MISS _(23)
#define MIPS_COP_0_EXT_HIT  _(24)

/*
 * Extension control register bits, definitions and semantics
 */
/* The Opcode field holds the opcode assigned to the given Extension slot (Z).
 * When this field is written to, the CAM used for decoding the Extension Instructions is updated
 * (in CAM Decoding mode).  The value can be one of the eight Extension Opcodes, or any other opcode
 * iff the Extension has priority and is meant to mask an existing instruction.
 */
#define EXTCTL_OP       0xfc000000
/* The Kernel/User flag denotes that Extension Z is a Kernel[0] or User[1] mode Extension.
 */
#define EXTCTL_KU       0x02000000
/* The Scope flag denotes that Extension Z is a Local[0] (per process) or Global[1] (shared) Extension.
 */
#define EXTCTL_SC       0x01000000
/* The Peripheral flag denotes that Extension Z is a Peripheral Extension.
 */
#define EXTCTL_PER      0x00080000
/* The Interrupt flag denotes that Extension Z has an interrupt.
 */
#define EXTCTL_INT      0x00040000
/* The Virtual/Physical flag denotes that Extension Z uses Virtual[0] or Physical[0] addresses 
 * when accessing the memory bus.
 */
#define EXTCTL_VP       0x00020000
/* The State field denotes the state of Extension Z.  (Loaded, Config, Running, Suspend, etc)
 */
#define EXTCTL_ST       0x0000f000
/* The Priority field denotes the execution priority for Extension Z during arbitration.
 */
#define EXTCTL_PR       0x00000f00
/* The Privileged flag denotes that Extension Z has access to security sensitive system resources.
 */
#define EXTCTL_PRV      0x00000080
/* The Trap flag denotes whether an RI exception will be generated for an Extension Z's instruction 
 * if Extension Z is disabled.
 */
#define EXTCTL_TR       0x00000010
/* The Trapped flag denotes that a trap occurred during Extension Z's last execution and it was unable
 * to complete.
 */
#define EXTCTL_TD       0x00000008
/* The Clock Enable flag denotes that the clock for Extension Z is active.
 */
#define EXTCTL_CE       0x00000004
/* The Enable flag denotes that Extension Z is enabled for execution.
 */
#define EXTCTL_EN       0x00000002
/* The Loaded flag denotes that Extension Z has been loaded
 */
#define EXTCTL_LD       0x00000001

/*
 * unsigned int acc_get_misses(int op_number);
 *
 * Get the number of misses for the given opcode on the current processor
 * nb: clears the counter too
 */
LEAF(acc_get_misses)
	    la      v0,1f
        andi    a0,a0,7
        sll     a0,a0,3
        addu    a0,v0,a0
        jr      a0
        nop
1:
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 0
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 1
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 2
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 3
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 4
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 5
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 6
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_MISS, 7
END(acc_get_misses)

/*
 * unsigned int acc_get_hits(int slot_number);
 *
 * Get the number of hits for the given accelerator slot on the current processor
 * nb: clears the counter too
 */
LEAF(acc_get_hits)
	    la      v0,1f
        andi    a0,a0,7
        sll     a0,a0,3
        addu    a0,v0,a0
        jr      a0
        nop
1:
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 0
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 1
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 2
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 3
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 4
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 5
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 6
        j       ra
        mfc0    v0, MIPS_COP_0_EXT_HIT, 7
END(acc_get_hits)

/*
 * unsigned int acc_activate(int slot_number, uint32_t value);
 *
 * Make the given accelerator slot in/active
 * Returns the previous value in the control register
 */
LEAF(acc_activate)
	    la      v0,1f
        andi    a0,a0,7
        sll     a0,a0,4
        addu    a0,v0,a0
        jr      a0
        nop
1:
        mfc0    v0, MIPS_COP_0_EXT_CTL, 0
        mtc0    a1, MIPS_COP_0_EXT_CTL, 0
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 1
        mtc0    a1, MIPS_COP_0_EXT_CTL, 1
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 2
        mtc0    a1, MIPS_COP_0_EXT_CTL, 2
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 3
        mtc0    a1, MIPS_COP_0_EXT_CTL, 3
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 4
        mtc0    a1, MIPS_COP_0_EXT_CTL, 4
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 5
        mtc0    a1, MIPS_COP_0_EXT_CTL, 5
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 6
        mtc0    a1, MIPS_COP_0_EXT_CTL, 6
        j       ra
        nop

        mfc0    v0, MIPS_COP_0_EXT_CTL, 7
        mtc0    a1, MIPS_COP_0_EXT_CTL, 7
        j       ra
        nop
END(acc_activate)
