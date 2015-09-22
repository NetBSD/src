/*	$NetBSD: pci_msi_machdep.c,v 1.5.2.3 2015/09/22 12:05:54 skrll Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan Inc.
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

/*
 * TODO
 *
 *     - PBA (Pending Bit Array) support
 *     - HyperTransport mapping support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_msi_machdep.c,v 1.5.2.3 2015/09/22 12:05:54 skrll Exp $");

#include "opt_intrdebug.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>

#include <machine/i82093var.h>
#include <machine/pic.h>

#include <x86/pci/msipic.h>
#include <x86/pci/pci_msi_machdep.h>

#ifdef INTRDEBUG
#define MSIDEBUG
#endif

#ifdef MSIDEBUG
#define DPRINTF(msg) printf msg
#else
#define DPRINTF(msg)
#endif

static pci_intr_handle_t
pci_msi_calculate_handle(struct pic *msi_pic, int vector)
{
	pci_intr_handle_t pih;

	KASSERT(msipic_is_msi_pic(msi_pic));

	pih = __SHIFTIN((uint64_t)msipic_get_devid(msi_pic), MSI_INT_DEV_MASK)
	    | __SHIFTIN((uint64_t)vector, MSI_INT_VEC_MASK)
	    | APIC_INT_VIA_MSI;
	if (msi_pic->pic_type == PIC_MSI)
		MSI_INT_MAKE_MSI(pih);
	else if (msi_pic->pic_type == PIC_MSIX)
		MSI_INT_MAKE_MSIX(pih);
	else
		panic("%s: Unexpected pic_type: %d\n", __func__,
		    msi_pic->pic_type);

	return pih;
}

static pci_intr_handle_t *
pci_msi_alloc_vectors(struct pic *msi_pic, uint *table_indexes, int *count)
{
	struct intrsource *isp;
	pci_intr_handle_t *vectors, pih;
	int i;
	const char *intrstr;
	char intrstr_buf[INTRIDBUF];

	vectors = kmem_zalloc(sizeof(vectors[0]) * (*count), KM_SLEEP);
	if (vectors == NULL) {
		DPRINTF(("cannot allocate vectors\n"));
		return NULL;
	}

	mutex_enter(&cpu_lock);
	for (i = 0; i < *count; i++) {
		u_int table_index;

		if (table_indexes == NULL)
			table_index = i;
		else
			table_index = table_indexes[i];

		pih = pci_msi_calculate_handle(msi_pic, table_index);

		intrstr = x86_pci_msi_string(NULL, pih, intrstr_buf,
		    sizeof(intrstr_buf));
		isp = intr_allocate_io_intrsource(intrstr);
		if (isp == NULL) {
			mutex_exit(&cpu_lock);
			DPRINTF(("can't allocate io_intersource\n"));
			kmem_free(vectors, sizeof(vectors[0]) * (*count));
			return NULL;
		}

		vectors[i] = pih;
	}
	mutex_exit(&cpu_lock);

	return vectors;
}

static void
pci_msi_free_vectors(struct pic *msi_pic, pci_intr_handle_t *pihs, int count)
{
	pci_intr_handle_t pih;
	int i;
	const char *intrstr;
	char intrstr_buf[INTRIDBUF];

	mutex_enter(&cpu_lock);
	for (i = 0; i < count; i++) {
		pih = pci_msi_calculate_handle(msi_pic, i);
		intrstr = x86_pci_msi_string(NULL, pih, intrstr_buf,
		    sizeof(intrstr_buf));
		intr_free_io_intrsource(intrstr);
	}
	mutex_exit(&cpu_lock);

	kmem_free(pihs, sizeof(pihs[0]) * count);
}

static int
pci_msi_alloc_common(pci_intr_handle_t **ihps, int *count,
    const struct pci_attach_args *pa, bool exact)
{
	struct pic *msi_pic;
	pci_intr_handle_t *vectors;
	int error, i;

	if ((pa->pa_flags & PCI_FLAGS_MSI_OKAY) == 0) {
		DPRINTF(("PCI host bridge does not support MSI.\n"));
		return ENODEV;
	}

	msi_pic = msipic_construct_msi_pic(pa);
	if (msi_pic == NULL) {
		DPRINTF(("cannot allocate MSI pic.\n"));
		return EINVAL;
	}

	vectors = NULL;
	while (*count > 0) {
		vectors = pci_msi_alloc_vectors(msi_pic, NULL, count);
		if (vectors != NULL)
			break;

		if (exact) {
			DPRINTF(("cannot allocate MSI vectors.\n"));
			msipic_destruct_msi_pic(msi_pic);
			return ENOMEM;
		} else {
			(*count) >>= 1; /* must be power of 2. */
			continue;
		}
	}
	if (vectors == NULL) {
		DPRINTF(("cannot allocate MSI vectors.\n"));
		msipic_destruct_msi_pic(msi_pic);
		return ENOMEM;
	}

	for (i = 0; i < *count; i++) {
		MSI_INT_MAKE_MSI(vectors[i]);
	}

	error = msipic_set_msi_vectors(msi_pic, NULL, *count);
	if (error) {
		pci_msi_free_vectors(msi_pic, vectors, *count);
		msipic_destruct_msi_pic(msi_pic);
		return error;
	}

	*ihps = vectors;
	return 0;
}

