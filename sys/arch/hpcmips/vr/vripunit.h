/*	$NetBSD: vripunit.h,v 1.2.2.2 2002/02/11 20:08:14 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMURA Shin
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
    
#ifndef _VRIPUNIT_H_
#define _VRIPUNIT_H_

enum vrip_unit_id {
	VRIP_UNIT_PMU,
	VRIP_UNIT_RTC,
	VRIP_UNIT_PIU,
	VRIP_UNIT_KIU,
	VRIP_UNIT_SIU,
	VRIP_UNIT_GIU,
	VRIP_UNIT_LED,
	VRIP_UNIT_AIU,
	VRIP_UNIT_FIR,
	VRIP_UNIT_DSIU,
	VRIP_UNIT_PCIU,
	VRIP_UNIT_SCU,
	VRIP_UNIT_CSI,
	VRIP_UNIT_BCU,
};

#ifdef VRIPUNIT_DEFINE_UNIT_NICKNAME
#define VRPMU	VRIP_UNIT_PMU
#define VRRTC	VRIP_UNIT_RTC
#define VRPIU	VRIP_UNIT_PIU
#define VRKIU	VRIP_UNIT_KIU
#define VRSIU	VRIP_UNIT_SIU
#define VRGIU	VRIP_UNIT_GIU
#define VRLED	VRIP_UNIT_LED
#define VRAIU	VRIP_UNIT_AIU
#define VRFIR	VRIP_UNIT_FIR
#define VRDSIU	VRIP_UNIT_DSIU
#define VRPCIU	VRIP_UNIT_PCIU
#define VRSCU	VRIP_UNIT_SCU
#define VRCSI	VRIP_UNIT_CSI
#define VRBCU	VRIP_UNIT_BCU
#endif VRIPUNIT_DEFINE_UNIT_NICKNAME

#endif /* _VRIPUNIT_H_ */
