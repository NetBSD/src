/*	$NetBSD: if_tlp_ap.c,v 1.10.14.1 2009/05/13 17:18:10 jym Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Atsushi Onoe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_tlp_ap.c,v 1.10.14.1 2009/05/13 17:18:10 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/cache.h>

#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/tulipreg.h>
#include <dev/ic/tulipvar.h>

#include <newsmips/apbus/apbusvar.h>

#define	TLP_AP_ROM	0x00000000	/* AProm entry address */
#define	TLP_AP_CFG	0x00040000	/* PCI configuration registers */
#define		TLP_AP_CFG_CFID	0x00		/* Identification */
#define		TLP_AP_CFG_CFCS	0x04		/* Command and status */
#define		TLP_AP_CFG_CFRV	0x08		/* Revision */
#define		TLP_AP_CFG_CFLT	0x0c		/* Latency timer */
#define		TLP_AP_CFG_CBIO	0x10		/* Base I/O address */
#define		TLP_AP_CFG_CBMA	0x14		/* Base memory address */
#define		TLP_AP_CFG_CFIT	0x3c		/* Interrupt */
#define	TLP_AP_CSR	0x00080000	/* CSR base address */
#define	TLP_AP_RST	0x00100000	/* Board Reset */


struct tulip_ap_softc {
	struct tulip_softc sc_tulip;	/* real Tulip softc */
	bus_space_tag_t sc_cfst;
	bus_space_handle_t sc_cfsh;
};

static int	tlp_ap_match(device_t, cfdata_t, void *);
static void	tlp_ap_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tlp_ap, sizeof(struct tulip_ap_softc),
    tlp_ap_match, tlp_ap_attach, NULL, NULL);

static void tlp_ap_preinit(struct tulip_softc *);
static void tlp_ap_tmsw_init(struct tulip_softc *);
static void tlp_ap_getmedia(struct tulip_softc *, struct ifmediareq *);
static int tlp_ap_setmedia(struct tulip_softc *);

const struct tulip_mediasw tlp_ap_mediasw = {
	tlp_ap_tmsw_init, tlp_ap_getmedia, tlp_ap_setmedia
};

static int
tlp_ap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "cbasetx") != 0)
		return 0;

	return 1;
}

/*
 * Install interface into kernel networking data structures
 */
