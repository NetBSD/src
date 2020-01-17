/* $NetBSD: gicv3_its.c,v 1.23.2.1 2020/01/17 21:47:23 ad Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gicv3_its.c,v 1.23.2.1 2020/01/17 21:47:23 ad Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/bitops.h>

#include <uvm/uvm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/pic/picvar.h>
#include <arm/cortex/gicv3_its.h>

/*
 * ITS translation table sizes
 */
#define	GITS_COMMANDS_SIZE	0x1000
#define	GITS_COMMANDS_ALIGN	0x10000

#define	GITS_ITT_ALIGN		0x100

/*
 * IIDR values used for errata
 */
#define GITS_IIDR_PID_CAVIUM_THUNDERX	0xa1
#define GITS_IIDR_IMP_CAVIUM		0x34c
#define	GITS_IIDR_CAVIUM_ERRATA_MASK	(GITS_IIDR_Implementor|GITS_IIDR_ProductID|GITS_IIDR_Variant)
#define	GITS_IIDR_CAVIUM_ERRATA_VALUE							\
		(__SHIFTIN(GITS_IIDR_IMP_CAVIUM, GITS_IIDR_Implementor) |		\
		 __SHIFTIN(GITS_IIDR_PID_CAVIUM_THUNDERX, GITS_IIDR_ProductID) |	\
		 __SHIFTIN(0, GITS_IIDR_Variant))

static const char * gits_cache_type[] = {
	[GITS_Cache_DEVICE_nGnRnE]	= "Device-nGnRnE",
	[GITS_Cache_NORMAL_NC]		= "Non-cacheable",
	[GITS_Cache_NORMAL_RA_WT]	= "Cacheable RA WT",
	[GITS_Cache_NORMAL_RA_WB]	= "Cacheable RA WB",
	[GITS_Cache_NORMAL_WA_WT]	= "Cacheable WA WT",
	[GITS_Cache_NORMAL_WA_WB]	= "Cacheable WA WB",
	[GITS_Cache_NORMAL_RA_WA_WT]	= "Cacheable RA WA WT",
	[GITS_Cache_NORMAL_RA_WA_WB]	= "Cacheable RA WA WB",
};

static const char * gits_share_type[] = {
	[GITS_Shareability_NS]		= "Non-shareable",
	[GITS_Shareability_IS]		= "Inner shareable",
	[GITS_Shareability_OS]		= "Outer shareable",
	[3]				= "(Reserved)",
};

static inline uint32_t
gits_read_4(struct gicv3_its *its, bus_size_t reg)
{
	return bus_space_read_4(its->its_bst, its->its_bsh, reg);
}

static inline void
gits_write_4(struct gicv3_its *its, bus_size_t reg, uint32_t val)
{
	bus_space_write_4(its->its_bst, its->its_bsh, reg, val);
}

static inline uint64_t
gits_read_8(struct gicv3_its *its, bus_size_t reg)
{
	return bus_space_read_8(its->its_bst, its->its_bsh, reg);
}

static inline void
gits_write_8(struct gicv3_its *its, bus_size_t reg, uint64_t val)
{
	bus_space_write_8(its->its_bst, its->its_bsh, reg, val);
}

static inline void
gits_command(struct gicv3_its *its, const struct gicv3_its_command *cmd)
{
	uint64_t cwriter;
	u_int woff;

	cwriter = gits_read_8(its, GITS_CWRITER);
	woff = cwriter & GITS_CWRITER_Offset;

	memcpy(its->its_cmd.base + woff, cmd->dw, sizeof(cmd->dw));
	bus_dmamap_sync(its->its_dmat, its->its_cmd.map, woff, sizeof(cmd->dw), BUS_DMASYNC_PREWRITE);

	woff += sizeof(cmd->dw);
	if (woff == its->its_cmd.len)
		woff = 0;

	gits_write_8(its, GITS_CWRITER, woff);
}

