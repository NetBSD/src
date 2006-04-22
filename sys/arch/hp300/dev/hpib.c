/*	$NetBSD: hpib.c,v 1.31.6.1 2006/04/22 11:37:26 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *	@(#)hpib.c	8.2 (Berkeley) 1/12/94
 */

/*
 * HP-IB bus driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpib.c,v 1.31.6.1 2006/04/22 11:37:26 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <hp300/dev/dmavar.h>

#include <hp300/dev/hpibvar.h>

#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include "ioconf.h"

static int	hpibbusmatch(struct device *, struct cfdata *, void *);
static void	hpibbusattach(struct device *, struct device *, void *);

CFATTACH_DECL(hpibbus, sizeof(struct hpibbus_softc),
    hpibbusmatch, hpibbusattach, NULL, NULL);

static void	hpibbus_attach_children(struct hpibbus_softc *);
static int	hpibbussearch(struct device *, struct cfdata *,
			      const int *, void *);
static int	hpibbusprint(void *, const char *);

static int	hpibbus_alloc(struct hpibbus_softc *, int, int);
#if 0
static void	hpibbus_free(struct hpibbus_softc *, int, int);
#endif

static void	hpibstart(void *);
static void	hpibdone(void *);

int	hpibtimeout = 100000;	/* # of status tests before we give up */
int	hpibidtimeout = 10000;	/* # of status tests for hpibid() calls */
int	hpibdmathresh = 3;	/* byte count beyond which to attempt dma */

/*
 * HP-IB is essentially an IEEE 488 bus, with an HP command
 * set (CS/80 on `newer' devices, Amigo on before-you-were-born
 * devices) thrown on top.  Devices that respond to CS/80 (and
 * probably Amigo, too) are tagged with a 16-bit ID.
 *
 * HP-IB has a 2-level addressing scheme; slave, the analog
 * of a SCSI ID, and punit, the analog of a SCSI LUN.  Unforunately,
 * IDs are on a per-slave basis; punits are often used for disk
 * drives that have an accompanying tape drive on the second punit.
 *
 * In addition, not all HP-IB devices speak CS/80 or Amigo.
 * Examples of such devices are HP-IB plotters, which simply
 * take raw plotter commands over 488.  These devices do not
 * have ID tags, and often the host cannot even tell if such
 * a device is attached to the system!
 *
 * These two nasty bits mean that we have to treat HP-IB as
 * an indirect bus.  However, since we are given some ID
 * information, it is unreasonable to disallow cloning of
 * CS/80 devices.
 *
 * To deal with all of this, we use the semi-twisted scheme
 * in hpibbus_attach_children().  For each HP-IB slave, we loop
 * through all of the possibly-configured children, allowing
 * them to modify the punit parameter (but NOT the slave!).
 *
 * This is evil, but what can you do?
 */

static int
hpibbusmatch(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
hpibbusattach(struct device *parent, struct device *self, void *aux)
{
	struct hpibbus_softc *sc = (struct hpibbus_softc *)self;
	struct hpibdev_attach_args *ha = aux;

	printf("\n");

	/* Get the operations vector for the controller. */
	sc->sc_ops = ha->ha_ops;
	sc->sc_type = ha->ha_type;		/* XXX */
	sc->sc_ba = ha->ha_ba;
	*(ha->ha_softcpp) = sc;			/* XXX */

	hpibreset(device_unit(self));		/* XXX souldn't be here */

	/*
	 * Initialize the DMA queue entry.
	 */
	sc->sc_dq = malloc(sizeof(struct dmaqueue), M_DEVBUF, M_NOWAIT);
	if (sc->sc_dq == NULL) {
		printf("%s: can't allocate DMA queue entry\n", self->dv_xname);
		return;
	}
	sc->sc_dq->dq_softc = sc;
	sc->sc_dq->dq_start = hpibstart;
	sc->sc_dq->dq_done = hpibdone;

	/* Initialize the slave request queue. */
	TAILQ_INIT(&sc->sc_queue);

	/* Attach any devices on the bus. */
	hpibbus_attach_children(sc);
}

static void
hpibbus_attach_children(struct hpibbus_softc *sc)
{
	struct hpibbus_attach_args ha;
	int slave;

	for (slave = 0; slave < 8; slave++) {
		/*
		 * Get the ID tag for the device, if any.
		 * Plotters won't identify themselves, and
		 * get the same value as non-existent devices.
		 */
		ha.ha_id = hpibid(device_unit(&sc->sc_dev), slave);

		ha.ha_slave = slave;	/* not to be modified by children */
		ha.ha_punit = 0;	/* children modify this */

		/*
		 * Search though all configured children for this bus.
		 */
		config_search_ia(hpibbussearch, &sc->sc_dev, "hpibbus", &ha);
	}
}

static int
hpibbussearch(struct device *parent, struct cfdata *cf,
	      const int *ldesc, void *aux)
{
	struct hpibbus_softc *sc = (struct hpibbus_softc *)parent;
	struct hpibbus_attach_args *ha = aux;

	/* Make sure this is in a consistent state. */
	ha->ha_punit = 0;

	if (config_match(parent, cf, ha) > 0) {
		/*
		 * The device probe has succeeded, and filled in
		 * the punit information.  Make sure the configuration
		 * allows for this slave/punit combination.
		 */
		if (cf->hpibbuscf_slave != HPIBBUSCF_SLAVE_DEFAULT &&
		    cf->hpibbuscf_slave != ha->ha_slave)
			goto out;
		if (cf->hpibbuscf_punit != HPIBBUSCF_PUNIT_DEFAULT &&
		    cf->hpibbuscf_punit != ha->ha_punit)
			goto out;

		/*
		 * Allocate the device's address from the bus's
		 * resource map.
		 */
		if (hpibbus_alloc(sc, ha->ha_slave, ha->ha_punit))
			goto out;

		/*
		 * This device is allowed; attach it.
		 */
		config_attach(parent, cf, ha, hpibbusprint);
	}
 out:
	return (0);
}

static int
hpibbusprint(void *aux, const char *pnp)
{
	struct hpibbus_attach_args *ha = aux;

	aprint_normal(" slave %d punit %d", ha->ha_slave, ha->ha_punit);
	return (UNCONF);
}

int
hpibdevprint(void *aux, const char *pnp)
{

	/* only hpibbus's can attach to hpibdev's -- easy. */
	if (pnp != NULL)
		aprint_normal("hpibbus at %s", pnp);
	return (UNCONF);
}

void
hpibreset(int unit)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	(*sc->sc_ops->hpib_reset)(sc);
}

