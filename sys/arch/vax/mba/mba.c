/*	$NetBSD: mba.c,v 1.22 2000/06/05 00:09:19 matt Exp $ */
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

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/scb.h>
#include <machine/nexus.h>
#include <machine/pte.h>
#include <machine/pcb.h>
#include <machine/sid.h>
#include <machine/cpu.h>

#include <vax/mba/mbareg.h>
#include <vax/mba/mbavar.h>

#include "locators.h"

struct	mbaunit mbaunit[] = {
	{MBADT_RP04,	"rp04", MB_RP},
	{MBADT_RP05,	"rp05", MB_RP},
	{MBADT_RP06,	"rp06", MB_RP},
	{MBADT_RP07,	"rp07", MB_RP},
	{MBADT_RM02,	"rm02", MB_RP},
	{MBADT_RM03,	"rm03", MB_RP},
	{MBADT_RM05,	"rm05", MB_RP},
	{MBADT_RM80,	"rm80", MB_RP},
	{0,		0,	0}
};

int	mbamatch(struct device *, struct cfdata *, void *);
void	mbaattach(struct device *, struct device *, void *);
void	mbaintr(void *);
int	mbaprint(void *, const char *);
void	mbaqueue(struct mba_device *);
void	mbastart(struct mba_softc *);
void	mbamapregs(struct mba_softc *);

struct	cfattach mba_cmi_ca = {
	sizeof(struct mba_softc), mbamatch, mbaattach
};

struct	cfattach mba_sbi_ca = {
	sizeof(struct mba_softc), mbamatch, mbaattach
};

#define	MBA_WCSR(reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))
#define MBA_RCSR(reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))

/*
 * Look if this is a massbuss adapter.
 */
int
mbamatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct	sbi_attach_args *sa = (struct sbi_attach_args *)aux;

	if (vax_cputype == VAX_750) {
		if (cf->cf_loc[CMICF_TR] != CMICF_TR_DEFAULT &&
		    cf->cf_loc[CMICF_TR] != sa->sa_nexnum)
			return 0;
	} else {
		if (cf->cf_loc[SBICF_TR] != SBICF_TR_DEFAULT &&
		    cf->cf_loc[SBICF_TR] != sa->sa_nexnum)
			return 0;
	}

	if (sa->sa_type == NEX_MBA)
		return 1;

	return 0;
}

/*
 * Attach the found massbuss adapter. Setup its interrupt vectors,
 * reset it and go searching for drives on it.
 */
void
mbaattach(struct device *parent, struct device *self, void *aux)
{
	struct	mba_softc *sc = (void *)self;
	struct	sbi_attach_args *sa = (struct sbi_attach_args *)aux;
	struct	mba_attach_args ma;
	int	i, j;

	printf("\n");
	sc->sc_iot = sa->sa_iot;
	sc->sc_ioh = sa->sa_ioh;
	/*
	 * Set up interrupt vectors for this MBA.
	 */
	for (i = 14; i < 18; i++)
		scb_vecalloc(vecnum(0, i, sa->sa_nexnum),
		    mbaintr, sc, SCB_ISTACK, &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		self->dv_xname, "intr");

	sc->sc_first = 0;
	sc->sc_last = (void *)&sc->sc_first;
	MBA_WCSR(MBA_CR, MBACR_INIT);	/* Reset adapter */
	MBA_WCSR(MBA_CR, MBACR_IE);	/* Enable interrupts */

	for (i = 0; i < MAXMBADEV; i++) {
		sc->sc_state = SC_AUTOCONF;
		if ((MBA_RCSR(MUREG(i, MU_DS)) & MBADS_DPR) == 0) 
			continue;
		/* We have a drive, ok. */
		ma.ma_unit = i;
		ma.ma_type = MBA_RCSR(MUREG(i, MU_DT)) & 0777;
		j = 0;
		while (mbaunit[j++].nr)
			if (mbaunit[j].nr == ma.ma_type)
				break;
		ma.ma_devtyp = mbaunit[j].devtyp;
		ma.ma_name = mbaunit[j].name;
		ma.ma_iot = sc->sc_iot;
		ma.ma_ioh = sc->sc_ioh + MUREG(i, 0);
		config_found(&sc->sc_dev, (void *)&ma, mbaprint);
	}
}

/*
 * We got an interrupt. Check type of interrupt and call the specific
 * device interrupt handling routine.
 */
void
mbaintr(void *mba)
{
	struct	mba_softc *sc = mba;
	struct	mba_device *md;
	struct	buf *bp;
	int	itype, attn, anr;

	itype = MBA_RCSR(MBA_SR);
	MBA_WCSR(MBA_SR, itype);

	attn = MBA_RCSR(MUREG(0, MU_AS)) & 0xff;
	MBA_WCSR(MUREG(0, MU_AS), attn);

	if (sc->sc_state == SC_AUTOCONF)
		return;	/* During autoconfig */

	md = sc->sc_first;
	bp = BUFQ_FIRST(&md->md_q);
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
			 * by calling mbastart().
			 */
			BUFQ_REMOVE(&md->md_q, bp);
			sc->sc_first = md->md_back;
			md->md_back = 0;
			if (sc->sc_first == 0)
				sc->sc_last = (void *)&sc->sc_first;

			if (BUFQ_FIRST(&md->md_q) != NULL) {
				sc->sc_last->md_back = md;
				sc->sc_last = md;
			}
	
			bp->b_resid = 0;
			biodone(bp);
			if (sc->sc_first)
				mbastart(sc);
			break;

		case XFER_RESTART:
			/*
			 * Something went wrong with the transfer. Try again.
			 */
			mbastart(sc);
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
mbaprint(void *aux, const char *mbaname)
{
	struct  mba_attach_args *ma = aux;

	if (mbaname) {
		if (ma->ma_name)
			printf("%s", ma->ma_name);
		else
			printf("device type %o", ma->ma_type);
		printf(" at %s", mbaname);
	}
	printf(" drive %d", ma->ma_unit);
	return (ma->ma_name ? UNCONF : UNSUPP);
}

/*
 * A device calls mbaqueue() when it wants to get on the adapter queue.
 * Called at splbio(). If the adapter is inactive, start it. 
 */
void
mbaqueue(struct mba_device *md)
{
	struct	mba_softc *sc = md->md_mba;
	int	i = (int)sc->sc_first;

	sc->sc_last->md_back = md;
	sc->sc_last = md;

	if (i == 0)
		mbastart(sc);
}

/*
 * Start activity on (idling) adapter. Calls mbamapregs() to setup
 * for dma transfer, then the unit-specific start routine.
 */
void
mbastart(struct mba_softc *sc)
{
	struct	mba_device *md = sc->sc_first;
	struct	buf *bp = BUFQ_FIRST(&md->md_q);

	disk_reallymapin(bp, (void *)(sc->sc_ioh + MAPREG(0)), 0, PG_V);

	sc->sc_state = SC_ACTIVE;
	MBA_WCSR(MBA_VAR, ((u_int)bp->b_data & VAX_PGOFSET));
	MBA_WCSR(MBA_BC, (~bp->b_bcount) + 1);
	(*md->md_start)(md);		/* machine-dependent start */
}
