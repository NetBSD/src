/*	$NetBSD: imc.c,v 1.9 2003/07/15 03:35:52 lukem Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imc.c,v 1.9 2003/07/15 03:35:52 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/dev/imcreg.h>

#include "locators.h"

struct imc_softc {
	struct device sc_dev;

	int eisa_present : 1;
};

static int	imc_match(struct device *, struct cfdata *, void *);
static void	imc_attach(struct device *, struct device *, void *);
static int	imc_print(void *, const char *);

CFATTACH_DECL(imc, sizeof(struct imc_softc),
    imc_match, imc_attach, NULL, NULL);

struct imc_attach_args {
	const char* iaa_name;

	bus_space_tag_t iaa_st;
	bus_space_handle_t iaa_sh;

/* ? */
	long	iaa_offset;
	int	iaa_intr;
#if 0
	int	iaa_stride;
#endif
};

static int
imc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	/*
	 * The IMC is an INDY/INDIGO2 thing.
	 */
	if (mach_type != MACH_SGI_IP22)
		return (0);

	/* Make sure it's actually there and readable */
	if (badaddr((void*)MIPS_PHYS_TO_KSEG1(IMC_SYSID), sizeof(u_int32_t)))
		return (0);

	return (1);
}

static void
imc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	u_int32_t reg;
	struct imc_attach_args iaa;
	struct imc_softc *isc = (void *) self;
	u_int32_t sysid = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_SYSID);

	/* EISA present bit is on even on Indys, so don't trust it! */
	if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
		isc->eisa_present = (sysid & IMC_SYSID_HAVEISA);
	else
		isc->eisa_present = 0;

	printf("\nimc0: Revision %d", (sysid & IMC_SYSID_REVMASK));

	if (isc->eisa_present)
		printf(", EISA bus present");

	printf("\n");

	/* Clear CPU/GIO error status registers to clear any leftover bits. */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_CPU_ERRSTAT) = 0;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_GIO_ERRSTAT) = 0;

	/*
	 * Enable parity reporting on GIO/main memory transactions.
	 * Disable parity checking on CPU bus transactions (as turning
	 * it on seems to cause spurious bus errors), but enable parity
	 * checking on CPU reads from main memory (note that this bit
	 * has the opposite sense... Turning it on turns the checks off!).
	 * Finally, turn on interrupt writes to the CPU from the MC.
	 */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_CPUCTRL0);
	reg &= ~IMC_CPUCTRL0_NCHKMEMPAR;
	reg |= (IMC_CPUCTRL0_GPR | IMC_CPUCTRL0_MPR | IMC_CPUCTRL0_INTENA);
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_CPUCTRL0) = reg;

	/* Setup the MC write buffer depth */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_CPUCTRL1);
	reg = (reg & ~IMC_CPUCTRL1_MCHWMSK) | 13;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_CPUCTRL1) = reg;

	/*
	 * Set GIO64 arbitrator configuration register:
	 *
	 * Preserve PROM-set graphics-related bits, as they seem to depend
	 * on the graphics variant present and I'm not sure how to figure
	 * that out or 100% sure what the correct settings are for each.
	 */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_GIO64ARB);
	reg &= (IMC_GIO64ARB_GRX64 | IMC_GIO64ARB_GRXRT | IMC_GIO64ARB_GRXMST);

	/* GIO64 invariant for all IP22 platforms: one GIO bus, HPC1 @ 64 */
	reg |= IMC_GIO64ARB_ONEGIO | IMC_GIO64ARB_HPC64;

	/* Rest of settings are machine/board dependant */
	switch (mach_subtype) {
	case MACH_SGI_IP22_GUINESS:
		/* EISA can bus-master, is 64-bit */
		reg |= (IMC_GIO64ARB_EISAMST | IMC_GIO64ARB_EISA64);
		break;

	case MACH_SGI_IP22_FULLHOUSE:
		/*
		 * All Fullhouse boards have a 64-bit HPC2 and pipelined
		 * EXP0 slot.
		 */
		reg |= (IMC_GIO64ARB_HPCEXP64 | IMC_GIO64ARB_EXP0PIPE);

		if (mach_boardrev < 2) {
			/* EXP0 realtime, EXP1 can master */
			reg |= (IMC_GIO64ARB_EXP0RT | IMC_GIO64ARB_EXP1MST);
		} else {
			/* EXP1 pipelined as well, EISA masters */
			reg |= (IMC_GIO64ARB_EXP1PIPE | IMC_GIO64ARB_EISAMST);
		}
		break;
	}

	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(IMC_GIO64ARB) = reg;

	if (isc->eisa_present) {
#if notyet
		memset(&iaa, 0, sizeof(iaa));

		iaa.iaa_name = "eisa";
		(void)config_found(self, (void*)&iaa, imc_print);
#endif
	}

	memset(&iaa, 0, sizeof(iaa));

	iaa.iaa_name = "gio";
	(void)config_found(self, (void*)&iaa, imc_print);
}


static int
imc_print(aux, name)
	void *aux;
	const char *name;
{
	struct imc_attach_args* iaa = aux;

	if (name)
		aprint_normal("%s at %s", iaa->iaa_name, name);

	return UNCONF;
}

