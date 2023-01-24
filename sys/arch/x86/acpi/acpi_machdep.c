/* $NetBSD: acpi_machdep.c,v 1.35 2023/01/24 09:35:20 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.35 2023/01/24 09:35:20 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/bootinfo.h>
#include <machine/autoconf.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_mcfg.h>

#include <machine/acpi_machdep.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/pic.h>
#include <machine/pmap_private.h>

#include <x86/efi.h>

#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "ioapic.h"

#include "acpica.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"
#include "opt_vga.h"

#ifdef XEN
#include <xen/hypervisor.h>
#endif

/*
 * Default VBIOS reset method for non-HW accelerated VGA drivers.
 */
#ifdef VGA_POST
# define VBIOS_RESET_DEFAULT	2
#else
# define VBIOS_RESET_DEFAULT	1
#endif

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

#ifdef XENPV
	/*
	 * Obtain the ACPI RSDP from the hypervisor.
	 * This is the only way to go if Xen booted from EFI: the
	 * Extended BIOS Data Area (EBDA) is not mapped, and Xen
	 * does not pass an EFI SystemTable to the kernel.
	 */
        struct xen_platform_op op = {
                .cmd = XENPF_firmware_info,
                .u.firmware_info = {
                        .type = XEN_FW_EFI_INFO,
                        .index = XEN_FW_EFI_CONFIG_TABLE
                }
        };
        union xenpf_efi_info *info = &op.u.firmware_info.u.efi_info;

        if (HYPERVISOR_platform_op(&op) == 0) {
		struct efi_cfgtbl *ct;
		int i;

		ct = AcpiOsMapMemory(info->cfg.addr,
		    sizeof(*ct) * info->cfg.nent);

		for (i = 0; i < info->cfg.nent; i++) {
                	if (memcmp(&ct[i].ct_uuid,
			    &EFI_UUID_ACPI20, sizeof(EFI_UUID_ACPI20)) == 0) {
				PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)
				    (uintptr_t)ct[i].ct_data;
				if (PhysicalAddress)
					goto out;

			}
		}

		for (i = 0; i < info->cfg.nent; i++) {
                	if (memcmp(&ct[i].ct_uuid,
			    &EFI_UUID_ACPI10, sizeof(EFI_UUID_ACPI10)) == 0) {
				PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)
				    (uintptr_t)ct[i].ct_data;
				if (PhysicalAddress)
					goto out;

			}
		}
out:
		AcpiOsUnmapMemory(ct, sizeof(*ct) * info->cfg.nent);

		if (PhysicalAddress)
			return PhysicalAddress;
	}
#else
#ifdef XEN
	if (vm_guest == VM_GUEST_XENPVH) {
		PhysicalAddress = hvm_start_info->rsdp_paddr;
		if (PhysicalAddress)
			return PhysicalAddress;
	}
#endif
	/*
	 * Get the ACPI RSDP from EFI SystemTable. This works when the
	 * kernel was loaded from EFI bootloader.
	 */
	if (efi_probe()) {
		PhysicalAddress = efi_getcfgtblpa(&EFI_UUID_ACPI20);
		if (!PhysicalAddress)
			PhysicalAddress = efi_getcfgtblpa(&EFI_UUID_ACPI10);
		if (PhysicalAddress)
			return PhysicalAddress;
	}

