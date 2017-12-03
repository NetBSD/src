/*	$NetBSD: dove.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: dove.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $");

#define _INTR_PRIVATE

#include "mvsocgpp.h"
#include "mvsocpmu.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/intr.h>

#include <arm/cpufunc.h>
#include <arm/pic/picvar.h>
#include <arm/pic/picvar.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsocpmuvar.h>
#include <arm/marvell/dovereg.h>

#include <dev/marvell/marvellreg.h>


#define read_dbreg	read_mlmbreg
#define write_dbreg	write_mlmbreg
#if NMVSOCPMU > 0
#define READ_PMUREG(sc, o)	\
		bus_space_read_4((sc)->sc_iot, (sc)->sc_pmch, (o))
#define WRITE_PMUREG(sc, o, v)	\
		bus_space_write_4((sc)->sc_iot, (sc)->sc_pmch, (o), (v))
#else
vaddr_t pmu_base = -1;
#define READ_PMUREG(sc, o)	(*(volatile uint32_t *)(pmu_base + (o)))
#endif

static void dove_intr_init(void);

static void dove_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void dove_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void dove_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void dove_pic_source_name(struct pic_softc *, int, char *, size_t);

static int dove_find_pending_irqs(void);

static void dove_getclks(bus_addr_t);
static int dove_clkgating(struct marvell_attach_args *);

#if NMVSOCPMU > 0
struct dove_pmu_softc {
	struct mvsocpmu_softc sc_mvsocpmu_sc;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_pmch;	/* Power Management Core handler */
	bus_space_handle_t sc_pmh;	/* Power Management handler */

	int sc_xpratio;
	int sc_dpratio;
};
static int dove_pmu_match(device_t, struct cfdata *, void *);
static void dove_pmu_attach(device_t, device_t, void *);
static int dove_pmu_intr(void *);
static int dove_tm_val2uc(int);
static int dove_tm_uc2val(int);
static int dove_dfs_slow(struct dove_pmu_softc *, bool);

CFATTACH_DECL_NEW(mvsocpmu, sizeof(struct dove_pmu_softc),
    dove_pmu_match, dove_pmu_attach, NULL, NULL);
#endif


static const char * const sources[64] = {
    "Bridge(0)",       "Host2CPUDoorbell(1)","CPU2HostDoorbell(2)","NF(3)",
    "PDMA(4)",         "SPI1(5)",         "SPI0(6)",         "UART0(7)",
    "UART1(8)",        "UART2(9)",        "UART3(10)",       "TWSI(11)",
    "GPIO7_0(12)",     "GPIO15_8(13)",    "GPIO23_16(14)",   "PEX0_Err(15)",
    "PEX0_INT(16)",    "PEX1_Err(17)",    "PEX1_INT(18)",    "Audio0_INT(19)",
    "Audio0_Err(20)",  "Audio1_INT(21)",  "Audio1_Err(22)",  "USBBr(23)",
    "USB0Cnt(24)",     "USB1Cnt(25)",     "GbERx(26)",       "GbETx(27)",
    "GbEMisc(28)",     "GbESum(29)",      "GbEErr(30)",      "SecurityInt(31)",

    "AC97(32)",        "PMU(33)",         "CAM(34)",         "SD0(35)",
    "SD1(36)",        "SD0_wakeup_Int(37)","SD1_wakeup_Int(38)","XOR0_DMA0(39)",
    "XOR0_DMA1(40)",   "XOR0Err(41)",     "XOR1_DMA0(42)",   "XOR1_DMA1(43)",
    "XOR1Err(44)",     "IRE_DCON(45)",    "LCD1(46)",        "LCD0(47)",
    "GPU(48)",         "Reserved(49)",    "Reserved_18(50)", "Vmeta(51)",
    "Reserved_20(52)", "Reserved_21(53)", "SSPTimer(54)",    "SSPInt(55)",
    "MemoryErr(56)", "DwnstrmExclTrn(57)","UpstrmAddrErr(58)","SecurityErr(59)",
    "GPIO_31_24(60)",  "HighGPIO(61)",    "SATAInt(62)",     "Reserved_31(63)"
};

