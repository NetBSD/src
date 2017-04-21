/*	$NetBSD: pmc.c,v 1.17.34.1 2017/04/21 16:54:15 bouyer Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * Copyright 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: pmc.c,v 1.17.34.1 2017/04/21 16:54:15 bouyer Exp $");
#endif

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stdbool.h>
#include <machine/sysarch.h>
#include <machine/specialreg.h>
#include <machine/param.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(__i386__)
typedef struct i386_pmc_info_args x86_pmc_info_args_t;
typedef struct i386_pmc_startstop_args x86_pmc_startstop_args_t;
typedef struct i386_pmc_read_args x86_pmc_read_args_t;
#else /* amd64 */
typedef struct x86_64_pmc_info_args x86_pmc_info_args_t;
typedef struct x86_64_pmc_startstop_args x86_pmc_startstop_args_t;
typedef struct x86_64_pmc_read_args x86_pmc_read_args_t;
#endif

typedef struct {
	const char *name;
	uint32_t val;
	uint32_t unit;
} pmc_name2val_t;

typedef struct {
	int type;
	const pmc_name2val_t *pmc_names;
	size_t size;
} pmc_name2val_cpu_t;

static const pmc_name2val_t i686_names[] = {
	{ "mem-refs",			PMC6_DATA_MEM_REFS,		0 },
	{ "l1cache-lines",		PMC6_DCU_LINES_IN,		0 },
	{ "l1cache-mlines",		PMC6_DCU_M_LINES_IN,		0 },
	{ "l1cache-mlines-evict",	PMC6_DCU_M_LINES_OUT,		0 },
	{ "l1cache-miss-wait",		PMC6_DCU_MISS_OUTSTANDING,	0 },
	{ "ins-fetch",			PMC6_IFU_IFETCH,		0 },
	{ "ins-fetch-misses",		PMC6_IFU_IFETCH_MISS,		0 },
	{ "itlb-misses",		PMC6_IFU_IFETCH_MISS,		0 },
	{ "insfetch-mem-stall",		PMC6_IFU_MEM_STALL,		0 },
	{ "insfetch-decode-stall",	PMC6_ILD_STALL,			0 },
	{ "l2cache-insfetch",		PMC6_L2_IFETCH,			0x0f },
	{ "l2cache-data-loads",		PMC6_L2_LD,			0x0f },
	{ "l2cache-data-stores",	PMC6_L2_ST,			0x0f },
	{ "l2cache-lines",		PMC6_L2_LINES_IN,		0 },
	{ "l2cache-lines-evict",	PMC6_L2_LINES_OUT,		0 },
	{ "l2cache-mlines",		PMC6_L2_M_LINES_INM,		0 },
	{ "l2cache-mlines-evict",	PMC6_L2_M_LINES_OUTM,		0x0f },
	{ "l2cache-reqs",		PMC6_L2_RQSTS,			0 },
	{ "l2cache-addr-strobes",	PMC6_L2_ADS,			0 },
	{ "l2cache-data-busy",		PMC6_L2_DBUS_BUSY,		0 },
	{ "l2cache-data-busy-read",	PMC6_L2_DBUS_BUSY_RD,		0 },
	{ "bus-drdy-clocks-self",	PMC6_BUS_DRDY_CLOCKS,		0x00 },
	{ "bus-drdy-clocks-any",	PMC6_BUS_DRDY_CLOCKS,		0x20 },
	{ "bus-lock-clocks-self",	PMC6_BUS_LOCK_CLOCKS,		0x00 },
	{ "bus-lock-clocks-any",	PMC6_BUS_LOCK_CLOCKS,		0x20 },
	{ "bus-req-outstanding-self",	PMC6_BUS_REQ_OUTSTANDING,	0x00 },
	{ "bus-req-outstanding-any",	PMC6_BUS_REQ_OUTSTANDING,	0x20 },
	{ "bus-burst-reads-self",	PMC6_BUS_TRAN_BRD,		0x00 },
	{ "bus-burst-reads-any",	PMC6_BUS_TRAN_BRD,		0x20 },
	{ "bus-read-for-ownership-self",PMC6_BUS_TRAN_RFO,		0x00 },
	{ "bus-read-for-ownership-any",	PMC6_BUS_TRAN_RFO,		0x20 },
	{ "bus-write-back-self",	PMC6_BUS_TRANS_WB,		0x00 },
	{ "bus-write-back-any",		PMC6_BUS_TRANS_WB,		0x20 },
	{ "bus-ins-fetches-self",	PMC6_BUS_TRAN_IFETCH,		0x00 },
	{ "bus-ins-fetches-any",	PMC6_BUS_TRAN_IFETCH,		0x20 },
	{ "bus-invalidates-self",	PMC6_BUS_TRAN_INVAL,		0x00 },
	{ "bus-invalidates-any",	PMC6_BUS_TRAN_INVAL,		0x20 },
	{ "bus-partial-writes-self",	PMC6_BUS_TRAN_PWR,		0x00 },
	{ "bus-partial-writes-any",	PMC6_BUS_TRAN_PWR,		0x20 },
	{ "bus-partial-trans-self",	PMC6_BUS_TRANS_P,		0x00 },
	{ "bus-partial-trans-any",	PMC6_BUS_TRANS_P,		0x20 },
	{ "bus-io-trans-self",		PMC6_BUS_TRANS_IO,		0x00 },
	{ "bus-io-trans-any",		PMC6_BUS_TRANS_IO,		0x20 },
	{ "bus-deferred-trans-self",	PMC6_BUS_TRAN_DEF,		0x00 },
	{ "bus-deferred-trans-any",	PMC6_BUS_TRAN_DEF,		0x20 },
	{ "bus-burst-trans-self",	PMC6_BUS_TRAN_BURST,		0x00 },
	{ "bus-burst-trans-any",	PMC6_BUS_TRAN_BURST,		0x20 },
	{ "bus-total-trans-self",	PMC6_BUS_TRAN_ANY,		0x00 },
	{ "bus-total-trans-any",	PMC6_BUS_TRAN_ANY,		0x20 },
	{ "bus-mem-trans-self",		PMC6_BUS_TRAN_MEM,		0x00 },
	{ "bus-mem-trans-any",		PMC6_BUS_TRAN_MEM,		0x20 },
	{ "bus-recv-cycles",		PMC6_BUS_DATA_RCV,		0 },
	{ "bus-bnr-cycles",		PMC6_BUS_BNR_DRV,		0 },
	{ "bus-hit-cycles",		PMC6_BUS_HIT_DRV,		0 },
	{ "bus-hitm-cycles",		PMC6_BUS_HITM_DRDV,		0 },
	{ "bus-snoop-stall",		PMC6_BUS_SNOOP_STALL,		0 },
	{ "fpu-flops",			PMC6_FLOPS,			0 },
	{ "fpu-comp-ops",		PMC6_FP_COMP_OPS_EXE,		0 },
	{ "fpu-except-assist",		PMC6_FP_ASSIST,			0 },
	{ "fpu-mul",			PMC6_MUL,			0 },
	{ "fpu-div",			PMC6_DIV,			0 },
	{ "fpu-div-busy",		PMC6_CYCLES_DIV_BUSY,		0 },
	{ "mem-sb-blocks",		PMC6_LD_BLOCKS,			0 },
	{ "mem-sb-drains",		PMC6_SB_DRAINS,			0 },
	{ "mem-misalign-ref",		PMC6_MISALIGN_MEM_REF,		0 },
	{ "ins-pref-dispatch-nta",	PMC6_EMON_KNI_PREF_DISPATCHED,	0x01 },
	{ "ins-pref-dispatch-t1",	PMC6_EMON_KNI_PREF_DISPATCHED,	0x01 },
	{ "ins-pref-dispatch-t2",	PMC6_EMON_KNI_PREF_DISPATCHED,	0x02 },
	{ "ins-pref-dispatch-weak",	PMC6_EMON_KNI_PREF_DISPATCHED,	0x03 },
	{ "ins-pref-miss-nta",		PMC6_EMON_KNI_PREF_MISS,	0x01 },
	{ "ins-pref-miss-t1",		PMC6_EMON_KNI_PREF_MISS,	0x01 },
	{ "ins-pref-miss-t2",		PMC6_EMON_KNI_PREF_MISS,	0x02 },
	{ "ins-pref-miss-weak",		PMC6_EMON_KNI_PREF_MISS,	0x03 },
	{ "ins-retired",		PMC6_INST_RETIRED,		0 },
	{ "uops-retired",		PMC6_UOPS_RETIRED,		0 },
	{ "ins-decoded",		PMC6_INST_DECODED,		0 },
	{ "ins-stream-retired-packed-scalar",
	    PMC6_EMON_KNI_INST_RETIRED, 0x00 },
	{ "ins-stream-retired-scalar",
	    PMC6_EMON_KNI_INST_RETIRED, 0x01 },
	{ "ins-stream-comp-retired-packed-scalar",
	    PMC6_EMON_KNI_COMP_INST_RET, 0x00 },
	{ "ins-stream-comp-retired--scalar",
	    PMC6_EMON_KNI_COMP_INST_RET, 0x01 },
	{ "int-hw",			PMC6_HW_INT_RX,			0 },
	{ "int-cycles-masked",		PMC6_CYCLES_INT_MASKED,		0 },
	{ "int-cycles-masked-pending",
	    PMC6_CYCLES_INT_PENDING_AND_MASKED, 0 },
	{ "branch-retired",		PMC6_BR_INST_RETIRED,		0 },
	{ "branch-miss-retired",	PMC6_BR_MISS_PRED_RETIRED,	0 },
	{ "branch-taken-retired",	PMC6_BR_TAKEN_RETIRED,		0 },
	{ "branch-taken-mispred-retired", PMC6_BR_MISS_PRED_TAKEN_RET,	0 },
	{ "branch-decoded",		PMC6_BR_INST_DECODED,		0 },
	{ "branch-btb-miss",		PMC6_BTB_MISSES,		0 },
	{ "branch-bogus",		PMC6_BR_BOGUS,			0 },
	{ "branch-baclear",		PMC6_BACLEARS,			0 },
	{ "stall-resource",		PMC6_RESOURCE_STALLS,		0 },
	{ "stall-partial",		PMC6_PARTIAL_RAT_STALLS,	0 },
	{ "seg-loads",			PMC6_SEGMENT_REG_LOADS,		0 },
	{ "unhalted-cycles",		PMC6_CPU_CLK_UNHALTED,		0 },
	{ "mmx-exec",			PMC6_MMX_INSTR_EXEC,		0 },
	{ "mmx-sat-exec",		PMC6_MMX_SAT_INSTR_EXEC,	0 },
	{ "mmx-uops-exec",		PMC6_MMX_UOPS_EXEC,		0x0f },
	{ "mmx-exec-packed-mul",	PMC6_MMX_INSTR_TYPE_EXEC,	0x01 },
	{ "mmx-exec-packed-shift",	PMC6_MMX_INSTR_TYPE_EXEC,	0x02 },
	{ "mmx-exec-pack-ops",		PMC6_MMX_INSTR_TYPE_EXEC,	0x04 },
	{ "mmx-exec-unpack-ops",	PMC6_MMX_INSTR_TYPE_EXEC,	0x08 },
	{ "mmx-exec-packed-logical",	PMC6_MMX_INSTR_TYPE_EXEC,	0x10 },
	{ "mmx-exec-packed-arith",	PMC6_MMX_INSTR_TYPE_EXEC,	0x20 },
	{ "mmx-trans-mmx-float",	PMC6_FP_MMX_TRANS,		0x00 },
	{ "mmx-trans-float-mmx",	PMC6_FP_MMX_TRANS,		0x01 },
	{ "mmx-assist",			PMC6_MMX_ASSIST,		0 },
	{ "mmx-retire",			PMC6_MMX_INSTR_RET,		0 },
	{ "seg-rename-stalls-es",	PMC6_SEG_RENAME_STALLS,		0x01 },
	{ "seg-rename-stalls-ds",	PMC6_SEG_RENAME_STALLS,		0x02 },
	{ "seg-rename-stalls-fs",	PMC6_SEG_RENAME_STALLS,		0x04 },
	{ "seg-rename-stalls-gs",	PMC6_SEG_RENAME_STALLS,		0x08 },
	{ "seg-rename-stalls-all",	PMC6_SEG_RENAME_STALLS,		0x0f },
	{ "seg-rename-es",		PMC6_SEG_REG_RENAMES,		0x01 },
	{ "seg-rename-ds",		PMC6_SEG_REG_RENAMES,		0x02 },
	{ "seg-rename-fs",		PMC6_SEG_REG_RENAMES,		0x04 },
	{ "seg-rename-gs",		PMC6_SEG_REG_RENAMES,		0x08 },
	{ "seg-rename-all",		PMC6_SEG_REG_RENAMES,		0x0f },
	{ "seg-rename-retire",		PMC6_RET_SEG_RENAMES,		0 },
};

