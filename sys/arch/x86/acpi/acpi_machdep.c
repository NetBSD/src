/* $NetBSD: acpi_machdep.c,v 1.10 2015/10/06 15:06:05 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.10 2015/10/06 15:06:05 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/bootinfo.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_mcfg.h>

#include <machine/acpi_machdep.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/pic.h>

#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "ioapic.h"

#include "acpica.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

ACPI_STATUS
acpi_md_OsInitialize(void)
{
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
acpi_md_OsGetRootPointer(void)
{
	ACPI_PHYSICAL_ADDRESS PhysicalAddress;
	ACPI_STATUS Status;

	Status = AcpiFindRootPointer(&PhysicalAddress);

	if (ACPI_FAILURE(Status))
		PhysicalAddress = 0;

	return PhysicalAddress;
}

struct acpi_md_override {
	int irq;
	int pin;
	int flags;
};

#if NIOAPIC > 0
static ACPI_STATUS
acpi_md_findoverride(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_INTERRUPT_OVERRIDE *iop;
	struct acpi_md_override *ovrp;

	if (hdrp->Type != ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
		return AE_OK;
	}

	iop = (void *)hdrp;
	ovrp = aux;
	if (iop->SourceIrq == ovrp->irq) {
		ovrp->pin = iop->GlobalIrq;
		ovrp->flags = iop->IntiFlags;
	}
	return AE_OK;
}
#endif

ACPI_STATUS
acpi_md_OsInstallInterruptHandler(uint32_t InterruptNumber,
    ACPI_OSD_HANDLER ServiceRoutine, void *Context, void **cookiep)
{
	void *ih;
	struct pic *pic;
#if NIOAPIC > 0
	struct ioapic_softc *sc;
	struct acpi_md_override ovr;
	struct mp_intr_map tmpmap, *mip, **mipp = NULL;
#endif
	int irq, pin, type, redir, mpflags;

	/*
	 * ACPI interrupts default to level-triggered active-low.
	 */

	type = IST_LEVEL;
	mpflags = (MPS_INTTR_LEVEL << 2) | MPS_INTPO_ACTLO;
	redir = IOAPIC_REDLO_LEVEL | IOAPIC_REDLO_ACTLO;

#if NIOAPIC > 0

	/*
	 * Apply any MADT override setting.
	 */

	ovr.irq = InterruptNumber;
	ovr.pin = -1;
	if (acpi_madt_map() == AE_OK) {
		acpi_madt_walk(acpi_md_findoverride, &ovr);
		acpi_madt_unmap();
	} else {
		aprint_debug("acpi_madt_map() failed, can't check for MADT override\n");
	}

	if (ovr.pin != -1) {
		bool sci = InterruptNumber == AcpiGbl_FADT.SciInterrupt;
		int polarity = ovr.flags & ACPI_MADT_POLARITY_MASK;
		int trigger = ovr.flags & ACPI_MADT_TRIGGER_MASK;

		InterruptNumber = ovr.pin;
		if (polarity == ACPI_MADT_POLARITY_ACTIVE_HIGH ||
		    (!sci && polarity == ACPI_MADT_POLARITY_CONFORMS)) {
			mpflags &= ~MPS_INTPO_ACTLO;
			mpflags |= MPS_INTPO_ACTHI;
			redir &= ~IOAPIC_REDLO_ACTLO;
		}
		if (trigger == ACPI_MADT_TRIGGER_EDGE ||
		    (!sci && trigger == ACPI_MADT_TRIGGER_CONFORMS)) {
			type = IST_EDGE;
			mpflags &= ~(MPS_INTTR_LEVEL << 2);
			mpflags |= (MPS_INTTR_EDGE << 2);
			redir &= ~IOAPIC_REDLO_LEVEL;
		}
	}

	/*
	 * If the interrupt is handled via IOAPIC, update the map.
	 * If the map isn't set up yet, install a temporary one.
	 */

	sc = ioapic_find_bybase(InterruptNumber);
	if (sc != NULL) {
		pic = &sc->sc_pic;

		if (pic->pic_type == PIC_IOAPIC) {
			pin = (int)InterruptNumber - pic->pic_vecbase;
			irq = -1;
		} else {
			irq = pin = (int)InterruptNumber;
		}

		mip = sc->sc_pins[pin].ip_map;
		if (mip) {
			mip->flags &= ~0xf;
			mip->flags |= mpflags;
			mip->redir &= ~(IOAPIC_REDLO_LEVEL |
					IOAPIC_REDLO_ACTLO);
			mip->redir |= redir;
		} else {
			mipp = &sc->sc_pins[pin].ip_map;
			*mipp = &tmpmap;
			tmpmap.redir = redir;
			tmpmap.flags = mpflags;
		}
	} else
#endif
	{
		pic = &i8259_pic;
		irq = pin = (int)InterruptNumber;
	}

	/*
	 * XXX probably, IPL_BIO is enough.
	 */
	ih = intr_establish(irq, pic, pin, type, IPL_TTY,
	    (int (*)(void *)) ServiceRoutine, Context, false);

#if NIOAPIC > 0
	if (mipp) {
		*mipp = NULL;
	}
#endif

	if (ih == NULL)
		return AE_NO_MEMORY;

	*cookiep = ih;

	return AE_OK;
}

