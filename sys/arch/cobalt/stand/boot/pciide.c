/*	$NetBSD: pciide.c,v 1.3 2004/01/03 01:50:53 thorpej Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <mips/cpuregs.h>

#include "boot.h"
#include "wdvar.h"

int
pciide_init(chp, unit)
	struct wdc_channel *chp;
	u_int *unit;
{
	u_long bpa, addr;
	int compatchan = 0;

	/*
	 * two channels per chip, two drives per channel
	 */
	compatchan = *unit / PCIIDE_CHANNEL_NDEV;
	if (compatchan >= PCIIDE_NUM_CHANNELS)
		return (ENXIO);
	*unit %= PCIIDE_CHANNEL_NDEV;

	DPRINTF(("[pciide] unit: %d, channel: %d\n", *unit, compatchan));

	/*
	 * XXX map?
	 */
	bpa = PCIIDE_COMPAT_CMD_BASE(compatchan);
	addr = MIPS_PHYS_TO_KSEG1(bpa);
	if (bpa < 0x10000000)
		addr += 0x10000000;

	chp->c_base = (u_int8_t*)addr;
	chp->c_data = (u_int16_t*)(addr + wd_data);
	return (0);
}
