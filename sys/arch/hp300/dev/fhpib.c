/*	$NetBSD: fhpib.c,v 1.40 2014/03/24 19:42:58 christos Exp $	*/

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
 *	@(#)fhpib.c	8.2 (Berkeley) 1/12/94
 */

/*
 * 98625A/B HPIB driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fhpib.c,v 1.40 2014/03/24 19:42:58 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/intr.h>

#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>

#include <hp300/dev/dmavar.h>

#include <hp300/dev/fhpibreg.h>
#include <hp300/dev/hpibvar.h>

/*
 * Inline version of fhpibwait to be used in places where
 * we don't worry about getting hung.
 */
#define	FHPIBWAIT(hd, m)	while (((hd)->hpib_intr & (m)) == 0) DELAY(1)

#ifdef DEBUG
int	fhpibdebugunit = -1;
int	fhpibdebug = 0;
#define FDB_FAIL	0x01
#define FDB_DMA		0x02
#define FDB_WAIT	0x04
#define FDB_PPOLL	0x08

int	dopriodma = 0;	/* use high priority DMA */
int	doworddma = 1;	/* non-zero if we should attempt word DMA */
int	doppollint = 1;	/* use ppoll interrupts instead of watchdog */
int	fhpibppolldelay = 50;
#endif

static void	fhpibifc(struct fhpibdevice *);
static void	fhpibdmadone(void *);
static int	fhpibwait(struct fhpibdevice *, int);

static void	fhpibreset(struct hpibbus_softc *);
static int	fhpibsend(struct hpibbus_softc *, int, int, void *, int);
static int	fhpibrecv(struct hpibbus_softc *, int, int, void *, int);
static int	fhpibppoll(struct hpibbus_softc *);
static void	fhpibppwatch(void *);
static void	fhpibgo(struct hpibbus_softc *, int, int, void *, int, int,
		    int);
static void	fhpibdone(struct hpibbus_softc *);
static int	fhpibintr(void *);

/*
 * Our controller ops structure.
 */
static struct hpib_controller fhpib_controller = {
	fhpibreset,
	fhpibsend,
	fhpibrecv,
	fhpibppoll,
	fhpibppwatch,
	fhpibgo,
	fhpibdone,
	fhpibintr
};

struct fhpib_softc {
	device_t sc_dev;		/* generic device glue */
	struct fhpibdevice *sc_regs;	/* device registers */
	int	sc_cmd;
	struct hpibbus_softc *sc_hpibbus; /* XXX */
	struct callout sc_dmadone_ch;
	struct callout sc_ppwatch_ch;
};

static int	fhpibmatch(device_t, cfdata_t, void *);
static void	fhpibattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(fhpib, sizeof(struct fhpib_softc),
    fhpibmatch, fhpibattach, NULL, NULL);

static int
fhpibmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_FHPIB)
		return 1;

	return 0;
}

static void
fhpibattach(device_t parent, device_t self, void *aux)
{
	struct fhpib_softc *sc = device_private(self);
	struct dio_attach_args *da = aux;
	struct hpibdev_attach_args ha;
	bus_space_handle_t bsh;

	sc->sc_dev = self;
	if (bus_space_map(da->da_bst, da->da_addr, da->da_size, 0, &bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}
	sc->sc_regs = bus_space_vaddr(da->da_bst, bsh);

	aprint_normal(": %s\n", DIO_DEVICE_DESC_FHPIB);

	/* Establish the interrupt handler. */
	(void)dio_intr_establish(fhpibintr, sc, da->da_ipl, IPL_BIO);

	callout_init(&sc->sc_dmadone_ch, 0);
	callout_init(&sc->sc_ppwatch_ch, 0);

	ha.ha_ops = &fhpib_controller;
	ha.ha_type = HPIBC;			/* XXX */
	ha.ha_ba = HPIBC_BA;
	ha.ha_softcpp = &sc->sc_hpibbus;	/* XXX */
	(void)config_found(self, &ha, hpibdevprint);
}

static void
fhpibreset(struct hpibbus_softc *hs)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;

	hd->hpib_cid = 0xFF;
	DELAY(100);
	hd->hpib_cmd = CT_8BIT;
	hd->hpib_ar = AR_ARONC;
	fhpibifc(hd);
	hd->hpib_ie = IDS_IE;
	hd->hpib_data = C_DCL;
	DELAY(100000);
	/*
	 * See if we can do word DMA.
	 * If so, we should be able to write and read back the appropos bit.
	 */
	hd->hpib_ie |= IDS_WDMA;
	if (hd->hpib_ie & IDS_WDMA) {
		hd->hpib_ie &= ~IDS_WDMA;
		hs->sc_flags |= HPIBF_DMA16;
#ifdef DEBUG
		if (fhpibdebug & FDB_DMA)
			printf("fhpibtype: %s has word DMA\n",
			    device_xname(sc->sc_dev));
#endif
	}
}

