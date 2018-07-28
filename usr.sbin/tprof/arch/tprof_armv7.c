/* $NetBSD: tprof_armv7.c,v 1.1.2.2 2018/07/28 04:38:15 pgoyette Exp $ */

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
 * Common event numbers, from ARM ARM.
 */
static struct pmu_event pmu_armv7_common_events[] = {
	{ 0x00,	"SW_INCR" },
	{ 0x01,	"L1I_CACHE_REFILL" },
	{ 0x02,	"L1I_TLB_REFILL" },
	{ 0x03,	"L1D_CACHE_REFILL" },
	{ 0x04,	"L1D_CACHE" },
	{ 0x05,	"L1D_TLB_REFILL" },
	{ 0x06,	"LD_RETIRED" },
	{ 0x07,	"ST_RETIRED" },
	{ 0x08,	"INST_RETIRED" },
	{ 0x09,	"EXC_TAKEN" },
	{ 0x0a,	"EXC_RETURN" },
	{ 0x0b,	"CID_WRITE_RETIRED" },
	{ 0x0c,	"PC_WRITE_RETIRED" },
	{ 0x0d,	"BR_IMMED_RETIRED" },
	{ 0x0e,	"BR_RETURN_RETIRED" },
	{ 0x0f,	"UNALIGNED_LDST_RETIRED" },
	{ 0x10,	"BR_MIS_PRED" },
	{ 0x11,	"CPU_CYCLES" },
	{ 0x12,	"BR_PRED" },
	{ 0x13,	"MEM_ACCESS" },
	{ 0x14,	"L1I_CACHE" },
	{ 0x15,	"L1D_CACHE_WB" },
	{ 0x16,	"L2D_CACHE" },
	{ 0x17,	"L2D_CACHE_REFILL" },
	{ 0x18,	"L2D_CACHE_WB" },
	{ 0x19,	"BUS_ACCESS" },
	{ 0x1a,	"MEMORY_ERROR" },
	{ 0x1b,	"INST_SPEC" },
	{ 0x1c,	"TTBR_WRITE_RETIRED" },
	{ 0x1d,	"BUS_CYCLES" },
};

static struct pmu_event_table pmu_armv7 = {
	.name = "ARMv7 common architectural and microarchitectural events",
	.events = pmu_armv7_common_events,
	.nevents = __arraycount(pmu_armv7_common_events),
	.next = NULL
};

static struct pmu_event_table *events = NULL;

int
tprof_event_init(uint32_t ident)
{
	if (ident != TPROF_IDENT_ARMV7_GENERIC)
		return -1;

	events = &pmu_armv7;

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
