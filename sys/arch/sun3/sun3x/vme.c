/*	$NetBSD: vme.c,v 1.11.6.1 2006/04/22 11:38:05 simonb Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: vme.c,v 1.11.6.1 2006/04/22 11:38:05 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

/* Does this machine have a VME bus? */
extern int cpu_has_vme;

/*
 * Convert vme unit number to bus type,
 * but only for supported bus types.
 * (See autoconf.h and vme.h)
 */
#define VME_UNITS	6
static const struct {
	int bustype;
	char name[8];
} vme_info[VME_UNITS] = {
	{ BUS_VME16D16, "A16/D16" },
	{ BUS_VME16D32, "A16/D32" },
	{ BUS_VME24D16, "A24/D16" },
	{ BUS_VME24D32, "A24/D32" },
	{ BUS_VME32D16, "A32/D16" },
	{ BUS_VME32D32, "A32/D32" },
};

static int  vme_match(struct device *, struct cfdata *, void *);
static void vme_attach(struct device *, struct device *, void *);

CFATTACH_DECL(vme, sizeof(struct device),
    vme_match, vme_attach, NULL, NULL);

static int 
vme_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	int unit;

	if (cpu_has_vme == 0)
		return (0);

	unit = cf->cf_unit;
	if (unit >= VME_UNITS)
		return (0);

	if (ca->ca_bustype != vme_info[unit].bustype)
		return (0);

	return (1);
}

static void 
vme_attach(struct device *parent, struct device *self, void *args)
{
	int unit;

	unit = device_unit(self);
	printf(": (%s)\n", vme_info[unit].name);

	/* We know ca_bustype == BUS_VMExx */
	config_search_ia(bus_scan, self, "vme", args);
}