static const pmc_name2val_t k7_names[] = {
	{ "l1cache-access",		K7_DATA_CACHE_ACCESS,		0 },
	{ "l1cache-miss",		K7_DATA_CACHE_MISS,		0 },
	{ "l1cache-refill",		K7_DATA_CACHE_REFILL,		0x1f },
	{ "l1cache-refill-invalid",	K7_DATA_CACHE_REFILL,		0x01 },
	{ "l1cache-refill-shared",	K7_DATA_CACHE_REFILL,		0x02 },
	{ "l1cache-refill-exclusive",	K7_DATA_CACHE_REFILL,		0x04 },
	{ "l1cache-refill-owner",	K7_DATA_CACHE_REFILL,		0x08 },
	{ "l1cache-refill-modified",	K7_DATA_CACHE_REFILL,		0x10 },
	{ "l1cache-load",		K7_DATA_CACHE_REFILL_SYSTEM,	0x1f },
	{ "l1cache-load-invalid",	K7_DATA_CACHE_REFILL_SYSTEM,	0x01 },
	{ "l1cache-load-shared",	K7_DATA_CACHE_REFILL_SYSTEM,	0x02 },
	{ "l1cache-load-exclusive",	K7_DATA_CACHE_REFILL_SYSTEM,	0x04 },
	{ "l1cache-load-owner",		K7_DATA_CACHE_REFILL_SYSTEM,	0x08 },
	{ "l1cache-load-modified",	K7_DATA_CACHE_REFILL_SYSTEM,	0x10 },
	{ "l1cache-writeback",		K7_DATA_CACHE_WBACK,		0x1f },
	{ "l1cache-writeback-invalid",	K7_DATA_CACHE_WBACK,		0x01 },
	{ "l1cache-writeback-shared",	K7_DATA_CACHE_WBACK,		0x02 },
	{ "l1cache-writeback-exclusive",K7_DATA_CACHE_WBACK,		0x04 },
	{ "l1cache-writeback-owner",	K7_DATA_CACHE_WBACK,		0x08 },
	{ "l1cache-writeback-modified",	K7_DATA_CACHE_WBACK,		0x10 },
	{ "l1DTLB-miss",		K7_L1_DTLB_MISS,		0 },
	{ "l2DTLB-miss",		K7_L2_DTLB_MISS,		0 },
	{ "l1ITLB-miss",		K7_L1_ITLB_MISS,		0 },
	{ "l2ITLB-miss",		K7_L2_ITLB_MISS,		0 },
	{ "mem-misalign-ref",		K7_MISALIGNED_DATA_REF,		0 },
	{ "ins-fetch",			K7_IFU_IFETCH,			0 },
	{ "ins-fetch-miss",		K7_IFU_IFETCH_MISS,		0 },
	{ "ins-refill-l2",		K7_IFU_REFILL_FROM_L2,		0 },
	{ "ins-refill-sys",		K7_IFU_REFILL_FROM_SYSTEM,	0 },
	{ "ins-retired",		K7_RETIRED_INST,		0 },
	{ "ops-retired",		K7_RETIRED_OPS,			0 },
	{ "branch-retired",		K7_RETIRED_BRANCH,		0 },
	{ "branch-miss-retired",	K7_RETIRED_BRANCH_MISPREDICTED,	0 },
	{ "branch-taken-retired",	K7_RETIRED_TAKEN_BRANCH,	0 },
	{ "branch-taked-miss-retired",
	    K7_RETIRED_TAKEN_BRANCH_MISPREDICTED,	0 },
	{ "branch-far-retired",
	    K7_RETIRED_FAR_CONTROL_TRANSFER,		0 },
	{ "branch-resync-retired",	K7_RETIRED_RESYNC_BRANCH,	0 },
	{ "int-hw",			K7_HW_INTR_RECV,		0 },
	{ "int-cycles-masked",		K7_CYCLES_INT_MASKED,		0 },
	{ "int-cycles-masked-pending",
	    K7_CYCLES_INT_PENDING_AND_MASKED, 0 },
};

