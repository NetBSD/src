/*	$NetBSD: config.c,v 1.1.1.1.4.2 2008/10/19 22:40:49 haad Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
__RCSID("$NetBSD: config.c,v 1.1.1.1.4.2 2008/10/19 22:40:49 haad Exp $");
#endif

/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "admin.h"
#include "lib.h"

const char *pkg_vulnerabilities_dir;
const char *pkg_vulnerabilities_file;
const char *pkg_vulnerabilities_url;
const char *ignore_advisories = NULL;
const char tnf_vulnerability_base[] = "ftp://ftp.NetBSD.org/pub/NetBSD/packages/vulns";

static struct config_variable {
	const char *name;
	const char **var;
} config_variables[] = {
	{ "GPG", &gpg_cmd },
	{ "PKGVULNDIR", &pkg_vulnerabilities_dir },
	{ "PKGVULNURL", &pkg_vulnerabilities_url },
	{ "IGNORE_URL", &ignore_advisories },
	{ NULL, NULL }
};

void
pkg_install_config(const char *config_file)
{
	char *value;
	int ret;
	struct config_variable *var;

	for (var = config_variables; var->name != NULL; ++var) {
		value = var_get(config_file, var->name);
		if (value != NULL)
			*var->var = value;
	}

	if (pkg_vulnerabilities_dir == NULL)
		pkg_vulnerabilities_dir = _pkgdb_getPKGDB_DIR();
	ret = asprintf(&value, "%s/pkg-vulnerabilities", pkg_vulnerabilities_dir);
	pkg_vulnerabilities_file = value;
	if (ret == -1)
		err(EXIT_FAILURE, "asprintf failed");
	if (pkg_vulnerabilities_url == NULL) {
		ret = asprintf(&value, "%s/pkg-vulnerabilities.gz",
		    tnf_vulnerability_base);
		pkg_vulnerabilities_url = value;
		if (ret == -1)
			err(EXIT_FAILURE, "asprintf failed");
	}
}

void
pkg_install_show_variable(const char *var_name)
{
	struct config_variable *var;

	for (var = config_variables; var->name != NULL; ++var) {
		if (strcmp(var->name, var_name) != 0)
			continue;
		if (*var->var != NULL)
			puts(*var->var);
	}
}