static void
fhpibifc(struct fhpibdevice *hd)
{

	hd->hpib_cmd |= CT_IFC;
	hd->hpib_cmd |= CT_INITFIFO;
	DELAY(100);
	hd->hpib_cmd &= ~CT_IFC;
	hd->hpib_cmd |= CT_REN;
	hd->hpib_stat = ST_ATN;
}

static int
fhpibsend(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int origcnt)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	int cnt = origcnt;
	int timo;
	char *addr = ptr;

	hd->hpib_stat = 0;
	hd->hpib_imask = IM_IDLE | IM_ROOM;
	if (fhpibwait(hd, IM_IDLE) < 0)
		goto senderr;
	hd->hpib_stat = ST_ATN;
	hd->hpib_data = C_UNL;
	hd->hpib_data = C_TAG + hs->sc_ba;
	hd->hpib_data = C_LAG + slave;
	if (sec < 0) {
		if (sec == -2)		/* selected device clear KLUDGE */
			hd->hpib_data = C_SDC;
	} else
		hd->hpib_data = C_SCG + sec;
	if (fhpibwait(hd, IM_IDLE) < 0)
		goto senderr;
	if (cnt) {
		hd->hpib_stat = ST_WRITE;
		while (--cnt) {
			hd->hpib_data = *addr++;
			timo = hpibtimeout;
			while ((hd->hpib_intr & IM_ROOM) == 0) {
				if (--timo <= 0)
					goto senderr;
				DELAY(1);
			}
		}
		hd->hpib_stat = ST_EOI;
		hd->hpib_data = *addr;
		FHPIBWAIT(hd, IM_ROOM);
		hd->hpib_stat = ST_ATN;
		/* XXX: HP-UX claims bug with CS80 transparent messages */
		if (sec == 0x12)
			DELAY(150);
		hd->hpib_data = C_UNL;
		(void) fhpibwait(hd, IM_IDLE);
	}
	hd->hpib_imask = 0;
	return origcnt;

senderr:
	hd->hpib_imask = 0;
	fhpibifc(hd);
#ifdef DEBUG
	if (fhpibdebug & FDB_FAIL) {
		printf("%s: fhpibsend failed: slave %d, sec %x, ",
		    device_xname(sc->sc_dev), slave, sec);
		printf("sent %d of %d bytes\n", origcnt-cnt-1, origcnt);
	}
#endif
	return origcnt - cnt - 1;
}

static int
fhpibrecv(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int origcnt)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	int cnt = origcnt;
	int timo;
	char *addr = ptr;

	/*
	 * Slave < 0 implies continuation of a previous receive
	 * that probably timed out.
	 */
	if (slave >= 0) {
		hd->hpib_stat = 0;
		hd->hpib_imask = IM_IDLE | IM_ROOM | IM_BYTE;
		if (fhpibwait(hd, IM_IDLE) < 0)
			goto recverror;
		hd->hpib_stat = ST_ATN;
		hd->hpib_data = C_UNL;
		hd->hpib_data = C_LAG + hs->sc_ba;
		hd->hpib_data = C_TAG + slave;
		if (sec != -1)
			hd->hpib_data = C_SCG + sec;
		if (fhpibwait(hd, IM_IDLE) < 0)
			goto recverror;
		hd->hpib_stat = ST_READ0;
		hd->hpib_data = 0;
	}
	if (cnt) {
		while (--cnt >= 0) {
			timo = hpibtimeout;
			while ((hd->hpib_intr & IM_BYTE) == 0) {
				if (--timo == 0)
					goto recvbyteserror;
				DELAY(1);
			}
			*addr++ = hd->hpib_data;
		}
		FHPIBWAIT(hd, IM_ROOM);
		hd->hpib_stat = ST_ATN;
		hd->hpib_data = (slave == 31) ? C_UNA : C_UNT;
		(void) fhpibwait(hd, IM_IDLE);
	}
	hd->hpib_imask = 0;
	return origcnt;

