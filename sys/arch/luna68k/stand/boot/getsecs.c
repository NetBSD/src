/*	$NetBSD: getsecs.c,v 1.1.6.2 2013/02/25 00:28:49 tls Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <machine/cpu.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include <luna68k/dev/timekeeper.h>
#include <luna68k/stand/boot/samachdep.h>

satime_t
getsecs(void)
{
	volatile uint8_t *mclock;
	u_int t;

	mclock = (volatile uint8_t *)0x45000000;

	if (machtype == LUNA_I) {
		mclock += 2040;
		mclock[MK_CSR] |= MK_CSR_READ;
		t =  bcdtobin(mclock[MK_SEC]);
		t += bcdtobin(mclock[MK_MIN]) * 60;
		t += bcdtobin(mclock[MK_HOUR]) * 60 * 60;
		mclock[MK_CSR] &= ~MK_CSR_READ;
	} else {
		while (mclock[MC_REGA] & MC_REGA_UIP)
			continue;
		t =  mclock[MC_SEC];
		t += mclock[MC_MIN] * 60;
		t += mclock[MC_HOUR] * 60 * 60;
	}

	return (satime_t)t;
}