static inline void
gits_command_mapc(struct gicv3_its *its, uint16_t icid, uint64_t rdbase, bool v)
{
	struct gicv3_its_command cmd;

	KASSERT((rdbase & 0xffff) == 0);

	/*
	 * Map a collection table entry (ICID) to the target redistributor (RDbase).
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_MAPC;
	cmd.dw[2] = icid;
	if (v) {
		cmd.dw[2] |= rdbase;
		cmd.dw[2] |= __BIT(63);
	}

	gits_command(its, &cmd);
}

static inline void
gits_command_mapd(struct gicv3_its *its, uint32_t deviceid, uint64_t itt_addr, u_int size, bool v)
{
	struct gicv3_its_command cmd;

	KASSERT((itt_addr & 0xff) == 0);

	/*
	 * Map a device table entry (DeviceID) to its associated ITT (ITT_addr).
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_MAPD | ((uint64_t)deviceid << 32);
	cmd.dw[1] = size;
	if (v) {
		cmd.dw[2] = itt_addr | __BIT(63);
	}

	gits_command(its, &cmd);
}

static inline void
gits_command_mapti(struct gicv3_its *its, uint32_t deviceid, uint32_t eventid, uint32_t pintid, uint16_t icid)
{
	struct gicv3_its_command cmd;

	/*
	 * Map the event defined by EventID and DeviceID to its associated ITE, defined by ICID and pINTID
	 * in the ITT associated with DeviceID.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_MAPTI | ((uint64_t)deviceid << 32);
	cmd.dw[1] = eventid | ((uint64_t)pintid << 32);
	cmd.dw[2] = icid;

	gits_command(its, &cmd);
}

static inline void
gits_command_movi(struct gicv3_its *its, uint32_t deviceid, uint32_t eventid, uint16_t icid)
{
	struct gicv3_its_command cmd;

	/*
	 * Update the ICID field in the ITT entry for the event defined by DeviceID and
	 * EventID.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_MOVI | ((uint64_t)deviceid << 32);
	cmd.dw[1] = eventid;
	cmd.dw[2] = icid;

	gits_command(its, &cmd);
}

static inline void
gits_command_inv(struct gicv3_its *its, uint32_t deviceid, uint32_t eventid)
{
	struct gicv3_its_command cmd;

	/*
	 * Ensure any caching in the redistributors associated with the specified
	 * EventID is consistent with the LPI configuration tables.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_INV | ((uint64_t)deviceid << 32);
	cmd.dw[1] = eventid;

	gits_command(its, &cmd);
}

static inline void
gits_command_invall(struct gicv3_its *its, uint16_t icid)
{
	struct gicv3_its_command cmd;

	/*
	 * Ensure any caching associated with this ICID is consistent with LPI
	 * configuration tables for all redistributors.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_INVALL;
	cmd.dw[2] = icid;

	gits_command(its, &cmd);
}

static inline void
gits_command_sync(struct gicv3_its *its, uint64_t rdbase)
{
	struct gicv3_its_command cmd;

	KASSERT((rdbase & 0xffff) == 0);

	/*
	 * Ensure all outstanding ITS operations associated with physical interrupts
	 * for the specified redistributor (RDbase) are globally observed before
	 * further ITS commands are executed.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.dw[0] = GITS_CMD_SYNC;
	cmd.dw[2] = rdbase;

	gits_command(its, &cmd);
}

static inline int
gits_wait(struct gicv3_its *its)
{
	u_int woff, roff;
	int retry = 100000;

	/*
	 * The ITS command queue is empty when CWRITER and CREADR specify the
	 * same base address offset value.
	 */
	for (retry = 1000; retry > 0; retry--) {
		woff = gits_read_8(its, GITS_CWRITER) & GITS_CWRITER_Offset;
		roff = gits_read_8(its, GITS_CREADR) & GITS_CREADR_Offset;
		if (woff == roff)
			break;
		delay(100);
	}
	if (retry == 0) {
		device_printf(its->its_gic->sc_dev, "ITS command queue timeout\n");
		return ETIMEDOUT;
	}

	return 0;
}

static int
gicv3_its_msi_alloc_lpi(struct gicv3_its *its,
    const struct pci_attach_args *pa)
{
	struct pci_attach_args *new_pa;
	int n;

	for (n = 0; n < its->its_pic->pic_maxsources; n++) {
		if (its->its_pa[n] == NULL) {
			new_pa = kmem_alloc(sizeof(*new_pa), KM_SLEEP);
			memcpy(new_pa, pa, sizeof(*new_pa));
			its->its_pa[n] = new_pa;
			return n + its->its_pic->pic_irqbase;
		}
	}

        return -1;
}

