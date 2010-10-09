/*	$NetBSD: kirkwood.c,v 1.1.2.2 2010/10/09 03:31:40 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: kirkwood.c,v 1.1.2.2 2010/10/09 03:31:40 yamt Exp $");

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

static void kirkwood_pic_unblock_low_irqs(struct pic_softc *, size_t, uint32_t);
static void kirkwood_pic_unblock_high_irqs(struct pic_softc *, size_t,
					   uint32_t);
static void kirkwood_pic_block_low_irqs(struct pic_softc *, size_t, uint32_t);
static void kirkwood_pic_block_high_irqs(struct pic_softc *, size_t, uint32_t);
static int kirkwood_pic_find_pending_high_irqs(struct pic_softc *);
static void kirkwood_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void kirkwood_pic_source_name(struct pic_softc *, int, char *, size_t);

static int kirkwood_find_pending_irqs(void);

static const char * const sources[64] = {
    "MainHighSum(0)",  "Bridge(1)",       "Host2CPU DB(2)",  "CPU2Host DB(3)",
    "Reserved_4(4)",   "Xor0Chan0(5)",    "Xor0Chan1(6)",    "Xor1Chan0(7)",
    "Xor1Chan1(8)",    "PEX0INT(9)",      "Reserved(10)",    "GbE0Sum(11)",
    "GbE0Rx(12)",      "GbE0Tx(13)",      "GbE0Misc(14)",    "GbE1Sum(15)",
    "GbE1Rx(16)",      "GbE1Tx(17)",      "GbE1Misc(18)",    "USB0Cnt(19)",
    "Reserved(20)",    "Sata(21)",        "SecurityInt(22)", "SPIInt(23)",
    "AudioINT(24)",    "Reserved(25)",    "TS0Int(26)",      "Reserved(27)",
    "SDIOInt(28)",     "TWSI(29)",        "AVBInt(30)",      "TDMInt(31)"

    "Reserved(32)",    "Uart0Int(33)",    "Uart1Int(34)",    "GPIOLo7_0(35)"
    "GPIOLo8_15(36)",  "GPIOLo16_23(37)", "GPIOLo24_31(38)", "GPIOHi7_0(39)"
    "GPIOHi8_15(40)",  "GPIOHi16_23(41)", "XOR0Err(42)",     "XOR1Err(43)"
    "PEX0Err(44)",     "Reserved(45)",    "GbE0Err(46)",     "GbE1Err(47)"
    "USBErr(48)",      "SecurityErr(49)", "AudioErr(50)",    "Reserved(51)"
    "Reserved(52)",    "RTCInt(53)",      "Reserved(54)",    "Reserved(55)"
    "Reserved(56)",    "Reserved(57)",    "Reserved(58)",    "Reserved(59)"
    "Reserved(60)",    "Reserved(61)",    "Reserved(62)",    "Reserved(63)"
};

static struct pic_ops kirkwood_picops_low = {
	.pic_unblock_irqs = kirkwood_pic_unblock_low_irqs,
	.pic_block_irqs = kirkwood_pic_block_low_irqs,
	.pic_establish_irq = kirkwood_pic_establish_irq,
	.pic_source_name = kirkwood_pic_source_name,
};
static struct pic_ops kirkwood_picops_high = {
	.pic_unblock_irqs = kirkwood_pic_unblock_high_irqs,
	.pic_block_irqs = kirkwood_pic_block_high_irqs,
	.pic_find_pending_irqs = kirkwood_pic_find_pending_high_irqs,
	.pic_establish_irq = kirkwood_pic_establish_irq,
	.pic_source_name = kirkwood_pic_source_name,
};
static struct pic_softc kirkwood_pic_low = {
	.pic_ops = &kirkwood_picops_low,
	.pic_maxsources = 32,
	.pic_name = "kirkwood_low",
};
static struct pic_softc kirkwood_pic_high = {
	.pic_ops = &kirkwood_picops_high,
	.pic_maxsources = 32,
	.pic_name = "kirkwood_high",
};


/*
 * kirkwood_intr_bootstrap:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
kirkwood_intr_bootstrap(void)
{
	extern void (*mvsoc_intr_init)(void);

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
	}
	gpp_irqbase = 96;	/* Main Low(32) + High(32) + Bridge(32) */
