/*	$NetBSD: gt.c,v 1.12 2005/12/24 20:27:41 perry Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gt.c -- GT system controller driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt.c,v 1.12 2005/12/24 20:27:41 perry Exp $");

#include "opt_marvell.h"
#include "locators.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#define _BUS_SPACE_PRIVATE
#define _BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <powerpc/spr.h>
#include <powerpc/oea/hid.h>

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtethreg.h>

#ifdef DEBUG
#include <sys/systm.h>	/* for Debugger() */
#endif

#if ((GT_MPP_WATCHDOG & 0xf0f0f0f0) != 0)
# error		/* unqualified: configuration botch! */
#endif
#if ((GT_MPP_WATCHDOG & GT_MPP_INTERRUPTS) != 0)
# error		/* conflict: configuration botch! */
#endif

static void	gt_comm_intr_enb(struct gt_softc *);
static void	gt_devbus_intr_enb(struct gt_softc *);
#ifdef GT_ECC
static void	gt_ecc_intr_enb(struct gt_softc *);
#endif


void gt_init_hostid (struct gt_softc *);
void gt_init_interrupt (struct gt_softc *);
static int gt_comm_intr (void *);

void gt_watchdog_init(struct gt_softc *);
void gt_watchdog_enable(void);
void gt_watchdog_disable(void);
void gt_watchdog_reset(void);

extern struct cfdriver gt_cd;

static int gtfound = 0;

static struct gt_softc *gt_watchdog_sc = 0;
static int gt_watchdog_state = 0;

int
gt_cfprint (void *aux, const char *pnp)
{
	struct gt_attach_args *ga = aux;

	if (pnp) {
		aprint_normal("%s at %s", ga->ga_name, pnp);
	}

	aprint_normal(" unit %d", ga->ga_unit);
	return (UNCONF);
}


static int
gt_cfsearch(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct gt_softc *gt = (struct gt_softc *) parent;
	struct gt_attach_args ga;

	ga.ga_name = cf->cf_name;
	ga.ga_dmat = gt->gt_dmat;
	ga.ga_memt = gt->gt_memt;
	ga.ga_memh = gt->gt_memh;
	ga.ga_unit = cf->cf_loc[GTCF_UNIT];

	if (config_match(parent, cf, &ga) > 0)
		config_attach(parent, cf, &ga, gt_cfprint);

	return (0);
}

