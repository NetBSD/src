/*	$NetBSD: ivsc.c,v 1.8 1995/01/05 07:22:38 chopps Exp $	*/

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
 *	@(#)ivsdma.c
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <vm/vm.h>		/* XXXX Kludge for IVS Vector - mlh */
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/scireg.h>
#include <amiga/dev/scivar.h>
#include <amiga/dev/zbusvar.h>

int ivscprint __P((void *auxp, char *));
void ivscattach __P((struct device *, struct device *, void *));
int ivscmatch __P((struct device *, struct cfdata *, void *));

int ivsc_intr __P((void));
#ifdef notyet
int ivsc_dma_xfer_in __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));
int ivsc_dma_xfer_out __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));
#endif

struct scsi_adapter ivsc_scsiswitch = {
	sci_scsicmd,
	sci_minphys,
	0,			/* no lun support */
	0,			/* no lun support */
};

struct scsi_device ivsc_scsidev = {
	NULL,		/* use default error handler */
	NULL,		/* do not have a start functio */
	NULL,		/* have no async handler */
	NULL,		/* Use default done routine */
};

#define QPRINTF

#ifdef DEBUG
extern int sci_debug;
#endif

extern int sci_data_wait;

struct cfdriver ivsccd = {
	NULL, "ivsc", (cfmatch_t)ivscmatch, ivscattach, 
	DV_DULL, sizeof(struct sci_softc), NULL, 0 };

/*
 * if this is an IVS board
 */
int
ivscmatch(pdp, cdp, auxp)
	struct device *pdp;
	struct cfdata *cdp;
	void *auxp;
{
	struct zbus_args *zap;

	zap = auxp;

	/*
	 * Check manufacturer and product id.
	 */
	if (zap->manid != 2112 ||	/* If manufacturer is IVS */
	    (zap->prodid != 52 &&	/*   product = Trumpcard */
	    zap->prodid != 243))	/*   product = Vector SCSI */
		return(0);		/* didn't match */
	if (zap->prodid == 243) {
		/*
		 * XXXX Ouch! board addresss isn't Zorro II or Zorro III!
		 * XXXX Kludge it up until I can do it better (MLH).
		 *
		 * XXXX pa 0x00f00000 shouldn't be used for anything
		 */
		if (pmap_extract(kernel_pmap, (vm_offset_t) ztwomap(0x00f00000))
		    == 0x00f00000) {
			physaccess(ztwomap(0x00f00000),zap->pa,
			    NBPG, PG_W|PG_CI);
			zap->va = (void *) ztwomap(0x00f00000) +
			    ((int)zap->pa & PGOFSET);
#ifdef DEBUG
			printf("IVS Vector: mapped to %x kva %x\n",
			    ztwomap(0x00f00000), zap->va);
#endif
		}
		else {
			printf("Unable to map IVS Vector SCSI\n");
			return (0);
		}
	}
	return(1);
}

void
ivscattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	volatile u_char *rp;
	struct sci_softc *sc;
	struct zbus_args *zap;

	printf("\n");

	zap = auxp;
	
	sc = (struct sci_softc *)dp;
	rp = zap->va + 0x40;
	sc->sci_data = rp;
	sc->sci_odata = rp;
	sc->sci_icmd = rp + 2;
	sc->sci_mode = rp + 4;
	sc->sci_tcmd = rp + 6;
	sc->sci_bus_csr = rp + 8;
	sc->sci_sel_enb = rp + 8;
	sc->sci_csr = rp + 10;
	sc->sci_dma_send = rp + 10;
	sc->sci_idata = rp + 12;
	sc->sci_trecv = rp + 12;
	sc->sci_iack = rp + 14;
	sc->sci_irecv = rp + 14;

#ifdef notyet
	sc->dma_xfer_in = ivsc_dma_xfer_in;
	sc->dma_xfer_out = ivsc_dma_xfer_out;
#endif

	scireset(sc);

	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_target = 7;
	sc->sc_link.adapter = &ivsc_scsiswitch;
	sc->sc_link.device = &ivsc_scsidev;
	sc->sc_link.openings = 1;
	TAILQ_INIT(&sc->sc_xslist);

	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, &sc->sc_link, ivscprint);
}

/*
 * print diag if pnp is NULL else just extra
 */
int
ivscprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	if (pnp == NULL)
		return(UNCONF);
	return(QUIET);
}

#ifdef notyet
int
ivsc_dma_xfer_in (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
}

int
ivsc_dma_xfer_out (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
}
#endif

int
ivsc_intr()
{
	struct sci_softc *dev;
	int i, found;
	u_char stat;

	found = 0;
	for (i = 0; i < ivsccd.cd_ndevs; i++) {
		dev = ivsccd.cd_devs[i];
		if (dev == NULL)
			continue;
		if ((*dev->sci_csr & SCI_CSR_INT) == 0)
			continue;
		++found;
		stat = *dev->sci_iack;
	}
	return (found);
}