static const pmc_name2val_t f10h_names[] = {
	{ "seg-load-all",		F10H_SEGMENT_REG_LOADS,		0x7f },
	{ "seg-load-es",		F10H_SEGMENT_REG_LOADS,		0x01 },
	{ "seg-load-cs",		F10H_SEGMENT_REG_LOADS,		0x02 },
	{ "seg-load-ss",		F10H_SEGMENT_REG_LOADS,		0x04 },
	{ "seg-load-ds",		F10H_SEGMENT_REG_LOADS,		0x08 },
	{ "seg-load-fs",		F10H_SEGMENT_REG_LOADS,		0x10 },
	{ "seg-load-gs",		F10H_SEGMENT_REG_LOADS,		0x20 },
	{ "seg-load-hs",		F10H_SEGMENT_REG_LOADS,		0x40 },
	{ "l1cache-access",		F10H_DATA_CACHE_ACCESS,		0 },
	{ "l1cache-miss",		F10H_DATA_CACHE_MISS,		0 },
	{ "l1cache-refill",		F10H_DATA_CACHE_REFILL_FROM_L2,	0x1f },
	{ "l1cache-refill-invalid",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x01 },
	{ "l1cache-refill-shared",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x02 },
	{ "l1cache-refill-exclusive",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x04 },
	{ "l1cache-refill-owner",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x08 },
	{ "l1cache-refill-modified",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x10 },
	{ "l1cache-load",		F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x1f },
	{ "l1cache-load-invalid",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x01 },
	{ "l1cache-load-shared",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x02 },
	{ "l1cache-load-exclusive",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x04 },
	{ "l1cache-load-owner",		F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x08 },
	{ "l1cache-load-modified",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x10 },
	{ "l1cache-writeback",		F10H_CACHE_LINES_EVICTED,	0x1f },
	{ "l1cache-writeback-invalid",	F10H_CACHE_LINES_EVICTED,	0x01 },
	{ "l1cache-writeback-shared",	F10H_CACHE_LINES_EVICTED,	0x02 },
	{ "l1cache-writeback-exclusive",F10H_CACHE_LINES_EVICTED,	0x04 },
	{ "l1cache-writeback-owner",	F10H_CACHE_LINES_EVICTED,	0x08 },
	{ "l1cache-writeback-modified",	F10H_CACHE_LINES_EVICTED,	0x10 },
	{ "l1DTLB-hit-all",		F10H_L1_DTLB_HIT,		0x07 },
	{ "l1DTLB-hit-4Kpage",		F10H_L1_DTLB_HIT,		0x01 },
	{ "l1DTLB-hit-2Mpage",		F10H_L1_DTLB_HIT,		0x02 },
	{ "l1DTLB-hit-1Gpage",		F10H_L1_DTLB_HIT,		0x04 },
	{ "l1DTLB-miss-all",		F10H_L1_DTLB_MISS,		0x07 },
	{ "l1DTLB-miss-4Kpage",		F10H_L1_DTLB_MISS,		0x01 },
	{ "l1DTLB-miss-2Mpage",		F10H_L1_DTLB_MISS,		0x02 },
	{ "l1DTLB-miss-1Gpage",		F10H_L1_DTLB_MISS,		0x04 },
	{ "l2DTLB-miss-all",		F10H_L2_DTLB_MISS,		0x03 },
	{ "l2DTLB-miss-4Kpage",		F10H_L2_DTLB_MISS,		0x01 },
	{ "l2DTLB-miss-2Mpage",		F10H_L2_DTLB_MISS,		0x02 },
	/* l2DTLB-miss-1Gpage: reserved on some revisions, so disabled */
	{ "l1ITLB-miss",		F10H_L1_ITLB_MISS,		0 },
	{ "l2ITLB-miss-all",		F10H_L2_ITLB_MISS,		0x03 },
	{ "l2ITLB-miss-4Kpage",		F10H_L2_ITLB_MISS,		0x01 },
	{ "l2ITLB-miss-2Mpage",		F10H_L2_ITLB_MISS,		0x02 },
	{ "mem-misalign-ref",		F10H_MISALIGNED_ACCESS,		0 },
	{ "ins-fetch",			F10H_INSTRUCTION_CACHE_FETCH,	0 },
	{ "ins-fetch-miss",		F10H_INSTRUCTION_CACHE_MISS,	0 },
	{ "ins-refill-l2",		F10H_INSTRUCTION_CACHE_REFILL_FROM_L2,	0 },
	{ "ins-refill-sys",		F10H_INSTRUCTION_CACHE_REFILL_FROM_SYS,	0 },
	{ "ins-fetch-stall",		F10H_INSTRUCTION_FETCH_STALL,	0 },
	{ "ins-retired",		F10H_RETIRED_INSTRUCTIONS,	0 },
	{ "ins-empty",			F10H_DECODER_EMPTY,	0 },
	{ "ops-retired",		F10H_RETIRED_UOPS,		0 },
	{ "branch-retired",		F10H_RETIRED_BRANCH,		0 },
	{ "branch-miss-retired",	F10H_RETIRED_MISPREDICTED_BRANCH,0 },
	{ "branch-taken-retired",	F10H_RETIRED_TAKEN_BRANCH,	0 },
	{ "branch-taken-miss-retired",	F10H_RETIRED_TAKEN_BRANCH_MISPREDICTED,	0 },
	{ "branch-far-retired", 	F10H_RETIRED_FAR_CONTROL_TRANSFER, 0 },
	{ "branch-resync-retired",	F10H_RETIRED_BRANCH_RESYNC,	0 },
	{ "branch-near-retired",	F10H_RETIRED_NEAR_RETURNS,	0 },
	{ "branch-near-miss-retired",	F10H_RETIRED_NEAR_RETURNS_MISPREDICTED,	0 },
	{ "branch-indirect-miss-retired", F10H_RETIRED_INDIRECT_BRANCH_MISPREDICTED,	0 },
	{ "int-hw",			F10H_INTERRUPTS_TAKEN,		0 },
	{ "int-cycles-masked",		F10H_INTERRUPTS_MASKED_CYCLES,	0 },
	{ "int-cycles-masked-pending",
	    F10H_INTERRUPTS_MASKED_CYCLES_INTERRUPT_PENDING, 0 },
	{ "fpu-exceptions",		F10H_FPU_EXCEPTIONS, 0 },
	{ "break-match0",		F10H_DR0_BREAKPOINT_MATCHES,	0 },
	{ "break-match1",		F10H_DR1_BREAKPOINT_MATCHES,	0 },
	{ "break-match2",		F10H_DR2_BREAKPOINT_MATCHES,	0 },
	{ "break-match3",		F10H_DR3_BREAKPOINT_MATCHES,	0 },
};

