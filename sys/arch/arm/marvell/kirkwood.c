/*	$NetBSD: kirkwood.c,v 1.7.2.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kirkwood.c,v 1.7.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#define _INTR_PRIVATE

#include "mvsocgpp.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>
#include <arm/pic/picvar.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/kirkwoodreg.h>

#include <dev/marvell/marvellreg.h>


static void kirkwood_intr_init(void);

static void kirkwood_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void kirkwood_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void kirkwood_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void kirkwood_pic_source_name(struct pic_softc *, int, char *, size_t);

static int kirkwood_find_pending_irqs(void);

static void kirkwood_getclks(vaddr_t);
static int kirkwood_clkgating(struct marvell_attach_args *);

static const char * const sources[64] = {
    "MainHighSum(0)",  "Bridge(1)",       "Host2CPU DB(2)",  "CPU2Host DB(3)",
    "Reserved_4(4)",   "Xor0Chan0(5)",    "Xor0Chan1(6)",    "Xor1Chan0(7)",
    "Xor1Chan1(8)",    "PEX0INT(9)",      "Reserved(10)",    "GbE0Sum(11)",
    "GbE0Rx(12)",      "GbE0Tx(13)",      "GbE0Misc(14)",    "GbE1Sum(15)",
    "GbE1Rx(16)",      "GbE1Tx(17)",      "GbE1Misc(18)",    "USB0Cnt(19)",
    "Reserved(20)",    "Sata(21)",        "SecurityInt(22)", "SPIInt(23)",
    "AudioINT(24)",    "Reserved(25)",    "TS0Int(26)",      "Reserved(27)",
    "SDIOInt(28)",     "TWSI(29)",        "AVBInt(30)",      "TDMInt(31)",

    "Reserved(32)",    "Uart0Int(33)",    "Uart1Int(34)",    "GPIOLo7_0(35)",
    "GPIOLo8_15(36)",  "GPIOLo16_23(37)", "GPIOLo24_31(38)", "GPIOHi7_0(39)",
    "GPIOHi8_15(40)",  "GPIOHi16_23(41)", "XOR0Err(42)",     "XOR1Err(43)",
    "PEX0Err(44)",     "Reserved(45)",    "GbE0Err(46)",     "GbE1Err(47)",
    "USBErr(48)",      "SecurityErr(49)", "AudioErr(50)",    "Reserved(51)",
    "Reserved(52)",    "RTCInt(53)",      "Reserved(54)",    "Reserved(55)",
    "Reserved(56)",    "Reserved(57)",    "Reserved(58)",    "Reserved(59)",
    "Reserved(60)",    "Reserved(61)",    "Reserved(62)",    "Reserved(63)"
};

static struct pic_ops kirkwood_picops = {
	.pic_unblock_irqs = kirkwood_pic_unblock_irqs,
	.pic_block_irqs = kirkwood_pic_block_irqs,
	.pic_establish_irq = kirkwood_pic_establish_irq,
	.pic_source_name = kirkwood_pic_source_name,
};
static struct pic_softc kirkwood_pic = {
	.pic_ops = &kirkwood_picops,
	.pic_maxsources = 64,
	.pic_name = "kirkwood",
};

static struct {
	bus_size_t offset;
	uint32_t bits;
} clkgatings[]= {
	{ KIRKWOOD_GBE0_BASE,	(1 << 0) },
	{ MVSOC_PEX_BASE,	(1 << 2) },
	{ KIRKWOOD_USB_BASE,	(1 << 3) },
	{ KIRKWOOD_SDIO_BASE,	(1 << 4) },
	{ KIRKWOOD_MTS_BASE,	(1 << 5) },
#if 0
	{ Dunit, (1 << 6) },	/* SDRAM Unit Clock */
	{ Runit, (1 << 7) },	/* Runit Clock */
#endif
	{ KIRKWOOD_IDMAC_BASE,	(1 << 8) | (1 << 16) },
	{ KIRKWOOD_AUDIO_BASE,	(1 << 9) },
	{ KIRKWOOD_SATAHC_BASE, (1 << 14) | (1 << 15) },
	{ KIRKWOOD_CESA_BASE,	(1 << 17) },
	{ KIRKWOOD_GBE1_BASE,	(1 << 19) },
	{ KIRKWOOD_TDM_BASE,	(1 << 20) },
};


/*
 * kirkwood_bootstrap:
 *
 *	Initialize the rest of the Kirkwood dependencies, making it
 *	ready to handle interrupts from devices.
 */
void
kirkwood_bootstrap(vaddr_t iobase)
{

	/* disable all interrupts */
	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR, 0);
	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR, 0);

	/* disable all bridge interrupts */
	write_mlmbreg(MVSOC_MLMB_MLMBIMR, 0);

	mvsoc_intr_init = kirkwood_intr_init;

#if NMVSOCGPP > 0
	switch (mvsoc_model()) {
	case MARVELL_KIRKWOOD_88F6180: gpp_npins = 30; break;
	case MARVELL_KIRKWOOD_88F6192: gpp_npins = 36; break;
	case MARVELL_KIRKWOOD_88F6281: gpp_npins = 50; break;
	case MARVELL_KIRKWOOD_88F6282: gpp_npins = 50; break;
	}
	gpp_irqbase = 96;	/* Main Low(32) + High(32) + Bridge(32) */
#endif

	kirkwood_getclks(iobase);
	mvsoc_clkgating = kirkwood_clkgating;
}

