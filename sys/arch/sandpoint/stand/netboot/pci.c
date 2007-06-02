/* $NetBSD: pci.c,v 1.1.2.1 2007/06/02 08:44:38 nisimura Exp $ */

#include <sys/param.h>
#include <lib/libsa/stand.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h> 

#include "globals.h"

/*
 * "Map B" layout
 *
 * practice direct mode configuration scheme with CONFIG_ADDR
 * (0xfec0'0000) and CONFIG_DATA (0xfee0'0000).
 */
#define PCI_MEMBASE	0x80000000
#define	PCI_MEMLIMIT	0xfbffffff	/* EUMB is next to this */
#define PCI_IOBASE	0x00001000	/* reserves room for via 686B */
#define	PCI_IOLIMIT	0x000fffff
#define	CONFIG_ADDR	0xfec00000
#define	CONFIG_DATA	0xfee00000

#define MAXNDEVS 32

static unsigned cfgread(unsigned, unsigned, unsigned, unsigned);
static void cfgwrite(unsigned, unsigned, unsigned, unsigned, unsigned);
static int _pcifinddev(unsigned, unsigned, unsigned, unsigned *);
static void busprobe(unsigned);
static void devicemap(unsigned, unsigned, unsigned);
static void deviceinit(unsigned, unsigned, unsigned);

unsigned memstart, memlimit;
unsigned iostart, iolimit;
unsigned maxbus;

void
pcisetup()
{

	memstart = PCI_MEMBASE;
	memlimit = PCI_MEMLIMIT;
	iostart =  PCI_IOBASE;
	iolimit =  PCI_IOLIMIT;
	maxbus = 0;

	busprobe(0);
}

unsigned
pcimaketag(b, d, f)
	int b, d, f;
{
	return (1U << 31) | (b << 16) | (d << 11) | (f << 8);
}

void
pcidecomposetag(tag, b, d, f)
	unsigned tag;
	int *b, *d, *f;
{
	if (b != NULL)
		*b = (tag >> 16) & 0xff;
	if (d != NULL)
		*d = (tag >> 11) & 0x1f;
	if (f != NULL)
		*f = (tag >> 8) & 0x7;
	return;
}

int
pcifinddev(vend, prod, tag)
	unsigned vend, prod;
	unsigned *tag;
{
	return _pcifinddev(vend, prod, 0, tag);
}

unsigned
pcicfgread(tag, off)
	unsigned tag, off;
{
	unsigned cfg;
	
	cfg = tag | (off &~ 3);
	iohtole32(CONFIG_ADDR, cfg);
	return iole32toh(CONFIG_DATA);
}

void
pcicfgwrite(tag, off, val)
	unsigned tag, off, val;
{
	unsigned cfg;

	cfg = tag | (off &~ 3);
	iohtole32(CONFIG_ADDR, cfg);
	iohtole32(CONFIG_DATA, val);
}

static unsigned
cfgread(b, d, f, off)
	unsigned b, d, f, off;
{
	unsigned cfg;
	
	cfg = (1U << 31) | (b << 16) | (d << 11) | (f << 8) | off | 0;
	iohtole32(CONFIG_ADDR, cfg);
	return iole32toh(CONFIG_DATA);
}

static void
cfgwrite(b, d, f, off, val)
	unsigned b, d, f, off, val;
{
	unsigned cfg;

	cfg = (1U << 31) | (b << 16) | (d << 11) | (f << 8) | off | 0;
	iohtole32(CONFIG_ADDR, cfg);
	iohtole32(CONFIG_DATA, val);
}

static int _maxbus;

static int
_pcifinddev(vend, prod, bus, tag)
	unsigned vend, prod, bus;
	unsigned *tag;
{
	unsigned device, function, nfunctions;
	unsigned pciid, bhlcr;

	for (device = 0; device < MAXNDEVS; device++) {
		pciid = cfgread(bus, device, 0, PCI_ID_REG);
		if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
			continue;
		if (PCI_VENDOR(pciid) == 0)
			continue;
		if ((cfgread(bus, device, 0, 0x08) >> 16) == 0x0604) {
			/* exploring bus beyond PCI-PCI bridge */
			_maxbus = (bus += 1);
			if (_pcifinddev(vend, prod, bus, tag) == 0)
				return 0;
			continue;
		}
		bhlcr = cfgread(bus, device, 0, PCI_BHLC_REG);
		nfunctions = (PCI_HDRTYPE_MULTIFN(bhlcr)) ? 8 : 1;
		for (function = 0; function < nfunctions; function++) {
			pciid = cfgread(bus, device, function, PCI_ID_REG);
			if (PCI_VENDOR(pciid) == PCI_VENDOR_INVALID)
				continue;
			if (PCI_VENDOR(pciid) == 0)
				continue;
			if (PCI_VENDOR(pciid) == vend
			    && PCI_PRODUCT(pciid) == prod) {
				*tag = pcimaketag(bus, device, function);
				return 0;
			}
		}
	}
	*tag = ~0;
	return -1;
}

static void
busprobe(bus)
	unsigned bus;
{
	unsigned device, function, nfunctions;
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

			deviceinit(bus, device, function);
		}
	}
}

static void
deviceinit(bus, dev, func)
	unsigned bus, dev, func;
{
	unsigned val;

	/* 0x00 */
	printf("%02d:%02d:%02d:", bus, dev, func);
	val = cfgread(bus, dev, func, 0x00);
	printf(" chip %04x,%04x", val & 0xffff, val>>16);
	val = cfgread(bus, dev, func, 0x2c);
	printf(" card %04x,%04x", val & 0xffff, val>>16);
	val = cfgread(bus, dev, func, 0x08);
	printf(" rev %02x class %02x,%02x,%02x",
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

	devicemap(bus, dev, func);

	/* descending toward PCI-PCI bridge */
	if ((cfgread(bus, dev, func, 0x08) >> 16) == 0x0604) {
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

		busprobe(new);

		/* adjust 0x18 */
		val = cfgread(bus, dev, func, 0x18);
		val = (maxbus << 16) | (val & 0xffff);
		cfgwrite(bus, dev, func, 0x18, val);
	}
}

static void
devicemap(bus, dev, func)
	unsigned bus, dev, func;
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