static struct pic_ops dove_picops = {
	.pic_unblock_irqs = dove_pic_unblock_irqs,
	.pic_block_irqs = dove_pic_block_irqs,
	.pic_establish_irq = dove_pic_establish_irq,
	.pic_source_name = dove_pic_source_name,
};
static struct pic_softc dove_pic = {
	.pic_ops = &dove_picops,
	.pic_maxsources = 64,
	.pic_name = "dove",
};

static struct {
	bus_size_t offset;
	uint32_t bits;
} clkgatings[]= {
	{ DOVE_USB0_BASE,	(1 << 0) },
	{ DOVE_USB1_BASE,	(1 << 1) },
	{ DOVE_GBE_BASE,	(1 << 2) | (1 << 30) },
	{ DOVE_SATAHC_BASE,	(1 << 3) },
	{ MVSOC_PEX_BASE,	(1 << 4) },
	{ DOVE_PEX1_BASE,	(1 << 5) },
	{ DOVE_SDHC0_BASE,	(1 << 8) },
	{ DOVE_SDHC1_BASE,	(1 << 9) },
	{ DOVE_NAND_BASE,	(1 << 10) },
	{ DOVE_CAMERA_BASE,	(1 << 11) },
	{ DOVE_AUDIO0_BASE,	(1 << 12) },
	{ DOVE_AUDIO1_BASE,	(1 << 13) },
	{ DOVE_CESA_BASE,	(1 << 15) },
#if 0
	{ PDMA, (1 << 22) },	/* PdmaEnClock */
#endif
	{ DOVE_XORE_BASE,	(1 << 23) | (1 << 24) },
};


/*
 * dove_bootstrap:
 *
 *	Initialize the rest of the Dove dependencies, making it
 *	ready to handle interrupts from devices.
 *	And clks, PMU.
 */
void
dove_bootstrap(bus_addr_t iobase)
{

	/* disable all interrupts */
	write_dbreg(DOVE_DB_MIRQIMR, 0);
	write_dbreg(DOVE_DB_SMIRQIMR, 0);

	/* disable all bridge interrupts */
	write_mlmbreg(MVSOC_MLMB_MLMBIMR, 0);

	mvsoc_intr_init = dove_intr_init;

#if NMVSOCGPP > 0
	/*
	 * 64 General Purpose Port I/O (GPIO [63:0]) and
	 * an additional eight General Purpose Outputs (GPO [71:64]).
	 */
	gpp_npins = 72;
	gpp_irqbase = 96;	/* Main(32) + Second Main(32) + Bridge(32) */
#endif

	dove_getclks(iobase);

	mvsoc_clkgating = dove_clkgating;
#if NMVSOCPMU == 0
	pmu_base = iobase + DOVE_PMU_BASE;
#endif
}

static void
dove_intr_init(void)
{
	extern struct pic_softc mvsoc_bridge_pic;
	void *ih __diagused;

	pic_add(&dove_pic, 0);

	pic_add(&mvsoc_bridge_pic, 64);
	ih = intr_establish(DOVE_IRQ_BRIDGE, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &mvsoc_bridge_pic);
	KASSERT(ih != NULL);

	find_pending_irqs = dove_find_pending_irqs;
}

/* ARGSUSED */
static void
dove_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	const size_t reg = DOVE_DB_MIRQIMR
	   + irqbase * (DOVE_DB_SMIRQIMR - DOVE_DB_MIRQIMR) / 32;

	KASSERT(irqbase < 64);
	write_dbreg(reg, read_dbreg(reg) | irq_mask);
}

/* ARGSUSED */
static void
dove_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
			uint32_t irq_mask)
{
	const size_t reg = DOVE_DB_MIRQIMR
	   + irqbase * (DOVE_DB_SMIRQIMR - DOVE_DB_MIRQIMR) / 32;

	KASSERT(irqbase < 64);
	write_dbreg(reg, read_dbreg(reg) & ~irq_mask);
}

/* ARGSUSED */
static void
dove_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
dove_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	strlcpy(buf, sources[pic->pic_irqbase + irq], len);
}

/*
 * Called with interrupts disabled
 */
static int
dove_find_pending_irqs(void)
{
	int ipl = 0;

	uint32_t cause = read_dbreg(DOVE_DB_MICR);
	uint32_t pending = read_dbreg(DOVE_DB_MIRQIMR);
	pending &= cause;
	if (pending)
		ipl |= pic_mark_pending_sources(&dove_pic, 0, pending);

	uint32_t cause2 = read_dbreg(DOVE_DB_SMICR);
	uint32_t pending2 = read_dbreg(DOVE_DB_SMIRQIMR);
	pending2 &= cause2;
	if (pending2)
		ipl |= pic_mark_pending_sources(&dove_pic, 32, pending2);

	return ipl;
}