static void *
pci_msi_common_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, struct pic *pic,
    const char *xname)
{
	int irq, pin;
	bool mpsafe;

	KASSERT(INT_VIA_MSI(ih));

	irq = -1;
	pin = MSI_INT_VEC(ih);
	mpsafe = ((ih & MPSAFE_MASK) != 0);

	return intr_establish_xname(irq, pic, pin, IST_EDGE, level, func, arg,
	    mpsafe, xname);
}

static void
pci_msi_common_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	intr_disestablish(cookie);
}

static int
pci_msix_alloc_common(pci_intr_handle_t **ihps, u_int *table_indexes,
    int *count, const struct pci_attach_args *pa, bool exact)
{
	struct pic *msix_pic;
	pci_intr_handle_t *vectors;
	int error, i;

	if ((pa->pa_flags & PCI_FLAGS_MSIX_OKAY) == 0) {
		DPRINTF(("PCI host bridge does not support MSI-X.\n"));
		return ENODEV;
	}

	msix_pic = msipic_construct_msix_pic(pa);
	if (msix_pic == NULL)
		return EINVAL;

	vectors = NULL;
	while (*count > 0) {
		vectors = pci_msi_alloc_vectors(msix_pic, table_indexes, count);
		if (vectors != NULL)
			break;

		if (exact) {
			DPRINTF(("cannot allocate MSI-X vectors.\n"));
			msipic_destruct_msix_pic(msix_pic);
			return ENOMEM;
		} else {
			(*count)--;
			continue;
		}
	}
	if (vectors == NULL) {
		DPRINTF(("cannot allocate MSI-X vectors.\n"));
		msipic_destruct_msix_pic(msix_pic);
		return ENOMEM;
	}

	for (i = 0; i < *count; i++) {
		MSI_INT_MAKE_MSIX(vectors[i]);
	}

	error = msipic_set_msi_vectors(msix_pic, vectors, *count);
	if (error) {
		pci_msi_free_vectors(msix_pic, vectors, *count);
		msipic_destruct_msix_pic(msix_pic);
		return error;
	}

	*ihps = vectors;
	return 0;
}

static int
x86_pci_msi_alloc(pci_intr_handle_t **ihps, int *count,
    const struct pci_attach_args *pa)
{

	return pci_msi_alloc_common(ihps, count, pa, false);
}

static int
x86_pci_msi_alloc_exact(pci_intr_handle_t **ihps, int count,
    const struct pci_attach_args *pa)
{

	return pci_msi_alloc_common(ihps, &count, pa, true);
}

static void
x86_pci_msi_release_internal(pci_intr_handle_t *pihs, int count)
{
	struct pic *pic;

	pic = msipic_find_msi_pic(MSI_INT_DEV(pihs[0]));
	if (pic == NULL)
		return;

	pci_msi_free_vectors(pic, pihs, count);
	msipic_destruct_msi_pic(pic);
}

