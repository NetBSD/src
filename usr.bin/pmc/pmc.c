/*	$NetBSD: pmc.c,v 1.16 2007/12/15 19:44:52 perry Exp $	*/

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
__RCSID("$NetBSD: pmc.c,v 1.16 2007/12/15 19:44:52 perry Exp $");
#endif

#include <sys/types.h>
#include <machine/sysarch.h>
#include <machine/specialreg.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct pmc_name2val {
	const char *name;
	int val;
	int unit;
};

static const struct pmc_name2val i586_names[] = {
	{ "tlb-data-miss",		PMC5_DATA_TLB_MISS,		0 },
	{ "tlb-ins-miss",		PMC5_INST_TLB_MISS,		0 },
	{ "l1cache-ins-miss", 		PMC5_INST_CACHE_MISS,		0 },
	{ "l1cache-data-miss",		PMC5_DATA_RW_MISS,		0 },
	{ "l1cache-data-miss-read",	PMC5_DATA_READ_MISS,		0 },
	{ "l1cache-data-miss-write",	PMC5_DATA_WRITE_MISS,		0 },
	{ "l1cache-writeback",		PMC5_DATA_LINES_WBACK,		0 },
	{ "l1cache-writeback-hit",	PMC5_WRITE_M_E,			0 },
	{ "l2cache-data-snoop",		PMC5_DATA_CACHE_SNOOP,		0 },
	{ "l2cache-data-snoop-hit",	PMC5_DATA_CACHE_SNOOP_HIT,	0 },
	{ "mem-read",			PMC5_DATA_READ,			0 },
	{ "mem-write",			PMC5_DATA_WRITE,		0 },
	{ "mem-access",			PMC5_DATA_RW,			0 },
	{ "mem-access-both-pipes",	PMC5_MEM_ACCESS_BOTH_PIPES,	0 },
	{ "mem-bank-conflicts",		PMC5_BANK_CONFLICTS,		0 },
	{ "mem-misalign-ref",		PMC5_MISALIGNED_DATA,		0 },
	{ "mem-uncached-read",		PMC5_NONCACHE_MEM_READ,		0 },
	{ "seg-load-any",		PMC5_SEGMENT_REG_LOAD,		0 },
	{ "branch",			PMC5_BRANCHES,			0 },
	{ "branch-btb-hit",		PMC5_BTB_HITS,			0 },
	{ "branch-taken",		PMC5_BRANCH_TAKEN,		0 },
	{ "ins-read",			PMC5_INST_READ,			0 },
	{ "ins-pipeline-flush",		PMC5_PIPELINE_FLUSH,		0 },
	{ "ins-executed",		PMC5_INST_EXECUTED,		0 },
	{ "ins-executed-vpipe",		PMC5_INST_EXECUTED_V_PIPE,	0 },
	{ "ins-stall-agi",		PMC5_AGI_STALL,			0 },
	{ "ins-stall-write",		PMC5_WRITE_BACKUP_STALL,	0 },
	{ "ins-stall-data",		PMC5_DATA_READ_STALL,		0 },
	{ "ins-stall-writeline",	PMC5_WRITE_E_M_STALL,		0 },
	{ "bus-utilization",		PMC5_BUS_UTILIZATION,		0 },
	{ "bus-locked",			PMC5_LOCKED_BUS,		0 },
	{ "bus-io-cycle",		PMC5_IO_CYCLE,			0 },
	{ "fpu-flops", 			PMC5_FLOPS,			0 },
	{ "int-hw",			PMC5_HARDWARE_INTR,		0 },
	{ "break-match0",		PMC5_BP0_MATCH,			0 },
	{ "break-match1",		PMC5_BP1_MATCH,			0 },
	{ "break-match2",		PMC5_BP2_MATCH,			0 },
	{ "break-match3",		PMC5_BP3_MATCH,			0 },
};

