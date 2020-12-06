/* $NetBSD: acpi_quirks.c,v 1.22 2020/12/06 11:38:28 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

__KERNEL_RCSID(0, "$NetBSD: acpi_quirks.c,v 1.22 2020/12/06 11:38:28 jmcneill Exp $");

#include "opt_acpi.h"

#include <sys/param.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define _COMPONENT	ACPI_UTILITIES
ACPI_MODULE_NAME	("acpi_quirks")

#define AQ_GT		0	/* > */
#define AQ_LT		1	/* < */
#define AQ_GTE		2	/* >= */
#define AQ_LTE		3	/* <= */
#define AQ_EQ		4	/* == */

static int acpi_quirks_revcmp(uint32_t, uint32_t, int);

static const struct acpi_quirk acpi_quirks[] = {

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
acpi_quirks_revcmp(uint32_t tabval, uint32_t wanted, int op)
{

	switch (op) {

	case AQ_GT:
		return (tabval > wanted) ? 0 : 1;

	case AQ_LT:
		return (tabval < wanted) ? 0 : 1;

	case AQ_LTE:
		return (tabval <= wanted) ? 0 : 1;

	case AQ_GTE:
		return (tabval >= wanted) ? 0 : 1;

	case AQ_EQ:
		return (tabval == wanted) ? 0 : 1;

	default:
		return 1;
	}
}

#ifdef ACPI_BLACKLIST_YEAR
static int
acpi_quirks_bios_year(void)
{
	const char *datestr = pmf_get_platform("bios-date");
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
 * Simple function to search the quirk table. Only to be
 * used after AcpiLoadTables() has been successfully called.
 */
int
acpi_find_quirks(void)
{
	ACPI_TABLE_HEADER fadt, dsdt, xsdt, *hdr;
	const struct acpi_quirk *aq;
	ACPI_STATUS rv;
	size_t i, len;

#ifdef ACPI_BLACKLIST_YEAR
	int year = acpi_quirks_bios_year();

	if (year != -1 && year <= ACPI_BLACKLIST_YEAR)
		return ACPI_QUIRK_OLDBIOS;
#endif

	rv = AcpiGetTableHeader(ACPI_SIG_FADT, 0, &fadt);

	if (ACPI_FAILURE(rv))
		(void)memset(&fadt, 0, sizeof(fadt));

	rv = AcpiGetTableHeader(ACPI_SIG_DSDT, 0, &dsdt);

	if (ACPI_FAILURE(rv))
		(void)memset(&dsdt, 0, sizeof(dsdt));

	rv = AcpiGetTableHeader(ACPI_SIG_XSDT, 0, &xsdt);

	if (ACPI_FAILURE(rv))
		(void)memset(&xsdt, 0, sizeof(xsdt));

	for (i = 0; i < __arraycount(acpi_quirks); i++) {

		aq = &acpi_quirks[i];

		if (strncmp(aq->aq_tabletype, ACPI_SIG_DSDT, 4) == 0)
			hdr = &dsdt;
		else if (strncmp(aq->aq_tabletype, ACPI_SIG_XSDT, 4) == 0)
			hdr = &xsdt;
		else if (strncmp(aq->aq_tabletype, ACPI_SIG_FADT, 4) == 0)
			hdr = &fadt;
		else {
			continue;
		}

		len = strlen(aq->aq_oemid);

		if (strncmp(aq->aq_oemid, hdr->OemId, len) != 0)
			continue;

		if (acpi_quirks_revcmp(aq->aq_oemrev,
			hdr->OemRevision, aq->aq_cmpop) != 0)
			continue;

		len = strlen(aq->aq_tabid);

		if (strncmp(aq->aq_tabid, hdr->OemTableId, len) != 0)
			continue;

		return aq->aq_quirks;
	}

	return 0;
}
