/*	$NetBSD: wdc_pcmcia.c,v 1.47 2002/03/31 07:19:03 martin Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_pcmcia.c,v 1.47 2002/03/31 07:19:03 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define WDC_PCMCIA_REG_NPORTS      8
#define WDC_PCMCIA_AUXREG_OFFSET   (WDC_PCMCIA_REG_NPORTS + 6)
#define WDC_PCMCIA_AUXREG_NPORTS   2

struct wdc_pcmcia_softc {
	struct wdc_softc sc_wdcdev;
	struct channel_softc *wdc_chanptr;
	struct channel_softc wdc_channel;
	struct pcmcia_io_handle sc_pioh;
	struct pcmcia_io_handle sc_auxpioh;
	struct pcmcia_mem_handle sc_pmembaseh;
	struct pcmcia_mem_handle sc_pmemh;
	struct pcmcia_mem_handle sc_auxpmemh;
	int sc_memwindow;
	int sc_iowindow;
	int sc_auxiowindow;
	void *sc_ih;
	struct pcmcia_function *sc_pf;
	int sc_flags;
#define	WDC_PCMCIA_ATTACH	0x0001
#define WDC_PCMCIA_MEMMODE	0x0002
};

static int wdc_pcmcia_match	__P((struct device *, struct cfdata *, void *));
static void wdc_pcmcia_attach	__P((struct device *, struct device *, void *));
static int wdc_pcmcia_detach	__P((struct device *, int));

struct cfattach wdc_pcmcia_ca = {
	sizeof(struct wdc_pcmcia_softc), wdc_pcmcia_match, wdc_pcmcia_attach,
	wdc_pcmcia_detach, wdcactivate
};

const struct wdc_pcmcia_product {
	u_int32_t	wpp_vendor;	/* vendor ID */
	u_int32_t	wpp_product;	/* product ID */
	int		wpp_quirk_flag;	/* Quirk flags */
#define WDC_PCMCIA_NO_EXTRA_RESETS	0x02 /* Only reset ctrl once */
	const char	*wpp_cis_info[4];	/* XXX necessary? */
	const char	*wpp_name;	/* product name */
} wdc_pcmcia_products[] = {

	{ /* PCMCIA_VENDOR_DIGITAL XXX */ 0x0100,
	  PCMCIA_PRODUCT_DIGITAL_MOBILE_MEDIA_CDROM,
	  0, { NULL, "Digital Mobile Media CD-ROM", NULL, NULL },
	  PCMCIA_STR_DIGITAL_MOBILE_MEDIA_CDROM },

	{ PCMCIA_VENDOR_IBM,
	  PCMCIA_PRODUCT_IBM_PORTABLE_CDROM,
	  0, { NULL, "PCMCIA Portable CD-ROM Drive", NULL, NULL },
	  PCMCIA_STR_IBM_PORTABLE_CDROM },

	/* The TEAC IDE/Card II is used on the Sony Vaio */
	{ PCMCIA_VENDOR_TEAC,
	  PCMCIA_PRODUCT_TEAC_IDECARDII,
	  WDC_PCMCIA_NO_EXTRA_RESETS,
	  PCMCIA_CIS_TEAC_IDECARDII,
	  PCMCIA_STR_TEAC_IDECARDII },

	/*
	 * A fujitsu rebranded panasonic drive that reports 
	 * itself as function "scsi", disk interface 0
	 */
	{ PCMCIA_VENDOR_PANASONIC,
	  PCMCIA_PRODUCT_PANASONIC_KXLC005,
	  0,
	  PCMCIA_CIS_PANASONIC_KXLC005,
	  PCMCIA_STR_PANASONIC_KXLC005 },

	/*
	 * EXP IDE/ATAPI DVD Card use with some DVD players.
	 * Does not have a vendor ID or product ID.
	 */
	{ -1,
	  -1,
	  0,
	  PCMCIA_CIS_EXP_EXPMULTIMEDIA,
	  PCMCIA_STR_EXP_EXPMULTIMEDIA },

	/* Mobile Dock 2, neither vendor ID nor product ID */
	{ -1, -1, 0,
	  { "SHUTTLE TECHNOLOGY LTD.", "PCCARD-IDE/ATAPI Adapter", NULL, NULL},
	  "SHUTTLE TECHNOLOGY IDE/ATAPI Adapter"
	},

	/* Toshiba Portege 3110 CD, neither vendor ID nor product ID */
	{ -1, -1, 0,
	  { "FREECOM", "PCCARD-IDE", NULL, NULL},
	  "FREECOM PCCARD-IDE"
	},

	/* Random CD-ROM, (badged AMACOM), neither vendor ID nor product ID */ 
	{ -1, -1, 0,
	  { "PCMCIA", "CD-ROM", NULL, NULL},
	  "PCMCIA CD-ROM"
	},

	/* IO DATA CBIDE2, with neither vendor ID nor product ID */
	{ -1, -1, 0,
	  PCMCIA_CIS_IODATA_CBIDE2,
	  PCMCIA_STR_IODATA_CBIDE2
	},

	/* 
	 * Novac PCMCIA-IDE Card for HD530P IDE Box, 
	 * with neither vendor ID nor product ID
	 */
	{ -1, -1, 0,
	  { "PCMCIA", "PnPIDE", NULL, NULL},
	  "Novac PCCARD-IDE"
	},

	{ 0, 0, 0, { NULL, NULL, NULL, NULL}, NULL }
};

