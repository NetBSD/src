/*	$NetBSD: wdc_pcmcia.c,v 1.11 1998/10/10 22:01:24 thorpej Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Onno van der Linden and by Manuel Bouyer.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/isa/isavar.h>
#include <dev/ic/wdcvar.h>

#define WDC_PCMCIA_REG_NPORTS      8
#define WDC_PCMCIA_AUXREG_OFFSET   (WDC_PCMCIA_REG_NPORTS + 6)
#define WDC_PCMCIA_AUXREG_NPORTS   2

struct wdc_pcmcia_softc {
	struct wdc_softc sc_wdcdev;
	struct wdc_attachment_data sc_ad;
	struct pcmcia_io_handle sc_pioh;
	struct pcmcia_io_handle sc_auxpioh;
	int sc_iowindow;
	int sc_auxiowindow;
	void *sc_ih;
};

static int wdc_pcmcia_match	__P((struct device *, struct cfdata *, void *));
static void wdc_pcmcia_attach	__P((struct device *, struct device *, void *));

struct cfattach wdc_pcmcia_ca = {
	sizeof(struct wdc_pcmcia_softc), wdc_pcmcia_match, wdc_pcmcia_attach
};

struct wdc_pcmcia_product {
	u_int32_t	wpp_vendor;	/* vendor ID */
	u_int32_t	wpp_product;	/* product ID */
	int		wpp_quirk_flag;	/* Quirk flags */
#define WDC_PCMCIA_FORCE_16BIT_IO	0x01 /* Don't use PCMCIA_WIDTH_AUTO */
#define WDC_PCMCIA_NO_EXTRA_RESETS	0x02 /* Only reset ctrl once */
	const char	*wpp_cis_info[4];	/* XXX necessary? */
	const char	*wpp_name;	/* product name */
} wdc_pcmcia_products[] = {

	{ /* PCMCIA_VENDOR_DIGITAL XXX */ 0x0100,
	  PCMCIA_PRODUCT_DIGITAL_MOBILE_MEDIA_CDROM,
	  0, { NULL, "Digital Mobile Media CD-ROM", NULL, NULL },
	  PCMCIA_STR_DIGITAL_MOBILE_MEDIA_CDROM },

	{ PCMCIA_VENDOR_HAGIWARASYSCOM,
	  -1,			/* XXX */
	  WDC_PCMCIA_FORCE_16BIT_IO,
	  { NULL, NULL, NULL, NULL },
	 "Hagiwara SYS-COM ComactFlash Card" },

	/* CANON FC-8M is also matches.  */
	{ PCMCIA_VENDOR_SANDISK,
	  PCMCIA_PRODUCT_SANDISK_SDCFB,
	  WDC_PCMCIA_FORCE_16BIT_IO /* _AUTO also works */,
	  { NULL, NULL, NULL, NULL },
	  PCMCIA_STR_SANDISK_SDCFB },

	/* The TEAC IDE/Card II is used on the Sony Vaio */
	{ PCMCIA_VENDOR_TEAC,
	  PCMCIA_PRODUCT_TEAC_IDECARDII,
	  WDC_PCMCIA_NO_EXTRA_RESETS,
	  PCMCIA_CIS_TEAC_IDECARDII,
	  PCMCIA_STR_TEAC_IDECARDII },

	{ 0, 0, 0, { NULL, NULL, NULL, NULL}, NULL }
};

struct wdc_pcmcia_product *
	wdc_pcmcia_lookup __P((struct pcmcia_attach_args *));

struct wdc_pcmcia_product *
wdc_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	struct wdc_pcmcia_product *wpp;
	int i, cis_match;

	for (wpp = wdc_pcmcia_products; wpp->wpp_name != NULL; wpp++)
		if ((wpp->wpp_vendor == -1 ||
		     pa->manufacturer == wpp->wpp_vendor) &&
		    (wpp->wpp_product == -1 ||
		     pa->product == wpp->wpp_product)) {
			cis_match = 1;
			for (i = 0; i < 4; i++) {
				if (!(wpp->wpp_cis_info[i] == NULL ||
				      (pa->card->cis1_info[i] != NULL &&
				       strcmp(pa->card->cis1_info[i],
					      wpp->wpp_cis_info[i]) == 0)))
					cis_match = 0;
			}
			if (cis_match)
				return(wpp);
		}

	return (NULL);
}