static void
gicv3_its_msi_free_lpi(struct gicv3_its *its, int lpi)
{
	struct pci_attach_args *pa;

	KASSERT(lpi >= its->its_pic->pic_irqbase);

	pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	its->its_pa[lpi - its->its_pic->pic_irqbase] = NULL;
	kmem_free(pa, sizeof(*pa));
}

static uint32_t
gicv3_its_devid(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint32_t devid;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	devid = (b << 8) | (d << 3) | f;

	return pci_get_devid(pc, devid);
}

static int
gicv3_its_device_map(struct gicv3_its *its, uint32_t devid, u_int count)
{
	struct gicv3_its_device *dev;
	u_int vectors;

	vectors = MAX(2, count);
	while (!powerof2(vectors))
		vectors++;

	const uint64_t typer = gits_read_8(its, GITS_TYPER);
	const u_int itt_entry_size = __SHIFTOUT(typer, GITS_TYPER_ITT_entry_size) + 1;
	const u_int itt_size = roundup(vectors * itt_entry_size, GITS_ITT_ALIGN);

	LIST_FOREACH(dev, &its->its_devices, dev_list)
		if (dev->dev_id == devid) {
			return itt_size <= dev->dev_size ? 0 : EEXIST;
		}

	dev = kmem_alloc(sizeof(*dev), KM_SLEEP);
	dev->dev_id = devid;
	dev->dev_size = itt_size;
	gicv3_dma_alloc(its->its_gic, &dev->dev_itt, itt_size, GITS_ITT_ALIGN);
	LIST_INSERT_HEAD(&its->its_devices, dev, dev_list);

	/*
	 * Map the device to the ITT
	 */
	const u_int id_bits = __SHIFTOUT(typer, GITS_TYPER_ID_bits) + 1;
	gits_command_mapd(its, devid, dev->dev_itt.segs[0].ds_addr, id_bits - 1, true);
	gits_wait(its);

	return 0;
}

static void
gicv3_its_msi_enable(struct gicv3_its *its, int lpi, int count)
{
	const struct pci_attach_args *pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("gicv3_its_msi_enable: device is not MSI-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MME_MASK;
	ctl |= __SHIFTIN(ilog2(count), PCI_MSI_CTL_MME_MASK);
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);

	const uint64_t addr = its->its_base + GITS_TRANSLATER;
	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	if (ctl & PCI_MSI_CTL_64BIT_ADDR) {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_LO,
		    addr & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_HI,
		    (addr >> 32) & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA64,
		    lpi - its->its_pic->pic_irqbase);
	} else {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR,
		    addr & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA,
		    lpi - its->its_pic->pic_irqbase);
	}
	ctl |= PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static void
gicv3_its_msi_disable(struct gicv3_its *its, int lpi)
{
	const struct pci_attach_args *pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("gicv3_its_msi_enable: device is not MSI-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static void
gicv3_its_msix_enable(struct gicv3_its *its, int lpi, int msix_vec,
    bus_space_tag_t bst, bus_space_handle_t bsh)
{
	const struct pci_attach_args *pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL))
		panic("gicv3_its_msix_enable: device is not MSI-X-capable");

	const uint64_t addr = its->its_base + GITS_TRANSLATER;
	const uint64_t entry_base = PCI_MSIX_TABLE_ENTRY_SIZE * msix_vec;
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_LO, (uint32_t)addr);
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_HI, (uint32_t)(addr >> 32));
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_DATA, lpi - its->its_pic->pic_irqbase);
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL, 0);

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl |= PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
}

static void
gicv3_its_msix_disable(struct gicv3_its *its, int lpi)
{
	const struct pci_attach_args *pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL))
		panic("gicv3_its_msix_disable: device is not MSI-X-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl &= ~PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
}

