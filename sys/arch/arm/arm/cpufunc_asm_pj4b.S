/*	$NetBSD: cpufunc_asm_pj4b.S,v 1.2.2.3 2014/08/20 00:02:44 tls Exp $ */

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

#define TTB_RGN_OC_WB	(3 << 3)
#define TTB_IRGN_WB	((1 << 0) | (1 << 6))

/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_FLAGS_UP	(TTB_IRGN_WB | TTB_RGN_OC_WB)

.Lpj4b_cache_line_size:
	.word	_C_LABEL(arm_dcache_align)

ENTRY(pj4b_setttb)
	/* Cache synchronization is not required as this core has PIPT caches */
	dsb
#if defined(L2CACHE_ENABLE) && defined(AURORA_L2_PT_WALK)
	orr	r2, r0, #TTB_FLAGS_UP
#else
	bic	r2, r0, #0x18
#endif
	mcr	p15, 0, r0, c2, c0, 0	/* load TTBR0 */
#ifdef ARM_MMU_EXTENDED
	cmp	r1, #0
	mcreq	p15, 0, r0, c2, c0, 1	/* load TTBR1 */
#else
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* invalidate I+D TLBs */
#endif
	isb
	dsb
	RET
END(pj4b_setttb)

ENTRY(pj4b_tlb_flushID)
	mcr	p15, 0, r0, c8, c7, 0	/* flush I+D tlb */
	dsb
	RET
END(pj4b_tlb_flushID)

ENTRY(pj4b_tlb_flushID_SE)
	mcr	p15, 0, r0, c8, c7, 1	@flush I+D tlb single entry
#if PAGE_SIZE == 2 * L2_S_SIZE
	add	r0, r0, L2_S_SIZE
	mcr	p15, 0, r0, c8, c7, 1	@flush I+D tlb single entry
#endif
	dsb
	RET
END(pj4b_tlb_flushID_SE)

ENTRY(pj4b_icache_sync_range)
	ldr	ip, .Lpj4b_cache_line_size
	ldr	ip, [ip]
	sub	r1, r1, #1		/* Don't overrun */
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c5, 1	/* Invalidate I cache SE with VA */
	mcr	p15, 0, r0, c7, c14, 1	/* Clean and invalidate D cache SE with VA */
	add	r0, r0, ip
	subs	r1, r1, ip
	bpl	1b
	isb				/* instruction synchronization barrier */
	dsb
	RET
END(pj4b_icache_sync_range)

ENTRY(pj4b_dcache_inv_range)
	ldr	ip, .Lpj4b_cache_line_size
	ldr	ip, [ip]
	sub	r1, r1, #1		/* Don't overrun */
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c6, 1
	add	r0, r0, ip
	subs	r1, r1, ip
	bpl	1b
	dsb
	RET
END(pj4b_dcache_inv_range)

ENTRY(pj4b_idcache_wbinv_range)
	ldr	ip, .Lpj4b_cache_line_size
	ldr	ip, [ip]
	sub	r1, r1, #1		/* Don't overrun */
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c5, 1
	mcr	p15, 0, r0, c7, c14, 1	/* L2C clean and invalidate entry */
	add	r0, r0, ip
	subs	r1, r1, ip
	bpl	1b
	dsb
	RET
END(pj4b_idcache_wbinv_range)

ENTRY(pj4b_dcache_wbinv_range)
	ldr	ip, .Lpj4b_cache_line_size
	ldr	ip, [ip]
	sub	r1, r1, #1		/* Don't overrun */
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c14, 1
	add	r0, r0, ip
	subs	r1, r1, ip
	bpl	1b
	dsb
	RET
END(pj4b_dcache_wbinv_range)

ENTRY(pj4b_dcache_wb_range)
	ldr	ip, .Lpj4b_cache_line_size
	ldr	ip, [ip]
	sub	r1, r1, #1		/* Don't overrun */
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c14, 1	/* Clean and invalidate D cache SE with VA */
	add	r0, r0, ip
	subs	r1, r1, ip
	bpl	1b
	dsb
	RET
END(pj4b_dcache_wb_range)

ENTRY(pj4b_drain_readbuf)
	isb
	RET
END(pj4b_drain_readbuf)

ENTRY(pj4b_drain_writebuf)
	dsb
	RET
END(pj4b_drain_writebuf)

ENTRY(pj4b_flush_brnchtgt_all)
	mcr	p15, 0, r0, c7, c5, 6	/* flush entrie branch target cache */
	RET
END(pj4b_flush_brnchtgt_all)

ENTRY(pj4b_flush_brnchtgt_va)
	mcr	p15, 0, r0, c7, c5, 7	/* flush branch target cache by VA */
	RET
END(pj4b_flush_brnchtgt_va)

ENTRY(pj4b_context_switch)
	dsb
#if defined(L2CACHE_ENABLE) && defined(AURORA_L2_PT_WALK)
	orr	r2, r0, #TTB_FLAGS_UP
#else
	bic	r2, r0, #0x18
#endif
	mcr	p15, 0, r2, c2, c0, 0 	@set the new TTBR0
#ifdef ARM_MMU_EXTENDED
	cmp	r1, #0
	mcreq	p15, 0, r2, c2, c0, 1 	@set the new TTBR1
#else
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@flush the I+D
#endif
	dsb
	isb
	RET
END(pj4b_context_switch)

ENTRY(pj4b_config)
	/* Set Auxiliary Debug Modes Control 2 register */
	mrc	p15, 1, r0, c15, c1, 2
	bic	r0, r0, #(1 << 23)
	orr	r0, r0, #(1 << 25)
	orr	r0, r0, #(1 << 27)
	orr	r0, r0, #(1 << 29)
	orr	r0, r0, #(1 << 30)
	mcr	p15, 1, r0, c15, c1, 2
	RET
END(pj4b_config)
