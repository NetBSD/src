/* $NetBSD: dec_axppci_33.c,v 1.71 2025/03/09 01:06:41 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996, 1997 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_axppci_33.c,v 1.71 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/lcareg.h>
#include <alpha/pci/lcavar.h>

void dec_axppci_33_init(void);
static void dec_axppci_33_cons_init(void);
static void dec_axppci_33_device_register(device_t, void *);

const struct alpha_variation_table dec_axppci_33_variations[] = {
	{ 0, "Alpha PC AXPpci33 (\"NoName\")" },
	{ 0, NULL },
};

static struct lca_config *lca_preinit(void);

static struct lca_config *
lca_preinit(void)
{
	extern struct lca_config lca_configuration;

	lca_init(&lca_configuration);
	return &lca_configuration;
}

#define	NSIO_PORT  0x26e	/* Hardware enabled option: 0x398 */
#define	NSIO_BASE  0
#define	NSIO_INDEX NSIO_BASE
#define	NSIO_DATA  1
#define	NSIO_SIZE  2
#define	NSIO_CFG0  0
#define	NSIO_CFG1  1
#define	NSIO_CFG2  2
#define	NSIO_IDE_ENABLE 0x40

void
dec_axppci_33_init(void)
{
	int cfg0val;
	uint64_t variation;
	bus_space_tag_t iot;
	struct lca_config *lcp;
	bus_space_handle_t nsio;
#define	A33_NSIOBARRIER(type) bus_space_barrier(iot, nsio,\
				NSIO_BASE, NSIO_SIZE, (type))

	platform.family = "DEC AXPpci";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_axppci_33_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "lca";
	platform.cons_init = dec_axppci_33_cons_init;
	platform.device_register = dec_axppci_33_device_register;

	lcp = lca_preinit();
	iot = &lcp->lc_iot;
	if (bus_space_map(iot, NSIO_PORT, NSIO_SIZE, 0, &nsio))
		return;

	bus_space_write_1(iot, nsio, NSIO_INDEX, NSIO_CFG0);
	A33_NSIOBARRIER(BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
	cfg0val = bus_space_read_1(iot, nsio, NSIO_DATA);

	cfg0val |= NSIO_IDE_ENABLE;

	bus_space_write_1(iot, nsio, NSIO_INDEX, NSIO_CFG0);
	A33_NSIOBARRIER(BUS_SPACE_BARRIER_WRITE);
	bus_space_write_1(iot, nsio, NSIO_DATA, cfg0val);
	A33_NSIOBARRIER(BUS_SPACE_BARRIER_WRITE);
	bus_space_write_1(iot, nsio, NSIO_DATA, cfg0val);

	/* Leave nsio mapped to catch any accidental port space collisions  */

	lca_probe_bcache();
}

static void
dec_axppci_33_cons_init(void)
{
	struct lca_config *lcp;

	lcp = lca_preinit();

	pci_consinit(&lcp->lc_pc, &lcp->lc_iot, &lcp->lc_memt,
	    &lcp->lc_iot, &lcp->lc_memt);
}

static void
dec_axppci_33_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}
