/*	$NetBSD: imc.c,v 1.1 2001/05/11 04:22:55 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include "locators.h"

struct imc_softc {
	struct device sc_dev;

	int eisa_present : 1;
};

static int	imc_match(struct device *, struct cfdata *, void *);
static void	imc_attach(struct device *, struct device *, void *);
static int	imc_print(void *, const char *);

struct cfattach imc_ca = {
	sizeof(struct imc_softc), imc_match, imc_attach
};

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
	struct mainbus_attach_args *ma = aux;

	/*
	 * The IMC is an INDY/INDIGO2 thing.
	 */
	switch (ma->ma_arch) {
	case 22:
		/* Make sure it's actually there and readable */
		if (badaddr((void*)MIPS_PHYS_TO_KSEG1(0x1fa0001c), 
						sizeof(u_int32_t)))
			return 0;

		return 1;
	default:
		return 0;
	}
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
        u_int32_t sysid = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa0001c);

	isc->eisa_present = (sysid >> 4) & 1;

	printf("\nimc0: Revision %d", (sysid & 0x03));

	if (isc->eisa_present)
	    printf(", EISA bus present");

	printf("\n");

	/* Clear CPU/GIO error status registers to clear any leftover bits. */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa000ec) = 0;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa000fc) = 0;

	/* 
	 * Enable parity reporting on memory/GIO accesses, turn off parity 
	 * checking on CPU reads from memory (flags cribbed from Linux -- 
	 * why is the last one off?).  Also, enable MC interrupt writes to
	 * the CPU.
	 */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa00004);
	reg |= 0x4002060;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa00004) = reg;
	
	/* Setup the MC write buffer depth */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa0000c);
	reg &= ~0xf;
	reg |= 0xd;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa0000c) = reg;

	/* Set GIO64 arbitrator configuration register. */

	/* GIO64 invariant for all IP22 platforms: one GIO bus, HPC1 @ 64 */
	reg = 0x401;

	/* XXXrkb: I2 setting for now */
	reg |= 0xc222;	/* GFX, HPC2 @ 64, EXP1,2 pipelined, EISA masters */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa00084) = reg;

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
		printf("%s at %s", iaa->iaa_name, name);

	return UNCONF;
}