#endif
}

static void
kirkwood_intr_init(void)
{
	extern struct pic_softc mvsoc_bridge_pic;
	void *ih;

	pic_add(&kirkwood_pic_low, 0);

	pic_add(&kirkwood_pic_high, 32);
	ih = intr_establish(KIRKWOOD_IRQ_HIGH, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &kirkwood_pic_high);
	KASSERT(ih != NULL);

	pic_add(&mvsoc_bridge_pic, 64);
	ih = intr_establish(KIRKWOOD_IRQ_BRIDGE, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &mvsoc_bridge_pic);
	KASSERT(ih != NULL);

	find_pending_irqs = kirkwood_find_pending_irqs;
}

/* ARGSUSED */
static void
kirkwood_pic_unblock_low_irqs(struct pic_softc *pic, size_t irqbase,
			      uint32_t irq_mask)
{

	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR,
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR) | irq_mask);
}

/* ARGSUSED */
static void
kirkwood_pic_unblock_high_irqs(struct pic_softc *pic, size_t irqbase,
			       uint32_t irq_mask)
{

	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR,
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR) | irq_mask);
}

/* ARGSUSED */
static void
kirkwood_pic_block_low_irqs(struct pic_softc *pic, size_t irqbase,
			    uint32_t irq_mask)
{

	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR,
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR) & ~irq_mask);
}

/* ARGSUSED */
static void
kirkwood_pic_block_high_irqs(struct pic_softc *pic, size_t irqbase,
			     uint32_t irq_mask)
{

	write_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR,
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR) & ~irq_mask);
}

static int
kirkwood_pic_find_pending_high_irqs(struct pic_softc *pic)
{
	uint32_t pending;

	pending = read_mlmbreg(KIRKWOOD_MLMB_MICHR) &
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMHR);
	if (pending == 0)
		return 0;
	pic_mark_pending_sources(pic, 0, pending);
	return 1;
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
	uint32_t pending;

	pending = read_mlmbreg(KIRKWOOD_MLMB_MICLR) &
	    read_mlmbreg(KIRKWOOD_MLMB_MIRQIMLR);
	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&kirkwood_pic_low, 0, pending);
}

/*
 * Clock functions
 */

void
kirkwood_getclks(bus_addr_t iobase)
{
	uint32_t reg;
	uint16_t model;

#define MHz	* 1000 * 1000

	model = mvsoc_model();
	if (model == MARVELL_KIRKWOOD_88F6281)
		mvTclk = 200 MHz;
	else		/* 166MHz */
		mvTclk = 166666667;

	reg = *(volatile uint32_t *)(iobase + KIRKWOOD_MPP_BASE +
	    KIRKWOOD_MPP_SAMPLE_AT_RESET);
	if (model == MARVELL_KIRKWOOD_88F6180) {
		switch (reg & 0x0000001c) {
		case 0x00000014: mvPclk =  600 MHz;
		case 0x00000018: mvPclk =  800 MHz;
		default:
			panic("unknown mvPclk\n");
		}
		mvSysclk = 200 MHz;
	} else {
		switch (reg & 0x0040001a) {
		case 0x00000008: mvPclk =  600 MHz; break;
		case 0x00400008: mvPclk =  800 MHz; break;
		case 0x0040000a: mvPclk = 1000 MHz; break;
		case 0x00000012: mvPclk = 1200 MHz; break;
		case 0x00000018: mvPclk = 1200 MHz; break;
		default:
			panic("unknown mvPclk\n");
		}

		switch (reg & 0x000001e0) {
		case 0x00000060: mvSysclk = mvPclk * 2 / 5; break;
		case 0x00000080: mvSysclk = mvPclk * 1 / 3; break;
		case 0x000000c0: mvSysclk = mvPclk * 1 / 4; break;
		default:
			panic("unknown mvSysclk\n");
		}
	}

#undef MHz

}