static const struct pmc_name2val i686_names[] = {
	{ "mem-refs",			PMC6_DATA_MEM_REFS,		0 },
	{ "l1cache-lines",		PMC6_DCU_LINES_IN,		0 },
	{ "l1cache-mlines",		PMC6_DCU_M_LINES_IN,		0 },
	{ "l1cache-mlines-evict",	PMC6_DCU_M_LINES_OUT,		0 },
	{ "l1cache-miss-wait",		PMC6_DCU_MISS_OUTSTANDING, 	0 },
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

static const struct pmc_name2val k7_names[] = {
	{ "seg-load-all",		K7_SEGMENT_REG_LOADS,		0x7f },
	{ "seg-load-es",		K7_SEGMENT_REG_LOADS,		0x01 },
	{ "seg-load-cs",		K7_SEGMENT_REG_LOADS,		0x02 },
	{ "seg-load-ss",		K7_SEGMENT_REG_LOADS,		0x04 },
	{ "seg-load-ds",		K7_SEGMENT_REG_LOADS,		0x08 },
	{ "seg-load-fs",		K7_SEGMENT_REG_LOADS,		0x10 },
	{ "seg-load-gs",		K7_SEGMENT_REG_LOADS,		0x20 },
	{ "seg-load-hs",		K7_SEGMENT_REG_LOADS,		0x40 },
	{ "seg-load-stall",		K7_SEGMENT_LOAD_STALL,		0 },
	{ "l1cache-access",		K7_DATA_CACHE_ACCESS,		0 },
	{ "l1cache-miss",		K7_DATA_CACHE_MISS,	 	0 },
	{ "l1cache-refill",		K7_DATA_CACHE_REFILL,	 	0x1f },
	{ "l1cache-refill-invalid",	K7_DATA_CACHE_REFILL,	 	0x01 },
	{ "l1cache-refill-shared",	K7_DATA_CACHE_REFILL,	 	0x02 },
	{ "l1cache-refill-exclusive",	K7_DATA_CACHE_REFILL,	 	0x04 },
	{ "l1cache-refill-owner",	K7_DATA_CACHE_REFILL,	 	0x08 },
	{ "l1cache-refill-modified",	K7_DATA_CACHE_REFILL,	 	0x10 },
	{ "l1cache-load",		K7_DATA_CACHE_REFILL_SYSTEM, 	0x1f },
	{ "l1cache-load-invalid",	K7_DATA_CACHE_REFILL_SYSTEM, 	0x01 },
	{ "l1cache-load-shared",	K7_DATA_CACHE_REFILL_SYSTEM, 	0x02 },
	{ "l1cache-load-exclusive",	K7_DATA_CACHE_REFILL_SYSTEM, 	0x04 },
	{ "l1cache-load-owner",		K7_DATA_CACHE_REFILL_SYSTEM, 	0x08 },
	{ "l1cache-load-modified",	K7_DATA_CACHE_REFILL_SYSTEM, 	0x10 },
	{ "l1cache-writeback",		K7_DATA_CACHE_WBACK,	 	0x1f },
	{ "l1cache-writeback-invalid",	K7_DATA_CACHE_WBACK,	 	0x01 },
	{ "l1cache-writeback-shared",	K7_DATA_CACHE_WBACK,	 	0x02 },
	{ "l1cache-writeback-exclusive",K7_DATA_CACHE_WBACK,	 	0x04 },
	{ "l1cache-writeback-owner",	K7_DATA_CACHE_WBACK,	 	0x08 },
	{ "l1cache-writeback-modified",	K7_DATA_CACHE_WBACK,	 	0x10 },
	{ "l2cache-access",		K7_L2_REQUEST,			0xff },
	{ "l2cache-tag-read",		K7_L2_REQUEST,			0x01 },
	{ "l2cache-tag-write",		K7_L2_REQUEST,			0x02 },
	{ "l2cache-inst-read",		K7_L2_REQUEST,			0x04 },
	{ "l2cache-inst-load",		K7_L2_REQUEST,			0x08 },
	{ "l2cache-data-store",		K7_L2_REQUEST,			0x10 },
	{ "l2cache-data-loadmem",	K7_L2_REQUEST,			0x20 },
	{ "l2cache-data-write",		K7_L2_REQUEST,			0x40 },
	{ "l2cache-data-move",		K7_L2_REQUEST,			0x80 },
	{ "l2cache-access-busy",	K7_L2_REQUEST_BUSY,		0 },
	{ "l2cache-hit",		K7_L2_DTLB_HIT,		 	0 },
	{ "l2cache-miss",		K7_L2_DTLB_MISS,	 	0 },
	{ "mem-misalign-ref",		K7_MISALIGNED_DATA_REF,		0 },
	{ "mem-access",			K7_SYSTEM_REQUEST,		0 },
	{ "mem-access-uc",		K7_SYSTEM_REQUEST_TYPE,		0x01 },
	{ "mem-access-wc",		K7_SYSTEM_REQUEST_TYPE,		0x04 },
	{ "mem-access-wt",		K7_SYSTEM_REQUEST_TYPE,		0x10 },
	{ "mem-access-wp",		K7_SYSTEM_REQUEST_TYPE,		0x20 },
	{ "mem-access-wb",		K7_SYSTEM_REQUEST_TYPE,		0x40 },
	{ "ins-fetch",			K7_IFU_IFETCH,			0 },
	{ "ins-fetch-miss",		K7_IFU_IFETCH_MISS,		0 },
	{ "ins-refill-l2",		K7_IFU_REFILL_FROM_L2,		0 },
	{ "ins-refill-mem",		K7_IFU_REFILL_FROM_SYSTEM,	0 },
	{ "ins-fetch-stall",		K7_IFU_STALL,			0 },
	{ "ins-retired",		K7_RETIRED_INST,		0 },
	{ "ins-empty",			K7_INSTRUCTION_DECODER_EMPTY,	0 },
	{ "itlb-miss-l1",		K7_ITLB_L1_MISS,		0 },
	{ "itlb-miss-l2",		K7_ITLB_L2_MISS,		0 },
	{ "ops-retired",		K7_RETIRED_OPS,			0 },
	{ "branch-retired",		K7_RETIRED_BRANCHES,		0 },
	{ "branch-miss-retired",	K7_RETIRED_BRANCH_MISPREDICTED,	0 },
	{ "branch-taken-retired",	K7_RETIRED_TAKEN_BRANCH,	0 },
	{ "branch-taked-miss-retired",
	    K7_RETIRED_TAKEN_BRANCH_MISPREDICTED,	0 },
	{ "branch-far-retired",
	    K7_RETIRED_FAR_CONTROL_TRANSFER,		0 },
	{ "branch-resync-retired",	K7_RETIRED_RESYNC_BRANCH,	0 },
	{ "branch-near-retired",	K7_RETIRED_NEAR_RETURNS,	0 },
	{ "branch-near-miss-retired",
	    K7_RETIRED_NEAR_RETURNS_MISPREDICTED,	0 },
	{ "branch-indirect-miss-retired",
	    K7_RETIRED_INDIRECT_MISPREDICTED,	0 },
	{ "int-hw",			K7_HW_INTR_RECV,		0 },
	{ "int-cycles-masked",		K7_CYCLES_INT_MASKED,		0 },
	{ "int-cycles-masked-pending",
	    K7_CYCLES_INT_PENDING_AND_MASKED, 0 },
	{ "break-match0",		K7_BP0_MATCH,			0 },
	{ "break-match1",		K7_BP1_MATCH,			0 },
	{ "break-match2",		K7_BP2_MATCH,			0 },
	{ "break-match3",		K7_BP3_MATCH,			0 },
};

static struct pmc_name2val_cpus {
	int type;
	const struct pmc_name2val *pmc_names;
	int size;
} pmc_cpus[] = {
	{ PMC_TYPE_I586, i586_names,
	  sizeof(i586_names)/sizeof(struct pmc_name2val) },
	{ PMC_TYPE_I686, i686_names,
	  sizeof(i686_names)/sizeof(struct pmc_name2val) },
	{ PMC_TYPE_K7, k7_names,
	  sizeof(k7_names)/sizeof(struct pmc_name2val) },
};


static const struct pmc_name2val_cpus *
pmc_lookup_cpu(int type)
{
	int i;

	for (i = 0; i < sizeof(pmc_cpus)/sizeof(struct pmc_name2val_cpus);
	    i++) {
		if (pmc_cpus[i].type == type)
			return (&pmc_cpus[i]);
	}
	return (NULL);
}


static const struct pmc_name2val *
find_pmc_name(const struct pmc_name2val_cpus *pncp, const char *name)
{
	int i;
	const struct pmc_name2val *pnp = NULL;

	for (i = 0; i < pncp->size; i++) {
		if (strcmp(pncp->pmc_names[i].name, name) == 0) {
			pnp = &pncp->pmc_names[i];
			break;
		}
	}

	return (pnp);
}

static void
list_pmc_names(const struct pmc_name2val_cpus *pncp)
{
	int i, n, left, pairs;

	(void)printf("Supported performance counter events:\n");
	n = pncp->size;
	pairs = n / 2;
	left = n % 2;

	for (i = 0; i < pairs; i++)
		(void)printf("    %37s %37s\n", pncp->pmc_names[i * 2].name,
		    pncp->pmc_names[i * 2 + 1].name);
	if (left != 0)
		(void)printf("\t%37s\n", pncp->pmc_names[n - 1].name);
}

static void usage(void) __dead;
static void usage(void)
{

	(void)fprintf(stderr, "usage: %s -h\n"
			"       %s -C\n"
			"       %s -c <event> command [options] ...\n",
	    getprogname(), getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	int c, status, ret0, ret1, errn0, errn1;
	const char * volatile event = "unknown";
	const struct pmc_name2val_cpus *pncp;
	const struct pmc_name2val *pnp;
	struct i386_pmc_info_args pi;
	struct i386_pmc_startstop_args pss0, pss1;
	struct i386_pmc_read_args pr0, pr1;
	pid_t pid;

	setprogname(argv[0]);
	errn0 = errn1 = 0;

	if (i386_pmc_info(&pi) < 0)
		errx(2, "PMC support is not compiled into the kernel");

	pncp = pmc_lookup_cpu(pi.type);
	if (pncp == NULL)
		errx(3, "PMC counters are not supported for your CPU (0x%x)",
		    pi.type);

	pnp = NULL;
	while ((c = getopt(argc, argv, "Cc:h")) != -1) {
		switch (c) {
		case 'C':
			if (argc != 2)
				usage();
			/*
			 * Just clear both counters. Useful if
			 * a previous run got killed and did not
			 * clean up.
			 */
			(void)memset(&pss0, 0, sizeof pss0);
			(void)i386_pmc_startstop(&pss0);
			pss0.counter = 1;
			(void)i386_pmc_startstop(&pss0);
			return 0;
		case 'c':
			event = optarg;
			pnp = find_pmc_name(pncp, event);
			break;
		case 'h':
			if (argc != 2)
				usage();
			list_pmc_names(pncp);
			return 0;
		default:
			usage();
		}
	}

	if (pnp == NULL || argc <= optind)
		usage();

	(void)memset(&pss0, 0, sizeof pss0);
	(void)memset(&pss1, 0, sizeof pss1);
	pss0.event = pss1.event = pnp->val;
	pss0.unit = pss1.unit = pnp->unit;
	pss0.flags = PMC_SETUP_USER;
	pss0.counter = 0;
	pss1.flags = PMC_SETUP_KERNEL;
	pss1.counter = 1;

	/*
	 * XXX should catch signals and tidy up in the parent.
	 */
	if (i386_pmc_startstop(&pss0) < 0)
		err(4, "pmc_start user");
	pss0.flags = 0;
	if (i386_pmc_startstop(&pss1) < 0)
		err(5, "pmc_start kernel");
	pss1.flags = 0;

	pid = vfork();

	switch(pid) {
	case -1:
		err(5, "vfork");
	case 0:
		(void)execvp(argv[optind], &argv[optind]);
		err(6, "execvp");
	}

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);

	if (waitpid(pid, &status, 0) == -1)
		err(6, "waitpid");
	if (!WIFEXITED(status))
		return 0;

	/*
	 * Do not immediately exit on errors below. The counters
	 * must be stopped first, or subsequent runs will get
	 * EBUSY.
	 */
	pr0.counter = 0;
	ret0 = i386_pmc_read(&pr0);
	if (ret0 < 0)
		errn0 = errno;
	pr1.counter = 1;
	ret1 = i386_pmc_read(&pr1);
	if (ret1 < 0)
		errn1 = errno;

	if (i386_pmc_startstop(&pss0) < 0)
		warn("pmc_stop user");
	if (i386_pmc_startstop(&pss1) < 0)
		warn("pmc_stop kernel");

	if (ret0 < 0) {
		errno = errn0;
		err(6, "pmc_read");
	}
	if (ret1 < 0) {
		errno = errn1;
		err(7, "pmc_read");
	}

	(void)printf("%s: user %llu kernel %llu\n", event, pr0.val, pr1.val);

	return 0;
}
