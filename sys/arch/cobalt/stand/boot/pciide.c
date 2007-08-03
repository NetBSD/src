/*	$NetBSD: pciide.c,v 1.6.2.2 2007/08/03 13:15:57 tsutsui Exp $	*/

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

#define COBALT_IO_SPACE_BASE	0x10000000	/* XXX VT82C586 ISA I/O space */

int
pciide_init(struct wdc_channel *chp, u_int *unit)
{
	uint32_t cmdreg, ctlreg;
	int i, compatchan = 0;

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
	cmdreg = MIPS_PHYS_TO_KSEG1(COBALT_IO_SPACE_BASE +
	    PCIIDE_COMPAT_CMD_BASE(compatchan));
	ctlreg = MIPS_PHYS_TO_KSEG1(COBALT_IO_SPACE_BASE +
	    PCIIDE_COMPAT_CTL_BASE(compatchan));

	/* set up cmd regsiters */
	chp->c_cmdbase = (uint8_t *)cmdreg;
	chp->c_data = (uint16_t *)(cmdreg + wd_data);
	for (i = 0; i < WDC_NPORTS; i++)
		chp->c_cmdreg[i] = chp->c_cmdbase + i;
	/* set up shadow registers */
	chp->c_cmdreg[wd_status]   = chp->c_cmdreg[wd_command];
	chp->c_cmdreg[wd_features] = chp->c_cmdreg[wd_precomp];
	/* set up ctl registers */
	chp->c_ctlbase = (uint8_t *)ctlreg;

	return 0;
}
