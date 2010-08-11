/* $NetBSD: brdsetup.c,v 1.6.2.5 2010/08/11 22:52:39 yamt Exp $ */

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

#include <powerpc/oea/spr.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libkern/libkern.h>

#include <machine/bootinfo.h>

#include "globals.h"

#define BRD_DECL(xxx) \
    void xxx ## setup(struct brdprop *); \
    void xxx ## brdfix(struct brdprop *); \
    void xxx ## pcifix(struct brdprop *); \
    void xxx ## reset(void)

BRD_DECL(mot);
BRD_DECL(enc);
BRD_DECL(kuro);
BRD_DECL(syno);
BRD_DECL(qnap);

static struct brdprop brdlist[] = {
    {
	"sandpoint",
	"Sandpoint X3",
	BRD_SANDPOINTX3,
	0,
	"com", 0x3f8, 115200,
	motsetup, motbrdfix, motpcifix },
    {
	"encpp1",
	"EnCore PP1",
	BRD_ENCOREPP1,
	0,
	"com", 0x3f8, 115200,
	encsetup, encbrdfix, encpcifix },
    {
	"kurobox",
	"KuroBox",
	BRD_KUROBOX,
	32768000,
	"eumb", 0x4600, 57600,
	kurosetup, kurobrdfix, kuropcifix },
    {
	"synology",
	"Synology DS",
	BRD_SYNOLOGY,
	33164691,	/* from Synology/Linux source */
	/* 33168000,		XXX better precision? */
	"eumb", 0x4500, 115200,
	synosetup, synobrdfix, synopcifix, synoreset },
    {
	"qnap",
	"QNAP TS-101",
	BRD_QNAPTS101,
	0,
	"eumb", 0x4500, 115200,
	NULL, NULL, qnappcifix },
    {
	"iomega",
	"IOMEGA Storcenter",
	BRD_STORCENTER,
	0,
	"eumb", 0x4500, 115200,
	NULL, NULL, NULL },
    {
	"unknown",
	"Unknown board",
	BRD_UNKNOWN,
	0,
	"eumb", 0x4500, 115200,
	NULL, NULL, NULL }, /* must be the last */
};

static struct brdprop *brdprop;
static uint32_t ticks_per_sec, ns_per_tick;

static void brdfixup(void);
static void setup(void);
static inline uint32_t cputype(void);
static inline u_quad_t mftb(void);
static void init_uart(unsigned, unsigned, uint8_t);
static void send_sat(char *);

const unsigned dcache_line_size = 32;		/* 32B linesize */
const unsigned dcache_range_size = 4 * 1024;	/* 16KB / 4-way */

unsigned uart1base;	/* console */
unsigned uart2base;	/* optional satellite processor */
#define THR		0
#define DLB		0
#define DMB		1
#define IER		1
#define FCR		2
#define LCR		3
#define  LCR_DLAB	0x80
#define  LCR_PEVEN	0x18
#define  LCR_PNONE	0x00
#define  LCR_8BITS	0x03
#define MCR		4
#define  MCR_RTS	0x02
#define  MCR_DTR	0x01
#define LSR		5
#define  LSR_THRE	0x20
#define DCR		0x11
#define UART_READ(base, r)	*(volatile char *)(base + (r))
#define UART_WRITE(base, r, v)	*(volatile char *)(base + (r)) = (v)

void brdsetup(void);	/* called by entry.S */

