/*	$NetBSD: mace.c,v 1.12 2003/07/15 03:35:52 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mace.c,v 1.12 2003/07/15 03:35:52 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/dev/macevar.h>
#include <sgimips/dev/macereg.h>
#include <sgimips/dev/crimereg.h>

#include "locators.h"

struct mace_softc {
	struct device sc_dev;
};

static int	mace_match(struct device *, struct cfdata *, void *);
static void	mace_attach(struct device *, struct device *, void *);
static int	mace_print(void *, const char *);
static int	mace_search(struct device *, struct cfdata *, void *);
void mace_intr(int irqs);

CFATTACH_DECL(mace, sizeof(struct mace_softc),
    mace_match, mace_attach, NULL, NULL);

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
	u_int32_t id;

        id = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_REV_INFO_R);
	aprint_normal(": rev %x", id);
	printf("\n");

	aprint_debug("%s: isa sts %llx\n", self->dv_xname,
	    *(volatile u_int64_t *)0xbf310010);
	aprint_debug("%s: isa msk %llx\n", self->dv_xname,
	    *(volatile u_int64_t *)0xbf310018);

	/* 
	 * Disable interrupts.  These are enabled and unmasked during 
	 * interrupt establishment 
	 */
        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_ADDR) = 0;
        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_FLAGS) = 0;
        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_CONTROL) = 0xff008500;
        *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(MACE_ISA_INT_MASK) = 0;

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
		aprint_normal(" offset 0x%lx", maa->maa_offset);
#if 0
	if (maa->maa_intr != MACECF_INTR_DEFAULT)
		aprint_normal(" intr %d", maa->maa_intr);
	if (maa->maa_offset != MACECF_STRIDE_DEFAULT)
		aprint_normal(" stride 0x%lx", maa->maa_stride);
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
#if 0
		maa.maa_intr = cf->cf_loc[MACECF_INTR];
		maa.maa_stride = cf->cf_loc[MACECF_STRIDE];
#endif
		maa.maa_st = 3;
								/* XXX */
		maa.maa_sh = MIPS_PHYS_TO_KSEG1(maa.maa_offset + 0x1f000000);

		tryagain = 0;
		if (config_match(parent, cf, &maa) > 0) {
			config_attach(parent, cf, &maa, mace_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}

	} while (tryagain);

	return 0;
}

#define MACE_NINTR 8	/* XXX */

struct {
	int	(*func)(void *);
	void	*arg;
} maceintrtab[MACE_NINTR];

void *
mace_intr_establish(intr, level, func, arg)
	int intr;
	int level;
	int (*func)(void *);
	void *arg;
{
        u_int64_t mask;

        if (intr < 0 || intr >= 8)
                panic("invalid interrupt number");

        if (maceintrtab[intr].func != NULL)
                return NULL;	/* panic("Cannot share MACE interrupts!"); */

        maceintrtab[intr].func = func;
        maceintrtab[intr].arg = arg;
        mask = *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_INTMASK);
        mask |= (1 << intr);
        *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_INTMASK) = mask;
	aprint_debug("mace: established interrupt %d (level %i)\n", intr, level);
	aprint_debug("mace: CRM_MASK now %llx\n", *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_INTMASK));
	return (void *)&maceintrtab[intr];
}

void 
mace_intr(int irqs)
{
	int i;
 
	/* printf("mace_intr: irqs %x\n", irqs); */
 
	for (i = 0; i < MACE_NINTR; i++) {
		if (irqs & (1 << i)) {
			if (maceintrtab[i].func != NULL)
		  		(maceintrtab[i].func)(maceintrtab[i].arg);
			else
				printf("Unexpected mace interrupt %d\n", i);
        	}
	}
}

