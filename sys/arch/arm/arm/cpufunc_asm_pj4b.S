/*	$NetBSD: cpufunc_asm_pj4b.S,v 1.7 2015/04/15 10:52:18 hsuenaga Exp $ */

/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "assym.h"
#include <arm/asm.h>
#include <arm/locore.h>

.Lpj4b_cache_line_size:
	.word	_C_LABEL(arm_dcache_align)

ENTRY(pj4b_cpu_sleep)
	dsb
	wfi				@ wait for an interrupt
	dsb				@ Erratum#ARM-CPU-4742
	b	irq_idle_entry		@ assume we got an interrupt
END(pj4b_cpu_sleep)

ENTRY(pj4b_config)
	/* Set Auxiliary Debug Modes Control 0 register */
	mrc	p15, 1, r0, c15, c1, 0
	bic	r0, r0, #(1 << 12)	@ Erratum#ARM-CPU-6136
					@ LDSTM 1st issue is single word
	orr	r0, r0, #(1 << 22)	@ DVM_WAKEUP enable
	mcr	p15, 1, r0, c15, c1, 0

	/* Set Auxiliary Debug Modes Control 1 register */
	mrc	p15, 1, r0, c15, c1, 1
	bic	r0, r0, #(1 << 2)	@ Erratum#ARM-CPU-6409
					@ Disable static branch prediction
	orr	r0, r0, #(1 << 5)	@ STREX backoff disable
	mcr	p15, 1, r0, c15, c1, 1

	/* Set Auxiliary Debug Modes Control 2 register */
	mrc	p15, 1, r0, c15, c1, 2
	bic	r0, r0, #(1 << 23)	@ Enable fast LDR
	orr	r0, r0, #(1 << 25)	@ Intervention Interleave disable
	orr	r0, r0, #(1 << 27)	@ Critical word 1st sequencing dis.
	orr	r0, r0, #(1 << 29)	@ Disable MO device R/W
	orr	r0, r0, #(1 << 30)	@ L1 cache strict round-robin 
	orr	r0, r0, #(1 << 31)	@ Enable write evict
	mcr	p15, 1, r0, c15, c1, 2

	/* Set Auxiliary FUnction Modes Control 0 register */
	mrc	p15, 1, r0, c15, c2, 0
#ifdef MULTIPROCESSOR
	orr	r0, r0, #(1 << 1)	@ SMP/nAMP enable
#endif
	orr	r0, r0, #(1 << 2)	@ L2 parity enable
	orr	r0, r0, #(1 << 8)	@ Cache & TLB maintenance broadcast
	mcr	p15, 1, r0, c15, c2, 0

	RET
END(pj4b_config)