/*
 * Clock functions
 */

static void
dove_getclks(bus_addr_t iobase)
{
	uint32_t val;

#define MHz	* 1000 * 1000

	val = *(volatile uint32_t *)(iobase + DOVE_MISC_BASE +
	    DOVE_MISC_SAMPLE_AT_RESET0);

	switch (val & 0x01800000) {
	case 0x00000000: mvTclk = 166 MHz; break;
	case 0x00800000: mvTclk = 125 MHz; break;
	default:
		panic("unknown mvTclk\n");
	}

	switch (val & 0x000001e0) {
	case 0x000000a0: mvPclk = 1000 MHz; break;
	case 0x000000c0: mvPclk =  933 MHz; break;
	case 0x000000e0: mvPclk =  933 MHz; break;
	case 0x00000100: mvPclk =  800 MHz; break;
	case 0x00000120: mvPclk =  800 MHz; break;
	case 0x00000140: mvPclk =  800 MHz; break;
	case 0x00000160: mvPclk = 1067 MHz; break;
	case 0x00000180: mvPclk =  667 MHz; break;
	case 0x000001a0: mvPclk =  533 MHz; break;
	case 0x000001c0: mvPclk =  400 MHz; break;
	case 0x000001e0: mvPclk =  333 MHz; break;
	default:
		panic("unknown mvPclk\n");
	}

	switch (val & 0x0000f000) {
	case 0x00000000: mvSysclk = mvPclk /  1; break;
	case 0x00002000: mvSysclk = mvPclk /  2; break;
	case 0x00004000: mvSysclk = mvPclk /  3; break;
	case 0x00006000: mvSysclk = mvPclk /  4; break;
	case 0x00008000: mvSysclk = mvPclk /  5; break;
	case 0x0000a000: mvSysclk = mvPclk /  6; break;
	case 0x0000c000: mvSysclk = mvPclk /  7; break;
	case 0x0000e000: mvSysclk = mvPclk /  8; break;
	case 0x0000f000: mvSysclk = mvPclk / 10; break;
	}

#undef MHz

}

static int
dove_clkgating(struct marvell_attach_args *mva)
{
	uint32_t val;
	int i;

#if NMVSOCPMU > 0
	struct dove_pmu_softc *pmu =
	    device_private(device_find_by_xname("mvsocpmu0"));

	if (pmu == NULL)
		return 0;
#else
	KASSERT(pmu_base != -1);
#endif

	if (strcmp(mva->mva_name, "mvsocpmu") == 0)
		return 0;

	for (i = 0; i < __arraycount(clkgatings); i++) {
		if (clkgatings[i].offset == mva->mva_offset) {
			val = READ_PMUREG(pmu, DOVE_PMU_CGCR);
			if ((val & clkgatings[i].bits) == clkgatings[i].bits)
				/* Clock enabled */
				return 0;
			return 1;
		}
	}
	/* Clock Gating not support */
	return 0;
}

#if NMVSOCPMU > 0
static int
dove_pmu_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (mvsocpmu_match(parent, match, aux) == 0)
		return 0;

	if (mva->mva_offset == MVA_OFFSET_DEFAULT ||
	    mva->mva_irq == MVA_IRQ_DEFAULT)
		return 0;

	mva->mva_size = DOVE_PMU_SIZE;
	return 1;
}

