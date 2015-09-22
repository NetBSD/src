/*	$NetBSD: ar_conf.c,v 1.2.30.1 2015/09/22 12:05:46 skrll Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: ar_conf.c,v 1.2.30.1 2015/09/22 12:05:46 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>

#include "opt_wisoc.h"

#include <mips/cpuregs.h>
#include <mips/locore.h>

#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/ar9344reg.h>

struct atheros_chip {
	const struct atheros_platformsw *ac_platformsw;
	const struct atheros_boardsw *ac_boardsw;
	uint8_t ac_chipid;
	uint8_t ac_chipmask;
	uint8_t ac_cid;
	uint8_t ac_pid;
	const char ac_name[8];
};

static const struct atheros_chip chips[] = {
#ifdef WISOC_AR5312
	{
		.ac_platformsw =	&ar5312_platformsw,
		.ac_boardsw =		&ar5312_boardsw,
		.ac_chipid =		ARCHIP_AR5312,
		.ac_chipmask =		0xff,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_4Kc,
		.ac_name =		"AR5312"
	},
#endif
#ifdef WISOC_AR5315
	{
		.ac_platformsw =	&ar5315_platformsw,
		.ac_boardsw =		&ar5315_boardsw,
		.ac_chipid =		ARCHIP_AR5315,
		.ac_chipmask =		0xf0,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_4Kc,
		.ac_name =		"AR5315"
	},
#endif
#ifdef WISOC_AR7100
	{
		.ac_platformsw =	&ar7100_platformsw,
		.ac_chipid =		ARCHIP_AR7130,
		.ac_chipmask =		0xf3,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_24K,
		.ac_name =		"AR7130"
	}, {
		.ac_platformsw =	&ar7100_platformsw,
		.ac_chipid =		ARCHIP_AR7141,
		.ac_chipmask =		0xf3,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_24K,
		.ac_name =		"AR7141"
	}, {
		.ac_platformsw =	&ar7100_platformsw,
		.ac_chipid =		ARCHIP_AR7161,
		.ac_chipmask =		0xf3,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_24K,
		.ac_name =		"AR7161"
	},
#endif
#ifdef WISOC_AR9344
	{
		.ac_platformsw =	&ar9344_platformsw,
		.ac_chipid =		0x20,	/* 7240? */
		.ac_chipmask =		0xff,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_74K,
		.ac_name =		"AR7240"
	}, {
		.ac_platformsw =	&ar9344_platformsw,
		.ac_chipid =		ARCHIP_AR9344,
		.ac_chipmask =		0xff,
		.ac_cid =		MIPS_PRID_CID_MTI,
		.ac_pid =		MIPS_74K,
		.ac_name =		"AR9344"
	},
#endif
};

__CTASSERT(__arraycount(chips) > 0);

const struct atheros_platformsw *platformsw;

static const struct atheros_chip *my_chip;

static struct arfreqs chip_freqs;

const char *
atheros_get_cpuname(void)
{
	return my_chip->ac_name;
}

u_int
atheros_get_chipid(void)
{
	return my_chip->ac_chipid;
}

uint32_t
atheros_get_uart_freq(void)
{
	if (chip_freqs.freq_uart)
		return chip_freqs.freq_uart;

	return chip_freqs.freq_bus;
}

uint32_t
atheros_get_bus_freq(void)
{
	return chip_freqs.freq_bus;
}

uint32_t
atheros_get_cpu_freq(void)
{
	return chip_freqs.freq_cpu;
}

uint32_t
atheros_get_mem_freq(void)
{
	return chip_freqs.freq_mem;
}

void
atheros_set_platformsw(void)
{
	const u_int cid = MIPS_PRID_CID(mips_options.mips_cpu_id);
	const u_int pid = MIPS_PRID_IMPL(mips_options.mips_cpu_id);

	for (const struct atheros_chip *ac = chips;
	    ac < chips + __arraycount(chips);
	    ac++) {
		const struct atheros_platformsw * const apsw = ac->ac_platformsw;
                if (cid != ac->ac_cid || pid != ac->ac_pid)
			continue;

		const uint32_t revision_id = *(volatile uint32_t *)
		    MIPS_PHYS_TO_KSEG1(apsw->apsw_revision_id_addr);
		const uint32_t chipid = AR9344_REVISION_CHIPID(revision_id);

		if ((chipid & ac->ac_chipmask) == ac->ac_chipid) {
			platformsw = apsw;
			my_chip = ac;
			atheros_early_consinit();
			printf("Early console started!\n");
			(*apsw->apsw_get_freqs)(&chip_freqs);
			printf("freqs: cpu=%u bus=%u mem=%u ref=%u pll=%u\n",
			     chip_freqs.freq_cpu,
			     chip_freqs.freq_bus,
			     chip_freqs.freq_mem,
			     chip_freqs.freq_ref,
			     chip_freqs.freq_pll);
			return;
		}
	}
	panic("%s: unrecognized platform", __func__);
}

#include "ath_arbus.h"
#if NATH_ARBUS > 0
#include <ah_soc.h>

const struct ar531x_boarddata *
atheros_get_board_info(void)
{
	const struct atheros_boardsw * const boardsw = my_chip->ac_boardsw;
	if (boardsw == NULL)
		return NULL;
	return (*boardsw->absw_get_board_info)();
}

/*
 * Locate board and radio configuration data in flash.
 */
int
atheros_get_board_config(struct ar531x_config *config)
{
	const struct atheros_boardsw * const boardsw = my_chip->ac_boardsw;
	if (boardsw == NULL)
		return ENOENT;

	config->board = (*boardsw->absw_get_board_info)();
	if (config->board == NULL)
		return ENOENT;

	config->radio = (*boardsw->absw_get_radio_info)();
	if (config->radio == NULL)
		return ENOENT;		/* XXX distinct code */

	return 0;
}
#endif /* NATH_ARBUS > 0 */
