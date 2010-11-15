/*	$NetBSD: if_shmem.c,v 1.29 2010/11/15 22:48:06 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_shmem.c,v 1.29 2010/11/15 22:48:06 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
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
#include "rump_net_private.h"

static int shmif_clone(struct if_clone *, int);
static int shmif_unclone(struct ifnet *);

struct if_clone shmif_cloner =
    IF_CLONE_INITIALIZER("shmif", shmif_clone, shmif_unclone);

/*
 * Do r/w prefault for backend pages when attaching the interface.
 * At least logically thinking improves performance (although no
 * mlocking is done, so they might go away).
 */
#define PREFAULT_RW

/*
 * A virtual ethernet interface which uses shared memory from a
 * memory mapped file as the bus.
 */

static int	shmif_init(struct ifnet *);
static int	shmif_ioctl(struct ifnet *, u_long, void *);
static void	shmif_start(struct ifnet *);
static void	shmif_stop(struct ifnet *, int);

#include "shmifvar.h"

struct shmif_sc {
	struct ethercom sc_ec;
	uint8_t sc_myaddr[6];
	struct shmif_mem *sc_busmem;
	int sc_memfd;
	int sc_kq;

	char *sc_backfile;
	size_t sc_backfilelen;

	uint64_t sc_devgen;
	uint32_t sc_nextpacket;
};

static const uint32_t busversion = SHMIF_VERSION;

static void shmif_rcv(void *);

static uint32_t numif;

#define LOCK_UNLOCKED	0
#define LOCK_LOCKED	1
#define LOCK_COOLDOWN	1001

/*
 * This locking needs work and will misbehave severely if:
 * 1) the backing memory has to be paged in
 * 2) some lockholder exits while holding the lock
 */
static void
shmif_lockbus(struct shmif_mem *busmem)
{
	int i = 0;

	while (__predict_false(atomic_cas_32(&busmem->shm_lock,
	    LOCK_UNLOCKED, LOCK_LOCKED) == LOCK_LOCKED)) {
		if (__predict_false(++i > LOCK_COOLDOWN)) {
			uint64_t sec, nsec;
			int error;

			sec = 0;
			nsec = 1000*1000; /* 1ms */
			rumpuser_nanosleep(&sec, &nsec, &error);
			i = 0;
		}
		continue;
	}
	membar_enter();
}

static void
shmif_unlockbus(struct shmif_mem *busmem)
{
	unsigned int old;

	membar_exit();
	old = atomic_swap_32(&busmem->shm_lock, LOCK_UNLOCKED);
	KASSERT(old == LOCK_LOCKED);
}

static int
allocif(int unit, struct shmif_sc **scp)
{
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0xa0, 0x00, 0x00, 0x00, 0x00 };
	struct shmif_sc *sc;
	struct ifnet *ifp;
	uint32_t randnum;
	unsigned mynum = unit;

	randnum = arc4random();
	memcpy(&enaddr[2], &randnum, sizeof(randnum));

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_memfd = -1;

	ifp = &sc->sc_ec.ec_if;
	memcpy(sc->sc_myaddr, enaddr, sizeof(enaddr));

	sprintf(ifp->if_xname, "shmif%d", mynum);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST;
	ifp->if_init = shmif_init;
	ifp->if_ioctl = shmif_ioctl;
	ifp->if_start = shmif_start;
	ifp->if_stop = shmif_stop;
	ifp->if_mtu = ETHERMTU;

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	aprint_verbose("shmif%d: Ethernet address %s\n",
	    mynum, ether_sprintf(enaddr));

	if (scp)
		*scp = sc;

	return 0;
}