const struct wdc_pcmcia_product *
	wdc_pcmcia_lookup __P((struct pcmcia_attach_args *));

int	wdc_pcmcia_enable __P((struct device *, int));

const struct wdc_pcmcia_product *
wdc_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	const struct wdc_pcmcia_product *wpp;
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
				return (wpp);
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

	if (pa->pf->function == PCMCIA_FUNCTION_DISK && 
	    pa->pf->pf_funce_disk_interface == PCMCIA_TPLFE_DDI_PCCARD_ATA) {
		return 10;
	}

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
	const struct wdc_pcmcia_product *wpp;
	bus_size_t offset;
	int quirks;

	sc->sc_pf = pa->pf;

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

	/* 
	 * Compact Flash memory mapped mode
	 * CF+ and CompactFlash Spec. Rev 1.4, 6.1.3 Memory Mapped Addressing.
	 * http://www.compactflash.org/cfspc1_4.pdf
	 */
	if (cfe == NULL) {
		SIMPLEQ_FOREACH(cfe, &pa->pf->cfe_head, cfe_list) {
			if (cfe->iftype != PCMCIA_IFTYPE_MEMORY)
				continue;
			if (pcmcia_mem_alloc(pa->pf, cfe->memspace[0].length,
			    &sc->sc_pmembaseh) == 0) {
				sc->sc_flags |= WDC_PCMCIA_MEMMODE;
				break;
			}
		}
	}

	if (cfe == NULL) {
		printf(": can't handle card info\n");
		goto no_config_entry;
	}

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		goto enable_failed;
	}

	wpp = wdc_pcmcia_lookup(pa);
	if (wpp != NULL)
		quirks = wpp->wpp_quirk_flag;
	else
		quirks = 0;

	if (sc->sc_flags & WDC_PCMCIA_MEMMODE) {
		if (pcmcia_mem_map(pa->pf, PCMCIA_MEM_COMMON, 0,
		    sc->sc_pmembaseh.size, &sc->sc_pmembaseh, &offset,
		    &sc->sc_memwindow)) {
			printf(": can't map memory space\n");
			goto map_failed;
		}

		sc->sc_pmemh.memt = sc->sc_pmembaseh.memt;
		if (offset == 0) {
			sc->sc_pmemh.memh = sc->sc_pmembaseh.memh;
		} else {
			if (bus_space_subregion(sc->sc_pmemh.memt,
				sc->sc_pmembaseh.memh, offset,
				WDC_PCMCIA_REG_NPORTS, &sc->sc_pmemh.memh))
				goto mapaux_failed;
		}

		sc->sc_auxpmemh.memt = sc->sc_pmemh.memt;
		if (bus_space_subregion(sc->sc_pmemh.memt,
		    sc->sc_pmemh.memh, WDC_PCMCIA_AUXREG_OFFSET,
		    WDC_PCMCIA_AUXREG_NPORTS, &sc->sc_auxpmemh.memh))
			goto mapaux_failed;
		
		printf(" memory mapped mode");
	} else {
		if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0,
		    sc->sc_pioh.size, &sc->sc_pioh, &sc->sc_iowindow)) {
			printf(": can't map first I/O space\n");
			goto map_failed;
		} 
	}

	if (cfe->num_iospace <= 1 || sc->sc_flags & WDC_PCMCIA_MEMMODE)
		sc->sc_auxiowindow = -1;
	else if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0,
	    sc->sc_auxpioh.size, &sc->sc_auxpioh, &sc->sc_auxiowindow)) {
		printf(": can't map second I/O space\n");
		goto mapaux_failed;
	}

	if ((wpp != NULL) && (wpp->wpp_name != NULL))
		printf(": %s", wpp->wpp_name);
	
	printf("\n");

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	if (sc->sc_flags & WDC_PCMCIA_MEMMODE) {
		sc->wdc_channel.cmd_iot = sc->sc_pmemh.memt;
		sc->wdc_channel.cmd_ioh = sc->sc_pmemh.memh;
		sc->wdc_channel.ctl_iot = sc->sc_auxpmemh.memt;
		sc->wdc_channel.ctl_ioh = sc->sc_auxpmemh.memh;
	} else {
		sc->wdc_channel.cmd_iot = sc->sc_pioh.iot;
		sc->wdc_channel.cmd_ioh = sc->sc_pioh.ioh;
		sc->wdc_channel.ctl_iot = sc->sc_auxpioh.iot;
		sc->wdc_channel.ctl_ioh = sc->sc_auxpioh.ioh;
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32;
	}
	sc->wdc_channel.data32iot = sc->wdc_channel.cmd_iot;
	sc->wdc_channel.data32ioh = sc->wdc_channel.cmd_ioh;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_SINGLE_DRIVE;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->wdc_chanptr = &sc->wdc_channel;
	sc->sc_wdcdev.channels = &sc->wdc_chanptr;
	sc->sc_wdcdev.nchannels = 1;
	sc->wdc_channel.channel = 0;
	sc->wdc_channel.wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue = malloc(sizeof(struct channel_queue),
	    M_DEVBUF, M_NOWAIT);
	if (sc->wdc_channel.ch_queue == NULL) {
		printf("%s: can't allocate memory for command queue\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		goto ch_queue_alloc_failed;
	}
	if (quirks & WDC_PCMCIA_NO_EXTRA_RESETS)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_NO_EXTRA_RESETS;

	/* We can enable and disable the controller. */
	sc->sc_wdcdev.sc_atapi_adapter._generic.adapt_enable =
	    wdc_pcmcia_enable;

	sc->sc_flags |= WDC_PCMCIA_ATTACH;
	wdcattach(&sc->wdc_channel);	/* should return an error XXX */
	sc->sc_flags &= ~WDC_PCMCIA_ATTACH;
	return;

 ch_queue_alloc_failed:
	/* Unmap our aux i/o window. */
	if (!(sc->sc_flags & WDC_PCMCIA_MEMMODE) && (sc->sc_auxiowindow != -1))
		pcmcia_io_unmap(sc->sc_pf, sc->sc_auxiowindow);

 mapaux_failed:
	/* Unmap our i/o window. */
	if (sc->sc_flags & WDC_PCMCIA_MEMMODE)
		pcmcia_mem_unmap(sc->sc_pf, sc->sc_memwindow);
	else
		pcmcia_io_unmap(sc->sc_pf, sc->sc_iowindow);

 map_failed:
	/* Disable the function */
	pcmcia_function_disable(sc->sc_pf);

 enable_failed:
	/* Unmap our i/o space. */
	if (sc->sc_flags & WDC_PCMCIA_MEMMODE) {
		pcmcia_mem_free(sc->sc_pf, &sc->sc_pmembaseh);
	} else  {
		pcmcia_io_free(sc->sc_pf, &sc->sc_pioh);
		if (cfe->num_iospace == 2)
		    pcmcia_io_free(sc->sc_pf, &sc->sc_auxpioh);
	}
 no_config_entry:
	sc->sc_iowindow = -1;
}