static const pmc_name2val_cpu_t pmc_cpus[] = {
	{ PMC_TYPE_I686, i686_names,
	  sizeof(i686_names) / sizeof(pmc_name2val_t) },
	{ PMC_TYPE_K7, k7_names,
	  sizeof(k7_names) / sizeof(pmc_name2val_t) },
	{ PMC_TYPE_F10H, f10h_names,
	  sizeof(f10h_names) / sizeof(pmc_name2val_t) },
};

/* -------------------------------------------------------------------------- */

static void usage(void) __dead;

static void pmc_list(const pmc_name2val_cpu_t *, char **);
static void pmc_start(const pmc_name2val_cpu_t *, char **);
static void pmc_stop(const pmc_name2val_cpu_t *, char **);

static const pmc_name2val_cpu_t *pmc_lookup_cpu(int);
static const pmc_name2val_t *pmc_find_event(const pmc_name2val_cpu_t *,
    const char *);

static int x86_pmc_info(x86_pmc_info_args_t *);
static int x86_pmc_startstop(x86_pmc_startstop_args_t *);
static int x86_pmc_read(x86_pmc_read_args_t *);

static uint32_t pmc_ncounters;

static struct cmdtab {
	const char *label;
	bool takesargs;
	bool argsoptional;
	void (*func)(const pmc_name2val_cpu_t *, char **);
} const pmc_cmdtab[] = {
	{ "list",	false, false, pmc_list },
	{ "start",	true,  false, pmc_start },
	{ "stop",	false, false, pmc_stop },
	{ NULL,		false, false, NULL },
};

