/*	$NetBSD: cc_microtime.h,v 1.6 2008/04/28 20:24:10 martin Exp $	*/

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
	int64_t cc_pad[3];
};

#endif /* _SYS_CC_MICROTIME_H_ */
