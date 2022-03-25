/* $NetBSD: fdt_userconf.c,v 1.1 2022/03/25 21:23:51 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_userconf.c,v 1.1 2022/03/25 21:23:51 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/userconf.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

#define	FDT_CHOSEN_PATH			"/chosen"
#define	FDT_CHOSEN_USERCONF_PROP	"netbsd,userconf"

void
userconf_bootinfo(void)
{
	const void *fdt = fdtbus_get_data();
	const char *cmd;
	int chosen, index;

	if (fdt == NULL) {
		return;
	}

	chosen = fdt_path_offset(fdt, FDT_CHOSEN_PATH);
	if (chosen < 0) {
		return;
	}

	index = 0;
	do {
		cmd = fdt_stringlist_get(fdt, chosen, FDT_CHOSEN_USERCONF_PROP,
		    index++, NULL);
		if (cmd != NULL) {
			aprint_debug("Processing userconf command: %s\n", cmd);
			userconf_parse(__UNCONST(cmd));
		}
	} while (cmd != NULL);
}
