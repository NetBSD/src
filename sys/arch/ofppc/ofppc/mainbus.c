/*	$NetBSD: mainbus.c,v 1.12 2003/07/15 02:46:33 lukem Exp $	 */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.12 2003/07/15 02:46:33 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <machine/platform.h>

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

extern struct cfdriver mainbus_cd;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ofbus_attach_args oba;
	char buf[32];
	const char * const *ssp, *sp = NULL;
	int node;

	static const char * const openfirmware_special[] = {
		/*
		 * These are _root_ devices to ignore.  Others must be
		 * handled elsewhere, if at all.
		 */
		"virtual-memory",
		"mmu",
		"aliases",
		"memory",
		"openprom",
		"options",
		"packages",
		"chosen",

		/*
		 * This one is extra-special .. we make a special case
		 * and attach CPUs early.
		 */
		"cpus",

		NULL
	};

	printf(": %s\n", platform_name);

	/*
	 * Before we do anything else, attach CPUs.  We do this early,
	 * because we might need to make CPU dependent decisions during
	 * the autoconfiguration process.  Also, it's a little weird to
	 * see CPUs after other devices in the boot messages.
	 */
	node = OF_finddevice("/cpus");
	if (node == -1) {

		/*
		 * No /cpus node; assume they're all children of the
		 * root OFW node.
		 */
		node = OF_peer(0);
	}
	for (node = OF_child(node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "device_type", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "cpu") != 0)
			continue;

		oba.oba_busname = "cpu";
		of_packagename(node, oba.oba_ofname, sizeof oba.oba_ofname);
		oba.oba_phandle = node;
		(void) config_found(self, &oba, mainbus_print);
	}

	/*
	 * Now attach the rest of the devices on the system.
	 */
	for (node = OF_child(OF_peer(0)); node != 0; node = OF_peer(node)) {

		/*
		 * Make sure it's not a CPU (we've already attached those).
		 */
		if (OF_getprop(node, "device_type", buf, sizeof(buf)) > 0 &&
		    strcmp(buf, "cpu") == 0)
			continue;

		/*
		 * Make sure this isn't one of our "special" child nodes.
		 */
		OF_getprop(node, "name", buf, sizeof(buf));
		for (ssp = openfirmware_special; (sp = *ssp) != NULL; ssp++) {
			if (strcmp(buf, sp) == 0)
				break;
		}
		if (sp != NULL)
			continue;

		oba.oba_busname = "ofw";
		of_packagename(node, oba.oba_ofname, sizeof oba.oba_ofname);
		oba.oba_phandle = node;
		(void) config_found(self, &oba, mainbus_print);
	}
}

int
mainbus_print(void *aux, const char *pnp)
{
	struct ofbus_attach_args *oba = aux;

	if (pnp)
		aprint_normal("%s at %s", oba->oba_ofname, pnp);
	else
		aprint_normal(" (%s)", oba->oba_ofname);
	return (UNCONF);
}
