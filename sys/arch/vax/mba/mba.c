/*	$NetBSD: mba.c,v 1.39.18.1 2017/12/03 11:36:48 jdolecek Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mba.c,v 1.39.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/proc.h>

#include <machine/scb.h>
#include <machine/nexus.h>
#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/sid.h>

#include <vax/mba/mbareg.h>
#include <vax/mba/mbavar.h>

#include "locators.h"

const struct mbaunit mbaunit[] = {
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

void	mbaqueue(struct mba_device *);

static int	mbamatch(device_t, cfdata_t, void *);
static void	mbaattach(device_t, device_t, void *);
static void	mbaintr(void *);
static int	mbaprint(void *, const char *);
static void	mbastart(struct mba_softc *);

CFATTACH_DECL_NEW(mba_cmi, sizeof(struct mba_softc),
    mbamatch, mbaattach, NULL, NULL);

CFATTACH_DECL_NEW(mba_sbi, sizeof(struct mba_softc),
    mbamatch, mbaattach, NULL, NULL);

#define	MBA_WCSR(reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))
#define MBA_RCSR(reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))

/*
 * Look if this is a massbuss adapter.
 */
int
mbamatch(device_t parent, cfdata_t cf, void *aux)
{
	struct sbi_attach_args * const sa = aux;

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
mbaattach(device_t parent, device_t self, void *aux)
{
	struct mba_softc * const sc = device_private(self);
	struct sbi_attach_args * const sa = aux;
	struct mba_attach_args ma;
	int	i, j;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;
	sc->sc_ioh = sa->sa_ioh;
	/*
	 * Set up interrupt vectors for this MBA.
	 */
	for (i = 0x14; i < 0x18; i++)
		scb_vecalloc(vecnum(0, i, sa->sa_nexnum),
		    mbaintr, sc, SCB_ISTACK, &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		device_xname(self), "intr");

	STAILQ_INIT(&sc->sc_xfers);
	MBA_WCSR(MBA_CR, MBACR_INIT);	/* Reset adapter */
	MBA_WCSR(MBA_CR, MBACR_IE);	/* Enable interrupts */

	for (i = 0; i < MAXMBADEV; i++) {
		sc->sc_state = SC_AUTOCONF;
		if ((MBA_RCSR(MUREG(i, MU_DS)) & MBADS_DPR) == 0) 
			continue;
		/* We have a drive, ok. */
		ma.ma_unit = i;
		ma.ma_type = MBA_RCSR(MUREG(i, MU_DT)) & 0xf1ff;
		for (j = 0; mbaunit[j].nr; j++)
			if (mbaunit[j].nr == ma.ma_type)
				break;
		ma.ma_devtyp = mbaunit[j].devtyp;
		ma.ma_name = mbaunit[j].name;
		ma.ma_iot = sc->sc_iot;
		ma.ma_ioh = sc->sc_ioh + MUREG(i, 0);
		config_found(sc->sc_dev, &ma, mbaprint);
	}
}

/*
 * We got an interrupt. Check type of interrupt and call the specific
 * device interrupt handling routine.
 */
void
mbaintr(void *mba)
{
	struct mba_softc * const sc = mba;
	struct mba_device *md;
	struct buf *bp;
	int itype, attn, anr;

	itype = MBA_RCSR(MBA_SR);
	MBA_WCSR(MBA_SR, itype);

	attn = MBA_RCSR(MUREG(0, MU_AS)) & 0xff;
	MBA_WCSR(MUREG(0, MU_AS), attn);

	if (sc->sc_state == SC_AUTOCONF)
		return;	/* During autoconfig */

	md = STAILQ_FIRST(&sc->sc_xfers);
	bp = bufq_peek(md->md_q);
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
			(void)bufq_get(md->md_q);
			STAILQ_REMOVE_HEAD(&sc->sc_xfers, md_link);
			if (bufq_peek(md->md_q) != NULL) {
				STAILQ_INSERT_TAIL(&sc->sc_xfers, md, md_link);
			}
	
			bp->b_resid = 0;
			biodone(bp);
			if (!STAILQ_EMPTY(&sc->sc_xfers))
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
	struct mba_attach_args * const ma = aux;

	if (mbaname) {
		if (ma->ma_name)
			aprint_normal("%s", ma->ma_name);
		else
			aprint_normal("device type %o", ma->ma_type);
		aprint_normal(" at %s", mbaname);
	}
	aprint_normal(" drive %d", ma->ma_unit);
	return (ma->ma_name ? UNCONF : UNSUPP);
}

/*
 * A device calls mbaqueue() when it wants to get on the adapter queue.
 * Called at splbio(). If the adapter is inactive, start it. 
 */
void
mbaqueue(struct mba_device *md)
{
	struct mba_softc * const sc = md->md_mba;
	bool was_empty = STAILQ_EMPTY(&sc->sc_xfers);

	STAILQ_INSERT_TAIL(&sc->sc_xfers, md, md_link);

	if (was_empty)
		mbastart(sc);
}

/*
 * Start activity on (idling) adapter. Calls disk_reallymapin() to setup
 * for DMA transfer, then the unit-specific start routine.
 */
void
mbastart(struct mba_softc *sc)
{
	struct mba_device * const md = STAILQ_FIRST(&sc->sc_xfers);
	struct buf *bp = bufq_peek(md->md_q);

	disk_reallymapin(bp, (void *)(sc->sc_ioh + MAPREG(0)), 0, PG_V);

	sc->sc_state = SC_ACTIVE;
	MBA_WCSR(MBA_VAR, ((u_int)bp->b_data & VAX_PGOFSET));
	MBA_WCSR(MBA_BC, (~bp->b_bcount) + 1);
	(*md->md_start)(md);		/* machine-dependent start */
}
