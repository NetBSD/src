/*	$NetBSD: orion.c,v 1.4.6.2 2017/12/03 11:35:54 jdolecek Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: orion.c,v 1.4.6.2 2017/12/03 11:35:54 jdolecek Exp $");

#define _INTR_PRIVATE

#include "mvsocgpp.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>
#include <arm/pic/picvar.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/orionreg.h>

#include <dev/marvell/marvellreg.h>


static void orion_intr_init(void);

static void orion_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void orion_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void orion_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void orion_pic_source_name(struct pic_softc *, int, char *, size_t);

static int orion_find_pending_irqs(void);

static void orion_getclks(vaddr_t);

static const char * const sources[64] = {
    "Bridge(0)",       "Host2CPU DB(1)",  "CPU2Host DB(2)",  "UART0(3)",
    "UART1(4)",        "TWSI(5)",         "GPIO7_0(6)",      "GPIO15_8(7)",
    "GPIO23_16(8)",    "GPIO31_24(9)",    "PEX0Err(10)",     "PEX0INT(11)",
    "PEX1Err/USBCnt1", "PEX1INT(13)",     "DEVErr(14)",      "PCIErr(15)",
    "USBBr(16)",       "USBCnt0(17)",     "GbERx(18)",       "GbETx(19)",
    "GbEMisc(20)",     "GbESum(21)",      "GbEErr(22)",      "DMAErr(23)",
    "IDMA0(24)",       "IDMA1(25)",       "IDMA2(26)",       "IDMA3(27)",
    "SecIntr(28)",     "SataIntr(29)",    "XOR0(30)",        "XOR1(31)"
};

static struct pic_ops orion_picops = {
	.pic_unblock_irqs = orion_pic_unblock_irqs,
	.pic_block_irqs = orion_pic_block_irqs,
	.pic_establish_irq = orion_pic_establish_irq,
	.pic_source_name = orion_pic_source_name,
};
static struct pic_softc orion_pic = {
	.pic_ops = &orion_picops,
	.pic_maxsources = 32,
	.pic_name = "orion_pic",
};


/*
 * orion_bootstrap:
 *
 *	Initialize the rest of the Orion dependences, making it
 *	ready to handle interrupts from devices.
 */
void
orion_bootstrap(vaddr_t iobase)
{

	/* disable all interrupts */
	write_mlmbreg(ORION_MLMB_MIRQIMR, 0);

	/* disable all bridge interrupts */
	write_mlmbreg(MVSOC_MLMB_MLMBIMR, 0);

	mvsoc_intr_init = orion_intr_init;

#if NMVSOCGPP > 0
	gpp_npins = 32;
	gpp_irqbase = 64;	/* Main(32) + Bridge(32) */
#endif

	orion_getclks(iobase);
}

static void
orion_intr_init(void)
{
	extern struct pic_softc mvsoc_bridge_pic;
	void *ih __diagused;

	pic_add(&orion_pic, 0);

	pic_add(&mvsoc_bridge_pic, 32);
	ih = intr_establish(ORION_IRQ_BRIDGE, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &mvsoc_bridge_pic);
	KASSERT(ih != NULL);

	find_pending_irqs = orion_find_pending_irqs;
}

/* ARGSUSED */
static void
orion_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{

	write_mlmbreg(ORION_MLMB_MIRQIMR,
	    read_mlmbreg(ORION_MLMB_MIRQIMR) | irq_mask);
}

/* ARGSUSED */
static void
orion_pic_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{

	write_mlmbreg(ORION_MLMB_MIRQIMR,
	    read_mlmbreg(ORION_MLMB_MIRQIMR) & ~irq_mask);
}

/* ARGSUSED */
static void
orion_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
orion_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	strlcpy(buf, sources[pic->pic_irqbase + irq], len);
}

/*
 * Called with interrupts disabled
 */
static int
orion_find_pending_irqs(void)
{
	uint32_t pending;

	pending =
	    read_mlmbreg(ORION_MLMB_MICR) & read_mlmbreg(ORION_MLMB_MIRQIMR);
	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&orion_pic, 0, pending);
}

/*
 * Clock functions
 */

