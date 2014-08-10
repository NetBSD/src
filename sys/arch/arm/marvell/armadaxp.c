/*	$NetBSD: armadaxp.c,v 1.7.2.1 2014/08/10 06:53:52 tls Exp $	*/
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadaxp.c,v 1.7.2.1 2014/08/10 06:53:52 tls Exp $");

#define _INTR_PRIVATE

#include "opt_mvsoc.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>
#include <arm/pic/picvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/armadaxpreg.h>

#include <dev/marvell/marvellreg.h>

#define EXTRACT_XP_CPU_FREQ_FIELD(sar)	(((0x01 & (sar >> 52)) << 3) | \
					    (0x07 & (sar >> 21)))
#define EXTRACT_XP_FAB_FREQ_FIELD(sar)	(((0x01 & (sar >> 51)) << 4) | \
					    (0x0F & (sar >> 24)))
#define EXTRACT_370_CPU_FREQ_FIELD(sar)	((sar >> 11) & 0xf)
#define EXTRACT_370_FAB_FREQ_FIELD(sar)	((sar >> 15) & 0x1f)

#define	MPIC_WRITE(reg, val)		(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_handle, reg, val))
#define	MPIC_CPU_WRITE(reg, val)	(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg, val))

#define	MPIC_READ(reg)			(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_handle, reg))
#define	MPIC_CPU_READ(reg)		(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg))

#define	L2_WRITE(reg, val)		(bus_space_write_4(&mvsoc_bs_tag, \
					    l2_handle, reg, val))
#define	L2_READ(reg)			(bus_space_read_4(&mvsoc_bs_tag, \
					    l2_handle, reg))
bus_space_handle_t mpic_cpu_handle;
static bus_space_handle_t mpic_handle, l2_handle;
int l2cache_state = 0;
int iocc_state = 0;
#define read_miscreg(r)		(*(volatile uint32_t *)(misc_base + (r)))
vaddr_t misc_base;

extern void (*mvsoc_intr_init)(void);
static void armadaxp_intr_init(void);

static void armadaxp_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void armadaxp_pic_set_priority(struct pic_softc *, int);

static int armadaxp_find_pending_irqs(void);
static void armadaxp_pic_block_irq(struct pic_softc *, size_t);
void armadaxp_io_coherency_init(void);
int armadaxp_l2_init(bus_addr_t);

struct vco_freq_ratio {
	uint8_t	vco_cpu;	/* VCO to CLK0(CPU) clock ratio */
	uint8_t	vco_l2c;	/* VCO to NB(L2 cache) clock ratio */
	uint8_t	vco_hcl;	/* VCO to HCLK(DDR controller) clock ratio */
	uint8_t	vco_ddr;	/* VCO to DR(DDR memory) clock ratio */
};

static struct vco_freq_ratio freq_conf_table[] = {
/*00*/	{ 1, 1,	 4,  2 },
/*01*/	{ 1, 2,	 2,  2 },
/*02*/	{ 2, 2,	 6,  3 },
/*03*/	{ 2, 2,	 3,  3 },
/*04*/	{ 1, 2,	 3,  3 },
/*05*/	{ 1, 2,	 4,  2 },
/*06*/	{ 1, 1,	 2,  2 },
/*07*/	{ 2, 3,	 6,  6 },
/*08*/	{ 2, 3,	 5,  5 },
/*09*/	{ 1, 2,	 6,  3 },
/*10*/	{ 2, 4,	10,  5 },
/*11*/	{ 1, 3,	 6,  6 },
/*12*/	{ 1, 2,	 5,  5 },
/*13*/	{ 1, 3,	 6,  3 },
/*14*/	{ 1, 2,	 5,  5 },
/*15*/	{ 2, 2,	 5,  5 },
/*16*/	{ 1, 1,	 3,  3 },
/*17*/	{ 2, 5,	10, 10 },
/*18*/	{ 1, 3,	 8,  4 },
/*19*/	{ 1, 1,	 2,  1 },
/*20*/	{ 2, 3,	 6,  3 },
/*21*/	{ 1, 2,	 8,  4 },
/*22*/	{ 2, 5,	10,  5 }
};

static uint16_t clock_table_xp[] = {
	1000, 1066, 1200, 1333, 1500, 1666, 1800, 2000,
	 600,  667,  800, 1600, 2133, 2200, 2400
};
static uint16_t clock_table_370[] = {
	 400,  533,  667,  800, 1000, 1067, 1200, 1333,
	1500, 1600, 1667, 1800, 2000,  333,  600,  900,
	   0
};