static int
wdc_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (wdc_pcmcia_lookup(pa) != NULL)
		return (1);

	return (0);
}

static void
wdc_pcmcia_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct wdc_pcmcia_softc *sc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct wdc_pcmcia_product *wpp;

	for (cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head); cfe != NULL;
	    cfe = SIMPLEQ_NEXT(cfe, cfe_list)) {
		if (cfe->num_iospace != 1 && cfe->num_iospace != 2)
			continue;

		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length,
		    cfe->iospace[0].start == 0 ? cfe->iospace[0].length : 0,
		    &sc->sc_pioh))
			continue;

		if (cfe->num_iospace == 2) {
			if (!pcmcia_io_alloc(pa->pf, cfe->iospace[1].start,
			    cfe->iospace[1].length, 0, &sc->sc_auxpioh))
				break;
		} else /* num_iospace == 1 */ {
			sc->sc_auxpioh.iot = sc->sc_pioh.iot;
			if (!bus_space_subregion(sc->sc_pioh.iot,
			    sc->sc_pioh.ioh, WDC_PCMCIA_AUXREG_OFFSET,
			    WDC_PCMCIA_AUXREG_NPORTS, &sc->sc_auxpioh.ioh))
				break;
		}
		pcmcia_io_free(pa->pf, &sc->sc_pioh);
	}

	if (cfe == NULL) {
		printf(": can't handle card info\n");
		return;
	}

	sc->sc_ad.iot = sc->sc_pioh.iot;
	sc->sc_ad.ioh = sc->sc_pioh.ioh;
	sc->sc_ad.auxiot = sc->sc_auxpioh.iot;
	sc->sc_ad.auxioh = sc->sc_auxpioh.ioh;

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		return;
	}

	/*
	 * XXX  DEC Mobile Media CDROM is not yet tested whether it works
	 * XXX  with PCMCIA_WIDTH_IO16.  HAGIWARA SYS-COM HPC-CF32 doesn't
	 * XXX  work with PCMCIA_WIDTH_AUTO.
	 * XXX  CANON FC-8M (SANDISK SDCFB 8M) works for both _AUTO and IO16.
	 * XXX  So, here is temporary work around.
	 */
	wpp = wdc_pcmcia_lookup(pa);
	if (wpp == NULL)
		panic("wdc_pcmcia_attach: impossible");

	if (pcmcia_io_map(pa->pf,
			  wpp->wpp_quirk_flag & WDC_PCMCIA_FORCE_16BIT_IO ?
			  PCMCIA_WIDTH_IO16 : PCMCIA_WIDTH_AUTO, 0,
			  sc->sc_pioh.size, &sc->sc_pioh, &sc->sc_iowindow)) {
		printf(": can't map first I/O space\n");
		return;
	} 

	/*
	 * Currently, # of iospace is 1 except DIGITAL Mobile Media CD-ROM.
	 * So whether the work around like above is necessary or not
	 * is unknown.  XXX.
	 */
	if (cfe->num_iospace > 1 &&
	    pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0,
	    sc->sc_auxpioh.size, &sc->sc_auxpioh, &sc->sc_auxiowindow)) {
		printf(": can't map second I/O space\n");
		return;
	}

	printf("\n");

	sc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_BIO, wdcintr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n", self->dv_xname);
		return;
	}

	if (wpp->wpp_quirk_flag & WDC_PCMCIA_NO_EXTRA_RESETS)
		sc->sc_ad.flags |= WDC_NO_EXTRA_RESETS;

	wdcattach(&sc->sc_wdcdev, &sc->sc_ad);
}
