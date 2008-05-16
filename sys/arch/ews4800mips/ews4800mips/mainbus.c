/*	$NetBSD: mainbus.c,v 1.3.4.1 2008/05/16 02:22:19 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.3.4.1 2008/05/16 02:22:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/sbdvar.h>

static int mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);
static int mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_found;

int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{

	if (mainbus_found == 1)
		return 0;

	return 1;
}

void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;
	const char **p;

	mainbus_found = 1;
	aprint_normal("\n");

	/* CPU  */
	ma.ma_name = "cpu";
	config_found(self, &ma, mainbus_print);

	/* System Board dependent device busses */
	if (platform.mainbusdevs != NULL) {
		for (p = platform.mainbusdevs; *p != NULL; p++) {
			ma.ma_name = *p;
			config_found(self, &ma, mainbus_print);
		}
	}
}

int
mainbus_print(void *aux, const char *pnp)
{

	return pnp ? QUIET : UNCONF;
}
