/*	$NetBSD: OsdEnvironment.c,v 1.2.2.3 2002/06/23 17:45:04 jdolecek Exp $	*/

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
 * 6.1: Environmental support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: OsdEnvironment.c,v 1.2.2.3 2002/06/23 17:45:04 jdolecek Exp $");

#include <sys/types.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpi_osd.h>

#include <machine/acpi_machdep.h>

#define	_COMPONENT	ACPI_OS_SERVICES
ACPI_MODULE_NAME("ENVIRONMENT");

/*
 * AcpiOsInitialize:
 *
 *	Initialize the OSL subsystem.
 */
ACPI_STATUS
AcpiOsInitialize(void)
{
	ACPI_STATUS rv;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	/* Initialize the Osd Scheduler. */
	acpi_osd_sched_init();

	rv = acpi_md_OsInitialize();

	return_ACPI_STATUS(rv);
}

/*
 * AcpiOsTerminate:
 *
 *	Terminate the OSL subsystem.
 */
ACPI_STATUS
AcpiOsTerminate(void)
{
	ACPI_STATUS rv;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	/* Tear down the Osd Scheduler. */
	acpi_osd_sched_fini();

	rv = acpi_md_OsTerminate();

	return_ACPI_STATUS(rv);
}

/*
 * AcpiOsGetRootPointer:
 *
 *	Obtain the Root ACPI talbe pointer (RSDP)
 */
ACPI_STATUS
AcpiOsGetRootPointer(UINT32 Flags, ACPI_POINTER *PhysicalAddress)
{

	/*
	 * IA-32: Use AcpiFindRootPointer() to locate the RSDP.
	 *
	 * IA-64: Use the EFI.
	 *
	 * We let MD code handle this since there are multiple
	 * ways to do it.
	 */

	return (acpi_md_OsGetRootPointer(Flags, PhysicalAddress));
}
