/*	$NetBSD: obio.c,v 1.8 2003/07/15 02:43:44 lukem Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.8 2003/07/15 02:43:44 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/mainboard.h>
#include <machine/bus.h>
#include <machine/sysconf.h>

static int	obio_match __P((struct device *, struct cfdata *, void *));
static void	obio_attach __P((struct device *, struct device *, void *));
static int	obio_search __P((struct device *, struct cfdata *, void *));
static int	obio_print __P((void *, const char *));
static void	obio_intr_establish  __P((bus_space_tag_t, int, int, int,
					  int (*)(void *), void *));

CFATTACH_DECL(obio, sizeof(struct device),
    obio_match, obio_attach, NULL, NULL);

extern struct cfdriver obio_cd;

struct mipsco_bus_space obio_bustag;
struct mipsco_bus_dma_tag obio_dmatag;
 
static int
obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, obio_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs *ca = aux;

	/* PIZAZZ (Mips 3000 Magnum)  Address Map */
	mipsco_bus_space_init(&obio_bustag, "obio", 
			      0x18000000, 0xb8000000,
			      0xb8000000, 0x08000000);

	_bus_dma_tag_init(&obio_dmatag);
	obio_bustag.bs_intr_establish = obio_intr_establish; /* XXX */

	ca->ca_bustag = &obio_bustag;
	ca->ca_dmatag = &obio_dmatag;

	printf("\n");
	config_search(obio_search, self, ca);
}

static int
obio_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	ca->ca_addr = cf->cf_addr;
	ca->ca_name = cf->cf_name;

	if (config_match(parent, cf, ca) != 0)
		config_attach(parent, cf, ca, obio_print);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
obio_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	if (name)
		return(QUIET);

	if (ca->ca_addr != -1)
		aprint_normal(" addr 0x%x", ca->ca_addr);

	return(UNCONF);
}

void
obio_intr_establish(bst, level, pri, flags, func, arg)
	bus_space_tag_t bst;
	int level;
	int pri;
	int flags;
	int (*func) __P((void *));
	void *arg;
{
	(*platform.intr_establish)(level, func, arg);
}
