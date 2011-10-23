/*	$NetBSD: iscsi_profile.c,v 1.1 2011/10/23 21:15:02 agc Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
#include "iscsi_globals.h"

#ifdef ISCSI_PERFTEST

command_perf_t *perfdata = NULL;
int perf_active = 0;

static int perfcount = 0;
static int perfindex = 0;
static uint64_t ticks_per_100us = 0;

/*
 * perf_begin_command:
 *    Start command profiling.
 *    Allocates space for a command entry, sets the data length and
 *    flags, and gets the begin cycle count.
 *    If no room is left in the data buffer, nothing is set.
 *
 *    Note: The perf_index set in the CCB is the index + 1, so that 0 is
 *          reserved as profiling not active (this is the default state
 *          for CCB fields).
 *
 *    This function must only be called if perf_active is nonzero.
 *    The wrapper macro checks for this condition.
 *
 *    Parameter:
 *          ccb      The associated CCB.
 *          nowait   Nonzero if this is a no-wait command.
 */

void
perf_begin_command(ccb_t *ccb, int nowait)
{
	int idx;

	/* Only trace commands that actually transfer data so we don't skew results */
	if (ccb->data_len < 512) {
		return;
	}
	idx = perfindex++;
	if (idx >= perfcount) {
		--perfindex;
		return;
	}
	perfdata[idx].datalen = ccb->data_len;
	if (ccb->data_in) {
		perfdata[idx].status |= PERF_FLG_READ;
	}
	if (nowait) {
		perfdata[idx].status |= PERF_FLG_NOWAIT;
	}
	ccb->perf_index = ++idx;
	perf_snap(idx, PERF_BEGIN_COMMAND);
}


/*
 * perf_end_command:
 *    Ends command profiling.
 *    Gets the end cycle count, and sets the complete flag.
 *
 *    Parameter:
 *          ccb      The associated CCB.
 */

void
perf_end_command(ccb_t *ccb)
{
	int idx = ccb->perf_index;

	perf_snap(idx, PERF_END_COMMAND);
	perfdata[idx - 1].status |=
			(ccb->status & 0xffffff) | PERF_FLG_COMPLETE;
	ccb->perf_index = 0;
}


/*
 * perf_do_stop:
 *    Stops performance data collection (internal function).
 *    Clears perf_active, and waits until all commands have completed.
 *    The wait is limited to 5 seconds.
 */

STATIC void
perf_do_stop(void)
{
	int i, n = 0;

	perf_active = 0;
	do {
		for (i = 0; i < perfindex; i++) {
			if (!(perfdata[i].status & PERF_FLG_COMPLETE)) {
				break;
			}
		}
		if (i < perfindex) {
			tsleep(&perfindex, PWAIT, "wait_perf", 1 * hz);
		}
	} while (n++ < 10 && i < perfindex);
}


/*
 * perf_start:
 *    Starts performance data collection.
 *    If collection is already running, it is stopped.
 *    Allocates a buffer if necessary, clears the buffer, resets indices.
 *    Measures tick timing.
 *
 *    Parameter:
 *          par      The ioctl parameter specifying the number of elements.
 */

void
perf_start(iscsi_perf_startstop_parameters_t *par)
{
	uint64_t rv, rv1, rv2;
	int i, s;

	if (par->num_elements <= 0) {
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}
	if (perf_active)
		perf_do_stop();

	if (perfdata == NULL || perfcount < par->num_elements) {
		if (perfdata != NULL) {
			free(perfdata, M_DEVBUF);
		}
		perfdata = malloc(par->num_elements * sizeof(*perfdata),
					M_DEVBUF, M_WAITOK);
		if (perfdata == NULL) {
			perfcount = 0;
			par->status = ISCSI_STATUS_NO_RESOURCES;
			return;
		}
		perfcount = par->num_elements;
	}

	(void) memset(perfdata, 0x0, perfcount * sizeof(*perfdata));
	perfindex = 0;

	rv = 0;
	for (i = 0; i < 5; i++) {
		s = splhigh();

		__asm __volatile("rdtsc":"=A"(rv1));
		delay(100);
		__asm __volatile("rdtsc":"=A"(rv2));

		splx(s);
		rv += rv2 - rv1;
	}

	ticks_per_100us = rv / 5;
	par->status = ISCSI_STATUS_SUCCESS;

	perf_active = 1;
}


/*
 * perf_start:
 *    Stops performance data collection (external interface).
 *
 *    Parameter:
 *          par      The ioctl parameter receiving the number of
 *                   elements collected.
 */

void
perf_stop(iscsi_perf_startstop_parameters_t *par)
{
	if (perfdata == NULL || perfcount <= 0 || !perf_active) {
		par->status = ISCSI_STATUS_GENERAL_ERROR;
		return;
	}
	perf_do_stop();
	par->num_elements = perfindex;
	par->status = ISCSI_STATUS_SUCCESS;
}


/*
 * perf_get:
 *    Retrieves performance data.
 *
 *    Parameter:
 *          par      The ioctl parameter.
 */

void
perf_get(iscsi_perf_get_parameters_t *par)
{
	int nelem;

	if (perfdata == NULL || perfcount <= 0) {
		par->status = ISCSI_STATUS_GENERAL_ERROR;
		return;
	}
	par->status = ISCSI_STATUS_SUCCESS;

	if (!par->buffersize) {
		par->buffersize = perfindex * sizeof(command_perf_t);
		par->num_elements = perfindex;
		return;
	}
	nelem = par->buffersize / sizeof(command_perf_t);
	if (!nelem) {
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}
	nelem = min(nelem, perfindex);
	copyout(perfdata, par->buffer, nelem * sizeof(command_perf_t));
	par->num_elements = nelem;
	par->ticks_per_100us = ticks_per_100us;
}

#endif
