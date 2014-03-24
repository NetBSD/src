/*	$NetBSD: nhpib.c,v 1.41 2014/03/24 19:42:58 christos Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)nhpib.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Internal/98624 HPIB driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nhpib.c,v 1.41 2014/03/24 19:42:58 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hp300/dev/intiovar.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/dmavar.h>

#include <hp300/dev/nhpibreg.h>
#include <hp300/dev/hpibvar.h>

/*
 * ODD parity table for listen and talk addresses and secondary commands.
 * The TI9914A doesn't produce the parity bit.
 */
static const u_char listnr_par[] = {
	0040,0241,0242,0043,0244,0045,0046,0247,
	0250,0051,0052,0253,0054,0255,0256,0057,
	0260,0061,0062,0263,0064,0265,0266,0067,
	0070,0271,0272,0073,0274,0075,0076,0277,
};
static const u_char talker_par[] = {
	0100,0301,0302,0103,0304,0105,0106,0307,
	0310,0111,0112,0313,0114,0315,0316,0117,
	0320,0121,0122,0323,0124,0325,0326,0127,
	0130,0331,0332,0133,0334,0135,0136,0337,
};
static const u_char sec_par[] = {
	0340,0141,0142,0343,0144,0345,0346,0147,
	0150,0351,0352,0153,0354,0155,0156,0357,
	0160,0361,0362,0163,0364,0165,0166,0367,
	0370,0171,0172,0373,0174,0375,0376,0177
};

static void	nhpibifc(struct nhpibdevice *);
static void	nhpibreadtimo(void *);
static int	nhpibwait(struct nhpibdevice *, int);

static void	nhpibreset(struct hpibbus_softc *);
static int	nhpibsend(struct hpibbus_softc *, int, int, void *, int);
static int	nhpibrecv(struct hpibbus_softc *, int, int, void *, int);
static int	nhpibppoll(struct hpibbus_softc *);
static void	nhpibppwatch(void *);
static void	nhpibgo(struct hpibbus_softc *, int, int, void *, int, int,
		    int);
static void	nhpibdone(struct hpibbus_softc *);
static int	nhpibintr(void *);

/*
 * Our controller ops structure.
 */
static struct hpib_controller nhpib_controller = {
	nhpibreset,
	nhpibsend,
	nhpibrecv,
	nhpibppoll,
	nhpibppwatch,
	nhpibgo,
	nhpibdone,
	nhpibintr
};

struct nhpib_softc {
	device_t sc_dev;		/* generic device glue */

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct nhpibdevice *sc_regs;	/* device registers */
	struct hpibbus_softc *sc_hpibbus; /* XXX */

	int sc_myaddr;
	int sc_type;

	struct callout sc_read_ch;
	struct callout sc_ppwatch_ch;
};

static int	nhpib_dio_match(device_t, cfdata_t, void *);
static void	nhpib_dio_attach(device_t, device_t, void *);
static int	nhpib_intio_match(device_t, cfdata_t, void *);
static void	nhpib_intio_attach(device_t, device_t, void *);

static void	nhpib_common_attach(struct nhpib_softc *, const char *);

CFATTACH_DECL_NEW(nhpib_dio, sizeof(struct nhpib_softc),
    nhpib_dio_match, nhpib_dio_attach, NULL, NULL);

CFATTACH_DECL_NEW(nhpib_intio, sizeof(struct nhpib_softc),
    nhpib_intio_match, nhpib_intio_attach, NULL, NULL);

static int
nhpib_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp("hpib", ia->ia_modname) == 0)
		return 1;

	return 0;
}

static int
nhpib_dio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_NHPIB)
		return 1;

	return 0;
}

