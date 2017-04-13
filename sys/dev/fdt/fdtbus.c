/* $NetBSD: fdtbus.c,v 1.4 2017/04/13 22:12:53 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdtbus.c,v 1.4 2017/04/13 22:12:53 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdtvar.h>

#define	FDT_MAX_PATH	256

static int	fdt_match(device_t, cfdata_t, void *);
static void	fdt_attach(device_t, device_t, void *);
static void	fdt_scan(device_t, const struct fdt_attach_args *, const char *);
static bool	fdt_is_init(const struct fdt_attach_args *, const char *);

CFATTACH_DECL_NEW(fdt, 0,
    fdt_match, fdt_attach, NULL, NULL);

static int	fdt_print(void *, const char *);

static int
fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	if (!OF_child(faa->faa_phandle))
		return 0;

	return 1;
}

static void
fdt_attach(device_t parent, device_t self, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	char *model;
	int len, n;

	aprint_naive("\n");
	len = OF_getproplen(phandle, "model");
	if (len > 0) {
		model = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "model", model, len) == len) {
			aprint_normal(": %s\n", model);
		} else {
			aprint_normal("\n");
		}
		kmem_free(model, len);
	} else {
		aprint_normal("\n");
	}

	for (n = 0; n < faa->faa_ninit; n++) {
		fdt_scan(self, faa, faa->faa_init[n]);
	}
	fdt_scan(self, faa, NULL);
}

static void
fdt_scan(device_t self, const struct fdt_attach_args *faa, const char *devname)
{
	const int phandle = faa->faa_phandle;
	int len, child;
	char *name, *status;

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		struct fdt_attach_args nfaa = *faa;
		nfaa.faa_phandle = child;

		/* Only attach to nodes with a compatible property */
		len = OF_getproplen(child, "compatible");
		if (len <= 0)
			continue;

		/* If there is a "status" property, make sure it is "okay" */
		len = OF_getproplen(child, "status");
		if (len > 0) {
			status = kmem_zalloc(len, KM_SLEEP);
			int alen __diagused = OF_getprop(child, "status", status, len);
			KASSERT(alen == len);
			const bool okay_p = strcmp(status, "okay") == 0 ||
					    strcmp(status, "ok") == 0;
			kmem_free(status, len);
			if (!okay_p) {
				continue;
			}
		}

		/* Attach the device */
		len = OF_getproplen(child, "name");
		if (len <= 0) {
			continue;
		}
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(child, "name", name, len) == len) {
			nfaa.faa_name = name;
			if ((devname != NULL && strcmp(name, devname) == 0) ||
			    (devname == NULL && !fdt_is_init(faa, name))) {
				config_found(self, &nfaa, fdt_print);
			}
		}
		kmem_free(name, len);
	}
}

static bool
fdt_is_init(const struct fdt_attach_args *faa, const char *devname)
{
	u_int n;

	for (n = 0; n < faa->faa_ninit; n++) {
		if (strcmp(faa->faa_init[n], devname) == 0)
			return true;
	}

	return false;
}

static int
fdt_print(void *aux, const char *pnp)
{
	const struct fdt_attach_args * const faa = aux;
	char buf[FDT_MAX_PATH];
	const char *name = buf;

	if (pnp) {
		if (!fdtbus_get_path(faa->faa_phandle, buf, sizeof(buf)))
			name = faa->faa_name;
		aprint_normal("%s at %s", name, pnp);
	}

	return UNCONF;
}