void
gt_attach_common(struct gt_softc *gt)
{
	uint32_t cpucfg, cpumode, cpumstr;
#ifdef DEBUG
	uint32_t loaddr, hiaddr;
#endif

	gtfound = 1;

	cpumode = gt_read(gt, GT_CPU_Mode);
	aprint_normal(": id %d", GT_CPUMode_MultiGTID_GET(cpumode));
	if (cpumode & GT_CPUMode_MultiGT)
		aprint_normal (" (multi)");
	switch (GT_CPUMode_CPUType_GET(cpumode)) {
	case 4: aprint_normal(", 60x bus"); break;
	case 5: aprint_normal(", MPX bus"); break;
	default: aprint_normal(", %#x(?) bus", GT_CPUMode_CPUType_GET(cpumode)); break;
	}

	cpumstr = gt_read(gt, GT_CPU_Master_Ctl);
	cpumstr &= ~(GT_CPUMstrCtl_CleanBlock|GT_CPUMstrCtl_FlushBlock);
#if 0
	cpumstr |= GT_CPUMstrCtl_CleanBlock|GT_CPUMstrCtl_FlushBlock;
#endif
	gt_write(gt, GT_CPU_Master_Ctl, cpumstr);

	switch (cpumstr & (GT_CPUMstrCtl_CleanBlock|GT_CPUMstrCtl_FlushBlock)) {
	case 0: break;
	case GT_CPUMstrCtl_CleanBlock: aprint_normal(", snoop=clean"); break;
	case GT_CPUMstrCtl_FlushBlock: aprint_normal(", snoop=flush"); break;
	case GT_CPUMstrCtl_CleanBlock|GT_CPUMstrCtl_FlushBlock:
		aprint_normal(", snoop=clean&flush"); break;
	}
	aprint_normal(" wdog=%#x,%#x\n",
		gt_read(gt, GT_WDOG_Config),
		gt_read(gt, GT_WDOG_Value));

#if DEBUG
	loaddr = GT_LowAddr_GET(gt_read(gt, GT_SCS0_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_SCS0_High_Decode));
	aprint_normal("%s:      scs[0]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_SCS1_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_SCS1_High_Decode));
	aprint_normal("%s:      scs[1]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_SCS2_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_SCS2_High_Decode));
	aprint_normal("%s:      scs[2]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_SCS3_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_SCS3_High_Decode));
	aprint_normal("%s:      scs[3]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CS0_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CS0_High_Decode));
	aprint_normal("%s:       cs[0]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CS1_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CS1_High_Decode));
	aprint_normal("%s:       cs[1]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CS2_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CS2_High_Decode));
	aprint_normal("%s:       cs[2]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CS3_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CS3_High_Decode));
	aprint_normal("%s:       cs[3]=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_BootCS_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_BootCS_High_Decode));
	aprint_normal("%s:      bootcs=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI0_IO_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI0_IO_High_Decode));
	aprint_normal("%s:      pci0io=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI0_IO_Remap);
	aprint_normal("remap=%#010x\n", loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI0_Mem0_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI0_Mem0_High_Decode));
	aprint_normal("%s:  pci0mem[0]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI0_Mem0_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI0_Mem0_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI0_Mem1_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI0_Mem1_High_Decode));
	aprint_normal("%s:  pci0mem[1]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI0_Mem1_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI0_Mem1_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI0_Mem2_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI0_Mem2_High_Decode));
	aprint_normal("%s:  pci0mem[2]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI0_Mem2_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI0_Mem2_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI0_Mem3_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI0_Mem3_High_Decode));
	aprint_normal("%s:  pci0mem[3]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI0_Mem3_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI0_Mem3_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI1_IO_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI1_IO_High_Decode));
	aprint_normal("%s:      pci1io=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI1_IO_Remap);
	aprint_normal("remap=%#010x\n", loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI1_Mem0_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI1_Mem0_High_Decode));
	aprint_normal("%s:  pci1mem[0]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI1_Mem0_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI1_Mem0_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI1_Mem1_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI1_Mem1_High_Decode));
	aprint_normal("%s:  pci1mem[1]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI1_Mem1_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI1_Mem1_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI1_Mem2_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI1_Mem2_High_Decode));
	aprint_normal("%s:  pci1mem[2]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI1_Mem2_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI1_Mem2_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_PCI1_Mem3_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_PCI1_Mem3_High_Decode));
	aprint_normal("%s:  pci1mem[3]=%#10x-%#10x  ", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = gt_read(gt, GT_PCI1_Mem3_Remap_Low);
	hiaddr = gt_read(gt, GT_PCI1_Mem3_Remap_High);
	aprint_normal("remap=%#010x.%#010x\n", hiaddr, loaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_Internal_Decode));
	aprint_normal("%s:    internal=%#10x-%#10x\n", gt->gt_dev.dv_xname,
		loaddr, loaddr+256*1024);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CPU0_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CPU0_High_Decode));
	aprint_normal("%s:        cpu0=%#10x-%#10x\n", gt->gt_dev.dv_xname, loaddr, hiaddr);

	loaddr = GT_LowAddr_GET(gt_read(gt, GT_CPU1_Low_Decode));
	hiaddr = GT_HighAddr_GET(gt_read(gt, GT_CPU1_High_Decode));
	aprint_normal("%s:        cpu1=%#10x-%#10x", gt->gt_dev.dv_xname, loaddr, hiaddr);
#endif

	aprint_normal("%s:", gt->gt_dev.dv_xname);

	cpucfg = gt_read(gt, GT_CPU_Cfg);
	cpucfg |= GT_CPUCfg_ConfSBDis;		/* per errata #46 */
	cpucfg |= GT_CPUCfg_AACKDelay;		/* per restriction #18 */
	gt_write(gt, GT_CPU_Cfg, cpucfg);
	if (cpucfg & GT_CPUCfg_Pipeline)
		aprint_normal(" pipeline");
	if (cpucfg & GT_CPUCfg_AACKDelay)
		aprint_normal(" aack-delay");
	if (cpucfg & GT_CPUCfg_RdOOO)
		aprint_normal(" read-ooo");
	if (cpucfg & GT_CPUCfg_IOSBDis)
		aprint_normal(" io-sb-dis");
	if (cpucfg & GT_CPUCfg_ConfSBDis)
		aprint_normal(" conf-sb-dis");
	if (cpucfg & GT_CPUCfg_ClkSync)
		aprint_normal(" clk-sync");
	aprint_normal("\n");

	gt_init_hostid(gt);

	gt_watchdog_init(gt);

	gt_init_interrupt(gt);

#ifdef GT_ECC
	gt_ecc_intr_enb(gt);
#endif

	gt_comm_intr_enb(gt);
	gt_devbus_intr_enb(gt);

	gt_watchdog_disable();
	config_search_ia(gt_cfsearch, &gt->gt_dev, "gt", NULL);
	gt_watchdog_service();
	gt_watchdog_enable();
}

void
gt_init_hostid(struct gt_softc *gt)
{

	hostid = 1;	/* XXX: Used by i2c; needs work -- AKB */
}

void
gt_init_interrupt(struct gt_softc *gt)
{
	u_int32_t mppirpts = GT_MPP_INTERRUPTS;		/* from config */
	u_int32_t r;
	u_int32_t mppbit;
	u_int32_t mask;
	u_int32_t mppsel;
	u_int32_t regoff;

	gt_write(gt, ICR_CIM_LO, 0);
	gt_write(gt, ICR_CIM_HI, 0);

	/*
	 * configure the GPP interrupts:
	 * - set the configured MPP pins in GPP mode
	 * - set the configured GPP pins to input, active low, interrupt enbl
	 */
#ifdef DEBUG
	printf("%s: mpp cfg ", gt->gt_dev.dv_xname);
	for (regoff = GT_MPP_Control0; regoff <= GT_MPP_Control3; regoff += 4)
		printf("%#x ", gt_read(gt, regoff));
	printf(", mppirpts 0x%x\n", mppirpts);
#endif
	mppbit = 0x1;
	for (regoff = GT_MPP_Control0; regoff <= GT_MPP_Control3; regoff += 4) {
		mask = 0;
		for (mppsel = 0xf; mppsel; mppsel <<= 4) {
			if (mppirpts & mppbit)
				mask |= mppsel;
			mppbit <<= 1;
		}
		if (mask) {
			r = gt_read(gt, regoff);
			r &= ~mask;
			gt_write(gt, regoff, r);
		}
	}

	r = gt_read(gt, GT_GPP_IO_Control);
	r &= ~mppirpts;
	gt_write(gt, GT_GPP_IO_Control, r);

	r = gt_read(gt, GT_GPP_Level_Control);
	r |= mppirpts;
	gt_write(gt, GT_GPP_Level_Control, r);

	r = gt_read(gt, GT_GPP_Interrupt_Mask);
	r |= mppirpts;
	gt_write(gt, GT_GPP_Interrupt_Mask, r);
}

uint32_t
gt_read_mpp (void)
{
	return gt_read((struct gt_softc *)gt_cd.cd_devs[0], GT_GPP_Value);
}

#if 0
int
gt_bs_extent_init(struct discovery_bus_space *bs, char *name)
{
	u_long start, end;
	int i, j, error;

	if (bs->bs_nregion == 0) {
		bs->bs_extent = extent_create(name, 0xffffffffUL, 0xffffffffUL,
		    M_DEVBUF, NULL, 0, EX_NOCOALESCE|EX_WAITOK);
		KASSERT(bs->bs_extent != NULL);
		return 0;
	}
	/*
	 * Find the top and bottoms of this bus space.
	 */
	start = bs->bs_regions[0].br_start;
	end = bs->bs_regions[0].br_end;
#ifdef DEBUG
	if (gtpci_debug > 1)
		printf("gtpci_bs_extent_init: %s: region %d: %#lx-%#lx\n",
			name, 0, bs->bs_regions[0].br_start,
			bs->bs_regions[0].br_end);
#endif
	for (i = 1; i < bs->bs_nregion; i++) {
		if (bs->bs_regions[i].br_start < start)
			start = bs->bs_regions[i].br_start;
		if (bs->bs_regions[i].br_end > end)
			end = bs->bs_regions[i].br_end;
#ifdef DEBUG
		if (gtpci_debug > 1)
			printf("gtpci_bs_extent_init: %s: region %d:"
				" %#lx-%#lx\n",
				name, i, bs->bs_regions[i].br_start,
				bs->bs_regions[i].br_end);
#endif
	}
	/*
	 * Now that we have the top and bottom limits of this
	 * bus space, create the extent map that will manage this
	 * space for us.
	 */
#ifdef DEBUG
	if (gtpci_debug > 1)
		printf("gtpci_bs_extent_init: %s: create: %#lx-%#lx\n",
			name, start, end);
#endif
	bs->bs_extent = extent_create(name, start, end, M_DEVBUF,
		NULL, 0, EX_NOCOALESCE|EX_WAITOK);
	KASSERT(bs->bs_extent != NULL);

	/* If there was more than one bus space region, then there
	 * might gaps in between them.  Allocate the gap so that
	 * they will not be legal addresses in the extent.
	 */
	for (i = 0; i < bs->bs_nregion && bs->bs_nregion > 1; i++) {
		/* Initial start is "infinity" and the inital end is
		 * is the end of this bus region.
		 */
		start = ~0UL;
		end = bs->bs_regions[i].br_end;
		/* For each region, if it starts after this region but less
		 * than the saved start, use its start address.  If the start
		 * address is one past the end address, then we're done
		 */
		for (j = 0; j < bs->bs_nregion && start > end + 1; j++) {
			if (i == j)
				continue;
			if (bs->bs_regions[j].br_start > end &&
			    bs->bs_regions[j].br_start < start)
				start = bs->bs_regions[j].br_start;
		}
		/*
		 * If we found a gap, allocate it away.
		 */
		if (start != ~0UL && start != end + 1) {
#ifdef DEBUG
			if (gtpci_debug > 1)
				printf("gtpci_bs_extent_init: %s: alloc(hole): %#lx-%#lx\n",
					name, end + 1, start - 1);
#endif
			error = extent_alloc_region(bs->bs_extent, end + 1,
				start - (end + 1), EX_NOWAIT);
			KASSERT(error == 0);
		}
	}
	return 1;
}
#endif

/*
 * unknown board, enable everything
 */
# define GT_CommUnitIntr_DFLT	GT_CommUnitIntr_S0|GT_CommUnitIntr_S1 \
				|GT_CommUnitIntr_E0|GT_CommUnitIntr_E1 \
				|GT_CommUnitIntr_E2

static const char * const gt_comm_subunit_name[8] = {
	"ethernet 0",
	"ethernet 1",
	"ethernet 2",
	"(reserved)",
	"MPSC 0",
	"MPSC 1",
	"(reserved)",
	"(sel)",
};

static int
gt_comm_intr(void *arg)
{
	struct gt_softc *gt = (struct gt_softc *)arg;
	u_int32_t cause;
	u_int32_t addr;
	unsigned int mask;
	int i;

	cause = gt_read(gt, GT_CommUnitIntr_Cause);
	gt_write(gt, GT_CommUnitIntr_Cause, ~cause);
	addr = gt_read(gt, GT_CommUnitIntr_ErrAddr);

	printf("%s: Comm Unit irpt, cause %#x addr %#x\n",
		gt->gt_dev.dv_xname, cause, addr);

	cause &= GT_CommUnitIntr_DFLT;
	if (cause == 0)
		return 0;

	mask = 0x7;
	for (i=0; i<7; i++) {
		if (cause & mask) {
			printf("%s: Comm Unit %s:", gt->gt_dev.dv_xname,
				gt_comm_subunit_name[i]);
			if (cause & 1)
				printf(" AddrMiss");
			if (cause & 2)
				printf(" AccProt");
			if (cause & 4)
				printf(" WrProt");
			printf("\n");
		}
		cause >>= 4;
	}
	return 1;
}

/*
 * gt_comm_intr_init - enable GT-64260 Comm Unit interrupts
 */
static void
gt_comm_intr_enb(struct gt_softc *gt)
{
	u_int32_t cause;

	cause = gt_read(gt, GT_CommUnitIntr_Cause);
	if (cause)
		gt_write(gt, GT_CommUnitIntr_Cause, ~cause);
	gt_write(gt, GT_CommUnitIntr_Mask, GT_CommUnitIntr_DFLT);
	(void)gt_read(gt, GT_CommUnitIntr_ErrAddr);

	intr_establish(IRQ_COMM, IST_LEVEL, IPL_GTERR, gt_comm_intr, gt);
	printf("%s: Comm Unit irpt at %d\n", gt->gt_dev.dv_xname, IRQ_COMM);
}

#ifdef GT_ECC
static char *gt_ecc_intr_str[4] = {
	"(none)",
	"single bit",
	"double bit",
	"(reserved)"
};

static int
gt_ecc_intr(void *arg)
{
	struct gt_softc *gt = (struct gt_softc *)arg;
	u_int32_t addr;
	u_int32_t dlo;
	u_int32_t dhi;
	u_int32_t rec;
	u_int32_t calc;
	u_int32_t count;
	int err;

	count = gt_read(gt, GT_ECC_Count);
	dlo   = gt_read(gt, GT_ECC_Data_Lo);
	dhi   = gt_read(gt, GT_ECC_Data_Hi);
	rec   = gt_read(gt, GT_ECC_Rec);
	calc  = gt_read(gt, GT_ECC_Calc);
	addr  = gt_read(gt, GT_ECC_Addr);	/* read last! */
	gt_write(gt, GT_ECC_Addr, 0);		/* clear irpt */

	err = addr & 0x3;

	printf("%s: ECC error: %s: "
		"addr %#x data %#x.%#x rec %#x calc %#x cnt %#x\n",
		gt->gt_dev.dv_xname, gt_ecc_intr_str[err],
		addr, dhi, dlo, rec, calc, count);

	if (err == 2)
		panic("ecc");

	return (err == 1);
}

/*
 * gt_ecc_intr_enb - enable GT-64260 ECC interrupts
 */
static void
gt_ecc_intr_enb(struct gt_softc *gt)
{
	u_int32_t ctl;

	ctl = gt_read(gt, GT_ECC_Ctl);
	ctl |= 1 << 16;		/* XXX 1-bit threshold == 1 */
	gt_write(gt, GT_ECC_Ctl, ctl);
	(void)gt_read(gt, GT_ECC_Data_Lo);
	(void)gt_read(gt, GT_ECC_Data_Hi);
	(void)gt_read(gt, GT_ECC_Rec);
	(void)gt_read(gt, GT_ECC_Calc);
	(void)gt_read(gt, GT_ECC_Addr);	/* read last! */
	gt_write(gt, GT_ECC_Addr, 0);		/* clear irpt */

	intr_establish(IRQ_ECC, IST_LEVEL, IPL_GTERR, gt_ecc_intr, gt);
	printf("%s: ECC irpt at %d\n", gt->gt_dev.dv_xname, IRQ_ECC);
}
#endif	/* GT_ECC */


#ifndef GT_MPP_WATCHDOG
void
gt_watchdog_init(struct gt_softc *gt)
{
	u_int32_t r;
	unsigned int omsr;

	omsr = extintr_disable();

	printf("%s: watchdog", gt->gt_dev.dv_xname);

	/*
	 * handle case where firmware started watchdog
	 */
	r = gt_read(gt, GT_WDOG_Config);
	printf(" status %#x,%#x:",
		r, gt_read(gt, GT_WDOG_Value));
	if ((r & 0x80000000) != 0) {
		gt_watchdog_sc = gt;		/* enabled */
		gt_watchdog_state = 1;
		printf(" firmware-enabled\n");
		gt_watchdog_service();
		return;
	} else {
		printf(" firmware-disabled\n");
	}

	extintr_restore(omsr);
}

#else	/* GT_MPP_WATCHDOG */

void
gt_watchdog_init(struct gt_softc *gt)
{
	u_int32_t mpp_watchdog = GT_MPP_WATCHDOG;	/* from config */
	u_int32_t r;
	u_int32_t cfgbits;
	u_int32_t mppbits;
	u_int32_t mppmask=0;
	u_int32_t regoff;
	unsigned int omsr;

	printf("%s: watchdog", gt->gt_dev.dv_xname);

	if (mpp_watchdog == 0) {
		printf(" not configured\n");
		return;
	}

#if 0
	if (afw_wdog_ctl == 1) {
		printf(" admin disabled\n");
		return;
	}
#endif

	omsr = extintr_disable();

	/*
	 * if firmware started watchdog, we disable and start
	 * from scratch to get it in a known state.
	 *
	 * on GT-64260A we always see 0xffffffff
	 * in both the GT_WDOG_Config_Enb and GT_WDOG_Value regsiters.
	 * Use AFW-supplied flag to determine run state.
	 */
	r = gt_read(gt, GT_WDOG_Config);
	if (r != ~0) {
		if ((r & GT_WDOG_Config_Enb) != 0) {
			gt_write(gt, GT_WDOG_Config,
				(GT_WDOG_Config_Ctl1a | GT_WDOG_Preset_DFLT));
			gt_write(gt, GT_WDOG_Config,
				(GT_WDOG_Config_Ctl1b | GT_WDOG_Preset_DFLT));
		}
	} else {
#if 0
		if (afw_wdog_state == 1) {
			gt_write(gt, GT_WDOG_Config,
				(GT_WDOG_Config_Ctl1a | GT_WDOG_Preset_DFLT));
			gt_write(gt, GT_WDOG_Config,
				(GT_WDOG_Config_Ctl1b | GT_WDOG_Preset_DFLT));
		}
#endif
	}

	/*
	 * "the watchdog timer can be activated only after
	 * configuring two MPP pins to act as WDE and WDNMI"
	 */
	mppbits = 0;
	cfgbits = 0x3;
	for (regoff = GT_MPP_Control0; regoff <= GT_MPP_Control3; regoff += 4) {
		if ((mpp_watchdog & cfgbits) == cfgbits) {
			mppbits = 0x99;
			mppmask = 0xff;
			break;
		}
		cfgbits <<= 2;
		if ((mpp_watchdog & cfgbits) == cfgbits) {
			mppbits = 0x9900;
			mppmask = 0xff00;
			break;
		}
		cfgbits <<= 6;	/* skip unqualified bits */
	}
	if (mppbits == 0) {
		printf(" config error\n");
		extintr_restore(omsr);
		return;
	}

	r = gt_read(gt, regoff);
	r &= ~mppmask;
	r |= mppbits;
	gt_write(gt, regoff, r);
	printf(" mpp %#x %#x", regoff, mppbits);

	gt_write(gt, GT_WDOG_Value, GT_WDOG_NMI_DFLT);

	gt_write(gt, GT_WDOG_Config,
		(GT_WDOG_Config_Ctl1a | GT_WDOG_Preset_DFLT));
	gt_write(gt, GT_WDOG_Config,
		(GT_WDOG_Config_Ctl1b | GT_WDOG_Preset_DFLT));


	r = gt_read(gt, GT_WDOG_Config),
	printf(" status %#x,%#x: %s",
		r, gt_read(gt, GT_WDOG_Value),
		((r & GT_WDOG_Config_Enb) != 0) ? "enabled" : "botch");

	if ((r & GT_WDOG_Config_Enb) != 0) {
		register_t hid0;

		gt_watchdog_sc = gt;		/* enabled */
		gt_watchdog_state = 1;

		/*
		 * configure EMCP in HID0 in case it's not already set
		 */
		__asm volatile("sync");
		hid0 = mfspr(SPR_HID0);
		if ((hid0 & HID0_EMCP) == 0) {
			hid0 |= HID0_EMCP;
			__asm volatile("sync"); mtspr(SPR_HID0, hid0);
			__asm volatile("sync"); hid0 = mfspr(SPR_HID0);
			printf(", EMCP set");
		}
	}
	printf("\n");

	extintr_restore(omsr);
}
#endif	/* GT_MPP_WATCHDOG */

#ifdef DEBUG
u_int32_t hid0_print(void);
u_int32_t
hid0_print()
{
	u_int32_t hid0;
	__asm volatile("sync; mfspr %0,1008;" : "=r"(hid0));
	printf("hid0: %#x\n", hid0);
	return hid0;
}
#endif

void
gt_watchdog_enable(void)
{
	struct gt_softc *gt;
	unsigned int omsr;

	omsr = extintr_disable();
	gt = gt_watchdog_sc;
	if ((gt != NULL) && (gt_watchdog_state == 0)) {
		gt_watchdog_state = 1;

		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1a | GT_WDOG_Preset_DFLT));
		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1b | GT_WDOG_Preset_DFLT));
	}
	extintr_restore(omsr);
}

void
gt_watchdog_disable(void)
{
	struct gt_softc *gt;
	unsigned int omsr;

	omsr = extintr_disable();
	gt = gt_watchdog_sc;
	if ((gt != NULL) && (gt_watchdog_state != 0)) {
		gt_watchdog_state = 0;

		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1a | GT_WDOG_Preset_DFLT));
		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1b | GT_WDOG_Preset_DFLT));
	}
	extintr_restore(omsr);
}

#ifdef DEBUG
int inhibit_watchdog_service = 0;
#endif
void
gt_watchdog_service(void)
{
	struct gt_softc *gt = gt_watchdog_sc;

	if ((gt == NULL) || (gt_watchdog_state == 0))
		return;		/* not enabled */
#ifdef DEBUG
	if (inhibit_watchdog_service)
		return;
#endif

	gt_write(gt, GT_WDOG_Config,
		(GT_WDOG_Config_Ctl2a | GT_WDOG_Preset_DFLT));
	gt_write(gt, GT_WDOG_Config,
		(GT_WDOG_Config_Ctl2b | GT_WDOG_Preset_DFLT));
}

/*
 * gt_watchdog_reset - force a watchdog reset using Preset_VAL=0
 */
void
gt_watchdog_reset()
{
	struct gt_softc *gt = gt_watchdog_sc;
	u_int32_t r;

	(void)extintr_disable();
	r = gt_read(gt, GT_WDOG_Config);
	gt_write(gt, GT_WDOG_Config, (GT_WDOG_Config_Ctl1a | 0));
	gt_write(gt, GT_WDOG_Config, (GT_WDOG_Config_Ctl1b | 0));
	if ((r & GT_WDOG_Config_Enb) != 0) {
		/*
		 * was enabled, we just toggled it off, toggle on again
		 */
		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1a | 0));
		gt_write(gt, GT_WDOG_Config,
			(GT_WDOG_Config_Ctl1b | 0));
	}
	for(;;);
}

static int
gt_devbus_intr(void *arg)
{
	struct gt_softc *gt = (struct gt_softc *)arg;
	u_int32_t cause;
	u_int32_t addr;

	cause = gt_read(gt, GT_DEVBUS_ICAUSE);
	addr = gt_read(gt, GT_DEVBUS_ERR_ADDR);
	gt_write(gt, GT_DEVBUS_ICAUSE, 0);	/* clear irpt */

	if (cause & GT_DEVBUS_DBurstErr) {
		printf("%s: Device Bus error: burst violation",
			gt->gt_dev.dv_xname);
		if ((cause & GT_DEVBUS_Sel) == 0)
			printf(", addr %#x", addr);
		printf("\n");
	}
	if (cause & GT_DEVBUS_DRdyErr) {
		printf("%s: Device Bus error: ready timer expired",
			gt->gt_dev.dv_xname);
		if ((cause & GT_DEVBUS_Sel) != 0)
			printf(", addr %#x\n", addr);
		printf("\n");
	}

	return (cause != 0);
}

/*
 * gt_ecc_intr_enb - enable GT-64260 ECC interrupts
 */
static void
gt_devbus_intr_enb(struct gt_softc *gt)
{
	gt_write(gt, GT_DEVBUS_IMASK,
		GT_DEVBUS_DBurstErr|GT_DEVBUS_DRdyErr);
	(void)gt_read(gt, GT_DEVBUS_ERR_ADDR);	/* clear addr */
	gt_write(gt, GT_ECC_Addr, 0);		/* clear irpt */

	intr_establish(IRQ_DEV, IST_LEVEL, IPL_GTERR, gt_devbus_intr, gt);
	printf("%s: Device Bus Error irpt at %d\n",
		gt->gt_dev.dv_xname, IRQ_DEV);
}


int
gt_mii_read(
	struct device *child,
	struct device *parent,
	int phy,
	int reg)
{
	struct gt_softc * const gt = (struct gt_softc *) parent;
	uint32_t data;
	int count = 10000;

	do {
		DELAY(10);
		data = gt_read(gt, ETH_ESMIR);
	} while ((data & ETH_ESMIR_Busy) && count-- > 0);

	if (count == 0) {
		printf("%s: mii read for phy %d reg %d busied out\n",
			child->dv_xname, phy, reg);
		return ETH_ESMIR_Value_GET(data);
	}

	gt_write(gt, ETH_ESMIR, ETH_ESMIR_READ(phy, reg));

	count = 10000;
	do {
		DELAY(10);
		data = gt_read(gt, ETH_ESMIR);
	} while ((data & ETH_ESMIR_ReadValid) == 0 && count-- > 0);

	if (count == 0)
		printf("%s: mii read for phy %d reg %d timed out\n",
			child->dv_xname, phy, reg);
#if defined(GTMIIDEBUG)
	printf("%s: mii_read(%d, %d): %#x data %#x\n",
		child->dv_xname, phy, reg,
		data, ETH_ESMIR_Value_GET(data));
#endif
	return ETH_ESMIR_Value_GET(data);
}

void
gt_mii_write (
	struct device *child,
	struct device *parent,
	int phy, int reg,
	int value)
{
	struct gt_softc * const gt = (struct gt_softc *) parent;
	uint32_t data;
	int count = 10000;

	do {
		DELAY(10);
		data = gt_read(gt, ETH_ESMIR);
	} while ((data & ETH_ESMIR_Busy) && count-- > 0);

	if (count == 0) {
		printf("%s: mii write for phy %d reg %d busied out (busy)\n",
			child->dv_xname, phy, reg);
		return;
	}

	gt_write(gt, ETH_ESMIR,
		 ETH_ESMIR_WRITE(phy, reg, value));

	count = 10000;
	do {
		DELAY(10);
		data = gt_read(gt, ETH_ESMIR);
	} while ((data & ETH_ESMIR_Busy) && count-- > 0);

	if (count == 0)
		printf("%s: mii write for phy %d reg %d timed out\n",
			child->dv_xname, phy, reg);
#if defined(GTMIIDEBUG)
	printf("%s: mii_write(%d, %d, %#x)\n",
		child->dv_xname, phy, reg, value);
#endif
}

/*
 * Since the memory and pci spaces are mapped 1:1 we just need
 * to return unity here
 */
bus_addr_t
gt_dma_phys_to_bus_mem(bus_dma_tag_t t, bus_addr_t a)
{
	return a;
}
bus_addr_t
gt_dma_bus_mem_to_phys(bus_dma_tag_t t, bus_addr_t a)
{
	return a;
}