static int
initbackend(struct shmif_sc *sc, int memfd)
{
	volatile uint8_t v;
	volatile uint8_t *p;
	int error;

	sc->sc_busmem = rumpuser_filemmap(memfd, 0, BUSMEM_SIZE,
	    RUMPUSER_FILEMMAP_TRUNCATE | RUMPUSER_FILEMMAP_SHARED
	    | RUMPUSER_FILEMMAP_READ | RUMPUSER_FILEMMAP_WRITE, &error);
	if (error)
		return error;

	if (sc->sc_busmem->shm_magic
	    && sc->sc_busmem->shm_magic != SHMIF_MAGIC) {
		printf("bus is not magical");
		rumpuser_unmap(sc->sc_busmem, BUSMEM_SIZE);
		return ENOEXEC; 
	}

	/* Prefault in pages to minimize runtime penalty with buslock */
	for (p = (uint8_t *)sc->sc_busmem;
	    p < (uint8_t *)sc->sc_busmem + BUSMEM_SIZE;
	    p += PAGE_SIZE)
		v = *p;

	shmif_lockbus(sc->sc_busmem);
	/* we're first?  initialize bus */
	if (sc->sc_busmem->shm_magic == 0) {
		sc->sc_busmem->shm_magic = SHMIF_MAGIC;
		sc->sc_busmem->shm_first = BUSMEM_DATASIZE;
	}

	sc->sc_nextpacket = sc->sc_busmem->shm_last;
	sc->sc_devgen = sc->sc_busmem->shm_gen;

#ifdef PREFAULT_RW
	for (p = (uint8_t *)sc->sc_busmem;
	    p < (uint8_t *)sc->sc_busmem + BUSMEM_SIZE;
	    p += PAGE_SIZE) {
		v = *p;
		*p = v;
	}
#endif
	shmif_unlockbus(sc->sc_busmem);

	sc->sc_kq = rumpuser_writewatchfile_setup(-1, memfd, 0, &error);
	if (sc->sc_kq == -1)
		return error;

	sc->sc_memfd = memfd;
	return 0;
}

static void
finibackend(struct shmif_sc *sc)
{
	int dummy;

	kmem_free(sc->sc_backfile, sc->sc_backfilelen);
	sc->sc_backfile = NULL;
	sc->sc_backfilelen = 0;

	rumpuser_unmap(sc->sc_busmem, BUSMEM_SIZE);
	rumpuser_close(sc->sc_memfd, &dummy);
	rumpuser_close(sc->sc_kq, &dummy);
}

int
rump_shmif_create(const char *path, int *ifnum)
{
	struct shmif_sc *sc;
	int mynum, error, memfd, dummy;

	memfd = rumpuser_open(path, O_RDWR | O_CREAT, &error);
	if (memfd == -1)
		return error;

	mynum = atomic_inc_uint_nv(&numif)-1;
	if ((error = allocif(mynum, &sc)) != 0) {
		rumpuser_close(memfd, &dummy);
		return error;
	}
	error = initbackend(sc, memfd);
	if (error) {
		rumpuser_close(memfd, &dummy);
		/* XXX: free sc */
		return error;
	}

	sc->sc_backfilelen = strlen(path)+1;
	sc->sc_backfile = kmem_alloc(sc->sc_backfilelen, KM_SLEEP);
	strcpy(sc->sc_backfile, path);

	if (ifnum)
		*ifnum = mynum;

	return 0;
}

static int
shmif_clone(struct if_clone *ifc, int unit)
{
	int mynum;

	/* not atomic against rump_shmif_create().  so "don't do it". */
	if (unit >= mynum)
		mynum = unit+1;

	return allocif(unit, NULL);
}

static int
shmif_unclone(struct ifnet *ifp)
{

	return EOPNOTSUPP;
}

