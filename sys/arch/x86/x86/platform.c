/* $NetBSD: platform.c,v 1.1.8.1 2007/12/27 00:43:28 mjf Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.1.8.1 2007/12/27 00:43:28 mjf Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/pmf.h>

#include <arch/x86/include/smbiosvar.h>

void		platform_init(void);	/* XXX */
static void	platform_add(struct smbtable *, const char *, int);
static void	platform_print(void);

void
platform_init(void)
{
	struct smbtable smbios;
	struct smbios_sys *psys;

	smbios.cookie = 0;

	if (smbios_find_table(SMBIOS_TYPE_SYSTEM, &smbios)) {
		psys = smbios.tblhdr;

		platform_add(&smbios, "system-manufacturer", psys->vendor);
		platform_add(&smbios, "system-product-name", psys->product);
		platform_add(&smbios, "system-version", psys->version);
		platform_add(&smbios, "system-serial-number", psys->serial);
	}

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
		aprint_normal("Generic");
	else
		aprint_normal("%s", manuf);
	if (prod == NULL)
		aprint_normal(" PC");
	else
		aprint_normal(" %s", prod);
	if (ver != NULL)
		aprint_normal(" (%s)", ver);
	aprint_normal("\n");
}

static void
platform_add(struct smbtable *tbl, const char *key, int idx)
{
	char tmpbuf[128]; /* XXX is this long enough? */

	if (smbios_get_string(tbl, idx, tmpbuf, 128) != NULL)
		pmf_set_platform(key, tmpbuf);
}
