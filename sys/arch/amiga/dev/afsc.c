/*
 * Copyright (c) 1994 Michael L. Hitch
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dma.c
 *	$Id: afsc.c,v 1.1 1994/10/06 19:06:40 chopps Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>
#include <amiga/dev/zthreebusvar.h>

int afscprint __P((void *auxp, char *));
void afscattach __P((struct device *, struct device *, void *));
int afscmatch __P((struct device *, struct cfdata *, void *));
int siopintr __P((struct siop_softc *));

struct scsi_adapter afsc_scsiswitch = {
	siop_scsicmd,
	siop_minphys,
	0,			/* no lun support */
	0,			/* no lun support */
	siop_adinfo,
	"afsc",
};

struct scsi_device afsc_scsidev = {
	NULL,		/* use default error handler */
	NULL,		/* do not have a start functio */
	NULL,		/* have no async handler */
	NULL,		/* Use default done routine */
	"afsc",
	0,
};


#ifdef DEBUG
#endif

struct cfdriver afsccd = {
	NULL, "afsc", afscmatch, afscattach, 
	DV_DULL, sizeof(struct siop_softc), NULL, 0 };

/*
 * if we are a Commodore Amiga A4091
 */
int
afscmatch(pdp, cdp, auxp)
	struct device *pdp;
	struct cfdata *cdp;
	void *auxp;
{
	struct zthreebus_args *zap;

	zap = auxp;
	if (zap->manid == 514 && zap->prodid == 84)
		return(1);
	return(0);
}

void
afscattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct siop_softc *sc;
	struct zthreebus_args *zap;
	siop_regmap_p rp;

	printf("\n");

	zap = auxp;
	
	sc = (struct siop_softc *)dp;
	sc->sc_siopp = rp = zap->va + 0x00800000;

	/*
	 * DCNTL = 37.51->50MHZ / SCLK/2
	 * CTEST7 = 80 [disable burst]
	 */
	sc->sc_clock_freq = 0x8000;

	
	siopinitialize(sc);

	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_targ = 7;
	sc->sc_link.adapter = &afsc_scsiswitch;
	sc->sc_link.device = &afsc_scsidev;
	TAILQ_INIT(&sc->sc_xslist);

	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, &sc->sc_link, afscprint);
}

/*
 * print diag if pnp is NULL else just extra
 */
int
afscprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	if (pnp == NULL)
		return(UNCONF);
	return(QUIET);
}

int
afsc_dmaintr()
{
	siop_regmap_p rp;
	struct siop_softc *dev;
	int i, found;
	u_char istat;

	found = 0;
	for (i = 0; i < afsccd.cd_ndevs; i++) {
		dev = afsccd.cd_devs[i];
		if (dev == NULL)
			continue;
		rp = dev->sc_siopp;
		istat = rp->siop_istat;
		if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
			continue;
		if ((dev->sc_flags & (SIOP_DMA | SIOP_SELECTED)) == SIOP_SELECTED)
			continue;	/* doing non-interrupt I/O */
		found++;
		dev->sc_istat = istat;
		dev->sc_dstat = rp->siop_dstat;
		dev->sc_sstat0 = rp->siop_sstat0;
		siopintr(dev);
	}
	return(found);
}