static struct pic_ops armadaxp_picops = {
	.pic_unblock_irqs = armadaxp_pic_unblock_irqs,
	.pic_block_irqs = armadaxp_pic_block_irqs,
	.pic_establish_irq = armadaxp_pic_establish_irq,
	.pic_set_priority = armadaxp_pic_set_priority,
};

static struct pic_softc armadaxp_pic = {
	.pic_ops = &armadaxp_picops,
	.pic_name = "armadaxp",
};

static struct {
	bus_size_t offset;
	uint32_t bits;
} clkgatings[]= {
	{ ARMADAXP_GBE3_BASE,	(1 << 1) },
	{ ARMADAXP_GBE2_BASE,	(1 << 2) },
	{ ARMADAXP_GBE1_BASE,	(1 << 3) },
	{ ARMADAXP_GBE0_BASE,	(1 << 4) },
	{ MVSOC_PEX_BASE,	(1 << 5) },
	{ ARMADAXP_PEX01_BASE,	(1 << 6) },
	{ ARMADAXP_PEX02_BASE,	(1 << 7) },
	{ ARMADAXP_PEX03_BASE,	(1 << 8) },
	{ ARMADAXP_PEX10_BASE,	(1 << 9) },
	{ ARMADAXP_PEX11_BASE,	(1 << 10) },
	{ ARMADAXP_PEX12_BASE,	(1 << 11) },
	{ ARMADAXP_PEX13_BASE,	(1 << 12) },
#if 0
	{ NetA, (1 << 13) },
#endif
	{ ARMADAXP_SATAHC_BASE,	(1 << 14) | (1 << 15) | (1 << 29) | (1 << 30) },
	{ ARMADAXP_LCD_BASE,	(1 << 16) },
	{ ARMADAXP_SDIO_BASE,	(1 << 17) },
	{ ARMADAXP_USB1_BASE,	(1 << 19) },
	{ ARMADAXP_USB2_BASE,	(1 << 20) },
	{ ARMADAXP_PEX2_BASE,	(1 << 26) },
	{ ARMADAXP_PEX3_BASE,	(1 << 27) },
#if 0
	{ DDR, (1 << 28) },
#endif
};

/*
 * armadaxp_intr_bootstrap:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
armadaxp_intr_bootstrap(bus_addr_t pbase)
{
	int i;

	/* Map MPIC base and MPIC percpu base registers */
	if (bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_MLMB_MPIC_BASE,
	    0x500, 0, &mpic_handle) != 0)
		panic("%s: Could not map MPIC registers", __func__);
	if (bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_MLMB_MPIC_CPU_BASE,
	    0x800, 0, &mpic_cpu_handle) != 0)
		panic("%s: Could not map MPIC percpu registers", __func__);

	/* Disable all interrupts */
	for (i = 0; i < 116; i++)
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, i);

	mvsoc_intr_init = armadaxp_intr_init;
}

static void
armadaxp_intr_init(void)
{
	int ctrl;

	/* Get max interrupts */
	armadaxp_pic.pic_maxsources =
	    ((MPIC_READ(ARMADAXP_MLMB_MPIC_CTRL) >> 2) & 0x7FF);

	if (!armadaxp_pic.pic_maxsources)
		armadaxp_pic.pic_maxsources = 116;

	pic_add(&armadaxp_pic, 0);

	ctrl = MPIC_READ(ARMADAXP_MLMB_MPIC_CTRL);
	/* Enable IRQ prioritization */
	ctrl |= (1 << 0);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_CTRL, ctrl);

	find_pending_irqs = armadaxp_find_pending_irqs;
}

static void
armadaxp_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	int n;

	while (irq_mask != 0) {
		n = ffs(irq_mask) - 1;
		KASSERT(pic->pic_maxsources >= n + irqbase);
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ISE, n + irqbase);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ICM, n + irqbase);
		if ((n + irqbase) == 0)
			MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_DOORBELL_MASK,
			    0xffffffff);
		irq_mask &= ~__BIT(n);
	}
}

static void
armadaxp_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	int n;

	while (irq_mask != 0) {
		n = ffs(irq_mask) - 1;
		KASSERT(pic->pic_maxsources >= n + irqbase);
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, n + irqbase);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ISM, n + irqbase);
		irq_mask &= ~__BIT(n);
	}
}

