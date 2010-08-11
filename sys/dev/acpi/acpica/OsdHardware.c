/*	$NetBSD: OsdHardware.c,v 1.4.10.2 2010/08/11 22:53:17 yamt Exp $	*/

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
 * OS Services Layer
 *
 * 6.7: Address Space Access: Port Input/Output
 * 6.8: Address Space Access: Memory and Memory Mapped I/O
 * 6.9: Address Space Access: PCI Configuration Space
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: OsdHardware.c,v 1.4.10.2 2010/08/11 22:53:17 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>

#include <machine/acpi_machdep.h>

/*
 * ACPICA doesn't provide much in the way of letting us know which
 * hardware resources it wants to use.  We therefore have to resort
 * to calling machinde-dependent code to do the access for us.
 */

/*
 * AcpiOsReadPort:
 *
 *	Read a value from an input port.
 */
ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{

	switch (Width) {
	case 8:
		*Value = acpi_md_OsIn8(Address);
		break;

	case 16:
		*Value = acpi_md_OsIn16(Address);
		break;

	case 32:
		*Value = acpi_md_OsIn32(Address);
		break;

	default:
		return AE_BAD_PARAMETER;
	}

	return AE_OK;
}

/*
 * AcpiOsWritePort:
 *
 *	Write a value to an output port.
 */
ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{

	switch (Width) {
	case 8:
		acpi_md_OsOut8(Address, Value);
		break;

	case 16:
		acpi_md_OsOut16(Address, Value);
		break;

	case 32:
		acpi_md_OsOut32(Address, Value);
		break;

	default:
		return AE_BAD_PARAMETER;
	}

	return AE_OK;
}

/*
 * AcpiOsReadMemory:
 *
 *	Read a value from a memory location.
 */
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
	void *LogicalAddress;
	ACPI_STATUS rv = AE_OK;

	LogicalAddress = AcpiOsMapMemory(Address, Width / 8);
	if (LogicalAddress == NULL)
		return AE_NOT_EXIST;

	switch (Width) {
	case 8:
		*Value = *(volatile uint8_t *) LogicalAddress;
		break;

	case 16:
		*Value = *(volatile uint16_t *) LogicalAddress;
		break;

	case 32:
		*Value = *(volatile uint32_t *) LogicalAddress;
		break;

	default:
		rv = AE_BAD_PARAMETER;
	}

	AcpiOsUnmapMemory(LogicalAddress, Width / 8);

	return rv;
}

/*
 * AcpiOsWriteMemory:
 *
 *	Write a value to a memory location.
 */
ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	void *LogicalAddress;
	ACPI_STATUS rv = AE_OK;

	LogicalAddress = AcpiOsMapMemory(Address, Width / 8);
	if (LogicalAddress == NULL)
		return AE_NOT_FOUND;

	switch (Width) {
	case 8:
		*(volatile uint8_t *) LogicalAddress = Value;
		break;

	case 16:
		*(volatile uint16_t *) LogicalAddress = Value;
		break;

	case 32:
		*(volatile uint32_t *) LogicalAddress = Value;
		break;

	default:
		rv = AE_BAD_PARAMETER;
	}

	AcpiOsUnmapMemory(LogicalAddress, Width / 8);

	return rv;
}

/*
 * AcpiOsReadPciConfiguration:
 *
 *	Read a value from a PCI configuration register.
 */
ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, void *Value,
    UINT32 Width)
{
	pcitag_t tag;
	pcireg_t tmp;

	/* XXX Need to deal with "segment" ("hose" in Alpha terminology). */

	if (PciId->Bus >= 256 || PciId->Device >= 32 || PciId->Function >= 8)
		return AE_BAD_PARAMETER;

	tag = pci_make_tag(acpi_softc->sc_pc, PciId->Bus, PciId->Device,
	    PciId->Function);
	tmp = pci_conf_read(acpi_softc->sc_pc, tag, Register & ~3);

	switch (Width) {
	case 8:
		*(uint8_t *) Value = (tmp >> ((Register & 3) * 8)) & 0xff;
		break;

	case 16:
		*(uint16_t *) Value = (tmp >> ((Register & 3) * 8)) & 0xffff;
		break;

	case 32:
		*(uint32_t *) Value = tmp;
		break;

	default:
		return AE_BAD_PARAMETER;
	}

	return AE_OK;
}

