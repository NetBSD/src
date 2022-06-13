/*	$NetBSD: tprof_x86.c,v 1.11 2022/06/13 07:40:58 msaitoh Exp $	*/

/*
 * Copyright (c) 2018-2019 The NetBSD Foundation, Inc.
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
	{ "topdown-slots",		0xA4, 0x01, true },
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
 * Intel Silvermont/Airmont.
 */
static struct name_to_event intel_silvermont_airmont_names[] = {
	{ "REHABQ.LD_BLOCK_ST_FORWARD",		0x03, 0x01, true },
	{ "REHABQ.LD_BLOCK_STD_NOTREADY",	0x03, 0x02, true },
	{ "REHABQ.ST_SPLITS",			0x03, 0x04, true },
	{ "REHABQ.LD_SPLITS",			0x03, 0x08, true },
	{ "REHABQ.LOCK",			0x03, 0x10, true },
	{ "REHABQ.STA_FULL",			0x03, 0x20, true },
	{ "REHABQ.ANY_LD",			0x03, 0x40, true },
	{ "REHABQ.ANY_ST",			0x03, 0x80, true },
	{ "MEM_UOPS_RETIRED.L1_MISS_LOADS",	0x04, 0x01, true },
	{ "MEM_UOPS_RETIRED.L2_HIT_LOADS",	0x04, 0x02, true },
	{ "MEM_UOPS_RETIRED.L2_MISS_LOADS",	0x04, 0x04, true },
	{ "MEM_UOPS_RETIRED.DTLB_MISS_LOADS",	0x04, 0x08, true },
	{ "MEM_UOPS_RETIRED.UTLB_MISS",		0x04, 0x10, true },
	{ "MEM_UOPS_RETIRED.HITM",		0x04, 0x20, true },
	{ "MEM_UOPS_RETIRED.ALL_LOADS",		0x04, 0x40, true },
	{ "MEM_UOP_RETIRED.ALL_STORES",		0x04, 0x80, true },
	{ "PAGE_WALKS.D_SIDE_CYCLES",		0x05, 0x01, true },
	{ "PAGE_WALKS.I_SIDE_CYCLES",		0x05, 0x02, true },
	{ "PAGE_WALKS.WALKS",			0x05, 0x03, true },
	{ "LONGEST_LAT_CACHE.MISS",		0x2E, 0x41, true },
	{ "LONGEST_LAT_CACHE.REFERENCE",	0x2E, 0x4F, true },
	{ "L2_REJECT_XQ.ALL",			0x30, 0x00, true },
	{ "CORE_REJECT_L2Q.ALL",		0x31, 0x00, true },
	{ "CPU_CLK_UNHALTED.CORE_P",		0x3C, 0x00, true },
	{ "CPU_CLK_UNHALTED.REF_P",		0x3C, 0x01, true },
	{ "ICACHE.HIT",				0x80, 0x01, true },
	{ "ICACHE.MISSES",			0x80, 0x02, true },
	{ "ICACHE.ACCESSES",			0x80, 0x03, true },
	{ "OFFCORE_RESPONSE_0",			0xB7, 0x01, true },
	{ "OFFCORE_RESPONSE_1",			0xB7, 0x02, true },
	{ "INST_RETIRED.ANY_P",			0xC0, 0x00, true },
	{ "UOPS_RETIRED.MS",			0xC2, 0x01, true },
	{ "UOPS_RETIRED.ALL",			0xC2, 0x10, true },
	{ "MACHINE_CLEARS.SMC",			0xC3, 0x01, true },
	{ "MACHINE_CLEARS.MEMORY_ORDERING",	0xC3, 0x02, true },
	{ "MACHINE_CLEARS.FP_ASSIST",		0xC3, 0x04, true },
	{ "MACHINE_CLEARS.ALL",			0xC3, 0x08, true },
	{ "BR_INST_RETIRED.ALL_BRANCHES",	0xC4, 0x00, true },
	{ "BR_INST_RETIRED.JCC",		0xC4, 0x7E, true },
	{ "BR_INST_RETIRED.FAR_BRANCH",		0xC4, 0xBF, true },
	{ "BR_INST_RETIRED.NON_RETURN_IND",	0xC4, 0xEB, true },
	{ "BR_INST_RETIRED.RETURN",		0xC4, 0xF7, true },
	{ "BR_INST_RETIRED.CALL",		0xC4, 0xF9, true },
	{ "BR_INST_RETIRED.IND_CALL",		0xC4, 0xFB, true },
	{ "BR_INST_RETIRED.REL_CALL",		0xC4, 0xFD, true },
	{ "BR_INST_RETIRED.TAKEN_JCC",		0xC4, 0xFE, true },
	{ "BR_MISP_RETIRED.ALL_BRANCHES",	0xC5, 0x00, true },
	{ "BR_MISP_RETIRED.JCC",		0xC5, 0x7E, true },
	{ "BR_MISP_RETIRED.FAR",		0xC5, 0xBF, true },
	{ "BR_MISP_RETIRED.NON_RETURN_IND",	0xC5, 0xEB, true },
	{ "BR_MISP_RETIRED.RETURN",		0xC5, 0xF7, true },
	{ "BR_MISP_RETIRED.CALL",		0xC5, 0xF9, true },
	{ "BR_MISP_RETIRED.IND_CALL",		0xC5, 0xFB, true },
	{ "BR_MISP_RETIRED.REL_CALL",		0xC5, 0xFD, true },
	{ "BR_MISP_RETIRED.TAKEN_JCC",		0xC5, 0xFE, true },
	{ "NO_ALLOC_CYCLES.ROB_FULL",		0xCA, 0x01, true },
	{ "NO_ALLOC_CYCLES.RAT_STALL",		0xCA, 0x20, true },
	{ "NO_ALLOC_CYCLES.ALL",		0xCA, 0x3F, true },
	{ "NO_ALLOC_CYCLES.NOT_DELIVERED",	0xCA, 0x50, true },
	{ "RS_FULL_STALL.MEC",			0xCB, 0x01, true },
	{ "RS_FULL_STALL.ALL",			0xCB, 0x1F, true },
	{ "CYCLES_DIV_BUSY.ANY",		0xCD, 0x01, true },
	{ "BACLEARS.ALL",			0xE6, 0x01, true },
	{ "BACLEARS.RETURN",			0xE6, 0x08, true },
	{ "BACLEARS.COND",			0xE6, 0x10, true },
	{ "MS_DECODED.MS_ENTRY",		0xE7, 0x01, true },
};