recverror:
	fhpibifc(hd);
recvbyteserror:
	hd->hpib_imask = 0;
#ifdef DEBUG
	if (fhpibdebug & FDB_FAIL) {
		printf("%s: fhpibrecv failed: slave %d, sec %x, ",
		    device_xname(sc->sc_dev), slave, sec);
		printf("got %d of %d bytes\n", origcnt-cnt-1, origcnt);
	}
#endif
	return origcnt - cnt - 1;
}

static void
fhpibgo(struct hpibbus_softc *hs, int slave, int sec, void *ptr, int count,
    int rw, int timo)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	int i;
	char *addr = ptr;
	int flags = 0;

	hs->sc_flags |= HPIBF_IO;
	if (timo)
		hs->sc_flags |= HPIBF_TIMO;
	if (rw == B_READ)
		hs->sc_flags |= HPIBF_READ;
#ifdef DEBUG
	else if (hs->sc_flags & HPIBF_READ) {
		printf("%s: HPIBF_READ still set\n", __func__);
		hs->sc_flags &= ~HPIBF_READ;
	}
#endif
	hs->sc_count = count;
	hs->sc_addr = addr;
#ifdef DEBUG
	/* fhpibtransfer[unit]++;			XXX */
#endif
	if ((hs->sc_flags & HPIBF_DMA16) &&
	    ((int)addr & 1) == 0 && count && (count & 1) == 0
#ifdef DEBUG
	    && doworddma
#endif
	    ) {
#ifdef DEBUG
		/* fhpibworddma[unit]++;		XXX */
#endif
		flags |= DMAGO_WORD;
		hd->hpib_latch = 0;
	}
#ifdef DEBUG
	if (dopriodma)
		flags |= DMAGO_PRI;
#endif
	if (hs->sc_flags & HPIBF_READ) {
		sc->sc_cmd = CT_REN | CT_8BIT;
		hs->sc_curcnt = count;
		dmago(hs->sc_dq->dq_chan, addr, count, flags|DMAGO_READ);
		if (fhpibrecv(hs, slave, sec, 0, 0) < 0) {
#ifdef DEBUG
			printf("%s: recv failed, retrying...\n", __func__);
#endif
			(void)fhpibrecv(hs, slave, sec, 0, 0);
		}
		i = hd->hpib_cmd;
		hd->hpib_cmd = sc->sc_cmd;
		hd->hpib_ie = IDS_DMA(hs->sc_dq->dq_chan) |
			((flags & DMAGO_WORD) ? IDS_WDMA : 0);
		__USE(i);
		return;
	}
	sc->sc_cmd = CT_REN | CT_8BIT | CT_FIFOSEL;
	if (count < hpibdmathresh) {
#ifdef DEBUG
		/* fhpibnondma[unit]++;			XXX */
		if (flags & DMAGO_WORD)
			/* fhpibworddma[unit]--;	XXX */ ;
#endif
		hs->sc_curcnt = count;
		(void) fhpibsend(hs, slave, sec, addr, count);
		fhpibdone(hs);
		return;
	}
	count -= (flags & DMAGO_WORD) ? 2 : 1;
	hs->sc_curcnt = count;
	dmago(hs->sc_dq->dq_chan, addr, count, flags);
	if (fhpibsend(hs, slave, sec, 0, 0) < 0) {
#ifdef DEBUG
		printf("%s: send failed, retrying...\n", __func__);
#endif
		(void)fhpibsend(hs, slave, sec, 0, 0);
	}
	i = hd->hpib_cmd;
	hd->hpib_cmd = sc->sc_cmd;
	hd->hpib_ie = IDS_DMA(hs->sc_dq->dq_chan) | IDS_WRITE |
		((flags & DMAGO_WORD) ? IDS_WDMA : 0);
	__USE(i);
}

/*
 * A DMA read can finish but the device can still be waiting (MAG-tape
 * with more data than we're waiting for).  This timeout routine
 * takes care of that.  Somehow, the thing gets hosed.  For now, since
 * this should be a very rare occurence, we RESET it.
 */
