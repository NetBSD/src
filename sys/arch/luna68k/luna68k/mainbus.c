/* $NetBSD: mainbus.c,v 1.2 2000/01/07 05:13:08 nisimura Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.2 2000/01/07 05:13:08 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

static struct mainbus_attach_args devs[] = {
	{ "clock",  0x45000000, -1 },	/* Mostek/Dallas TimeKeeper */
	{ "le",	    0xf1000000, 3 },	/* Am7990 */
	{ "sio",    0x51000000, 6 },	/* uPD7201A */
	{ "fb",	    0xc1100000, -1 },	/* BrookTree RAMDAC */
	{ "spc",    0xe1000000, 2 },	/* MB89352 */
#if 0
	{ "spc",    0xe1000040, 2 },	/* ditto */
#endif
};

static void mainbus_attach __P((struct device *, struct device *, void *));
static int  mainbus_match __P((struct device *, struct cfdata *, void *));
static int  mainbus_print __P((void *, const char *));

const struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

static int
mainbus_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	static int mainbus_matched;

	if (mainbus_matched)
		return (0);

	return ((mainbus_matched = 1));
}

static void
mainbus_attach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	int i;
	
	if (machtype == LUNA_II)
		devs[1].ma_addr = 0xf0000000;
	printf("\n");
	for (i = 0; i < sizeof(devs)/sizeof(devs[0]); i++)
		config_found(self, (void *)&devs[i], mainbus_print);
}

static int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct mainbus_attach_args *ma = aux;

	if (pnp)
		printf("%s at %s", ma->ma_name, pnp);

	return (UNCONF);
}
