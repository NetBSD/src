/*  $NetBSD: mii.c,v 1.1.2.1 1998/11/10 06:02:44 cgd Exp $   */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#if defined(SIOCSIFMEDIA)
#include <net/if_media.h>
#endif

#include <dev/mii/mii_adapter.h>
#include <dev/mii/mii_phy.h>
#include <dev/mii/generic_phy.h>

#include "locators.h"

/* The mii bus private data definitions */

struct mii_softc {
	struct device sc_dev;
	mii_data_t *adapter;
	mii_phy_t *phy[32];
	mii_phy_t *current_phy;
};

static void mii_sync __P((mii_data_t *));
static void mii_sendbit __P((mii_data_t *, u_int32_t, int));

#ifdef __BROKEN_INDIRECT_CONFIG
int miimatch __P((struct device *, void *, void *));
int mii_configmatch __P((struct device *, void *, void *));
#else
int miimatch __P((struct device *, struct cfdata *, void *));
int mii_configmatch __P((struct device *, struct cfdata *, void *));
#endif
void miiattach __P((struct device *, struct device *, void *));

int mii_print __P((void *, const char *));

struct cfattach mii_ca = {
	sizeof(struct mii_softc), miimatch, miiattach
};

struct cfdriver mii_cd = {
	NULL, "mii", DV_DULL
};

int mii_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	mii_phy_t *phy = aux;
	if (pnp)
		printf("ID %x at %s", phy->phy_id, pnp);
	printf(" dev %d", phy->dev);
	return (UNCONF);
}

#ifdef __BROKEN_INDIRECT_CONFIG
int
miimatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
#else
int
miimatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#endif
	return 1;
}

void
miiattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int phy_id_l, phy_id_h;
	int i;
	mii_phy_t *phy;
	struct mii_softc *sc = (struct mii_softc *)self;
	mii_data_t *adapter = aux;
	/* struct cfdata *cf; */

	printf("\n");
	sc->adapter = adapter;
	sc->adapter->mii_sc = sc; 
	sc->current_phy = NULL;
	
	for (i=0; i < 32; i++) {
		phy_id_h = mii_readreg(sc, i, PHY_IDH);
		phy_id_l = mii_readreg(sc, i, PHY_IDL);
#ifdef MII_DEBUG
		printf("Id of PHY 0x%x: 0x%x%x\n", i,
			phy_id_h, phy_id_l);
#endif
		if (phy_id_h != -1 && phy_id_l != -1) {
			phy = malloc(sizeof(mii_phy_t), M_DEVBUF, M_WAITOK);
			phy->phy_id = ((phy_id_h & 0xffff) << 16) | (phy_id_l & 0xffff);
			phy->adapter_id = adapter->adapter_id;
			phy->dev = i;
			phy->mii_softc = sc;
#if 0
			if ((cf = config_search(mii_configmatch, (struct device*)sc, phy))
				!= NULL) {
				sc->phy[i] = phy;
				config_attach((struct device*)sc, cf, phy, mii_print);
			} else {
				sc->phy[i] = NULL;
				mii_print(phy, sc->sc_dev.dv_xname);
				printf(" not configured\n");
				free(phy, M_DEVBUF);
			}
#else
			if (config_found_sm(self, phy, mii_print, mii_configmatch)
				!= NULL) {
				sc->phy[i] = phy;
			} else {
				sc->phy[i] = NULL;
				free(phy, M_DEVBUF);
			}
#endif
		}
	}
}

#ifdef __BROKEN_INDIRECT_CONFIG
int
mii_configmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf =match;
#else
int
mii_configmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#endif
	mii_phy_t *phy = aux;

	if (cf->cf_loc[MIICF_DEV] != MIICF_DEV_DEFAULT &&
		cf->cf_loc[MIICF_DEV] != phy->dev)
		return 0;
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

static void
mii_sync(adapter)
	mii_data_t* adapter;
{
	int i;

	adapter->mii_clrbit(adapter->adapter_softc, MII_TXEN);
	for (i=0; i<32; i++) {
		adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
		adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);
	}
}

static void
mii_sendbit(adapter, data, nbits)
	mii_data_t *adapter;
	u_int32_t data;
	int nbits;
{
	int i;

	adapter->mii_setbit(adapter->adapter_softc, MII_TXEN);
	for (i = 1 << (nbits -1); i; i = i >>  1) {
		adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
		adapter->mii_readbit(adapter->adapter_softc, MII_CLOCK);
		if (data & i)
			adapter->mii_setbit(adapter->adapter_softc, MII_DATA);
		else
			adapter->mii_clrbit(adapter->adapter_softc, MII_DATA);
		adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);
		adapter->mii_readbit(adapter->adapter_softc, MII_CLOCK);
	}
}