void
brdsetup(void)
{
	static uint8_t pci_to_memclk[] = {
		30, 30, 10, 10, 20, 10, 10, 10,
		10, 20, 20, 15, 20, 15, 20, 30,
		30, 40, 15, 40, 20, 25, 20, 40,
		25, 20, 10, 20, 15, 15, 20, 00
	};
	static uint8_t mem_to_cpuclk[] = {
		25, 30, 45, 20, 20, 00, 10, 30,
		30, 20, 45, 30, 25, 35, 30, 35,
		20, 25, 20, 30, 35, 40, 40, 20,
		30, 25, 40, 30, 30, 25, 35, 00
	};
	char *consname;
	int consport;
	uint32_t extclk;
	unsigned pchb, pcib, val;
	extern struct btinfo_memory bi_mem;
	extern struct btinfo_console bi_cons;
	extern struct btinfo_clock bi_clk;
	extern struct btinfo_prodfamily bi_fam;

	/*
	 * CHRP specification "Map-B" BAT012 layout
	 *   BAT0 0000-0000 (256MB) SDRAM
	 *   BAT1 8000-0000 (256MB) PCI mem space
	 *   BAT2 fc00-0000 (64MB)  EUMB, PCI I/O space, misc devs, flash
	 *
	 * EUMBBAR is at fc00-0000.
	 */
	pchb = pcimaketag(0, 0, 0);
	pcicfgwrite(pchb, 0x78, 0xfc000000);

	brdtype = BRD_UNKNOWN;
	extclk = EXT_CLK_FREQ;	/* usually 33MHz */
	busclock = 0;

	if (pcifinddev(0x10ad, 0x0565, &pcib) == 0) {
		brdtype = BRD_SANDPOINTX3;
	}
	else if (pcifinddev(0x1106, 0x0686, &pcib) == 0) {
		brdtype = BRD_ENCOREPP1;
	}
	else if ((pcicfgread(pcimaketag(0, 11, 0), PCI_CLASS_REG) >> 16) ==
	    PCI_CLASS_ETH) {
		/* tlp (ADMtek AN985) or re (RealTek 8169S) at dev 11 */
		brdtype = BRD_KUROBOX;
	}
	else if (PCI_VENDOR(pcicfgread(pcimaketag(0, 15, 0), PCI_ID_REG)) ==
	    0x11ab) {				/* PCI_VENDOR_MARVELL */
		brdtype = BRD_SYNOLOGY;
	}
	else if (PCI_VENDOR(pcicfgread(pcimaketag(0, 15, 0), PCI_ID_REG)) ==
	    0x8086) {				/* PCI_VENDOR_INTEL */
		brdtype = BRD_QNAPTS101;
	}

	brdprop = brd_lookup(brdtype);

	/* brd dependent adjustments */
	setup();

	/* determine clock frequencies */
	if (brdprop->extclk != 0)
		extclk = brdprop->extclk;
	if (busclock == 0) {
		if (cputype() == MPC8245) {
			/* PLL_CFG from PCI host bridge register 0xe2 */
			val = pcicfgread(pchb, 0xe0);
			busclock = (extclk *
			    pci_to_memclk[(val >> 19) & 0x1f] + 10) / 10;
			/* PLLRATIO from HID1 */
			__asm ("mfspr %0,1009" : "=r"(val));
			cpuclock = ((uint64_t)busclock *
			    mem_to_cpuclk[val >> 27] + 10) / 10;
		} else
			busclock = 100000000;	/* 100MHz bus clock default */
	}
	ticks_per_sec = busclock >> 2;
	ns_per_tick = 1000000000 / ticks_per_sec;

	/* now prepare serial console */
	consname = brdprop->consname;
	consport = brdprop->consport;
	if (strcmp(consname, "eumb") == 0) {
		uart1base = 0xfc000000 + consport;	/* 0x4500, 0x4600 */
		UART_WRITE(uart1base, DCR, 0x01);	/* enable DUART mode */
		uart2base = uart1base ^ 0x0300;
	} else
		uart1base = 0xfe000000 + consport;	/* 0x3f8, 0x2f8 */

	/* more brd adjustments */
	brdfixup();

	bi_mem.memsize = mpc107memsize();
	snprintf(bi_cons.devname, sizeof(bi_cons.devname), consname);
	bi_cons.addr = consport;
	bi_cons.speed = brdprop->consspeed;
	bi_clk.ticks_per_sec = ticks_per_sec;
	snprintf(bi_fam.name, sizeof(bi_fam.name), brdprop->family);
}

struct brdprop *
brd_lookup(int brd)
{
	u_int i;

	for (i = 0; i < sizeof(brdlist)/sizeof(brdlist[0]); i++) {
		if (brdlist[i].brdtype == brd)
			return &brdlist[i];
	}
	return &brdlist[i - 1];
}

