/*	$NetBSD: pcib.c,v 1.1 2001/05/28 16:22:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.1 2001/05/28 16:22:21 thorpej Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032var.h>
#endif

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064var.h>
#endif 
 
#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032var.h>
#endif


struct pcib_softc {
	struct device	sc_dev;
};

int	pcib_match(struct device *, struct cfdata *, void *);
void	pcib_attach(struct device *, struct device *, void *);

struct cfattach pcib_ca = {
	sizeof(struct pcib_softc), pcib_match, pcib_attach,
};

int	pcib_print(void *, const char *pnp);
void	pcib_isa_attach_hook(struct device *, struct device *,
	    struct isabus_attach_args *);

void	pcib_bridge_callback(struct device *);

int
pcib_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA)
		return (1);

	return (0);
}

void
pcib_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	config_defer(self, pcib_bridge_callback);
}

void
pcib_bridge_callback(self)
	struct device *self;
{
	struct pcib_softc *sc = (struct pcib_softc *)self;
	struct isabus_attach_args iba;

	memset(&iba, 0, sizeof(iba));

	iba.iba_busname = "isa";
#if defined(ALGOR_P4032)
	    {
		/* XXX */
	    }
#elif defined(ALGOR_P5064)
	    {
		struct p5064_config *acp = &p5064_configuration;

		iba.iba_iot = &acp->ac_iot;
		iba.iba_memt = &acp->ac_memt;
		iba.iba_dmat = &acp->ac_isa_dmat;
		iba.iba_ic = &acp->ac_ic;
	    }
#elif defined(ALGOR_P6032)
	    {
		/* XXX */
	    }
#endif

	iba.iba_ic->ic_attach_hook = pcib_isa_attach_hook;

	(void) config_found(&sc->sc_dev, &iba, pcib_print);
}

int
pcib_print(void *aux, const char *pnp)
{
	struct isabus_attach_args *iba;

	if (pnp)
		printf("%s at %s", iba->iba_busname, pnp);
	return (UNCONF);
}

void
pcib_isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}
