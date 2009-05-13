/*	$NetBSD: if_shmem.c,v 1.6.4.2 2009/05/13 17:23:02 jym Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by The Nokia Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_shmem.c,v 1.6.4.2 2009/05/13 17:23:02 jym Exp $");

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/atomic.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * A virtual ethernet interface which uses shared memory from a
 * memory mapped file as the bus.
 */

static int	shmif_init(struct ifnet *);
static int	shmif_ioctl(struct ifnet *, u_long, void *);
static void	shmif_start(struct ifnet *);
static void	shmif_stop(struct ifnet *, int);

struct shmif_sc {
	struct ethercom sc_ec;
	uint8_t sc_myaddr[6];
	uint8_t *sc_busmem;
	int sc_memfd;
	int sc_kq;

	uint32_t sc_nextpacket;
	uint32_t sc_prevgen;
};
#define IFMEM_LOCK		(0)
#define IFMEM_GENERATION	(8)
#define IFMEM_LASTPACKET	(12)
#define IFMEM_WAKEUP		(16)
#define IFMEM_DATA		(20)

#define BUSCTRL_ATOFF(sc, off)	((uint32_t *)(sc->sc_busmem+(off)))

#define BUSMEM_SIZE 65536 /* enough? */

static void shmif_rcv(void *);

static uint32_t numif;

/*
 * This locking needs work and will misbehave severely if:
 * 1) the backing memory has to be paged in
 * 2) some lockholder exits while holding the lock
 */
static void
lockbus(struct shmif_sc *sc)
{

	__cpu_simple_lock((__cpu_simple_lock_t *)sc->sc_busmem);
}

static void
unlockbus(struct shmif_sc *sc)
{

	__cpu_simple_unlock((__cpu_simple_lock_t *)sc->sc_busmem);
}

static uint32_t
busread(struct shmif_sc *sc, void *dest, uint32_t off, size_t len)
{
	size_t chunk;

	KASSERT(len < (BUSMEM_SIZE - IFMEM_DATA) && off <= BUSMEM_SIZE);
	chunk = MIN(len, BUSMEM_SIZE - off);
	memcpy(dest, sc->sc_busmem + off, chunk);
	len -= chunk;

	if (len == 0)
		return off + chunk;

	/* else, wraps around */
	off = IFMEM_DATA;
	sc->sc_prevgen = *BUSCTRL_ATOFF(sc, IFMEM_GENERATION);

	/* finish reading */
	memcpy((uint8_t *)dest + chunk, sc->sc_busmem + off, len);
	return off + len;
}

static uint32_t
buswrite(struct shmif_sc *sc, uint32_t off, void *data, size_t len)
{
	size_t chunk;

	KASSERT(len < (BUSMEM_SIZE - IFMEM_DATA) && off <= BUSMEM_SIZE);

	chunk = MIN(len, BUSMEM_SIZE - off);
	memcpy(sc->sc_busmem + off, data, chunk);
	len -= chunk;

	if (len == 0)
		return off + chunk;

	DPRINTF(("buswrite wrap: wrote %d bytes to %d, left %d to %d",
	    chunk, off, len, IFMEM_DATA));

	/* else, wraps around */
	off = IFMEM_DATA;
	(*BUSCTRL_ATOFF(sc, IFMEM_GENERATION))++;
	sc->sc_prevgen = *BUSCTRL_ATOFF(sc, IFMEM_GENERATION);

	/* finish writing */
	memcpy(sc->sc_busmem + off, (uint8_t *)data + chunk, len);
	return off + len;
}

static inline uint32_t
advance(uint32_t oldoff, uint32_t delta)
{
	uint32_t newoff;

	newoff = oldoff + delta;
	if (newoff >= BUSMEM_SIZE)
		newoff -= (BUSMEM_SIZE - IFMEM_DATA);
	return newoff;

}

static uint32_t
nextpktoff(struct shmif_sc *sc, uint32_t oldoff)
{
	uint32_t oldlen;

	busread(sc, &oldlen, oldoff, 4);
	KASSERT(oldlen < BUSMEM_SIZE - IFMEM_DATA);

	return advance(oldoff, 4 + oldlen);
}

int rump_shmif_create(const char *, int *); /* XXX */

int
rump_shmif_create(const char *path, int *ifnum)
{
	struct shmif_sc *sc;
	struct ifnet *ifp;
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0xa0, 0x00, 0x00, 0x00, 0x00 };
	uint32_t randnum;
	unsigned mynum;
	int error;

	randnum = arc4random();
	memcpy(&enaddr[2], &randnum, 4);
	mynum = atomic_inc_uint_nv(&numif)-1;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	ifp = &sc->sc_ec.ec_if;
	memcpy(sc->sc_myaddr, enaddr, sizeof(enaddr));

	sc->sc_memfd = rumpuser_open(path, O_RDWR | O_CREAT, &error);
	if (sc->sc_memfd == -1)
		goto fail;
	sc->sc_busmem = rumpuser_filemmap(sc->sc_memfd, 0, BUSMEM_SIZE,
	    1, 1, &error);
	if (error)
		goto fail;

	lockbus(sc);
	if (*BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET) == 0)
		*BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET) = IFMEM_DATA;
	sc->sc_nextpacket = *BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET);
	sc->sc_prevgen = *BUSCTRL_ATOFF(sc, IFMEM_GENERATION);
	unlockbus(sc);

	sc->sc_kq = rumpuser_writewatchfile_setup(-1, sc->sc_memfd, 0, &error);
	if (sc->sc_kq == -1)
		goto fail;

	sprintf(ifp->if_xname, "shmif%d", mynum);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = shmif_init;
	ifp->if_ioctl = shmif_ioctl;
	ifp->if_start = shmif_start;
	ifp->if_stop = shmif_stop;
	ifp->if_mtu = 1518;

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	if (ifnum)
		*ifnum = mynum;
	return 0;

 fail:
	panic("rump_shmemif_create: fixme");
}