static void
setup()
{
	if (brdprop->setup == NULL)
		return;
	(*brdprop->setup)(brdprop);
}

static void
brdfixup()
{
	if (brdprop->brdfix == NULL)
		return;
	(*brdprop->brdfix)(brdprop);
}

void
pcifixup()
{
	if (brdprop->pcifix == NULL)
		return;
	(*brdprop->pcifix)(brdprop);
}

void
encsetup(struct brdprop *brd)
{
#ifdef COSNAME
	brd->consname = CONSNAME;
#endif
#ifdef CONSPORT
	brd->consport = CONSPORT;
#endif
#ifdef CONSSPEED
	brd->consspeed = CONSSPEED;
#endif
}

void
encbrdfix(struct brdprop *brd)
{
	unsigned ac97, ide, pcib, pmgt, usb12, umot4, val;

/*
 * VIA82C686B Southbridge
 *	0.22.0	1106.0686	PCI-ISA bridge
 *	0.22.1	1106.0571	IDE (viaide)
 *	0.22.2	1106.3038	USB 0/1 (uhci)
 *	0.22.3	1106.3038	USB 2/3 (uhci)
 *	0.22.4	1106.3057	power management
 *	0.22.5	1106.3058	AC97 (auvia)
 */
	pcib  = pcimaketag(0, 22, 0);
	ide   = pcimaketag(0, 22, 1);
	usb12 = pcimaketag(0, 22, 2);
	umot4 = pcimaketag(0, 22, 3);
	pmgt  = pcimaketag(0, 22, 4);
	ac97  = pcimaketag(0, 22, 5);

#define	CFG(i,v) do { \
   *(volatile unsigned char *)(0xfe000000 + 0x3f0) = (i); \
   *(volatile unsigned char *)(0xfe000000 + 0x3f1) = (v); \
   } while (0)
	val = pcicfgread(pcib, 0x84);
	val |= (02 << 8);
	pcicfgwrite(pcib, 0x84, val);
	CFG(0xe2, 0x0f); /* use COM1/2, don't use FDC/LPT */
	val = pcicfgread(pcib, 0x84);
	val &= ~(02 << 8);
	pcicfgwrite(pcib, 0x84, val);

	/* route pin C to i8259 IRQ 5, pin D to 11 */
	val = pcicfgread(pcib, 0x54);
	val = (val & 0xff) | 0xb0500000; /* Dx CB Ax xS */
	pcicfgwrite(pcib, 0x54, val);

	/* enable EISA ELCR1 (0x4d0) and ELCR2 (0x4d1) */
	val = pcicfgread(pcib, 0x44);
	val = val | 0x20000000;
	pcicfgwrite(pcib, 0x44, val);

	/* select level trigger for IRQ 5/11 at ELCR1/2 */
	*(volatile uint8_t *)0xfe0004d0 = 0x20; /* bit 5 */
	*(volatile uint8_t *)0xfe0004d1 = 0x08; /* bit 11 */

	/* USB and AC97 are hardwired with pin D and C */
	val = pcicfgread(usb12, 0x3c) &~ 0xff;
	val |= 11;
	pcicfgwrite(usb12, 0x3c, val);
	val = pcicfgread(umot4, 0x3c) &~ 0xff;
	val |= 11;
	pcicfgwrite(umot4, 0x3c, val);
	val = pcicfgread(ac97, 0x3c) &~ 0xff;
	val |= 5;
	pcicfgwrite(ac97, 0x3c, val);
}

void
motsetup(struct brdprop *brd)
{
#ifdef COSNAME
	brd->consname = CONSNAME;
#endif
#ifdef CONSPORT
	brd->consport = CONSPORT;
#endif
#ifdef CONSSPEED
	brd->consspeed = CONSSPEED;
#endif
}

void
motbrdfix(struct brdprop *brd)
{
/*
 * WinBond/Symphony Lab 83C553 with PC87308 "SuperIO"
 *
 *	0.11.0	10ad.0565	PCI-ISA bridge
 *	0.11.1	10ad.0105	IDE (slide)
 */
}

