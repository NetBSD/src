/*	$NetBSD: tprof_x86.c,v 1.4.2.2 2018/07/28 04:38:15 pgoyette Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <machine/specialreg.h>
#include <dev/tprof/tprof_ioctl.h>
#include "../tprof.h"

int tprof_event_init(uint32_t);
void tprof_event_list(void);
void tprof_event_lookup(const char *, struct tprof_param *);

struct name_to_event {
	const char *name;
	uint64_t event;
	uint64_t unit;
	bool enabled;
};

struct event_table {
	const char *tablename;
	struct name_to_event *names;
	size_t nevents;
	struct event_table *next;
};

static struct event_table *cpuevents = NULL;

static void x86_cpuid(unsigned int *eax, unsigned int *ebx,
    unsigned int *ecx, unsigned int *edx)
{
	asm volatile("cpuid"
	    : "=a" (*eax),
	      "=b" (*ebx),
	      "=c" (*ecx),
	      "=d" (*edx)
	    : "0" (*eax), "2" (*ecx));
}

/* -------------------------------------------------------------------------- */

/*
 * Intel Architectural Version 1.
 */
static struct name_to_event intel_arch1_names[] = {
	/* Event Name - Event Select - UMask */
	{ "unhalted-core-cycles",	0x3C, 0x00, true },
	{ "instruction-retired",	0xC0, 0x00, true },
	{ "unhalted-reference-cycles",	0x3C, 0x01, true },
	{ "llc-reference",		0x2E, 0x4F, true },
	{ "llc-misses",			0x2E, 0x41, true },
	{ "branch-instruction-retired",	0xC4, 0x00, true },
	{ "branch-misses-retired",	0xC5, 0x00, true },
};

