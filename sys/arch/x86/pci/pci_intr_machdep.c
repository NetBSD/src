/*	$NetBSD: pci_intr_machdep.c,v 1.42 2018/01/04 01:01:59 knakahara Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_intr_machdep.c,v 1.42 2018/01/04 01:01:59 knakahara Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>

#include <dev/pci/pcivar.h>

#include "ioapic.h"
#include "eisa.h"
#include "acpica.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

#include <machine/i82489reg.h>

#if NIOAPIC > 0 || NACPICA > 0
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/mpconfig.h>
#include <machine/mpbiosvar.h>
#include <machine/pic.h>
#include <x86/pci/pci_msi_machdep.h>
#endif

#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NACPICA > 0
#include <machine/mpacpi.h>
#endif

int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_intr_pin_t pin = pa->pa_intrpin;
	pci_intr_line_t line = pa->pa_intrline;
	pci_chipset_tag_t ipc, pc = pa->pa_pc;
#if NIOAPIC > 0 || NACPICA > 0
	pci_intr_pin_t rawpin = pa->pa_rawintrpin;
	int bus, dev, func;
#endif

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_INTR_MAP) == 0)
			continue;
		return (*ipc->pc_ov->ov_intr_map)(ipc->pc_ctx, pa, ihp);
	}

	if (pin == 0) {
		/* No IRQ used. */
		goto bad;
	}

	*ihp = 0;

	if (pin > PCI_INTERRUPT_PIN_MAX) {
		aprint_normal("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

#if NIOAPIC > 0 || NACPICA > 0
	KASSERT(rawpin >= PCI_INTERRUPT_PIN_A);
	KASSERT(rawpin <= PCI_INTERRUPT_PIN_D);
	pci_decompose_tag(pc, pa->pa_tag, &bus, &dev, &func);
	if (mp_busses != NULL) {
		/*
		 * Note: PCI_INTERRUPT_PIN_A == 1 where intr_find_mpmapping
		 * wants pci bus_pin encoding which uses INT_A == 0.
		 */
		if (intr_find_mpmapping(bus,
		    (dev << 2) | (rawpin - PCI_INTERRUPT_PIN_A), ihp) == 0) {
			if (APIC_IRQ_LEGACY_IRQ(*ihp) == 0)
				*ihp |= line;
			return 0;
		}
		/*
		 * No explicit PCI mapping found. This is not fatal,
		 * we'll try the ISA (or possibly EISA) mappings next.
		 */
	}
#endif

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
	if (line == 0 || line == X86_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		aprint_normal("pci_intr_map: no mapping for pin %c (line=%02x)\n",
		       '@' + pin, line);
		goto bad;
	} else {
		if (line >= NUM_LEGACY_IRQS) {
			aprint_normal("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
		if (line == 2) {
			aprint_normal("pci_intr_map: changed line 2 to line 9\n");
			line = 9;
		}
	}
#if NIOAPIC > 0 || NACPICA > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, line, ihp) == 0) {
			if ((*ihp & 0xff) == 0)
				*ihp |= line;
			return 0;
		}
#if NEISA > 0
		if (intr_find_mpmapping(mp_eisa_bus, line, ihp) == 0) {
			if ((*ihp & 0xff) == 0)
				*ihp |= line;
			return 0;
		}
#endif
		aprint_normal("pci_intr_map: bus %d dev %d func %d pin %d; line %d\n",
		    bus, dev, func, pin, line);
		aprint_normal("pci_intr_map: no MP mapping found\n");
	}
#endif

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	pci_chipset_tag_t ipc;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_INTR_STRING) == 0)
			continue;
		return (*ipc->pc_ov->ov_intr_string)(ipc->pc_ctx, pc, ih,
		    buf, len);
	}

	if (INT_VIA_MSI(ih))
		return x86_pci_msi_string(pc, ih, buf, len);

	return intr_string(ih & ~MPSAFE_MASK, buf, len);
}


const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	pci_chipset_tag_t ipc;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_INTR_EVCNT) == 0)
			continue;
		return (*ipc->pc_ov->ov_intr_evcnt)(ipc->pc_ctx, pc, ih);
	}

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		if (data) {
			 *ih |= MPSAFE_MASK;
		} else {
			 *ih &= ~MPSAFE_MASK;
		}
		/* XXX Set live if already mapped. */
		return 0;
	default:
		return ENODEV;
	}
}

static int
pci_intr_find_intx_irq(pci_intr_handle_t ih, int *irq, struct pic **pic,
    int *pin)
{

	KASSERT(irq != NULL);
	KASSERT(pic != NULL);
	KASSERT(pin != NULL);

	*pic = &i8259_pic;
	*pin = *irq = APIC_IRQ_LEGACY_IRQ(ih);

#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC) {
		struct ioapic_softc *ioapic;

		ioapic = ioapic_find(APIC_IRQ_APIC(ih));
		if (ioapic == NULL)
			return ENOENT;
		*pic = &ioapic->sc_pic;
		*pin = APIC_IRQ_PIN(ih);
		*irq = APIC_IRQ_LEGACY_IRQ(ih);
		if (*irq < 0 || *irq >= NUM_LEGACY_IRQS)
			*irq = -1;
	}
