/*	$NetBSD: wdc_ofisa.c,v 1.13 2003/09/19 21:36:05 mycroft Exp $	*/

/*
 * Copyright 1997, 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * OFW Attachment for 'wdc' disk controller driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_ofisa.c,v 1.13 2003/09/19 21:36:05 mycroft Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/wdcreg.h>		/* ??? */
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_ofisa_softc {
	struct wdc_softc sc_wdcdev;
	struct channel_softc *wdc_chanlist[1];	
	struct channel_softc wdc_channel;
	struct channel_queue wdc_chqueue;
	void	*sc_ih;
};

int wdc_ofisa_probe __P((struct device *, struct cfdata *, void *));
void wdc_ofisa_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(wdc_ofisa, sizeof(struct wdc_ofisa_softc),
    wdc_ofisa_probe, wdc_ofisa_attach, NULL, NULL);

int
wdc_ofisa_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofisa_attach_args *aa = aux;
	static const char *const compatible_strings[] = { "pnpPNP,600", NULL };
	int rv = 0;

	if (of_compatible(aa->oba.oba_phandle, compatible_strings) != -1)
		rv = 5;
#ifdef _WDC_OFISA_MD_MATCH
	if (!rv)
		rv = wdc_ofisa_md_match(parent, cf, aux);
#endif
	return (rv);
}

void
wdc_ofisa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_ofisa_softc *sc = (void *)self;
	struct ofisa_attach_args *aa = aux;
	struct ofisa_reg_desc reg[2];
	struct ofisa_intr_desc intr;
	int n;

	/*
	 * We're living on an ofw.  We have to ask the OFW what our
	 * registers and interrupts properties look like.
	 *
	 * We expect exactly two register regions and one interrupt.
	 */

	n = ofisa_reg_get(aa->oba.oba_phandle, reg, 2);
#ifdef _WDC_OFISA_MD_REG_FIXUP
	n = wdc_ofisa_md_reg_fixup(parent, self, aux, reg, 2, n);
#endif
	if (n != 2) {
		printf(": error getting register data\n");
		return;
	}
	if (reg[0].len != 8 || reg[1].len != 2) {
		printf(": weird register size (%lu/%lu, expected 8/2)\n",
		    (unsigned long)reg[0].len, (unsigned long)reg[1].len);
		return;
	}

	n = ofisa_intr_get(aa->oba.oba_phandle, &intr, 1);
#ifdef _WDC_OFISA_MD_INTR_FIXUP
	n = wdc_ofisa_md_intr_fixup(parent, self, aux, &intr, 1, n);
#endif
	if (n != 1) {
		printf(": error getting interrupt data\n");
		return;
	}

	sc->wdc_channel.cmd_iot =
	    (reg[0].type == OFISA_REG_TYPE_IO) ? aa->iot : aa->memt;
	sc->wdc_channel.ctl_iot =
	    (reg[1].type == OFISA_REG_TYPE_IO) ? aa->iot : aa->memt;
        if (bus_space_map(sc->wdc_channel.cmd_iot, reg[0].addr, 8, 0,
              &sc->wdc_channel.cmd_ioh) ||
            bus_space_map(sc->wdc_channel.ctl_iot, reg[1].addr, 1, 0,
	      &sc->wdc_channel.ctl_ioh)) {
                printf(": can't map register spaces\n");
		return;
        }

	sc->sc_ih = isa_intr_establish(aa->ic, intr.irq, intr.share,
	    IPL_BIO, wdcintr, &sc->wdc_channel);

	printf("\n");
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	sc->wdc_chanlist[0] = &sc->wdc_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanlist;
	sc->sc_wdcdev.nchannels = 1;
	sc->wdc_channel.channel = 0;
	sc->wdc_channel.wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue = &sc->wdc_chqueue;

	config_interrupts(self, wdcattach);

#if 0
	printf("%s: registers: ", sc->sc_dev.dv_xname);
	ofisa_reg_print(reg, 2);
	printf("\n");
	printf("%s: interrupts: ", sc->sc_dev.dv_xname);
	ofisa_intr_print(&intr, 1);
	printf("\n");
#endif
}
