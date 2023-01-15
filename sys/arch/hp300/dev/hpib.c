/*	$NetBSD: hpib.c,v 1.45 2023/01/15 06:19:45 tsutsui Exp $	*/

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
 *	@(#)hpib.c	8.2 (Berkeley) 1/12/94
 */

/*
 * HP-IB bus driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpib.c,v 1.45 2023/01/15 06:19:45 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/device.h>

#include <hp300/dev/dmavar.h>

#include <hp300/dev/hpibvar.h>

#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include "ioconf.h"

static int	hpibbusmatch(device_t, cfdata_t, void *);
static void	hpibbusattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(hpibbus, sizeof(struct hpibbus_softc),
    hpibbusmatch, hpibbusattach, NULL, NULL);

static void	hpibbus_attach_children(struct hpibbus_softc *);
static int	hpibbussubmatch(device_t, cfdata_t, const int *ldesc, void *);
static int	hpibbusprint(void *, const char *);

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
 * * We nevertheless probe the whole (slave, punit) tuple space, since
 * drivers for devices with a unique ID know exactly where to attach;
 * and we disallow ``star'' locators for other drivers.
 */

static int
hpibbusmatch(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
hpibbusattach(device_t parent, device_t self, void *aux)
{
	struct hpibbus_softc *sc = device_private(self);
	struct hpibdev_attach_args *ha = aux;

	sc->sc_dev = self;
	aprint_normal("\n");

	/* Get the operations vector for the controller. */
	sc->sc_ops = ha->ha_ops;
	sc->sc_type = ha->ha_type;		/* XXX */
	sc->sc_ba = ha->ha_ba;
	*(ha->ha_softcpp) = sc;			/* XXX */

	hpibreset(device_unit(self));		/* XXX souldn't be here */

	/*
	 * Initialize the DMA queue entry.
	 */
	sc->sc_dq = kmem_alloc(sizeof(struct dmaqueue), KM_SLEEP);
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
	int id, slave, punit;
	int i;

	for (slave = 0; slave < HPIB_NSLAVES; slave++) {
		/*
		 * Get the ID tag for the device, if any.
		 * Plotters won't identify themselves, and
		 * get the same value as non-existent devices.
		 * However, aging HP-IB drives are slow to respond; try up
		 * to three times to get a valid ID.
		 */
		for (i = 0; i < 3; i++) {
			id = hpibid(device_unit(sc->sc_dev), slave);
			if ((id & 0x200) != 0)
				break;
			delay(10000);
		}

		for (punit = 0; punit < HPIB_NPUNITS; punit++) {
			/*
			 * Search through all configured children for this bus.
			 */
			ha.ha_id = id;
			ha.ha_slave = slave;
			ha.ha_punit = punit;
			config_found(sc->sc_dev, &ha, hpibbusprint,
			    CFARGS(.submatch = hpibbussubmatch));
		}
	}
}

static int
hpibbussubmatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct hpibbus_attach_args *ha = aux;

	if (cf->hpibbuscf_slave != HPIBBUSCF_SLAVE_DEFAULT &&
	    cf->hpibbuscf_slave != ha->ha_slave)
		return 0;
	if (cf->hpibbuscf_punit != HPIBBUSCF_PUNIT_DEFAULT &&
	    cf->hpibbuscf_punit != ha->ha_punit)
		return 0;

	return config_match(parent, cf, aux);
}

static int
hpibbusprint(void *aux, const char *pnp)
{
	struct hpibbus_attach_args *ha = aux;

	if (pnp != NULL) {
		if (ha->ha_id == 0 || ha->ha_punit != 0 /* XXX */)
			return QUIET;
		printf("HP-IB device (id %04X) at %s", ha->ha_id, pnp);
	}
	aprint_normal(" slave %d punit %d", ha->ha_slave, ha->ha_punit);
	return UNCONF;
}

int
hpibdevprint(void *aux, const char *pnp)
{

	/* only hpibbus's can attach to hpibdev's -- easy. */
	if (pnp != NULL)
		aprint_normal("hpibbus at %s", pnp);
	return UNCONF;
}

void
hpibreset(int unit)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	(*sc->sc_ops->hpib_reset)(sc);
}

int
hpibreq(device_t pdev, struct hpibqueue *hq)
{
	struct hpibbus_softc *sc = device_private(pdev);
	int s;

	s = splhigh();	/* XXXthorpej */
	TAILQ_INSERT_TAIL(&sc->sc_queue, hq, hq_list);
	splx(s);

	if (TAILQ_FIRST(&sc->sc_queue) == hq)
		return 1;

	return 0;
}

void
hpibfree(device_t pdev, struct hpibqueue *hq)
{
	struct hpibbus_softc *sc = device_private(pdev);
	int s;

	s = splhigh();	/* XXXthorpej */
	TAILQ_REMOVE(&sc->sc_queue, hq, hq_list);
	splx(s);

	if ((hq = TAILQ_FIRST(&sc->sc_queue)) != NULL)
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
	return id;
}

int
hpibsend(int unit, int slave, int sec, void *addr, int cnt)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	return (*sc->sc_ops->hpib_send)(sc, slave, sec, addr, cnt);
}

int
hpibrecv(int unit, int slave, int sec, void *addr, int cnt)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	return (*sc->sc_ops->hpib_recv)(sc, slave, sec, addr, cnt);
}

int
hpibpptest(int unit, int slave)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	return (*sc->sc_ops->hpib_ppoll)(sc) & (0x80 >> slave);
}

void
hpibppclear(int unit)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	sc->sc_flags &= ~HPIBF_PPOLL;
}

void
hpibawait(int unit)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd, unit);

	sc->sc_flags |= HPIBF_PPOLL;
	(*sc->sc_ops->hpib_ppwatch)(sc);
}

int
hpibswait(int unit, int slave)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd, unit);
	int timo = hpibtimeout;
	int mask, (*ppoll)(struct hpibbus_softc *);

	ppoll = sc->sc_ops->hpib_ppoll;
	mask = 0x80 >> slave;
	while (((*ppoll)(sc) & mask) == 0) {
		if (--timo == 0) {
			printf("%s: swait timeout\n", device_xname(sc->sc_dev));
			return -1;
		}
	}
	return 0;
}

int
hpibustart(int unit)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

	if (sc->sc_type == HPIBA)
		sc->sc_dq->dq_chan = DMA0;
	else
		sc->sc_dq->dq_chan = DMA0 | DMA1;
	if (dmareq(sc->sc_dq))
		return 1;
	return 0;
}

static void
hpibstart(void *arg)
{
	struct hpibbus_softc *sc = arg;
	struct hpibqueue *hq;

	hq = TAILQ_FIRST(&sc->sc_queue);
	(*hq->hq_go)(hq->hq_softc);
}

void
hpibgo(int unit, int slave, int sec, void *vbuf, int count, int rw, int timo)
{
	struct hpibbus_softc *sc = device_lookup_private(&hpibbus_cd,unit);

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

	return (sc->sc_ops->hpib_intr)(arg);
}
