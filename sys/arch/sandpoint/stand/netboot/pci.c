/* $NetBSD: pci.c,v 1.7.14.1 2009/05/13 17:18:16 jym Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

/*
 * "Map B" layout
 *
 * practice direct mode configuration scheme with CONFIG_ADDR
 * (0xfec0'0000) and CONFIG_DATA (0xfee0'0000).
 */
#define PCI_MEMBASE	0x80000000
#define PCI_MEMLIMIT	0xfbffffff	/* EUMB is next to this */
#define PCI_IOBASE	0x00001000	/* reserves room for via 686B */
#define PCI_IOLIMIT	0x000fffff
#define CONFIG_ADDR	0xfec00000
#define CONFIG_DATA	0xfee00000

#define MAXNDEVS 32

#include "globals.h"

static unsigned cfgread(int, int, int, int);
static void cfgwrite(int, int, int, int, unsigned);
static void _buswalk(int,
		int (*)(int, int, int, unsigned long), unsigned long);
static int _pcilookup(int,
		int (*)(int, int, int, unsigned long), unsigned long,
		unsigned [][2], int, int);
static int deviceinit(int, int, int, unsigned long);
static void memassign(int, int, int);
static int devmatch(int, int, int, unsigned long);
static int clsmatch(int, int, int, unsigned long);

unsigned memstart, memlimit;
unsigned iostart, iolimit;
unsigned maxbus;

void
pcisetup(void)
{

	memstart = PCI_MEMBASE;
	memlimit = PCI_MEMLIMIT;
	iostart =  PCI_IOBASE;
	iolimit =  PCI_IOLIMIT;
	maxbus = 0;

	(void)_buswalk(0, deviceinit, 0UL);
}

int
pcifinddev(unsigned vend, unsigned prod, unsigned *tag)
{
	unsigned pciid, target[1][2];

	pciid = PCI_DEVICE(vend, prod);
	if (_pcilookup(0, devmatch, pciid, target, 0, 1)) {
		*tag = target[0][1];
		return 0;
	}
	*tag = ~0;
	return -1;
} 

int
pcilookup(type, list, max)
	unsigned type;
	unsigned list[][2];
	int max;
{

	return _pcilookup(0, clsmatch, type, list, 0, max);
}

unsigned
pcimaketag(int b, int d, int f)
{

	return (1U << 31) | (b << 16) | (d << 11) | (f << 8);
}

void
pcidecomposetag(unsigned tag, int *b, int *d, int *f)
{

	if (b != NULL)
		*b = (tag >> 16) & 0xff;
	if (d != NULL)
		*d = (tag >> 11) & 0x1f;
	if (f != NULL)
		*f = (tag >> 8) & 0x7;
	return;
}

unsigned
pcicfgread(unsigned tag, int off)
{
	unsigned cfg;
	
	cfg = tag | (off &~ 03);
	iohtole32(CONFIG_ADDR, cfg);
	return iole32toh(CONFIG_DATA);
}

void
pcicfgwrite(unsigned tag, int off, unsigned val)
{
	unsigned cfg;

	cfg = tag | (off &~ 03);
	iohtole32(CONFIG_ADDR, cfg);
	iohtole32(CONFIG_DATA, val);
}

static unsigned
cfgread(int b, int d, int f, int off)
{
	unsigned cfg;
	
	off &= ~03;
	cfg = (1U << 31) | (b << 16) | (d << 11) | (f << 8) | off | 0;
	iohtole32(CONFIG_ADDR, cfg);
	return iole32toh(CONFIG_DATA);
}

static void
cfgwrite(int b, int d, int f, int off, unsigned val)
{
	unsigned cfg;

	off &= ~03;
	cfg = (1U << 31) | (b << 16) | (d << 11) | (f << 8) | off | 0;
	iohtole32(CONFIG_ADDR, cfg);
	iohtole32(CONFIG_DATA, val);
}

static void
_buswalk(int bus, int (*proc)(int, int, int, unsigned long), unsigned long data)
{
	int device, function, nfunctions;
	unsigned pciid, bhlcr;

	for (device = 0; device < MAXNDEVS; device++) {
		pciid = cfgread(bus, device, 0, PCI_ID_REG);
		if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
			continue;
		if (PCI_VENDOR(pciid) == 0)
			continue;
		bhlcr = cfgread(bus, device, 0, PCI_BHLC_REG);
		nfunctions = (PCI_HDRTYPE_MULTIFN(bhlcr)) ? 8 : 1;
		for (function = 0; function < nfunctions; function++) {
			pciid = cfgread(bus, device, function, PCI_ID_REG);
			if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
				continue;
			if (PCI_VENDOR(pciid) == 0)
				continue;

			if ((*proc)(bus, device, function, data) != 0)
				goto out; /* early exit */
		}
	}
  out:;
}

static int
deviceinit(int bus, int dev, int func, unsigned long data)
{
	unsigned val;

	/* 0x00 */
	printf("%02d:%02d:%02d:", bus, dev, func);
	val = cfgread(bus, dev, func, 0x00);
	printf(" chip %04x.%04x", val & 0xffff, val>>16);
	val = cfgread(bus, dev, func, 0x2c);
	printf(" card %04x.%04x", val & 0xffff, val>>16);
	val = cfgread(bus, dev, func, 0x08);
	printf(" rev %02x class %02x.%02x.%02x",
		val & 0xff, (val>>24), (val>>16) & 0xff, (val>>8) & 0xff);
	val = cfgread(bus, dev, func, 0x0c);
	printf(" hdr %02x\n", (val>>16) & 0xff);

	/* 0x04 */
	val = cfgread(bus, dev, func, 0x04);
	val |= 0xffff0107; /* enable IO,MEM,MASTER,SERR */
	cfgwrite(bus, dev, func, 0x04, val);

	/* 0x0c */
	val = 0x80 << 8 | 0x08 /* 32B cache line */;
	cfgwrite(bus, dev, func, 0x0c, val);

#if 1
/* skip IDE controller BAR assignment */
val = cfgread(bus, dev, func, PCI_CLASS_REG);
if ((val >> 16) == PCI_CLASS_IDE)
	return 0;
#endif
	memassign(bus, dev, func);

	/* descending toward PCI-PCI bridge */
	if ((cfgread(bus, dev, func, 0x08) >> 16) == PCI_CLASS_PPB) {
		unsigned new;

		/* 0x18 */
		new = (maxbus += 1);
		val = (0xff << 16) | (new << 8) | bus;
		cfgwrite(bus, dev, func, 0x18, val);

		/* 0x1c and 0x30 */
		val = (iostart + (0xfff)) & ~0xfff; /* 4KB boundary */
		iostart = val;
		val = 0xffff0000 | (iolimit & 0xf000) | (val & 0xf000) >> 8;
		cfgwrite(bus, dev, func, 0x1c, val);
		val = (iolimit & 0xffff0000) | (val & 0xffff0000) >> 16;
		cfgwrite(bus, dev, func, 0x30, val);

		/* 0x20 */
		val = (memstart + 0xfffff) &~ 0xfffff; /* 1MB boundary */
		memstart = val;
		val = (memlimit & 0xffff0000) | (val & 0xffff0000) >> 16;
		cfgwrite(bus, dev, func, 0x20, val);

		/* redo 0x04 */
		val = cfgread(bus, dev, func, 0x04);
		val |= 0xffff0107;
		cfgwrite(bus, dev, func, 0x04, val);

		_buswalk(new, deviceinit, data);

		/* adjust 0x18 */
		val = cfgread(bus, dev, func, 0x18);
		val = (maxbus << 16) | (val & 0xffff);
		cfgwrite(bus, dev, func, 0x18, val);
	}
	return 0;
}

static void
memassign(int bus, int dev, int func)
{
	unsigned val, maxbar, mapr, req, mapbase, size;

	val = cfgread(bus, dev, func, 0x0c);
	switch (PCI_HDRTYPE_TYPE(val)) {
	case 0:
		maxbar = 0x10 + 6 * 4; break;
	case 1:
		maxbar = 0x10 + 2 * 4; break;
	default:
		maxbar = 0x10 + 1 * 4; break;
	}
	for (mapr = 0x10; mapr < maxbar; mapr += 4) {
		cfgwrite(bus, dev, func, mapr, 0xffffffff);
		val = cfgread(bus, dev, func, mapr);
		if (val & 01) {
			/* PCI IO space */
			req = ~(val & 0xfffffffc) + 1;
			if (req & (req - 1))	/* power of 2 */
				continue;
			if (req == 0)		/* ever exists */
				continue;
			size = (req > 0x10) ? req : 0x10;
			mapbase = (iostart + size - 1) & ~(size - 1);
			if (mapbase + size > iolimit)
				continue;

			iostart = mapbase + size;
			/* establish IO space */
			cfgwrite(bus, dev, func, mapr, mapbase | 01);
		}
		else {
			/* PCI memory space */
			req = ~(val & 0xfffffff0) + 1;
			if (req & (req - 1))	/* power of 2 */
				continue;
			if (req == 0)		/* ever exists */
				continue;
			val &= 0x6;
			if (val == 2 || val == 6)
				continue;
			size = (req > 0x1000) ? req : 0x1000;
			mapbase = (memstart + size - 1) & ~(size - 1);
			if (mapbase + size > memlimit)
				continue;

			memstart = mapbase + size;
			/* establish memory space */
			cfgwrite(bus, dev, func, mapr, mapbase);
			if (val == 4) {
				mapr += 4;
				cfgwrite(bus, dev, func, mapr, 0);
			}
		}
printf("%s base %x size %x\n", (val & 01) ? "i/o" : "mem", mapbase, size);
	}
}

static int
devmatch(int bus, int dev, int func, unsigned long data)
{
	unsigned pciid;

	pciid = cfgread(bus, dev, func, PCI_ID_REG);
	return (pciid == (unsigned)data);
}

static int
clsmatch(int bus, int dev, int func, unsigned long data)
{
	unsigned class;

	class = cfgread(bus, dev, func, PCI_CLASS_REG);
	return ((class >> 16) == (unsigned)data);
}

static int
_pcilookup(int bus, int (*match)(int, int, int, unsigned long), unsigned long data, list, int index, int limit)
unsigned list[][2];
{
	int device, function, nfuncs;
	unsigned pciid, bhlcr, class;
	
	for (device = 0; device < MAXNDEVS; device++) {
		pciid = cfgread(bus, device, 0, PCI_ID_REG);
		if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
			continue;
		if (PCI_VENDOR(pciid) == 0)
			continue;
		class = cfgread(bus, device, 0, PCI_CLASS_REG);
		if ((class >> 16) == PCI_CLASS_PPB) {
			/* exploring bus beyond PCI-PCI bridge */
			index = _pcilookup(bus + 1,
				    match, data, list, index, limit);
			if (index >= limit)
				goto out;
			continue;
		}
		bhlcr = cfgread(bus, device, 0, PCI_BHLC_REG);
		nfuncs = (PCI_HDRTYPE_MULTIFN(bhlcr)) ? 8 : 1;
		for (function = 0; function < nfuncs; function++) {
			pciid = cfgread(bus, device, function, PCI_ID_REG);
			if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
				continue;
			if (PCI_VENDOR(pciid) == 0)
				continue;
			if ((*match)(bus, device, function, data)) {
				list[index][0] = pciid;
				list[index][1] = 
				     pcimaketag(bus, device, function);
				index += 1;
				if (index >= limit)
					goto out;
			}
		}
	}
  out:
	return index;
}