static pci_intr_handle_t *
gicv3_its_msi_alloc(struct arm_pci_msi *msi, int *count,
    const struct pci_attach_args *pa, bool exact)
{
	struct gicv3_its * const its = msi->msi_priv;
	struct cpu_info * const ci = cpu_lookup(0);
	pci_intr_handle_t *vectors;
	int n, off;

	if (!pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSI, &off, NULL))
		return NULL;

	const uint64_t typer = gits_read_8(its, GITS_TYPER);
	const u_int id_bits = __SHIFTOUT(typer, GITS_TYPER_ID_bits) + 1;
	if (*count == 0 || *count > (1 << id_bits))
		return NULL;

	const uint32_t devid = gicv3_its_devid(pa->pa_pc, pa->pa_tag);

	if (gicv3_its_device_map(its, devid, *count) != 0)
		return NULL;

	vectors = kmem_alloc(sizeof(*vectors) * *count, KM_SLEEP);
	for (n = 0; n < *count; n++) {
		const int lpi = gicv3_its_msi_alloc_lpi(its, pa);
		vectors[n] = ARM_PCI_INTR_MSI |
		    __SHIFTIN(lpi, ARM_PCI_INTR_IRQ) |
		    __SHIFTIN(n, ARM_PCI_INTR_MSI_VEC) |
		    __SHIFTIN(msi->msi_id, ARM_PCI_INTR_FRAME);

		if (n == 0)
			gicv3_its_msi_enable(its, lpi, *count);

		/*
		 * Record target PE
		 */
		its->its_targets[lpi - its->its_pic->pic_irqbase] = ci;

		/*
		 * Map event
		 */
		gits_command_mapti(its, devid, lpi - its->its_pic->pic_irqbase, lpi, cpu_index(ci));
		gits_command_sync(its, its->its_rdbase[cpu_index(ci)]);
	}
	gits_wait(its);

	return vectors;
}

static pci_intr_handle_t *
gicv3_its_msix_alloc(struct arm_pci_msi *msi, u_int *table_indexes, int *count,
    const struct pci_attach_args *pa, bool exact)
{
	struct gicv3_its * const its = msi->msi_priv;
	struct cpu_info *ci = cpu_lookup(0);
	pci_intr_handle_t *vectors;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t bsz;
	uint32_t table_offset, table_size;
	int n, off, bar, error;
	pcireg_t tbl;

	if (!pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSIX, &off, NULL))
		return NULL;

	const uint64_t typer = gits_read_8(its, GITS_TYPER);
	const u_int id_bits = __SHIFTOUT(typer, GITS_TYPER_ID_bits) + 1;
	if (*count == 0 || *count > (1 << id_bits))
		return NULL;

	tbl = pci_conf_read(pa->pa_pc, pa->pa_tag, off + PCI_MSIX_TBLOFFSET);
	bar = PCI_BAR0 + (4 * (tbl & PCI_MSIX_TBLBIR_MASK));
	table_offset = tbl & PCI_MSIX_TBLOFFSET_MASK;
	table_size = pci_msix_count(pa->pa_pc, pa->pa_tag) * PCI_MSIX_TABLE_ENTRY_SIZE;
	if (table_size == 0)
		return NULL;

	error = pci_mapreg_submap(pa, bar, pci_mapreg_type(pa->pa_pc, pa->pa_tag, bar),
	    BUS_SPACE_MAP_LINEAR, roundup(table_size, PAGE_SIZE), table_offset,
	    &bst, &bsh, NULL, &bsz);
	if (error)
		return NULL;

	const uint32_t devid = gicv3_its_devid(pa->pa_pc, pa->pa_tag);

	if (gicv3_its_device_map(its, devid, *count) != 0) {
		bus_space_unmap(bst, bsh, bsz);
		return NULL;
	}

	vectors = kmem_alloc(sizeof(*vectors) * *count, KM_SLEEP);
	for (n = 0; n < *count; n++) {
		const int lpi = gicv3_its_msi_alloc_lpi(its, pa);
		const int msix_vec = table_indexes ? table_indexes[n] : n;
		vectors[msix_vec] = ARM_PCI_INTR_MSIX |
		    __SHIFTIN(lpi, ARM_PCI_INTR_IRQ) |
		    __SHIFTIN(msix_vec, ARM_PCI_INTR_MSI_VEC) |
		    __SHIFTIN(msi->msi_id, ARM_PCI_INTR_FRAME);

		gicv3_its_msix_enable(its, lpi, msix_vec, bst, bsh);

		/*
		 * Record target PE
		 */
		its->its_targets[lpi - its->its_pic->pic_irqbase] = ci;

		/*
		 * Map event
		 */
		gits_command_mapti(its, devid, lpi - its->its_pic->pic_irqbase, lpi, cpu_index(ci));
		gits_command_sync(its, its->its_rdbase[cpu_index(ci)]);
	}
	gits_wait(its);

	bus_space_unmap(bst, bsh, bsz);

	return vectors;
}