static void
fhpibdmadone(void *arg)
{
	struct hpibbus_softc *hs = arg;
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	int s = splbio();

	if (hs->sc_flags & HPIBF_IO) {
		struct fhpibdevice *hd = sc->sc_regs;
		struct hpibqueue *hq;

		hd->hpib_imask = 0;
		hd->hpib_cid = 0xFF;
		DELAY(100);
		hd->hpib_cmd = CT_8BIT;
		hd->hpib_ar = AR_ARONC;
		fhpibifc(hd);
		hd->hpib_ie = IDS_IE;
		hs->sc_flags &= ~(HPIBF_DONE|HPIBF_IO|HPIBF_READ|HPIBF_TIMO);
		dmafree(hs->sc_dq);

		hq = TAILQ_FIRST(&hs->sc_queue);
		(hq->hq_intr)(hq->hq_softc);
	}
	splx(s);
}

static void
fhpibdone(struct hpibbus_softc *hs)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	char *addr;
	int cnt;

	cnt = hs->sc_curcnt;
	hs->sc_addr += cnt;
	hs->sc_count -= cnt;
#ifdef DEBUG
	if ((fhpibdebug & FDB_DMA) &&
	    fhpibdebugunit == device_unit(sc->sc_dev))
		printf("%s: addr %p cnt %d\n",
		    __func__, hs->sc_addr, hs->sc_count);
#endif
	if (hs->sc_flags & HPIBF_READ) {
		hd->hpib_imask = IM_IDLE | IM_BYTE;
		if (hs->sc_flags & HPIBF_TIMO)
			callout_reset(&sc->sc_dmadone_ch, hz >> 2,
			    fhpibdmadone, hs);
	} else {
		cnt = hs->sc_count;
		if (cnt) {
			addr = hs->sc_addr;
			hd->hpib_imask = IM_IDLE | IM_ROOM;
			FHPIBWAIT(hd, IM_IDLE);
			hd->hpib_stat = ST_WRITE;
			while (--cnt) {
				hd->hpib_data = *addr++;
				FHPIBWAIT(hd, IM_ROOM);
			}
			hd->hpib_stat = ST_EOI;
			hd->hpib_data = *addr;
		}
		hd->hpib_imask = IM_IDLE;
	}
	hs->sc_flags |= HPIBF_DONE;
	hd->hpib_stat = ST_IENAB;
	hd->hpib_ie = IDS_IE;
}

static int
fhpibintr(void *arg)
{
	struct fhpib_softc *sc = arg;
	struct hpibbus_softc *hs = sc->sc_hpibbus;
	struct fhpibdevice *hd = sc->sc_regs;
	struct hpibqueue *hq;
	int stat0;

	stat0 = hd->hpib_ids;
	if ((stat0 & (IDS_IE|IDS_IR)) != (IDS_IE|IDS_IR)) {
#ifdef DEBUG
		if ((fhpibdebug & FDB_FAIL) && (stat0 & IDS_IR) &&
		    (hs->sc_flags & (HPIBF_IO|HPIBF_DONE)) != HPIBF_IO)
			printf("%s: fhpibintr: bad status %x\n",
			    device_xname(sc->sc_dev), stat0);
		/* fhpibbadint[0]++;			XXX */
#endif
		return 0;
	}
	if ((hs->sc_flags & (HPIBF_IO|HPIBF_DONE)) == HPIBF_IO) {
#ifdef DEBUG
		/* fhpibbadint[1]++;			XXX */
#endif
		return 0;
	}
#ifdef DEBUG
	if ((fhpibdebug & FDB_DMA) &&
	    fhpibdebugunit == device_unit(sc->sc_dev))
		printf("%s: flags %x\n", __func__, hs->sc_flags);
#endif
	hq = TAILQ_FIRST(&hs->sc_queue);
	if (hs->sc_flags & HPIBF_IO) {
		if (hs->sc_flags & HPIBF_TIMO)
			callout_stop(&sc->sc_dmadone_ch);
		stat0 = hd->hpib_cmd;
		hd->hpib_cmd = sc->sc_cmd & ~CT_8BIT;
		hd->hpib_stat = 0;
		hd->hpib_cmd = CT_REN | CT_8BIT;
		stat0 = hd->hpib_intr;
		hd->hpib_imask = 0;
		hs->sc_flags &= ~(HPIBF_DONE|HPIBF_IO|HPIBF_READ|HPIBF_TIMO);
		dmafree(hs->sc_dq);
		(hq->hq_intr)(hq->hq_softc);
	} else if (hs->sc_flags & HPIBF_PPOLL) {
		stat0 = hd->hpib_intr;
#ifdef DEBUG
		if ((fhpibdebug & FDB_FAIL) &&
		    doppollint && (stat0 & IM_PPRESP) == 0)
			printf("%s: fhpibintr: bad intr reg %x\n",
			    device_xname(sc->sc_dev), stat0);
#endif
		hd->hpib_stat = 0;
		hd->hpib_imask = 0;
#ifdef DEBUG
		stat0 = fhpibppoll(hs);
		if ((fhpibdebug & FDB_PPOLL) &&
		    fhpibdebugunit == device_unit(sc->sc_dev))
			printf("%s: got PPOLL status %x\n", __func__, stat0);
		if ((stat0 & (0x80 >> hq->hq_slave)) == 0) {
			/*
			 * XXX give it another shot (68040)
			 */
			/* fhpibppollfail[unit]++;	XXX */
			DELAY(fhpibppolldelay);
			stat0 = fhpibppoll(hs);
			if ((stat0 & (0x80 >> hq->hq_slave)) == 0 &&
			    (fhpibdebug & FDB_PPOLL) &&
			    fhpibdebugunit == device_unit(sc->sc_dev))
				printf("%s: PPOLL: unit %d slave %d stat %x\n",
				    __func__, device_unit(sc->sc_dev),
				    hq->hq_slave, stat0);
		}
#endif
		hs->sc_flags &= ~HPIBF_PPOLL;
		(hq->hq_intr)(hq->hq_softc);
	}
	return 1;
}