void
motpcifix(struct brdprop *brd)
{
	unsigned ide, nic, pcib, steer, val;
	int line;

	pcib = pcimaketag(0, 11, 0);
	ide  = pcimaketag(0, 11, 1);
	nic  = pcimaketag(0, 15, 0);

	/*
	 * //// WinBond PIRQ ////
	 * 0x40 - bit 5 (0x20) indicates PIRQ presense
	 * 0x60 - PIRQ interrupt routing steer
	 */
	if (pcicfgread(pcib, 0x40) & 0x20) {
		steer = pcicfgread(pcib, 0x60);
		if ((steer & 0x80808080) == 0x80808080)
			printf("PIRQ[0-3] disabled\n");
		else {
			unsigned i, v = steer;
			for (i = 0; i < 4; i++, v >>= 8) {
				if ((v & 0x80) != 0 || (v & 0xf) == 0)
					continue;
				printf("PIRQ[%d]=%d\n", i, v & 0xf);
				}
			}
		}
#if 1
	/*
	 * //// IDE fixup -- case A ////
	 * - "native PCI mode" (ide 0x09)
	 * - don't use ISA IRQ14/15 (pcib 0x43)
	 * - native IDE for both channels (ide 0x40)
	 * - LEGIRQ bit 11 steers interrupt to pin C (ide 0x40)
	 * - sign as PCI pin C line 11 (ide 0x3d/3c)
	 */
	/* ide: 0x09 - programming interface; 1000'SsPp */
	val = pcicfgread(ide, 0x08);
	val &= 0xffff00ff;
	pcicfgwrite(ide, 0x08, val | (0x8f << 8));

	/* pcib: 0x43 - IDE interrupt routing */
	val = pcicfgread(pcib, 0x40) & 0x00ffffff;
	pcicfgwrite(pcib, 0x40, val);

	/* pcib: 0x45/44 - PCI interrupt routing */
	val = pcicfgread(pcib, 0x44) & 0xffff0000;
	pcicfgwrite(pcib, 0x44, val);

	/* ide: 0x41/40 - IDE channel */
	val = pcicfgread(ide, 0x40) & 0xffff0000;
	val |= (1 << 11) | 0x33; /* LEGIRQ turns on PCI interrupt */
	pcicfgwrite(ide, 0x40, val);

	/* ide: 0x3d/3c - use PCI pin C/line 11 */
	val = pcicfgread(ide, 0x3c) & 0xffffff00;
	val |= 11; /* pin designation is hardwired to pin A */
	pcicfgwrite(ide, 0x3c, val);
#else
	/*
	 * //// IDE fixup -- case B ////
	 * - "compatiblity mode" (ide 0x09)
	 * - IDE primary/secondary interrupt routing (pcib 0x43)
	 * - PCI interrupt routing (pcib 0x45/44)
	 * - no PCI pin/line assignment (ide 0x3d/3c)
	 */
	/* ide: 0x09 - programming interface; 1000'SsPp */
	val = pcicfgread(ide, 0x08);
	val &= 0xffff00ff;
	pcicfgwrite(ide, 0x08, val | (0x8a << 8));

	/* pcib: 0x43 - IDE interrupt routing */
	val = pcicfgread(pcib, 0x40) & 0x00ffffff;
	pcicfgwrite(pcib, 0x40, val | (0xee << 24));

	/* ide: 0x45/44 - PCI interrupt routing */
	val = pcicfgread(ide, 0x44) & 0xffff0000;
	pcicfgwrite(ide, 0x44, val);

	/* ide: 0x3d/3c - turn off PCI pin/line */
	val = pcicfgread(ide, 0x3c) & 0xffff0000;
	pcicfgwrite(ide, 0x3c, val);
#endif

	/*
	 * //// fxp fixup ////
	 * - use PCI pin A line 15 (fxp 0x3d/3c)
	 */
	val = pcicfgread(nic, 0x3c) & 0xffff0000;
	pcidecomposetag(nic, NULL, &line, NULL);
	val |= (('A' - '@') << 8) | line;
	pcicfgwrite(nic, 0x3c, val);
}

