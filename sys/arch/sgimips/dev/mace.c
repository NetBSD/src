/*	$NetBSD: mace.c,v 1.3.4.2 2002/04/01 07:42:20 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

/*
 * O2 MACE
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/dev/macereg.h>
#include <sgimips/dev/macevar.h>

#include "locators.h"

struct mace_softc {
	struct device sc_dev;
};

static int	mace_match(struct device *, struct cfdata *, void *);
static void	mace_attach(struct device *, struct device *, void *);
static int	mace_print(void *, const char *);
static int	mace_search(struct device *, struct cfdata *, void *);

struct cfattach mace_ca = {
	sizeof(struct mace_softc), mace_match, mace_attach
};

static int
mace_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	/*
	 * The MACE is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return (1);

	return (0);
}

static void
mace_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	printf("\n");

	/*
	 * Enable all "ISA" interrupts.
	 */
#if 0
	printf("mace0: isa sts %llx\n", *(volatile u_int64_t *)0xbf310010);
	printf("mace0: isa msk %llx\n", *(volatile u_int64_t *)0xbf310018);
	*(volatile u_int64_t *)0xbf310018 = 0xffffffff;
#endif
	printf("%s: isa sts %llx\n", self->dv_xname,
	    *(volatile u_int64_t *)0xbf310010);
	printf("%s: isa msk %llx\n", self->dv_xname,
	    *(volatile u_int64_t *)0xbf310018);

	config_search(mace_search, self, NULL);
}


static int
mace_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct mace_attach_args *maa = aux;

	if (pnp != 0)
		return QUIET;

	if (maa->maa_offset != MACECF_OFFSET_DEFAULT)
		printf(" offset 0x%lx", maa->maa_offset);
	if (maa->maa_intr != MACECF_INTR_DEFAULT)
		printf(" intr %d", maa->maa_intr);
#if 0
	if (maa->maa_offset != MACECF_STRIDE_DEFAULT)
		printf(" stride 0x%lx", maa->maa_stride);
#endif

	return UNCONF;
}

static int
mace_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mace_attach_args maa;
	int tryagain;

	do {
		maa.maa_offset = cf->cf_loc[MACECF_OFFSET];
		maa.maa_intr = cf->cf_loc[MACECF_INTR];
#if 0
		maa.maa_stride = cf->cf_loc[MACECF_STRIDE];
#endif
		maa.maa_st = 3;
								/* XXX */
		maa.maa_sh = MIPS_PHYS_TO_KSEG1(maa.maa_offset + 0x1f000000);

		tryagain = 0;
		if ((*cf->cf_attach->ca_match)(parent, cf, &maa) > 0) {
			config_attach(parent, cf, &maa, mace_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}

	} while (tryagain);

	return 0;
}

void *
mace_intr_establish(intr, level, func, arg)
	int intr;
	int level;
	int (*func)(void *);
	void *arg;
{
	/* XXX */

	return 0;
}