static void
tlp_ap_attach(device_t parent, device_t self, void *aux)
{
	struct tulip_ap_softc *psc = device_private(self);
	struct tulip_softc *sc = &psc->sc_tulip;
	struct apbus_attach_args *apa = aux;
	uint8_t enaddr[ETHER_ADDR_LEN];
	u_int intrmask;
	int i;

	sc->sc_dev = self;

	aprint_normal(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);

	/* PCI configuration register */
	psc->sc_cfst = 0;
	psc->sc_cfsh = apa->apa_hwbase + TLP_AP_CFG;
	sc->sc_devno = apa->apa_slotno;
	sc->sc_rev = bus_space_read_4(psc->sc_cfst, psc->sc_cfsh,
	    TLP_AP_CFG_CFRV);
	switch (bus_space_read_4(psc->sc_cfst, psc->sc_cfsh, TLP_AP_CFG_CFID)) {
	case 0x00091011:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_21140A;
		else
			sc->sc_chip = TULIP_CHIP_21140;
		break;
	default:
		aprint_error(": unable to handle your board\n");
		return;
	}

	aprint_normal(": %s Ethernet, pass %d.%d\n",
	    tlp_chip_names[sc->sc_chip],
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	/* CSR */
	sc->sc_st = 0;
	sc->sc_sh = apa->apa_hwbase + TLP_AP_CSR;
	sc->sc_dmat = apbus_dmatag_init(apa);
	if (sc->sc_dmat == NULL) {
		aprint_error_dev(self, "cannot allocate memory\n");
		return;
	}

	/*
	 * Initialize bus specific parameters.
	 */
	if (mips_sdcache_line_size > 0)
		sc->sc_cacheline = mips_sdcache_line_size / 4;
	else if (mips_pdcache_line_size > 0)
		sc->sc_cacheline = mips_pdcache_line_size / 4;
	else
		sc->sc_cacheline = 4;
	sc->sc_maxburst = sc->sc_cacheline;		/* XXX */
	sc->sc_regshift = 3;
	sc->sc_flags |= TULIPF_DBO | TULIPF_BLE;	/* Big Endian BUS */
	sc->sc_flags |= TULIPF_ENABLED;			/* No Power Mgmt */

	/*
	 * Reset hardware.
	 */
	bus_space_write_4(0, apa->apa_hwbase + TLP_AP_RST, 0, 1);
	delay(100);

	/*
	 * Initialize PCI configuration register
	 */
	bus_space_write_4(psc->sc_cfst, psc->sc_cfsh,
	    TLP_AP_CFG_CFCS, 0x00000005);	/* Master, IO */
	bus_space_write_4(psc->sc_cfst, psc->sc_cfsh,
	    TLP_AP_CFG_CFLT, 0x00000100);
	bus_space_write_4(psc->sc_cfst, psc->sc_cfsh,
	    TLP_AP_CFG_CBIO, MIPS_KSEG1_TO_PHYS(sc->sc_sh));
	bus_space_write_4(psc->sc_cfst, psc->sc_cfsh,
	    TLP_AP_CFG_CFIT, 0x00000101);

	/*
	 * Initialize general purpose port register.
	 */
	TULIP_WRITE(sc, CSR_GPP, GPP_GPC | 0xc0);	/* read */
	TULIP_WRITE(sc, CSR_GPP, 0x40);			/* dipsw port on */
	i = TULIP_READ(sc, CSR_GPP) & GPP_MD;		/* dipsw contents */
	TULIP_WRITE(sc, CSR_GPP, 0xc0);			/* dipsw port off */
	TULIP_WRITE(sc, CSR_GPP, GPP_GPC | 0xcf);	/* read write */
	if (sc->sc_maxburst == 16) {
		TULIP_WRITE(sc, CSR_GPP, 0x8f);		/* 16word burst */
		TULIP_WRITE(sc, CSR_GPP, 0xcf);
	} else {
		TULIP_WRITE(sc, CSR_GPP, 0x8e);		/* 8word burst */
		TULIP_WRITE(sc, CSR_GPP, 0xce);
	}
	TULIP_WRITE(sc, CSR_GPP, GPP_GPC | 0xcf);	/* read write */
	TULIP_WRITE(sc, CSR_GPP, 0xc3);			/* mask abort/DMA err */
	TULIP_WRITE(sc, CSR_GPP, 0xcf);			/* mask abort/DMA err */

	if (tlp_read_srom(sc) == 0) {
		aprint_error_dev(self, "srom read failed\n");
		free(sc->sc_dmat, M_DEVBUF);
		return;
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		enaddr[i] = sc->sc_srom[0x11 + i * 2];

	sc->sc_mediasw = &tlp_ap_mediasw;

	/*
	 * Finish off the attach.
	 */
	tlp_attach(sc, enaddr);

	intrmask = SLOTTOMASK(apa->apa_slotno);
	apbus_intr_establish(0, /* interrupt level (0 or 1) */
	    intrmask,
	    0, /* priority */
	    tlp_intr, sc, apa->apa_name, apa->apa_ctlnum);
}

static void
tlp_ap_preinit(struct tulip_softc *sc)
{

	sc->sc_opmode |= OPMODE_MBO | OPMODE_SCR | OPMODE_PCS | OPMODE_HBD |
	    OPMODE_PS;
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
}

static void
tlp_ap_tmsw_init(struct tulip_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	sc->sc_preinit = tlp_ap_preinit;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = NULL;
	sc->sc_mii.mii_writereg = NULL;
	sc->sc_mii.mii_statchg = sc->sc_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);
	ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_100_TX, 0, NULL);
	ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0,
	    NULL);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_100_TX);
}

static void
tlp_ap_getmedia(struct tulip_softc *sc, struct ifmediareq *ifmr)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	ifmr->ifm_status = IFM_AVALID;
	if (ifp->if_flags & IFF_RUNNING)
		ifmr->ifm_status |= IFM_ACTIVE;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

static int
tlp_ap_setmedia(struct tulip_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	sc->sc_mii.mii_media_active = sc->sc_mii.mii_media.ifm_cur->ifm_media;

	if (ifp->if_flags & IFF_UP)
		tlp_idle(sc, OPMODE_ST|OPMODE_SR);
	if (sc->sc_mii.mii_media_active & IFM_FDX)
		sc->sc_opmode |= OPMODE_FD;
	else
		sc->sc_opmode &= ~OPMODE_FD;
	if (ifp->if_flags & IFF_UP)
		TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
	return 0;
}