void
encpcifix(struct brdprop *brd)
{
	unsigned ide, irq, nic, pcib, steer, val;

#define	STEER(v, b) (((v) & (b)) ? "edge" : "level")
	pcib = pcimaketag(0, 22, 0);
	ide  = pcimaketag(0, 22, 1);
	nic  = pcimaketag(0, 25, 0);

	/*
	 * //// VIA PIRQ ////
	 * 0x57/56/55/54 - Dx CB Ax xS
	 */
	val = pcicfgread(pcib, 0x54);	/* Dx CB Ax xs */
	steer = val & 0xf;
	irq = (val >> 12) & 0xf;	/* 15:12 */
	if (irq) {
		printf("pin A -> irq %d, %s\n",
			irq, STEER(steer, 0x1));
	}
	irq = (val >> 16) & 0xf;	/* 19:16 */
	if (irq) {
		printf("pin B -> irq %d, %s\n",
			irq, STEER(steer, 0x2));
	}
	irq = (val >> 20) & 0xf;	/* 23:20 */
	if (irq) {
		printf("pin C -> irq %d, %s\n",
			irq, STEER(steer, 0x4));
	}
	irq = (val >> 28);		/* 31:28 */
	if (irq) {
		printf("pin D -> irq %d, %s\n",
			irq, STEER(steer, 0x8));
	}
#if 0
	/*
	 * //// IDE fixup ////
	 * - "native mode" (ide 0x09)
	 * - use primary only (ide 0x40)
	 */
	/* ide: 0x09 - programming interface; 1000'SsPp */
	val = pcicfgread(ide, 0x08) & 0xffff00ff;
	pcicfgwrite(ide, 0x08, val | (0x8f << 8));

	/* ide: 0x10-20 - leave them PCI memory space assigned */

	/* ide: 0x40 - use primary only */
	val = pcicfgread(ide, 0x40) &~ 03;
	val |= 02;
	pcicfgwrite(ide, 0x40, val);
#else
	/*
	 * //// IDE fixup ////
	 * - "compatiblity mode" (ide 0x09)
	 * - use primary only (ide 0x40)
	 * - remove PCI pin assignment (ide 0x3d)
	 */
	/* ide: 0x09 - programming interface; 1000'SsPp */
	val = pcicfgread(ide, 0x08) & 0xffff00ff;
	val |= (0x8a << 8);
	pcicfgwrite(ide, 0x08, val);

	/* ide: 0x10-20 */
	/*
	experiment shows writing ide: 0x09 changes these
	register behaviour. The pcicfgwrite() above writes
	0x8a at ide: 0x09 to make sure legacy IDE.  Then
	reading BAR0-3 is to return value 0s even though
	pcisetup() has written range assignments.  Value
	overwrite makes no effect. Having 0x8f for native
	PCIIDE doesn't change register values and brings no
	weirdness.
	 */

	/* ide: 0x40 - use primary only */
	val = pcicfgread(ide, 0x40) &~ 03;
	val |= 02;
	pcicfgwrite(ide, 0x40, val);

		/* ide: 0x3d/3c - turn off PCI pin */
	val = pcicfgread(ide, 0x3c) & 0xffff00ff;
	pcicfgwrite(ide, 0x3c, val);
#endif
	/*
	 * //// USBx2, audio, and modem fixup ////
	 * - disable USB #0 and #1 (pcib 0x48 and 0x85)
	 * - disable AC97 audio and MC97 modem (pcib 0x85)
	 */

	/* pcib: 0x48 - disable USB #0 at function 2 */
	val = pcicfgread(pcib, 0x48);
	pcicfgwrite(pcib, 0x48, val | 04);

	/* pcib: 0x85 - disable USB #1 at function 3 */
	/* pcib: 0x85 - disable AC97/MC97 at function 5/6 */
	val = pcicfgread(pcib, 0x84);
	pcicfgwrite(pcib, 0x84, val | 0x1c00);

	/*
	 * //// fxp fixup ////
	 * - use PCI pin A line 25 (fxp 0x3d/3c)
	 */
	/* 0x3d/3c - PCI pin/line */
	val = pcicfgread(nic, 0x3c) & 0xffff0000;
	val |= (('A' - '@') << 8) | 25;
	pcicfgwrite(nic, 0x3c, val);
}