static void
dove_pmu_attach(device_t parent, device_t self, void *aux)
{
	struct dove_pmu_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;
	uint32_t tdc0, cpucdc0;

	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(sc->sc_iot, mva->mva_ioh,
	    mva->mva_offset, mva->mva_size, &sc->sc_pmch))
		panic("%s: Cannot map core registers", device_xname(self));
	if (bus_space_subregion(sc->sc_iot, mva->mva_ioh,
	    mva->mva_offset + (DOVE_PMU_BASE2 - DOVE_PMU_BASE),
	    DOVE_PMU_SIZE, &sc->sc_pmh))
		panic("%s: Cannot map registers", device_xname(self));
	if (bus_space_subregion(sc->sc_iot, mva->mva_ioh,
	    mva->mva_offset + (DOVE_PMU_SRAM_BASE - DOVE_PMU_BASE),
	    DOVE_PMU_SRAM_SIZE, &sc->sc_pmh))
		panic("%s: Cannot map SRAM", device_xname(self));

	tdc0 = READ_PMUREG(sc, DOVE_PMU_TDC0R);
	tdc0 &= ~(DOVE_PMU_TDC0R_THERMAVGNUM_MASK |
	    DOVE_PMU_TDC0R_THERMREFCALCOUNT_MASK |
	    DOVE_PMU_TDC0R_THERMSELVCAL_MASK);
	tdc0 |= (DOVE_PMU_TDC0R_THERMAVGNUM_2 |
	    DOVE_PMU_TDC0R_THERMREFCALCOUNT(0xf1) |
	    DOVE_PMU_TDC0R_THERMSELVCAL(2));
	WRITE_PMUREG(sc, DOVE_PMU_TDC0R, tdc0);
	WRITE_PMUREG(sc, DOVE_PMU_TDC0R,
	    READ_PMUREG(sc, DOVE_PMU_TDC0R) | DOVE_PMU_TDC0R_THERMSOFTRESET);
	delay(1);
	WRITE_PMUREG(sc, DOVE_PMU_TDC0R,
	    READ_PMUREG(sc, DOVE_PMU_TDC0R) & ~DOVE_PMU_TDC0R_THERMSOFTRESET);
	cpucdc0 = READ_PMUREG(sc, DOVE_PMU_CPUCDC0R);
	sc->sc_xpratio = DOVE_PMU_CPUCDC0R_XPRATIO(cpucdc0);
	sc->sc_dpratio = DOVE_PMU_CPUCDC0R_DPRATIO(cpucdc0);

	sc->sc_mvsocpmu_sc.sc_iot = mva->mva_iot;

	if (bus_space_subregion(sc->sc_iot, sc->sc_pmch,
	    DOVE_PMU_TM_BASE, MVSOC_PMU_TM_SIZE, &sc->sc_mvsocpmu_sc.sc_tmh))
		panic("%s: Cannot map thermal managaer registers",
		    device_xname(self));
	sc->sc_mvsocpmu_sc.sc_uc2val = dove_tm_uc2val;
	sc->sc_mvsocpmu_sc.sc_val2uc = dove_tm_val2uc;

	mvsocpmu_attach(parent, self, aux);

	WRITE_PMUREG(sc, DOVE_PMU_PMUICR, 0);
	WRITE_PMUREG(sc, DOVE_PMU_PMUIMR, DOVE_PMU_PMUI_THERMOVERHEAT);

	marvell_intr_establish(mva->mva_irq, IPL_HIGH, dove_pmu_intr, sc);
}

static int
dove_pmu_intr(void *arg)
{
	struct dove_pmu_softc *sc = arg;
	uint32_t cause, mask;

	mask = READ_PMUREG(sc, DOVE_PMU_PMUIMR);
	cause = READ_PMUREG(sc, DOVE_PMU_PMUICR);
printf("dove pmu intr: cause 0x%x, mask 0x%x\n", cause, mask);
	WRITE_PMUREG(sc, DOVE_PMU_PMUICR, 0);
	cause &= mask;

	if (cause & DOVE_PMU_PMUI_BATTFAULT) {
printf("  Battery Falut\n");
	}
	if (cause & DOVE_PMU_PMUI_RTCALARM) {
printf("  RTC Alarm\n");
	}
	if (cause & DOVE_PMU_PMUI_THERMOVERHEAT) {
		mask |= DOVE_PMU_PMUI_THERMCOOLING;
		if (dove_dfs_slow(sc, true) == 0)
			mask &= ~DOVE_PMU_PMUI_THERMOVERHEAT;
		WRITE_PMUREG(sc, DOVE_PMU_PMUIMR, mask);
	}
	if (cause & DOVE_PMU_PMUI_THERMCOOLING) {
		mask |= DOVE_PMU_PMUI_THERMOVERHEAT;
		if (dove_dfs_slow(sc, false) == 0)
			mask &= ~DOVE_PMU_PMUI_THERMCOOLING;
		WRITE_PMUREG(sc, DOVE_PMU_PMUIMR, mask);
	}
	if (cause & DOVE_PMU_PMUI_DVSDONE) {
printf("  DVS Done\n");
	}
	if (cause & DOVE_PMU_PMUI_DFSDONE) {
printf("  DFS Done\n");
	}

	return 0;
}

