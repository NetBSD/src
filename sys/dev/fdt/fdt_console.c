/* $NetBSD: fdt_console.c,v 1.1 2025/09/06 22:53:48 thorpej Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_console.c,v 1.1 2025/09/06 22:53:48 thorpej Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_console.h>

#ifndef FDT_DEFAULT_STDOUT_PATH
#define	FDT_DEFAULT_STDOUT_PATH		"serial0:115200n8"
#endif

const struct fdt_console *
fdtbus_get_console(void)
{
	static const struct fdt_console_info *booted_console = NULL;

	if (booted_console == NULL) {
		__link_set_decl(fdt_consoles, struct fdt_console_info);
		struct fdt_console_info * const *info;
		const struct fdt_console_info *best_info = NULL;
		const int phandle = fdtbus_get_stdout_phandle();
		int best_match = 0;

		if (phandle == -1) {
			printf("WARNING: no console device\n");
			return NULL;
		}

		__link_set_foreach(info, fdt_consoles) {
			const int match = (*info)->ops->match(phandle);
			if (match > best_match) {
				best_match = match;
				best_info = *info;
			}
		}

		booted_console = best_info;
	}

	return booted_console == NULL ? NULL : booted_console->ops;
}

const char *
fdtbus_get_stdout_path(void)
{
	const char *prop;

	const int off = fdt_path_offset(fdtbus_get_data(), "/chosen");
	if (off >= 0) {
		prop = fdt_getprop(fdtbus_get_data(), off, "stdout-path", NULL);
		if (prop != NULL)
			return prop;
	}

	/* If the stdout-path property is not found, return the default */
	return FDT_DEFAULT_STDOUT_PATH;
}

int
fdtbus_get_stdout_phandle(void)
{
	const char *prop, *p;
	int off, len;

	prop = fdtbus_get_stdout_path();
	if (prop == NULL)
		return -1;

	p = strchr(prop, ':');
	len = p == NULL ? strlen(prop) : (p - prop);
	if (*prop != '/') {
		/* Alias */
		prop = fdt_get_alias_namelen(fdtbus_get_data(), prop, len);
		if (prop == NULL)
			return -1;
		len = strlen(prop);
	}
	off = fdt_path_offset_namelen(fdtbus_get_data(), prop, len);
	if (off < 0)
		return -1;

	return fdtbus_offset2phandle(off);
}

int
fdtbus_get_stdout_speed(void)
{
	const char *prop, *p;

	prop = fdtbus_get_stdout_path();
	if (prop == NULL)
		return -1;

	p = strchr(prop, ':');
	if (p == NULL)
		return -1;

	return (int)strtoul(p + 1, NULL, 10);
}

tcflag_t
fdtbus_get_stdout_flags(void)
{
	const char *prop, *p;
	tcflag_t flags = TTYDEF_CFLAG;
	char *ep;

	prop = fdtbus_get_stdout_path();
	if (prop == NULL)
		return flags;

	p = strchr(prop, ':');
	if (p == NULL)
		return flags;

	ep = NULL;
	(void)strtoul(p + 1, &ep, 10);
	if (ep == NULL)
		return flags;

	/* <baud>{<parity>{<bits>{<flow>}}} */
	while (*ep) {
		switch (*ep) {
		/* parity */
		case 'n':	flags &= ~(PARENB|PARODD); break;
		case 'e':	flags &= ~PARODD; flags |= PARENB; break;
		case 'o':	flags |= (PARENB|PARODD); break;
		/* bits */
		case '5':	flags &= ~CSIZE; flags |= CS5; break;
		case '6':	flags &= ~CSIZE; flags |= CS6; break;
		case '7':	flags &= ~CSIZE; flags |= CS7; break;
		case '8':	flags &= ~CSIZE; flags |= CS8; break;
		/* flow */
		case 'r':	flags |= CRTSCTS; break;
		}
		ep++;
	}

	return flags;
}
