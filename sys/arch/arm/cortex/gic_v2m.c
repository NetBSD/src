/* $NetBSD: gic_v2m.c,v 1.1 2018/10/21 00:42:05 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_v2m.c,v 1.1 2018/10/21 00:42:05 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kmem.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/pic/picvar.h>
#include <arm/cortex/gic_v2m.h>

static int
gic_v2m_msi_alloc_spi(struct gic_v2m_frame *frame, int count,
    const struct pci_attach_args *pa)
{
	int spi, n;

	for (spi = frame->frame_base;
	     spi < frame->frame_base + frame->frame_count; ) {
		if (frame->frame_pa[spi] == NULL) {
			for (n = 1; n < count; n++)
				if (frame->frame_pa[spi + n] != NULL)
					goto next_spi;

			for (n = 0; n < count; n++)
				frame->frame_pa[spi + n] = pa;

			return spi;
		}
next_spi:
		spi += count;
	}

	return -1;
}

static void
gic_v2m_msi_free_spi(struct gic_v2m_frame *frame, int spi)
{
	frame->frame_pa[spi] = NULL;
}

static int
gic_v2m_msi_available_spi(struct gic_v2m_frame *frame)
{
	int spi, n;

	for (spi = frame->frame_base, n = 0;
	     spi < frame->frame_base + frame->frame_count;
	     spi++) {
		if (frame->frame_pa[spi] == NULL)
			n++;
	}

	return n;
}

static void
gic_v2m_msi_enable(struct gic_v2m_frame *frame, int spi)
{
	const struct pci_attach_args *pa = frame->frame_pa[spi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("gic_v2m_msi_enable: device is not MSI-capable");

	const uint64_t addr = frame->frame_reg + GIC_MSI_SETSPI;
	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	if (ctl & PCI_MSI_CTL_64BIT_ADDR) {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_LO,
		    addr & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_HI,
		    (addr >> 32) & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA64, spi);
	} else {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR,
		    addr & 0xffffffff);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA, spi);
	}
	ctl |= PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static void
gic_v2m_msi_disable(struct gic_v2m_frame *frame, int spi)
{
	const struct pci_attach_args *pa = frame->frame_pa[spi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("gic_v2m_msi_enable: device is not MSI-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static pci_intr_handle_t *
gic_v2m_msi_alloc(struct arm_pci_msi *msi, int *count,
    const struct pci_attach_args *pa, bool exact)
{
	struct gic_v2m_frame * const frame = msi->msi_priv;
	pci_intr_handle_t *vectors;
	int n;

	const int avail = gic_v2m_msi_available_spi(frame);
	if (exact && *count > avail)
		return NULL;

	while (*count > avail) {
		if (avail < *count)
			(*count) >>= 1;
	}
	if (*count == 0)
		return NULL;

	const int spi_base = gic_v2m_msi_alloc_spi(frame, *count, pa);
	if (spi_base == -1)
		return NULL;

	vectors = kmem_alloc(sizeof(*vectors) * *count, KM_SLEEP);
	for (n = 0; n < *count; n++) {
		const int spi = spi_base + n;
		vectors[n] = ARM_PCI_INTR_MSI |
		    __SHIFTIN(spi, ARM_PCI_INTR_IRQ) |
		    __SHIFTIN(n, ARM_PCI_INTR_MSI_VEC) |
		    __SHIFTIN(msi->msi_id, ARM_PCI_INTR_FRAME);

		gic_v2m_msi_enable(frame, spi);
	}

	return vectors;
}

static void *
gic_v2m_msi_intr_establish(struct arm_pci_msi *msi,
    pci_intr_handle_t ih, int ipl, int (*func)(void *), void *arg)
{
	struct gic_v2m_frame * const frame = msi->msi_priv;

	const int spi = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int mpsafe = (ih & ARM_PCI_INTR_MPSAFE) ? IST_MPSAFE : 0;

	return pic_establish_intr(frame->frame_pic, spi, ipl,
	    IST_EDGE | mpsafe, func, arg);
}

static void
gic_v2m_msi_intr_release(struct arm_pci_msi *msi, pci_intr_handle_t *pih,
    int count)
{
	struct gic_v2m_frame * const frame = msi->msi_priv;
	int n;

	for (n = 0; n < count; n++) {
		const int spi = __SHIFTOUT(pih[n], ARM_PCI_INTR_IRQ);
		gic_v2m_msi_disable(frame, spi);
		gic_v2m_msi_free_spi(frame, spi);
		struct intrsource * const is =
		    frame->frame_pic->pic_sources[spi];
		if (is != NULL)
			pic_disestablish_source(is);
	}
}

int
gic_v2m_init(struct gic_v2m_frame *frame, device_t dev, uint32_t frame_id)
{
	struct arm_pci_msi *msi = &frame->frame_msi;

	msi->msi_dev = dev;
	msi->msi_priv = frame;
	msi->msi_alloc = gic_v2m_msi_alloc;
	msi->msi_intr_establish = gic_v2m_msi_intr_establish;
	msi->msi_intr_release = gic_v2m_msi_intr_release;

	return arm_pci_msi_add(msi);
}
