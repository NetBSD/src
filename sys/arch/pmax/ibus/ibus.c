/*	$NetBSD: ibus.c,v 1.7 2000/02/29 04:41:48 nisimura Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ibus.c,v 1.7 2000/02/29 04:41:48 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <pmax/ibus/ibusvar.h>

#include "locators.h"

static int	ibussubmatch __P((struct device *, struct cfdata *, void *));

void
ibusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ibus_dev_attach_args *ida = aux;
	struct ibus_attach_args *ia;
	int i;

	printf("\n");

	/*
	 * Loop through the devices and attach them.  If a probe-size
	 * is specified, it's an optional item on the platform and
	 * do a badaddr() test to make sure it's there.
	 */
	for (i = 0; i < ida->ida_ndevs; i++) {
		ia = &ida->ida_devs[i];
		if (ia->ia_basz != 0 &&
		    badaddr((caddr_t)ia->ia_addr, ia->ia_basz) != 0)
			continue;
		config_found_sm(self, ia, ibusprint, ibussubmatch);
	}
}

static int
ibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ibus_attach_args *ia = aux;
	paddr_t pa;

	pa = MIPS_KSEG1_TO_PHYS(ia->ia_addr);

	if (cf->cf_loc[IBUSCF_ADDR] != IBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[IBUSCF_ADDR] != pa)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
ibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct ibus_attach_args *ia = aux;

	if (pnp)
		printf("%s at %s", ia->ia_name, pnp);

	printf(" addr 0x%x", MIPS_KSEG1_TO_PHYS(ia->ia_addr));

	return (UNCONF);
}

void
ibus_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	(*platform.intr_establish)(dev, cookie, level, handler, arg);
}