int mii_readreg(v, phy, reg)
	void *v;
	u_int16_t phy;
	u_int16_t reg;
{
	mii_data_t *adapter = ((struct mii_softc *)v)->adapter;
	u_int16_t val = 0;
	int err =0;
	int i;

	if (adapter->mii_readreg) /* adapter has a special way to read PHYs */
		return adapter->mii_readreg(adapter->adapter_softc, phy, reg);

	/* else read using the control lines */
	mii_sync(adapter);
	mii_sendbit(adapter, MII_START, 2);
	mii_sendbit(adapter, MII_READ, 2);
	mii_sendbit(adapter, phy, 5);
	mii_sendbit(adapter, reg, 5);

	adapter->mii_clrbit(adapter->adapter_softc, MII_TXEN);
	adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
	adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);
	adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);

	err = adapter->mii_readbit(adapter->adapter_softc, MII_DATA);
	adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);

	for (i=0; i<16; i++) {
		val = val << 1;
		adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
		if (!err)
			if (adapter->mii_readbit(adapter->adapter_softc, MII_DATA))
				val |= 1;
		adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);
	}
	adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
	adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);

	if (!err)
		return val;
	else
		return -1;
}

void mii_writereg(v, phy, reg, data)
	void *v;
	u_int16_t phy;
	u_int16_t reg;
	u_int16_t data;
{
	mii_data_t *adapter = ((struct mii_softc *)v)->adapter;

	if (adapter->mii_writereg) { /* adapter has a special way to write PHYs */
		adapter->mii_writereg(adapter, phy, reg, data);
		return;
	}

	/* else write using the control lines */
	mii_sync(adapter);
	mii_sendbit(adapter, MII_START, 2);
	mii_sendbit(adapter, MII_WRITE, 2);
	mii_sendbit(adapter, phy, 5);
	mii_sendbit(adapter, reg, 5);
	mii_sendbit(adapter, MII_ACK, 2);
	mii_sendbit(adapter, data, 16);

	adapter->mii_clrbit(adapter->adapter_softc, MII_CLOCK);
	adapter->mii_setbit(adapter->adapter_softc, MII_CLOCK);
}

void mii_media_add(ifmedia, adapter)
	struct ifmedia *ifmedia;
	mii_data_t *adapter;
{
	struct mii_softc *sc = adapter->mii_sc;
	int i;
	u_int32_t media = 0;

	for (i = 0; i < 32; i++) {
		if (sc->phy[i])
			media |= sc->phy[i]->phy_media;
	}
	if (media & PHY_BNC)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_10_2, 0, NULL);
	if (media & PHY_AUI)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_10_5, 0, NULL);
	if (media & PHY_10baseT)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_10_T, 0, NULL);
	if (media & PHY_10baseTfd)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_10_T | IFM_FDX, 0, NULL);
	if (media & PHY_100baseTx)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_100_TX, 0, NULL);
	if (media & PHY_100baseTxfd)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_100_TX | IFM_FDX, 0, NULL);
	if (media & PHY_100baseT4)
		ifmedia_add(ifmedia, IFM_ETHER | IFM_100_T4, 0, NULL);
}

int mii_mediachg(adapter)
	mii_data_t *adapter;
{
	struct mii_softc *sc = adapter->mii_sc;
	int i, best = -1, error = 0;
	int media = adapter->mii_media_active;

	sc->current_phy = NULL;

	for (i=0; i<32; i++) {
		if (sc->phy[i] == NULL)
			continue;
		switch (sc->phy[i]->phy_media_set(media, sc->phy[i]->phy_softc)) {
		case -1: /* PHY not available */
			break;
		case 0: /* link sucessfully selected */
			sc->current_phy = sc->phy[i];
			break;
		case ENETDOWN: /* link selected but not up */
			best = i;
			break;
		default:
			break;
		}
	}
	if (sc->current_phy == NULL) {
	/*
	 * We didn't find a valid media. Select the best one (i.e.
	 * last supported but not up). If media != autoselect, don't report
	 * any error code.
	 */
		if (best < 0)
			return EINVAL;
		sc->current_phy = sc->phy[best];
		error = sc->phy[best]->phy_media_set(media, sc->phy[best]->phy_softc);
		if (media != IFM_AUTO)
		 error = 0;
	}
	/* power down all but current phy */
	for (i=0; i<32; i++) {
		if (sc->phy[i] != sc->current_phy) {
			if (sc->phy[i] == NULL)
				mii_writereg(sc, i, PHY_CONTROL, CTRL_ISO);
			else
				sc->phy[i]->phy_pdown(sc->phy[i]->phy_softc);
		}
	}
	return error;
}

void mii_pollstat(adapter)
	mii_data_t *adapter;
{
	struct mii_softc *sc = adapter->mii_sc;

	adapter->mii_media_status = IFM_AVALID;
	if (sc->current_phy == NULL)
		return;
	if (sc->current_phy->phy_status(adapter->mii_media_active,
		sc->current_phy->phy_softc) == 0)
		adapter->mii_media_status |= IFM_ACTIVE;
}
