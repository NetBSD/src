/* $NetBSD: platform.c,v 1.9 2010/09/06 15:54:27 jmcneill Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "isa.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.9 2010/09/06 15:54:27 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/pmf.h>

#include <dev/isa/isavar.h>

#include <arch/x86/include/smbiosvar.h>

void		platform_init(void);	/* XXX */
static void	platform_add(struct smbtable *, const char *, int);
static void	platform_add_date(struct smbtable *, const char *, int);
static void	platform_print(void);

void
platform_init(void)
{
	struct smbtable smbios;
	struct smbios_sys *psys;
	struct smbios_struct_bios *pbios;
	struct smbios_slot *pslot;
	int nisa, nother;

	smbios.cookie = 0;
	if (smbios_find_table(SMBIOS_TYPE_SYSTEM, &smbios)) {
		psys = smbios.tblhdr;

		platform_add(&smbios, "system-manufacturer", psys->vendor);
		platform_add(&smbios, "system-product-name", psys->product);
		platform_add(&smbios, "system-version", psys->version);
		platform_add(&smbios, "system-serial-number", psys->serial);
	}

	smbios.cookie = 0;
	if (smbios_find_table(SMBIOS_TYPE_BIOS, &smbios)) {
		pbios = smbios.tblhdr;

		platform_add(&smbios, "firmware-vendor", pbios->vendor);
		platform_add(&smbios, "firmware-version", pbios->version);
		platform_add_date(&smbios, "firmware-date", pbios->release);
	}

	smbios.cookie = 0;
	nisa = 0;
	nother = 0;
	while (smbios_find_table(SMBIOS_TYPE_SLOTS, &smbios)) {
		pslot = smbios.tblhdr;
		switch (pslot->type) {
		case SMBIOS_SLOT_ISA:
		case SMBIOS_SLOT_EISA:
			nisa++;
			break;
		default:
			nother++;
			break;
		}
	}

#if NISA > 0
	if ((nother | nisa) != 0) {
		/* Only if there seems to be good expansion slot info. */
		isa_set_slotcount(nisa);
	}
#endif

	platform_print();
}

static void
platform_print(void)
{
	const char *manuf, *prod, *ver;

	manuf = pmf_get_platform("system-manufacturer");
	prod = pmf_get_platform("system-product-name");
	ver = pmf_get_platform("system-version");

	if (manuf == NULL)
		aprint_verbose("Generic");
	else
		aprint_verbose("%s", manuf);
	if (prod == NULL)
		aprint_verbose(" PC");
	else
		aprint_verbose(" %s", prod);
	if (ver != NULL)
		aprint_verbose(" (%s)", ver);
	aprint_verbose("\n");
}

static void
platform_add(struct smbtable *tbl, const char *key, int idx)
{
	char tmpbuf[128]; /* XXX is this long enough? */

	if (smbios_get_string(tbl, idx, tmpbuf, 128) != NULL)
		pmf_set_platform(key, tmpbuf);
}

static int
platform_scan_date(char *buf, unsigned int *month, unsigned int *day,
    unsigned int *year)
{
	char *p, *s;

	s = buf;
	p = strchr(s, '/');
	if (p) *p = '\0';
	*month = strtoul(s, NULL, 10);
	if (!p) return 1;

	s = p + 1;
	p = strchr(s, '/');
	if (p) *p = '\0';
	*day = strtoul(s, NULL, 10);
	if (!p) return 2;

	s = p + 1;
	*year = strtoul(s, NULL, 10);
	return 3;
}

static void
platform_add_date(struct smbtable *tbl, const char *key, int idx)
{
	unsigned int month, day, year;
	char tmpbuf[128], datestr[9];

	if (smbios_get_string(tbl, idx, tmpbuf, 128) == NULL)
		return;
	if (platform_scan_date(tmpbuf, &month, &day, &year) != 3)
		return;
	if (month == 0 || month > 12 || day == 0 || day > 31)
		return;
	if (year < 100)
		year += 1900;
	if (year > 9999)
		return;
	sprintf(datestr, "%04u%02u%02u", year, month, day);
	pmf_set_platform(key, datestr);
}