void
kurosetup(struct brdprop *brd)
{

	if (PCI_VENDOR(pcicfgread(pcimaketag(0, 11, 0), PCI_ID_REG)) == 0x10ec)
		brd->extclk = 32768000; /* decr 2457600Hz */
	else
		brd->extclk = 32521333; /* decr 2439100Hz */
}

void
kurobrdfix(struct brdprop *brd)
{

	init_uart(uart2base, 9600, LCR_8BITS | LCR_PEVEN);
	/* Stop Watchdog */
	send_sat("AAAAFFFFJJJJ>>>>VVVV>>>>ZZZZVVVVKKKK");
}

void
kuropcifix(struct brdprop *brd)
{
	unsigned ide, nic, usb, val;

	nic = pcimaketag(0, 11, 0);
	val = pcicfgread(nic, 0x3c) & 0xffffff00;
	val |= 11;
	pcicfgwrite(nic, 0x3c, val);

	ide = pcimaketag(0, 12, 0);
	val = pcicfgread(ide, 0x3c) & 0xffffff00;
	val |= 12;
	pcicfgwrite(ide, 0x3c, val);

	usb = pcimaketag(0, 14, 0);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 1);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 2);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);
}

void
synosetup(struct brdprop *brd)
{
	/* nothing */
}

void
synobrdfix(struct brdprop *brd)
{

	init_uart(uart2base, 9600, LCR_8BITS | LCR_PNONE);
	/* beep, power LED on, status LED off */
	send_sat("247");
}

void
synopcifix(struct brdprop *brd)
{
	unsigned ide, nic, usb, val;

	ide = pcimaketag(0, 13, 0);
	val = pcicfgread(ide, 0x3c) & 0xffffff00;
	val |= 13;
	pcicfgwrite(ide, 0x3c, val);

	usb = pcimaketag(0, 14, 0);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 1);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 2);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	nic = pcimaketag(0, 15, 0);
	val = pcicfgread(nic, 0x3c) & 0xffffff00;
	val |= 15;
	pcicfgwrite(nic, 0x3c, val);
}

void
qnappcifix(struct brdprop *brd)
{
	unsigned ide, nic, usb, val;

	ide = pcimaketag(0, 13, 0);
	val = pcicfgread(ide, 0x3c) & 0xffffff00;
	val |= 13;
	pcicfgwrite(ide, 0x3c, val);

	usb = pcimaketag(0, 14, 0);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 1);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	usb = pcimaketag(0, 14, 2);
	val = pcicfgread(usb, 0x3c) & 0xffffff00;
	val |= 14;
	pcicfgwrite(usb, 0x3c, val);

	nic = pcimaketag(0, 15, 0);
	val = pcicfgread(nic, 0x3c) & 0xffffff00;
	val |= 15;
	pcicfgwrite(nic, 0x3c, val);
}

void
synoreset()
{

	send_sat("C");
	/*NOTRECHED*/
}

void
_rtt(void)
{
	if (brdprop->reset != NULL)
		(*brdprop->reset)();
	else
		run(0, 0, 0, 0, (void *)0xFFF00100); /* reset entry */
	/*NOTREACHED*/
}