static int
fhpibppoll(struct hpibbus_softc *hs)
{
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	int ppoll;

	hd->hpib_stat = 0;
	hd->hpib_psense = 0;
	hd->hpib_pmask = 0xFF;
	hd->hpib_imask = IM_PPRESP | IM_PABORT;
	DELAY(25);
	hd->hpib_intr = IM_PABORT;
	ppoll = hd->hpib_data;
	if (hd->hpib_intr & IM_PABORT)
		ppoll = 0;
	hd->hpib_imask = 0;
	hd->hpib_pmask = 0;
	hd->hpib_stat = ST_IENAB;
	return ppoll;
}

static int
fhpibwait(struct fhpibdevice *hd, int x)
{
	int timo = hpibtimeout;

	while ((hd->hpib_intr & x) == 0 && --timo)
		DELAY(1);
	if (timo == 0) {
#ifdef DEBUG
		if (fhpibdebug & FDB_FAIL)
			printf("%s(%p, %x) timeout\n", __func__, hd, x);
#endif
		return -1;
	}
	return 0;
}

/*
 * XXX: this will have to change if we ever allow more than one
 * pending operation per HP-IB.
 */
static void
fhpibppwatch(void *arg)
{
	struct hpibbus_softc *hs = arg;
	struct fhpib_softc *sc = device_private(device_parent(hs->sc_dev));
	struct fhpibdevice *hd = sc->sc_regs;
	int slave;

	if ((hs->sc_flags & HPIBF_PPOLL) == 0)
		return;
	slave = (0x80 >> TAILQ_FIRST(&hs->sc_queue)->hq_slave);
#ifdef DEBUG
	if (!doppollint) {
		if (fhpibppoll(hs) & slave) {
			hd->hpib_stat = ST_IENAB;
			hd->hpib_imask = IM_IDLE | IM_ROOM;
		} else
			callout_reset(&sc->sc_ppwatch_ch, 1, fhpibppwatch, sc);
		return;
	}
	if ((fhpibdebug & FDB_PPOLL) &&
	    device_unit(sc->sc_dev) == fhpibdebugunit)
		printf("%s: sense request on %s\n",
		    __func__, device_xname(sc->sc_dev));
#endif
	hd->hpib_psense = ~slave;
	hd->hpib_pmask = slave;
	hd->hpib_stat = ST_IENAB;
	hd->hpib_imask = IM_PPRESP | IM_PABORT;
	hd->hpib_ie = IDS_IE;
}