static int
shmif_init(struct ifnet *ifp)
{
	struct shmif_sc *sc = ifp->if_softc;
	int error = 0;

	if (sc->sc_memfd == -1)
		return ENXIO;

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
	struct shmif_sc *sc = ifp->if_softc;
	struct ifdrv *ifd;
	char *path;
	int s, rv, memfd, dummy;

	s = splnet();
	switch (cmd) {
	case SIOCGLINKSTR:
		ifd = data;

		if (sc->sc_backfilelen == 0) {
			rv = ENOENT;
			break;
		}

		ifd->ifd_len = sc->sc_backfilelen;
		if (ifd->ifd_cmd == IFLINKSTR_QUERYLEN) {
			rv = 0;
			break;
		}

		if (ifd->ifd_cmd != 0) {
			rv = EINVAL;
			break;
		}

		rv = copyoutstr(sc->sc_backfile, ifd->ifd_data,
		    MIN(sc->sc_backfilelen, ifd->ifd_len), NULL);
		break;
	case SIOCSLINKSTR:
		if (ifp->if_flags & IFF_UP) {
			rv = EBUSY;
			break;
		}

		ifd = data;
		if (ifd->ifd_cmd == IFLINKSTR_UNSET) {
			finibackend(sc);
			rv = 0;
			break;
		} else if (ifd->ifd_cmd != 0) {
			rv = EINVAL;
			break;
		} else if (sc->sc_backfile) {
			rv = EBUSY;
			break;
		}

		if (ifd->ifd_len > MAXPATHLEN) {
			rv = E2BIG;
			break;
		} else if (ifd->ifd_len < 1) {
			rv = EINVAL;
			break;
		}

		path = kmem_alloc(ifd->ifd_len, KM_SLEEP);
		rv = copyinstr(ifd->ifd_data, path, ifd->ifd_len, NULL);
		if (rv) {
			kmem_free(path, ifd->ifd_len);
			break;
		}
		memfd = rumpuser_open(path, O_RDWR | O_CREAT, &rv);
		if (memfd == -1) {
			kmem_free(path, ifd->ifd_len);
			break;
		}
		rv = initbackend(sc, memfd);
		if (rv) {
			kmem_free(path, ifd->ifd_len);
			rumpuser_close(memfd, &dummy);
			break;
		}
		sc->sc_backfile = path;
		sc->sc_backfilelen = ifd->ifd_len;

		break;
	default:
		rv = ether_ioctl(ifp, cmd, data);
		if (rv == ENETRESET)
			rv = 0;
		break;
	}
	splx(s);

	return rv;
}

/* send everything in-context */
static void
shmif_start(struct ifnet *ifp)
{
	struct shmif_sc *sc = ifp->if_softc;
	struct shmif_mem *busmem = sc->sc_busmem;
	struct mbuf *m, *m0;
	uint32_t dataoff;
	uint32_t pktsize, pktwrote;
	bool wrote = false;
	bool wrap;
	int error;

	ifp->if_flags |= IFF_OACTIVE;

	for (;;) {
		struct shmif_pkthdr sp;
		struct timeval tv;

		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL) {
			break;
		}

		pktsize = 0;
		for (m = m0; m != NULL; m = m->m_next) {
			pktsize += m->m_len;
		}
		KASSERT(pktsize <= ETHERMTU + ETHER_HDR_LEN);

		getmicrouptime(&tv);
		sp.sp_len = pktsize;
		sp.sp_sec = tv.tv_sec;
		sp.sp_usec = tv.tv_usec;

		shmif_lockbus(busmem);
		KASSERT(busmem->shm_magic == SHMIF_MAGIC);
		busmem->shm_last = shmif_nextpktoff(busmem, busmem->shm_last);

		wrap = false;
		dataoff = shmif_buswrite(busmem,
		    busmem->shm_last, &sp, sizeof(sp), &wrap);
		pktwrote = 0;
		for (m = m0; m != NULL; m = m->m_next) {
			pktwrote += m->m_len;
			dataoff = shmif_buswrite(busmem, dataoff,
			    mtod(m, void *), m->m_len, &wrap);
		}
		KASSERT(pktwrote == pktsize);
		if (wrap) {
			busmem->shm_gen++;
			DPRINTF(("bus generation now %d\n", busmem->shm_gen));
		}
		shmif_unlockbus(busmem);

		m_freem(m0);
		wrote = true;

		DPRINTF(("shmif_start: send %d bytes at off %d\n",
		    pktsize, busmem->shm_last));
	}

	ifp->if_flags &= ~IFF_OACTIVE;

	/* wakeup */
	if (wrote)
		rumpuser_pwrite(sc->sc_memfd,
		    &busversion, sizeof(busversion), IFMEM_WAKEUP, &error);
}

static void
shmif_stop(struct ifnet *ifp, int disable)
{

	panic("%s: unimpl", __func__);
}


/*
 * Check if we have been sleeping too long.  Basically,
 * our in-sc nextpkt must by first <= nextpkt <= last"+1".
 * We use the fact that first is guaranteed to never overlap
 * with the last frame in the ring.
 */
