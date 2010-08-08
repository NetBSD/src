/* $NetBSD: siisata.c,v 1.12 2010/08/08 11:54:46 phx Exp $ */

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

#include <lib/libsa/stand.h>

#include "globals.h"

static uint32_t pciiobase = PCI_XIOBASE;

int siisata_match(unsigned, void *);
void *siisata_init(unsigned, void *);

int
siisata_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1095, 0x3112): /* SiI 3112 SATALink */
	case PCI_DEVICE(0x1095, 0x3512): /*     3512 SATALink */
	case PCI_DEVICE(0x1095, 0x3114): /* SiI 3114 SATALink */
		return 1;
	}
	return 0;
}

void *
siisata_init(unsigned tag, void *data)
{
	unsigned val, cls;
	int nchan, n;
	struct dkdev_ata *l;
	static int ba5bccregs[] = { 0x40, 0x44, 0x240, 0x244 };

	l = alloc(sizeof(struct dkdev_ata));
	memset(l, 0, sizeof(struct dkdev_ata));
	l->iobuf = allocaligned(512, 16);
	l->tag = tag;

	l->bar[0] = pciiobase + (pcicfgread(tag, 0x10) &~ 01);
	l->bar[1] = pciiobase + (pcicfgread(tag, 0x14) &~ 01);
	l->bar[2] = pciiobase + (pcicfgread(tag, 0x18) &~ 01);
	l->bar[3] = pciiobase + (pcicfgread(tag, 0x1c) &~ 01);
	l->bar[4] = pciiobase + (pcicfgread(tag, 0x20) &~ 01);
	l->bar[5] = pcicfgread(tag, 0x24) &~ 0x3ff;

	val = pcicfgread(tag, PCI_ID_REG);
	if ((PCI_PRODUCT(val) & 0xf) == 0x2) {
		/* 3112/3512 */
		/* reset BA5 indirect access enable */
		val = pcicfgread(tag, 0x40);
		pcicfgwrite(tag, 0x40, val & ~02);
		/* reset everything, enable BA5 access */
		pcicfgwrite(tag, 0x88, 0x100f3);
		delay(50 * 1000);
		pcicfgwrite(tag, 0x88, 0x10000);
		delay(50 * 1000);		
		l->chan[0].cmd = l->bar[0];
		l->chan[0].ctl = l->chan[0].alt = l->bar[1] | 02;
		l->chan[0].dma = l->bar[4] + 0x0;
		l->chan[1].cmd = l->bar[2];
		l->chan[1].ctl = l->chan[1].alt = l->bar[3] | 02;
		l->chan[1].dma = l->bar[4] + 0x8;
		nchan = 2;
	}
	else {
		/* 3114 - assume BA5 access is possible */
		l->chan[0].cmd = l->bar[5] + 0x080;
		l->chan[0].ctl = l->chan[0].alt = (l->bar[5] + 0x088) | 02;
		l->chan[1].cmd = l->bar[5] + 0x0c0;
		l->chan[1].ctl = l->chan[1].alt = (l->bar[5] + 0x0c8) | 02;
		l->chan[2].cmd = l->bar[5] + 0x280;
		l->chan[2].ctl = l->chan[2].alt = (l->bar[5] + 0x288) | 02;
		l->chan[3].cmd = l->bar[5] + 0x2c0;
		l->chan[3].ctl = l->chan[3].alt = (l->bar[5] + 0x2c8) | 02;
		nchan = 4;
	}

	/* fixup cache line */
	cls = (pcicfgread(tag, 0xc) & 0xff) << 2;
	if (cls > 224) {
		cls = (pcicfgread(tag, 0xc) & 0xff) | (224>>2);
		pcicfgwrite(tag, 0xc, cls);
		cls = 224;
	} else if (cls < 32)
		cls = 32;
	cls = (cls + 31) / 32;
	for (n = 0; n < nchan; n++) {
		val = in32rb(l->bar[5] + ba5bccregs[n]);
		if ((val & 0x7) < cls)
			out32rb(l->bar[5] + ba5bccregs[n], (val & 0x07) | cls);
	}

	/* detect presence */
	for (n = 0; n < nchan; n++) {
		l->presense[n] = satapresense(l, n);
		if (l->presense[n])
			printf("port %d device present\n", n);
	}

	out32rb(l->bar[5] + 0xb4, 01);
	out32rb(l->bar[5] + 0xf4, 01);
	out32rb(l->bar[5] + 0xa4, 0x328a);
	out32rb(l->bar[5] + 0xe4, 0x328a);

	for (val=0; val<0x100; val+=4) {
		if ((val & 0xf) == 0)
			printf("0x%02x:",val);
		printf(" 0x%08x",pcicfgread(tag,val));
		if ((val & 0xf) == 0xc)
			printf("\n");
	}
	return l;
}
