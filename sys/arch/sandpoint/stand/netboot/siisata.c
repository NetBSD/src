/* $NetBSD: siisata.c,v 1.7 2008/05/12 09:29:56 nisimura Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/param.h>
#include <sys/disklabel.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atareg.h>

#include <lib/libsa/stand.h>
#include "globals.h"

/*
 * - no vtophys() translation, vaddr_t == paddr_t. 
 */
#define DEVTOV(pa) 		(uint32_t)(pa)

void *siisata_init(unsigned, unsigned);

static void map3112chan(unsigned, int, struct atac_channel *);
static void map3114chan(unsigned, int, struct atac_channel *);

void *
siisata_init(unsigned tag, unsigned data)
{
	unsigned val, chvalid;
	struct atac_softc *l;

	val = pcicfgread(tag, PCI_ID_REG);
	switch (val) {
	case PCI_DEVICE(0x1095, 0x3112): /* SiI 3112 SATALink */
	case PCI_DEVICE(0x1095, 0x3512): /*     3512 SATALink */
		chvalid = 0x3;
		break;
	case PCI_DEVICE(0x1095, 0x3114): /* SiI 3114 SATALink */
		chvalid = 0xf;
		break;
	default:
		return NULL;
	}

	l = alloc(sizeof(struct atac_softc));
	memset(l, 0, sizeof(struct atac_softc));

	if ((PCI_PRODUCT(val) & 0xf) == 0x2) {
		map3112chan(tag, 0, &l->channel[0]);
		map3112chan(tag, 1, &l->channel[1]);
	}
	else {
		map3114chan(tag, 0, &l->channel[0]);
		map3114chan(tag, 1, &l->channel[1]);
		map3114chan(tag, 2, &l->channel[2]);
		map3114chan(tag, 3, &l->channel[3]);
	}
	l->chvalid = chvalid & data;
	l->tag = tag;
	return l;
}

/* BAR0-3 */
static const struct {
	int cmd, ctl;
} regstd[2] = {
	{ 0x10, 0x14 },
	{ 0x18, 0x1c },
};

static void
map3112chan(unsigned tag, int ch, struct atac_channel *cp)
{
	int i;

	cp->c_cmdbase = (void *)DEVTOV(~07 & pcicfgread(tag, regstd[ch].cmd));
	cp->c_ctlbase = (void *)DEVTOV(~03 & pcicfgread(tag, regstd[ch].ctl));
	cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
	for (i = 0; i < 8; i++)
		cp->c_cmdreg[i] = cp->c_cmdbase + i;
	cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
	cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
}

/* offset to BAR5 */
static const struct {
	int IDE_TF0, IDE_TF8;
} reg3114[4] = {
	{ 0x080, 0x091 },
	{ 0x0c0, 0x0d1 },
	{ 0x280, 0x291 },
	{ 0x2c0, 0x2d1 },
};

static void
map3114chan(unsigned tag, int ch, struct atac_channel *cp)
{
	int i;
	uint8_t *ba5;

	ba5 = (uint8_t *)DEVTOV(pcicfgread(tag, 0x24)); /* PCI_BAR5 */
	cp->c_cmdbase = ba5 + reg3114[ch].IDE_TF0;
	cp->c_ctlbase = ba5 + reg3114[ch].IDE_TF8;
	cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
	for (i = 0; i < 8; i++)
		cp->c_cmdreg[i] = cp->c_cmdbase + i;
	cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
	cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
}