static void usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s list\n", progname);
	fprintf(stderr, "       %s start [name:option ...]\n", progname);
	fprintf(stderr, "       %s stop\n", progname);
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

static void
pmc_list(const pmc_name2val_cpu_t *pncp, char **argv)
{
	size_t i, n, mid;

	printf("Supported performance counter events:\n");
	n = pncp->size;
	mid = n / 2;

	for (i = 0; i < mid; i++)
		printf("    %-37s %-37s\n", pncp->pmc_names[i].name,
		    pncp->pmc_names[mid + i].name);
	if ((n % 2) != 0)
		printf("    %-37s\n", pncp->pmc_names[n - 1].name);
}

static void
pmc_start(const pmc_name2val_cpu_t *pncp, char **argv)
{
	x86_pmc_startstop_args_t pmcargs[PMC_NCOUNTERS], *pmcarg;
	char *tokens[PMC_NCOUNTERS][2], *event, *options;
	const pmc_name2val_t *pnp;
	uint32_t flags;
	size_t n, i;

	for (n = 0; n < pmc_ncounters; n++) {
		if (argv[n] == NULL)
			break;
		tokens[n][0] = strtok(argv[n], ":");
		tokens[n][1] = strtok(NULL, ":");
		if (tokens[n][1] == NULL)
			usage();
	}

	for (i = 0; i < n; i++) {
		pmcarg = &pmcargs[i];
		event = tokens[i][0];
		options = tokens[i][1];

		pnp = pmc_find_event(pncp, event);
		if (pnp == NULL)
			errx(EXIT_FAILURE, "Unable to find '%s'", event);
		flags = 0;
		if (strchr(options, 'u'))
			flags |= PMC_SETUP_USER;
		if (strchr(options, 'k'))
			flags |= PMC_SETUP_KERNEL;
		if (flags == 0)
			errx(EXIT_FAILURE, "Missing counter option");

		memset(pmcarg, 0, sizeof(*pmcarg));
		pmcarg->counter = i;
		pmcarg->event = pnp->val;
		pmcarg->unit = pnp->unit;
		pmcarg->flags = flags;
	}

	for (i = 0; i < n; i++) {
		pmcarg = &pmcargs[i];
		if (x86_pmc_startstop(pmcarg) < 0)
			errx(EXIT_FAILURE, "Unable to start counter #%zu", i);
		printf("Counter #%zu started\n", i);
	}
}

