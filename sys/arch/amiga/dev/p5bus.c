/*      $NetBSD: p5bus.c,v 1.3 2012/02/12 16:34:07 matt Exp $ */

/*-
 * Copyright (c) 2011, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/* Driver for internal BlizzardPPC, CyberStorm Mk-III/PPC bus. */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/p5busvar.h>

#define ZORRO_MANID_P5		8512
#define ZORRO_PRODID_CSPPC	100 
#define ZORRO_PRODID_BPPC	110

#define P5_ROM_OFF		0xF00010
#define P5_SN_LEN		7

static int	p5bus_match(struct device *pdp, struct cfdata *cfp, void *auxp);
static void	p5bus_attach(device_t parent, device_t self, void *aux);
static char*	p5bus_cardsn(void);
static int	p5bus_print(void *aux, const char *str);
static void	p5bus_callback(device_t self);

struct p5bus_softc {
	device_t	sc_dev;
	uint8_t		sc_cardtype;
	uint8_t		sc_has_scsi;
#define P5BUS_SCSI_NONE	0
#define P5BUS_SCSI_710		1	/* NCR 53C710 */
#define P5BUS_SCSI_770		2	/* NCR 53C770 */
	uint8_t		sc_has_ppc;
#define P5BUS_PPC_NONE		0	/* CS Mk-III only */
#define P5BUS_PPC_OK		1	/* has working PPC CPU */
};

CFATTACH_DECL_NEW(p5bus, sizeof(struct p5bus_softc),
    p5bus_match, p5bus_attach, NULL, NULL);

static int
p5bus_match(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	struct zbus_args *zap;

	zap = auxp;

	if (zap->manid != ZORRO_MANID_P5) 
		return 0;


	if ((zap->prodid != ZORRO_PRODID_BPPC) && 
	    (zap->prodid != ZORRO_PRODID_CSPPC)) 
		return 0;

	return 1;
}

static void
p5bus_attach(device_t parent, device_t self, void *aux)
{
	struct p5bus_softc *sc;
	struct zbus_args *zap;
	struct p5bus_attach_args p5baa;
	char *sn;

	zap = aux;
	sc = device_private(self);
	sc->sc_dev = self;

	sn = p5bus_cardsn();

	aprint_normal(": Phase5 PowerUP on-board bus\n");

	/* "Detect" what devices are present and attach the right drivers. */

	if (zap->prodid == ZORRO_PRODID_CSPPC) {

		if (sn[0] == 'F') {
			aprint_normal_dev(sc->sc_dev, 
			    "CyberStorm Mk-III (sn %s)\n", sn);
			sc->sc_has_ppc = P5BUS_PPC_NONE;
		} else {
			aprint_normal_dev(sc->sc_dev, 
			    "CyberStorm PPC 604e (sn %s)\n", sn);
			sc->sc_has_ppc = P5BUS_PPC_OK;
		}

		sc->sc_cardtype = P5_CARDTYPE_CS;
		sc->sc_has_scsi = P5BUS_SCSI_770;
	
	} else if (zap->prodid == ZORRO_PRODID_BPPC) {

		if (sn[0] != 'I') {	/* only "+" model has SCSI */
			aprint_normal_dev(sc->sc_dev, 
			    "BlizzardPPC 603e (sn %s)\n", sn);
			sc->sc_has_scsi = P5BUS_SCSI_NONE;
		} else {
			aprint_normal_dev(sc->sc_dev, 
			    "BlizzardPPC 603e+ (sn %s)\n", sn);
			sc->sc_has_scsi = P5BUS_SCSI_710;
		}
	
		sc->sc_cardtype = P5_CARDTYPE_BPPC;	
		sc->sc_has_ppc = P5BUS_PPC_OK;

	}

	p5baa.p5baa_cardtype = sc->sc_cardtype;

	/* Attach the SCSI host adapters. */
	switch (sc->sc_has_scsi) {
	case P5BUS_SCSI_710:
		strcpy(p5baa.p5baa_name, "bppcsc");
		config_found_ia(sc->sc_dev, "p5bus", &p5baa, p5bus_print);
		break;
	case P5BUS_SCSI_770:
		strcpy(p5baa.p5baa_name, "cbiiisc");
		config_found_ia(sc->sc_dev, "p5bus", &p5baa, p5bus_print);
		break;
	default:
		break;
	}

	/*
	 * We need to wait for possible p5membar attachments. Defer the rest 
	 * until parent (zbus) is completely configured. 
	 */
	config_defer(self, p5bus_callback);

}

/* Continue the attachment. */
static void
p5bus_callback(device_t self) {

	struct p5bus_attach_args p5baa;
	struct p5bus_softc *sc;

	sc = device_private(self);
	p5baa.p5baa_cardtype = sc->sc_cardtype;

	/* p5pb is always found, probe is inside of p5pb driver */
	strcpy(p5baa.p5baa_name, "p5pb");
	config_found_ia(sc->sc_dev, "p5bus", &p5baa, p5bus_print);
}

/* Get serial number of the card. */
static char * 
p5bus_cardsn(void)
{
	char *snr, *sn;

	sn = kmem_zalloc(P5_SN_LEN + 1, KM_SLEEP);
	snr = (char *)__UNVOLATILE(ztwomap(P5_ROM_OFF));

	memcpy(sn, snr, P5_SN_LEN);
	return sn;
}

static int
p5bus_print(void *aux, const char *str)
{
	if (str == NULL)
		return 0;

	printf("%s ", str);

	return 0;
}

