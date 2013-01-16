/* $NetBSD: platform.c,v 1.11.6.2 2013/01/16 05:33:10 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.11.6.2 2013/01/16 05:33:10 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/uuid.h>
#include <sys/pmf.h>

#if NISA > 0
#include <dev/isa/isavar.h>
#endif

#include <arch/x86/include/smbiosvar.h>

static int platform_dminode = CTL_EOL;

void		platform_init(void);	/* XXX */
static void	platform_add(struct smbtable *, const char *, int);
static void	platform_add_date(struct smbtable *, const char *, int);
static void	platform_add_uuid(struct smbtable *, const char *,
				  const uint8_t *);
static int	platform_dmi_sysctl(SYSCTLFN_PROTO);
static void	platform_print(void);

/* list of private DMI sysctl nodes */
static const char *platform_private_nodes[] = {
	"board-serial",
	"system-serial",
	"system-uuid",
	NULL
};

void
platform_init(void)
{
	struct smbtable smbios;
	struct smbios_sys *psys;
	struct smbios_struct_bios *pbios;
	struct smbios_board *pboard;
	struct smbios_slot *pslot;
	int nisa, nother;

	smbios.cookie = 0;
	if (smbios_find_table(SMBIOS_TYPE_SYSTEM, &smbios)) {
		psys = smbios.tblhdr;

		platform_add(&smbios, "system-vendor", psys->vendor);
		platform_add(&smbios, "system-product", psys->product);
		platform_add(&smbios, "system-version", psys->version);
		platform_add(&smbios, "system-serial", psys->serial);
		platform_add_uuid(&smbios, "system-uuid", psys->uuid);
	}

	smbios.cookie = 0;
	if (smbios_find_table(SMBIOS_TYPE_BIOS, &smbios)) {
		pbios = smbios.tblhdr;

		platform_add(&smbios, "bios-vendor", pbios->vendor);
		platform_add(&smbios, "bios-version", pbios->version);
		platform_add_date(&smbios, "bios-date", pbios->release);
	}

	smbios.cookie = 0;
	if (smbios_find_table(SMBIOS_TYPE_BASEBOARD, &smbios)) {
		pboard = smbios.tblhdr;

		platform_add(&smbios, "board-vendor", pboard->vendor);
		platform_add(&smbios, "board-product", pboard->product);
		platform_add(&smbios, "board-version", pboard->version);
		platform_add(&smbios, "board-serial", pboard->serial);
		platform_add(&smbios, "board-asset-tag", pboard->asset);
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
	const char *vend, *prod, *ver;

	vend = pmf_get_platform("system-vendor");
	prod = pmf_get_platform("system-product");
	ver = pmf_get_platform("system-version");

	if (vend == NULL)
		aprint_verbose("Generic");
	else
		aprint_verbose("%s", vend);
	if (prod == NULL)
		aprint_verbose(" PC");
	else
		aprint_verbose(" %s", prod);
	if (ver != NULL)
		aprint_verbose(" (%s)", ver);
	aprint_verbose("\n");
}

static bool
platform_sysctl_is_private(const char *key)
{
	unsigned int n;

	for (n = 0; platform_private_nodes[n] != NULL; n++) {
		if (strcmp(key, platform_private_nodes[n]) == 0) {
			return true;
		}
	}

	return false;
}

static void
platform_create_sysctl(const char *key)
{
	int flags = 0, err;

	if (pmf_get_platform(key) == NULL)
		return;

	/* If the key is marked private, set CTLFLAG_PRIVATE flag */
	if (platform_sysctl_is_private(key))
		flags |= CTLFLAG_PRIVATE;

	err = sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_READONLY | flags, CTLTYPE_STRING,
	    key, NULL, platform_dmi_sysctl, 0, NULL, 0,
	    CTL_MACHDEP, platform_dminode, CTL_CREATE, CTL_EOL);
	if (err != 0)
		printf("platform: sysctl_createv "
		    "(machdep.dmi.%s) failed, err = %d\n",
		    key, err);
}

static void
platform_add(struct smbtable *tbl, const char *key, int idx)
{
	char tmpbuf[128]; /* XXX is this long enough? */

	if (smbios_get_string(tbl, idx, tmpbuf, 128) != NULL) {
		/* add to platform dictionary */
		pmf_set_platform(key, tmpbuf);

		/* create sysctl node */
		platform_create_sysctl(key);
	}
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
	if (year > 9999)
		return;
	if (year < 70)
		year += 2000;
	else if (year < 100)
		year += 1900;
	sprintf(datestr, "%04u%02u%02u", year, month, day);
	pmf_set_platform(key, datestr);
	platform_create_sysctl(key);
}

static void
platform_add_uuid(struct smbtable *tbl, const char *key, const uint8_t *buf)
{
	struct uuid uuid;
	char tmpbuf[UUID_STR_LEN];

	uuid_dec_le(buf, &uuid);
	uuid_snprintf(tmpbuf, sizeof(tmpbuf), &uuid);

	pmf_set_platform(key, tmpbuf);
	platform_create_sysctl(key);
}

static int
platform_dmi_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	const char *v;
	int err = 0;

	node = *rnode;

	v = pmf_get_platform(node.sysctl_name);
	if (v == NULL)
		return ENOENT;

	node.sysctl_data = __UNCONST(v);
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	return 0;
}

SYSCTL_SETUP(sysctl_dmi_setup, "sysctl machdep.dmi subtree setup")
{
	const struct sysctlnode *rnode;
	int err;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep",
	    NULL, NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);
	if (err)
		return;

	err = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "dmi",
	    SYSCTL_DESCR("DMI table information"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (err)
		return;

	platform_dminode = rnode->sysctl_num;
}
