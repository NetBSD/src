/* $NetBSD: pci_msi_machdep.c,v 1.1 2018/10/21 00:42:06 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_msi_machdep.c,v 1.1 2018/10/21 00:42:06 jmcneill Exp $");

#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/pic/picvar.h>

#include <arm/pci/pci_msi_machdep.h>

static SIMPLEQ_HEAD(, arm_pci_msi) arm_pci_msi_list =
    SIMPLEQ_HEAD_INITIALIZER(arm_pci_msi_list);

static struct arm_pci_msi *
arm_pci_msi_find_frame(pci_intr_handle_t ih)
{
	struct arm_pci_msi *msip;

	const int id = __SHIFTOUT(ih, ARM_PCI_INTR_FRAME);

	SIMPLEQ_FOREACH(msip, &arm_pci_msi_list, msi_link)
		if (id == msip->msi_id)
			return msip;

	return NULL;
}

static int
arm_pci_msi_alloc_common(pci_intr_handle_t **ihps, int *count, const struct pci_attach_args *pa, bool exact)
{
	pci_intr_handle_t *vectors;
	struct arm_pci_msi *msi;

	if ((pa->pa_flags & PCI_FLAGS_MSI_OKAY) == 0)
		return ENODEV;

	msi = SIMPLEQ_FIRST(&arm_pci_msi_list);		/* XXX multiple frame support */
	if (msi == NULL || msi->msi_alloc == NULL)
		return EINVAL;

	vectors = msi->msi_alloc(msi, count, pa, exact);
	if (vectors == NULL)
		return ENOMEM;

	*ihps = vectors;

	return 0;
}

/*
 * arm_pci_msi MD API
 */

int
arm_pci_msi_add(struct arm_pci_msi *msi)
{
	SIMPLEQ_INSERT_TAIL(&arm_pci_msi_list, msi, msi_link);

	return 0;
}

void *
arm_pci_msi_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t pih,
    int ipl, int (*func)(void *), void *arg)
{
	struct arm_pci_msi *msi;

	msi = arm_pci_msi_find_frame(pih);
	if (msi == NULL)
		return NULL;

	return msi->msi_intr_establish(msi, pih, ipl, func, arg);
}

/*
 * pci_msi(9) implementation
 */

int
pci_msi_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, int *count)
{
	return arm_pci_msi_alloc_common(ihps, count, pa, false);
}

int
pci_msi_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, int count)
{
	return arm_pci_msi_alloc_common(ihps, &count, pa, true);
}

int
pci_msix_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, int *count)
{
	return EOPNOTSUPP;
}

int
pci_msix_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, int count)
{
	return EOPNOTSUPP;
}

int
pci_msix_alloc_map(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, u_int *table_indexes, int count)
{
	return EOPNOTSUPP;
}

int
pci_intx_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihp)
{
	pci_intr_handle_t *pih;

	if (ihp == NULL)
		return EINVAL;

	pih = kmem_alloc(sizeof(*pih), KM_SLEEP);
	if (pci_intr_map(pa, pih) != 0) {
		kmem_free(pih, sizeof(*pih));
		return EINVAL;
	}
	*ihp = pih;

	return 0;
}

int
pci_intr_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps, int *counts, pci_intr_type_t max_type)
{
	int intx_count, msi_count, msix_count, error;

	error = EINVAL;
	intx_count = 1;
	msi_count = 1;
	msix_count = 0;

	if (counts != NULL) {
		switch (max_type) {
		case PCI_INTR_TYPE_MSIX:
			msix_count = counts[PCI_INTR_TYPE_MSIX];
			/* FALLTHROUGH */
		case PCI_INTR_TYPE_MSI:
			msi_count = counts[PCI_INTR_TYPE_MSI];
			/* FALLTHROUGH */
		case PCI_INTR_TYPE_INTX:
			intx_count = counts[PCI_INTR_TYPE_INTX];
			if (intx_count > 1)
				return EINVAL;
			break;
		default:
			return EINVAL;
		}
		memset(counts, 0, sizeof(*counts) * PCI_INTR_TYPE_SIZE);
	}

	if (msix_count == -1)
		msix_count = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (msix_count > 0 && (error = pci_msix_alloc_exact(pa, ihps, msix_count)) == 0) {
		if (counts == NULL)
			return EINVAL;
		counts[PCI_INTR_TYPE_MSIX] = msix_count;
		return 0;
	}

	if (msi_count == -1)
		msi_count = pci_msi_count(pa->pa_pc, pa->pa_tag);
	if (msi_count > 0 && (error = pci_msi_alloc_exact(pa, ihps, msi_count)) == 0) {
		if (counts != NULL)
			counts[PCI_INTR_TYPE_MSI] = msi_count;
		return 0;
	}

	if (intx_count > 0 && (error = pci_intx_alloc(pa, ihps)) == 0) {
		if (counts != NULL)
			counts[PCI_INTR_TYPE_INTX] = intx_count;
		return 0;
	}

	return error;
}

void 
pci_intr_release(pci_chipset_tag_t pc, pci_intr_handle_t *pih, int count)
{
	struct arm_pci_msi *msi;

	if (pih == NULL || count == 0)
		return;

	msi = arm_pci_msi_find_frame(pih[0]);
	KASSERT(msi != NULL);
	msi->msi_intr_release(msi, pih, count);

	kmem_free(pih, sizeof(*pih) * count);
}

pci_intr_type_t
pci_intr_type(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	if (ih & ARM_PCI_INTR_MSIX)
		return PCI_INTR_TYPE_MSIX;

	if (ih & ARM_PCI_INTR_MSI)
		return PCI_INTR_TYPE_MSI;

	return PCI_INTR_TYPE_INTX;
}