int
wdc_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct wdc_pcmcia_softc *sc = (struct wdc_pcmcia_softc *)self;
	int error;

	if (sc->sc_iowindow == -1)
		/* Nothing to detach */
		return (0);

	if ((error = wdcdetach(self, flags)) != 0)
		return (error);

	if (sc->wdc_channel.ch_queue != NULL)
		free(sc->wdc_channel.ch_queue, M_DEVBUF);

	/* Unmap our i/o window and i/o space. */
	if (sc->sc_flags & WDC_PCMCIA_MEMMODE) {
		pcmcia_mem_unmap(sc->sc_pf, sc->sc_memwindow);
		pcmcia_mem_free(sc->sc_pf, &sc->sc_pmembaseh);
	} else {
		pcmcia_io_unmap(sc->sc_pf, sc->sc_iowindow);
		pcmcia_io_free(sc->sc_pf, &sc->sc_pioh);
		if (sc->sc_auxiowindow != -1) {
			pcmcia_io_unmap(sc->sc_pf, sc->sc_auxiowindow);
			pcmcia_io_free(sc->sc_pf, &sc->sc_auxpioh);
		}
	}

	return (0);
}

int
wdc_pcmcia_enable(self, onoff)
	struct device *self;
	int onoff;
{
	struct wdc_pcmcia_softc *sc = (void *)self;

	if (onoff) {
		/* Establish the interrupt handler. */
		sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_BIO,
		    wdcintr, &sc->wdc_channel);
		if (sc->sc_ih == NULL) {
			printf("%s: "
			    "couldn't establish interrupt handler\n",
			    sc->sc_wdcdev.sc_dev.dv_xname);
			return (EIO);
		}

		/* See the comment in aic_pcmcia_enable */
		if ((sc->sc_flags & WDC_PCMCIA_ATTACH) == 0) {
			if (pcmcia_function_enable(sc->sc_pf)) {
				printf("%s: couldn't enable PCMCIA function\n",
				    sc->sc_wdcdev.sc_dev.dv_xname);
				pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
				return (EIO);
			}
			wdcreset(&sc->wdc_channel, VERBOSE);
		}
	} else {
		pcmcia_function_disable(sc->sc_pf);
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
	}

	return (0);
}