static int
shmif_init(struct ifnet *ifp)
{
	int error = 0;

	if (rump_threads) {
		error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
		    shmif_rcv, ifp, NULL, "shmif");
	} else {
		printf("WARNING: threads not enabled, shmif NOT working\n");
	}

	ifp->if_flags |= IFF_RUNNING;
	return error;
}

static int
shmif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, rv;

	s = splnet();
	rv = ether_ioctl(ifp, cmd, data);
	splx(s);

	return rv;
}

/* send everything in-context */
static void
shmif_start(struct ifnet *ifp)
{
	struct shmif_sc *sc = ifp->if_softc;
	struct mbuf *m, *m0;
	uint32_t lastoff, dataoff, npktlenoff;
	uint32_t pktsize = 0;
	bool wrote = false;
	int error;

	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL) {
			break;
		}

		lockbus(sc);
		lastoff = *BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET);

		npktlenoff = nextpktoff(sc, lastoff);
		dataoff = advance(npktlenoff, 4);

		for (m = m0; m != NULL; m = m->m_next) {
			pktsize += m->m_len;
			dataoff = buswrite(sc, dataoff, mtod(m, void *),
			    m->m_len);
		}
		buswrite(sc, npktlenoff, &pktsize, 4);
		*BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET) = npktlenoff;
		unlockbus(sc);

		m_freem(m0);
		wrote = true;

		DPRINTF(("shmif_start: send %d bytes at off %d\n",
		    pktsize, npktlenoff));
	}
	/* wakeup */
	if (wrote)
		rumpuser_pwrite(sc->sc_memfd, &error, 4, IFMEM_WAKEUP, &error);
}

static void
shmif_stop(struct ifnet *ifp, int disable)
{

	panic("%s: unimpl", __func__);
}

static void
shmif_rcv(void *arg)
{
	struct ifnet *ifp = arg;
	struct shmif_sc *sc = ifp->if_softc;
	struct mbuf *m = NULL;
	struct ether_header *eth;
	uint32_t nextpkt, pktlen, lastpkt, busgen, lastnext;
	int error;

	for (;;) {
		if (m == NULL) {
			m = m_gethdr(M_WAIT, MT_DATA);
			MCLGET(m, M_WAIT);
		}

		DPRINTF(("waiting %d/%d\n", sc->sc_nextpacket, sc->sc_prevgen));

		KASSERT(m->m_flags & M_EXT);
		lockbus(sc);
		lastpkt = *BUSCTRL_ATOFF(sc, IFMEM_LASTPACKET);
		busgen = *BUSCTRL_ATOFF(sc, IFMEM_GENERATION);
		lastnext = nextpktoff(sc, lastpkt);
		if ((lastnext > sc->sc_nextpacket && busgen > sc->sc_prevgen)
		    || (busgen > sc->sc_prevgen+1)) {
			nextpkt = lastpkt;
			sc->sc_prevgen = busgen;
			printf("DROPPING\n");
		} else {
			nextpkt = sc->sc_nextpacket;
		}

		/* need more data? */
		if (lastnext == nextpkt && sc->sc_prevgen == busgen){
			unlockbus(sc);
			error = 0;
			rumpuser_writewatchfile_wait(sc->sc_kq, NULL, &error);
			if (__predict_false(error))
				printf("shmif_rcv: wait failed %d\n", error);
			continue;
		}

		busread(sc, &pktlen, nextpkt, 4);
		busread(sc, mtod(m, void *), advance(nextpkt, 4), pktlen);

		DPRINTF(("shmif_rcv: read packet of length %d at %d\n",
		    pktlen, nextpkt));

		sc->sc_nextpacket = nextpktoff(sc, nextpkt);
		sc->sc_prevgen = busgen;
		unlockbus(sc);

		m->m_len = m->m_pkthdr.len = pktlen;
		m->m_pkthdr.rcvif = ifp;

		/* if it's to us, don't pass up and reuse storage space */
		eth = mtod(m, struct ether_header *);
		if (memcmp(eth->ether_shost, sc->sc_myaddr, 6) != 0) {
			ifp->if_input(ifp, m);
			m = NULL;
		}
	}

	panic("shmif_worker is a lazy boy %d\n", error);
}
