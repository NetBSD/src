/* $NetBSD: tcbus.c,v 1.5 2000/01/09 03:56:11 simonb Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: tcbus.c,v 1.5 2000/01/09 03:56:11 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#define	_PMAX_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/tc/tcvar.h>
#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/turbochannel.h>

/*
 * Which system models were configured?
 */
#include "opt_dec_3max.h"
#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

/* XXX XXX TO BE REMOVED XXX XXX */
/*
 * DECstation tc implementations dont' have a tcasic to handle interrupts,
 * and the mapping to CPU interrupt lines is model-dependent.
 * We have to pass TC interrupt establish/disestablish requests up to
 * motherboard-specific code.
 */
static void	tc_ds_intr_establish __P((struct device *, void *,
				int, int (*)(void *), void *));
static void	tc_ds_intr_disestablish __P((struct device *, void *));
/* XXX XXX XXX XXX XXX XXX XXX */

static bus_dma_tag_t tc_ds_get_dma_tag __P((int));

extern struct tcbus_attach_args kn02_tc_desc[];	/* XXX */
extern struct tcbus_attach_args kmin_tc_desc[];	/* XXX */
extern struct tcbus_attach_args xine_tc_desc[];	/* XXX */
extern struct tcbus_attach_args kn03_tc_desc[];	/* XXX */

static int	tcbus_match __P((struct device *, struct cfdata *, void *));
static void	tcbus_attach __P((struct device *, struct device *, void *));

struct cfattach tcbus_ca = {
	sizeof(struct tc_softc), tcbus_match, tcbus_attach,
};

static int tcbus_found;

int
tcbus_match(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (tcbus_found || strcmp(ma->ma_name, "tcbus"))
		return 0;

	return 1;
}

void
tcbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tcbus_attach_args *tba;

	tcbus_found = 1;

	switch (systype) {
#ifdef DEC_3MAX
	case DS_3MAX:
		tba = &kn02_tc_desc[0]; break;
#endif
#ifdef DEC_3MIN
	case DS_3MIN:
		tba = &kmin_tc_desc[0]; break;
#endif
#ifdef DEC_MAXINE
	case DS_MAXINE:
		tba = &xine_tc_desc[0]; break;
#endif
#ifdef DEC_3MAXPLUS
	case DS_3MAXPLUS:
		tba = &kn03_tc_desc[0]; break;
#endif
	default:
		panic("tcbus_attach: no TURBOchannel configured for systype = %d", systype);
	}

	tba->tba_busname = "tc";
	tba->tba_memt = 0;
	tba->tba_intr_establish = tc_ds_intr_establish;
	tba->tba_intr_disestablish = tc_ds_intr_disestablish;
	tba->tba_get_dma_tag = tc_ds_get_dma_tag;

	tcattach(parent, self, tba);
}

/*
 * Return the DMA tag for use by the specified TURBOchannel slot.
 */
static bus_dma_tag_t
tc_ds_get_dma_tag(slot)
	int slot;
{
	/*
	 * All DECstations use the default DMA tag.
	 */
	return (&pmax_default_bus_dma_tag);
}

/*
 * Establish an interrupt handler.
 * For both TC and IOCTL asic, we must upcall to motherboard-specific
 * interrupt-hanlder functions,  in case we need to recompute masks for
 * CPU interrupt lines.
 */
static void
tc_ds_intr_establish(dev, cookie, level, handler, val)
	struct device *dev;
	void *cookie;
	int level;
        int (*handler) __P((void *));
	void *val;
{

#ifdef DIAGNOSTIC
	if (tc_enable_interrupt == NULL)
	    panic("tc_intr_establish: tc_enable not set\n");
#endif

#ifdef DEBUG
	printf("tc_intr_establish: slot %d level %d handler %p sc %p on\n",
		(int) cookie, (int) level, handler,  val);
#endif

	 /*
	  * Enable the interrupt from tc (or ioctl asic) slot with NetBSD/pmax
	  * sw-convention name ``cookie'' on this CPU.
	  * XXX store the level somewhere for selective enabling of
	  * interrupts from TC option slots.
	  */
	 (*tc_enable_interrupt)((unsigned)cookie, handler, val, 1);
}

static void
tc_ds_intr_disestablish(dev, arg)
	struct device *dev;
	void *arg;
{
	/*(*tc_enable_interrupt) (ca->ca_slot, handler, 0);*/
    	printf("cannot dis-establish IOASIC interrupts\n");
}

#include "rasterconsole.h"

#if NRASTERCONSOLE > 0

#include "mfb.h"
#include "cfb.h"
#include "sfb.h"
#include "px.h"

#include <machine/fbio.h>
#include <machine/fbvar.h>
#include <pmax/dev/cfbvar.h>
#include <pmax/dev/mfbvar.h>
#include <machine/pmioctl.h>	/* XXX shouldn't need this here for pxvar.h */
#include <pmax/dev/fbreg.h>	/* XXX shouldn't need this here for pxvar.h */
#include <pmax/dev/pxreg.h>	/* XXX shouldn't need this here for pxvar.h */
#include <pmax/dev/pxvar.h>
#include <pmax/dev/sfbvar.h>

#include <machine/dec_prom.h>

int
tcfb_cnattach(slotno)
	int slotno;
{
	tc_addr_t tcaddr;
	char tcname[TC_ROM_LLEN];
	struct fbinfo *fi;

	tcaddr = (*callv->_slot_address)(slotno);
	if (tc_badaddr(tcaddr) || tc_checkslot(tcaddr, tcname) == 0)
		panic("TC console designated by PROM does not exist!?");

#if NSFB > 0
	if (strncmp("PMAGB-BA", tcname, TC_ROM_LLEN) == 0) {
		fballoc((caddr_t)tcaddr, &fi);
		sfbinit(fi, (caddr_t)tcaddr, 0, 1);
		return 1;
	}
#endif
#if NCFB > 0
	if (strncmp("PMAG-BA ", tcname, TC_ROM_LLEN) == 0) {
		fballoc((caddr_t)tcaddr, &fi);
		cfbinit(fi, (caddr_t)tcaddr, 0, 1);
		return 1;
	}
#endif
#if NMFB > 0
	if (strncmp("PMAG-AA ", tcname, TC_ROM_LLEN) == 0) {
		fballoc((caddr_t)tcaddr, &fi);
		mfbinit(fi, (caddr_t)tcaddr, 0, 1);
		return 1;
	}
#endif
#if NPX > 0
	if (strncmp("PMAG-CA ", tcname, TC_ROM_LLEN) == 0
	    || strncmp("PMAG-DA ", tcname, TC_ROM_LLEN) == 0
	    || strncmp("PMAG-FA ", tcname, TC_ROM_LLEN) == 0) {
		fballoc((caddr_t)tcaddr, &fi);
		px_init(fi, (caddr_t)tcaddr, 0, 1);
		return 1;
	}
#endif
	return 0;
}

#endif
