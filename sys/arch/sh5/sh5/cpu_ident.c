/*	$NetBSD: cpu_ident.c,v 1.1 2003/03/13 13:44:18 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <machine/cacheops.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bootparams.h>

#include <sh5/sh5/stb1var.h>

struct sh5_cache_ops sh5_cache_ops;
struct sh5_tlb_ops sh5_tlb_ops;

/*
 * The STB1 was the first "Evaluation" implementation of the SH5
 * architecture. We use its cache/tlb description in the case where
 * we don't recognise the CPU ID.
 *
 * This is enough to limp along to the point where we can bitch to
 * the user that their cpu is not supported.
 */
static struct sh5_cache_ops stb1_cache_ops = {
	_sh5_stb1_cache_dpurge,
	_sh5_stb1_cache_dpurge_iinv,
	_sh5_stb1_cache_dinv,
	_sh5_stb1_cache_dinv_iinv,
	_sh5_stb1_cache_iinv,
	_sh5_stb1_cache_iinv_all,
	_sh5_stb1_cache_purge_all,
	{
		/* Data cache */
		STB1_CACHE_SIZE,
		SH5_CACHE_INFO_TYPE_VIPT,
		SH5_CACHE_INFO_WRITE_BACK,
		STB1_CACHE_LINE_SIZE,
		STB1_CACHE_NWAYS,
		STB1_CACHE_NSETS
	},
	{
		/* Instruction cache */
		STB1_CACHE_SIZE,
		SH5_CACHE_INFO_TYPE_VIVT,
		SH5_CACHE_INFO_WRITE_NONE,
		STB1_CACHE_LINE_SIZE,
		STB1_CACHE_NWAYS,
		STB1_CACHE_NSETS
	}
};

static struct sh5_tlb_ops stb1_tlb_ops = {
	_sh5_stb1_tlbinv_cookie,
	_sh5_stb1_tlbinv_all,
	_sh5_stb1_tlbload,
	STB1_TLB_NSLOTS,
	STB1_TLB_NSLOTS
};


void
cpu_identify(void)
{
	struct boot_params *bp = &bootparams;

	switch (bp->bp_cpu[0].cpuid) {
#ifdef CPU_STB1
	case SH5_CPUID_STB1:
		sh5_cache_ops = stb1_cache_ops;
		sh5_tlb_ops = stb1_tlb_ops;
		break;
#endif

	default:
		/*
		 * Default to the STB1 ops.
		 * We'll bitch later about lack of specific cpu support.
		 */
		sh5_cache_ops = stb1_cache_ops;
		sh5_tlb_ops = stb1_tlb_ops;
		break;
	}
}
