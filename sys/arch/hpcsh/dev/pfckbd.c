/*	$NetBSD: pfckbd.c,v 1.29.6.1 2017/12/03 11:36:15 jdolecek Exp $	*/

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

/*
 * Matrix scan keyboard connected to SH7709, SH7709A PFC module.
 * currently, HP Jornada 680/690, HITACHI PERSONA HPW-50PAD only.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pfckbd.c,v 1.29.6.1 2017/12/03 11:36:15 jdolecek Exp $");

#include "debug_hpcsh.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/bus.h>

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

static struct pfckbd_core {
	int pc_attached;
	int pc_enabled;
	struct callout pc_soft_ch;
	struct hpckbd_ic_if pc_if;
	struct hpckbd_if *pc_hpckbd;
	uint16_t pc_column[8];
	void (*pc_callout)(struct pfckbd_core *);
} pfckbd_core;

static int pfckbd_match(device_t, cfdata_t, void *);
static void pfckbd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pfckbd, 0,
    pfckbd_match, pfckbd_attach, NULL, NULL);

static void pfckbd_ifsetup(struct pfckbd_core *);

/* callbacks for hpckbd */
static int pfckbd_input_establish(void *, struct hpckbd_if *);
static int pfckbd_poll(void *);

static void pfckbd_input(struct pfckbd_core *, int, uint16_t);

static void (*pfckbd_callout_lookup(void))(struct pfckbd_core *);
static void pfckbd_callout(void *);
static void pfckbd_callout_hp(struct pfckbd_core *);
static void pfckbd_callout_hitachi(struct pfckbd_core *);
void pfckbd_poll_hitachi_power(void);


/* callout function table. this function is platfrom specific. */
static const struct {
	platid_mask_t *platform;
	void (*func)(struct pfckbd_core *);
} pfckbd_calloutfunc_table[] = {
	{ &platid_mask_MACH_HP		, pfckbd_callout_hp },
	{ &platid_mask_MACH_HITACHI	, pfckbd_callout_hitachi }
};


void
pfckbd_cnattach(void)
{
	struct pfckbd_core *pc = &pfckbd_core;

	if ((cpu_product != CPU_PRODUCT_7709)
	    && (cpu_product != CPU_PRODUCT_7709A))
		return;

	/* initialize interface */
	pfckbd_ifsetup(pc);

	/* attach descendants */
	hpckbd_cnattach(&pc->pc_if);
}

static int
pfckbd_match(device_t parent, cfdata_t cf, void *aux)
{

	if ((cpu_product != CPU_PRODUCT_7709)
	    && (cpu_product != CPU_PRODUCT_7709A))
		return 0;

	return !pfckbd_core.pc_attached; /* attach only once */
}

