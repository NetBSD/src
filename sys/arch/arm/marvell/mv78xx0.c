/*	$NetBSD: mv78xx0.c,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: mv78xx0.c,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $");

#define _INTR_PRIVATE

#include "mvsocgpp.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>
#include <arm/pic/picvar.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mv78xx0reg.h>

#include <dev/marvell/marvellreg.h>

#define MV78XX0_ICI_MICR(g)	(MV78XX0_ICI_MICLR + ((g) << 2))
#define MV78XX0_ICI_IRQIMR(g)	(MV78XX0_ICI_IRQIMLR + ((g) << 2))

static void mv78xx0_intr_init(void);

static void mv78xx0_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void mv78xx0_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void mv78xx0_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void mv78xx0_pic_source_name(struct pic_softc *, int, char *, size_t);

static int mv78xx0_find_pending_irqs(void);

static const char * const sources[64] = {
    "ErrSum(0)",       "SPI(1)",          "TWSI0(2)",        "TWSI1(3)",
    "IDMA0(4)",        "IDMA1(5)",        "IDMA2(6)",        "IDMA3(7)",
    "Timer0(8)",       "Timer1(9)",       "Timer2(10)",      "Timer3(11)",
    "UART0(12)",       "UART1(13)",       "UART2(14)",       "UART3(15)",
    "USB0(16)",        "USB1(17)",        "USB2(18)",        "Crypto(19)",
    "Reserved(20)",    "Reserved(21)",    "XOR0(22)",        "XOR1(23)",
    "Reserved(24)",    "Reserved(25)",    "SATA(26)",        "TDMI_INT(27)",
    "Reserved(28)",    "Reserved(29)",    "Reserved(30)",    "Reserved(31)"

    "PEX00INTA(32)",   "PEX01INTA(33)",   "PEX02INTA(34)",   "PEX03INTA(35)"
    "PEX10INTA(36)",   "PEX11INTA(37)",   "PEX12INTA(38)",   "PEX13INTA(39)"
    "GE00Sum(40)",     "GE00Rx(41)",      "GE00Tx(42)",      "GE00Misc(43)"
    "GE01Sum(44)",     "GE01Rx(45)",      "GE01Tx(46)",      "GE01Misc(47)"
    "GE10Sum(48)",     "GE10Rx(49)",      "GE10Tx(50)",      "GE10Misc(51)"
    "GE11Sum(52)",     "GE11Rx(53)",      "GE11Tx(54)",      "GE11Misc(55)"
    "GPIO0_7(56)",     "GPIO8_15(57)",    "GPIO16_23(58)",   "GPIO24_31(59)"
    "DB_INT(60)",      "DB_OUT(61)",      "Reserved(62)",    "Reserved(63)"
};

static struct pic_ops mv78xx0_picops = {
	.pic_unblock_irqs = mv78xx0_pic_unblock_irqs,
	.pic_block_irqs = mv78xx0_pic_block_irqs,
	.pic_establish_irq = mv78xx0_pic_establish_irq,
	.pic_source_name = mv78xx0_pic_source_name,
};
static struct pic_softc mv78xx0_pic = {
	.pic_ops = &mv78xx0_picops,
	.pic_maxsources = 64,
	.pic_name = "mv78xx0_pic",
};


/*
 * mv78xx0_intr_bootstrap:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
mv78xx0_intr_bootstrap(void)
{
	extern void (*mvsoc_intr_init)(void);

	/* disable all interrupts */
	write_mlmbreg(MV78XX0_ICI_IRQIMER, 0);
	write_mlmbreg(MV78XX0_ICI_IRQIMLR, 0);
	write_mlmbreg(MV78XX0_ICI_IRQIMHR, 0);

	/* disable all bridge interrupts */
	write_mlmbreg(MVSOC_MLMB_MLMBIMR, 0);

	mvsoc_intr_init = mv78xx0_intr_init;

	gpp_npins = 32;
	gpp_irqbase = 64;	/* Main Low(32) + High(32) */
}