static int
x86_pci_msix_alloc(pci_intr_handle_t **ihps, int *count,
    const struct pci_attach_args *pa)
{

	return pci_msix_alloc_common(ihps, NULL, count, pa, false);
}

static int
x86_pci_msix_alloc_exact(pci_intr_handle_t **ihps, int count,
    const struct pci_attach_args *pa)
{

	return pci_msix_alloc_common(ihps, NULL, &count, pa, true);
}

static int
x86_pci_msix_alloc_map(pci_intr_handle_t **ihps, u_int *table_indexes,
    int count, const struct pci_attach_args *pa)
{

	return pci_msix_alloc_common(ihps, table_indexes, &count, pa, true);
}

static void
x86_pci_msix_release_internal(pci_intr_handle_t *pihs, int count)
{
	struct pic *pic;

	pic = msipic_find_msi_pic(MSI_INT_DEV(pihs[0]));
	if (pic == NULL)
		return;

	pci_msi_free_vectors(pic, pihs, count);
	msipic_destruct_msix_pic(pic);
}

/*****************************************************************************/
/*
 * extern for MD code.
 */

/*
 * Return intrid for a MSI/MSI-X device.
 * "buf" must be allocated by caller.
 */
const char *
x86_pci_msi_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	int dev, vec;

	KASSERT(INT_VIA_MSI(ih));

	dev = MSI_INT_DEV(ih);
	vec = MSI_INT_VEC(ih);
	if (MSI_INT_IS_MSIX(ih))
		snprintf(buf, len, "msix%d vec %d", dev, vec);
	else
		snprintf(buf, len, "msi%d vec %d", dev, vec);

	return buf;
}

/*
 * Release MSI handles.
 */
void
x86_pci_msi_release(pci_chipset_tag_t pc, pci_intr_handle_t *pihs, int count)
{

	if (count < 1)
		return;

	return x86_pci_msi_release_internal(pihs, count);
}

/*
 * Establish a MSI handle.
 * If multiple MSI handle is requied to establish, device driver must call
 * this function for each handle.
 */
void *
x86_pci_msi_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, const char *xname)
{
	struct pic *pic;

	pic = msipic_find_msi_pic(MSI_INT_DEV(ih));
	if (pic == NULL) {
		DPRINTF(("pci_intr_handler has no msi_pic\n"));
		return NULL;
	}

	return pci_msi_common_establish(pc, ih, level, func, arg, pic, xname);
}

/*
 * Disestablish a MSI handle.
 * If multiple MSI handle is requied to disestablish, device driver must call
 * this function for each handle.
 */
void
x86_pci_msi_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	pci_msi_common_disestablish(pc, cookie);
}

/*
 * Release MSI-X handles.
 */
void
x86_pci_msix_release(pci_chipset_tag_t pc, pci_intr_handle_t *pihs, int count)
{

	if (count < 1)
		return;

	return x86_pci_msix_release_internal(pihs, count);
}

/*
 * Establish a MSI-X handle.
 * If multiple MSI-X handle is requied to establish, device driver must call
 * this function for each handle.
 */
void *
x86_pci_msix_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, const char *xname)
{
	struct pic *pic;

	pic = msipic_find_msi_pic(MSI_INT_DEV(ih));
	if (pic == NULL) {
		DPRINTF(("pci_intr_handler has no msi_pic\n"));
		return NULL;
	}

	return pci_msi_common_establish(pc, ih, level, func, arg, pic, xname);
}

/*
 * Disestablish a MSI-X handle.
 * If multiple MSI-X handle is requied to disestablish, device driver must call
 * this function for each handle.
 */
void
x86_pci_msix_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	pci_msi_common_disestablish(pc, cookie);
}

/*****************************************************************************/
/*
 * extern for MI code.
 */

/*
 * This function is used by device drivers like pci_intr_map().
 *
 * "ihps" is the array  of vector numbers which MSI used instead of IRQ number.
 * "count" must be power of 2.
 * "count" can decrease if struct intrsource cannot be allocated.
 * if count == 0, return non-zero value.
 */
