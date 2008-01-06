/*	$NetBSD: clock.c,v 1.1.70.1 2008/01/06 05:00:53 wrstuden Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

#include <sys/param.h>
#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <lib/libsa/net.h>

#include <dev/ic/mc146818reg.h>

#include <mips/cpuregs.h>

#include "boot.h"

#define DELAY_CALIBRATE	1000

#define MCCLOCK_BASE	0x10000070
#define MCCLOCK_REG	0
#define MCCLOCK_DATA	1

void
delay(int ms)
{
	/*
	 * XXX need *real* clock calibration.
	 */
	volatile register int N = ms * DELAY_CALIBRATE;
	for (; --N;)
		__insn_barrier();
}

time_t
getsecs(void)
{
	volatile uint8_t *mcclock_reg, *mcclock_data;
	u_int sec;

	mcclock_reg  = (void *)MIPS_PHYS_TO_KSEG1(MCCLOCK_BASE + MCCLOCK_REG);
	mcclock_data = (void *)MIPS_PHYS_TO_KSEG1(MCCLOCK_BASE + MCCLOCK_DATA);

	*mcclock_reg = MC_SEC;
	sec  = bcdtobin(*mcclock_data);
	*mcclock_reg = MC_MIN;
	sec += bcdtobin(*mcclock_data) * 60;
	*mcclock_reg = MC_HOUR;
	sec += bcdtobin(*mcclock_data) * 60 * 60;

	return (time_t)sec;
}