static int
dove_tm_uc2val(int v)
{

	return (2281638 - v / 1000 * 10) / 7298;
}

static int
dove_tm_val2uc(int v)
{

	return (2281638 - 7298 * v) / 10 * 1000;
}

static int
dove_dfs_slow(struct dove_pmu_softc *sc, bool slow)
{
	uint32_t control, status, psw, pmucr;
	int rv;
uint32_t cause0, cause1, cause2;

	status = READ_PMUREG(sc, DOVE_PMU_CPUSDFSSR);
	status &= DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_MASK;
	if ((slow && status == DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_SLOW) ||
	    (!slow && status == DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_TURBO))
		return 0;

cause0 = READ_PMUREG(sc, DOVE_PMU_PMUICR);
	/*
	 * 1. Disable the CPU FIQ and IRQ interrupts.
	 */
	psw = disable_interrupts(I32_bit | F32_bit);

	/*
	 * 2. Program the new CPU Speed mode in the CPU Subsystem DFS Control
	 *    Register.
	 */
	control = READ_PMUREG(sc, DOVE_PMU_CPUSDFSCR);
	if (slow) {
		control |= DOVE_PMU_CPUSDFSCR_CPUSLOWEN;
		control |= DOVE_PMU_CPUSDFSCR_CPUL2CR(sc->sc_dpratio);
	} else {
		control &= ~DOVE_PMU_CPUSDFSCR_CPUSLOWEN;
		control |= DOVE_PMU_CPUSDFSCR_CPUL2CR(sc->sc_xpratio);
	}
	WRITE_PMUREG(sc, DOVE_PMU_CPUSDFSCR, control);

	/*
	 * 3. Enable the <DFSDone> field in the PMU Interrupts Mask Register
	 *    to wake up the CPU when the DFS procedure has been completed.
	 */
	WRITE_PMUREG(sc, DOVE_PMU_PMUIMR,
	    READ_PMUREG(sc, DOVE_PMU_PMUIMR) | DOVE_PMU_PMUI_DFSDONE);

	/*
	 * 4. Set the <MaskFIQ> and <MaskIRQ> field in the PMU Control Register.
	 *    The PMU masks the main interrupt pins of the Interrupt Controller
	 *    (FIQ and IRQ) from, so that they cannot be asserted to the CPU
	 *    core.
	 */
	pmucr = bus_space_read_4(sc->sc_iot, sc->sc_pmh, DOVE_PMU_PMUCR);
cause1 = READ_PMUREG(sc, DOVE_PMU_PMUICR);
	bus_space_write_4(sc->sc_iot, sc->sc_pmh, DOVE_PMU_PMUCR,
	    pmucr | DOVE_PMU_PMUCR_MASKFIQ | DOVE_PMU_PMUCR_MASKIRQ);

	/*
	 * 5. Set the <DFSEn> field in the CPU Subsystem DFS Control Register.
	 */
	WRITE_PMUREG(sc, DOVE_PMU_CPUSDFSCR,
	    READ_PMUREG(sc, DOVE_PMU_CPUSDFSCR) | DOVE_PMU_CPUSDFSCR_DFSEN);

	/*
	 * 6. Use the WFI instruction (Wait for Interrupt), to place the CPU
	 *    in Sleep mode.
	 */
cause2 = READ_PMUREG(sc, DOVE_PMU_PMUICR);
	__asm("wfi");

	status = READ_PMUREG(sc, DOVE_PMU_CPUSDFSSR);
	status &= DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_MASK;
	if ((slow && status == DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_SLOW) ||
	    (!slow && status == DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_TURBO)) {
		rv = 0;
		printf("DFS changed to %s\n", slow ? "slow" : "turbo");
	} else {
		rv = 1;
		printf("DFS failed to %s\n", slow ? "slow" : "turbo");
	}

	bus_space_write_4(sc->sc_iot, sc->sc_pmh, DOVE_PMU_PMUCR, pmucr);
	restore_interrupts(psw);
printf("causes: 0x%x -> 0x%x -> 0x%x\n", cause0, cause1, cause2);

	return rv;
}
#endif
