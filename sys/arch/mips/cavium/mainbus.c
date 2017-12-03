/*	$NetBSD: mainbus.c,v 1.1.18.2 2017/12/03 11:36:26 jdolecek Exp $	*/

/*
 * Copyright (c) 2007
 *      Internet Initiative Japan, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1.18.2 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cavium/include/mainbusvar.h>

static int	mainbus_match(device_t, struct cfdata *, void *);
static void	mainbus_attach(device_t, device_t, void *);
static int	mainbus_submatch(device_t, cfdata_t, const int *, void *);
static int	mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, sizeof(device_t), mainbus_match, mainbus_attach,
    NULL, NULL);

static int
mainbus_match(device_t parent, struct cfdata *match, void *aux)
{
	static int once = 0;

	if (once != 0)
		return 0;
	once = 1;

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	int i;
	struct mainbus_attach_args aa;

	aprint_normal("\n");

	for (i = 0; i < (int)mainbus_ndevs; i++) {
		aa.aa_name = mainbus_devs[i];
		(void)config_found_sm_loc(self, "mainbus", NULL, &aa,
		    mainbus_print, mainbus_submatch);
	}
}

static int
mainbus_submatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct mainbus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;

	return config_match(parent, cf, aux);
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *aa = aux;

	if (pnp != 0)
		return QUIET;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	return UNCONF;
}
