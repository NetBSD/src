/*-
 * Copyright (c) 2010 Per Odlund <per.odlund@armagedon.se>
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

/* ARMv7 assembly functions for manipulating caches and other core functions.
 * Based on cpufuncs for v6 and xscale.
 */

#include "assym.h"
#include <machine/cpu.h>
#include <machine/asm.h>

#define ISB .word 0xF57FF06F;
#define DMB .word 0xF57FF05F;
#define DSB .word 0xF57FF04F;
#define WFI .word 0xC320F003;

#define DCACHE_SIZE		0x00008000

#define entrysize		#32


ENTRY(armv7_cpu_sleep)
    tst r0, #0x00000000 			@shouldn't sleep 0
    WFI 
    RET

ENTRY(armv7_wait)
    mrc p15, 0, r0, c2, c0, 0  /* arbitrary read of CP15 */
    add r0, r0, #0			   /* a stall */
    RET

ENTRY(armv7_context_switch)
    mcr p15, 0, r0, c7, c10, 4  /* drain the write buffer */
    mcr p15, 0, r0, c2, c0, 0 	/* set the new TTB */
    mcr p15, 0, r0, c8, c7, 0	/* flush the I+D */
    RET

ENTRY(armv7_tlb_flushID_SE)
    mcr p15, 0, r0, c8, c7, 1	/* flush I+D tlb single entry */
    mcr p15, 0, r0, c7, c10, 4  /* drain write buffer */
    RET


ENTRY(armv7_setttb)
/* Does this even exist on armv7? */
#ifdef PMAP_CACHE_VIVT
    stmdb	sp!, {r0, lr}
    bl _C_LABEL(armv7_idcache_wbinv_all) 	/* Clean the D cache */
    ldmia	sp!, {r0, lr}
#endif

    mcr p15, 0, r0, c2, c0, 0   /* load new TTB */
    mcr p15, 0, r0, c8, c7, 0   /* invalidate I+D TLBs */
    mcr p15, 0, r0, c7, c10, 4  /* drain the write buffer */

    RET

/* Cache operations. */

/* LINTSTUB: void armv7_icache_sync_range(vaddr_t, vsize_t); */
ENTRY_NP(armv7_icache_sync_range)
1:
    mcr p15, 0, r0, c7, c5, 1		@invalidate the I-Cache line
    mcr p15, 0, r0, c7, c10, 1		@wb the D-Cache line
    add r0, r0, entrysize
    subs r1, r1, entrysize
    bhi 1b

    mcr p15, 0, r0, c7, c10, 4  	@drain the write buffer, BSB 
    RET

/* LINTSTUB: void armv7_icache_sync_all(void); */
ENTRY_NP(armv7_icache_sync_all)
    /*
     * We assume that the code here can never be out of sync with the
     * dcache, so that we can safely flush the Icache and fall through
     * into the Dcache cleaning code.
     */
    stmdb	sp!, {r0, lr}
    bl _C_LABEL(armv7_idcache_wbinv_all) 	/* Clean the D cache */
    ldmia	sp!, {r0, lr}
    mcr p15, 0, r0, c7, c10, 4  			/* drain the write buffer, BSB */
    RET

ENTRY(armv7_dcache_wb_range)
1:
    mcr p15, 0, r0, c7, c10, 1		@wb the D-Cache
    add r0, r0, entrysize
    subs r1, r1, entrysize
    bhi 1b
    mcr p15, 0, r0, c7, c10, 4  	@drain the write buffer, BSB 
    RET

/* LINTSTUB: void armv7_dcache_wbinv_range(vaddr_t, vsize_t); */
ENTRY(armv7_dcache_wbinv_range)
1:
    mcr p15, 0, r0, c7, c14, 1		@wb and inv the D-Cache line
    add r0, r0, entrysize
    subs r1, r1, entrysize
    bhi 1b
    mcr p15, 0, r0, c7, c10, 4  	@drain the write buffer, BSB 
    RET

/* * LINTSTUB: void armv7_dcache_inv_range(vaddr_t, vsize_t); */
ENTRY(armv7_dcache_inv_range)
1:
    mcr	p15, 0, r0, c7, c6, 1		@invalidate the D-Cache line  
    add r0, r0, entrysize 
    subs r1, r1, entrysize
    bhi 1b
    mcr p15, 0, r0, c7, c10, 4  	@drain the write buffer, BSB 
    RET


ENTRY(armv7_idcache_wbinv_range)
1:
    mcr p15, 0, r0, c7, c5, 1		@invalidate the I-Cache line
    mcr p15, 0, r0, c7, c14, 1 		@wb and inv the D-Cache line
    add r0, r0, entrysize
    subs r1, r1, entrysize
    bhi 1b
    mcr p15, 0, r0, c7, c10, 4  	@drain the write buffer, BSB 
    RET

ENTRY_NP(armv7_idcache_wbinv_all)
    /*
     * We assume that the code here can never be out of sync with the
     * dcache, so that we can safely flush the Icache and fall through
     * into the Dcache purging code.
     */
    DMB
    mcr p15, 0, r0, c7, c5, 0
    b _C_LABEL(armv7_dcache_wbinv_all)

/*
 * armv7_dcache_wbinv_all is in cpufunc.c. It's really too long to
 * write in assembler.
 */
