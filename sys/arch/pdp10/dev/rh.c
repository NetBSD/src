/*	$NetBSD: rh.c,v 1.1 2003/08/19 10:51:57 ragge Exp $ */
/*
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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

/*
 * Simple massbus drive routine.
 * TODO:
 *  Autoconfig new devices 'on the fly'.
 *  More intelligent way to handle different interrupts.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/buf.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/io.h>
#include <machine/pcb.h>

#include <pdp10/dev/ebus.h>
#include <pdp10/dev/rhreg.h>
#include <pdp10/dev/rhvar.h>

#include "ioconf.h"
#include "locators.h"

struct	rhunit rhunit[] = {
	{RH_DT_RP04,	"rp04", MB_RP},
	{RH_DT_RP05,	"rp05", MB_RP},
	{RH_DT_RP06,	"rp06", MB_RP},
	{RH_DT_RP07,	"rp07", MB_RP},
	{RH_DT_RM02,	"rm02", MB_RP},
	{RH_DT_RM03,	"rm03", MB_RP},
	{RH_DT_RM05,	"rm05", MB_RP},
	{RH_DT_RM80,	"rm80", MB_RP},
	{RH_DT_TU45,	"tu45", MB_TU},
	{0,		0,	0}
};

int	rhmatch(struct device *, struct cfdata *, void *);
void	rhattach(struct device *, struct device *, void *);
void	rhintr(void *);
int	rhprint(void *, const char *);
void	rhqueue(struct rh_device *);
void	rhstart(struct rh_softc *);
void	rhmapregs(struct rh_softc *);

struct	cfattach rh_ca = {
	sizeof(struct rh_softc), rhmatch, rhattach
};

#define MBA_WCSR(reg, val) \
	DATAO(sc->sc_ioh | (reg << 30), val)
#define MBA_RCSR(reg) \
	({ int rv; DATAI(sc->sc_ioh | (reg << 30), rv); rv; })

#define	RH_READREG(disk, reg, val) \
	DATAO(sc->sc_ioh, ((reg) << 30) | ((disk) << 18)); \
	DATAI(sc->sc_ioh, val)
#define RH_WRITEREG(disk, reg, val) \
	DATAO(sc->sc_ioh, ((reg) << 30) | ((disk) << 18) | 04000000000 | (val))

/*
 * Look if this is a massbuss adapter.
 */
int
rhmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;
	int rv;

	/* Try a reset */
	CONO(ea->ea_ioh, RH20_CONO_RST|RH20_CONO_RCL);
	/* DELAY(1000); */
	/* Check presence by toggling the ATE bit */
	CONI(ea->ea_ioh, rv);
	if (rv & RH20_CONI_ATE)
		return 0;
	CONO(ea->ea_ioh, RH20_CONO_ATE);
	CONI(ea->ea_ioh, rv);
	if (rv & RH20_CONI_ATE)
		return 1;

	return 0;
}

/*
 * Attach the found massbuss adapter. Setup its interrupt vectors,
 * reset it and go searching for drives on it.
 */
void
rhattach(struct device *parent, struct device *self, void *aux)
{
	struct	rh_softc *sc = (void *)self;
	struct	ebus_attach_args *ea = aux;
	struct	rh_attach_args ma;
	int	i, j, rv;

	printf(": RH20\n");
	sc->sc_iot = ea->ea_iot;
	sc->sc_ioh = ea->ea_ioh;

	CONO(ea->ea_ioh, RH20_CONO_MEN|RH20_CONO_ATE|IPL_BIO);

	sc->sc_first = 0;
	sc->sc_last = (void *)&sc->sc_first;

	RH_WRITEREG(0, RH_IVIR, MAKEIV(IPL_BIO));

	for (i = 0; i < MAXMBADEV; i++) {
		sc->sc_state = SC_AUTOCONF;
		RH_READREG(i, RH_DS, rv);
		if ((rv & RH_DS_DPR) == 0) {
			CONO(sc->sc_ioh, RH20_CONO_RAE|RH20_CONO_ATE|IPL_BIO);
			continue;
		}
		/* We have a drive, ok. */
		ma.ma_unit = i;
		RH_READREG(i, RH_DT, rv);
		ma.ma_type = rv & 0777;
		j = 0;
		while (rhunit[j++].nr)
			if (rhunit[j].nr == ma.ma_type)
				break;
		ma.ma_devtyp = rhunit[j].devtyp;
		ma.ma_name = rhunit[j].name;
		ma.ma_iot = sc->sc_iot;
		ma.ma_ioh = sc->sc_ioh;
		config_found(&sc->sc_dev, (void *)&ma, rhprint);
	}
}

