/* $NetBSD: pci_machdep_common.c,v 1.15.6.3 2017/12/03 11:36:37 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * Generic PowerPC functions for dealing with a PCI bridge.  For most cases,
 * these functions will work just fine, however, some machines may need
 * specialized code, so those ports are free to write thier own functions
 * and call those instead where appropriate.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep_common.c,v 1.15.6.3 2017/12/03 11:36:37 jdolecek Exp $");

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pciidereg.h>

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	._dmamap_create = _bus_dmamap_create,
	._dmamap_destroy = _bus_dmamap_destroy,
	._dmamap_load = _bus_dmamap_load,
	._dmamap_load_mbuf = _bus_dmamap_load_mbuf,
	._dmamap_load_uio = _bus_dmamap_load_uio,
	._dmamap_load_raw = _bus_dmamap_load_raw,
	._dmamap_unload = _bus_dmamap_unload,

	._dmamem_alloc = _bus_dmamem_alloc,
	._dmamem_free = _bus_dmamem_free,
	._dmamem_map = _bus_dmamem_map,
	._dmamem_unmap = _bus_dmamem_unmap,
	._dmamem_mmap = _bus_dmamem_mmap,
};

int
genppc_pci_bus_maxdevs(void *v, int busno)
{
	return 32;
}

const char *
genppc_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
#ifdef ICU_LEN
	if (ih == 0 || ih >= ICU_LEN
/* XXX on macppc it's completely legal to have PCI interrupts on a slave PIC */
#ifdef IRQ_SLAVE
	    || ih == IRQ_SLAVE
#endif
	    )
		panic("pci_intr_string: bogus handle 0x%x", ih);
#endif

	snprintf(buf, len, "irq %d", ih);
	return buf;
	
}

const struct evcnt *
genppc_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
genppc_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg, const char *xname)
{

#ifdef ICU_LEN
	if (ih == 0 || ih >= ICU_LEN
#ifdef IRQ_SLAVE
	    || ih == IRQ_SLAVE
#endif
	    )
		panic("pci_intr_establish: bogus handle 0x%x", ih);
#endif

	return intr_establish_xname(ih, IST_LEVEL, level, func, arg, xname);
}

void
genppc_pci_intr_disestablish(void *v, void *cookie)
{

	intr_disestablish(cookie);
}

int
genppc_pci_intr_setattr(void *v, pci_intr_handle_t *ihp, int attr,
    uint64_t data)
{

	return ENODEV;
}

pci_intr_type_t
genppc_pci_intr_type(void *v, pci_intr_handle_t ih)
{

	return PCI_INTR_TYPE_INTX;
}

int
genppc_pci_intr_alloc(const struct pci_attach_args *pa,
    pci_intr_handle_t **ihps, int *counts, pci_intr_type_t max_type)
{
	pci_intr_handle_t *ihp;

	if (counts != NULL && counts[PCI_INTR_TYPE_INTX] == 0)
		return EINVAL;

	ihp = kmem_alloc(sizeof(*ihp), KM_SLEEP);
	if (pci_intr_map(pa, ihp)) {
		kmem_free(ihp, sizeof(*ihp));
		return EINVAL;
	}

	*ihps = ihp;
	return 0;
}

void
genppc_pci_intr_release(void *v, pci_intr_handle_t *pih, int count)
{

	if (pih == NULL)
		return;

	KASSERT(count == 1);
	kmem_free(pih, sizeof(*pih));
}

int
genppc_pci_intx_alloc(const struct pci_attach_args *pa,
    pci_intr_handle_t **ihps)
{
	pci_intr_handle_t *handle;
	int error;

	handle = kmem_zalloc(sizeof(*handle), KM_SLEEP);
	error = pci_intr_map(pa, handle);
	if (error != 0) {
		kmem_free(handle, sizeof(*handle));
		return error;
	}

	*ihps = handle;
	return 0;
}

void
genppc_pci_conf_interrupt(void *v, int bus, int dev, int pin,
    int swiz, int *iline)
{
	/* do nothing */
}

int
genppc_pci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	return (PCI_CONF_DEFAULT);
}

int
genppc_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;
	int line = pa->pa_intrline;
	
#ifdef DEBUG
	printf("%s: pin: %d, line: %d\n", __func__, pin, line);
#endif

	if (pin == 0) {
		/* No IRQ used. */
		aprint_error("pci_intr_map: interrupt pin %d\n", pin);
		goto bad;
	}

	if (pin > 4) {
		aprint_error("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	/*
	 * Section 6.2.4, `Miscellaneous Functions', says that 255 means
	 * `unknown' or `no connection' on a PC.  We assume that a device with
	 * `no connection' either doesn't have an interrupt (in which case the
	 * pin number should be 0, and would have been noticed above), or
	 * wasn't configured by the BIOS (in which case we punt, since there's
	 * no real way we can know how the interrupt lines are mapped in the
	 * hardware).
	 *
	 * XXX
	 * Since IRQ 0 is only used by the clock, and we can't actually be sure
	 * that the BIOS did its job, we also recognize that as meaning that
	 * the BIOS has not configured the device.
	 */
	if (line == 0 || line == 255) {
		aprint_error("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
#ifdef ICU_LEN
	} else {
		if (line >= ICU_LEN) {
			aprint_error("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
#endif
	}

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

/* experimental MSI support */
int
genppc_pci_msi_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count, bool exact)
{

	return EOPNOTSUPP;
}

/* experimental MSI-X support */
int
genppc_pci_msix_alloc(const struct pci_attach_args *pa,
    pci_intr_handle_t **ihps, u_int *table_indexes, int *count, bool exact)
{

	return EOPNOTSUPP;
}

void
genppc_pci_chipset_msi_init(pci_chipset_tag_t pc)
{
	pc->pc_msi_alloc = genppc_pci_msi_alloc;
}

void
genppc_pci_chipset_msix_init(pci_chipset_tag_t pc)
{
	pc->pc_msix_alloc = genppc_pci_msix_alloc;
}

#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#include <machine/isa_machdep.h>
#include "isa.h"

void *genppc_pciide_machdep_compat_intr_establish(device_t,
    struct pci_attach_args *, int, int (*)(void *), void *);

void *
genppc_pciide_machdep_compat_intr_establish(device_t dev,
    struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
#if NISA > 0
	int irq;
	void *cookie;

	irq = PCIIDE_COMPAT_IRQ(chan);
	cookie = isa_intr_establish(NULL, irq, IST_LEVEL, IPL_BIO, func, arg);
	if (cookie == NULL)
		return (NULL);
	aprint_normal_dev(dev, "%s channel interrupting at irq %d\n",
	    PCIIDE_CHANNEL_NAME(chan), irq);
	return (cookie);
#else
	panic("pciide_machdep_compat_intr_establish() called");
#endif
}
#endif /* __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH */
