/*	$NetBSD: hpc.c,v 1.13 2003/07/15 03:35:53 lukem Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001 Rafal K. Boni
 * Copyright (c) 2001 Jason R. Thorpe
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpc.c,v 1.13 2003/07/15 03:35:53 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/machtype.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/iocreg.h>

#include "locators.h"

const struct hpc_device {
	const char *hd_name;
	bus_addr_t hd_devoff;
	bus_addr_t hd_dmaoff;
	int hd_irq;
	int hd_sysmask;
#define	HPCDEV_IP22		(1U << 0)	/* Indigo2 */
#define	HPCDEV_IP24		(1U << 1)	/* Indy */
} hpc_devices[] = {
	{ "zsc",
	  /* XXX Magic numbers */
	  HPC_PBUS_CH6_DEVREGS + 0x30,	0,
	  29,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "sq",
	  HPC_ENET_DEVREGS, HPC_ENET_REGS,
	  3,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "wdsc",
	  HPC_SCSI0_DEVREGS, HPC_SCSI0_REGS,
	  1,	/* XXX 1 = IRQ_LOCAL0 + 1 */
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "wdsc",
	  HPC_SCSI1_DEVREGS, HPC_SCSI1_REGS,
	  2,	/* XXX 2 = IRQ_LOCAL0 + 2 */
	  HPCDEV_IP22 },

	{ "dsclock",
	  HPC_PBUS_BBRAM, 0,
	  -1,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ NULL,
	  0, 0,
	  0,
	  0
	}
};

struct hpc_softc {
	struct device 		sc_dev;

	bus_addr_t		sc_base;

	bus_space_tag_t		sc_ct;
	bus_space_handle_t	sc_ch;
};

extern int mach_type;		/* IPxx type */
extern int mach_subtype;	/* subtype: eg., Guiness/Fullhouse for IP22 */
extern int mach_boardrev;	/* machine board revision, in case it matters */

extern struct sgimips_bus_dma_tag sgimips_default_bus_dma_tag;

static int powerintr_established;

int	hpc_match(struct device *, struct cfdata *, void *);
void	hpc_attach(struct device *, struct device *, void *);
int	hpc_print(void *, const char *);

int	hpc_submatch(struct device *, struct cfdata *, void *);

int	hpc_power_intr(void *);

CFATTACH_DECL(hpc, sizeof(struct hpc_softc),
    hpc_match, hpc_attach, NULL, NULL);

int
hpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct gio_attach_args* ga = aux;

	/* Make sure it's actually there and readable */
	if (badaddr((void*)MIPS_PHYS_TO_KSEG1(ga->ga_addr), sizeof(u_int32_t)))
		return 0;

	return 1;
}

void
hpc_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpc_softc *sc = (struct hpc_softc *)self;
	struct gio_attach_args* ga = aux;
	struct hpc_attach_args ha;
	const struct hpc_device *hd;
	int sysmask, hpctype;

	switch (mach_type) {
	case MACH_SGI_IP22:
		hpctype = 3;
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
			sysmask = HPCDEV_IP22;
		else
			sysmask = HPCDEV_IP24;
		break;

	default:
		panic("hpc_attach: can't handle HPC on an IP%d",
		    mach_type);
	};

	printf(": SGI HPC%d\n", hpctype);

	sc->sc_ct = 1;
	sc->sc_ch = ga->ga_ioh;

	sc->sc_base = ga->ga_addr;

	for (hd = hpc_devices; hd->hd_name != NULL; hd++) {
		if (!(hd->hd_sysmask & sysmask))
			continue;

		ha.ha_name = hd->hd_name;
		ha.ha_devoff = hd->hd_devoff;
		ha.ha_dmaoff = hd->hd_dmaoff;
		ha.ha_irq = hd->hd_irq;

		/* XXX This is disgusting. */
		ha.ha_st = 1;
		ha.ha_sh = MIPS_PHYS_TO_KSEG1(sc->sc_base);
		ha.ha_dmat = &sgimips_default_bus_dma_tag;

		(void) config_found_sm(self, &ha, hpc_print, hpc_submatch);
	}

	/*
	 * XXX: Only attach the powerfail interrupt once, since the
	 * interrupt code doesn't let you share interrupt just yet.
	 *
	 * Since the powerfail interrupt is hardcoded to read from
	 * a specific register anyway (XXX#2!), we don't care when
	 * it gets attached, as long as it only happens once.
	 */
	if (!powerintr_established) {
		cpu_intr_establish(9, IPL_NONE, hpc_power_intr, sc);
		powerintr_established++;
	}
}

int
hpc_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (cf->cf_loc[HPCCF_OFFSET] != HPCCF_OFFSET_DEFAULT &&
	    (bus_addr_t) cf->cf_loc[HPCCF_OFFSET] != ha->ha_devoff)
		return (0);

	return (config_match(parent, cf, aux));
}

int
hpc_print(void *aux, const char *pnp)
{
	struct hpc_attach_args *ha = aux;

	if (pnp)
		printf("%s at %s", ha->ha_name, pnp);

	printf(" offset 0x%lx", ha->ha_devoff);

	return (UNCONF);
}

int
hpc_power_intr(void *arg)
{
	u_int32_t pwr_reg;

	pwr_reg = *((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850));
	*((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850)) = pwr_reg;

	printf("hpc_power_intr: panel reg = %08x\n", pwr_reg);

	if (pwr_reg & 2)
		cpu_reboot(RB_HALT, NULL);

	return 1;
}
