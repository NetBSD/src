/*	$NetBSD: iscsi_profile.h,v 1.1 2011/10/23 21:15:02 agc Exp $	*/

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
#ifdef ISCSI_PERFTEST

command_perf_t *perfdata;
int perf_active;

void perf_begin_command(ccb_t *, int);
void perf_end_command(ccb_t *);
void perf_start(iscsi_perf_startstop_parameters_t *);
void perf_stop(iscsi_perf_startstop_parameters_t *);
void perf_get(iscsi_perf_get_parameters_t *);


static __inline void
perf_snap(int index, perfpoint_t which)
{
	uint64_t rv;

	__asm __volatile("rdtsc":"=A"(rv));

	perfdata[index - 1].pctr[which] = rv;
}

static __inline void
perf_snapcond(int index, perfpoint_t which)
{
	uint64_t *pdst = &perfdata[index - 1].pctr[which];
	uint64_t rv;

	if (!*pdst) {
		__asm __volatile("rdtsc":"=A"(rv));

		*pdst = rv;
	}
}

#define PERF_BEGIN(ccb, nowait) if (perf_active) perf_begin_command(ccb, nowait)
#define PERF_END(ccb) if (ccb->perf_index) perf_end_command(ccb)
#define PERF_SNAP(ccb,which) if (ccb->perf_index) \
								perf_snap(ccb->perf_index,which)
#define PERF_SNAPC(ccb,which) if (ccb->perf_index) \
								perf_snapcond(ccb->perf_index,which)
#define PERF_PDUSNAPB(pdu) if (pdu->perf_index) \
								perf_snapcond(pdu->perf_index,pdu->perf_which)
#define PERF_PDUSNAPE(pdu) if (pdu->perf_index) \
								perf_snap(pdu->perf_index,pdu->perf_which+1)
#define PERF_PDUSET(pdu,ccb,which) {pdu->perf_index = ccb->perf_index; \
									pdu->perf_which = which;}

#else

#define PERF_BEGIN(ccb, nowait)
#define PERF_END(ccb)
#define PERF_SNAP(ccb,which)
#define PERF_SNAPC(ccb,which)
#define PERF_PDUSNAPB(pdu)
#define PERF_PDUSNAPE(pdu)
#define PERF_PDUSET(pdu,ccb,which)

#endif