static void
kirkwood_intr_init(void)
{
	extern struct pic_softc mvsoc_bridge_pic;
	void *ih __diagused;

	pic_add(&kirkwood_pic, 0);

	pic_add(&mvsoc_bridge_pic, 64);
	ih = intr_establish(KIRKWOOD_IRQ_BRIDGE, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &mvsoc_bridge_pic);
	KASSERT(ih != NULL);

	find_pending_irqs = kirkwood_find_pending_irqs;
}

/* ARGSUSED */
static void
kirkwood_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
			  uint32_t irq_mask)
{
	const size_t reg = KIRKWOOD_MLMB_MIRQIMLR
	   + irqbase * (KIRKWOOD_MLMB_MIRQIMHR - KIRKWOOD_MLMB_MIRQIMLR) / 32;

	KASSERT(irqbase < 64);
	write_mlmbreg(reg, read_mlmbreg(reg) | irq_mask);
}

/* ARGSUSED */
static void
kirkwood_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
			uint32_t irq_mask)
{
	const size_t reg = KIRKWOOD_MLMB_MIRQIMLR
	   + irqbase * (KIRKWOOD_MLMB_MIRQIMHR - KIRKWOOD_MLMB_MIRQIMLR) / 32;

	KASSERT(irqbase < 64);
	write_mlmbreg(reg, read_mlmbreg(reg) & ~irq_mask);
}

/* ARGSUSED */
static void
kirkwood_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
kirkwood_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	strlcpy(buf, sources[pic->pic_irqbase + irq], len);
}

/*
 * Called with interrupts disabled
 */
static int
kirkwood_find_pending_irqs(void)
{
	int ipl = 0;

	uint32_t causelow = read_mlmbreg(KIRKWOOD_MLMB_MICLR);
	uint32_t pendinglow = read_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR);

	pendinglow &= causelow;
	if (pendinglow != 0)
		ipl |= pic_mark_pending_sources(&kirkwood_pic, 0, pendinglow);

	if ((causelow & (1 << KIRKWOOD_IRQ_HIGH)) == (1 << KIRKWOOD_IRQ_HIGH)) {
		uint32_t causehigh = read_mlmbreg(KIRKWOOD_MLMB_MICHR);
		uint32_t pendinghigh = read_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR);
		pendinghigh &= causehigh;
		ipl |= pic_mark_pending_sources(&kirkwood_pic, 32, pendinghigh);
	}

	return ipl;
}

/*
 * Clock functions
 */

static void
kirkwood_getclks(vaddr_t iobase)
{
	uint32_t reg;
	uint16_t model;

#define MHz	* 1000 * 1000

	model = mvsoc_model();
	if (model == MARVELL_KIRKWOOD_88F6281 ||
	    model == MARVELL_KIRKWOOD_88F6282)
		mvTclk = 200 MHz;
	else		/* 166MHz */
		mvTclk = 166666667;

	reg = *(volatile uint32_t *)(iobase + KIRKWOOD_MPP_BASE +
	    KIRKWOOD_MPP_SAMPLE_AT_RESET);
	if (model == MARVELL_KIRKWOOD_88F6180) {
		switch (reg & 0x0000001c) {
		case 0x00000014: mvPclk =  600 MHz; break;
		case 0x00000018: mvPclk =  800 MHz; break;
		default:
			panic("unknown mvPclk\n");
		}
		mvSysclk = 200 MHz;
	} else {
		switch (reg & 0x0040001a) {
		case 0x00000002: mvPclk =  400 MHz; break;
		case 0x00000008: mvPclk =  600 MHz; break;
		case 0x00400008: mvPclk =  800 MHz; break;
		case 0x0040000a: mvPclk = 1000 MHz; break;
		case 0x00000012: mvPclk = 1200 MHz; break;
		case 0x00000018: mvPclk = 1500 MHz; break;
		case 0x0000001a: mvPclk = 1600 MHz; break;
		case 0x00400018: mvPclk = 1800 MHz; break;
		case 0x0040001a: mvPclk = 2000 MHz; break;
		default:
			panic("unknown mvPclk\n");
		}

		switch (reg & 0x000001e0) {
		case 0x00000000: mvSysclk = mvPclk * 1 / 1; break;
		case 0x00000040: mvSysclk = mvPclk * 1 / 2; break;
		case 0x00000060: mvSysclk = mvPclk * 2 / 5; break;
		case 0x00000080: mvSysclk = mvPclk * 1 / 3; break;
		case 0x000000c0: mvSysclk = mvPclk * 1 / 4; break;
		case 0x000000e0: mvSysclk = mvPclk * 2 / 9; break;
		case 0x00000100: mvSysclk = mvPclk * 1 / 5; break;
		case 0x00000120: mvSysclk = mvPclk * 1 / 6; break;
		default:
			panic("unknown mvSysclk\n");
		}
	}

#undef MHz

}

static int
kirkwood_clkgating(struct marvell_attach_args *mva)
{
	uint32_t val;
	int i;

	for (i = 0; i < __arraycount(clkgatings); i++) {
		if (clkgatings[i].offset == mva->mva_offset) {
			val = read_mlmbreg(MVSOC_MLMB_CLKGATING);
			if ((val & clkgatings[i].bits) == clkgatings[i].bits)
				/* Clock enabled */
				return 0;
			return 1;
		}
	}
	/* Clock Gating not support */
	return 0;
}
