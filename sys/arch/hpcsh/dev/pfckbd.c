/*	$NetBSD: pfckbd.c,v 1.7 2002/02/28 01:56:59 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include "debug_hpcsh.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>

#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/hpc/hpckbdvar.h>

#include <sh3/pfcreg.h>

#include <hpcsh/dev/pfckbdvar.h>

#ifdef	PFCKBD_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	pfckbd_debug
#endif
#include <machine/debug.h>

STATIC int pfckbd_match(struct device *, struct cfdata *, void *);
STATIC void pfckbd_attach(struct device *, struct device *, void *);
STATIC void (*pfckbd_callout_lookup(void))(void *);
STATIC void pfckbd_callout_unknown(void *);
STATIC void pfckbd_callout_hp(void *);
STATIC void pfckbd_callout_hitachi(void *);

STATIC struct pfckbd_core {
	int pc_attached;
	int pc_enabled;
	struct callout pc_soft_ch;
	struct hpckbd_ic_if pc_if;
	struct hpckbd_if *pc_hpckbd;
	u_int16_t pc_column[8];
	void (*pc_callout)(void *);
} pfckbd_core;

/* callout function table. this function is platfrom specific. */
STATIC const struct {
	platid_mask_t *platform;
	void (*func)(void *);
} pfckbd_calloutfunc_table[] = {
	{ &platid_mask_MACH_HP		, pfckbd_callout_hp },
	{ &platid_mask_MACH_HITACHI	, pfckbd_callout_hitachi }
};

struct cfattach pfckbd_ca = {
	sizeof(struct device), pfckbd_match, pfckbd_attach
};

STATIC int pfckbd_poll(void *);
STATIC void pfckbd_ifsetup(struct pfckbd_core *);
STATIC int pfckbd_input_establish(void *, struct hpckbd_if *);
STATIC void pfckbd_input(struct hpckbd_if *, u_int16_t *, u_int16_t, int);

/*
 * Matrix scan keyboard connected to SH7709, SH7709A PFC module.
 * currently, HP Jornada 680/690, HITACHI PERSONA HPW-50PAD only.
 */
void
pfckbd_cnattach()
{
	struct pfckbd_core *pc = &pfckbd_core;
	
	if ((cpu_product != CPU_PRODUCT_7709) &&
	    (cpu_product != CPU_PRODUCT_7709A))
		return;

	/* initialize interface */
	pfckbd_ifsetup(pc);

	/* attach console */
	hpckbd_cnattach(&pc->pc_if);
}

int
pfckbd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if ((cpu_product != CPU_PRODUCT_7709) &&
	    (cpu_product != CPU_PRODUCT_7709A))
		return (0);

	return (!pfckbd_core.pc_attached);
}

void
pfckbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpckbd_attach_args haa;
	
	printf("\n");

	/* pfckbd is singleton. no more attach */
	pfckbd_core.pc_attached = 1;
	pfckbd_ifsetup(&pfckbd_core);

	/* attach hpckbd */
	haa.haa_ic = &pfckbd_core.pc_if; /* tell the hpckbd to my interface */
	config_found(self, &haa, hpckbd_print);

	/* install callout handler */
	callout_init(&pfckbd_core.pc_soft_ch);
	callout_reset(&pfckbd_core.pc_soft_ch, 1, pfckbd_core.pc_callout,
	    &pfckbd_core);
}

int
pfckbd_input_establish(void *ic, struct hpckbd_if *kbdif)
{
	struct pfckbd_core *pc = ic;

	/* save hpckbd interface */
	pc->pc_hpckbd = kbdif;
	/* ok to transact hpckbd */
	pc->pc_enabled = 1;

	return 0;
}

int
pfckbd_poll(void *arg)
{
	struct pfckbd_core *pc = arg;

	if (pc->pc_enabled)
		(*pc->pc_callout)(arg);

	return 0;
}

void
pfckbd_ifsetup(struct pfckbd_core *pc)
{
	int i;

	pc->pc_if.hii_ctx = pc;
	pc->pc_if.hii_establish	= pfckbd_input_establish;
	pc->pc_if.hii_poll = pfckbd_poll;
	for (i = 0; i < 8; i++)
		pc->pc_column[i] = 0xdfff;

	/* select PFC access method */
	pc->pc_callout = pfckbd_callout_lookup();
}