static void
armadaxp_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	int tmp;
	KASSERT(pic->pic_maxsources >= is->is_irq);
	tmp = MPIC_READ(ARMADAXP_MLMB_MPIC_ISCR_BASE + is->is_irq * 4);
	/* Clear previous priority */
	tmp &= ~(0xf << MPIC_ISCR_SHIFT);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_ISCR_BASE + is->is_irq * 4,
	    tmp | (is->is_ipl << MPIC_ISCR_SHIFT));
}

static void
armadaxp_pic_set_priority(struct pic_softc *pic, int ipl)
{
	int ctp;

	ctp = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_CTP);
	ctp &= ~(0xf << MPIC_CTP_SHIFT);
	ctp |= (ipl << MPIC_CTP_SHIFT);
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_CTP, ctp);
}

static int
armadaxp_find_pending_irqs(void)
{
	struct intrsource *is;
	int irq;

	irq = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_IIACK) & 0x3ff;

	/* Is it a spurious interrupt ?*/
	if (irq == 0x3ff)
		return 0;
	is = armadaxp_pic.pic_sources[irq];
	if (is == NULL) {
		printf("stray interrupt: %d\n", irq);
		return 0;
	}

	armadaxp_pic_block_irq(&armadaxp_pic, irq);
	pic_mark_pending(&armadaxp_pic, irq);

	return is->is_ipl;
}

static void
armadaxp_pic_block_irq(struct pic_softc *pic, size_t irq)
{

	KASSERT(pic->pic_maxsources >= irq);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, irq);
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ISM, irq);
}

/*
 * Clock functions
 */

void
armadaxp_getclks(void)
{
	uint64_t sar_reg;
	uint8_t  sar_cpu_freq, sar_fab_freq;

	if (cputype == CPU_ID_MV88SV584X_V7)
		mvTclk = 250000000; /* 250 MHz */
	else
		mvTclk = 200000000; /* 200 MHz */

	sar_reg = (read_miscreg(ARMADAXP_MISC_SAR_HI) << 31) |
	    read_miscreg(ARMADAXP_MISC_SAR_LO);

	sar_cpu_freq = EXTRACT_XP_CPU_FREQ_FIELD(sar_reg);
	sar_fab_freq = EXTRACT_XP_FAB_FREQ_FIELD(sar_reg);

	/* Check if CPU frequency field has correct value */
	if (sar_cpu_freq >= __arraycount(clock_table_xp))
		panic("Reserved value in cpu frequency configuration field: "
		    "%d", sar_cpu_freq);

	/* Check if fabric frequency field has correct value */
	if (sar_fab_freq >= __arraycount(freq_conf_table))
		panic("Reserved value in fabric frequency configuration field: "
		    "%d", sar_fab_freq);

	/* Get CPU clock frequency */
	mvPclk = clock_table_xp[sar_cpu_freq] *
	    freq_conf_table[sar_fab_freq].vco_cpu;

	/* Get L2CLK clock frequency and use as system clock (mvSysclk) */
	mvSysclk = mvPclk / freq_conf_table[sar_fab_freq].vco_l2c;

	/* Round mvSysclk value to integer MHz */
	if (((mvPclk % freq_conf_table[sar_fab_freq].vco_l2c) * 10 /
	    freq_conf_table[sar_fab_freq].vco_l2c) >= 5)
		mvSysclk++;

	mvPclk *= 1000000;
	mvSysclk *= 1000000;

	curcpu()->ci_data.cpu_cc_freq = mvPclk;
}

void
armada370_getclks(void)
{
	uint32_t sar;
	uint8_t  cpu_freq, fab_freq;

	sar = read_miscreg(ARMADAXP_MISC_SAR_LO);
	if (sar & 0x00100000)
		mvTclk = 200000000; /* 200 MHz */
	else
		mvTclk = 166666667; /* 166 MHz */

	cpu_freq = EXTRACT_370_CPU_FREQ_FIELD(sar);
	fab_freq = EXTRACT_370_FAB_FREQ_FIELD(sar);

	/* Check if CPU frequency field has correct value */
	if (cpu_freq >= __arraycount(clock_table_370))
		panic("Reserved value in cpu frequency configuration field: "
		    "%d", cpu_freq);

	/* Check if fabric frequency field has correct value */
	if (fab_freq >= __arraycount(freq_conf_table))
		panic("Reserved value in fabric frequency configuration field: "
		    "%d", fab_freq);

	/* Get CPU clock frequency */
	mvPclk = clock_table_370[cpu_freq] *
	    freq_conf_table[fab_freq].vco_cpu;

	/* Get L2CLK clock frequency and use as system clock (mvSysclk) */
	mvSysclk = mvPclk / freq_conf_table[fab_freq].vco_l2c;

	/* Round mvSysclk value to integer MHz */
	if (((mvPclk % freq_conf_table[fab_freq].vco_l2c) * 10 /
	    freq_conf_table[fab_freq].vco_l2c) >= 5)
		mvSysclk++;

	mvPclk *= 1000000;
	mvSysclk *= 1000000;
}

