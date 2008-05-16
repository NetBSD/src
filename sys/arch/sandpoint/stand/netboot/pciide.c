/* $NetBSD: pciide.c,v 1.5.4.1 2008/05/16 02:23:05 yamt Exp $ */

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

/*
 * cmdide
 * iteide
 * viaide
 * slide
 */

int pciide_match(unsigned, void *);
void *pciide_init(unsigned, void *);

#define PCIIDE_INTERFACE_PCI(chan)	(0x01 << (2 * (chan)))

/* native: PCI BAR0-3 */
static const struct {
	int cmd, ctl;
} pcibar[2] = {
	{ 0x10, 0x14 },
	{ 0x18, 0x1c },
};
/* legacy: ISA/PCI IO space */
static const struct {
	int cmd, ctl;
} compat[2] = {
	{ 0x1f0, 0x3f6 },
	{ 0x170, 0x376 },
};

int
pciide_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1095, 0x0680): /* SiI 0680 IDE */
	case PCI_DEVICE(0x1283, 0x8211): /* ITE 8211 IDE */
	case PCI_DEVICE(0x1106, 0x1571): /* VIA 82C586 IDE */
	case PCI_DEVICE(0x10ad, 0x0105): /* Symphony Labs 82C105 IDE */
		return 1;
	}
	return 0;
}

void *
pciide_init(unsigned tag, void *data)
{
	int ch, i;
	unsigned val;
	struct atac_softc *l;
	struct atac_channel *cp;

	l = alloc(sizeof(struct atac_softc));
	memset(l, 0, sizeof(struct atac_softc));
	val = pcicfgread(tag, PCI_CLASS_REG);
	for (ch = 0; ch < 2; ch += 1) {
		cp = &l->channel[ch];
		if (PCIIDE_INTERFACE_PCI(ch) & val) {
			cp->c_cmdbase =
			  (void *)DEVTOV(~03 & pcicfgread(tag, pcibar[ch].cmd));
			cp->c_ctlbase =
			  (void *)DEVTOV(~03 & pcicfgread(tag, pcibar[ch].ctl));
			cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
			for (i = 0; i < 8; i++)
				cp->c_cmdreg[i] = cp->c_cmdbase + i;
			cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
			cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
		}
		else {
			uint32_t pciiobase = 0xfe000000; /* !!! */
			cp->c_cmdbase =
			    (void *)DEVTOV(pciiobase + compat[ch].cmd);
			cp->c_ctlbase =
			    (void *)DEVTOV(pciiobase + compat[ch].ctl);
			cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
			for (i = 0; i < 8; i++)
				cp->c_cmdreg[i] = cp->c_cmdbase + i;
			cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
			cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
		}
		cp->compatchan = ch;
	}
	l->chvalid = 03 & (unsigned)data;
	l->tag = tag;
	return l;
}
