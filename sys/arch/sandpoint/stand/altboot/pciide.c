/* $NetBSD: pciide.c,v 1.7 2011/04/25 18:30:18 phx Exp $ */

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

static int cmdidefix(struct dkdev_ata *);

static uint32_t pciiobase = PCI_XIOBASE;

struct myops {
	int (*chipfix)(struct dkdev_ata *);
	int (*presense)(struct dkdev_ata *, int);
};
static struct myops defaultops = { NULL, NULL };
static struct myops cmdideops = { cmdidefix, NULL };
static struct myops *myops;

int
pciide_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1095, 0x0680): /* SiI 0680 IDE */
		myops = &cmdideops;
		return 1;
	case PCI_DEVICE(0x1283, 0x8211): /* ITE 8211 IDE */
	case PCI_DEVICE(0x1106, 0x1571): /* VIA 82C586 IDE */
	case PCI_DEVICE(0x1106, 0x3164): /* VIA VT6410 */
	case PCI_DEVICE(0x10ad, 0x0105): /* Symphony Labs 82C105 IDE */
	case PCI_DEVICE(0x10b8, 0x5229): /* ALi IDE */
	case PCI_DEVICE(0x1191, 0x0008): /* ACARD ATP865 */
	case PCI_DEVICE(0x1191, 0x0009): /* ACARD ATP865A */
		myops = &defaultops;
		return 1;
	}
	return 0;
}

void *
pciide_init(unsigned tag, void *data)
{
	int native, n;
	unsigned val;
	struct dkdev_ata *l;

	l = alloc(sizeof(struct dkdev_ata));
	memset(l, 0, sizeof(struct dkdev_ata));
	l->iobuf = allocaligned(512, 16);
	l->tag = tag;

	val = pcicfgread(tag, PCI_CLASS_REG);
	native = PCI_CLASS(val) != PCI_CLASS_IDE ||
	    (PCI_INTERFACE(val) & 05) != 0;
	if (native) {
		/* native, use BAR 01234 */
		l->bar[0] = pciiobase + (pcicfgread(tag, 0x10) &~ 01);
		l->bar[1] = pciiobase + (pcicfgread(tag, 0x14) &~ 01);
		l->bar[2] = pciiobase + (pcicfgread(tag, 0x18) &~ 01);
		l->bar[3] = pciiobase + (pcicfgread(tag, 0x1c) &~ 01);
		l->bar[4] = pciiobase + (pcicfgread(tag, 0x20) &~ 01);
		l->chan[0].cmd = l->bar[0];
		l->chan[0].ctl = l->chan[0].alt = l->bar[1] | 02;
		l->chan[0].dma = l->bar[4] + 0x0;
		l->chan[1].cmd = l->bar[2];
		l->chan[1].ctl = l->chan[1].alt = l->bar[3] | 02;
		l->chan[1].dma = l->bar[4] + 0x8;
	}
	else {
		/* legacy */
		l->bar[0]= pciiobase + 0x1f0;
		l->bar[1]= pciiobase + 0x3f4;
		l->bar[2]= pciiobase + 0x170;
		l->bar[3]= pciiobase + 0x374;
		l->chan[0].cmd = l->bar[0];
		l->chan[0].ctl = l->chan[0].alt = l->bar[1] | 02;
		l->chan[0].dma = 0;
		l->chan[1].cmd = l->bar[2];
		l->chan[1].ctl = l->chan[1].alt = l->bar[3] | 02;
		l->chan[1].dma = 0;
	}

	for (n = 0; n < 2; n++) {
		if (myops->presense && (*myops->presense)(l, n) == 0)
			l->presense[n] = 0; /* found not exist */
		else {
			/* check to see whether soft reset works */
			l->presense[n] = perform_atareset(l, n);
		}
		if (l->presense[n])
			printf("channel %d present\n", n);
	}

	/* make sure to have PIO0 */
	if (myops->chipfix)
		(*myops->chipfix)(l);

	return l;
}

static int
cmdidefix(struct dkdev_ata *l)
{
	int v;

	v = pcicfgread(l->tag, 0x80);
	pcicfgwrite(l->tag, 0x80, (v & ~0xff) | 0x01);
	v = pcicfgread(l->tag, 0x84);
	pcicfgwrite(l->tag, 0x84, (v & ~0xff) | 0x01);
	v = pcicfgread(l->tag, 0xa4);
	pcicfgwrite(l->tag, 0xa4, (v & ~0xffff) | 0x328a);
	v = pcicfgread(l->tag, 0xb4);
	pcicfgwrite(l->tag, 0xb4, (v & ~0xffff) | 0x328a);

	return 1;
}
