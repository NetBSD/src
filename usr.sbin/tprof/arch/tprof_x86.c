/*	$NetBSD: tprof_x86.c,v 1.3 2018/07/13 09:53:42 maxv Exp $	*/

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
