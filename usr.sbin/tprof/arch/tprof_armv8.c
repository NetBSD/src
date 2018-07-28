/* $NetBSD: tprof_armv8.c,v 1.1.2.2 2018/07/28 04:38:15 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <dev/tprof/tprof_ioctl.h>
#include "../tprof.h"

struct pmu_event {
	uint16_t		event;
	const char		*name;
};

struct pmu_event_table {
	const char		*name;
	struct pmu_event	*events;
	u_int			nevents;
	struct pmu_event_table	*next;
};

/*
 * Common event numbers, from ARMv8 ARM.
 */
static struct pmu_event pmu_armv8_common_events[] = {
	{ 0x0000,	"SW_INCR" },
	{ 0x0001,	"L1I_CACHE_REFILL" },
	{ 0x0002,	"L1I_TLB_REFILL" },
	{ 0x0003,	"L1D_CACHE_REFILL" },
	{ 0x0004,	"L1D_CACHE" },
	{ 0x0005,	"L1D_TLB_REFILL" },
	{ 0x0006,	"LD_RETIRED" },
	{ 0x0007,	"ST_RETIRED" },
	{ 0x0008,	"INST_RETIRED" },
	{ 0x0009,	"EXC_TAKEN" },
	{ 0x000a,	"EXC_RETURN" },
	{ 0x000b,	"CID_WRITE_RETIRED" },
	{ 0x000c,	"PC_WRITE_RETIRED" },
	{ 0x000d,	"BR_IMMED_RETIRED" },
	{ 0x000e,	"BR_RETURN_RETIRED" },
	{ 0x000f,	"UNALIGNED_LDST_RETIRED" },
	{ 0x0010,	"BR_MIS_PRED" },
	{ 0x0011,	"CPU_CYCLES" },
	{ 0x0012,	"BR_PRED" },
	{ 0x0013,	"MEM_ACCESS" },
	{ 0x0014,	"L1I_CACHE" },
	{ 0x0015,	"L1D_CACHE_WB" },
	{ 0x0016,	"L2D_CACHE" },
	{ 0x0017,	"L2D_CACHE_REFILL" },
	{ 0x0018,	"L2D_CACHE_WB" },
	{ 0x0019,	"BUS_ACCESS" },
	{ 0x001a,	"MEMORY_ERROR" },
	{ 0x001b,	"INST_SPEC" },
	{ 0x001c,	"TTBR_WRITE_RETIRED" },
	{ 0x001d,	"BUS_CYCLES" },
	{ 0x001e,	"CHAIN" },
	{ 0x001f,	"L1D_CACHE_ALLOCATE" },
	{ 0x0020,	"L2D_CACHE_ALLOCATE" },
	{ 0x0021,	"BR_RETIRED" },
	{ 0x0022,	"BR_MIS_PRED_RETIRED" },
	{ 0x0023,	"STALL_FRONTEND" },
	{ 0x0024,	"STALL_BACKEND" },
	{ 0x0025,	"L1D_TLB" },
	{ 0x0026,	"L1I_TLB" },
	{ 0x0027,	"L2I_CACHE" },
	{ 0x0028,	"L2I_CACHE_REFILL" },
	{ 0x0029,	"L3D_CACHE_ALLOCATE" },
	{ 0x002a,	"L3D_CACHE_REFILL" },
	{ 0x002b,	"L3D_CACHE" },
	{ 0x002c,	"L3D_CACHE_WB" },
	{ 0x002d,	"L2D_TLB_REFILL" },
	{ 0x002e,	"L2I_TLB_REFILL" },
	{ 0x002f,	"L2D_TLB" },
	{ 0x0030,	"L2I_TLB" },
	{ 0x0031,	"REMOTE_ACCESS" },
	{ 0x0032,	"LL_CACHE" },
	{ 0x0033,	"LL_CACHE_MISS" },
	{ 0x0034,	"DTLB_WALK" },
	{ 0x0035,	"ITLB_WALK" },
	{ 0x0036,	"LL_CACHE_RD" },
	{ 0x0037,	"LL_CACHE_MISS_RD" },
	{ 0x0038,	"REMOTE_ACCESS_RD" },
};

static struct pmu_event_table pmu_armv8 = {
	.name = "ARMv8 common architectural and microarchitectural events",
	.events = pmu_armv8_common_events,
	.nevents = __arraycount(pmu_armv8_common_events),
	.next = NULL
};

static struct pmu_event_table *events = NULL;

int
tprof_event_init(uint32_t ident)
{
	if (ident != TPROF_IDENT_ARMV8_GENERIC)
		return -1;

	events = &pmu_armv8;

	return 0;
}

static void
tprof_event_list_table(struct pmu_event_table *tbl)
{
	u_int n;

	printf("%s:\n", tbl->name);
	for (n = 0; n < tbl->nevents; n++) {
		printf("\t%s\n", tbl->events[n].name);
	}

	if (tbl->next)
		tprof_event_list_table(tbl->next);
}

void
tprof_event_list(void)
{
	tprof_event_list_table(events);
}

static void
tprof_event_lookup_table(const char *name, struct tprof_param *param,
    struct pmu_event_table *tbl)
{
	u_int n;

	for (n = 0; n < tbl->nevents; n++) {
		if (strcmp(tbl->events[n].name, name) == 0) {
			param->p_event = tbl->events[n].event;
			param->p_unit = 0;
			return;
		}
	}

	if (tbl->next)
		tprof_event_lookup_table(name, param, tbl->next);
	else
		errx(EXIT_FAILURE, "event '%s' unknown", name);
}

void
tprof_event_lookup(const char *name, struct tprof_param *param)
{
	tprof_event_lookup_table(name, param, events);
}
