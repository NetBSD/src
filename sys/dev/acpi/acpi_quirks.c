/*	$NetBSD: acpi_quirks.c,v 1.5 2004/04/10 11:48:10 kochi Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: acpi_quirks.c,v 1.5 2004/04/10 11:48:10 kochi Exp $");

#include "opt_acpi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

static struct acpi_quirk acpi_quirks[] = {
	/*
	 * This implementation seems to be in widespread use, but
	 * unfortunately, on some systems, it constructs a PCI hierarchy
	 * that doesn't match reality at all (like on SuperMicro boards).
	 */
	{ "PTLTD ", 0x06040000, ACPI_QUIRK_BADPCI | ACPI_QUIRK_BADIRQ },
	/*
	 * This is on my Appro 1224 Xi. It does not find all the busses
	 * in the ACPI tables.
	 */
	{ "A M I ", 0x02000304, ACPI_QUIRK_BADPCI | ACPI_QUIRK_BADIRQ },
};

/*
 * Simple function to search the quirk table. Only to be used after
 * AcpiLoadTables has been successfully called.
 */
int
acpi_find_quirks(void)
{
	int i, nquirks;
	struct acpi_quirk *aqp;

	nquirks = sizeof(acpi_quirks) / sizeof(struct acpi_quirk);

	for (i = 0; i < nquirks; i++) {
		aqp = &acpi_quirks[i];
		if (!strncmp(aqp->aq_oemid, AcpiGbl_XSDT->OemId, strlen(aqp->aq_oemid)) &&
		    aqp->aq_oemrev == AcpiGbl_XSDT->OemRevision)
			return aqp->aq_quirks;
	}
	return 0;
}