satime_t
getsecs(void)
{
	u_quad_t tb = mftb();

	return (tb / ticks_per_sec);
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(u_int n)
{
	u_quad_t tb;
	u_long scratch, tbh, tbl;

	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	asm volatile ("1: mftbu %0; cmpw %0,%1; blt 1b; bgt 2f; mftb %0; cmpw 0, %0,%2; blt 1b; 2:" : "=&r"(scratch) : "r"(tbh), "r"(tbl));
}

void
_wb(uint32_t adr, uint32_t siz)
{
	uint32_t bnd;

	asm volatile("eieio");
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile ("dcbst 0,%0" :: "r"(adr));
	asm volatile ("sync");
}

void
_wbinv(uint32_t adr, uint32_t siz)
{
	uint32_t bnd;

	asm volatile("eieio");
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile ("dcbf 0,%0" :: "r"(adr));
	asm volatile ("sync");
}

void
_inv(uint32_t adr, uint32_t siz)
{
	uint32_t bnd, off;

	off = adr & (dcache_line_size - 1);
	adr -= off;
	siz += off;
	asm volatile ("eieio");
	if (off != 0) {
		/* wbinv() leading unaligned dcache line */
		asm volatile ("dcbf 0,%0" :: "r"(adr));
		if (siz < dcache_line_size)
			goto done;
		adr += dcache_line_size;
		siz -= dcache_line_size;
	}
	bnd = adr + siz;
	off = bnd & (dcache_line_size - 1);
	if (off != 0) {
		/* wbinv() trailing unaligned dcache line */
		asm volatile ("dcbf 0,%0" :: "r"(bnd)); /* it's OK */
		if (siz < dcache_line_size)
			goto done;
		siz -= off;
	}
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size) {
		/* inv() intermediate dcache lines if ever */
		asm volatile ("dcbi 0,%0" :: "r"(adr));
	}
  done:
	asm volatile ("sync");
}

static inline uint32_t
cputype(void)
{
	uint32_t pvr;

	__asm volatile ("mfpvr %0" : "=r"(pvr));
	return pvr >> 16;
}

static inline u_quad_t
mftb(void)
{
	u_long scratch;
	u_quad_t tb;

	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return (tb);
}

static void
init_uart(unsigned base, unsigned speed, uint8_t lcr)
{
	unsigned div;

	div = busclock / speed / 16;
	UART_WRITE(base, LCR, 0x80);		/* turn on DLAB bit */
	UART_WRITE(base, FCR, 0x00);
	UART_WRITE(base, DMB, div >> 8);	/* set speed */
	UART_WRITE(base, DLB, div & 0xff);
	UART_WRITE(base, LCR, lcr);
	UART_WRITE(base, FCR, 0x07);		/* FIFO on, TXRX FIFO reset */
	UART_WRITE(base, IER, 0x00);		/* make sure INT disabled */
}

/* talk to satellite processor */
static void
send_sat(char *msg)
{
	unsigned savedbase;

	savedbase = uart1base;
	uart1base = uart2base;
	while (*msg)
		putchar(*msg++);
	uart1base = savedbase;
}

void
putchar(int c)
{
	unsigned timo, lsr;

	if (c == '\n')
		putchar('\r');

	timo = 0x00100000;
	do {
		lsr = UART_READ(uart1base, LSR);
	} while (timo-- > 0 && (lsr & LSR_THRE) == 0);
	if (timo > 0)
		UART_WRITE(uart1base, THR, c);
}