static void
orion_getclks(vaddr_t iobase)
{
	static const struct {
		int armddrclkval;
		uint32_t pclk;
		uint32_t sysclk;
	} sysclktbl[] = {
		{ ORION_PMISMPL_ARMDDRCLK_333_167, 333000000, 166666667 },
		{ ORION_PMISMPL_ARMDDRCLK_400_200, 400000000, 200000000 },
		{ ORION_PMISMPL_ARMDDRCLK_400_133, 400000000, 133333334 },
		{ ORION_PMISMPL_ARMDDRCLK_500_167, 500000000, 166666667 },
		{ ORION_PMISMPL_ARMDDRCLK_533_133, 533000000, 133333334 },
		{ ORION_PMISMPL_ARMDDRCLK_600_200, 600000000, 200000000 },
		{ ORION_PMISMPL_ARMDDRCLK_667_167, 667000000, 166666667 },
		{ ORION_PMISMPL_ARMDDRCLK_800_200, 800000000, 200000000 },
		{ ORION_PMISMPL_ARMDDRCLK_480_160, 480000000, 160000000 },
		{ ORION_PMISMPL_ARMDDRCLK_550_183, 550000000, 183333334 },
		{ ORION_PMISMPL_ARMDDRCLK_525_175, 525000000, 175000000 },
		{ ORION_PMISMPL_ARMDDRCLK_466_233, 466000000, 233000000 },
		{ ORION_PMISMPL_ARMDDRCLK_500_250, 500000000, 250000000 },
		{ ORION_PMISMPL_ARMDDRCLK_533_266, 533000000, 266000000 },
		{ ORION_PMISMPL_ARMDDRCLK_600_300, 600000000, 300000000 },
		{ ORION_PMISMPL_ARMDDRCLK_450_150, 450000000, 150000000 },
		{ ORION_PMISMPL_ARMDDRCLK_533_178, 533000000, 178000000 },
		{ ORION_PMISMPL_ARMDDRCLK_575_192, 575000000, 192000000 },
		{ ORION_PMISMPL_ARMDDRCLK_700_175, 700000000, 175000000 },
		{ ORION_PMISMPL_ARMDDRCLK_733_183, 733000000, 183333334 },
		{ ORION_PMISMPL_ARMDDRCLK_750_187, 750000000, 187000000 },
		{ ORION_PMISMPL_ARMDDRCLK_775_194, 775000000, 194000000 },
		{ ORION_PMISMPL_ARMDDRCLK_500_125, 500000000, 125000000 },
		{ ORION_PMISMPL_ARMDDRCLK_500_100, 500000000, 100000000 },
		{ ORION_PMISMPL_ARMDDRCLK_600_150, 600000000, 150000000 },
	};
	uint32_t reg, armddrclk, tclk;
	uint16_t model;
	int armddrclk_shift, tclk_shift, i;

	model = mvsoc_model();
	if (model == MARVELL_ORION_1_88F1181 ||
	    model == MARVELL_ORION_2_88F1281) {
		armddrclk_shift = 6;
		tclk_shift = 10;
	} else {
		armddrclk_shift = 4;
		tclk_shift = 8;
	}

	reg = *(volatile uint32_t *)(iobase + ORION_PMI_BASE +
	    ORION_PMI_SAMPLE_AT_RESET);
	armddrclk = (reg >> armddrclk_shift) & ORION_PMISMPL_ARMDDRCLK_MASK;
	if (model == PCI_PRODUCT_MARVELL_88F5281)
		if (reg & ORION_PMISMPL_ARMDDRCLK_H_MASK)
			armddrclk |= 0x00000010;	/* set to bit4 */
	for (i = 0; i < __arraycount(sysclktbl); i++)
		if (armddrclk == sysclktbl[i].armddrclkval) {
			mvPclk = sysclktbl[i].pclk;
			mvSysclk = sysclktbl[i].sysclk;
			break;
		}

	tclk = (reg >> tclk_shift) & ORION_PMISMPL_TCLK_MASK;
	switch (tclk) {
	case ORION_PMISMPL_TCLK_133:
		mvTclk = 133333333;	/* 133MHz */
		break;

	case ORION_PMISMPL_TCLK_150:
		mvTclk = 150000000;	/* 150MHz */
		break;

	case ORION_PMISMPL_TCLK_166:
		mvTclk = 166666667;	/* 166MHz */
		break;

	default:
		mvTclk = 100000000;	/* 100MHz */
		break;
	}
}
