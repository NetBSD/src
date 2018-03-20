/*	$NetBSD: acpi_machdep.c,v 1.7 2018/03/20 12:14:52 bouyer Exp $	*/
/*
 * Copyright (c) 2009 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Machine-dependent routines for ACPICA.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.7 2018/03/20 12:14:52 bouyer Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/efi.h>
#include <machine/intrdefs.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

#include <machine/acpi_machdep.h>


static struct uuid acpi20_table = EFI_TABLE_ACPI20;
static u_long acpi_root_phys;
int has_i8259 = 0;


ACPI_STATUS
acpi_md_OsInitialize(void)
{

	if (((ia64_get_cpuid(3) >> 24) & 0xff) == 0x07)
		has_i8259 = 1; /* Firmware on old Itanium systems is broken */

	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
acpi_md_OsGetRootPointer(void)
{
	void *acpi_root;

	if (acpi_root_phys == 0) {
		acpi_root = efi_get_table(&acpi20_table);
		if (acpi_root == NULL)
			return 0;
		acpi_root_phys = IA64_RR_MASK((u_long)acpi_root);
	}

	return acpi_root_phys;
}

ACPI_STATUS
acpi_md_OsInstallInterruptHandler(UINT32 InterruptNumber,
				  ACPI_OSD_HANDLER ServiceRoutine,
				  void *Context, void **cookiep,
				  const char *xname)
{
	static int isa_irq_to_vector_map[16] = {
	        /* i8259 IRQ translation, first 16 entries */
		0x2f, 0x20, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29,
		0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21,
	};
	int irq;
	void *ih;

	if (has_i8259 && InterruptNumber < 16)
		irq = isa_irq_to_vector_map[InterruptNumber];
	else
		irq = InterruptNumber;

	/*
	 * XXX probably, IPL_BIO is enough.
	 */
	ih = intr_establish(irq, IST_LEVEL, IPL_TTY,
	    (int (*)(void *)) ServiceRoutine, Context);
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
acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, UINT32 Length,
		    void **LogicalAddress)
{

	if (bus_space_map(IA64_BUS_SPACE_MEM, PhysicalAddress, Length,
	    0, (bus_space_handle_t *) LogicalAddress) == 0)
		return AE_OK;

	return AE_NO_MEMORY;
}

void
acpi_md_OsUnmapMemory(void *LogicalAddress, UINT32 Length)
{

	bus_space_unmap(IA64_BUS_SPACE_MEM, (bus_space_handle_t) LogicalAddress,
	    Length);
}

ACPI_STATUS
acpi_md_OsGetPhysicalAddress(void *LogicalAddress,
			     ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	paddr_t pa;

printf("%s\n", __func__);
	if (pmap_extract(pmap_kernel(), (vaddr_t) LogicalAddress, &pa)) {
		*PhysicalAddress = pa;
		return AE_OK;
	}

	return AE_ERROR;
}

BOOLEAN
acpi_md_OsReadable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = TRUE;
printf("%s: not yet...\n", __func__);

	return rv;
}

BOOLEAN
acpi_md_OsWritable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = FALSE;
printf("%s: not yet...\n", __func__);
	return rv;
}

void
acpi_md_OsEnableInterrupt(void)
{

	enable_intr();
}

void
acpi_md_OsDisableInterrupt(void)
{

	disable_intr();
}

uint32_t
acpi_md_pdc(void)
{
	return 0;
}

uint32_t
acpi_md_ncpus(void)
{
	return 0;		/* XXX. */
}

void
acpi_md_callback(struct acpi_softc *sc)
{
	/* Nothing. */
}

int
acpi_md_sleep(int state)
{
printf("%s: not yet...\n", __func__);
	return 0;
}