static void
pmc_stop(const pmc_name2val_cpu_t *pncp, char **argv)
{
	x86_pmc_cpuval_t cpuval[PMC_NCOUNTERS][MAXCPUS];
	x86_pmc_startstop_args_t pmcstop;
	x86_pmc_read_args_t pmcread;
	size_t i, j, n, nval = 0;

	/* Read the values. */
	for (n = 0; n < pmc_ncounters; n++) {
		memset(&pmcread, 0, sizeof(pmcread));
		pmcread.counter = n;
		pmcread.values = (x86_pmc_cpuval_t *)&cpuval[n];
		pmcread.nval = MAXCPUS;
		if (x86_pmc_read(&pmcread) < 0) {
			if (errno == ENOENT) {
				/*
				 * This counter is not running. So we stop the
				 * iteration here, since the next counters
				 * won't be running either.
				 */
				break;
			}
			errx(EXIT_FAILURE, "Unable to read counter #%zu", n);
		}
		nval = pmcread.nval;
	}

	/* Display the results. Should probably be revisited. */
	printf("Counter\t\t");
	for (i = 0; i < nval; i++) {
		printf("cpu%zu\t\t", i);
	}
	printf("\n");
	printf("-------\t\t");
	for (i = 0; i < nval; i++) {
		printf("----\t\t");
	}
	printf("\n");
	for (i = 0; i < n; i++) {
		printf("%zu\t\t", i);
		for (j = 0; j < nval; j++) {
			printf("%" PRIu64 "\t\t", cpuval[i][j].ctrval);
		}
		printf("\n");
	}

	/* Stop the counters. */
	for (i = 0; i < n; i++) {
		memset(&pmcstop, 0, sizeof(pmcstop));
		pmcstop.counter = i;
		if (x86_pmc_startstop(&pmcstop) < 0)
			errx(EXIT_FAILURE, "Unable to stop counter #%zu", i);
		printf("Counter #%zu stopped\n", i);
	}
}

