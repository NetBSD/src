/* $NetBSD: pciide.c,v 1.9.2.1 2012/04/17 00:06:50 yamt Exp $ */

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

static void cmdidefix(struct dkdev_ata *);
static void apoidefix(struct dkdev_ata *);
static void iteidefix(struct dkdev_ata *);

static uint32_t pciiobase = PCI_XIOBASE;

struct myops {
	void (*chipfix)(struct dkdev_ata *);
	int (*presense)(struct dkdev_ata *, int);
};
static struct myops defaultops = { NULL, NULL };
static struct myops cmdideops = { cmdidefix, NULL };
static struct myops apoideops = { apoidefix, NULL };
static struct myops iteideops = { iteidefix, NULL };
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
	case PCI_DEVICE(0x1106, 0x0571): /* VIA 82C586A/B/686A/B IDE */
	case PCI_DEVICE(0x1106, 0x1571): /* VIA 82C586 IDE */
	case PCI_DEVICE(0x1106, 0x3164): /* VIA VT6410 RAID IDE */
		myops = &apoideops;
		return 1;
	case PCI_DEVICE(0x1283, 0x8211): /* ITE 8211 IDE */
		myops = &iteideops;
		return 1;
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
	static int cntrl = 0;
	int native, n, retries;
	unsigned val;
	struct dkdev_ata *l;

	l = alloc(sizeof(struct dkdev_ata));
	memset(l, 0, sizeof(struct dkdev_ata));
	l->iobuf = allocaligned(512, 16);
	l->tag = tag;

	/* chipset specific fixes */
	if (myops->chipfix)
		(*myops->chipfix)(l);

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
		if (myops->presense != NULL && (*myops->presense)(l, n) == 0) {
			DPRINTF(("channel %d not present\n", n));
			l->presense[n] = 0;
			continue;
		} else if (get_drive_config(cntrl * 2 + n) == 0) {
			DPRINTF(("channel %d disabled by config\n", n));
			l->presense[n] = 0;
			continue;
		}

		if (atachkpwr(l, n) != ATA_PWR_ACTIVE) {
			/* drive is probably sleeping, wake it up */
			for (retries = 0; retries < 10; retries++) {
				wakeup_drive(l, n);
				DPRINTF(("channel %d spinning up...\n", n));
				delay(1000 * 1000);
				l->presense[n] = perform_atareset(l, n);
				if (atachkpwr(l, n) == ATA_PWR_ACTIVE)
					break;
			}
		} else {
			/* check to see whether soft reset works */
			DPRINTF(("channel %d active\n", n));
			l->presense[n] = perform_atareset(l, n);
		}

		if (l->presense[n])
			printf("channel %d present\n", n);
	}

	cntrl++;	/* increment controller number for next call */
	return l;
}

static void
cmdidefix(struct dkdev_ata *l)
{
	unsigned v;

	v = pcicfgread(l->tag, 0x80);
	pcicfgwrite(l->tag, 0x80, (v & ~0xff) | 0x01);
	v = pcicfgread(l->tag, 0x84);
	pcicfgwrite(l->tag, 0x84, (v & ~0xff) | 0x01);
	v = pcicfgread(l->tag, 0xa4);
	pcicfgwrite(l->tag, 0xa4, (v & ~0xffff) | 0x328a);
	v = pcicfgread(l->tag, 0xb4);
	pcicfgwrite(l->tag, 0xb4, (v & ~0xffff) | 0x328a);
}

static void
apoidefix(struct dkdev_ata *l)
{
	unsigned v;

	/* enable primary and secondary channel */
	v = pcicfgread(l->tag, 0x40) & ~0x03;
	pcicfgwrite(l->tag, 0x40, v | 0x03);
}

static void
iteidefix(struct dkdev_ata *l)
{
	unsigned v;

	/* set PCI mode and 66Mhz reference clock, disable IT8212 RAID */
	v = pcicfgread(l->tag, 0x50);
	pcicfgwrite(l->tag, 0x50, v & ~0x83);

	/* i/o configuration, enable channels, cables, IORDY */
	v = pcicfgread(l->tag, 0x40);
	pcicfgwrite(l->tag, 0x40, (v & ~0xffffff) | 0x36a0f3);
}
