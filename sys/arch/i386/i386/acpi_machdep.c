/*	$NetBSD: acpi_machdep.c,v 1.11 2003/03/04 13:44:08 yamt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent routines for ACPICA.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.11 2003/03/04 13:44:08 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

#include <machine/acpi_machdep.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#include <machine/i82093var.h>
#include <machine/pic.h>

#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "ioapic.h"

#include "opt_mpacpi.h"
#include "opt_mpbios.h"

ACPI_STATUS
acpi_md_OsInitialize(void)
{

	/* Nothing to do, yet. */
	return (AE_OK);
}

ACPI_STATUS
acpi_md_OsTerminate(void)
{

	/* Nothing to do, yet. */
	return (AE_OK);
}

ACPI_STATUS
acpi_md_OsGetRootPointer(UINT32 Flags, ACPI_POINTER *PhysicalAddress)
{

	return (AcpiFindRootPointer(Flags, PhysicalAddress));
}

ACPI_STATUS
acpi_md_OsInstallInterruptHandler(UINT32 InterruptNumber,
    OSD_HANDLER ServiceRoutine, void *Context, void **cookiep)
{
	void *ih;
	struct pic *pic;
	int irq;
	int pin;
#if NIOAPIC > 0
	struct ioapic_softc *sc;

	if (ioapics != NULL) {
		sc = ioapic_find_bybase(InterruptNumber);
		if (sc == NULL) {
			pin = (int)InterruptNumber;
			for (sc = ioapics ; sc != NULL && pin > sc->sc_apic_sz;
			     sc = sc->sc_next)
				pin -= sc->sc_apic_sz;
			if (sc == NULL)
				return AE_NOT_FOUND;
			if (nioapics > 1)
				printf("acpi: WARNING: no matching "
				       "I/O apic for SCI, assuming %s\n",
				    sc->sc_pic.pic_dev.dv_xname);
		} else
			pin = InterruptNumber - sc->sc_apic_vecbase;
		pic = (struct pic *)sc;
		irq = -1;
	} else
#endif
	{
		pic = &i8259_pic;
		irq = pin = (int)InterruptNumber;
	}

	/*
	 * XXX probably, IPL_BIO is enough.
	 */
	ih = intr_establish(irq, pic, pin, IST_LEVEL, IPL_VM,
	    (int (*)(void *)) ServiceRoutine, Context);
	if (ih == NULL)
		return (AE_NO_MEMORY);
	*cookiep = ih;
	return (AE_OK);
}

void
acpi_md_OsRemoveInterruptHandler(void *cookie)
{

	intr_disestablish(cookie);
}

ACPI_STATUS
acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress,
    UINT32 Length, void **LogicalAddress)
{

	if (_x86_memio_map(X86_BUS_SPACE_MEM, PhysicalAddress, Length,
	    0, (bus_space_handle_t *) LogicalAddress) == 0)
		return (AE_OK);

	return (AE_NO_MEMORY);
}

void
acpi_md_OsUnmapMemory(void *LogicalAddress, UINT32 Length)
{

	(void) _x86_memio_unmap(X86_BUS_SPACE_MEM,
	    (bus_space_handle_t) LogicalAddress, Length, NULL);
}

ACPI_STATUS
acpi_md_OsGetPhysicalAddress(void *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t) LogicalAddress, &pa)) {
		*PhysicalAddress = pa;
		return (AE_OK);
	}

	return (AE_ERROR);
}

BOOLEAN
acpi_md_OsReadable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = TRUE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return (FALSE);

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & PG_V) == 0) {
			rv = FALSE;
			break;
		}
	}

	return (rv);
}

BOOLEAN
acpi_md_OsWritable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = FALSE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return (FALSE);

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & (PG_V|PG_W)) != (PG_V|PG_W)) {
			rv = FALSE;
			break;
		}
	}

	return (rv);
}

void 
acpi_md_OsDisableInterrupt(void)
{
	disable_intr();
}

void
acpi_md_callback(struct device *acpi)
{
#ifdef MPACPI
#ifdef MPBIOS
	if (mpbios_scanned)
		return;
#endif
	mpacpi_find_interrupts(acpi);
#endif
}
