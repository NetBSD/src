/*	$NetBSD: mainbus.c,v 1.1 1997/10/14 06:47:46 sakamoto Exp $	*/

/*
 * Copyright Notice:
 *
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

static const char copyright0[] __attribute__ ((unused)) =	/* XXX */
"Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.";

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <bebox/isa/isa_machdep.h>

#include "pci.h"

int	mainbus_match __P((struct device *, void *, void *));
void	mainbus_attach __P((struct device *, struct device *, void *));

struct cfattach mainbus_ca = {
	sizeof(struct device), (cfmatch_t)mainbus_match, mainbus_attach
};

struct cfdriver mainbus_cd = {
	NULL, "mainbus", DV_DULL
};

int	mainbus_print __P((void *, const char *));

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
	struct isabus_attach_args mba_iba;
};

/*
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA.
 */
int	isa_has_been_seen;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union mainbus_attach_args mba;

	printf("\n");

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	mba.mba_pba.pba_busname = "pci";
	mba.mba_pba.pba_iot = BEBOX_BUS_SPACE_IO;
	mba.mba_pba.pba_memt = BEBOX_BUS_SPACE_MEM;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_ENABLED |
	    PCI_FLAGS_MEM_ENABLED;
	config_found(self, &mba.mba_pba, mainbus_print);
#endif

	if (isa_has_been_seen == 0) {
		mba.mba_iba.iba_busname = "isa";
		mba.mba_iba.iba_iot = BEBOX_BUS_SPACE_IO;
		mba.mba_iba.iba_memt = BEBOX_BUS_SPACE_MEM;
		config_found(self, &mba.mba_iba, mainbus_print);
	}
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		printf("%s at %s", mba->mba_busname, pnp);
	if (!strcmp(mba->mba_busname, "pci"))
		printf(" bus %d", mba->mba_pba.pba_bus);
	return (UNCONF);
}