static void
pfckbd_attach(device_t parent, device_t self, void *aux)
{
	struct hpckbd_attach_args haa;

	aprint_naive("\n");
	aprint_normal("\n");

	pfckbd_core.pc_attached = 1;

	pfckbd_ifsetup(&pfckbd_core);

	/* attach hpckbd */
	haa.haa_ic = &pfckbd_core.pc_if; /* tell hpckbd our interface */
	config_found(self, &haa, hpckbd_print);

	/* install callout handler */
	if (pfckbd_core.pc_callout != NULL) {
		callout_init(&pfckbd_core.pc_soft_ch, 0);
		callout_reset(&pfckbd_core.pc_soft_ch, 1,
			      pfckbd_callout, &pfckbd_core);
	}
	else
		aprint_error_dev(self, "unsupported platform\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}

static void
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


/*
 * Callback for hpckbd_initif
 */
static int
pfckbd_input_establish(void *ic, struct hpckbd_if *kbdif)
{
	struct pfckbd_core *pc = ic;

	pc->pc_hpckbd = kbdif;	/* save hpckbd interface */
	pc->pc_enabled = 1;	/* ok to talk to hpckbd */

	return 0;
}

/*
 * Callback for hpckbd_cngetc
 */
static int
pfckbd_poll(void *ic)
{
	struct pfckbd_core *pc = ic;

	if (pc->pc_enabled && pc->pc_callout != NULL)
		(*pc->pc_callout)(pc);

	return 0;
}

static void
pfckbd_callout(void *arg)
{
	struct pfckbd_core *pc = arg;

	(*pc->pc_callout)(pc);
	callout_schedule(&pc->pc_soft_ch, 1);
}


/*
 * Called by platform specific scan routines to report key events to hpckbd
 */
static void
pfckbd_input(struct pfckbd_core *pc, int column, uint16_t data)
{
	int row, type, val;
	unsigned int edge, mask;

	edge = data ^ pc->pc_column[column];
	if (edge == 0)
		return;		/* no changes in this column */

	pc->pc_column[column] = data;

	for (row = 0, mask = 1; row < 16; ++row, mask <<= 1) {
		if (mask & edge) {
			type = mask & data ? /* up */ 0 : /* down */ 1;
			DPRINTF("(%2d, %2d) %d \n", row, column, type);

			val = row * 8 + column;
			hpckbd_input(pc->pc_hpckbd, type, val);
		}
	}
}


/*
 * Platform dependent scan routines.
 */

/* Look up appropriate callback handler */
static void
(*pfckbd_callout_lookup(void))(struct pfckbd_core *)
{
	int i, n;

	n = sizeof(pfckbd_calloutfunc_table)
		/ sizeof(pfckbd_calloutfunc_table[0]);

	for (i = 0; i < n; i++)
		if (platid_match(&platid,
				 pfckbd_calloutfunc_table[i].platform))
			return pfckbd_calloutfunc_table[i].func;

	return NULL;
}

/*
 * HP Jornada680/690, HP620LX
 */
static void
pfckbd_callout_hp(struct pfckbd_core *pc)
{
#define PFCKBD_HP_PDCR_MASK 0xcc0c
#define PFCKBD_HP_PECR_MASK 0xf0cf

	/*
	 * Disable output on all lines but the n'th line in D.
	 * Pull the n'th scan line in D low.
	 */
#define PD(n)								\
	{ (uint16_t)(PFCKBD_HP_PDCR_MASK & (~(1 << (2*(n)+1)))),	\
	  (uint16_t)(PFCKBD_HP_PECR_MASK & 0xffff),			\
	  (uint8_t)~(1 << (n)),						\
	  0xff }

	/* Ditto for E */
#define PE(n)								\
	{ (uint16_t)(PFCKBD_HP_PDCR_MASK & 0xffff),			\
	  (uint16_t)(PFCKBD_HP_PECR_MASK & (~(1 << (2*(n)+1)))),	\
	  0xff,								\
	  (uint8_t)~(1 << (n)) }

	static const struct {
		uint16_t dc, ec; uint8_t d, e;
	} scan[] = {
		PD(1), PD(5), PE(1), PE(6), PE(7), PE(3), PE(0), PD(7)
	};

#undef PD
#undef PE

	uint16_t dc, ec;
	int column;
	uint16_t data;

	if (!pc->pc_enabled)
		return;

	/* bits in D/E control regs we do not touch (XXX: can they change?) */
	dc = _reg_read_2(SH7709_PDCR) & ~PFCKBD_HP_PDCR_MASK;
	ec = _reg_read_2(SH7709_PECR) & ~PFCKBD_HP_PECR_MASK;

	for (column = 0; column < 8; column++) {
		/* disable output to all lines except the one we scan */
		_reg_write_2(SH7709_PDCR, dc | scan[column].dc);
		_reg_write_2(SH7709_PECR, ec | scan[column].ec);
		delay(5);

		/* pull the scan line low */
		_reg_write_1(SH7709_PDDR, scan[column].d);
		_reg_write_1(SH7709_PEDR, scan[column].e);
		delay(50);

		/* read sense */
		data = _reg_read_1(SH7709_PFDR)
			| (_reg_read_1(SH7709_PCDR) << 8);

		pfckbd_input(pc, column, data);
	}

	/* scan no lines */
	_reg_write_1(SH7709_PDDR, 0xff);
	_reg_write_1(SH7709_PEDR, 0xff);

	/* enable all scan lines */
	_reg_write_2(SH7709_PDCR, dc | (0x5555 & PFCKBD_HP_PDCR_MASK));
	_reg_write_2(SH7709_PECR, ec | (0x5555 & PFCKBD_HP_PECR_MASK));

#if 0
	/* (ignore) extra keys/events (recorder buttons, lid, cable &c) */
	data = _reg_read_1(SH7709_PGDR) | (_reg_read_1(SH7709_PHDR) << 8);
#endif
}

/*
 * HITACH PERSONA (HPW-50PAD)
 */
static void
pfckbd_callout_hitachi(struct pfckbd_core *pc)
{
#define PFCKBD_HITACHI_PCCR_MASK 0xfff3
#define PFCKBD_HITACHI_PDCR_MASK 0x000c
#define PFCKBD_HITACHI_PECR_MASK 0x30cf

#define PFCKBD_HITACHI_PCDR_SCN_MASK 0xfd
#define PFCKBD_HITACHI_PDDR_SCN_MASK 0x02
#define PFCKBD_HITACHI_PEDR_SCN_MASK 0x4b

#define PFCKBD_HITACHI_PCDR_SNS_MASK 0x01
#define PFCKBD_HITACHI_PFDR_SNS_MASK 0xfe

	/*
	 * Disable output on all lines but the n'th line in C.
	 * Pull the n'th scan line in C low.
	 */
#define PC(n)								\
	{ (uint16_t)(PFCKBD_HITACHI_PCCR_MASK & (~(1 << (2*(n)+1)))),	\
	  (uint16_t)(PFCKBD_HITACHI_PDCR_MASK & 0xffff),		\
	  (uint16_t)(PFCKBD_HITACHI_PECR_MASK & 0xffff),		\
	  (uint8_t)(PFCKBD_HITACHI_PCDR_SCN_MASK & ~(1 << (n))),	\
	  PFCKBD_HITACHI_PDDR_SCN_MASK,					\
	  PFCKBD_HITACHI_PEDR_SCN_MASK }

	/* Ditto for D */
#define PD(n)								\
	{ (uint16_t)(PFCKBD_HITACHI_PCCR_MASK & 0xffff),		\
	  (uint16_t)(PFCKBD_HITACHI_PDCR_MASK & (~(1 << (2*(n)+1)))),	\
	  (uint16_t)(PFCKBD_HITACHI_PECR_MASK & 0xffff),		\
	  PFCKBD_HITACHI_PCDR_SCN_MASK,					\
	  (uint8_t)(PFCKBD_HITACHI_PDDR_SCN_MASK & ~(1 << (n))),	\
	  PFCKBD_HITACHI_PEDR_SCN_MASK }

	/* Ditto for E */
#define PE(n)								\
	{ (uint16_t)(PFCKBD_HITACHI_PCCR_MASK & 0xffff),		\
	  (uint16_t)(PFCKBD_HITACHI_PDCR_MASK & 0xffff),		\
	  (uint16_t)(PFCKBD_HITACHI_PECR_MASK & (~(1 << (2*(n)+1)))),	\
	  PFCKBD_HITACHI_PCDR_SCN_MASK,					\
	  PFCKBD_HITACHI_PDDR_SCN_MASK,					\
	  (uint8_t)(PFCKBD_HITACHI_PEDR_SCN_MASK & ~(1 << (n))) }

	static const struct {
		uint16_t cc, dc, ec; uint8_t c, d, e;
	} scan[] = {
		PE(6), PE(3), PE(1), PE(0), PC(7), PC(6), PC(5), PC(4),
		PC(3), PC(2), PD(1), PC(0)
	};

	uint16_t cc, dc, ec;
	uint8_t data[2], cd, dd, ed;
	int i;

	if (!pc->pc_enabled)
		return;

	/* bits in C/D/E control regs we do not touch (XXX: can they change?) */
	cc = _reg_read_2(SH7709_PCCR) & ~PFCKBD_HITACHI_PCCR_MASK;
	dc = _reg_read_2(SH7709_PDCR) & ~PFCKBD_HITACHI_PDCR_MASK;
	ec = _reg_read_2(SH7709_PECR) & ~PFCKBD_HITACHI_PECR_MASK;

	for (i = 0; i < 12; i++) {
		/* disable output to all lines except the one we scan */
		_reg_write_2(SH7709_PCCR, cc | scan[i].cc);
		_reg_write_2(SH7709_PDCR, dc | scan[i].dc);
		_reg_write_2(SH7709_PECR, ec | scan[i].ec);
		delay(5);

		cd = _reg_read_1(SH7709_PCDR) & ~PFCKBD_HITACHI_PCDR_SCN_MASK;
		dd = _reg_read_1(SH7709_PDDR) & ~PFCKBD_HITACHI_PDDR_SCN_MASK;
		ed = _reg_read_1(SH7709_PEDR) & ~PFCKBD_HITACHI_PEDR_SCN_MASK;

		/* pull the scan line low */
		_reg_write_1(SH7709_PCDR, cd | scan[i].c);
		_reg_write_1(SH7709_PDDR, dd | scan[i].d);
		_reg_write_1(SH7709_PEDR, ed | scan[i].e);
		delay(50);

		/* read sense */
		data[i & 0x1] =
		    (_reg_read_1(SH7709_PCDR) & PFCKBD_HITACHI_PCDR_SNS_MASK)
		  | (_reg_read_1(SH7709_PFDR) & PFCKBD_HITACHI_PFDR_SNS_MASK);

		if (i & 0x1)
			pfckbd_input(pc, (i >> 1), (data[0] | (data[1] << 8)));
	}

	/* enable all scan lines */
	_reg_write_2(SH7709_PCCR, cc | (0x5555 & PFCKBD_HITACHI_PCCR_MASK));
	_reg_write_2(SH7709_PDCR, dc | (0x5555 & PFCKBD_HITACHI_PDCR_MASK));
	_reg_write_2(SH7709_PECR, ec | (0x5555 & PFCKBD_HITACHI_PECR_MASK));
}

void
pfckbd_poll_hitachi_power(void)
{
	static const struct {
		uint16_t cc, dc, ec; uint8_t c, d, e;
	} poll = PD(1);

#undef PC
#undef PD
#undef PE

	uint16_t cc, dc, ec;
	uint8_t cd, dd, ed;

	/* bits in C/D/E control regs we do not touch (XXX: can they change?) */
	cc = _reg_read_2(SH7709_PCCR) & ~PFCKBD_HITACHI_PCCR_MASK;
	dc = _reg_read_2(SH7709_PDCR) & ~PFCKBD_HITACHI_PDCR_MASK;
	ec = _reg_read_2(SH7709_PECR) & ~PFCKBD_HITACHI_PECR_MASK;

	/* disable output to all lines except the one we scan */
	_reg_write_2(SH7709_PCCR, cc | poll.cc);
	_reg_write_2(SH7709_PDCR, dc | poll.dc);
	_reg_write_2(SH7709_PECR, ec | poll.ec);
	delay(5);

	cd = _reg_read_1(SH7709_PCDR) & ~PFCKBD_HITACHI_PCDR_SCN_MASK;
	dd = _reg_read_1(SH7709_PDDR) & ~PFCKBD_HITACHI_PDDR_SCN_MASK;
	ed = _reg_read_1(SH7709_PEDR) & ~PFCKBD_HITACHI_PEDR_SCN_MASK;

	/* pull the scan line low */
	_reg_write_1(SH7709_PCDR, cd | poll.c);
	_reg_write_1(SH7709_PDDR, dd | poll.d);
	_reg_write_1(SH7709_PEDR, ed | poll.e);
	delay(50);

	/* poll POWER On */
	while (_reg_read_1(SH7709_PCDR) & PFCKBD_HITACHI_PCDR_SNS_MASK & 0x01);

	/* enable all scan lines */
	_reg_write_2(SH7709_PCCR, cc | (0x5555 & PFCKBD_HITACHI_PCCR_MASK));
	_reg_write_2(SH7709_PDCR, dc | (0x5555 & PFCKBD_HITACHI_PDCR_MASK));
	_reg_write_2(SH7709_PECR, ec | (0x5555 & PFCKBD_HITACHI_PECR_MASK));
}
