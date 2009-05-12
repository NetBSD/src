/*	$NetBSD: gtpci.c,v 1.20 2009/05/12 12:18:45 cegger Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtpci.c,v 1.20 2009/05/12 12:18:45 cegger Exp $");

#include "opt_marvell.h"
#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <lib/libkern/libkern.h>

#define _BUS_SPACE_PRIVATE
#define _BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>

static int	gtpci_error_intr(void *);

static void	gtpci_bus_init(struct gtpci_chipset *);

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
static int gtpci_match(struct device *, cfdata_t, void *);
static void gtpci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(gtpci, sizeof(struct gtpci_softc),
    gtpci_match, gtpci_attach, NULL, NULL);

extern struct cfdriver gtpci_cd;

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

static const int pci_irqs[2][3] = {
    { IRQ_PCI0_0, IRQ_PCI0_1, IRQ_PCI0_2 },
    { IRQ_PCI1_0, IRQ_PCI1_1, IRQ_PCI1_2 },
};

static const struct pci_init {
	int bar_regno;
	u_int32_t bar_enable;
 	bus_addr_t low_decode;
	bus_addr_t high_decode;
	bus_addr_t barsize;
	bus_addr_t accctl_high;
	bus_addr_t accctl_low;
	bus_addr_t accctl_top;
} pci_initinfo[2][4] = {
    	{
		{
			0x10,			PCI_BARE_SCS0En,
			GT_SCS0_Low_Decode,	GT_SCS0_High_Decode,
			PCI_SCS0_BAR_SIZE(0),
			PCI_ACCESS_CONTROL_BASE_HIGH(0, 0),
			PCI_ACCESS_CONTROL_BASE_LOW(0, 0),
			PCI_ACCESS_CONTROL_TOP(0, 0),
		}, {
			0x14,			PCI_BARE_SCS1En,
			GT_SCS1_Low_Decode,	GT_SCS1_High_Decode,
			PCI_SCS1_BAR_SIZE(0),
			PCI_ACCESS_CONTROL_BASE_HIGH(0, 1),
			PCI_ACCESS_CONTROL_BASE_LOW(0, 1),
			PCI_ACCESS_CONTROL_TOP(0, 1),
		}, {
			0x18,			PCI_BARE_SCS2En,
			GT_SCS2_Low_Decode,	GT_SCS2_High_Decode,
			PCI_SCS2_BAR_SIZE(0),
			PCI_ACCESS_CONTROL_BASE_HIGH(0, 2),
			PCI_ACCESS_CONTROL_BASE_LOW(0, 2),
			PCI_ACCESS_CONTROL_TOP(0, 2),
		}, {
			0x1c,			PCI_BARE_SCS3En,
			GT_SCS3_Low_Decode,	GT_SCS3_High_Decode,
			PCI_SCS3_BAR_SIZE(0),
			PCI_ACCESS_CONTROL_BASE_HIGH(0, 3),
			PCI_ACCESS_CONTROL_BASE_LOW(0, 3),
			PCI_ACCESS_CONTROL_TOP(0, 3),
		},
	}, {
		{
			0x10,			PCI_BARE_SCS0En,
			GT_SCS0_Low_Decode,	GT_SCS0_High_Decode,
			PCI_SCS0_BAR_SIZE(1),
			PCI_ACCESS_CONTROL_BASE_HIGH(1, 0),
			PCI_ACCESS_CONTROL_BASE_LOW(1, 0),
			PCI_ACCESS_CONTROL_TOP(1, 0),
		}, {
			0x14,			PCI_BARE_SCS1En,
			GT_SCS1_Low_Decode,	GT_SCS1_High_Decode,
			PCI_SCS1_BAR_SIZE(1),
			PCI_ACCESS_CONTROL_BASE_HIGH(1, 1),
			PCI_ACCESS_CONTROL_BASE_LOW(1, 1),
			PCI_ACCESS_CONTROL_TOP(1, 1),
		}, {
			0x18,			PCI_BARE_SCS2En,
			GT_SCS2_Low_Decode,	GT_SCS2_High_Decode,
			PCI_SCS2_BAR_SIZE(1),
			PCI_ACCESS_CONTROL_BASE_HIGH(1, 2),
			PCI_ACCESS_CONTROL_BASE_LOW(1, 2),
			PCI_ACCESS_CONTROL_TOP(1, 2),
		}, {
			0x1c,			PCI_BARE_SCS3En,
			GT_SCS3_Low_Decode,	GT_SCS3_High_Decode,
			PCI_SCS3_BAR_SIZE(1),
			PCI_ACCESS_CONTROL_BASE_HIGH(1, 3),
			PCI_ACCESS_CONTROL_BASE_LOW(1, 3),
			PCI_ACCESS_CONTROL_TOP(1, 3),
		},
	}
};

int
gtpci_match(struct device *parent, cfdata_t self, void *aux)
{
	struct gt_softc * const gt = device_private(parent);
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
	struct gt_softc * const gt = device_private(parent);
	struct gtpci_softc * const gtp = device_private(self);
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

	/*
	 * Let's find out where we are located.
	 */
	data = gtpci_read(gtpc, PCI_P2P_CONFIGURATION(gtpc->gtpc_busno));
	gtpc->gtpc_self = gtpci_make_tag(&gtpc->gtpc_pc,
		PCI_P2PCFG_BusNum_GET(data), PCI_P2PCFG_DevNum_GET(data), 0);


	switch (busno) {
	case 0:
		gtpc->gtpc_io_bs = gt->gt_pci0_iot;
		gtpc->gtpc_mem_bs = gt->gt_pci0_memt;
		gtpc->gtpc_host = gt->gt_pci0_host;
		break;
	case 1:
		gtpc->gtpc_io_bs = gt->gt_pci1_iot;
		gtpc->gtpc_mem_bs = gt->gt_pci1_memt;
		gtpc->gtpc_host = gt->gt_pci1_host;
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

	/*
	 * clear any pre-existing error interrupt(s)
	 * clear latched pci error registers
	 * establish ISRs for PCI errors
	 * enable PCI error interrupts
	 */
	gtpci_write(gtpc, PCI_ERROR_MASK(gtpc->gtpc_busno), 0);
	gtpci_write(gtpc, PCI_ERROR_CAUSE(gtpc->gtpc_busno), 0);
	(void)gtpci_read(gtpc, PCI_ERROR_DATA_LOW(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_DATA_HIGH(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_COMMAND(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_ADDRESS_HIGH(gtpc->gtpc_busno));
	(void)gtpci_read(gtpc, PCI_ERROR_ADDRESS_LOW(gtpc->gtpc_busno));
	if (gtpc->gtpc_host) {
		intr_establish(pci_irqs[gtpc->gtpc_busno][0], IST_LEVEL,
		    IPL_VM, gtpci_error_intr, pc);
		intr_establish(pci_irqs[gtpc->gtpc_busno][1], IST_LEVEL,
		    IPL_VM, gtpci_error_intr, pc);
		intr_establish(pci_irqs[gtpc->gtpc_busno][2], IST_LEVEL,
		    IPL_VM, gtpci_error_intr, pc);
		aprint_normal_dev(pc->pc_parent, "%s%d error interrupts at irqs %s, %s, %s\n",
		    "pci", busno,
		    intr_string(pci_irqs[gtpc->gtpc_busno][0]),
		    intr_string(pci_irqs[gtpc->gtpc_busno][1]),
		    intr_string(pci_irqs[gtpc->gtpc_busno][2]));
		gtpci_write(gtpc, PCI_ERROR_MASK(gtpc->gtpc_busno),
		    PCI_SERRMSK_ALL_ERRS);
	}

	/*
	 * Fill in the pci_bus_attach_args
	 */
	pba.pba_pc = pc;
	pba.pba_bus = 0;
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
	/*
	 * Configure the pci bus.
	 */
	config_found_ia(self, "pcibus", &pba, gtpci_cfprint);

	gt_watchdog_service();

}

void
gtpci_bus_init(struct gtpci_chipset *gtpc)
{
	const struct pci_init *pi;
	uint32_t data, datal, datah;
	pcireg_t pcidata;
	int i;

	/*
	 * disable all BARs to start.
	 */
	gtpci_write(gtpc, PCI_BASE_ADDR_REGISTERS_ENABLE(gtpc->gtpc_busno),
	    0xffffffff);

#ifndef GT_PCI0_EXT_ARBITER
#define	GT_PCI0_EXT_ARBITER 0
#endif
#ifndef GT_PCI1_EXT_ARBITER
#define	GT_PCI1_EXT_ARBITER 0
#endif

	if (gtpc->gtpc_host &&
	    ((!GT_PCI0_EXT_ARBITER && gtpc->gtpc_busno == 0) ||
	     (!GT_PCI1_EXT_ARBITER && gtpc->gtpc_busno == 1))) {
		/*
		 * Enable internal arbiter
		 */
		data = gtpci_read(gtpc, PCI_ARBITER_CONTROL(gtpc->gtpc_busno));
		data |= PCI_ARBCTL_EN;
		gtpci_write(gtpc, PCI_ARBITER_CONTROL(gtpc->gtpc_busno), data);
	} else {
		/*
		 * Make sure the internal arbiter is disabled
		 */
		gtpci_write(gtpc, PCI_ARBITER_CONTROL(gtpc->gtpc_busno), 0);
	}

	/*
	 * Make the GT reflects reality.
	 * We always enable internal memory.
	 */
	if (gtpc->gtpc_host) {
		pcidata = gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    0x20) & 0xfff;
		gtpci_conf_write(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x20,
		    GT_LowAddr_GET(gtpci_read(gtpc, GT_Internal_Decode)) |
		    pcidata);
	}
	data = PCI_BARE_IntMemEn;

	for (i = 0, pi = pci_initinfo[gtpc->gtpc_busno]; i < 4; i++, pi++)
		gtpci_write(gtpc, pi->barsize, 0);

	if (gtpc->gtpc_host) {
		/*
		 * Enable bus master access (needed for config access).
		 */
		pcidata = gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    PCI_COMMAND_STATUS_REG);
		pcidata |= PCI_COMMAND_MASTER_ENABLE;
		gtpci_conf_write(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    PCI_COMMAND_STATUS_REG, pcidata);
	}

	/*
	 * Map each SCS BAR to correspond to each SDRAM decode register.
	 */
	for (i = 0, pi = pci_initinfo[gtpc->gtpc_busno]; i < 4; i++, pi++) {
		datal = gtpci_read(gtpc, pi->low_decode);
		datah = gtpci_read(gtpc, pi->high_decode);
		pcidata = gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    pi->bar_regno);
		gtpci_write(gtpc, pi->accctl_high, 0);
		if (datal < datah) {
			datal &= 0xfff;
			pcidata &= 0xfff;
			pcidata |= datal << 20;
			data |= pi->bar_enable;
			datah -= datal;
			datal |= PCI_ACCCTLBASEL_PrefetchEn|
			    PCI_ACCCTLBASEL_RdPrefetch|
			    PCI_ACCCTLBASEL_RdLinePrefetch|
			    PCI_ACCCTLBASEL_RdMulPrefetch|
			    PCI_ACCCTLBASEL_WBurst_8_QW|
			    PCI_ACCCTLBASEL_PCISwap_NoSwap;
			gtpci_write(gtpc, pi->accctl_low, datal);
		} else {
			pcidata &= 0xfff;
			datal = 0xfff|PCI_ACCCTLBASEL_PCISwap_NoSwap;
			datah = 0;
		}
		gtpci_write(gtpc, pi->barsize,
		    datah ? ((datah << 20) | 0xff000) : 0);
		if (gtpc->gtpc_host) {
			gtpci_conf_write(&gtpc->gtpc_pc, gtpc->gtpc_self,
			    pi->bar_regno, pcidata);
		}
		gtpci_write(gtpc, pi->accctl_low, datal);
		gtpci_write(gtpc, pi->accctl_top, datah);
	}

	/*
	 * Now re-enable those BARs that are real.
	 */
	gtpci_write(gtpc, PCI_BASE_ADDR_REGISTERS_ENABLE(gtpc->gtpc_busno),
	    ~data);

	if (gtpc->gtpc_host) {
		/*
		 * Enable I/O and memory (bus master is already enabled) access.
		 */
		pcidata = gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    PCI_COMMAND_STATUS_REG);
		pcidata |= PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE;
		gtpci_conf_write(&gtpc->gtpc_pc, gtpc->gtpc_self,
		    PCI_COMMAND_STATUS_REG, pcidata);
	}
}

