/*	$NetBSD: cc_microtime.h,v 1.2.14.1 2006/06/19 04:11:12 chap Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SYS_CC_MICROTIME_H_
#define	_SYS_CC_MICROTIME_H_

#ifndef __HAVE_TIMECOUNTER
extern struct timeval cc_microset_time;
void cc_microtime(struct timeval *);
void cc_microset(struct cpu_info *);

/*
 * Variables used by cc_microtime().
 */
struct cc_microtime_state {
	struct timeval cc_time;
	int64_t cc_cc;
	int64_t cc_ms_delta;
	int64_t cc_denom;
};
#else
/* XXX this file is not named right - rather something like tsc.h */
void tsc_calibrate_cpu(struct cpu_info *);
/*
 * Variables used by cc_microtime().
 */
struct cc_microtime_state {
	volatile u_int   cc_gen;   /* generation number for this data set */
	volatile int64_t cc_val;   /* reference TSC value at calibration time */
	volatile int64_t cc_cc;	   /* local TSC value at calibration time */
	volatile int64_t cc_delta; /* reference TSC difference for
				      last calibration period */
	volatile int64_t cc_denom; /* local TSC difference for
				      last calibration period */
};
#endif /* __HAVE_TIMECOUNTER */

#endif /* _SYS_CC_MICROTIME_H_ */