void
pfckbd_input(struct hpckbd_if *hpckbd, u_int16_t *buf, u_int16_t data,
    int column)
{
	int row, type, val;
	u_int16_t edge, mask;

	if ((edge = (data ^ buf[column]))) {
		buf[column] = data;

		for (row = 0, mask = 1; row < 16; row++, mask <<= 1) {
			if (mask & edge) {
				type = mask & data ? 0 : 1;
				val = row * 8 + column;
				DPRINTF("(%2d, %2d) %d \n",
				    row, column, type);
				hpckbd_input(hpckbd, type, val);
			}
		}
	}
}

/*
 * Platform dependent routines.
 */

/* Look up appropriate callback handler */
void (*pfckbd_callout_lookup())(void *)
{
	int i, n = sizeof(pfckbd_calloutfunc_table) /
	    sizeof(pfckbd_calloutfunc_table[0]);
	
	for (i = 0; i < n; i++)
		if (platid_match(&platid,
		    pfckbd_calloutfunc_table[i].platform))
			return (pfckbd_calloutfunc_table[i].func);

	return (pfckbd_callout_unknown);
}

/* Placeholder for unknown platform */
void
pfckbd_callout_unknown(void *arg)
{

	printf("%s: unknown keyboard switch\n", __FUNCTION__);
}

/* HP Jornada680/690, HP620LX */
void
pfckbd_callout_hp(void *arg)
{
	static const struct {
		u_int8_t d, e;
	} scan[] = {
		{ 0xfd, 0xff },
		{ 0xdf, 0xff },
		{ 0xff, 0xfd },
		{ 0xff, 0xbf },
		{ 0xff, 0x7f },
		{ 0xff, 0xf7 },
		{ 0xff, 0xfe },
		{ 0x7f, 0xff },
	};
	struct pfckbd_core *pc = arg;
	int column;
	u_int16_t data;

	if (!pc->pc_enabled)
		goto reinstall;

	for (column = 0; column < 8; column++) {
		_reg_write_1(SH7709_PDDR, scan[column].d);
		_reg_write_1(SH7709_PEDR, scan[column].e);
		delay(50);
		data = _reg_read_1(SH7709_PFDR) |
		    (_reg_read_1(SH7709_PCDR) << 8);

		pfckbd_input(pc->pc_hpckbd, pc->pc_column, data, column);
	}

	_reg_write_1(SH7709_PDDR, 0xff);
	_reg_write_1(SH7709_PEDR, 0xff);
	data = _reg_read_1(SH7709_PGDR) | (_reg_read_1(SH7709_PHDR) << 8);

 reinstall:
	callout_reset(&pc->pc_soft_ch, 1, pfckbd_callout_hp, pc);
}

/* HITACH PERSONA (HPW-50PAD) */
void
pfckbd_callout_hitachi(void *arg)
{
	static const struct {
		u_int8_t d, e;
	} scan[] = {
		{ 0xf5, 0xff },
		{ 0xd7, 0xff },
		{ 0xf7, 0xfd },
		{ 0xf7, 0xbf },
		{ 0xf7, 0x7f },
		{ 0xf7, 0xf7 },
		{ 0xf7, 0xfe },
		{ 0x77, 0xff },
	};
	struct pfckbd_core *pc = arg;
	u_int16_t data;
	int column;

	if (!pc->pc_enabled)
		goto reinstall;

	for (column = 0; column < 8; column++) {
		_reg_write_1(SH7709_PCDR, ~(1 << column));
		delay(50);
		data = ((_reg_read_1(SH7709_PFDR) & 0xfe) |
		    (_reg_read_1(SH7709_PCDR) & 0x01)) << 8;
		_reg_write_1(SH7709_PCDR, 0xff);
		_reg_write_1(SH7709_PDDR, scan[column].d);
		_reg_write_1(SH7709_PEDR, scan[column].e);
		delay(50);
		data |= (_reg_read_1(SH7709_PFDR) & 0xfe) |
		    (_reg_read_1(SH7709_PCDR) & 0x01);
		_reg_write_1(SH7709_PDDR, 0xf7);
		_reg_write_1(SH7709_PEDR, 0xff);

		pfckbd_input(pc->pc_hpckbd, pc->pc_column, data, column);
	}

	_reg_write_1(SH7709_PDDR, 0xf7);
	_reg_write_1(SH7709_PEDR, 0xff);
	data = _reg_read_1(SH7709_PGDR) | (_reg_read_1(SH7709_PHDR) << 8);

 reinstall:
	callout_reset(&pc->pc_soft_ch, 1, pfckbd_callout_hitachi, pc);
}
