/* $NetBSD: brdsetup.c,v 1.6.2.3 2009/06/20 07:20:07 yamt Exp $ */

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
#include <lib/libsa/net.h>
#include <lib/libkern/libkern.h>

#include "globals.h"

const unsigned dcache_line_size = 32;		/* 32B linesize */
const unsigned dcache_range_size = 4 * 1024;	/* 16KB / 4-way */

void brdsetup(void);
void setup_82C686B(void);
void setup_83C553F(void);

static inline u_quad_t mftb(void);

unsigned uartbase;
#define THR		0
#define DLB		0
#define DMB		1
#define IER		1
#define FCR		2
#define LCR		3
#define MCR		4
#define LSR		5
#define DCR		11
#define LSR_THRE	0x20
#define UART_READ(r)		*(volatile char *)(uartbase + (r))
#define UART_WRITE(r, v)	*(volatile char *)(uartbase + (r)) = (v)

void
brdsetup(void)
{
	unsigned pchb, pcib, div;

	/* BAT to arrange address space */

	/* EUMBBAR */
	pchb = pcimaketag(0, 0, 0);
	pcicfgwrite(pchb, 0x78, 0xfc000000);

	brdtype = BRD_UNKNOWN;
	if (pcifinddev(0x10ad, 0x0565, &pcib) == 0) {
		brdtype = BRD_SANDPOINTX3;
		setup_83C553F();
	}
	else if (pcifinddev(0x1106, 0x0686, &pcib) == 0) {
		brdtype = BRD_ENCOREPP1;
		setup_82C686B();
	}
	else if ((pcicfgread(pcimaketag(0, 11, 0), PCI_CLASS_REG) >> 16) ==
	    PCI_CLASS_ETH) {
		/* tlp (ADMtek AN985) or re (RealTek 8169S) at dev 11 */
		brdtype = BRD_KUROBOX;
		consname = "eumb";
		consport = 0x4600;
		consspeed = 57600;
	}

	/* now prepare serial console */
	if (strcmp(consname, "eumb") != 0)
		uartbase = 0xfe000000 + consport; /* 0x3f8, 0x2f8 */
	else {
		uartbase = 0xfc000000 + consport; /* 0x4500, 0x4600 */
		div = (TICKS_PER_SEC * 4) / consspeed / 16;
		UART_WRITE(DCR, 0x01);	/* 2 independent UART */
		UART_WRITE(LCR, 0x80);	/* turn on DLAB bit */
		UART_WRITE(FCR, 0x00);
		UART_WRITE(DMB, div >> 8);
		UART_WRITE(DLB, div & 0xff); /* 0x36 when 115200bps@100MHz */
		UART_WRITE(LCR, 0x03);	/* 8 N 1 */
		UART_WRITE(MCR, 0x03);	/* RTS DTR */
		UART_WRITE(FCR, 0x07);	/* FIFO_EN | RXSR | TXSR */
		UART_WRITE(IER, 0x00);	/* make sure INT disabled */
	}
}

void
putchar(int c)
{
	unsigned timo, lsr;

	if (c == '\n')
		putchar('\r');

	timo = 0x00100000;
	do {
		lsr = UART_READ(LSR);
	} while (timo-- > 0 && (lsr & LSR_THRE) == 0);
	if (timo > 0)
		UART_WRITE(THR, c);
}

void
_rtt(void)
{
	run(0, 0, 0, 0, (void *)0xFFF00100); /* reset entry */
	/* NOTREACHED */
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

satime_t
getsecs(void)
{
	u_quad_t tb = mftb();

	return (tb / TICKS_PER_SEC);
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(u_int n)
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;

	tb = mftb();
	tb += (n * 1000 + NS_PER_TICK - 1) / NS_PER_TICK;
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
	uint32_t off, bnd;

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

unsigned
mpc107memsize()
{
	unsigned tag, val, n, bankn, end;

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

	return (end + 1); /* size of bankN SDRAM */
}

/*
 * VIA82C686B Southbridge
 *	0.22.0	1106.0686	PCI-ISA bridge
 *	0.22.1	1106.0571	IDE (viaide)
 *	0.22.2	1106.3038	USB 0/1 (uhci)
 *	0.22.3	1106.3038	USB 2/3 (uhci)
 *	0.22.4	1106.3057	power management
 *	0.22.5	1106.3058	AC97 (auvia)
 */
void
setup_82C686B()
{
	unsigned pcib, ide, usb12, usb34, ac97, pmgt, val;

	pcib  = pcimaketag(0, 22, 0);
	ide   = pcimaketag(0, 22, 1);
	usb12 = pcimaketag(0, 22, 2);
	usb34 = pcimaketag(0, 22, 3);
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
	val = pcicfgread(usb34, 0x3c) &~ 0xff;
	val |= 11;
	pcicfgwrite(usb34, 0x3c, val);
	val = pcicfgread(ac97, 0x3c) &~ 0xff;
	val |= 5;
	pcicfgwrite(ac97, 0x3c, val);
}

/*
 * WinBond/Symphony Lab 83C553 with PC87308 "SuperIO"
 *
 *	0.11.0	10ad.0565	PCI-ISA bridge
 *	0.11.1	10ad.0105	IDE (slide)
 */
void
setup_83C553F()
{
#if 0
	unsigned pcib, ide, val;

	pcib = pcimaketag(0, 11, 0);
	ide  = pcimaketag(0, 11, 1);
#endif
}

void
pcifixup(void)
{
	unsigned pcib, ide, nic, val, steer, irq;
	int line;

	switch (brdtype) {
	case BRD_SANDPOINTX3:
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
		break;

	case BRD_ENCOREPP1:
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
		break;
	}
}