/*
 * We got an interrupt. Check type of interrupt and call the specific
 * device interrupt handling routine.
 */
void
rhintr(void *rh)
{
	struct	rh_softc *sc = rh;
	struct	rh_device *md;
	struct	buf *bp;
	int	itype, attn, anr;

	sc = rh_cd.cd_devs[0];

	CONI(sc->sc_ioh, itype);
	CONO(sc->sc_ioh, itype &
	    (RH20_CONO_DON|RH20_CONO_ATE|RH20_CONO_MEN));
	RH_READREG(0, RH_AS, attn);
	attn &= 0377;
	RH_WRITEREG(0, RH_AS, attn);

	if (sc->sc_state == SC_AUTOCONF)
		return;	/* During autoconfig */

	md = sc->sc_first;
	bp = BUFQ_PEEK(&md->md_q);
	/*
	 * A data-transfer interrupt. Current operation is finished,
	 * call that device's finish routine to see what to do next.
	 */
	if (sc->sc_state == SC_ACTIVE) {

		sc->sc_state = SC_IDLE;
		switch ((*md->md_finish)(md, itype, &attn)) {

		case XFER_FINISH:
			/*
			 * Transfer is finished. Take buffer of drive
			 * queue, and take drive of adapter queue.
			 * If more to transfer, start the adapter again
			 * by calling rhstart().
			 */
			(void)BUFQ_GET(&md->md_q);
			sc->sc_first = md->md_back;
			md->md_back = 0;
			if (sc->sc_first == 0)
				sc->sc_last = (void *)&sc->sc_first;

			if (BUFQ_PEEK(&md->md_q) != NULL) {
				sc->sc_last->md_back = md;
				sc->sc_last = md;
			}
	
			bp->b_resid = 0;
			biodone(bp);
			if (sc->sc_first)
				rhstart(sc);
			break;

		case XFER_RESTART:
			/*
			 * Something went wrong with the transfer. Try again.
			 */
			rhstart(sc);
			break;
		}
	}

	while (attn) {
		anr = ffs(attn) - 1;
		attn &= ~(1 << anr);
		if (sc->sc_md[anr]->md_attn == 0)
			panic("Should check for new MBA device %d", anr);
		(*sc->sc_md[anr]->md_attn)(sc->sc_md[anr]);
	}
}

int
rhprint(void *aux, const char *rhname)
{
	struct  rh_attach_args *ma = aux;

	if (rhname) {
		if (ma->ma_name)
			printf("%s", ma->ma_name);
		else
			printf("device type %o", ma->ma_type);
		printf(" at %s", rhname);
	}
	printf(" drive %d", ma->ma_unit);
	return (ma->ma_name ? UNCONF : UNSUPP);
}

/*
 * A device calls rhqueue() when it wants to get on the adapter queue.
 * Called at splbio(). If the adapter is inactive, start it. 
 */
void
rhqueue(struct rh_device *md)
{
	struct	rh_softc *sc = md->md_rh;
	int	i = (int)sc->sc_first;

	sc->sc_last->md_back = md;
	sc->sc_last = md;

	if (i == 0)
		rhstart(sc);
}

/*
 * Start activity on (idling) adapter. Calls rhmapregs() to setup
 * for dma transfer, then the unit-specific start routine.
 */
void
rhstart(struct rh_softc *sc)
{
	struct	rh_device *md = sc->sc_first;
	struct	buf *bp = BUFQ_PEEK(&md->md_q);
	int ncnt = bp->b_bcount/4;
	int blkcnt = ncnt/128;
	paddr_t pap;

	if (pmap_extract(pmap_kernel(), (vaddr_t)bp->b_data, &pap) == FALSE)
		panic("rhstart");

	if (pap & 03777)
		panic("rhstart: bad align");

	pap >>= 2;

	ept->ept_channel[0][DCH_CCL_OFF] = 
	    DCH_CCW_XFR|DCH_CCW_XHLT| pap | (ncnt << DCH_CCW_CNTSH);

	sc->sc_state = SC_ACTIVE;
	(*md->md_start)(md);		/* machine-dependent start */
	RH_WRITEREG(0, RH_SBAR, md->md_da);
	RH_WRITEREG(0, RH_STCR,
	    md->md_csr | RH_TCR_RCLP|RH_TCR_SES |
	    (((-blkcnt) << RH_TCR_NBCSH) & RH_TCR_NBC));
}
