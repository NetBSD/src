/*	$NetBSD: gtpci.c,v 1.2 2003/03/16 07:05:34 matt Exp $	*/

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

#include "opt_marvell.h"
#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <lib/libkern/libkern.h>

#define _BUS_SPACE_PRIVATE
#define _BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>
#include <dev/marvell/gtvar.h>

static int	gtpci_error_intr(void *);

static void	gtpci_bus_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
static int	gtpci_bus_maxdevs(pci_chipset_tag_t, int);

static const char *
		gtpci_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
static const struct evcnt *
		gtpci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
static void	*gtpci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	gtpci_intr_disestablish(pci_chipset_tag_t, void *);

#ifdef DEBUG
int gtpci_debug = 0;
#endif

struct gtpci_softc {
	struct device gtpci_dev;
	struct gtpci_chipset gtpci_gtpc;
};

static int gtpci_cfprint(void *, const char *);
static int gtpci_match(struct device *, struct cfdata *, void *);
static void gtpci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(gtpci, sizeof(struct gtpci_softc),
    gtpci_match, gtpci_attach, NULL, NULL);

extern struct cfdriver gtpci_cd;

static int gtpci_busmask;

const struct pci_chipset_functions gtpci_functions = {
	gtpci_bus_attach_hook,
	gtpci_bus_maxdevs,
	gtpci_md_bus_devorder,

	gtpci_make_tag,
	gtpci_decompose_tag,

	gtpci_conf_read,
	gtpci_conf_write,
	gtpci_md_conf_hook,
	gtpci_md_conf_interrupt,

	gtpci_md_intr_map,
	gtpci_intr_string,
	gtpci_intr_evcnt,
	gtpci_intr_establish,
	gtpci_intr_disestablish
};

int
gtpci_match(struct device *parent, struct cfdata *self, void *aux)
{
	struct gt_softc * const gt = (struct gt_softc *) parent;
	struct gt_attach_args * const ga = aux;

	return GT_PCIOK(gt, ga, &gtpci_cd);
}

int
gtpci_cfprint(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = (struct pcibus_attach_args *) aux;
	
	if (pnp)
		aprint_normal("pci at %s", pnp);

	aprint_normal(" bus %d", pba->pba_bus);

	return (UNCONF);
}