void
gtpci_bus_attach_hook(struct device *parent, struct device *self,
	struct pcibus_attach_args *pba)
{
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *) pba->pba_pc;
	uint32_t data;
#if defined(DEBUG)
	pcitag_t tag;
	int bus, dev;
	int i;
#endif

	if (gtpc->gtpc_pc.pc_parent != parent)
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

#if 0
	while ((data & PCI_MODE_PRst) == 0) {
		DELAY(10);
		data = gtpci_read(gtpc, PCI_MODE(gtpc->gtpc_busno));
		aprint_normal(".");
	}
#endif

	gtpci_bus_init(gtpc);
	gtpci_bus_configure(gtpc);

	data = gtpci_read(gtpc, PCI_COMMAND(gtpc->gtpc_busno));
	if (data & (PCI_CMD_MSwapEn|PCI_CMD_SSwapEn)) {
		aprint_normal("\n");
		aprint_normal_dev(self, "");
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

#if defined(DEBUG)
	if (gtpci_debug == 0)
		return;

	data = gtpci_read(gtpc, PCI_BASE_ADDR_REGISTERS_ENABLE(gtpc->gtpc_busno));
	aprint_normal("\n");
	aprint_normal_dev(self, "BARs enabled: %#x", data);

	aprint_normal("\n");
	aprint_normal_dev(self, "0:0:0\n");
	aprint_normal("   %sSCS0=%#010x",
		(data & 1) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x10));
	aprint_normal("/%#010x", gtpci_read(gtpc,
		PCI_SCS0_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS0_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS1=%#010x",
		(data & 2) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS1_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS1_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS2=%#010x",
		(data & 4) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS2_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS2_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sSCS3=%#010x",
		(data & 8) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_SCS3_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_SCS3_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("   %sIMem=%#010x",
		(data & PCI_BARE_IntMemEn) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x20));
	aprint_normal("\n");
	aprint_normal("    %sIIO=%#010x",
		(data & PCI_BARE_IntIOEn) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, gtpc->gtpc_self, 0x24));
	aprint_normal("\n");

	gtpci_decompose_tag(&gtpc->gtpc_pc, gtpc->gtpc_self, &bus, &dev, NULL);
	tag = gtpci_make_tag(&gtpc->gtpc_pc, bus, dev, 1);
	aprint_normal("    %sCS0=%#010x",
		(data & PCI_BARE_CS0En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x10));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS0_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS0_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS1=%#010x",
		(data & PCI_BARE_CS1En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS1_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS1_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS2=%#010x",
		(data & PCI_BARE_CS2En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS2_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS2_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCS3=%#010x",
		(data & PCI_BARE_CS3En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CS3_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CS3_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal(" %sBootCS=%#010x",
		(data & PCI_BARE_BootCSEn) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x20));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_BOOTCS_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_BOOTCS_ADDR_REMAP(gtpc->gtpc_busno)));

	tag = gtpci_make_tag(&gtpc->gtpc_pc, bus, tag, 2);
	aprint_normal("  %sP2PM0=%#010x",
		(data & PCI_BARE_P2PMem0En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x10));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_MEM0_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x.%#010x\n",
		gtpci_read(gtpc, PCI_P2P_MEM0_BASE_ADDR_REMAP_HIGH(gtpc->gtpc_busno)),
		gtpci_read(gtpc, PCI_P2P_MEM0_BASE_ADDR_REMAP_LOW(gtpc->gtpc_busno)));

	aprint_normal("  %sP2PM1=%#010x",
		(data & PCI_BARE_P2PMem1En) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x14));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_MEM1_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x.%#010x\n",
		gtpci_read(gtpc, PCI_P2P_MEM1_BASE_ADDR_REMAP_HIGH(gtpc->gtpc_busno)),
		gtpci_read(gtpc, PCI_P2P_MEM1_BASE_ADDR_REMAP_LOW(gtpc->gtpc_busno)));

	aprint_normal("  %sP2PIO=%#010x",
		(data & PCI_BARE_P2PIOEn) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x18));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_P2P_IO_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_P2P_IO_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	aprint_normal("    %sCPU=%#010x",
		(data & PCI_BARE_CPUEn) ? "-" : "+",
		gtpci_conf_read(&gtpc->gtpc_pc, tag, 0x1c));
	aprint_normal("/%#010x",
		gtpci_read(gtpc, PCI_CPU_BAR_SIZE(gtpc->gtpc_busno)));
	aprint_normal("  remap %#010x\n",
		gtpci_read(gtpc, PCI_CPU_BASE_ADDR_REMAP(gtpc->gtpc_busno)));

	for (i = 0; i < 8; i++) {
		aprint_normal("\n");
		aprint_normal_dev("Access Control %d: ", device_xname(self), i);
		data = gtpci_read(gtpc,
		    PCI_ACCESS_CONTROL_BASE_HIGH(gtpc->gtpc_busno, i));
		if (data)
			aprint_normal("base=0x%08x.", data);
		else
			aprint_normal("base=0x");
		data = gtpci_read(gtpc,
			PCI_ACCESS_CONTROL_BASE_LOW(gtpc->gtpc_busno, i));
		printf("%08x cfg=0x%08x", data << 20, data & ~0xfff);
		aprint_normal(" top=0x%03x00000",
		    gtpci_read(gtpc,
			PCI_ACCESS_CONTROL_TOP(gtpc->gtpc_busno, i)));
	}
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
	cause &= errmask | 0xf8000000;
	gtpci_write(gtpc, PCI_ERROR_CAUSE(gtpc->gtpc_busno), ~cause);
	printf("%s: pci%d error: cause=%#x mask=%#x",
		device_xname(pc->pc_parent), gtpc->gtpc_busno, cause, errmask);
	if ((cause & 0xf8000000) == 0) {
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
	printf("\n%s: pci%d error: %s cmd=%#x",
		device_xname(pc->pc_parent), gtpc->gtpc_busno,
		gtpci_error_strings[PCI_IC_SEL_GET(cause)], cmd);
	if (dhi == 0)
		printf(" data=%08x", dlo);
	else
		printf(" data=%08x.%08x", dhi, dlo);
	if (ahi == 0)
		printf(" address=%08x\n", alo);
	else
		printf(" address=%08x.%08x\n", ahi, alo);

#if defined(DEBUG) && defined(DDB)
	if (gtpci_debug > 1)
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
	if ((regno & 3) || (regno & ~0xff))
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
	if ((regno & 3) || (regno & ~0xff))
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
