/*	$NetBSD: trap.h,v 1.2 2011/01/18 01:02:54 matt Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_TRAP_H_
#define _POWERPC_BOOKE_TRAP_H_

enum ppc_booke_exceptions {
	T_CRITIAL_INPUT=0,
	T_MACHINE_CHECK=1,
	T_DSI=2,
	T_ISI=3,
	T_EXTERNAL_INPUT=4,
	T_ALIGNMENT=5,
	T_PROGRAM=6,
	T_FP_UNAVAILABLE=7,
	T_SYSTEM_CALL=8,
	T_AP_UNAVAILABLE=9,
	T_DECREMENTER=10,
	T_FIXED_INTERVAL=11,
	T_WATCHDOG=12,
	T_DATA_TLB_ERROR=13,
	T_INSTRUCTION_TLB_ERROR=14,
	T_DEBUG=15,
	T_SPE_UNAVAILABLE=32,
	T_EMBEDDED_FP_DATA=33,
	T_EMBEDDED_FP_ROUND=34,
	T_EMBEDDED_PERF_MONITOR=35,
	T_AST=64,
};

#endif /* _POWERPC_BOOKE_TRAP_H_ */