static void
mv78xx0_intr_init(void)
{

	pic_add(&mv78xx0_pic, 0);

	find_pending_irqs = mv78xx0_find_pending_irqs;
}

/* ARGSUSED */
static void
mv78xx0_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
			 uint32_t irq_mask)
{
	const size_t group = irqbase / 32;

	write_mlmbreg(MV78XX0_ICI_IRQIMR(group),
	    read_mlmbreg(MV78XX0_ICI_IRQIMR(group)) | irq_mask);
}

/* ARGSUSED */
static void
mv78xx0_pic_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	const size_t group = irqbase / 32;

	write_mlmbreg(MV78XX0_ICI_IRQIMR(group),
	    read_mlmbreg(MV78XX0_ICI_IRQIMR(group)) & ~irq_mask);
}

/* ARGSUSED */
static void
mv78xx0_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	uint32_t mlmbim;

	/* Also enable MbusL-Mbus Bridge Interrupt Mask, if irq is TimerX. */
	if (is->is_irq == MV78XX0_IRQ_TIMER0 ||
	    is->is_irq == MV78XX0_IRQ_TIMER1 ||
	    is->is_irq == MV78XX0_IRQ_TIMER2 ||
	    is->is_irq == MV78XX0_IRQ_TIMER3) {
		mlmbim = read_mlmbreg(MVSOC_MLMB_MLMBIMR);
		mlmbim |= TIMER_IRQ2MLMBIMR(is->is_irq);
		write_mlmbreg(MVSOC_MLMB_MLMBIMR, mlmbim);
	}
}

static void
mv78xx0_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	strlcpy(buf, sources[pic->pic_irqbase + irq], len);
}

/*
 * Called with interrupts disabled
 */
static int
mv78xx0_find_pending_irqs(void)
{
	uint32_t pending;
	int ipl = 0;

	pending = read_mlmbreg(MV78XX0_ICI_MICR(0)) &
	    read_mlmbreg(MV78XX0_ICI_IRQIMR(0));
	if (pending != 0)
		ipl = pic_mark_pending_sources(&mv78xx0_pic, 0, pending);

	pending = read_mlmbreg(MV78XX0_ICI_MICR(1)) &
	    read_mlmbreg(MV78XX0_ICI_IRQIMR(1));
	if (pending != 0)
		ipl |= pic_mark_pending_sources(&mv78xx0_pic, 32, pending);

	return ipl;
}

/*
 * Clock functions
 */

void
mv78xx0_getclks(bus_addr_t iobase)
{
	const static int sys2cpu_clk_ratio_m[] =	/* Mul constant */
	    { 1, 3, 2, 5, 3, 7, 4, 9, 5, 1, 6 };
	const static int sys2cpu_clk_ratio_n[] =	/* Div constant */
	    { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1 };
	uint32_t reg;
	int x;

#define MHz	* 1000 * 1000

	reg = *(volatile uint32_t *)(iobase + MV78XX0_SAMPLE_AT_RESET_HIGH);
	switch (reg & 0x180) {
	case 0x000: mvTclk = 166666667; break;
	case 0x080: mvTclk =   200 MHz; break;
	default:    mvTclk =   200 MHz; break;
	}

	reg = *(volatile uint32_t *)(iobase + MV78XX0_SAMPLE_AT_RESET_LOW);

	switch (reg & 0x0e0) {
	case 0x020: mvSysclk =   200 MHz; break;
	case 0x040: mvSysclk = 266666667; break;
	case 0x060: mvSysclk = 333333334; break;
	case 0x080: mvSysclk =   400 MHz; break;
	case 0x0a0: mvSysclk =   250 MHz; break;
	case 0x0c0: mvSysclk =   300 MHz; break;
	default:    mvSysclk = 266666667; break;
	}

	x = (reg & 0xf00) >> 8;
	mvPclk = sys2cpu_clk_ratio_m[x] * mvSysclk / sys2cpu_clk_ratio_n[x];

#undef MHz

}