/*
 * AcpiOsWritePciConfiguration:
 *
 *	Write a value to a PCI configuration register.
 */
ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
    ACPI_INTEGER Value, UINT32 Width)
{
	pcitag_t tag;
	pcireg_t tmp;

	/* XXX Need to deal with "segment" ("hose" in Alpha terminology). */

	tag = pci_make_tag(acpi_softc->sc_pc, PciId->Bus, PciId->Device,
	    PciId->Function);

	switch (Width) {
	case 8:
		tmp = pci_conf_read(acpi_softc->sc_pc, tag, Register & ~3);
		tmp &= ~(0xff << ((Register & 3) * 8));
		tmp |= (Value << ((Register & 3) * 8));
		break;

	case 16:
		tmp = pci_conf_read(acpi_softc->sc_pc, tag, Register & ~3);
		tmp &= ~(0xffff << ((Register & 3) * 8));
		tmp |= (Value << ((Register & 3) * 8));
		break;

	case 32:
		tmp = Value;
		break;

	default:
		return AE_BAD_PARAMETER;
	}

	pci_conf_write(acpi_softc->sc_pc, tag, Register & ~3, tmp);

	return AE_OK;
}

/*
 * acpi_os_derive_pciid_rec:
 *
 *	Helper function for AcpiOsDerivePciId.  The parameters are:
 *	- chandle:	a handle to the node whose PCI id shall be derived.
 *	- rhandle:	a handle the PCI root bridge upstream of chandle.
 *	- pciid:	where the derived PCI id is returned.
 *
 *	This function assumes that rhandle is a proper ancestor of chandle,
 *	and that pciid has already been filled by ACPICA:
 *	- segment# and bus# obtained from _SEG and _BBN on rhandle,
 *	- device# and function# obtained from _ADR on the ACPI device node
 *	  whose scope chandle is in).
 */
static ACPI_STATUS
acpi_os_derive_pciid_rec(ACPI_HANDLE chandle, ACPI_HANDLE rhandle, ACPI_PCI_ID *pciid)
{
	ACPI_HANDLE phandle;
	ACPI_INTEGER address;
	ACPI_OBJECT_TYPE objtype;
	ACPI_STATUS rv;
	uint16_t valb;

	KASSERT(chandle != rhandle);

	/*
	 * Get parent device node.  This is should not fail since chandle has
	 * at least one ancestor that is a device node: rhandle.
	 */
	phandle = chandle;
	do {
		rv = AcpiGetParent(phandle, &phandle);
		if (ACPI_FAILURE(rv))
			return rv;
		rv = AcpiGetType(phandle, &objtype);
		if (ACPI_FAILURE(rv))
			return rv;
	}
	while (objtype != ACPI_TYPE_DEVICE);

	/*
	 * If the parent is rhandle then we have nothing to do since ACPICA
	 * has pre-filled the PCI id to the best it could.
	 */
	if (phandle == rhandle)
		return AE_OK;

	/* Recursive call to get PCI id of the parent */
	rv = acpi_os_derive_pciid_rec(phandle, rhandle, pciid);
	if (ACPI_FAILURE(rv))
		return rv;

	/*
	 * If this is not an ACPI device, return the PCI id of its parent.
	 */
	rv = AcpiGetType(chandle, &objtype);
	if (ACPI_FAILURE(rv))
		return rv;
	if (objtype != ACPI_TYPE_DEVICE)
		return AE_OK;

	/*
	 * This is an ACPI device node.  Its parent device node is not a PCI
	 * root bridge.  Check that it is a PCI-to-PCI bridge and get its
	 * secondary bus#.
	 */
	rv = acpi_pcidev_ppb_downbus(pciid->Segment, pciid->Bus, pciid->Device,
	    pciid->Function, &valb);
	if (ACPI_FAILURE(rv))
		return rv;

	/* Get address (contains dev# and fun# for PCI devices). */
	rv = acpi_eval_integer(chandle, METHOD_NAME__ADR, &address);
	if (ACPI_FAILURE(rv))
		return rv;

	pciid->Bus = valb;
	pciid->Device = ACPI_HIWORD(ACPI_LODWORD(address));
	pciid->Function = ACPI_LOWORD(ACPI_LODWORD(address));
	return AE_OK;
}

/*
 * AcpiOsDerivePciId:
 *
 *	Derive correct PCI bus# by traversing bridges.
 *
 *	In ACPICA release 20100331 (as well as older versions), the interface
 *	of this function is not correctly documented in the ACPICA programmer
 *	reference.  The correct interface parameters to this function are:
 *	- rhandle:	a handle the PCI root bridge upstream of handle.
 *	- chandle:	a handle to the PCI_Config operation region.
 *	- PciId:	where the derived PCI id is returned.
 */
void
AcpiOsDerivePciId(
    ACPI_HANDLE        rhandle,
    ACPI_HANDLE        chandle,
    ACPI_PCI_ID        **PciId)
{
	ACPI_PCI_ID pciid;
	ACPI_STATUS rv;

	if (chandle == rhandle)
		return;

	pciid = **PciId;
	rv = acpi_os_derive_pciid_rec(chandle, rhandle, &pciid);
	if (ACPI_FAILURE(rv))
		return;

	(*PciId)->Bus = pciid.Bus;
}