/*
 * L2 Cache initialization
 */

int
armadaxp_l2_init(bus_addr_t pbase)
{
	u_int32_t reg;
	int ret;

	/* Map L2 space */
	ret = bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_L2_BASE,
	    0x1000, 0, &l2_handle);
	if (ret) {
		printf("%s: Cannot map L2 register space, ret:%d\n",
		    __func__, ret);
		return (-1);
	}

	/* Set L2 policy */
	reg = L2_READ(ARMADAXP_L2_AUX_CTRL);
	reg &= ~(L2_WBWT_MODE_MASK);
	reg &= ~(L2_REP_STRAT_MASK);
	reg |= L2_REP_STRAT_SEMIPLRU;
	L2_WRITE(ARMADAXP_L2_AUX_CTRL, reg);

	/* Invalidate L2 cache */
	L2_WRITE(ARMADAXP_L2_INV_WAY, L2_ALL_WAYS);

	/* Clear pending L2 interrupts */
	L2_WRITE(ARMADAXP_L2_INT_CAUSE, 0x1ff);

	/* Enable Cache and TLB maintenance broadcast */
	__asm__ __volatile__ ("mrc p15, 1, %0, c15, c2, 0" : "=r"(reg));
	reg |= (1 << 8);
	__asm__ __volatile__ ("mcr p15, 1, %0, c15, c2, 0" : :"r"(reg));

	/*
	 * Set the Point of Coherency and Point of Unification to DRAM.
	 * This is a reset value but anyway, configure this just in case.
	 */
	reg = read_mlmbreg(ARMADAXP_L2_CFU);
	reg |= (1 << 17) | (1 << 18);
	write_mlmbreg(ARMADAXP_L2_CFU, reg);

	/* Enable L2 cache */
	reg = L2_READ(ARMADAXP_L2_CTRL);
	L2_WRITE(ARMADAXP_L2_CTRL, reg | L2_ENABLE);

	/* Mark as enabled */
	l2cache_state = 1;

#ifdef DEBUG
	/* Configure and enable counter */
	L2_WRITE(ARMADAXP_L2_CNTR_CONF(0), 0xf0000 | (4 << 2));
	L2_WRITE(ARMADAXP_L2_CNTR_CONF(1), 0xf0000 | (2 << 2));
	L2_WRITE(ARMADAXP_L2_CNTR_CTRL, 0x303);
#endif

	return (0);
}

void
armadaxp_io_coherency_init(void)
{
	uint32_t reg;

	/* set CIB read snoop command to ReadUnique */
	reg = read_mlmbreg(MVSOC_MLMB_CIB_CTRL_CFG);
	reg &= ~(7 << 16);
	reg |= (7 << 16);
	write_mlmbreg(MVSOC_MLMB_CIB_CTRL_CFG, reg);
	/* enable CPUs in SMP group on Fabric coherency */
	reg = read_mlmbreg(MVSOC_MLMB_COHERENCY_FABRIC_CTRL);
	reg &= ~(0x3 << 24);
	reg |= (1 << 24);
	write_mlmbreg(MVSOC_MLMB_COHERENCY_FABRIC_CTRL, reg);

	reg = read_mlmbreg(MVSOC_MLMB_COHERENCY_FABRIC_CFG);
	reg &= ~(0x3 << 24);
	reg |= (1 << 24);
	write_mlmbreg(MVSOC_MLMB_COHERENCY_FABRIC_CFG, reg);

	/* Mark as enabled */
	iocc_state = 1;
}

int
armadaxp_clkgating(struct marvell_attach_args *mva)
{
	uint32_t val;
	int i;

	for (i = 0; i < __arraycount(clkgatings); i++) {
		if (clkgatings[i].offset == mva->mva_offset) {
			val = read_miscreg(ARMADAXP_MISC_PMCGC);
			if ((val & clkgatings[i].bits) == clkgatings[i].bits)
				/* Clock enabled */
				return 0;
			return 1;
		}
	}
	/* Clock Gating not support */
	return 0;
}