static void *
gicv3_its_msi_intr_establish(struct arm_pci_msi *msi,
    pci_intr_handle_t ih, int ipl, int (*func)(void *), void *arg, const char *xname)
{
	struct gicv3_its * const its = msi->msi_priv;
	const struct pci_attach_args *pa;
	void *intrh;

	const int lpi = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int mpsafe = (ih & ARM_PCI_INTR_MPSAFE) ? IST_MPSAFE : 0;

	intrh = pic_establish_intr(its->its_pic, lpi - its->its_pic->pic_irqbase, ipl,
	    IST_EDGE | mpsafe, func, arg, xname);
	if (intrh == NULL)
		return NULL;

	/* Invalidate LPI configuration tables */
	pa = its->its_pa[lpi - its->its_pic->pic_irqbase];
	KASSERT(pa != NULL);
	const uint32_t devid = gicv3_its_devid(pa->pa_pc, pa->pa_tag);
	gits_command_inv(its, devid, lpi - its->its_pic->pic_irqbase);

	return intrh;
}

static void
gicv3_its_msi_intr_release(struct arm_pci_msi *msi, pci_intr_handle_t *pih,
    int count)
{
	struct gicv3_its * const its = msi->msi_priv;
	int n;

	for (n = 0; n < count; n++) {
		const int lpi = __SHIFTOUT(pih[n], ARM_PCI_INTR_IRQ);
		KASSERT(lpi >= its->its_pic->pic_irqbase);
		if (pih[n] & ARM_PCI_INTR_MSIX)
			gicv3_its_msix_disable(its, lpi);
		if (pih[n] & ARM_PCI_INTR_MSI)
			gicv3_its_msi_disable(its, lpi);
		gicv3_its_msi_free_lpi(its, lpi);
		its->its_targets[lpi - its->its_pic->pic_irqbase] = NULL;
		struct intrsource * const is =
		    its->its_pic->pic_sources[lpi - its->its_pic->pic_irqbase];
		if (is != NULL)
			pic_disestablish_source(is);
	}
}

static void
gicv3_its_command_init(struct gicv3_softc *sc, struct gicv3_its *its)
{
	uint64_t cbaser;

	gicv3_dma_alloc(sc, &its->its_cmd, GITS_COMMANDS_SIZE, GITS_COMMANDS_ALIGN);

	cbaser = its->its_cmd.segs[0].ds_addr;
	cbaser |= __SHIFTIN(GITS_Cache_NORMAL_NC, GITS_CBASER_InnerCache);
	cbaser |= __SHIFTIN(GITS_Shareability_NS, GITS_CBASER_Shareability);
	cbaser |= __SHIFTIN((its->its_cmd.len / 4096) - 1, GITS_CBASER_Size);
	cbaser |= GITS_CBASER_Valid;

	gits_write_8(its, GITS_CBASER, cbaser);
	gits_write_8(its, GITS_CWRITER, 0);
}

static void
gicv3_its_table_params(struct gicv3_softc *sc, struct gicv3_its *its,
    u_int *devbits, u_int *innercache, u_int *share)
{

	const uint64_t typer = gits_read_8(its, GITS_TYPER);
	const uint32_t iidr = gits_read_4(its, GITS_IIDR);

	/* Default values */
	*devbits = __SHIFTOUT(typer, GITS_TYPER_Devbits) + 1;
	*innercache = GITS_Cache_NORMAL_WA_WB;
	*share = GITS_Shareability_IS;

	/* Cavium ThunderX errata */
	if ((iidr & GITS_IIDR_CAVIUM_ERRATA_MASK) == GITS_IIDR_CAVIUM_ERRATA_VALUE) {
		*devbits = 20;		/* 8Mb */
		*innercache = GITS_Cache_DEVICE_nGnRnE;
		aprint_normal_dev(sc->sc_dev, "Cavium ThunderX errata detected\n");
	}
}