static const pmc_name2val_cpu_t *
pmc_lookup_cpu(int type)
{
	size_t i, n = sizeof(pmc_cpus) / sizeof(pmc_name2val_cpu_t);

	for (i = 0; i < n; i++) {
		if (pmc_cpus[i].type == type)
			return &pmc_cpus[i];
	}

	return NULL;
}

static const pmc_name2val_t *
pmc_find_event(const pmc_name2val_cpu_t *pncp, const char *name)
{
	size_t i;

	for (i = 0; i < pncp->size; i++) {
		if (strcmp(pncp->pmc_names[i].name, name) == 0)
			return &pncp->pmc_names[i];
	}

	return NULL;
}

static int
x86_pmc_info(x86_pmc_info_args_t *args)
{
	if (sysarch(X86_PMC_INFO, args) == -1)
		return -1;
	return 0;
}

static int
x86_pmc_startstop(x86_pmc_startstop_args_t *args)
{
	if (sysarch(X86_PMC_STARTSTOP, args) == -1)
		return -1;
	return 0;
}

static int
x86_pmc_read(x86_pmc_read_args_t *args)
{
	if (sysarch(X86_PMC_READ, args) == -1)
		return -1;
	return 0;
}

int
main(int argc, char **argv)
{
	const pmc_name2val_cpu_t *pncp;
	x86_pmc_info_args_t pmcinfo;
	const struct cmdtab *ct;

	setprogname(argv[0]);
	argv += 1;

	if (x86_pmc_info(&pmcinfo) < 0)
		errx(EXIT_FAILURE, "PMC support not compiled into the kernel");
	if (pmcinfo.vers != 1)
		errx(EXIT_FAILURE, "Wrong PMC version");
	pmc_ncounters = pmcinfo.nctrs;

	pncp = pmc_lookup_cpu(pmcinfo.type);
	if (pncp == NULL)
		errx(EXIT_FAILURE, "PMC type 0x%x not recognized",
		    pmcinfo.type);

	for (ct = pmc_cmdtab; ct->label != NULL; ct++) {
		if (strcmp(argv[0], ct->label) == 0) {
			if (!ct->argsoptional &&
			    ((ct->takesargs == 0) ^ (argv[1] == NULL)))
			{
				usage();
			}
			(*ct->func)(pncp, argv + 1);
			break;
		}
	}

	return 0;
}