unsigned
mpc107memsize()
{
	unsigned bankn, end, n, tag, val;

	tag = pcimaketag(0, 0, 0);

	if (brdtype == BRD_ENCOREPP1) {
		/* the brd's PPCBOOT looks to have erroneous values */
		unsigned tbl[] = {
#define MPC106_MEMSTARTADDR1	0x80
#define MPC106_EXTMEMSTARTADDR1	0x88
#define MPC106_MEMENDADDR1	0x90
#define MPC106_EXTMEMENDADDR1	0x98
#define MPC106_MEMEN		0xa0
#define	BK0_S	0x00000000
#define	BK0_E	(128 << 20) - 1
#define BK1_S	0x3ff00000
#define BK1_E	0x3fffffff
#define BK2_S	0x3ff00000
#define BK2_E	0x3fffffff
#define BK3_S	0x3ff00000
#define BK3_E	0x3fffffff
#define AR(v, s) ((((v) & SAR_MASK) >> SAR_SHIFT) << (s))
#define XR(v, s) ((((v) & EAR_MASK) >> EAR_SHIFT) << (s))
#define SAR_MASK 0x0ff00000
#define SAR_SHIFT    20
#define EAR_MASK 0x30000000
#define EAR_SHIFT    28
		AR(BK0_S, 0) | AR(BK1_S, 8) | AR(BK2_S, 16) | AR(BK3_S, 24),
		XR(BK0_S, 0) | XR(BK1_S, 8) | XR(BK2_S, 16) | XR(BK3_S, 24),
		AR(BK0_E, 0) | AR(BK1_E, 8) | AR(BK2_E, 16) | AR(BK3_E, 24),
		XR(BK0_E, 0) | XR(BK1_E, 8) | XR(BK2_E, 16) | XR(BK3_E, 24),
		};
		tag = pcimaketag(0, 0, 0);
		pcicfgwrite(tag, MPC106_MEMSTARTADDR1, tbl[0]);
		pcicfgwrite(tag, MPC106_EXTMEMSTARTADDR1, tbl[1]);
		pcicfgwrite(tag, MPC106_MEMENDADDR1, tbl[2]);
		pcicfgwrite(tag, MPC106_EXTMEMENDADDR1,	tbl[3]);
		pcicfgwrite(tag, MPC106_MEMEN, 1);
	}

	bankn = 0;
	val = pcicfgread(tag, MPC106_MEMEN);
	for (n = 0; n < 4; n++) {
		if ((val & (1U << n)) == 0)
			break;
		bankn = n;
	}
	bankn = bankn * 8;

	val = pcicfgread(tag, MPC106_EXTMEMENDADDR1);
	end =  ((val >> bankn) & 0x03) << 28;
	val = pcicfgread(tag, MPC106_MEMENDADDR1);
	end |= ((val >> bankn) & 0xff) << 20;
	end |= 0xfffff;

	return (end + 1); /* assume the end address matches total amount */
}

struct fis_dir_entry {
	char		name[16];
	uint32_t	startaddr;
	uint32_t	loadaddr;
	uint32_t	flashsize;
	uint32_t	entryaddr;
	uint32_t	filesize;
	char		pad[256 - (16 + 5 * sizeof(uint32_t))];
};

#define FIS_LOWER_LIMIT	0xfff00000

/*
 * Look for a Redboot-style Flash Image System FIS-directory and
 * return a pointer to the start address of the requested file.
 */
static void *
redboot_fis_lookup(const char *filename)
{
	static const char FISdirname[16] = {
	    'F', 'I', 'S', ' ',
	    'd', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', 0, 0, 0
	};
	struct fis_dir_entry *dir;

	/*
	 * The FIS directory is usually in the last sector of the flash.
	 * But we do not know the sector size (erase size), so start
	 * at 0xffffff00 and scan backwards in steps of the FIS directory
	 * entry size (0x100).
	 */
	for (dir = (struct fis_dir_entry *)0xffffff00;
	    (uint32_t)dir >= FIS_LOWER_LIMIT; dir--)
		if (memcmp(dir->name, FISdirname, sizeof(FISdirname)) == 0)
			break;
	if ((uint32_t)dir < FIS_LOWER_LIMIT) {
		printf("No FIS directory found!\n");
		return NULL;
	}

	/* Now find filename by scanning the directory from beginning. */
	dir = (struct fis_dir_entry *)dir->startaddr;
	while (dir->name[0] != 0xff && (uint32_t)dir < 0xffffff00) {
		if (strcmp(dir->name, filename) == 0)
			return (void *)dir->startaddr;	/* found */
		dir++;
	}
	printf("\"%s\" not found in FIS directory!\n", filename);
	return NULL;
}

/*
 * For cost saving reasons some NAS boxes are missing the ROM for the
 * NIC's ethernet address and keep it in their Flash memory.
 */
void
read_mac_from_flash(uint8_t *mac)
{
	uint8_t *p;

	if (brdtype == BRD_SYNOLOGY) {
		p = redboot_fis_lookup("vendor");
		if (p != NULL) {
			memcpy(mac, p, 6);
			return;
		}
	} else
		printf("Warning: This board has no known method defined "
		    "to determine its MAC address!\n");

	/* set to 00:00:00:00:00:00 in case of error */
	memset(mac, 0, 6);
}
