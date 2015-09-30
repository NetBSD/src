/* $NetBSD: siisata.c,v 1.6 2015/09/30 14:14:32 phx Exp $ */

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

/*
 * - no vtophys() translation, vaddr_t == paddr_t.
 */
#define CSR_READ_4(r)		in32rb(r)
#define CSR_WRITE_4(r,v)	out32rb(r,v)

static int satapresense(struct dkdev_ata *, int);

static uint32_t pciiobase = PCI_XIOBASE;

int sata_delay[4] = { 3, 3, 3, 3 };	/* drive power-up delay per channel */

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
	unsigned idreg;
	int n, nchan, retries/*waitforspinup*/;
	struct dkdev_ata *l;

	l = alloc(sizeof(struct dkdev_ata));
	memset(l, 0, sizeof(struct dkdev_ata));
	l->iobuf = allocaligned(512, 16);
	l->tag = tag;

	idreg = pcicfgread(tag, PCI_ID_REG);
	l->bar[0] = pciiobase + (pcicfgread(tag, 0x10) &~ 01);
	l->bar[1] = pciiobase + (pcicfgread(tag, 0x14) &~ 01);
	l->bar[2] = pciiobase + (pcicfgread(tag, 0x18) &~ 01);
	l->bar[3] = pciiobase + (pcicfgread(tag, 0x1c) &~ 01);
	l->bar[4] = pciiobase + (pcicfgread(tag, 0x20) &~ 01);
	l->bar[5] = pcicfgread(tag, 0x24) &~ 0x3ff;

	if ((PCI_PRODUCT(idreg) & 0xf) == 0x2) {
		/* 3112/3512 */
		l->chan[0].cmd = l->bar[0];
		l->chan[0].ctl = l->chan[0].alt = l->bar[1] | 02;
		l->chan[0].dma = l->bar[4] + 0x0;
		l->chan[1].cmd = l->bar[2];
		l->chan[1].ctl = l->chan[1].alt = l->bar[3] | 02;
		l->chan[1].dma = l->bar[4] + 0x8;
		nchan = 2;
	}
	else {
		/* 3114 - assume BA5 access is possible XXX */
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

	/* configure PIO transfer mode */
	pcicfgwrite(tag, 0x80, 0x00);
	pcicfgwrite(tag, 0x84, 0x00);

	for (n = 0; n < nchan; n++) {
		l->presense[n] = satapresense(l, n);
		if (l->presense[n] == 0) {
			/* wait some seconds to power-up the drive */
			for (retries = 0; retries < sata_delay[n]; retries++) {
				wakeup_drive(l, n);
				printf("Waiting %2d seconds for powering up "
				    "port %d.\r", sata_delay[n] - retries, n);
				delay(1000 * 1000);
				if ((l->presense[n] = satapresense(l, n)) != 0)
					break;
			}
			putchar('\n');
			if (l->presense[n] == 0) {
				DPRINTF(("port %d not present\n", n));
				continue;
			}
		}
		if (atachkpwr(l, n) != ATA_PWR_ACTIVE) {
			/* drive is probably sleeping, wake it up */
			for (retries = 0; retries < 20; retries++) {
				wakeup_drive(l, n);
				DPRINTF(("port %d spinning up...\n", n));
				delay(1000 * 1000);
				l->presense[n] = perform_atareset(l, n);
				if (atachkpwr(l, n) == ATA_PWR_ACTIVE)
					break;
			}
		} else {
			/* check to see whether soft reset works */
			DPRINTF(("port %d active\n", n));
			for (retries = 0; retries < 20; retries++) {
				l->presense[n] = perform_atareset(l, n);
				if (l->presense[n] != 0)
					break;
				wakeup_drive(l, n);
				DPRINTF(("port %d cold-starting...\n", n));
				delay(1000 * 1000);
			}
		}

		if (l->presense[n])
			printf("port %d present\n", n);
	}
	return l;
}

static int
satapresense(struct dkdev_ata *l, int n)
{
#define VND_CH(n) (((n&02)<<8)+((n&01)<<7))
#define VND_SC(n) (0x100+VND_CH(n))
#define VND_SS(n) (0x104+VND_CH(n))

	uint32_t sc = l->bar[5] + VND_SC(n);
	uint32_t ss = l->bar[5] + VND_SS(n);
	unsigned val;

	val = (00 << 4) | (03 << 8);	/* any speed, no pwrmgt */
	CSR_WRITE_4(sc, val | 01);	/* perform init */
	delay(50 * 1000);
	CSR_WRITE_4(sc, val);
	delay(50 * 1000);	
	val = CSR_READ_4(ss);		/* has completed */
	return ((val & 03) == 03);	/* active drive found */
}