#endif
	/*
	 * Find ACPI RSDP from Extended BIOS Data Area (EBDA). This
	 * works when the kernel was started from BIOS bootloader,
	 * or for Xen PV when Xen was started from BIOS bootloader.
	 */
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
    ACPI_OSD_HANDLER ServiceRoutine, void *Context, void **cookiep,
    const char *xname)
{
	void *ih;

	ih = acpi_md_intr_establish(InterruptNumber, IPL_TTY, IST_LEVEL,
	    (int (*)(void *))ServiceRoutine, Context, /*mpsafe*/true, xname);
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

void *
acpi_md_intr_establish(uint32_t InterruptNumber, int ipl, int type,
    int (*handler)(void *), void *arg, bool mpsafe, const char *xname)
{
	void *ih;
	struct pic *pic;
	int irq = InterruptNumber, pin;
#if NIOAPIC > 0
	struct ioapic_softc *ioapic;
	struct acpi_md_override ovr;
	struct mp_intr_map tmpmap, *mip, **mipp = NULL;
	intr_handle_t mpih;
	int redir, mpflags;

	/*
	 * ACPI interrupts default to level-triggered active-low.
	 */

	mpflags = (MPS_INTTR_LEVEL << 2) | MPS_INTPO_ACTLO;
	redir = IOAPIC_REDLO_LEVEL | IOAPIC_REDLO_ACTLO;

	/*
	 * Apply any MADT override setting.
	 */

	ovr.irq = irq;
	ovr.pin = -1;
	if (acpi_madt_map() == AE_OK) {
		acpi_madt_walk(acpi_md_findoverride, &ovr);
		acpi_madt_unmap();
	} else {
		aprint_debug("acpi_madt_map() failed, can't check for MADT override\n");
	}

	if (ovr.pin != -1) {
		bool sci = irq == AcpiGbl_FADT.SciInterrupt;
		int polarity = ovr.flags & ACPI_MADT_POLARITY_MASK;
		int trigger = ovr.flags & ACPI_MADT_TRIGGER_MASK;

		irq = ovr.pin;
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

	pic = NULL;
	pin = irq;

	/*
	 * If the interrupt is handled via IOAPIC, update the map.
	 * If the map isn't set up yet, install a temporary one.
	 * Identify ISA & EISA interrupts
	 */
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, irq, &mpih) == 0 ||
		    intr_find_mpmapping(mp_eisa_bus, irq, &mpih) == 0) {
			if (!APIC_IRQ_ISLEGACY(mpih)) {
				pin = APIC_IRQ_PIN(mpih);
				ioapic = ioapic_find(APIC_IRQ_APIC(mpih));
				if (ioapic != NULL)
					pic = &ioapic->sc_pic;
			}
		}
	}

	if (pic == NULL) {
		/*
		 * If the interrupt is handled via IOAPIC, update the map.
		 * If the map isn't set up yet, install a temporary one.
		 */
		ioapic = ioapic_find_bybase(irq);
		if (ioapic != NULL) {
			pic = &ioapic->sc_pic;

			if (pic->pic_type == PIC_IOAPIC) {
				pin = irq - pic->pic_vecbase;
				irq = -1;
			} else {
				pin = irq;
			}

			mip = ioapic->sc_pins[pin].ip_map;
			if (mip) {
				mip->flags &= ~0xf;
				mip->flags |= mpflags;
				mip->redir &= ~(IOAPIC_REDLO_LEVEL |
						IOAPIC_REDLO_ACTLO);
				mip->redir |= redir;
			} else {
				mipp = &ioapic->sc_pins[pin].ip_map;
				*mipp = &tmpmap;
				tmpmap.redir = redir;
				tmpmap.flags = mpflags;
			}
		}
	}

	if (pic == NULL)
#endif
	{
		pic = &i8259_pic;
		pin = irq;
	}

	ih = intr_establish_xname(irq, pic, pin, type, ipl,
	    handler, arg, mpsafe, xname);

#if NIOAPIC > 0
	if (mipp) {
		*mipp = NULL;
	}
#endif

	return ih;
}

void
acpi_md_intr_mask(void *ih)
{
	intr_mask(ih);
}

void
acpi_md_intr_unmask(void *ih)
{
	intr_unmask(ih);
}

void
acpi_md_intr_disestablish(void *ih)
{
	intr_disestablish(ih);
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
		if ((*pte & PTE_P) == 0) {
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
		if ((*pte & (PTE_P|PTE_W)) != (PTE_P|PTE_W)) {
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

#ifndef XENPV
	if (lookup_bootinfo(BTINFO_EFIMEMMAP) != NULL)
		bim = efi_get_e820memmap();
	else
#endif
		bim = lookup_bootinfo(BTINFO_MEMMAP);
	if (bim == NULL)
		return false;

	size = *bus_end - bus_start + 1;
	size *= ACPIMCFG_SIZE_PER_BUS;
	for (i = 0; i < bim->num; i++) {
		mapaddr = bim->entry[i].addr;
		mapsize = bim->entry[i].size;
		type = bim->entry[i].type;

		aprint_debug("MCFG: MEMMAP: 0x%016" PRIx64
		    "-0x%016" PRIx64 ", size=0x%016" PRIx64
		    ", type=%d(%s)\n",
		    mapaddr, mapaddr + mapsize - 1, mapsize, type,
		    (type == BIM_Memory) ?  "Memory" :
		    (type == BIM_Reserved) ?  "Reserved" :
		    (type == BIM_ACPI) ? "ACPI" :
		    (type == BIM_NVS) ? "NVS" :
		    (type == BIM_PMEM) ? "Persistent" :
		    (type == BIM_PRAM) ? "Persistent (Legacy)" :
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

#ifndef XENPV
	acpi_md_sleep_init();
#endif

	acpimcfg_init(x86_bus_space_mem, &acpi_md_mcfg_ops);
}

#ifndef XENPV
void
device_acpi_register(device_t dev, void *aux)
{
	device_t parent;
	bool device_is_vga, device_is_pci, device_is_isa;

	parent = device_parent(dev);
	if (parent == NULL)
		return;

	device_is_vga = device_is_a(dev, "vga") || device_is_a(dev, "genfb");
	device_is_pci = device_is_a(parent, "pci");
	device_is_isa = device_is_a(parent, "isa");

	if (device_is_vga && (device_is_pci || device_is_isa)) {
		extern int acpi_md_vbios_reset;

		acpi_md_vbios_reset = VBIOS_RESET_DEFAULT;
	}
}
#endif