static struct event_table intel_silvermont_airmont = {
	.tablename = "Intel Silvermont/Airmont",
	.names = intel_silvermont_airmont_names,
	.nevents = sizeof(intel_silvermont_airmont_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_intel_silvermont_airmont(void)
{

	return &intel_silvermont_airmont;
}

/*
 * Intel Goldmont
 */
static struct name_to_event intel_goldmont_names[] = {
	{ "LD_BLOCKS.ALL_BLOCK",			0x03,	0x10, true },
	{ "LD_BLOCKS.UTLB_MISS",			0x03,	0x08, true },
	{ "LD_BLOCKS.STORE_FORWARD",			0x03,	0x02, true },
	{ "LD_BLOCKS.DATA_UNKNOWN",			0x03,	0x01, true },
	{ "LD_BLOCKS.4K_ALIAS",				0x03,	0x04, true },
	{ "PAGE_WALKS.D_SIDE_CYCLES",			0x05,	0x01, true },
	{ "PAGE_WALKS.I_SIDE_CYCLES",			0x05,	0x02, true },
	{ "PAGE_WALKS.CYCLES",				0x05,	0x03, true },
	{ "UOPS_ISSUED.ANY",				0x0E,	0x00, true },
	{ "MISALIGN_MEM_REF.LOAD_PAGE_SPLIT",		0x13,	0x02, true },
	{ "MISALIGN_MEM_REF.STORE_PAGE_SPLIT",		0x13,	0x04, true },
	{ "LONGEST_LAT_CACHE.REFERENCE",		0x2E,	0x4F, true },
	{ "LONGEST_LAT_CACHE.MISS",			0x2E,	0x41, true },
	{ "L2_REJECT_XQ.ALL",				0x30,	0x00, true },
	{ "CORE_REJECT_L2Q.ALL",			0x31,	0x00, true },
	{ "CPU_CLK_UNHALTED.CORE_P",			0x3C,	0x00, true },
	{ "CPU_CLK_UNHALTED.REF",			0x3C,	0x01, true },
	{ "DL1.DIRTY_EVICTION",				0x51,	0x01, true },
	{ "ICACHE.HIT",					0x80,	0x01, true },
	{ "ICACHE.MISSES",				0x80,	0x02, true },
	{ "ICACHE.ACCESSES",				0x80,	0x03, true },
	{ "ITLB.MISS",					0x81,	0x04, true },
	{ "FETCH_STALL.ALL",				0x86,	0x00, true },
	{ "FETCH_STALL.ITLB_FILL_PENDING_CYCLES",	0x86,	0x01, true },
	{ "FETCH_STALL.ICACHE_FILL_PENDING_CYCLES",	0x86,	0x02, true },
	{ "UOPS_NOT_DELIVERED.ANY",			0x9C,	0x00, true },
	{ "OFFCORE_RESPONSE.0",				0xB7,	0x01, true },
	{ "OFFCORE_RESPONSE.1",				0xB7,	0x02, true },
	{ "INST_RETIRED.ANY_P",				0xC0,	0x00, true },
	{ "UOPS_RETIRED.ANY",				0xC2,	0x00, true },
	{ "UOPS_RETIRED.MS",				0xC2,	0x01, true },
	{ "UOPS_RETIRED.FPDIV",				0xC2,	0x08, true },
	{ "UOPS_RETIRED.IDIV",				0xC2,	0x10, true },
	{ "MACHINE_CLEARS.SMC",				0xC3,	0x01, true },
	{ "MACHINE_CLEARS.MEMORY_ORDERING",		0xC3,	0x02, true },
	{ "MACHINE_CLEARS.FP_ASSIST",			0xC3,	0x04, true },
	{ "MACHINE_CLEARS.DISAMBIGUATION",		0xC3,	0x08, true },
	{ "MACHINE_CLEARS.ALL",				0xC3,	0x00, true },
	{ "BR_INST_RETIRED.ALL_BRANCHES",		0xC4,	0x00, true },
	{ "BR_INST_RETIRED.JCC",			0xC4,	0x7E, true },
	{ "BR_INST_RETIRED.ALL_TAKEN_BRANCHES",		0xC4,	0x80, true },
	{ "BR_INST_RETIRED.TAKEN_JCC",			0xC4,	0xFE, true },
	{ "BR_INST_RETIRED.CALL",			0xC4,	0xF9, true },
	{ "BR_INST_RETIRED.REL_CALL",			0xC4,	0xFD, true },
	{ "BR_INST_RETIRED.IND_CALL",			0xC4,	0xFB, true },
	{ "BR_INST_RETIRED.RETURN",			0xC4,	0xF7, true },
	{ "BR_INST_RETIRED.NON_RETURN_IND",		0xC4,	0xEB, true },
	{ "BR_INST_RETIRED.FAR_BRANCH",			0xC4,	0xBF, true },
	{ "BR_MISP_RETIRED.ALL_BRANCHES",		0xC5,	0x00, true },
	{ "BR_MISP_RETIRED.JCC",			0xC5,	0x7E, true },
	{ "BR_MISP_RETIRED.TAKEN_JCC",			0xC5,	0xFE, true },
	{ "BR_MISP_RETIRED.IND_CALL",			0xC5,	0xFB, true },
	{ "BR_MISP_RETIRED.RETURN",			0xC5,	0xF7, true },
	{ "BR_MISP_RETIRED.NON_RETURN_IND",		0xC5,	0xEB, true },
	{ "ISSUE_SLOTS_NOT_CONSUMED.RESOURCE_FULL",	0xCA,	0x01, true },
	{ "ISSUE_SLOTS_NOT_CONSUMED.RECOVERY",		0xCA,	0x02, true },
	{ "ISSUE_SLOTS_NOT_CONSUMED.ANY",		0xCA,	0x00, true },
	{ "HW_INTERRUPTS.RECEIVED",			0xCB,	0x01, true },
	{ "HW_INTERRUPTS.MASKED",			0xCB,	0x02, true },
	{ "HW_INTERRUPTS.PENDING_AND_MASKED",		0xCB,	0x04, true },
	{ "CYCLES_DIV_BUSY.ALL",			0xCD,	0x00, true },
	{ "CYCLES_DIV_BUSY.IDIV",			0xCD,	0x01, true },
	{ "CYCLES_DIV_BUSY.FPDIV",			0xCD,	0x02, true },
	{ "MEM_UOPS_RETIRED.ALL_LOADS",			0xD0,	0x81, true },
	{ "MEM_UOPS_RETIRED.ALL_STORES",		0xD0,	0x82, true },
	{ "MEM_UOPS_RETIRED.ALL",			0xD0,	0x83, true },
	{ "MEM_UOPS_RETIRED.DTLB_MISS_LOADS",		0xD0,	0x11, true },
	{ "MEM_UOPS_RETIRED.DTLB_MISS_STORES",		0xD0,	0x12, true },
	{ "MEM_UOPS_RETIRED.DTLB_MISS",			0xD0,	0x13, true },
	{ "MEM_UOPS_RETIRED.LOCK_LOADS",		0xD0,	0x21, true },
	{ "MEM_UOPS_RETIRED.SPLIT_LOADS",		0xD0,	0x41, true },
	{ "MEM_UOPS_RETIRED.SPLIT_STORES",		0xD0,	0x42, true },
	{ "MEM_UOPS_RETIRED.SPLIT",			0xD0,	0x43, true },
	{ "MEM_LOAD_UOPS_RETIRED.L1_HIT",		0xD1,	0x01, true },
	{ "MEM_LOAD_UOPS_RETIRED.L1_MISS",		0xD1,	0x08, true },
	{ "MEM_LOAD_UOPS_RETIRED.L2_HIT",		0xD1,	0x02, true },
	{ "MEM_LOAD_UOPS_RETIRED.L2_MISS",		0xD1,	0x10, true },
	{ "MEM_LOAD_UOPS_RETIRED.HITM",			0xD1,	0x20, true },
	{ "MEM_LOAD_UOPS_RETIRED.WCB_HIT",		0xD1,	0x40, true },
	{ "MEM_LOAD_UOPS_RETIRED.DRAM_HIT",		0xD1,	0x80, true },
	{ "BACLEARS.ALL",				0xE6,	0x01, true },
	{ "BACLEARS.RETURN",				0xE6,	0x08, true },
	{ "BACLEAR.CONDS",				0xE6,	0x10, true },
	{ "MS_DECODED.MS_ENTRY",			0xE7,	0x01, true },
	{ "DECODED_RESTRICTION.PREDECODE_WRONG",	0xE9,	0x01, true },
};

static struct event_table intel_goldmont = {
	.tablename = "Intel Goldmont",
	.names = intel_goldmont_names,
	.nevents = sizeof(intel_goldmont_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_intel_goldmont(void)
{

	return &intel_goldmont;
}

/*
 * Intel Goldmont Plus (Additions from Goldmont)
 */
static struct name_to_event intel_goldmontplus_names[] = {
	{ "INST_RETIRED.ANY",				0x00,	0x01, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_4K",		0x08,	0x02, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_2M_4M",	0x08,	0x04, true },
	{ "DTLB_LOAD_MISSES.WALK_COMPLETED_1GB",	0x08,	0x08, true },
	{ "DTLB_LOAD_MISSES.WALK_PENDING",		0x08,	0x10, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_4K",	0x49,	0x02, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_2M_4M",	0x49,	0x04, true },
	{ "DTLB_STORE_MISSES.WALK_COMPLETED_1GB",	0x49,	0x08, true },
	{ "DTLB_STORE_MISSES.WALK_PENDING",		0x49,	0x10, true },
	{ "EPT.WALK_PENDING",				0x4F,	0x10, true },
	{ "ITLB_MISSES.WALK_COMPLETED_4K",		0x85,	0x08, true },
	{ "ITLB_MISSES.WALK_COMPLETED_2M_4M",		0x85,	0x04, true },
	{ "ITLB_MISSES.WALK_COMPLETED_1GB",		0x85,	0x08, true },
	{ "ITLB_MISSES.WALK_PENDING",			0x85,	0x10, true },
	{ "TLB_FLUSHES.STLB_ANY",			0xBD,	0x20, true },
	{ "MACHINE_CLEARS.PAGE_FAULT",			0xC3,	0x20, true },
};

static struct event_table intel_goldmontplus = {
	.tablename = "Intel Goldmont Plus",
	.names = intel_goldmontplus_names,
	.nevents = sizeof(intel_goldmontplus_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

static struct event_table *
init_intel_goldmontplus(void)
{

	intel_goldmont.next = &intel_goldmontplus;

	return &intel_goldmont;
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
		case 0x37: /* Silvermont (Bay Trail) */
		case 0x4A: /* Silvermont (Tangier) */
		case 0x4C: /* Airmont (Braswell, Cherry Trail) */
		case 0x4D: /* Silvermont (Avoton, Rangeley) */
		case 0x5A: /* Silvermont (Anniedale) */
		case 0x5D: /* Silvermont (SoFIA) */
			table->next = init_intel_silvermont_airmont();
			break;
		case 0x5C: /* Goldmont (Apollo Lake) */
		case 0x5F: /* Goldmont (Denverton) */
			table->next = init_intel_goldmont();
			break;
		case 0x7A: /* Goldmont Plus (Gemini Lake) */
			table->next = init_intel_goldmontplus();
			break;
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

/*
 * AMD Family 15h
 */
static struct name_to_event amd_f15h_names[] = {
	{ "FpPipeAssignment",		0x000, 0x77, true },
	{ "FpSchedulerEmpty",		0x001, 0x00, true },
	{ "FpRetSseAvxOps",		0x003, 0xff, true },
	{ "FpNumMovElim",		0x004, 0x0f, true },
	{ "FpRetiredSerOps",		0x005, 0x0f, true },
	{ "LsSegRegLoads",		0x020, 0x7f, true },
	{ "LsPipeRestartSelfMod",	0x021, 0x00, true },
	{ "LsPipeRestartVarious",	0x022, 0x1f, true },
	{ "LsLoadQueueStoreQFull",	0x023, 0x03, true },
	{ "LsLockedOps",		0x024, 0x00, true },
	{ "LsRetClflushInstr",		0x026, 0x00, true },
	{ "LsRetCpuidInstr",		0x027, 0x00, true },
	{ "LsDispatch",			0x029, 0x07, true },
	{ "LsCanStoreToLoadFwOps",	0x02a, 0x03, true },
	{ "LsSmisReceived",		0x02b, 0x00, true },
	{ "LsExecClflushInstr",		0x030, 0x00, true },
	{ "LsMisalignStore",		0x032, 0x00, true },
	{ "LsFpLoadBufStall",		0x034, 0x00, true },
	{ "LsStlf",			0x035, 0x00, true },
	{ "DcCacheAccess",		0x040, 0x00, true },
	{ "DcCacheMiss",		0x041, 0x00, true },
	{ "DcCacheFillL2Sys",		0x042, 0x1f, true },
	{ "DcCacheFillSys",		0x043, 0x00, true },
	{ "DcUnifiedTlbHit",		0x045, 0x77, true },
	{ "DcUnifiedTlbMiss",		0x046, 0x77, true },
	{ "DcMisalignAccess",		0x047, 0x00, true },
	{ "DcPrefetchInstrDisp",	0x04b, 0x07, true },
	{ "DcIneffSwPrefetch",		0x052, 0x09, true },
	{ "CuCmdVictimBuf",		0x060, 0x98, true },
	{ "CuCmdMaskedOps",		0x061, 0x65, true },
	{ "CuCmdReadBlkOps",		0x062, 0x77, true },
	{ "CuCmdChgDirtyOps",		0x063, 0x08, true },
	{ "CuDramSysReq",		0x064, 0x00, true },
	{ "CuMemReqByType",		0x065, 0x83, true },
	{ "CuDataCachePrefetch",	0x067, 0x03, true },
	{ "CuMabReq",			0x068, 0xff, true },
	{ "CuMabWaitCyc",		0x069, 0xff, true },
	{ "CuSysRespCacheFill",		0x06c, 0x3f, true },
	{ "CuOctwordsWritten",		0x06d, 0x01, true },
	{ "CuCacheXInv",		0x075, 0x0f, true },
	{ "CuCpuClkNotHalted",		0x076, 0x00, true },
	{ "CuL2Req",			0x07d, 0x5f, true },
	{ "CuL2Miss",			0x07e, 0x17, true },
	{ "CuL2FillWb",			0x07f, 0x07, true },
	{ "CuPageSplintering",		0x165, 0x07, true },
	{ "CuL2PrefetchTrigEv",		0x16c, 0x03, true },
	{ "CuXabAllocStall",		0x177, 0x03, true },
	{ "CuFreeXabEntries",		0x17f, 0x01, true },
	{ "IcCacheFetch",		0x080, 0x00, true },
	{ "IcCacheMiss",		0x081, 0x00, true },
	{ "IcCacheFillL2",		0x082, 0x00, true },
	{ "IcCacheFillSys",		0x083, 0x00, true },
	{ "IcL1TlbMissL2Hit",		0x084, 0x00, true },
	{ "IcL1TlbMissL2Miss",		0x085, 0x07, true },
	{ "IcPipeRestartInstrStrProbe",	0x086, 0x00, true },
	{ "IcFetchStall",		0x087, 0x00, true },
	{ "IcRetStackHits",		0x088, 0x00, true },
	{ "IcRetStackOver",		0x089, 0x00, true },
	{ "IcCacheVictims",		0x08b, 0x00, true },
	{ "IcCacheLinesInv",		0x08c, 0x0f, true },
	{ "IcTlbReload",		0x099, 0x00, true },
	{ "IcTlbReloadAbort",		0x09a, 0x00, true },
	{ "IcUopsDispatched",		0x186, 0x01, true },
	{ "ExRetInstr",			0x0c0, 0x00, true },
	{ "ExRetCops",			0x0c1, 0x00, true },
	{ "ExRetBrn",			0x0c2, 0x00, true },
	{ "ExRetBrnMisp",		0x0c3, 0x00, true },
	{ "ExRetBrnTkn",		0x0c4, 0x00, true },
	{ "ExRetBrnTknMisp",		0x0c5, 0x00, true },
	{ "ExRetBrnFar",		0x0c6, 0x00, true },
	{ "ExRetBrnResync",		0x0c7, 0x00, true },
	{ "ExRetNearRet",		0x0c8, 0x00, true },
	{ "ExRetNearRetMispred",	0x0c9, 0x00, true },
	{ "ExRetBrnIndMisp",		0x0ca, 0x00, true },
	{ "ExRetMmxFpInstr@X87",	0x0cb, 0x01, true },
	{ "ExRetMmxFpInstr@Mmx",	0x0cb, 0x02, true },
	{ "ExRetMmxFpInstr@Sse",	0x0cb, 0x04, true },
	{ "ExIntMaskedCyc",		0x0cd, 0x00, true },
	{ "ExIntMaskedCycIntPend",	0x0ce, 0x00, true },
	{ "ExIntTaken",			0x0cf, 0x00, true },
	{ "ExDecEmpty",			0x0d0, 0x00, true },
	{ "ExDispStall",		0x0d1, 0x00, true },
	{ "ExUseqStallSer",		0x0d2, 0x00, true },
	{ "ExDispStallInstrRetQFull",	0x0d5, 0x00, true },
	{ "ExDispStallIntSchedQFull",	0x0d6, 0x00, true },
	{ "ExDispStallFpSchedQFull",	0x0d7, 0x00, true },
	{ "ExDispStallLdqFull",		0x0d8, 0x00, true },
	{ "ExUseqStallAllQuiet",	0x0d9, 0x00, true },
	{ "ExFpuEx",			0x0db, 0x1f, true },
	{ "ExBpDr0",			0x0dc, 0x8f, true },
	{ "ExBpDr1",			0x0dd, 0x8f, true },
	{ "ExBpDr2",			0x0de, 0x8f, true },
	{ "ExBpDr3",			0x0df, 0x8f, true },
	{ "ExRetx87FpOps",		0x1c0, 0x07, true },
	{ "ExTaggedIbsOps",		0x1cf, 0x07, true },
	{ "ExRetFusBrInstr",		0x1d0, 0x00, true },
	{ "ExDispStallStqFull",		0x1d8, 0x00, true },
	{ "ExCycNoDispIntPrfTok",	0x1dd, 0x00, true },
	{ "ExCycNoDispfpPrfTok",	0x1de, 0x00, true },
	{ "ExFpDispContention",		0x1df, 0x0f, true },
};

static struct event_table amd_f15h = {
	.tablename = "AMD Family 15h",
	.names = amd_f15h_names,
	.nevents = sizeof(amd_f15h_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

/*
 * AMD Family 17h
 */
static struct name_to_event amd_f17h_names[] = {
	{ "FpRetx87FpOps",		0x02, __BITS(2,0), true },
	{ "FpRetSseAvxOps",		0x03, __BITS(7,0), true },
	{ "FpRetiredSerOps",		0x05, __BITS(3,0), true },
	{ "LsL1DTlbMiss",		0x45, __BITS(7,0), true },
	{ "LsTableWalker",		0x46, __BITS(3,0), true },
	{ "LsMisalAccesses",		0x47, 0x00, true },
	{ "LsInefSwPref",		0x52, __BITS(1,0), true },
	{ "LsNotHaltedCyc",		0x76, 0x00, true },
	{ "IcFw32",			0x80, 0x00, true },
	{ "IcFw32Miss",			0x81, 0x00, true },
	{ "IcCacheFillL2",		0x82, 0x00, true },
	{ "IcCacheFillSys",		0x83, 0x00, true },
	{ "IcFetchStall",		0x87, __BITS(2,0), true },
	{ "IcCacheInval",		0x8C, __BITS(1,0), true },
	{ "BpL1TlbMissL2Hit",		0x84, 0x00, true },
	{ "BpL1TlbMissL2Miss",		0x85, 0x00, true },
	{ "BpSnpReSync",		0x86, 0x00, true },
	{ "BpL1BTBCorrect",		0x8A, 0x00, true },
	{ "BpL2BTBCorrect",		0x8B, 0x00, true },
	{ "BpTlbRel",			0x99, 0x00, true },
	{ "ExRetInstr",			0xC0, 0x00, true },
	{ "ExRetCops",			0xC1, 0x00, true },
	{ "ExRetBrn",			0xC2, 0x00, true },
	{ "ExRetBrnMisp",		0xC3, 0x00, true },
	{ "ExRetBrnTkn",		0xC4, 0x00, true },
	{ "ExRetBrnTknMisp",		0xC5, 0x00, true },
	{ "ExRetBrnFar",		0xC6, 0x00, true },
	{ "ExRetBrnResync",		0xC7, 0x00, true },
	{ "ExRetBrnIndMisp",		0xCA, 0x00, true },
	{ "ExRetNearRet",		0xC8, 0x00, true },
	{ "ExRetNearRetMispred",	0xC9, 0x00, true },
	{ "ExRetMmxFpInstr@X87",	0xCB, __BIT(0), true },
	{ "ExRetMmxFpInstr@Mmx",	0xCB, __BIT(1), true },
	{ "ExRetMmxFpInstr@Sse",	0xCB, __BIT(2), true },
	{ "ExRetCond",			0xD1, 0x00, true },
	{ "ExRetCondMisp",		0xD2, 0x00, true },
	{ "ExDivBusy",			0xD3, 0x00, true },
	{ "ExDivCount",			0xD4, 0x00, true },
};

static struct event_table amd_f17h = {
	.tablename = "AMD Family 17h",
	.names = amd_f17h_names,
	.nevents = sizeof(amd_f17h_names) /
	    sizeof(struct name_to_event),
	.next = NULL
};

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
		return &amd_f10h;
	case 0x15:
		return &amd_f15h;
	case 0x17:
		return &amd_f17h;
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
