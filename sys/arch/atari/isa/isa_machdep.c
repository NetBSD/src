/*	$NetBSD: isa_machdep.c,v 1.27 2003/07/15 01:19:54 lukem Exp $	*/

/*
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.27 2003/07/15 01:19:54 lukem Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _ATARI_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/ic/pckbcvar.h>
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#endif /* NPCKBC > 0 */

#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/atari/device.h>

#include "isadma.h"

#if NISADMA == 0

/*
 * Entry points for ISA DMA while no DMA capable devices are attached.
 * This must be a serious error. Cause a panic...
 */
struct atari_bus_dma_tag isa_bus_dma_tag = {
	0
};
#endif /* NISADMA == 0 */

static int	isabusprint __P((void *auxp, const char *));
static int	isabusmatch __P((struct device *, struct cfdata *, void *));
static void	isabusattach __P((struct device *, struct device *, void *));

struct isabus_softc {
	struct device sc_dev;
	struct atari_isa_chipset sc_chipset;
};

CFATTACH_DECL(isab, sizeof(struct isabus_softc),
    isabusmatch, isabusattach, NULL, NULL);

/*
 * We need some static storage to attach a console keyboard on the Milan
 * during early console init.
 */
static struct atari_bus_space	bs_storage[2];	/* 1 iot, 1 memt */

int
isabusmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	static int	nmatched = 0;

	if (strcmp((char *)auxp, "isab"))
		return (0); /* Wrong number... */

	if(atari_realconfig == 0)
		return (1);

	if (machineid & (ATARI_HADES|ATARI_MILAN)) {
		/*
		 * The Hades and Milan have only one pci bus
		 */
		if (nmatched)
			return (0);
		nmatched++;
		return (1);
	}
	return(0);
}

void
isabusattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct isabus_softc *sc = (struct isabus_softc *)dp;
	struct isabus_attach_args	iba;
	extern struct atari_bus_dma_tag isa_bus_dma_tag;
	extern void isa_bus_init(void);

	iba.iba_busname = "isa";
	iba.iba_dmat	= &isa_bus_dma_tag;
	iba.iba_iot     = leb_alloc_bus_space_tag(&bs_storage[0]);
	iba.iba_memt    = leb_alloc_bus_space_tag(&bs_storage[1]);
	iba.iba_ic	= &sc->sc_chipset;
	if ((iba.iba_iot == NULL) || (iba.iba_memt == NULL)) {
		printf("leb_alloc_bus_space_tag failed!\n");
		return;
	}
	iba.iba_iot->base  = ISA_IOSTART;
	iba.iba_memt->base = ISA_MEMSTART;

	if (machineid & ATARI_HADES)
	    MFP->mf_aer |= (IO_ISA1|IO_ISA2); /* ISA interrupts: LOW->HIGH */
	isa_bus_init();
	if (dp == NULL) { /* Early init */
#if (NPCKBC > 0)
		pckbc_cnattach(iba.iba_iot, IO_KBD, KBCMDP, PCKBC_KBD_SLOT);
#endif
		return;
	}

	printf("\n");
	config_found(dp, &iba, isabusprint);
}

int
isabusprint(auxp, name)
void		*auxp;
const char	*name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}

void
isa_attach_hook(parent, self, iba)
	struct device		  *parent, *self;
	struct isabus_attach_args *iba;
{
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}
