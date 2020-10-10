/*	$NetBSD: cctr.h,v 1.4 2020/10/10 03:05:04 thorpej Exp $	*/

/*-
 * Copyright (c) 2004, 2020 The NetBSD Foundation, Inc.
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

#ifndef _SYS_CCTR_H_
#define	_SYS_CCTR_H_

#include <sys/timetc.h>

/*
 * Variables used by cycle counter in kern_cctr.c.
 */
struct cctr_state {
	volatile int64_t cc_delta;	/* delta from primary CPU CC */
	u_int cc_cal_ticks;		/* # ticks between calibrations */
	u_int cc_ticks;			/* # ticks since last calibration */
};

#ifdef _KERNEL
struct cpu_info;

struct timecounter *cc_init(timecounter_get_t, uint64_t, const char *, int);
void	cc_init_secondary(struct cpu_info *);
u_int	cc_get_timecount(struct timecounter *);
void	cc_calibrate_cpu(struct cpu_info *);
void	cc_primary_cc(void);

#define	cc_hardclock(ci)						\
do {									\
	if ((ci)->ci_cc.cc_cal_ticks &&					\
	    ++(ci)->ci_cc.cc_ticks == (ci)->ci_cc.cc_cal_ticks) {	\
		(ci)->ci_cc.cc_ticks = 0;				\
		cc_calibrate_cpu((ci));					\
	}								\
} while (/*CONSTCOND*/0)
#endif /* _KERNEL */

#endif /* _SYS_CCTR_H_ */