static __inline bool
stillvalid_p(struct shmif_sc *sc)
{
	struct shmif_mem *busmem = sc->sc_busmem;
	unsigned gendiff = busmem->shm_gen - sc->sc_devgen;
	uint32_t lastoff, devoff;

	KASSERT(busmem->shm_first != busmem->shm_last);

	/* normalize onto a 2x busmem chunk */
	devoff = sc->sc_nextpacket;
	lastoff = shmif_nextpktoff(busmem, busmem->shm_last);

	/* trivial case */
	if (gendiff > 1)
		return false;
	KASSERT(gendiff <= 1);

	/* Normalize onto 2x busmem chunk */
	if (busmem->shm_first >= lastoff) {
		lastoff += BUSMEM_DATASIZE;
		if (gendiff == 0)
			devoff += BUSMEM_DATASIZE;
	} else {
		if (gendiff)
			return false;
	}

	return devoff >= busmem->shm_first && devoff <= lastoff;
}

static void
shmif_rcv(void *arg)
{
	struct ifnet *ifp = arg;
	struct shmif_sc *sc = ifp->if_softc;
	struct shmif_mem *busmem = sc->sc_busmem;
	struct mbuf *m = NULL;
	struct ether_header *eth;
	uint32_t nextpkt;
	bool wrap;
	int error;

	for (;;) {
		struct shmif_pkthdr sp;

		if (m == NULL) {
			m = m_gethdr(M_WAIT, MT_DATA);
			MCLGET(m, M_WAIT);
		}

		DPRINTF(("waiting %d/%d\n", sc->sc_nextpacket, sc->sc_devgen));
		KASSERT(m->m_flags & M_EXT);

		shmif_lockbus(busmem);
		KASSERT(busmem->shm_magic == SHMIF_MAGIC);
		KASSERT(busmem->shm_gen >= sc->sc_devgen);

		/* need more data? */
		if (sc->sc_devgen == busmem->shm_gen && 
		    shmif_nextpktoff(busmem, busmem->shm_last)
		     == sc->sc_nextpacket) {
			shmif_unlockbus(busmem);
			error = 0;
			rumpuser_writewatchfile_wait(sc->sc_kq, NULL, &error);
			if (__predict_false(error))
				printf("shmif_rcv: wait failed %d\n", error);
			continue;
		}

		if (stillvalid_p(sc)) {
			nextpkt = sc->sc_nextpacket;
		} else {
			KASSERT(busmem->shm_gen > 0);
			nextpkt = busmem->shm_first;
			if (busmem->shm_first > busmem->shm_last)
				sc->sc_devgen = busmem->shm_gen - 1;
			else
				sc->sc_devgen = busmem->shm_gen;
			DPRINTF(("dev %p overrun, new data: %d/%d\n",
			    sc, nextpkt, sc->sc_devgen));
		}

		/*
		 * If our read pointer is ahead the bus last write, our
		 * generation must be one behind.
		 */
		KASSERT(!(nextpkt > busmem->shm_last
		    && sc->sc_devgen == busmem->shm_gen));

		wrap = false;
		nextpkt = shmif_busread(busmem, &sp,
		    nextpkt, sizeof(sp), &wrap);
		KASSERT(sp.sp_len <= ETHERMTU + ETHER_HDR_LEN);
		nextpkt = shmif_busread(busmem, mtod(m, void *),
		    nextpkt, sp.sp_len, &wrap);

		DPRINTF(("shmif_rcv: read packet of length %d at %d\n",
		    sp.sp_len, nextpkt));

		sc->sc_nextpacket = nextpkt;
		shmif_unlockbus(sc->sc_busmem);

		if (wrap) {
			sc->sc_devgen++;
			DPRINTF(("dev %p generation now %d\n",
			    sc, sc->sc_devgen));
		}

		m->m_len = m->m_pkthdr.len = sp.sp_len;
		m->m_pkthdr.rcvif = ifp;

		/* if it's from us, don't pass up and reuse storage space */
		eth = mtod(m, struct ether_header *);
		if (memcmp(eth->ether_shost, sc->sc_myaddr, 6) != 0) {
			KERNEL_LOCK(1, NULL);
			ifp->if_input(ifp, m);
			KERNEL_UNLOCK_ONE(NULL);
			m = NULL;
		}
	}

	panic("shmif_worker is a lazy boy %d\n", error);
}
