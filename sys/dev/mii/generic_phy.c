/*	$NetBSD: generic_phy.c,v 1.2 1997/11/17 08:38:04 thorpej Exp $	*/
 
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
 *	This product includes software developed by Manuel Bouyer.
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
#include <sys/proc.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii_adapter.h>
#include <dev/mii/mii_phy.h>
#include <dev/mii/generic_phy.h>

int
phy_reset(sc)
	struct phy_softc *sc;
{
	int i, control;
	mii_phy_t *phy_link = sc->phy_link;

	/* reset the PHY, wait for the reset bit to clear */
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL,
	    CTRL_LOOPBK);
	DELAY(1000);
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL,
	    CTRL_LOOPBK | CTRL_RESET);
	i = 0;
	do {
		DELAY(100000);
		control = mii_readreg(phy_link->mii_softc, phy_link->dev,
		    PHY_CONTROL);
	} while ((control < 0 || (control & CTRL_RESET) != 0) && ++i < 10);
	if (control < 0 || (control & CTRL_RESET) != 0) {
		printf("%s: reset failed\n", sc->sc_dev.dv_xname);
		return (0);
	} 
	return (1);
}

int
phy_media_probe(sc)
	struct phy_softc *sc;
{
	mii_phy_t *phy_link = sc->phy_link;
	int reg;

	reg = mii_readreg(phy_link->mii_softc, phy_link->dev,
	    PHY_AN_Adv);
	if (reg < 0)
		return (2);
	if (reg & Adv_10bT)
		phy_link->phy_media |= PHY_10baseT;
	if (reg & Adv_10bT_fd)
		phy_link->phy_media |= PHY_10baseTfd;
	if (reg & Adv_100bTx)
		phy_link->phy_media |= PHY_100baseTx;
	if (reg & Adv_100bTx_fd)
		phy_link->phy_media |= PHY_100baseTxfd;
	if (reg & Adv_100bT4)
		phy_link->phy_media |= PHY_100baseT4;
	return (0);
}

void
phy_media_print(media)
	u_int32_t media;
{
	char tmpstr[80], *p = tmpstr;

	if (media & PHY_AUI)
		 p += sprintf(p, "AUI/");

	if (media & PHY_BNC)
		 p += sprintf(p, "BNC/");

	if (media & (PHY_10baseT | PHY_10baseTfd)) {
		p += sprintf(p, "10baseT/");
		if (media & PHY_10baseTfd) {
			p--;
			p += sprintf(p, "(full duplex)/");
		}
	}
	if (media & (PHY_100baseTx | PHY_100baseTxfd)) {
		p += sprintf(p, "100baseTx/");
		if (media & PHY_100baseTxfd) {
			p--;
			p += sprintf(p, "(full duplex)/");
		}
	}
	if (media & PHY_100baseT4)
		p += sprintf(p, "100baseT4/");
	p--;
	*p = '\0';
	printf("%s", tmpstr);
}
	
int
phy_media_set_10_100(sc, media)
	struct phy_softc *sc;
	int media;
{
	mii_phy_t *phy_link = sc->phy_link;
	int i, reg;
	char *mediastr = "10baseT";
	char *dpx;
	int subtype = IFM_SUBTYPE(media);

	if (subtype == IFM_10_T &&
	    (phy_link->phy_media & (PHY_10baseT | PHY_10baseTfd)) == 0)
		return (EINVAL);
	if (subtype == IFM_100_TX &&
	    (phy_link->phy_media & (PHY_100baseTx | PHY_100baseTxfd)) == 0)
		return (EINVAL);
	if (subtype == IFM_100_T4 &&
	    (phy_link->phy_media & PHY_100baseT4) == 0)
		return (EINVAL);

	reg = (media & IFM_FDX) ? CTRL_DUPLEX : 0;
	dpx = (media & IFM_FDX) ? " (full duplex)" : "";
	if (subtype == IFM_100_TX || subtype == IFM_100_T4) {
		reg |= CTRL_SPEED;
		mediastr = "100baseTx";
	}
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL, reg);
	i = 0;
	do {
		DELAY(100000);
		reg = mii_readreg(phy_link->mii_softc, phy_link->dev,
		    PHY_STATUS);
		if (reg < 0) {
			printf("%s: can't read status register\n",
			    sc->sc_dev.dv_xname);
			return (EIO);
		}
	} while ((reg & ST_LINK) == 0 && ++i < 10);
	if ((reg & ST_LINK) == 0) {
		printf("%s: %s%s: no carrier\n", sc->sc_dev.dv_xname,
			mediastr, dpx);
		/* phy_dumpreg(sc); */
		return (ENETDOWN);
	}
	/* printf("%s: selected %s%s\n", sc->sc_dev.dv_xname, mediastr, dpx); */
	return (0);
}

int
phy_status(media, v)
	int media;
	void *v;
{
	struct phy_softc * sc = v;
	int reg;
	reg = mii_readreg(sc->phy_link->mii_softc, sc->phy_link->dev,
	    PHY_STATUS);
	reg = mii_readreg(sc->phy_link->mii_softc, sc->phy_link->dev,
	    PHY_STATUS); /* Read twice to clear latched status */
	
	if ((reg & ST_LINK) == 0)
		return (ENETDOWN);
	return (0);
}

void
phy_dumpreg(sc)
	struct phy_softc * sc;
{   
	int i;

	for (i = 0; i <= 0x12; i++)
		printf("%s: reg 0x%x = 0x%x\n", sc->sc_dev.dv_xname, i,
		    mii_readreg(sc->phy_link->mii_softc, sc->phy_link->dev, i));
}   