#endif

	return 0;
}

static void *
pci_intr_establish_xname_internal(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, const char *xname)
{
	int pin, irq;
	struct pic *pic;
	bool mpsafe;
	pci_chipset_tag_t ipc;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_INTR_ESTABLISH) == 0)
			continue;
		return (*ipc->pc_ov->ov_intr_establish)(ipc->pc_ctx,
		    pc, ih, level, func, arg);
	}

	if (INT_VIA_MSI(ih)) {
		if (MSI_INT_IS_MSIX(ih))
			return x86_pci_msix_establish(pc, ih, level, func, arg,
			    xname);
		else
			return x86_pci_msi_establish(pc, ih, level, func, arg,
			    xname);
	}

	if (pci_intr_find_intx_irq(ih, &irq, &pic, &pin)) {
		aprint_normal("%s: bad pic %d\n", __func__,
		    APIC_IRQ_APIC(ih));
		return NULL;
	}

	mpsafe = ((ih & MPSAFE_MASK) != 0);

	return intr_establish_xname(irq, pic, pin, IST_LEVEL, level, func, arg,
	    mpsafe, xname);
}

void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg)
{

	return pci_intr_establish_xname_internal(pc, ih, level, func, arg, "unknown");
}

#ifdef __HAVE_PCI_MSI_MSIX
void *
pci_intr_establish_xname(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, const char *xname)
{

	return pci_intr_establish_xname_internal(pc, ih, level, func, arg, xname);
}
#endif


void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	pci_chipset_tag_t ipc;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_INTR_DISESTABLISH) == 0)
			continue;
		(*ipc->pc_ov->ov_intr_disestablish)(ipc->pc_ctx, pc, cookie);
		return;
	}

	/* MSI/MSI-X processing is switched in intr_disestablish(). */
	intr_disestablish(cookie);
}

#if NIOAPIC > 0
#ifdef __HAVE_PCI_MSI_MSIX
pci_intr_type_t
pci_intr_type(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	if (INT_VIA_MSI(ih)) {
		if (MSI_INT_IS_MSIX(ih))
			return PCI_INTR_TYPE_MSIX;
		else
			return PCI_INTR_TYPE_MSI;
	} else {
		return PCI_INTR_TYPE_INTX;
	}
}

static const char *
x86_pci_intx_create_intrid(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
#if !defined(XEN)
	int pin, irq;
	struct pic *pic;

	KASSERT(!INT_VIA_MSI(ih));

	pic = &i8259_pic;
	pin = irq = APIC_IRQ_LEGACY_IRQ(ih);

	if (pci_intr_find_intx_irq(ih, &irq, &pic, &pin)) {
		aprint_normal("%s: bad pic %d\n", __func__,
		    APIC_IRQ_APIC(ih));
		return NULL;
	}

	return intr_create_intrid(irq, pic, pin, buf, len);
#else
	return pci_intr_string(pc, ih, buf, len);
#endif /* !XEN */
}

static void
x86_pci_intx_release(pci_chipset_tag_t pc, pci_intr_handle_t *pih)
{
	char intrstr_buf[INTRIDBUF];
	const char *intrstr;

	intrstr = pci_intr_string(NULL, *pih, intrstr_buf, sizeof(intrstr_buf));
	mutex_enter(&cpu_lock);
	intr_free_io_intrsource(intrstr);
	mutex_exit(&cpu_lock);

	kmem_free(pih, sizeof(*pih));
}

int
pci_intx_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **pih)
{
	struct intrsource *isp;
	pci_intr_handle_t *handle;
	int error;
	char intrstr_buf[INTRIDBUF];
	const char *intrstr;

	handle = kmem_zalloc(sizeof(*handle), KM_SLEEP);
	if (pci_intr_map(pa, handle) != 0) {
		aprint_normal("cannot set up pci_intr_handle_t\n");
		error = EINVAL;
		goto error;
	}

	/*
	 * must be the same intrstr as intr_establish_xname()
	 */
	intrstr = x86_pci_intx_create_intrid(pa->pa_pc, *handle, intrstr_buf,
	    sizeof(intrstr_buf));
	mutex_enter(&cpu_lock);
	isp = intr_allocate_io_intrsource(intrstr);
	mutex_exit(&cpu_lock);
	if (isp == NULL) {
		aprint_normal("can't allocate io_intersource\n");
		error = ENOMEM;
		goto error;
	}

	*pih = handle;
	return 0;

error:
	kmem_free(handle, sizeof(*handle));
	return error;
}