static void
nhpib_intio_attach(device_t parent, device_t self, void *aux)
{
	struct nhpib_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	bus_space_tag_t bst = ia->ia_bst;
	const char *desc = "internal HP-IB";

	sc->sc_dev = self;
	if (bus_space_map(bst, ia->ia_iobase, INTIO_DEVSIZE, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	sc->sc_bst = bst;
	sc->sc_myaddr = HPIBA_BA;
	sc->sc_type = HPIBA;

	nhpib_common_attach(sc, desc);

	/* establish the interrupt handler */
	(void)intio_intr_establish(nhpibintr, sc, ia->ia_ipl, IPL_BIO);
}

static void
nhpib_dio_attach(device_t parent, device_t self, void *aux)
{
	struct nhpib_softc *sc = device_private(self);
	struct dio_attach_args *da = aux;
	bus_space_tag_t bst = da->da_bst;
	const char *desc = DIO_DEVICE_DESC_NHPIB;

	sc->sc_dev = self;
	if (bus_space_map(bst, da->da_addr, da->da_size, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = bst;
	/* read address off switches */
	sc->sc_myaddr = bus_space_read_1(sc->sc_bst, sc->sc_bsh, 5);
	sc->sc_type = HPIBB;

	nhpib_common_attach(sc, desc);

	/* establish the interrupt handler */
	(void)dio_intr_establish(nhpibintr, sc, da->da_ipl, IPL_BIO);
}

static void
nhpib_common_attach(struct nhpib_softc *sc, const char *desc)
{
	struct hpibdev_attach_args ha;

	aprint_normal(": %s\n", desc);

	sc->sc_regs = bus_space_vaddr(sc->sc_bst, sc->sc_bsh);

	callout_init(&sc->sc_read_ch, 0);
	callout_init(&sc->sc_ppwatch_ch, 0);

	ha.ha_ops = &nhpib_controller;
	ha.ha_type = sc->sc_type;			/* XXX */
	ha.ha_ba = sc->sc_myaddr;
	ha.ha_softcpp = &sc->sc_hpibbus;		/* XXX */
	(void)config_found(sc->sc_dev, &ha, hpibdevprint);
}

static void
nhpibreset(struct hpibbus_softc *hs)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;

	hd->hpib_acr = AUX_SSWRST;
	hd->hpib_ar = hs->sc_ba;
	hd->hpib_lim = LIS_ERR;
	hd->hpib_mim = 0;
	hd->hpib_acr = AUX_CDAI;
	hd->hpib_acr = AUX_CSHDW;
	hd->hpib_acr = AUX_SSTD1;
	hd->hpib_acr = AUX_SVSTD1;
	hd->hpib_acr = AUX_CPP;
	hd->hpib_acr = AUX_CHDFA;
	hd->hpib_acr = AUX_CHDFE;
	hd->hpib_acr = AUX_RHDF;
	hd->hpib_acr = AUX_CSWRST;
	nhpibifc(hd);
	hd->hpib_ie = IDS_IE;
	hd->hpib_data = C_DCL_P;
	DELAY(100000);
}

static void
nhpibifc(struct nhpibdevice *hd)
{

	hd->hpib_acr = AUX_TCA;
	hd->hpib_acr = AUX_CSRE;
	hd->hpib_acr = AUX_SSIC;
	DELAY(100);
	hd->hpib_acr = AUX_CSIC;
	hd->hpib_acr = AUX_SSRE;
}

static int
nhpibsend(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int origcnt)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;
	int cnt = origcnt;
	char *addr = ptr;

	hd->hpib_acr = AUX_TCA;
	hd->hpib_data = C_UNL_P;
	if (nhpibwait(hd, MIS_BO))
		goto senderror;
	hd->hpib_data = talker_par[hs->sc_ba];
	hd->hpib_acr = AUX_STON;
	if (nhpibwait(hd, MIS_BO))
		goto senderror;
	hd->hpib_data = listnr_par[slave];
	if (nhpibwait(hd, MIS_BO))
		goto senderror;
	if (sec >= 0 || sec == -2) {
		if (sec == -2)		/* selected device clear KLUDGE */
			hd->hpib_data = C_SDC_P;
		else
			hd->hpib_data = sec_par[sec];
		if (nhpibwait(hd, MIS_BO))
			goto senderror;
	}
	hd->hpib_acr = AUX_GTS;
	if (cnt) {
		while (--cnt > 0) {
			hd->hpib_data = *addr++;
			if (nhpibwait(hd, MIS_BO))
				goto senderror;
		}
		hd->hpib_acr = AUX_EOI;
		hd->hpib_data = *addr;
		if (nhpibwait(hd, MIS_BO))
			goto senderror;
		hd->hpib_acr = AUX_TCA;
#if 0
		/*
		 * May be causing 345 disks to hang due to interference
		 * with PPOLL mechanism.
		 */
		hd->hpib_data = C_UNL_P;
		(void) nhpibwait(hd, MIS_BO);
#endif
	}
	return origcnt;

senderror:
	nhpibifc(hd);
	return origcnt - cnt - 1;
}

static int
nhpibrecv(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int origcnt)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;
	int cnt = origcnt;
	char *addr = ptr;

	/*
	 * Slave < 0 implies continuation of a previous receive
	 * that probably timed out.
	 */
	if (slave >= 0) {
		hd->hpib_acr = AUX_TCA;
		hd->hpib_data = C_UNL_P;
		if (nhpibwait(hd, MIS_BO))
			goto recverror;
		hd->hpib_data = listnr_par[hs->sc_ba];
		hd->hpib_acr = AUX_SLON;
		if (nhpibwait(hd, MIS_BO))
			goto recverror;
		hd->hpib_data = talker_par[slave];
		if (nhpibwait(hd, MIS_BO))
			goto recverror;
		if (sec >= 0) {
			hd->hpib_data = sec_par[sec];
			if (nhpibwait(hd, MIS_BO))
				goto recverror;
		}
		hd->hpib_acr = AUX_RHDF;
		hd->hpib_acr = AUX_GTS;
	}
	if (cnt) {
		while (--cnt >= 0) {
			if (nhpibwait(hd, MIS_BI))
				goto recvbyteserror;
			*addr++ = hd->hpib_data;
		}
		hd->hpib_acr = AUX_TCA;
		hd->hpib_data = (slave == 31) ? C_UNA_P : C_UNT_P;
		(void) nhpibwait(hd, MIS_BO);
	}
	return origcnt;

recverror:
	nhpibifc(hd);
recvbyteserror:
	return origcnt - cnt - 1;
}

static void
nhpibgo(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int count,
    int rw, int timo)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;
	char *addr = ptr;

	hs->sc_flags |= HPIBF_IO;
	if (timo)
		hs->sc_flags |= HPIBF_TIMO;
	if (rw == B_READ)
		hs->sc_flags |= HPIBF_READ;
#ifdef DEBUG
	else if (hs->sc_flags & HPIBF_READ) {
		printf("nhpibgo: HPIBF_READ still set\n");
		hs->sc_flags &= ~HPIBF_READ;
	}
#endif
	hs->sc_count = count;
	hs->sc_addr = addr;
	if (hs->sc_flags & HPIBF_READ) {
		hs->sc_curcnt = count;
		dmago(hs->sc_dq->dq_chan, addr, count, DMAGO_BYTE|DMAGO_READ);
		nhpibrecv(hs, slave, sec, 0, 0);
		hd->hpib_mim = MIS_END;
	} else {
		hd->hpib_mim = 0;
		if (count < hpibdmathresh) {
			hs->sc_curcnt = count;
			nhpibsend(hs, slave, sec, addr, count);
			nhpibdone(hs);
			return;
		}
		hs->sc_curcnt = --count;
		dmago(hs->sc_dq->dq_chan, addr, count, DMAGO_BYTE);
		nhpibsend(hs, slave, sec, 0, 0);
	}
	hd->hpib_ie = IDS_IE | IDS_DMA(hs->sc_dq->dq_chan);
}

