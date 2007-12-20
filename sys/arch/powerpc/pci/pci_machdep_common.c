/* $NetBSD: pci_machdep_common.c,v 1.4 2007/12/20 22:24:40 phx Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep_common.c,v 1.4 2007/12/20 22:24:40 phx Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/isa_machdep.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pciidereg.h>

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
/* 
 * XXX for now macppc needs its own pci_bus_dma_tag
 * this will go away once we use the common bus_space stuff
 */
#ifndef macppc 
struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};
#endif
int
genppc_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return 32;
}

const char *
genppc_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0 || ih >= ICU_LEN
/* XXX on macppc it's completely legal to have PCI interrupts on a slave PIC */
#ifdef IRQ_SLAVE
	    || ih == IRQ_SLAVE
#endif
	    )
		panic("pci_intr_string: bogus handle 0x%x", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

const struct evcnt *
genppc_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
genppc_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih == 0 || ih >= ICU_LEN
#ifdef IRQ_SLAVE
	    || ih == IRQ_SLAVE
#endif
	    )
		panic("pci_intr_establish: bogus handle 0x%x", ih);

	return intr_establish(ih, IST_LEVEL, level, func, arg);
}

void
genppc_pci_intr_disestablish(void *v, void *cookie)
{

	intr_disestablish(cookie);
}

void
genppc_pci_conf_interrupt(pci_chipset_tag_t pct, int bus, int dev, int pin,
    int swiz, int *iline)
{
	/* do nothing */
}

int
genppc_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, int func,
	pcireg_t id)
{
	return (PCI_CONF_DEFAULT);
}

int
genppc_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;
	int line = pa->pa_intrline;
	
#if DEBUG
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
	} else {
		if (line >= ICU_LEN) {
			aprint_error("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
	}

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#include "isa.h"

void *genppc_pciide_machdep_compat_intr_establish(struct device *,
    struct pci_attach_args *, int, int (*)(void *), void *);

void *
genppc_pciide_machdep_compat_intr_establish(struct device *dev,
    struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
#if NISA > 0
	int irq;
	void *cookie;

	irq = PCIIDE_COMPAT_IRQ(chan);
	cookie = isa_intr_establish(NULL, irq, IST_LEVEL, IPL_BIO, func, arg);
	if (cookie == NULL)
		return (NULL);
	printf("%s: %s channel interrupting at irq %d\n", dev->dv_xname,
	    PCIIDE_CHANNEL_NAME(chan), irq);
	return (cookie);
#else
	panic("pciide_machdep_compat_intr_establish() called");
#endif
}
#endif /* __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH */
