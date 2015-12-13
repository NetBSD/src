/* $NetBSD: simplebus.c,v 1.1 2015/12/13 17:30:40 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: simplebus.c,v 1.1 2015/12/13 17:30:40 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

static int	simplebus_match(device_t, cfdata_t, void *);
static void	simplebus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(simplebus, 0,
    simplebus_match, simplebus_attach, NULL, NULL);

static int
simplebus_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "simple-bus", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
simplebus_attach(device_t parent, device_t self, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	char *name;
	int len;

	aprint_naive("\n");

	len = OF_getproplen(phandle, "name");
	if (len > 0) {
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "name", name, len) == len) {
			aprint_normal(": %s\n", name);
		} else {
			aprint_normal("\n");
		}
		kmem_free(name, len);
	} else {
		aprint_normal("\n");
	}

	struct fdt_attach_args nfaa = *faa;
	nfaa.faa_name = "simple-bus";
	nfaa.faa_init = NULL;
	nfaa.faa_ninit = 0;
	nfaa.faa_phandle = phandle;

	config_found(self, &nfaa, NULL);
}