int
hpibreq(struct device *pdev, struct hpibqueue *hq)
{
	struct hpibbus_softc *sc = (struct hpibbus_softc *)pdev;
	int s;

	s = splhigh();	/* XXXthorpej */
	TAILQ_INSERT_TAIL(&sc->sc_queue, hq, hq_list);
	splx(s);

	if (sc->sc_queue.tqh_first == hq)
		return (1);

	return (0);
}

void
hpibfree(struct device *pdev, struct hpibqueue *hq)
{
	struct hpibbus_softc *sc = (struct hpibbus_softc *)pdev;
	int s;

	s = splhigh();	/* XXXthorpej */
	TAILQ_REMOVE(&sc->sc_queue, hq, hq_list);
	splx(s);

	if ((hq = sc->sc_queue.tqh_first) != NULL)
		(*hq->hq_start)(hq->hq_softc);
}

int
hpibid(int unit, int slave)
{
	short id;
	int ohpibtimeout;

	/*
	 * XXX shorten timeout value so autoconfig doesn't
	 * take forever on slow CPUs.
	 */
	ohpibtimeout = hpibtimeout;
	hpibtimeout = hpibidtimeout * (cpuspeed / 8);
	if (hpibrecv(unit, 31, slave, &id, 2) != 2)
		id = 0;
	hpibtimeout = ohpibtimeout;
	return(id);
}

int
hpibsend(int unit, int slave, int sec, void *addr, int cnt)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	return ((*sc->sc_ops->hpib_send)(sc, slave, sec, addr, cnt));
}

int
hpibrecv(int unit, int slave, int sec, void *addr, int cnt)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	return ((*sc->sc_ops->hpib_recv)(sc, slave, sec, addr, cnt));
}

int
hpibpptest(int unit, int slave)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	return ((*sc->sc_ops->hpib_ppoll)(sc) & (0x80 >> slave));
}

void
hpibppclear(int unit)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	sc->sc_flags &= ~HPIBF_PPOLL;
}

void
hpibawait(int unit)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	sc->sc_flags |= HPIBF_PPOLL;
	(*sc->sc_ops->hpib_ppwatch)(sc);
}

int
hpibswait(int unit, int slave)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];
	int timo = hpibtimeout;
	int mask, (*ppoll)(struct hpibbus_softc *);

	ppoll = sc->sc_ops->hpib_ppoll;
	mask = 0x80 >> slave;
	while (((*ppoll)(sc) & mask) == 0) {
		if (--timo == 0) {
			printf("%s: swait timeout\n", sc->sc_dev.dv_xname);
			return(-1);
		}
	}
	return(0);
}

int
hpibustart(int unit)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	if (sc->sc_type == HPIBA)
		sc->sc_dq->dq_chan = DMA0;
	else
		sc->sc_dq->dq_chan = DMA0 | DMA1;
	if (dmareq(sc->sc_dq))
		return(1);
	return(0);
}

static void
hpibstart(void *arg)
{
	struct hpibbus_softc *sc = arg;
	struct hpibqueue *hq;

	hq = sc->sc_queue.tqh_first;
	(*hq->hq_go)(hq->hq_softc);
}

void
hpibgo(int unit, int slave, int sec, void *vbuf, int count, int rw, int timo)
{
	struct hpibbus_softc *sc = hpibbus_cd.cd_devs[unit];

	(*sc->sc_ops->hpib_go)(sc, slave, sec, vbuf, count, rw, timo);
}

static void
hpibdone(void *arg)
{
	struct hpibbus_softc *sc = arg;

	(*sc->sc_ops->hpib_done)(sc);
}

int
hpibintr(void *arg)
{
	struct hpibbus_softc *sc = arg;

	return ((sc->sc_ops->hpib_intr)(arg));
}

static int
hpibbus_alloc(struct hpibbus_softc *sc, int slave, int punit)
{

	if (slave >= HPIB_NSLAVES ||
	    punit >= HPIB_NPUNITS)
		panic("hpibbus_alloc: device address out of range");

	if (sc->sc_rmap[slave][punit] == 0) {
		sc->sc_rmap[slave][punit] = 1;
		return (0);
	}
	return (1);
}

#if 0
static void
hpibbus_free(struct hpibbus_softc *sc, int slave, int punit)
{

	if (slave >= HPIB_NSLAVES ||
	    punit >= HPIB_NPUNITS)
		panic("hpibbus_free: device address out of range");

#ifdef DIAGNOSTIC
	if (sc->sc_rmap[slave][punit] == 0)
		panic("hpibbus_free: not allocated");
#endif

	sc->sc_rmap[slave][punit] = 0;
}
#endif