/*
 * This timeout can only happen if a DMA read finishes DMAing with the read
 * still pending (more data in read transaction than the driver was prepared
 * to accept).  At the moment, variable-record tape drives are the only things
 * capabale of doing this.  We repeat the necessary code from nhpibintr() -
 * easier and quicker than calling nhpibintr() for this special case.
 */
static void
nhpibreadtimo(void *arg)
{
	struct hpibbus_softc *hs = arg;
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	int s = splbio();

	if (hs->sc_flags & HPIBF_IO) {
		struct nhpibdevice *hd = sc->sc_regs;
		struct hpibqueue *hq;

		hd->hpib_mim = 0;
		hd->hpib_acr = AUX_TCA;
		hs->sc_flags &= ~(HPIBF_DONE|HPIBF_IO|HPIBF_READ|HPIBF_TIMO);
		dmafree(hs->sc_dq);

		hq = TAILQ_FIRST(&hs->sc_queue);
		(hq->hq_intr)(hq->hq_softc);
	}
	splx(s);
}

static void
nhpibdone(struct hpibbus_softc *hs)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;
	int cnt;

	cnt = hs->sc_curcnt;
	hs->sc_addr += cnt;
	hs->sc_count -= cnt;
	hs->sc_flags |= HPIBF_DONE;
	hd->hpib_ie = IDS_IE;
	if (hs->sc_flags & HPIBF_READ) {
		if ((hs->sc_flags & HPIBF_TIMO) &&
		    (hd->hpib_ids & IDS_IR) == 0)
			callout_reset(&sc->sc_read_ch, hz >> 2,
			    nhpibreadtimo, hs);
	} else {
		if (hs->sc_count == 1) {
			(void) nhpibwait(hd, MIS_BO);
			hd->hpib_acr = AUX_EOI;
			hd->hpib_data = *hs->sc_addr;
			hd->hpib_mim = MIS_BO;
		}
#ifdef DEBUG
		else if (hs->sc_count)
			panic("nhpibdone");
#endif
	}
}

