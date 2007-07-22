/* $NetBSD: jensenio.c,v 1.14 2007/07/22 02:14:39 tsutsui Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Driver for the Jensen I/O bus.
 *
 * The Jensen I/O `bus' is comprised of two things:
 *
 *	- VLSI VL82C106 junk I/O chip
 *	- Intel EISA bus interface
 *
 * Access to the 82C106 is different than to the rest of EISA space, even
 * though it is a single address space.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: jensenio.c,v 1.14 2007/07/22 02:14:39 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <alpha/jensenio/jensenioreg.h>
#include <alpha/jensenio/jenseniovar.h>

#include "locators.h"

#include "eisa.h"

/*
 * The devices built-in to the VLSI VL82C106 junk I/O chip.
 */
const struct jensenio_dev {
	const char *jd_name;		/* device name */
	bus_addr_t jd_ioaddr;		/* I/O space address */
	int jd_irq[2];			/* Jensen IRQs */
} jensenio_devs[] = {
	{ "pckbc",	IO_KBD,		{ 0x980, 0x990 } },
	{ "com",	IO_COM1,	{ 0x900, -1 } },
	{ "com",	IO_COM2,	{ 0x920, -1 } },
	{ "lpt",	IO_LPT3,	{ 1, -1 } },
	{ "mcclock",	0x170,		{ -1, -1 } },
	{ NULL,		0,		{ -1, -1 } },
};

int	jensenio_match(struct device *, struct cfdata *, void *);
void	jensenio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(jensenio, sizeof(struct device),
    jensenio_match, jensenio_attach, NULL, NULL);

int	jensenio_print(void *, const char *);

int	jensenio_attached;

struct jensenio_config jensenio_configuration;

void	jensenio_eisa_attach_hook(struct device *, struct device *,
	    struct eisabus_attach_args *);
int	jensenio_eisa_maxslots(void *);

void	jensenio_isa_attach_hook(struct device *, struct device *,
	    struct isabus_attach_args *);

/*
 * Set up the Jensen's function pointers.
 */
void
jensenio_init(struct jensenio_config *jcp, int mallocsafe)
{

	/*
	 * Initialize the Host Address Extension register to 0
	 * (the firmware should have already done this).  We will
	 * disallow mapping of any device who's address would
	 * require a non-0 HAE.  This should be safe as the final
	 * draft of the Jensen system specification states that
	 * applications should follow this little rule.
	 *
	 * There's a good reason for this; actually using HAE would
	 * require a mutex around *every* EISA cycle!  Gross!
	 */
	REGVAL(JENSEN_HAE) = 0;
	alpha_mb();

	if (jcp->jc_initted == 0) {
		/* don't do these twice since they set up extents */
		jensenio_bus_io_init(&jcp->jc_eisa_iot, jcp);
		jensenio_bus_intio_init(&jcp->jc_internal_iot, jcp);
		jensenio_bus_mem_init(&jcp->jc_eisa_memt, jcp);
	}
	jcp->jc_mallocsafe = mallocsafe;
}

int
jensenio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, cf->cf_name) != 0)
		return (0);

	/* There can be only one. */
	if (jensenio_attached)
		return (0);

	return (1);
}

void
jensenio_attach(struct device *parent, struct device *self, void *aux)
{
	struct jensenio_attach_args ja;
	struct jensenio_config *jcp = &jensenio_configuration;
	int i;
	int locs[JENSENIOCF_NLOCS];

	printf("\n");

	jensenio_attached = 1;

	/*
	 * Done once at console init time, but we might need to do
	 * additional work this time.
	 */
	jensenio_init(jcp, 1);

	/*
	 * Initialize DMA.
	 */
	jensenio_dma_init(jcp);

	/*
	 * Initialize interrupts.
	 */
	jensenio_intr_init(jcp);

	/*
	 * First attach all of the built-in devices.
	 */
	for (i = 0; jensenio_devs[i].jd_name != NULL; i++) {
		ja.ja_name = jensenio_devs[i].jd_name;
		ja.ja_ioaddr = jensenio_devs[i].jd_ioaddr;
		ja.ja_irq[0] = jensenio_devs[i].jd_irq[0];
		ja.ja_irq[1] = jensenio_devs[i].jd_irq[1];

		ja.ja_iot = &jcp->jc_internal_iot;
		ja.ja_ec = &jcp->jc_ec;

		locs[JENSENIOCF_PORT] = jensenio_devs[i].jd_ioaddr;
		(void) config_found_sm_loc(self, "jensenio", locs, &ja,
		    jensenio_print, config_stdsubmatch);
	}

	/*
	 * Attach the EISA bus.
	 */
	jcp->jc_ec.ec_attach_hook = jensenio_eisa_attach_hook;
	jcp->jc_ec.ec_maxslots = jensenio_eisa_maxslots;

	ja.ja_eisa.eba_iot = &jcp->jc_eisa_iot;
	ja.ja_eisa.eba_memt = &jcp->jc_eisa_memt;
	ja.ja_eisa.eba_dmat = &jcp->jc_dmat_eisa;
	ja.ja_eisa.eba_ec = &jcp->jc_ec;
	(void) config_found_ia(self, "eisabus", &ja.ja_eisa, eisabusprint);

	/*
	 * Attach the ISA bus.
	 */
	jcp->jc_ic.ic_attach_hook = jensenio_isa_attach_hook;

	ja.ja_isa.iba_iot = &jcp->jc_eisa_iot;
	ja.ja_isa.iba_memt = &jcp->jc_eisa_memt;
	ja.ja_isa.iba_dmat = &jcp->jc_dmat_isa;
	ja.ja_isa.iba_ic = &jcp->jc_ic;
	(void) config_found_ia(self, "isabus", &ja.ja_isa, isabusprint);
}

int
jensenio_print(void *aux, const char *pnp)
{
	struct jensenio_attach_args *ja = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", ja->ja_name, pnp);

	aprint_normal(" port 0x%lx", ja->ja_ioaddr);

	return (UNCONF);
}

void
jensenio_eisa_attach_hook(struct device *parent, struct device *self,
    struct eisabus_attach_args *eba)
{

#if NEISA > 0
	/*
	 * Jensen's EISA config info is sparse, and is mapped at a
	 * different location that on other EISA systems.
	 */
	eisa_config_stride = 0x200;
	eisa_config_addr = JENSEN_FEPROM1;
	eisa_init(eba->eba_ec);
#endif
}

int
jensenio_eisa_maxslots(void *v)
{

	return (8);	/* jensen seems to have only 8 valid slots */
}

void
jensenio_isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}