void
gtpci_attach(struct device *parent, struct device *self, void *aux)
{
	struct pcibus_attach_args pba;
	struct gt_attach_args * const ga = aux;
	struct gt_softc * const gt = (struct gt_softc *) parent;
	struct gtpci_softc * const gtp = (struct gtpci_softc *) self;
	struct gtpci_chipset * const gtpc = &gtp->gtpci_gtpc;
	struct pci_chipset * const pc = &gtpc->gtpc_pc;
	const int busno = ga->ga_unit;
	uint32_t data;

	GT_PCIFOUND(gt, ga);

	pc->pc_funcs = &gtpci_functions;
	pc->pc_parent = self;

	gtpc->gtpc_busno = busno;
	gtpc->gtpc_cfgaddr = PCI_CONFIG_ADDR(busno);
	gtpc->gtpc_cfgdata = PCI_CONFIG_DATA(busno);
	gtpc->gtpc_syncreg = PCI_SYNC_REG(busno);
	gtpc->gtpc_gt_memt = ga->ga_memt;
	gtpc->gtpc_gt_memh = ga->ga_memh;

	switch (busno) {
	case 0:
		gtpc->gtpc_io_bs = gt->gt_pci0_iot;
		gtpc->gtpc_mem_bs = gt->gt_pci0_memt;
		break;
	case 1:
		gtpc->gtpc_io_bs = gt->gt_pci1_iot;
		gtpc->gtpc_mem_bs = gt->gt_pci1_memt;
		break;
	default:
		break;
	}

	/*
	 * If no bus_spaces exist, then it's been disabled.
	 */
	if (gtpc->gtpc_io_bs == NULL && gtpc->gtpc_mem_bs == NULL) {
		aprint_normal(": disabled\n");
		return;
	}

	aprint_normal("\n");

	pba.pba_pc = pc;
	pba.pba_bus = 0;
	pba.pba_busname = "pci";
	pba.pba_iot = gtpc->gtpc_io_bs;
	pba.pba_memt = gtpc->gtpc_mem_bs;
	pba.pba_dmat = gt->gt_dmat;
	pba.pba_flags = 0;
	if (pba.pba_iot != NULL)
		pba.pba_flags |= PCI_FLAGS_IO_ENABLED;
	if (pba.pba_memt != NULL)
		pba.pba_flags |= PCI_FLAGS_MEM_ENABLED;

	data = gtpci_read(gtpc, PCI_COMMAND(gtpc->gtpc_busno));
	if (data & PCI_CMD_MRdMul)
		pba.pba_flags |= PCI_FLAGS_MRM_OKAY;
	if (data & PCI_CMD_MRdLine)
		pba.pba_flags |= PCI_FLAGS_MRL_OKAY;
	pba.pba_flags |= PCI_FLAGS_MWI_OKAY;

	gt_watchdog_service();

	config_found(self, &pba, gtpci_cfprint);

	gt_watchdog_service();

	/*
	 * clear any pre-existing error interrupt(s)
	 * clear latched pci error registers
	 * establish ISRs for PCI errors
	 * enable PCI error interrupts
	 */
	gtpci_write(gtpc, PCI_ERROR_CAUSE(gtpc->gtpc_busno), 0);
	(void)gtpci_read(gtpc, PCI_ERROR_DATA_LOW(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_DATA_HIGH(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_COMMAND(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_ADDRESS_HIGH(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_ADDRESS_LOW(gtpc->gtpc_busno));
	if (gtpc->gtpc_busno == 0) {
		intr_establish(IRQ_PCI0_0, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		intr_establish(IRQ_PCI0_1, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		intr_establish(IRQ_PCI0_2, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		aprint_normal("%s: %s%d error interrupts at irqs %d, %d, %d\n", 
			pc->pc_parent->dv_xname, "pci", busno,
			IRQ_PCI0_0, IRQ_PCI0_1, IRQ_PCI0_2);
			
	} else {
		intr_establish(IRQ_PCI1_0, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		intr_establish(IRQ_PCI1_1, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		intr_establish(IRQ_PCI1_2, IST_LEVEL, IPL_GTERR,
			gtpci_error_intr, pc);
		aprint_normal("%s: %s%d error interrupts at irqs %d, %d, %d\n", 
			pc->pc_parent->dv_xname, "pci", busno,
			IRQ_PCI1_0, IRQ_PCI1_1, IRQ_PCI1_2);
	}
	gtpci_write(gtpc, PCI_ERROR_MASK(gtpc->gtpc_busno), PCI_SERRMSK_ALL_ERRS);

	gtpci_busmask |= (1 << busno);
}

void
gtpci_bus_attach_hook(struct device *parent, struct device *self,
	struct pcibus_attach_args *pba)
{
	pci_chipset_tag_t pc = pba->pba_pc;
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
	pcitag_t tag;
	uint32_t data;
	int i;

	if (pc->pc_parent != parent)
		return;

	data = gtpci_read(gtpc, PCI_MODE(gtpc->gtpc_busno));
	aprint_normal(": id %d%s%s%s%s%s%s%s%s",
		PCI_MODE_PciID_GET(data),
		(data & PCI_MODE_Pci64) ? ", 64bit" : "",
		(data & PCI_MODE_ExpRom) ? ", Expansion Rom" : "",
		(data & PCI_MODE_VPD) ? ", VPD" : "",
		(data & PCI_MODE_MSI) ? ", MSI" : "",
		(data & PCI_MODE_PMG) ? ", PMG" : "",
		(data & PCI_MODE_HotSwap) ? ", HotSwap" : "",
		(data & PCI_MODE_BIST) ? ", BIST" : "",
		(data & PCI_MODE_PRst) ? "" : ", PRst");

	while ((data & PCI_MODE_PRst) == 0) {
		data = gtpci_read(gtpc, PCI_MODE(gtpc->gtpc_busno));
		i = (i + 1) & 0x7ffffff; aprint_normal(".");
	}

	gtpci_config_bus(pc, gtpc->gtpc_busno);
	
	tag = gtpci_make_tag(pc, 0, 0, 0);
	data = gtpci_read(gtpc, PCI_BASE_ADDR_REGISTERS_ENABLE(gtpc->gtpc_busno));
	aprint_normal("\n%s: BARs enabled: %#x", self->dv_xname, data);

#if DEBUG
	aprint_normal("\n%s: 0:0:0\n", self->dv_xname);
	aprint_normal("   %sSCS0=%#010x",
		(data & 1) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x10));
	aprint_normal("/%#010x", gtpci_read(gtpc,
		PCI_SCS0_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS0_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS1=%#010x",
		(data & 2) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS1_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS1_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS2=%#010x",
		(data & 4) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS2_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS2_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS3=%#010x",
		(data & 8) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS3_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS3_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sIMem=%#010x",
		(data & PCI_BARE_IntMemEn) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x20));
	aprint_normal("\n");
	aprint_normal("    %sIIO=%#010x",
		(data & PCI_BARE_IntIOEn) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x24));
	aprint_normal("\n");
	tag = gtpci_make_tag(pc, 0, 0, 1);
	aprint_normal("    %sCS0=%#010x",
		(data & PCI_BARE_CS0En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x10));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS0_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS0_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS1=%#010x",
		(data & PCI_BARE_CS1En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS1_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS1_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS2=%#010x",
		(data & PCI_BARE_CS2En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS2_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS2_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS3=%#010x",
		(data & PCI_BARE_CS3En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS3_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS3_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal(" %sBootCS=%#010x",
		(data & PCI_BARE_BootCSEn) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x20));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_BOOTCS_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_BOOTCS_ADDR_REMAP(gtpc->gtpc_busno)));

	tag = gtpci_make_tag(pc, 0, 0, 2);
	aprint_normal("  %sP2PM0=%#010x",
		(data & PCI_BARE_P2PMem0En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x10));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_MEM0_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x.%#010x\n",
		gtpci_read(gtpc, PCI_P2P_MEM0_BASE_ADDR_REMAP_HIGH(gtpc->gtpc_busno)),
		gtpci_read(gtpc, PCI_P2P_MEM0_BASE_ADDR_REMAP_LOW(gtpc->gtpc_busno)));

	aprint_normal("  %sP2PM1=%#010x",
		(data & PCI_BARE_P2PMem1En) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_MEM1_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x.%#010x\n",
		gtpci_read(gtpc, PCI_P2P_MEM1_BASE_ADDR_REMAP_HIGH(gtpc->gtpc_busno)),
		gtpci_read(gtpc, PCI_P2P_MEM1_BASE_ADDR_REMAP_LOW(gtpc->gtpc_busno)));

	aprint_normal("  %sP2PIO=%#010x",
		(data & PCI_BARE_P2PIOEn) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_IO_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_P2P_IO_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCPU=%#010x",
		(data & PCI_BARE_CPUEn) ? "-" : "+",
		gtpci_conf_read(pc, tag, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CPU_BAR_SIZE(gtpc->gtpc_busno))+0x1000);
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CPU_BASE_ADDR_REMAP(gtpc->gtpc_busno)));
#endif
	tag = gtpci_make_tag(pc, 0, 0, 0);
	data = gtpci_read(gtpc, PCI_COMMAND(gtpc->gtpc_busno));
	if (data & (PCI_CMD_MSwapEn|PCI_CMD_SSwapEn)) {

		aprint_normal("\n%s: ", self->dv_xname);

		if (data & PCI_CMD_MSwapEn) {
			switch (data & (PCI_CMD_MWordSwap|PCI_CMD_MByteSwap)) {
			case PCI_CMD_MWordSwap:
				aprint_normal(" mswap=w"); break;
			case PCI_CMD_MByteSwap:
				aprint_normal(" mswap=b"); break;
			case PCI_CMD_MWordSwap|PCI_CMD_MByteSwap:
				aprint_normal(" mswap=b+w"); break;
			case 0:
				aprint_normal(" mswap=none"); break;
			}
		}

		if (data & PCI_CMD_SSwapEn) {
			switch (data & (PCI_CMD_SWordSwap|PCI_CMD_SByteSwap)) {
			case PCI_CMD_SWordSwap:
				aprint_normal(" sswap=w"); break;
			case PCI_CMD_SByteSwap:
				aprint_normal(" sswap=b"); break;
			case PCI_CMD_SWordSwap|PCI_CMD_SByteSwap:
				aprint_normal(" sswap=b+w"); break;
			case 0:
				aprint_normal(" sswap=none"); break;
			}
		}
	}

#if DEBUG
	for (i = 0; i < 8; i++) {
		aprint_normal("\n%s: Access Control %d: ", self->dv_xname, i);
		aprint_normal("base(hi=%#x,lo=%#x)=0x%08x.%03xxxxxx",
			PCI_ACCESS_CONTROL_BASE_HIGH(gtpc->gtpc_busno, i),
			PCI_ACCESS_CONTROL_BASE_LOW(gtpc->gtpc_busno, i),
			gtpci_read(gtpc,
				PCI_ACCESS_CONTROL_BASE_HIGH(gtpc->gtpc_busno, i)),
			gtpci_read(gtpc,
				PCI_ACCESS_CONTROL_BASE_LOW(gtpc->gtpc_busno, i)) & 0xfff);
		aprint_normal(" cfg=%#x",
			gtpci_read(gtpc,
				PCI_ACCESS_CONTROL_BASE_LOW(gtpc->gtpc_busno, i)) & ~0xfff);
		aprint_normal(" top(%#x)=%#x",
			PCI_ACCESS_CONTROL_TOP(gtpc->gtpc_busno, i),
			gtpci_read(gtpc,
				PCI_ACCESS_CONTROL_TOP(gtpc->gtpc_busno, i)));
	}
#endif

#if 0
	gtpci_write(gtpc, PCI_ACCESS_CONTROL_BASE_HIGH(gtpc->gtpc_busno, 0), 0);
	gtpci_write(gtpc, PCI_ACCESS_CONTROL_BASE_LOW(gtpc->gtpc_busno, 0),
		0x01001000);
	gtpci_write(gtpc, PCI_ACCESS_CONTROL_TOP(gtpc->gtpc_busno, 0),
		0x00000020);
#endif
}

static const char * const gtpci_error_strings[] = PCI_IC_SEL_Strings;

int
gtpci_error_intr(void *arg)
{
	pci_chipset_tag_t pc = arg;
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
	uint32_t cause, mask, errmask;
	u_int32_t alo, ahi, dlo, dhi, cmd;
	int i;

	cause = gtpci_read(gtpc, PCI_ERROR_CAUSE(gtpc->gtpc_busno));
	errmask = gtpci_read(gtpc, PCI_ERROR_MASK(gtpc->gtpc_busno));
	cause &= (errmask | (0x1f << 27));
	gtpci_write(gtpc, PCI_ERROR_CAUSE(gtpc->gtpc_busno), ~cause);
	printf("%s: pci%d error: cause=%#x mask=%#x",
		pc->pc_parent->dv_xname, gtpc->gtpc_busno, cause, errmask);
	if ((cause & ~(0x1f << 27)) == 0) {
		printf(" ?\n");
		return 0;
	}

	for (i = 0, mask = 1; i <= 26; i++, mask += mask)
		if (mask & cause)
			printf(" %s", gtpci_error_strings[i]);

	/*
	 * "no new data is latched until the PCI Error Low Address
	 * register is read.  This means that PCI Error Low Address
	 * register must be the last register read by the interrupt
	 * handler."
	 */
	dlo = gtpci_read(gtpc, PCI_ERROR_DATA_LOW(gtpc->gtpc_busno));
	dhi = gtpci_read(gtpc, PCI_ERROR_DATA_HIGH(gtpc->gtpc_busno));
	cmd = gtpci_read(gtpc, PCI_ERROR_COMMAND(gtpc->gtpc_busno));
	ahi = gtpci_read(gtpc, PCI_ERROR_ADDRESS_HIGH(gtpc->gtpc_busno));
	alo = gtpci_read(gtpc, PCI_ERROR_ADDRESS_LOW(gtpc->gtpc_busno));
	printf("\n%s: pci%d error: %s cmd %#x data %#x.%#x address=%#x.%#x\n",
		pc->pc_parent->dv_xname, gtpc->gtpc_busno,
		gtpci_error_strings[PCI_IC_SEL_GET(cause)],
		cmd, dhi, dlo, ahi, alo);

#if defined(DEBUG) && defined(DDB)
	if (gtpci_debug > 0)
		Debugger();
#endif
	return 1;
}


#if 0
void
gtpci_bs_region_add(pci_chipset_tag_t pc, struct discovery_bus_space *bs,
	struct gt_softc *gt, bus_addr_t lo, bus_addr_t hi)
{
	/* See how I/O space is configured.  Read the base and top
	 * registers.
	 */
	paddr_t pbasel, pbaseh;
	uint32_t datal, datah;

	datal = gtpci_read(gtpc, lo);
	datah = gtpci_read(gtpc, hi);
	pbasel = GT_LowAddr_GET(datal);
	pbaseh = GT_HighAddr_GET(datah);
	/*
	 * If the start is greater than the end, ignore the region.
 	 */
	if (pbaseh < pbasel)
		return;
	if ((pbasel & gt->gt_iobat_mask) == gt->gt_iobat_pbase
	    && (pbaseh & gt->gt_iobat_mask) == gt->gt_iobat_pbase) {
		bs->bs_regions[bs->bs_nregion].br_vbase =
			gt->gt_iobat_vbase + (pbasel & ~gt->gt_iobat_mask);
	}
	bs->bs_regions[bs->bs_nregion].br_pbase = pbasel;
	if (bs->bs_flags & _BUS_SPACE_RELATIVE) {
		bs->bs_regions[bs->bs_nregion].br_start = 0;
		bs->bs_regions[bs->bs_nregion].br_end = pbaseh - pbasel;
	} else {
		bs->bs_regions[bs->bs_nregion].br_start = pbasel;
		bs->bs_regions[bs->bs_nregion].br_end = pbaseh;
	}
	bs->bs_nregion++;
}
#endif

/*
 * Internal functions.
 */
int
gtpci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return 32;
}

pcitag_t
gtpci_make_tag(pci_chipset_tag_t pc, int busno, int devno, int funcno)
{
	return PCI_CFG_MAKE_TAG(busno, devno, funcno, 0);
}

void
gtpci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
		    int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = PCI_CFG_GET_BUSNO(tag);
	if (dp != NULL)
		*dp = PCI_CFG_GET_DEVNO(tag);
	if (fp != NULL)
		*fp = PCI_CFG_GET_FUNCNO(tag);
}

pcireg_t
gtpci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int regno)
{
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
#ifdef DIAGNOSTIC
	if ((regno & 3) || ((regno + 3) & ~0xff))
		panic("gtpci_conf_read: bad regno %#x\n", regno);
#endif
	gtpci_write(gtpc, gtpc->gtpc_cfgaddr, (int) tag | regno);
	return gtpci_read(gtpc, gtpc->gtpc_cfgdata);
}

void
gtpci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int regno, pcireg_t data)
{
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
#ifdef DIAGNOSTIC
	if ((regno & 3) || ((regno + 3) & ~0xff))
		panic("gtpci_conf_write: bad regno %#x\n", regno);
#endif
	gtpci_write(gtpc, gtpc->gtpc_cfgaddr, (int) tag | regno);
	gtpci_write(gtpc, gtpc->gtpc_cfgdata, data);
}

const char *
gtpci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t pih)
{
	return intr_string(pih);
}

const struct evcnt *
gtpci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t pih)
{
	return intr_evcnt(pih);
}

void *
gtpci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t pih,
    int ipl, int (*handler)(void *), void *arg)
{
	return intr_establish(pih, IST_LEVEL, ipl, handler, arg);
}

void
gtpci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	intr_disestablish(cookie);
}