static void
gicv3_its_table_init(struct gicv3_softc *sc, struct gicv3_its *its)
{
	u_int table_size, page_size, table_align;
	u_int devbits, innercache, share;
	const char *table_type;
	uint64_t baser;
	int tab;

	gicv3_its_table_params(sc, its, &devbits, &innercache, &share);

	for (tab = 0; tab < 8; tab++) {
		baser = gits_read_8(its, GITS_BASERn(tab));

		const u_int entry_size = __SHIFTOUT(baser, GITS_BASER_Entry_Size) + 1;

		switch (__SHIFTOUT(baser, GITS_BASER_Page_Size)) {
		case GITS_Page_Size_4KB:
			page_size = 4096;
			table_align = 4096;
			break;
		case GITS_Page_Size_16KB:
			page_size = 16384;
			table_align = 4096;
			break;
		case GITS_Page_Size_64KB:
		default:
			page_size = 65536;
			table_align = 65536;
			break;
		}

		switch (__SHIFTOUT(baser, GITS_BASER_Type)) {
		case GITS_Type_Devices:
			/*
			 * Table size scales with the width of the DeviceID.
			 */
			table_size = roundup(entry_size * (1 << devbits), page_size);
			table_type = "Devices";
			break;
		case GITS_Type_InterruptCollections:
			/*
			 * Allocate space for one interrupt collection per CPU.
			 */
			table_size = roundup(entry_size * MAXCPUS, page_size);
			table_type = "Collections";
			break;
		default:
			table_size = 0;
			break;
		}

		if (table_size == 0)
			continue;

		gicv3_dma_alloc(sc, &its->its_tab[tab], table_size, table_align);

		baser &= ~GITS_BASER_Size;
		baser |= __SHIFTIN(table_size / page_size - 1, GITS_BASER_Size);
		baser &= ~GITS_BASER_Physical_Address;
		baser |= its->its_tab[tab].segs[0].ds_addr;
		baser &= ~GITS_BASER_InnerCache;
		baser |= __SHIFTIN(innercache, GITS_BASER_InnerCache);
		baser &= ~GITS_BASER_Shareability;
		baser |= __SHIFTIN(share, GITS_BASER_Shareability);
		baser |= GITS_BASER_Valid;

		gits_write_8(its, GITS_BASERn(tab), baser);

		baser = gits_read_8(its, GITS_BASERn(tab));
		if (__SHIFTOUT(baser, GITS_BASER_Shareability) == GITS_Shareability_NS) {
			baser &= ~GITS_BASER_InnerCache;
			baser |= __SHIFTIN(GITS_Cache_NORMAL_NC, GITS_BASER_InnerCache);

			gits_write_8(its, GITS_BASERn(tab), baser);
		}

		baser = gits_read_8(its, GITS_BASERn(tab));
		aprint_normal_dev(sc->sc_dev, "ITS [#%d] %s table @ %#lx/%#x, %s, %s\n",
		    tab, table_type, its->its_tab[tab].segs[0].ds_addr, table_size,
		    gits_cache_type[__SHIFTOUT(baser, GITS_BASER_InnerCache)],
		    gits_share_type[__SHIFTOUT(baser, GITS_BASER_Shareability)]);
	}
}

static void
gicv3_its_enable(struct gicv3_softc *sc, struct gicv3_its *its)
{
	uint32_t ctlr;

	ctlr = gits_read_4(its, GITS_CTLR);
	ctlr |= GITS_CTLR_Enabled;
	gits_write_4(its, GITS_CTLR, ctlr);
}

static void
gicv3_its_cpu_init(void *priv, struct cpu_info *ci)
{
	struct gicv3_its * const its = priv;
	struct gicv3_softc * const sc = its->its_gic;
	const struct pci_attach_args *pa;
	uint64_t rdbase;
	size_t irq;

	const uint64_t typer = bus_space_read_8(sc->sc_bst, its->its_bsh, GITS_TYPER);
	if (typer & GITS_TYPER_PTA) {
		void *va = bus_space_vaddr(sc->sc_bst, sc->sc_bsh_r[ci->ci_gic_redist]);
		rdbase = vtophys((vaddr_t)va);
	} else {
		rdbase = (uint64_t)sc->sc_processor_id[cpu_index(ci)] << 16;
	}
	its->its_rdbase[cpu_index(ci)] = rdbase;

	/*
	 * Map collection ID of this CPU's index to this CPU's redistributor.
	 */
	gits_command_mapc(its, cpu_index(ci), rdbase, true);
	gits_command_invall(its, cpu_index(ci));
	gits_wait(its);

	/*
	 * Update routing for LPIs targetting this CPU
	 */
	for (irq = 0; irq < its->its_pic->pic_maxsources; irq++) {
		if (its->its_targets[irq] != ci)
			continue;
		pa = its->its_pa[irq];
		KASSERT(pa != NULL);

		const uint32_t devid = gicv3_its_devid(pa->pa_pc, pa->pa_tag);
		gits_command_movi(its, devid, irq, cpu_index(ci));
		gits_command_sync(its, its->its_rdbase[cpu_index(ci)]);
	}

	its->its_cpuonline[cpu_index(ci)] = true;
}