int
pci_msi_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{
	int hw_max;

	/* MSI vector count must be power of 2. */
	KASSERT(*count > 0);
	KASSERT(((*count - 1) & *count) == 0);

	hw_max = pci_msi_count(pa->pa_pc, pa->pa_tag);
	if (hw_max == 0)
		return ENODEV;

	if (*count > hw_max) {
		DPRINTF(("cut off MSI count to %d\n", hw_max));
		*count = hw_max; /* cut off hw_max */
	}

	return x86_pci_msi_alloc(ihps, count, pa);
}

/*
 * This function is used by device drivers like pci_intr_map().
 *
 * "ihps" is the array  of vector numbers which MSI used instead of IRQ number.
 * "count" must be power of 2.
 * "count" can not decrease.
 * If "count" struct intrsources cannot be allocated, return non-zero value.
 */
int
pci_msi_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{
	int hw_max;

	/* MSI vector count must be power of 2. */
	KASSERT(count > 0);
	KASSERT(((count - 1) & count) == 0);

	hw_max = pci_msi_count(pa->pa_pc, pa->pa_tag);
	if (hw_max == 0)
		return ENODEV;

	if (count > hw_max) {
		DPRINTF(("over hardware max MSI count %d\n", hw_max));
		return EINVAL;
	}

	return x86_pci_msi_alloc_exact(ihps, count, pa);
}

/*
 * This function is used by device drivers like pci_intr_map().
 *
 * "ihps" is the array  of vector numbers which MSI-X used instead of IRQ number.
 * "count" can decrease if enough struct intrsources cannot be allocated.
 * if count == 0, return non-zero value.
 */
int
pci_msix_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{
	int hw_max;

	KASSERT(*count > 0);

	hw_max = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (hw_max == 0)
		return ENODEV;

	if (*count > hw_max) {
		DPRINTF(("cut off MSI-X count to %d\n", hw_max));
		*count = hw_max; /* cut off hw_max */
	}

	return x86_pci_msix_alloc(ihps, count, pa);
}

/*
 * This function is used by device drivers like pci_intr_map().
 *
 * "ihps" is the array of vector numbers which MSI-X used instead of IRQ number.
 * "count" can not decrease.
 * If "count" struct intrsource cannot be allocated, return non-zero value.
 */
int
pci_msix_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{
	int hw_max;

	KASSERT(count > 0);

	hw_max = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (hw_max == 0)
		return ENODEV;

	if (count > hw_max) {
		DPRINTF(("over hardware max MSI-X count %d\n", hw_max));
		return EINVAL;
	}

	return x86_pci_msix_alloc_exact(ihps, count, pa);
}

/*
 * This function is used by device drivers like pci_intr_map().
 * Futhermore, this function can map each handle to a MSI-X table index.
 *
 * "ihps" is the array of vector numbers which MSI-X used instead of IRQ number.
 * "count" can not decrease.
 * "map" size must be equal to "count".
 * If "count" struct intrsource cannot be allocated, return non-zero value.
 * e.g.
 *     If "map" = { 1, 4, 0 },
 *     1st handle is bound to MSI-X index 1
 *     2nd handle is bound to MSI-X index 4
 *     3rd handle is bound to MSI-X index 0
 */
int
pci_msix_alloc_map(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    u_int *table_indexes, int count)
{
	int hw_max, i, j;

	KASSERT(count > 0);

	hw_max = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (hw_max == 0)
		return ENODEV;

	if (count > hw_max) {
		DPRINTF(("over hardware max MSI-X count %d\n", hw_max));
		return EINVAL;
	}

	/* check not to duplicate table_index */
	for (i = 0; i < count; i++) {
		u_int basing = table_indexes[i];

		KASSERT(table_indexes[i] < PCI_MSIX_MAX_VECTORS);
		if (basing >= hw_max) {
			DPRINTF(("table index is over hardware max MSI-X index %d\n",
				hw_max - 1));
			return EINVAL;
		}

		for (j = i + 1; j < count; j++) {
			if (basing == table_indexes[j]) {
				DPRINTF(("MSI-X table index duplicated\n"));
				return EINVAL;
			}
		}
	}

	return x86_pci_msix_alloc_map(ihps, table_indexes, count, pa);
}