static int
nhpibintr(void *arg)
{
	struct nhpib_softc *sc = arg;
	struct hpibbus_softc *hs = sc->sc_hpibbus;
	struct nhpibdevice *hd = sc->sc_regs;
	struct hpibqueue *hq;
	int stat0;
	int stat1;

	if ((hd->hpib_ids & IDS_IR) == 0)
		return 0;
	stat0 = hd->hpib_mis;
	stat1 = hd->hpib_lis;
	__USE(stat1);

	hq = TAILQ_FIRST(&hs->sc_queue);

	if (hs->sc_flags & HPIBF_IO) {
		hd->hpib_mim = 0;
		if ((hs->sc_flags & HPIBF_DONE) == 0) {
			hs->sc_flags &= ~HPIBF_TIMO;
			dmastop(hs->sc_dq->dq_chan);
		} else if (hs->sc_flags & HPIBF_TIMO)
			callout_stop(&sc->sc_read_ch);
		hd->hpib_acr = AUX_TCA;
		hs->sc_flags &= ~(HPIBF_DONE|HPIBF_IO|HPIBF_READ|HPIBF_TIMO);

		dmafree(hs->sc_dq);
		(hq->hq_intr)(hq->hq_softc);
	} else if (hs->sc_flags & HPIBF_PPOLL) {
		hd->hpib_mim = 0;
		stat0 = nhpibppoll(hs);
		if (stat0 & (0x80 >> hq->hq_slave)) {
			hs->sc_flags &= ~HPIBF_PPOLL;
			(hq->hq_intr)(hq->hq_softc);
		}
#ifdef DEBUG
		else
			printf("%s: PPOLL intr bad status %x\n",
			    device_xname(hs->sc_dev), stat0);
#endif
	}
	return 1;
}

static int
nhpibppoll(struct hpibbus_softc *hs)
{
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct nhpibdevice *hd = sc->sc_regs;
	int ppoll;

	hd->hpib_acr = AUX_SPP;
	DELAY(25);
	ppoll = hd->hpib_cpt;
	hd->hpib_acr = AUX_CPP;
	return ppoll;
}

#ifdef DEBUG
int nhpibreporttimo = 0;
#endif

static int
nhpibwait(struct nhpibdevice *hd, int x)
{
	int timo = hpibtimeout;

	while ((hd->hpib_mis & x) == 0 && --timo)
		DELAY(1);
	if (timo == 0) {
#ifdef DEBUG
		if (nhpibreporttimo)
			printf("hpib0: %s timo\n", x==MIS_BO?"OUT":"IN");
#endif
		return -1;
	}
	return 0;
}

static void
nhpibppwatch(void *arg)
{
	struct hpibbus_softc *hs = arg;
	struct nhpib_softc *sc = device_private(device_parent(hs->sc_dev));

	if ((hs->sc_flags & HPIBF_PPOLL) == 0)
		return;
again:
	if (nhpibppoll(hs) & (0x80 >> TAILQ_FIRST(&hs->sc_queue)->hq_slave))
		sc->sc_regs->hpib_mim = MIS_BO;
	else if (cold)
		/* timeouts not working yet */
		goto again;
	else
		callout_reset(&sc->sc_ppwatch_ch, 1, nhpibppwatch, hs);
}
