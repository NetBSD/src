/*	$NetBSD: tprof_x86.c,v 1.1 2018/07/13 07:56:29 maxv Exp $	*/

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
 * Intel Skylake/Kabylake. TODO: there are many more events available.
 */
static struct name_to_event intel_skylake_kabylake_names[] = {
	/* Event Name - Event Select - UMask */
	{ "itlb-misses-causes-a-walk",	0x85, 0x01, true },
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

	switch (CPUID_TO_MODEL(eax)) {
	case 0x4E: /* Skylake */
	case 0x5E: /* Skylake */
	case 0x8E: /* Kabylake */
	case 0x9E: /* Kabylake */
		table->next = init_intel_skylake_kabylake();
		break;
	}

	return table;
}

/* -------------------------------------------------------------------------- */

/*
 * AMD Family 10h
 */
static struct name_to_event amd_f10h_names[] = {
	{ "seg-load-all",		F10H_SEGMENT_REG_LOADS,		0x7f, true },
	{ "seg-load-es",		F10H_SEGMENT_REG_LOADS,		0x01, true },
	{ "seg-load-cs",		F10H_SEGMENT_REG_LOADS,		0x02, true },
	{ "seg-load-ss",		F10H_SEGMENT_REG_LOADS,		0x04, true },
	{ "seg-load-ds",		F10H_SEGMENT_REG_LOADS,		0x08, true },
	{ "seg-load-fs",		F10H_SEGMENT_REG_LOADS,		0x10, true },
	{ "seg-load-gs",		F10H_SEGMENT_REG_LOADS,		0x20, true },
	{ "seg-load-hs",		F10H_SEGMENT_REG_LOADS,		0x40, true },
	{ "l1cache-access",		F10H_DATA_CACHE_ACCESS,		0, true },
	{ "l1cache-miss",		F10H_DATA_CACHE_MISS,		0, true },
	{ "l1cache-refill",		F10H_DATA_CACHE_REFILL_FROM_L2,	0x1f, true },
	{ "l1cache-refill-invalid",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x01, true },
	{ "l1cache-refill-shared",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x02, true },
	{ "l1cache-refill-exclusive",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x04, true },
	{ "l1cache-refill-owner",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x08, true },
	{ "l1cache-refill-modified",	F10H_DATA_CACHE_REFILL_FROM_L2,	0x10, true },
	{ "l1cache-load",		F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x1f, true },
	{ "l1cache-load-invalid",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x01, true },
	{ "l1cache-load-shared",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x02, true },
	{ "l1cache-load-exclusive",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x04, true },
	{ "l1cache-load-owner",		F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x08, true },
	{ "l1cache-load-modified",	F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE,0x10, true },
	{ "l1cache-writeback",		F10H_CACHE_LINES_EVICTED,	0x1f, true },
	{ "l1cache-writeback-invalid",	F10H_CACHE_LINES_EVICTED,	0x01, true },
	{ "l1cache-writeback-shared",	F10H_CACHE_LINES_EVICTED,	0x02, true },
	{ "l1cache-writeback-exclusive",F10H_CACHE_LINES_EVICTED,	0x04, true },
	{ "l1cache-writeback-owner",	F10H_CACHE_LINES_EVICTED,	0x08, true },
	{ "l1cache-writeback-modified",	F10H_CACHE_LINES_EVICTED,	0x10, true },
	{ "l1DTLB-hit-all",		F10H_L1_DTLB_HIT,		0x07, true },
	{ "l1DTLB-hit-4Kpage",		F10H_L1_DTLB_HIT,		0x01, true },
	{ "l1DTLB-hit-2Mpage",		F10H_L1_DTLB_HIT,		0x02, true },
	{ "l1DTLB-hit-1Gpage",		F10H_L1_DTLB_HIT,		0x04, true },
	{ "l1DTLB-miss-all",		F10H_L1_DTLB_MISS,		0x07, true },
	{ "l1DTLB-miss-4Kpage",		F10H_L1_DTLB_MISS,		0x01, true },
	{ "l1DTLB-miss-2Mpage",		F10H_L1_DTLB_MISS,		0x02, true },
	{ "l1DTLB-miss-1Gpage",		F10H_L1_DTLB_MISS,		0x04, true },
	{ "l2DTLB-miss-all",		F10H_L2_DTLB_MISS,		0x03, true },
	{ "l2DTLB-miss-4Kpage",		F10H_L2_DTLB_MISS,		0x01, true },
	{ "l2DTLB-miss-2Mpage",		F10H_L2_DTLB_MISS,		0x02, true },
	/* l2DTLB-miss-1Gpage: reserved on some revisions, so disabled */
	{ "l1ITLB-miss",		F10H_L1_ITLB_MISS,		0, true },
	{ "l2ITLB-miss-all",		F10H_L2_ITLB_MISS,		0x03, true },
	{ "l2ITLB-miss-4Kpage",		F10H_L2_ITLB_MISS,		0x01, true },
	{ "l2ITLB-miss-2Mpage",		F10H_L2_ITLB_MISS,		0x02, true },
	{ "mem-misalign-ref",		F10H_MISALIGNED_ACCESS,		0, true },
	{ "ins-fetch",			F10H_INSTRUCTION_CACHE_FETCH,	0, true },
	{ "ins-fetch-miss",		F10H_INSTRUCTION_CACHE_MISS,	0, true },
	{ "ins-refill-l2",		F10H_INSTRUCTION_CACHE_REFILL_FROM_L2,	0, true },
	{ "ins-refill-sys",		F10H_INSTRUCTION_CACHE_REFILL_FROM_SYS,	0, true },
	{ "ins-fetch-stall",		F10H_INSTRUCTION_FETCH_STALL,	0, true },
	{ "ins-retired",		F10H_RETIRED_INSTRUCTIONS,	0, true },
	{ "ins-empty",			F10H_DECODER_EMPTY,	0, true },
	{ "ops-retired",		F10H_RETIRED_UOPS,		0, true },
	{ "branch-retired",		F10H_RETIRED_BRANCH,		0, true },
	{ "branch-miss-retired",	F10H_RETIRED_MISPREDICTED_BRANCH,0, true },
	{ "branch-taken-retired",	F10H_RETIRED_TAKEN_BRANCH,	0, true },
	{ "branch-taken-miss-retired",	F10H_RETIRED_TAKEN_BRANCH_MISPREDICTED,	0, true },
	{ "branch-far-retired", 	F10H_RETIRED_FAR_CONTROL_TRANSFER, 0, true },
	{ "branch-resync-retired",	F10H_RETIRED_BRANCH_RESYNC,	0, true },
	{ "branch-near-retired",	F10H_RETIRED_NEAR_RETURNS,	0, true },
	{ "branch-near-miss-retired",	F10H_RETIRED_NEAR_RETURNS_MISPREDICTED,	0, true },
	{ "branch-indirect-miss-retired", F10H_RETIRED_INDIRECT_BRANCH_MISPREDICTED,	0, true },
	{ "int-hw",			F10H_INTERRUPTS_TAKEN,		0, true },
	{ "int-cycles-masked",		F10H_INTERRUPTS_MASKED_CYCLES,	0, true },
	{ "int-cycles-masked-pending",
	    F10H_INTERRUPTS_MASKED_CYCLES_INTERRUPT_PENDING, 0, true },
	{ "fpu-exceptions",		F10H_FPU_EXCEPTIONS, 0, true },
	{ "break-match0",		F10H_DR0_BREAKPOINT_MATCHES,	0, true },
	{ "break-match1",		F10H_DR1_BREAKPOINT_MATCHES,	0, true },
	{ "break-match2",		F10H_DR2_BREAKPOINT_MATCHES,	0, true },
	{ "break-match3",		F10H_DR3_BREAKPOINT_MATCHES,	0, true },
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
	printf("\n");

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
