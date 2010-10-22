/*	$NetBSD: acpi_quirks.c,v 1.10.4.3 2010/10/22 07:21:53 uebayasi Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: acpi_quirks.c,v 1.10.4.3 2010/10/22 07:21:53 uebayasi Exp $");

#include "opt_acpi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/acpi/acpivar.h>

#define AQ_GT		0	/* > */
#define AQ_LT		1	/* < */
#define AQ_GTE		2	/* >= */
#define AQ_LTE		3	/* <= */
#define AQ_EQ		4	/* == */

static int acpi_rev_cmp(uint32_t, uint32_t, int);

/*
 * XXX: Add more.
 */
static struct acpi_quirk acpi_quirks[] = {

	{ ACPI_SIG_FADT, "ASUS  ", 0x30303031, AQ_LTE, "CUV4X-D ",
	  ACPI_QUIRK_BROKEN },

	{ ACPI_SIG_FADT, "PTLTD ", 0x06040000, AQ_LTE, "  FACP  ",
	  ACPI_QUIRK_BROKEN },

	{ ACPI_SIG_FADT, "NVIDIA", 0x06040000, AQ_EQ, "CK8     ",
	  ACPI_QUIRK_IRQ0 },

	{ ACPI_SIG_FADT, "HP    ", 0x06040012, AQ_LTE, "HWPC20F ",
	  ACPI_QUIRK_BROKEN },
};

static int
acpi_rev_cmp(uint32_t tabval, uint32_t wanted, int op)
{
	switch (op) {
	case AQ_GT:
		if (tabval > wanted)
			return 0;
		else
			return 1;
	case AQ_LT:
		if (tabval < wanted)
			return 0;
		else
			return 1;
	case AQ_LTE:
		if (tabval <= wanted)
			return 0;
		else
			return 1;
	case AQ_GTE:
		if (tabval >= wanted)
			return 0;
		else
			return 1;
	case AQ_EQ:
		if (tabval == wanted)
			return 0;
		else
			return 1;
	}
	return 1;
}

#ifdef ACPI_BLACKLIST_YEAR
static int
acpi_find_bios_year(void)
{
	const char *datestr = pmf_get_platform("firmware-date");
	unsigned long date;

	if (datestr == NULL)
		return -1;

	date = strtoul(datestr, NULL, 10);
	if (date == 0 || date == ULONG_MAX)
		return -1;
	if (date < 19000000 || date > 99999999)
		return -1;
	return date / 10000;
}
#endif

/*
 * Simple function to search the quirk table. Only to be used after
 * AcpiLoadTables has been successfully called.
 */
int
acpi_find_quirks(void)
{
	int i, nquirks;
	struct acpi_quirk *aqp;
	ACPI_TABLE_HEADER fadt, dsdt, xsdt, *hdr;
#ifdef ACPI_BLACKLIST_YEAR
	int year = acpi_find_bios_year();

	if (year != -1 && year <= ACPI_BLACKLIST_YEAR)
		return ACPI_QUIRK_OLDBIOS;
#endif

	nquirks = __arraycount(acpi_quirks);

	if (ACPI_FAILURE(AcpiGetTableHeader(ACPI_SIG_FADT, 0, &fadt)))
		memset(&fadt, 0, sizeof(fadt));
	if (ACPI_FAILURE(AcpiGetTableHeader(ACPI_SIG_DSDT, 0, &dsdt)))
		memset(&dsdt, 0, sizeof(dsdt));
	if (ACPI_FAILURE(AcpiGetTableHeader(ACPI_SIG_XSDT, 0, &xsdt)))
		memset(&xsdt, 0, sizeof(xsdt));

	for (i = 0; i < nquirks; i++) {
		aqp = &acpi_quirks[i];
		if (!strncmp(aqp->aq_tabletype, ACPI_SIG_DSDT, 4))
			hdr = &dsdt;
		else if (!strncmp(aqp->aq_tabletype, ACPI_SIG_XSDT, 4))
			hdr = &xsdt;
		else if (!strncmp(aqp->aq_tabletype, ACPI_SIG_FADT, 4))
			hdr = &fadt;
		else
			continue;
		if (strncmp(aqp->aq_oemid, hdr->OemId, strlen(aqp->aq_oemid)))
			continue;
		if (acpi_rev_cmp(aqp->aq_oemrev, hdr->OemRevision,
		    aqp->aq_cmpop))
			continue;
		if (strncmp(aqp->aq_tabid, hdr->OemTableId,
		    strlen(aqp->aq_tabid)))
			continue;
		return aqp->aq_quirks;
	}
	return 0;
}