/*
 * Interrupt handler allocation utility. This function calls each allocation
 * function as specified by arguments.
 * Currently callee functions are pci_intx_alloc(), pci_msi_alloc_exact(),
 * and pci_msix_alloc_exact().
 * pa       : pci_attach_args
 * ihps     : interrupt handlers
 * counts   : The array of number of required interrupt handlers.
 *            It is overwritten by allocated the number of handlers.
 *            CAUTION: The size of counts[] must be PCI_INTR_TYPE_SIZE.
 * max_type : "max" type of using interrupts. See below.
 *     e.g.
 *         If you want to use 5 MSI-X, 1 MSI, or INTx, you use "counts" as
 *             int counts[PCI_INTR_TYPE_SIZE];
 *             counts[PCI_INTR_TYPE_MSIX] = 5;
 *             counts[PCI_INTR_TYPE_MSI] = 1;
 *             counts[PCI_INTR_TYPE_INTX] = 1;
 *             error = pci_intr_alloc(pa, ihps, counts, PCI_INTR_TYPE_MSIX);
 *
 *         If you want to use hardware max number MSI-X or 1 MSI,
 *         and not to use INTx, you use "counts" as
 *             int counts[PCI_INTR_TYPE_SIZE];
 *             counts[PCI_INTR_TYPE_MSIX] = -1;
 *             counts[PCI_INTR_TYPE_MSI] = 1;
 *             counts[PCI_INTR_TYPE_INTX] = 0;
 *             error = pci_intr_alloc(pa, ihps, counts, PCI_INTR_TYPE_MSIX);
 *
 *         If you want to use 3 MSI or INTx, you can use "counts" as
 *             int counts[PCI_INTR_TYPE_SIZE];
 *             counts[PCI_INTR_TYPE_MSI] = 3;
 *             counts[PCI_INTR_TYPE_INTX] = 1;
 *             error = pci_intr_alloc(pa, ihps, counts, PCI_INTR_TYPE_MSI);
 *
 *         If you want to use 1 MSI or INTx (probably most general usage),
 *         you can simply use this API like
 *         below
 *             error = pci_intr_alloc(pa, ihps, NULL, 0);
 *                                                    ^ ignored
 */
int
pci_intr_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *counts, pci_intr_type_t max_type)
{
	int error;
	int intx_count, msi_count, msix_count;

	intx_count = msi_count = msix_count = 0;
	if (counts == NULL) { /* simple pattern */
		msi_count = 1;
		intx_count = 1;
	} else {
		switch(max_type) {
		case PCI_INTR_TYPE_MSIX:
			msix_count = counts[PCI_INTR_TYPE_MSIX];
			/* FALLTHROUGH */
		case PCI_INTR_TYPE_MSI:
			msi_count = counts[PCI_INTR_TYPE_MSI];
			/* FALLTHROUGH */
		case PCI_INTR_TYPE_INTX:
			intx_count = counts[PCI_INTR_TYPE_INTX];
			break;
		default:
			return EINVAL;
		}
	}

	if (counts != NULL)
		memset(counts, 0, sizeof(counts[0]) * PCI_INTR_TYPE_SIZE);
	error = EINVAL;

	/* try MSI-X */
	if (msix_count == -1) /* use hardware max */
		msix_count = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (msix_count > 0) {
		error = pci_msix_alloc_exact(pa, ihps, msix_count);
		if (error == 0) {
			KASSERTMSG(counts != NULL,
			    "If MSI-X is used, counts must not be NULL.");
			counts[PCI_INTR_TYPE_MSIX] = msix_count;
			goto out;
		}
	}

	/* try MSI */
	if (msi_count == -1) /* use hardware max */
		msi_count = pci_msi_count(pa->pa_pc, pa->pa_tag);
	if (msi_count > 0) {
		error = pci_msi_alloc_exact(pa, ihps, msi_count);
		if (error == 0) {
			if (counts != NULL)
				counts[PCI_INTR_TYPE_MSI] = msi_count;
			goto out;
		}
	}

	/* try INTx */
	if (intx_count != 0) { /* The number of INTx is always 1. */
		error = pci_intx_alloc(pa, ihps);
		if (error == 0) {
			if (counts != NULL)
				counts[PCI_INTR_TYPE_INTX] = 1;
		}
	}

 out:
	return error;
}

void
pci_intr_release(pci_chipset_tag_t pc, pci_intr_handle_t *pih, int count)
{
	if (pih == NULL)
		return;

	if (INT_VIA_MSI(*pih)) {
		if (MSI_INT_IS_MSIX(*pih))
			return x86_pci_msix_release(pc, pih, count);
		else
			return x86_pci_msi_release(pc, pih, count);
	} else {
		KASSERT(count == 1);
		return x86_pci_intx_release(pc, pih);
	}

}
#endif /* __HAVE_PCI_MSI_MSIX */
#endif /*  NIOAPIC > 0 */