void
acpi_md_OsRemoveInterruptHandler(void *cookie)
{
	intr_disestablish(cookie);
}

ACPI_STATUS
acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress,
    uint32_t Length, void **LogicalAddress)
{
	int rv;

	rv = _x86_memio_map(x86_bus_space_mem, PhysicalAddress,
	    Length, 0, (bus_space_handle_t *)LogicalAddress);

	return (rv != 0) ? AE_NO_MEMORY : AE_OK;
}

void
acpi_md_OsUnmapMemory(void *LogicalAddress, uint32_t Length)
{
	(void) _x86_memio_unmap(x86_bus_space_mem,
	    (bus_space_handle_t)LogicalAddress, Length, NULL);
}

ACPI_STATUS
acpi_md_OsGetPhysicalAddress(void *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t) LogicalAddress, &pa)) {
		*PhysicalAddress = pa;
		return AE_OK;
	}

	return AE_ERROR;
}

BOOLEAN
acpi_md_OsReadable(void *Pointer, uint32_t Length)
{
	BOOLEAN rv = TRUE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return FALSE;

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & PG_V) == 0) {
			rv = FALSE;
			break;
		}
	}

	return rv;
}

BOOLEAN
acpi_md_OsWritable(void *Pointer, uint32_t Length)
{
	BOOLEAN rv = TRUE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return FALSE;

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & (PG_V|PG_W)) != (PG_V|PG_W)) {
			rv = FALSE;
			break;
		}
	}

	return rv;
}

void
acpi_md_OsDisableInterrupt(void)
{
	x86_disable_intr();
}

void
acpi_md_OsEnableInterrupt(void)
{
	x86_enable_intr();
}

uint32_t
acpi_md_ncpus(void)
{
	return kcpuset_countset(kcpuset_attached);
}

static bool
acpi_md_mcfg_validate(uint64_t addr, int bus_start, int *bus_end)
{
	struct btinfo_memmap *bim;
	uint64_t size, mapaddr, mapsize;
	uint32_t type;
	int i, n;

	bim = lookup_bootinfo(BTINFO_MEMMAP);
	if (bim == NULL)
		return false;

	size = *bus_end - bus_start + 1;
	size *= ACPIMCFG_SIZE_PER_BUS;
	for (i = 0; i < bim->num; i++) {
		mapaddr = bim->entry[i].addr;
		mapsize = bim->entry[i].size;
		type = bim->entry[i].type;

		aprint_debug("MCFG: MEMMAP: 0x%016" PRIx64 "-0x%016" PRIx64
		    ", size=0x%016" PRIx64 ", type=%d(%s)\n",
		    mapaddr, mapaddr + mapsize - 1, mapsize, type,
		    (type == BIM_Memory) ?  "Memory" :
		    (type == BIM_Reserved) ?  "Reserved" :
		    (type == BIM_ACPI) ? "ACPI" :
		    (type == BIM_NVS) ? "NVS" :
		    "unknown");

		switch (type) {
		case BIM_ACPI:
		case BIM_Reserved:
			if (addr < mapaddr || addr >= mapaddr + mapsize)
				break;

			/* full map */
			if (addr + size <= mapaddr + mapsize)
				return true;

			/* partial map */
			n = (mapsize - (addr - mapaddr)) /
			    ACPIMCFG_SIZE_PER_BUS;
			/* bus_start == bus_end is not allowed. */
			if (n > 1) {
				*bus_end = bus_start + n - 1;
				return true;
			}
			aprint_debug("MCFG: bus %d-%d, address 0x%016" PRIx64
			    ": invalid size: request 0x%016" PRIx64 ", "
			    "actual 0x%016" PRIx64 "\n",
			    bus_start, *bus_end, addr, size, mapsize);
			break;
		}
	}
	aprint_debug("MCFG: bus %d-%d, address 0x%016" PRIx64 ": "
	    "no valid region\n", bus_start, *bus_end, addr);
	return false;
}

static uint32_t
acpi_md_mcfg_read(bus_space_tag_t bst, bus_space_handle_t bsh, bus_addr_t addr)
{
	vaddr_t va = bsh + addr;
	uint32_t data = (uint32_t) -1;

	KASSERT(bst == x86_bus_space_mem);

	__asm("movl %1, %0" : "=a" (data) : "m" (*(volatile uint32_t *)va));

	return data;
}

static void
acpi_md_mcfg_write(bus_space_tag_t bst, bus_space_handle_t bsh, bus_addr_t addr,
    uint32_t data)
{
	vaddr_t va = bsh + addr;

	KASSERT(bst == x86_bus_space_mem);

	__asm("movl %1, %0" : "=m" (*(volatile uint32_t *)va) : "a" (data));
}

static const struct acpimcfg_ops acpi_md_mcfg_ops = {
	.ao_validate = acpi_md_mcfg_validate,

	.ao_read = acpi_md_mcfg_read,
	.ao_write = acpi_md_mcfg_write,
};

void
acpi_md_callback(struct acpi_softc *sc)
{
#ifdef MPBIOS
	if (!mpbios_scanned)
#endif
	mpacpi_find_interrupts(sc);

#ifndef XEN
	acpi_md_sleep_init();
#endif

	acpimcfg_init(x86_bus_space_mem, &acpi_md_mcfg_ops);
}