static void
gicv3_its_get_affinity(void *priv, size_t irq, kcpuset_t *affinity)
{
	struct gicv3_its * const its = priv;
	struct cpu_info *ci;

	kcpuset_zero(affinity);
	ci = its->its_targets[irq];
	if (ci)
		kcpuset_set(affinity, cpu_index(ci));
}

static int
gicv3_its_set_affinity(void *priv, size_t irq, const kcpuset_t *affinity)
{
	struct gicv3_its * const its = priv;
	const struct pci_attach_args *pa;
	struct cpu_info *ci;

	const int set = kcpuset_countset(affinity);
	if (set != 1)
		return EINVAL;

	pa = its->its_pa[irq];
	if (pa == NULL)
		return EINVAL;

	ci = cpu_lookup(kcpuset_ffs(affinity) - 1);
	its->its_targets[irq] = ci;

	if (its->its_cpuonline[cpu_index(ci)] == true) {
		const uint32_t devid = gicv3_its_devid(pa->pa_pc, pa->pa_tag);
		gits_command_movi(its, devid, irq, cpu_index(ci));
		gits_command_sync(its, its->its_rdbase[cpu_index(ci)]);
	}

	return 0;
}

int
gicv3_its_init(struct gicv3_softc *sc, bus_space_handle_t bsh,
    uint64_t its_base, uint32_t its_id)
{
	struct gicv3_its *its;
	struct arm_pci_msi *msi;

	const uint64_t typer = bus_space_read_8(sc->sc_bst, bsh, GITS_TYPER);
	if ((typer & GITS_TYPER_Physical) == 0)
		return ENXIO;

	its = kmem_alloc(sizeof(*its), KM_SLEEP);
	its->its_id = its_id;
	its->its_bst = sc->sc_bst;
	its->its_bsh = bsh;
	its->its_dmat = sc->sc_dmat;
	its->its_base = its_base;
	its->its_pic = &sc->sc_lpi;
	snprintf(its->its_pic->pic_name, sizeof(its->its_pic->pic_name), "gicv3-its");
	KASSERT(its->its_pic->pic_maxsources > 0);
	its->its_pa = kmem_zalloc(sizeof(struct pci_attach_args *) * its->its_pic->pic_maxsources, KM_SLEEP);
	its->its_targets = kmem_zalloc(sizeof(struct cpu_info *) * its->its_pic->pic_maxsources, KM_SLEEP);
	its->its_gic = sc;
	its->its_cb.cpu_init = gicv3_its_cpu_init;
	its->its_cb.get_affinity = gicv3_its_get_affinity;
	its->its_cb.set_affinity = gicv3_its_set_affinity;
	its->its_cb.priv = its;
	LIST_INIT(&its->its_devices);
	LIST_INSERT_HEAD(&sc->sc_lpi_callbacks, &its->its_cb, list);

	gicv3_its_command_init(sc, its);
	gicv3_its_table_init(sc, its);

	gicv3_its_enable(sc, its);

	gicv3_its_cpu_init(its, curcpu());

	msi = &its->its_msi;
	msi->msi_id = its_id;
	msi->msi_dev = sc->sc_dev;
	msi->msi_priv = its;
	msi->msi_alloc = gicv3_its_msi_alloc;
	msi->msix_alloc = gicv3_its_msix_alloc;
	msi->msi_intr_establish = gicv3_its_msi_intr_establish;
	msi->msi_intr_release = gicv3_its_msi_intr_release;

	return arm_pci_msi_add(msi);
}