static struct event_table intel_arch1 = {
	.tablename = "Intel Architectural Version 1",
	.names = intel_arch1_names,
	.nevents = sizeof(intel_arch1_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_intel_arch1(void)
{
	unsigned int eax, ebx, ecx, edx;
	struct event_table *table;
	size_t i;

	eax = 0x0A;
	ebx = 0;
	ecx = 0;
	edx = 0;
	x86_cpuid(&eax, &ebx, &ecx, &edx);

	table = &intel_arch1;
	for (i = 0; i < table->nevents; i++) {
		/* Disable the unsupported events. */
		if ((ebx & (i << 1)) != 0)
			table->names[i].enabled = false;
	}

	return table;
}

/*
 * Intel Skylake/Kabylake.
 *
 * The events that are not listed, because they are of little interest or
 * require extra configuration:
 *     TX_*
 *     FRONTEND_RETIRED.*
 *     FP_ARITH_INST_RETIRED.*
 *     HLE_RETIRED.*
 *     RTM_RETIRED.*
 *     MEM_TRANS_RETIRED.*
 *     UOPS_DISPATCHED_PORT.*
 */
static struct name_to_event intel_skylake_kabylake_names[] = {
	/* Event Name - Event Select - UMask */
	{ "LD_BLOCKS.STORE_FORWARD",					0x03, 0x02, true },
	{ "LD_BLOCKS.NO_SR",						0x03, 0x08, true },
	{ "LD_BLOCKS_PARTIAL.ADDRESS_ALIAS",				0x07, 0x01, true },
	{ "DTLB_LOAD_MISSES.MISS_CAUSES_A_WALK",			0x08, 0x01, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_4K",				0x08, 0x02, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_2M_4M",			0x08, 0x04, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_1G",				0x08, 0x08, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED",				0x08, 0x0E, true },
	{ "DTLB_LOAD_MISSES.WALK_PENDING",				0x08, 0x10, true },
	{ "DTLB_LOAD_MISSES.STLB_HIT",					0x08, 0x20, true },
	{ "INT_MISC.RECOVERY_CYCLES",					0x0D, 0x01, true },
	{ "INT_MISC.CLEAR_RESTEER_CYCLES",				0x0D, 0x80, true },
	{ "UOPS_ISSUED.ANY",						0x0E, 0x01, true },
	{ "UOPS_ISSUED.VECTOR_WIDTH_MISMATCH",				0x0E, 0x02, true },
	{ "UOPS_ISSUED.SLOW_LEA",					0x0E, 0x20, true },
	{ "L2_RQSTS.DEMAND_DATA_RD_MISS",				0x24, 0x21, true },
	{ "L2_RQSTS.RFO_MISS",						0x24, 0x22, true },
	{ "L2_RQSTS.CODE_RD_MISS",					0x24, 0x24, true },
	{ "L2_RQSTS.ALL_DEMAND_MISS",					0x24, 0x27, true },
	{ "L2_RQSTS.PF_MISS",						0x24, 0x38, true },
	{ "L2_RQSTS.MISS",						0x24, 0x3F, true },
	{ "L2_RQSTS.DEMAND_DATA_RD_HIT",				0x24, 0x41, true },
	{ "L2_RQSTS.RFO_HIT",						0x24, 0x42, true },
	{ "L2_RQSTS.CODE_RD_HIT",					0x24, 0x44, true },
	{ "L2_RQSTS.PF_HIT",						0x24, 0xD8, true },
	{ "L2_RQSTS.ALL_DEMAND_DATA_RD",				0x24, 0xE1, true },
	{ "L2_RQSTS.ALL_RFO",						0x24, 0xE2, true },
	{ "L2_RQSTS.ALL_CODE_RD",					0x24, 0xE4, true },
	{ "L2_RQSTS.ALL_DEMAND_REFERENCES",				0x24, 0xE7, true },
	{ "L2_RQSTS.ALL_PF",						0x24, 0xF8, true },
	{ "L2_RQSTS.REFERENCES",					0x24, 0xFF, true },
	{ "SW_PREFETCH_ACCESS.NTA",					0x32, 0x01, true },
	{ "SW_PREFETCH_ACCESS.T0",					0x32, 0x02, true },
	{ "SW_PREFETCH_ACCESS.T1_T2",					0x32, 0x04, true },
	{ "SW_PREFETCH_ACCESS.PREFETCHW",				0x32, 0x08, true },
	{ "CPU_CLK_THREAD_UNHALTED.ONE_THREAD_ACTIVE",			0x3C, 0x02, true },
	{ "CPU_CLK_UNHALTED.ONE_THREAD_ACTIVE",				0x3C, 0x02, true },
	{ "L1D_PEND_MISS.PENDING",					0x48, 0x01, true },
	{ "L1D_PEND_MISS.FB_FULL",					0x48, 0x02, true },
	{ "DTLB_STORE_MISSES.MISS_CAUSES_A_WALK",			0x49, 0x01, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_4K",			0x49, 0x02, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_2M_4M",			0x49, 0x04, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_1G",			0x49, 0x08, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED",				0x49, 0x0E, true },
	{ "DTLB_STORE_MISSES.WALK_PENDING",				0x49, 0x10, true },
	{ "DTLB_STORE_MISSES.STLB_HIT",					0x49, 0x20, true },
	{ "LOAD_HIT_PRE.SW_PF",						0x4C, 0x01, true },
	{ "EPT.WALK_PENDING",						0x4F, 0x10, true },
	{ "L1D.REPLACEMENT",						0x51, 0x01, true },
	{ "RS_EVENTS.EMPTY_CYCLES",					0x5E, 0x01, true },
	{ "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_DATA_RD",		0x60, 0x01, true },
	{ "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_CODE_RD",		0x60, 0x02, true },
	{ "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_RFO",			0x60, 0x04, true },
	{ "OFFCORE_REQUESTS_OUTSTANDING.ALL_DATA_RD",			0x60, 0x08, true },
	{ "OFFCORE_REQUESTS_OUTSTANDING.L3_MISS_DEMAND_DATA_RD",	0x60, 0x10, true },
	{ "IDQ.MITE_UOPS",						0x79, 0x04, true },
	{ "IDQ.DSB_UOPS",						0x79, 0x08, true },
	{ "IDQ.MS_MITE_UOPS",						0x79, 0x20, true },
	{ "IDQ.MS_UOPS",						0x79, 0x30, true },
	{ "ICACHE_16B.IFDATA_STALL",					0x80, 0x04, true },
	{ "ICACHE_64B.IFTAG_HIT",					0x83, 0x01, true },
	{ "ICACHE_64B.IFTAG_MISS",					0x83, 0x02, true },
	{ "ICACHE_64B.IFTAG_STALL",					0x83, 0x04, true },
	{ "ITLB_MISSES.MISS_CAUSES_A_WALK",				0x85, 0x01, true },
	{ "ITLB_MISSES.WALK_COMPLETED_4K",				0x85, 0x02, true },
	{ "ITLB_MISSES.WALK_COMPLETED_2M_4M",				0x85, 0x04, true },
	{ "ITLB_MISSES.WALK_COMPLETED_1G",				0x85, 0x08, true },
	{ "ITLB_MISSES.WALK_COMPLETED",					0x85, 0x0E, true },
	{ "ITLB_MISSES.WALK_PENDING",					0x85, 0x10, true },
	{ "ITLB_MISSES.STLB_HIT",					0x85, 0x20, true },
	{ "ILD_STALL.LCP",						0x87, 0x01, true },
	{ "IDQ_UOPS_NOT_DELIVERED.CORE",				0x9C, 0x01, true },
	{ "RESOURCE_STALLS.ANY",					0xA2, 0x01, true },
	{ "RESOURCE_STALLS.SB",						0xA2, 0x08, true },
	{ "EXE_ACTIVITY.EXE_BOUND_0_PORTS",				0xA6, 0x01, true },
	{ "EXE_ACTIVITY.1_PORTS_UTIL",					0xA6, 0x02, true },
	{ "EXE_ACTIVITY.2_PORTS_UTIL",					0xA6, 0x04, true },
	{ "EXE_ACTIVITY.3_PORTS_UTIL",					0xA6, 0x08, true },
	{ "EXE_ACTIVITY.4_PORTS_UTIL",					0xA6, 0x10, true },
	{ "EXE_ACTIVITY.BOUND_ON_STORES",				0xA6, 0x40, true },
	{ "LSD.UOPS",							0xA8, 0x01, true },
	{ "DSB2MITE_SWITCHES.PENALTY_CYCLES",				0xAB, 0x02, true },
	{ "ITLB.ITLB_FLUSH",						0xAE, 0x01, true },
	{ "OFFCORE_REQUESTS.DEMAND_DATA_RD",				0xB0, 0x01, true },
	{ "OFFCORE_REQUESTS.DEMAND_CODE_RD",				0xB0, 0x02, true },
	{ "OFFCORE_REQUESTS.DEMAND_RFO",				0xB0, 0x04, true },
	{ "OFFCORE_REQUESTS.ALL_DATA_RD",				0xB0, 0x08, true },
	{ "OFFCORE_REQUESTS.L3_MISS_DEMAND_DATA_RD",			0xB0, 0x10, true },
	{ "OFFCORE_REQUESTS.ALL_REQUESTS",				0xB0, 0x80, true },
	{ "UOPS_EXECUTED.THREAD",					0xB1, 0x01, true },
	{ "UOPS_EXECUTED.CORE",						0xB1, 0x02, true },
	{ "UOPS_EXECUTED.X87",						0xB1, 0x10, true },
	{ "OFFCORE_REQUESTS_BUFFER.SQ_FULL",				0xB2, 0x01, true },
	{ "TLB_FLUSH.DTLB_THREAD",					0xBD, 0x01, true },
	{ "TLB_FLUSH.STLB_ANY",						0xBD, 0x20, true },
	{ "INST_RETIRED.PREC_DIST",					0xC0, 0x01, true },
	{ "OTHER_ASSISTS.ANY",						0xC1, 0x3F, true },
	{ "UOPS_RETIRED.RETIRE_SLOTS",					0xC2, 0x02, true },
	{ "MACHINE_CLEARS.MEMORY_ORDERING",				0xC3, 0x02, true },
	{ "MACHINE_CLEARS.SMC",						0xC3, 0x04, true },
	{ "BR_INST_RETIRED.CONDITIONAL",				0xC4, 0x01, true },
	{ "BR_INST_RETIRED.NEAR_CALL",					0xC4, 0x02, true },
	{ "BR_INST_RETIRED.NEAR_RETURN",				0xC4, 0x08, true },
	{ "BR_INST_RETIRED.NOT_TAKEN",					0xC4, 0x10, true },
	{ "BR_INST_RETIRED.NEAR_TAKEN",					0xC4, 0x20, true },
	{ "BR_INST_RETIRED.FAR_BRANCH",					0xC4, 0x40, true },
	{ "BR_MISP_RETIRED.CONDITIONAL",				0xC5, 0x01, true },
	{ "BR_MISP_RETIRED.NEAR_CALL",					0xC5, 0x02, true },
	{ "BR_MISP_RETIRED.NEAR_TAKEN",					0xC5, 0x20, true },
	{ "HW_INTERRUPTS.RECEIVED",					0xCB, 0x01, true },
	{ "MEM_INST_RETIRED.STLB_MISS_LOADS",				0xD0, 0x11, true },
	{ "MEM_INST_RETIRED.STLB_MISS_STORES",				0xD0, 0x12, true },
	{ "MEM_INST_RETIRED.LOCK_LOADS",				0xD0, 0x21, true },
	{ "MEM_INST_RETIRED.SPLIT_LOADS",				0xD0, 0x41, true },
	{ "MEM_INST_RETIRED.SPLIT_STORES",				0xD0, 0x42, true },
	{ "MEM_INST_RETIRED.ALL_LOADS",					0xD0, 0x81, true },
	{ "MEM_INST_RETIRED.ALL_STORES",				0xD0, 0x82, true },
	{ "MEM_LOAD_RETIRED.L1_HIT",					0xD1, 0x01, true },
	{ "MEM_LOAD_RETIRED.L2_HIT",					0xD1, 0x02, true },
	{ "MEM_LOAD_RETIRED.L3_HIT",					0xD1, 0x04, true },
	{ "MEM_LOAD_RETIRED.L1_MISS",					0xD1, 0x08, true },
	{ "MEM_LOAD_RETIRED.L2_MISS",					0xD1, 0x10, true },
	{ "MEM_LOAD_RETIRED.L3_MISS",					0xD1, 0x20, true },
	{ "MEM_LOAD_RETIRED.FB_HIT",					0xD1, 0x40, true },
	{ "MEM_LOAD_L3_HIT_RETIRED.XSNP_MISS",				0xD2, 0x01, true },
	{ "MEM_LOAD_L3_HIT_RETIRED.XSNP_HIT",				0xD2, 0x02, true },
	{ "MEM_LOAD_L3_HIT_RETIRED.XSNP_HITM",				0xD2, 0x04, true },
	{ "MEM_LOAD_L3_HIT_RETIRED.XSNP_NONE",				0xD2, 0x08, true },
	{ "MEM_LOAD_MISC_RETIRED.UC",					0xD4, 0x04, true },
	{ "BACLEARS.ANY",						0xE6, 0x01, true },
	{ "L2_TRANS.L2_WB",						0xF0, 0x40, true },
	{ "L2_LINES_IN.ALL",						0xF1, 0x1F, true },
	{ "L2_LINES_OUT.SILENT",					0xF2, 0x01, true },
	{ "L2_LINES_OUT.NON_SILENT",					0xF2, 0x02, true },
	{ "L2_LINES_OUT.USELESS_HWPF",					0xF2, 0x04, true },
	{ "SQ_MISC.SPLIT_LOCK",						0xF4, 0x10, true },
};

static struct event_table intel_skylake_kabylake = {
	.tablename = "Intel Skylake/Kabylake",
	.names = intel_skylake_kabylake_names,
	.nevents = sizeof(intel_skylake_kabylake_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_intel_skylake_kabylake(void)
{
	return &intel_skylake_kabylake;
}

static struct event_table *
init_intel_generic(void)
{
	unsigned int eax, ebx, ecx, edx;
	struct event_table *table;

	/*
	 * The kernel made sure the Architectural Version 1 PMCs were
	 * present.
	 */
	table = init_intel_arch1();

	/*
	 * Now query the additional (non-architectural) events. They
	 * depend on the CPU model.
	 */
	eax = 0x01;
	ebx = 0;
	ecx = 0;
	edx = 0;
	x86_cpuid(&eax, &ebx, &ecx, &edx);

	if (CPUID_TO_FAMILY(eax) == 6) {
		switch (CPUID_TO_MODEL(eax)) {
		case 0x4E: /* Skylake */
		case 0x5E: /* Skylake */
		case 0x8E: /* Kabylake */
		case 0x9E: /* Kabylake */
			table->next = init_intel_skylake_kabylake();
			break;
		}
	}

	return table;
}

/* -------------------------------------------------------------------------- */

/*
 * AMD Family 10h
 */
static struct name_to_event amd_f10h_names[] = {
	{ "seg-load-all",		0x20, 0x7f, true },
	{ "seg-load-es",		0x20, 0x01, true },
	{ "seg-load-cs",		0x20, 0x02, true },
	{ "seg-load-ss",		0x20, 0x04, true },
	{ "seg-load-ds",		0x20, 0x08, true },
	{ "seg-load-fs",		0x20, 0x10, true },
	{ "seg-load-gs",		0x20, 0x20, true },
	{ "seg-load-hs",		0x20, 0x40, true },
	{ "l1cache-access",		0x40, 0x00, true },
	{ "l1cache-miss",		0x41, 0x00, true },
	{ "l1cache-refill",		0x42, 0x1f, true },
	{ "l1cache-refill-invalid",	0x42, 0x01, true },
	{ "l1cache-refill-shared",	0x42, 0x02, true },
	{ "l1cache-refill-exclusive",	0x42, 0x04, true },
	{ "l1cache-refill-owner",	0x42, 0x08, true },
	{ "l1cache-refill-modified",	0x42, 0x10, true },
	{ "l1cache-load",		0x43, 0x1f, true },
	{ "l1cache-load-invalid",	0x43, 0x01, true },
	{ "l1cache-load-shared",	0x43, 0x02, true },
	{ "l1cache-load-exclusive",	0x43, 0x04, true },
	{ "l1cache-load-owner",		0x43, 0x08, true },
	{ "l1cache-load-modified",	0x43, 0x10, true },
	{ "l1cache-writeback",		0x44, 0x1f, true },
	{ "l1cache-writeback-invalid",	0x44, 0x01, true },
	{ "l1cache-writeback-shared",	0x44, 0x02, true },
	{ "l1cache-writeback-exclusive",0x44, 0x04, true },
	{ "l1cache-writeback-owner",	0x44, 0x08, true },
	{ "l1cache-writeback-modified",	0x44, 0x10, true },
	{ "l1DTLB-hit-all",		0x4D, 0x07, true },
	{ "l1DTLB-hit-4Kpage",		0x4D, 0x01, true },
	{ "l1DTLB-hit-2Mpage",		0x4D, 0x02, true },
	{ "l1DTLB-hit-1Gpage",		0x4D, 0x04, true },
	{ "l1DTLB-miss-all",		0x45, 0x07, true },
	{ "l1DTLB-miss-4Kpage",		0x45, 0x01, true },
	{ "l1DTLB-miss-2Mpage",		0x45, 0x02, true },
	{ "l1DTLB-miss-1Gpage",		0x45, 0x04, true },
	{ "l2DTLB-miss-all",		0x46, 0x03, true },
	{ "l2DTLB-miss-4Kpage",		0x46, 0x01, true },
	{ "l2DTLB-miss-2Mpage",		0x46, 0x02, true },
	/* l2DTLB-miss-1Gpage: reserved on some revisions, so disabled */
	{ "l1ITLB-miss",		0x84, 0x00, true },
	{ "l2ITLB-miss-all",		0x85, 0x03, true },
	{ "l2ITLB-miss-4Kpage",		0x85, 0x01, true },
	{ "l2ITLB-miss-2Mpage",		0x85, 0x02, true },
	{ "mem-misalign-ref",		0x47, 0x00, true },
	{ "ins-fetch",			0x80, 0x00, true },
	{ "ins-fetch-miss",		0x81, 0x00, true },
	{ "ins-refill-l2",		0x82, 0x00, true },
	{ "ins-refill-sys",		0x83, 0x00, true },
	{ "ins-fetch-stall",		0x87, 0x00, true },
	{ "ins-retired",		0xC0, 0x00, true },
	{ "ins-empty",			0xD0, 0x00, true },
	{ "ops-retired",		0xC1, 0x00, true },
	{ "branch-retired",		0xC2, 0x00, true },
	{ "branch-miss-retired",	0xC3, 0x00, true },
	{ "branch-taken-retired",	0xC4, 0x00, true },
	{ "branch-taken-miss-retired",	0xC5, 0x00, true },
	{ "branch-far-retired",		0xC6, 0x00, true },
	{ "branch-resync-retired",	0xC7, 0x00, true },
	{ "branch-near-retired",	0xC8, 0x00, true },
	{ "branch-near-miss-retired",	0xC9, 0x00, true },
	{ "branch-indirect-miss-retired", 0xCA, 0x00, true },
	{ "int-hw",			0xCF, 0x00, true },
	{ "int-cycles-masked",		0xCD, 0x00, true },
	{ "int-cycles-masked-pending",	0xCE, 0x00, true },
	{ "fpu-exceptions",		0xDB, 0x00, true },
	{ "break-match0",		0xDC, 0x00, true },
	{ "break-match1",		0xDD, 0x00, true },
	{ "break-match2",		0xDE, 0x00, true },
	{ "break-match3",		0xDF, 0x00, true },
};

static struct event_table amd_f10h = {
	.tablename = "AMD Family 10h",
	.names = amd_f10h_names,
	.nevents = sizeof(amd_f10h_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_amd_f10h(void)
{
	return &amd_f10h;
}

static struct event_table *
init_amd_generic(void)
{
	unsigned int eax, ebx, ecx, edx;

	eax = 0x01;
	ebx = 0;
	ecx = 0;
	edx = 0;
	x86_cpuid(&eax, &ebx, &ecx, &edx);

	switch (CPUID_TO_FAMILY(eax)) {
	case 0x10:
		return init_amd_f10h();
	}

	return NULL;
}

/* -------------------------------------------------------------------------- */

int
tprof_event_init(uint32_t ident)
{
	switch (ident) {
	case TPROF_IDENT_NONE:
		return -1;
	case TPROF_IDENT_INTEL_GENERIC:
		cpuevents = init_intel_generic();
		break;
	case TPROF_IDENT_AMD_GENERIC:
		cpuevents = init_amd_generic();
		break;
	}
	return (cpuevents == NULL) ? -1 : 0;
}

static void
recursive_event_list(struct event_table *table)
{
	size_t i;

	printf("%s:\n", table->tablename);
	for (i = 0; i < table->nevents; i++) {
		if (!table->names[i].enabled)
			continue;
		printf("\t%s\n", table->names[i].name);
	}

	if (table->next != NULL) {
		recursive_event_list(table->next);
	}
}

void
tprof_event_list(void)
{
	recursive_event_list(cpuevents);
}

static void
recursive_event_lookup(struct event_table *table, const char *name,
    struct tprof_param *param)
{
	size_t i;

	for (i = 0; i < table->nevents; i++) {
		if (!table->names[i].enabled)
			continue;
		if (!strcmp(table->names[i].name, name)) {
			param->p_event = table->names[i].event;
			param->p_unit = table->names[i].unit;
			return;
		}
	}

	if (table->next != NULL) {
		recursive_event_lookup(table->next, name, param);
	} else {
		errx(EXIT_FAILURE, "event '%s' unknown", name);
	}
}

void
tprof_event_lookup(const char *name, struct tprof_param *param)
{
	recursive_event_lookup(cpuevents, name, param);
}
