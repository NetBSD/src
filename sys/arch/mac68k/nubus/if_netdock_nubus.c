/*	$NetBSD: if_netdock_nubus.c,v 1.10 2005/12/24 20:07:15 perry Exp $	*/

/*
 * Copyright (C) 2000,2002 Daishi Kato <daishi@axlight.com>
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
 *      This product includes software developed by Daishi Kato
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Asante NetDock (for Duo series) driver
 * the chip inside is not known
 */

/*
 * The author would like to thank Takeo Kuwata <tkuwata@mac.com> for
 * his help in stabilizing this driver.
 */

/***********************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_netdock_nubus.c,v 1.10 2005/12/24 20:07:15 perry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#include "opt_inet.h"
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/viareg.h>
#include <mac68k/nubus/nubus.h>

/***********************/

#define NETDOCK_DEBUG

#define NETDOCK_NUBUS_CATEGORY	0x0020
#define NETDOCK_NUBUS_TYPE	0x0003
#define NETDOCK_NUBUS_DRSW	0x0103
#define NETDOCK_NUBUS_DRHW	0x0100

#define ETHERMICRODOCK_NUBUS_CATEGORY	0x0020
#define ETHERMICRODOCK_NUBUS_TYPE	0x0003
#define ETHERMICRODOCK_NUBUS_DRSW	0x0102
#define ETHERMICRODOCK_NUBUS_DRHW	0x0100

#define REG_ISR		0x000c
#define REG_000E	0x000e
#define REG_0000	0x0000
#define REG_0002	0x0002
#define REG_0004	0x0004
#define REG_0006	0x0006
#define REG_DATA	0x0008
#define REG_EFD00	0xefd00

#define ISR_ALL		0x3300	
#define ISR_TX		0x0200
#define ISR_RX		0x0100
#define ISR_READY	0x0800
#define ISR_BIT_0C	0x1000
#define ISR_BIT_0D	0x2000
#define ISR_MASK	0x0033
#define ISR_BIT_03	0x0008

#define REG_0002_BIT_04	0x0010
#define REG_0000_BIT_08	0x0100
#define REG_0004_BIT_0F	0x8000
#define REG_0004_BIT_07 0x0080
#define REG_DATA_BIT_02 0x0004
#define REG_DATA_BIT_03 0x0008
#define REG_DATA_BIT_04 0x0010
#define REG_DATA_BIT_05 0x0020
#define REG_DATA_BIT_08	0x0100
#define REG_DATA_BIT_07	0x0080
#define REG_DATA_BIT_0F	0x8000

/***********************/

typedef struct netdock_softc {
	struct device	sc_dev;
	struct ethercom	sc_ethercom;
#define sc_if		sc_ethercom.ec_if

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	u_int8_t	sc_enaddr[ETHER_ADDR_LEN];

} netdock_softc_t;

/***********************/

static int	netdock_nubus_match(struct device *, struct cfdata *, void *);
static void	netdock_nubus_attach(struct device *, struct device *, void *);
static int	netdock_nb_get_enaddr(bus_space_tag_t, bus_space_handle_t,
			struct nubus_attach_args *, u_int8_t *);
#ifdef NETDOCK_DEBUG_DRIVER
static void	netdock_print_driver(bus_space_tag_t, bus_space_handle_t,
			struct nubus_attach_args *);
#endif

int	netdock_setup(struct netdock_softc *, u_int8_t *);
void	netdock_intr(void *);

static void	netdock_watchdog(struct ifnet *);
static int	netdock_init(struct netdock_softc *);
static int	netdock_stop(struct netdock_softc *);
static int	netdock_ioctl(struct ifnet *, u_long, caddr_t);
static void	netdock_start(struct ifnet *);
static void	netdock_reset(struct netdock_softc *);
static void	netdock_txint(struct netdock_softc *);
static void	netdock_rxint(struct netdock_softc *);

static u_int	netdock_put(struct netdock_softc *, struct mbuf *);
static int	netdock_read(struct netdock_softc *, int);
static struct mbuf *netdock_get(struct netdock_softc *, int);

/***********************/

#define NIC_GET_1(sc, o)	(bus_space_read_1((sc)->sc_regt,	\
					(sc)->sc_regh, (o)))
#define NIC_PUT_1(sc, o, val)	(bus_space_write_1((sc)->sc_regt,	\
					(sc)->sc_regh, (o), (val)))
#define NIC_GET_2(sc, o)	(bus_space_read_2((sc)->sc_regt,	\
					(sc)->sc_regh, (o)))
#define NIC_PUT_2(sc, o, val)	(bus_space_write_2((sc)->sc_regt,	\
					(sc)->sc_regh, (o), (val)))
#define NIC_GET_4(sc, o)	(bus_space_read_4((sc)->sc_regt,	\
					(sc)->sc_regh, (o)))
#define NIC_PUT_4(sc, o, val)	(bus_space_write_4((sc)->sc_regt,	\
					(sc)->sc_regh, (o), (val)))

#define NIC_BSET(sc, o, b)						\
	__asm__ volatile("bset %0,%1" : : "di" ((u_short)(b)),	\
		"g" (*(u_int8_t *)((sc)->sc_regh.base + (o))))
#define NIC_BCLR(sc, o, b)						\
	__asm__ volatile("bclr %0,%1" : : "di" ((u_short)(b)),	\
		"g" (*(u_int8_t *)((sc)->sc_regh.base + (o))))
#define NIC_ANDW(sc, o, b)						\
	__asm__ volatile("andw %0,%1" : : "di" ((u_short)(b)),	\
		"g" (*(u_int8_t *)((sc)->sc_regh.base + (o))))
#define NIC_ORW(sc, o, b)						\
	__asm__ volatile("orw %0,%1" : : "di" ((u_short)(b)),	\
		"g" (*(u_int8_t *)((sc)->sc_regh.base + (o))))


/***********************/

CFATTACH_DECL(netdock_nubus, sizeof(struct netdock_softc),
    netdock_nubus_match, netdock_nubus_attach, NULL, NULL);

/***********************/

static int
netdock_nubus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct nubus_attach_args *na = (struct nubus_attach_args *)aux;
	bus_space_handle_t bsh;
	int rv;

	if (bus_space_map(na->na_tag, NUBUS_SLOT2PA(na->slot),
	    NBMEMSIZE, 0, &bsh))
		return (0);

	rv = 0;

	if (na->category == NETDOCK_NUBUS_CATEGORY &&
	    na->type == NETDOCK_NUBUS_TYPE &&
	    na->drsw == NETDOCK_NUBUS_DRSW) {
		/* assuming this IS Asante NetDock */
		rv = 1;
	}

	if (na->category == ETHERMICRODOCK_NUBUS_CATEGORY &&
	    na->type == ETHERMICRODOCK_NUBUS_TYPE &&
	    na->drsw == ETHERMICRODOCK_NUBUS_DRSW) {
		/* assuming this IS Newer EtherMicroDock */
		rv = 1;
	}

	bus_space_unmap(na->na_tag, bsh, NBMEMSIZE);

	return rv;
}

static void
netdock_nubus_attach(struct device *parent, struct device *self, void *aux)
{
	struct netdock_softc *sc = (struct netdock_softc *)self;
	struct nubus_attach_args *na = (struct nubus_attach_args *)aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	u_int8_t enaddr[ETHER_ADDR_LEN];
	const char *cardtype;

	bst = na->na_tag;
	if (bus_space_map(bst, NUBUS_SLOT2PA(na->slot), NBMEMSIZE, 0, &bsh)) {
		printf(": failed to map memory space.\n");
		return;
	}

	sc->sc_regt = bst;
	cardtype = nubus_get_card_name(bst, bsh, na->fmt);

#ifdef NETDOCK_DEBUG_DRIVER
	netdock_print_driver(bst, bsh, na);
#endif

	if (netdock_nb_get_enaddr(bst, bsh, na, enaddr)) {
		printf(": can't find MAC address.\n");
		bus_space_unmap(bst, bsh, NBMEMSIZE);
		return;
	}

	if (bus_space_subregion(bst, bsh, 0xe00300, 0xf0000, &sc->sc_regh)) {
		printf(": failed to map register space.\n");
		bus_space_unmap(bst, bsh, NBMEMSIZE);
		return;
	}

	printf(": %s\n", cardtype);

	if (netdock_setup(sc, enaddr)) {
		bus_space_unmap(bst, bsh, NBMEMSIZE);
		return;
	}

	add_nubus_intr(na->slot, netdock_intr, (void *)sc);

	return;
}

static int
netdock_nb_get_enaddr(bus_space_tag_t bst, bus_space_handle_t bsh,
    struct nubus_attach_args *na, u_int8_t *ep)
{
	nubus_dir dir;
	nubus_dirent dirent;

	/*
	 * these hardwired resource IDs are only for NetDock
	 */
	nubus_get_main_dir(na->fmt, &dir);
	if (nubus_find_rsrc(bst, bsh, na->fmt, &dir, 0x81, &dirent) <= 0)
		return 1;
	nubus_get_dir_from_rsrc(na->fmt, &dirent, &dir);
	if (nubus_find_rsrc(bst, bsh, na->fmt, &dir, 0x80, &dirent) <= 0)
		return 1;
	if (nubus_get_ind_data(bst, bsh, na->fmt, &dirent,
		ep, ETHER_ADDR_LEN) <= 0)
		return 1;

	return 0;
}

#ifdef NETDOCK_DEBUG_DRIVER
static void
netdock_print_driver(bus_space_tag_t bst, bus_space_handle_t bsh,
    struct nubus_attach_args *na)
{
#define HEADSIZE	(8+4)
#define CODESIZE	(6759-4)
	unsigned char mydata[HEADSIZE + CODESIZE];
	nubus_dir dir;
	nubus_dirent dirent;
	int i, rv;

	nubus_get_main_dir(na->fmt, &dir);
	rv = nubus_find_rsrc(bst, bsh, na->fmt, &dir, 0x81, &dirent);
	if (rv <= 0) {
		printf(": can't find sResource.\n");
		return;
	}
	nubus_get_dir_from_rsrc(na->fmt, &dirent, &dir);
	if (nubus_find_rsrc(bst, bsh, na->fmt, &dir, NUBUS_RSRC_DRVRDIR,
	    &dirent) <= 0) {
		printf(": can't find sResource.\n");
		return;
	}
	if (nubus_get_ind_data(bst, bsh, na->fmt,
		&dirent, mydata, HEADSIZE+CODESIZE) <= 0) {
		printf(": can't find indirect data.\n");
		return;
	}
	printf("\n########## begin driver dir");
	for (i = 0; i < HEADSIZE; i++) {
		if (i % 16 == 0)
			printf("\n%02x:",i);
		printf(" %02x", mydata[i]);
	}
	printf("\n########## begin driver code");
	for (i = 0; i < CODESIZE; i++) {
		if (i % 16 == 0)
			printf("\n%02x:",i);
		printf(" %02x", mydata[i + HEADSIZE]);
	}
#if 0
	printf("\n########## begin driver code (partial)\n");
#define PARTSIZE 256
#define OFFSET 0x1568
	for (i = OFFSET; i < OFFSET + PARTSIZE; i++) {
		if ((i - OFFSET) % 16 == 0)
			printf("\n%02x:",i);
		printf(" %02x", mydata[i + HEADSIZE]);
	}
#endif
	printf("\n########## end\n");
}
#endif


int
netdock_setup(struct netdock_softc *sc, u_int8_t *lladdr)
{
	struct ifnet *ifp = &sc->sc_if;

	memcpy(sc->sc_enaddr, lladdr, ETHER_ADDR_LEN);
	printf("%s: Ethernet address %s\n",
	    sc->sc_dev.dv_xname, ether_sprintf(lladdr));

	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_ioctl = netdock_ioctl;
	ifp->if_start = netdock_start;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	ifp->if_watchdog = netdock_watchdog;

	if_attach(ifp);
	ether_ifattach(ifp, lladdr);

	return (0);
}

static int
netdock_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifaddr *ifa;
	struct ifreq *ifr;
	struct netdock_softc *sc = ifp->if_softc;
	int s = splnet();
	int err = 0;
	int temp;

	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			(void)netdock_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			(void)netdock_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			netdock_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			(void)netdock_init(sc);
		} else {
			temp = ifp->if_flags & IFF_UP;
			netdock_reset(sc);
			ifp->if_flags |= temp;
			netdock_start(ifp);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		if (cmd == SIOCADDMULTI)
			err = ether_addmulti(ifr, &sc->sc_ethercom);
		else
			err = ether_delmulti(ifr, &sc->sc_ethercom);

		if (err == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				temp = ifp->if_flags & IFF_UP;
				netdock_reset(sc);
				ifp->if_flags |= temp;
			}
			err = 0;
		}
		break;
	default:
		err = EINVAL;
		break;
	}
	splx(s);
	return (err);
}

static void
netdock_start(struct ifnet *ifp)
{
	struct netdock_softc *sc = ifp->if_softc;
	struct mbuf *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (1) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			return;

		if ((m->m_flags & M_PKTHDR) == 0)
			panic("%s: netdock_start: no header mbuf",
				sc->sc_dev.dv_xname);

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		if ((netdock_put(sc, m)) == 0) {
			IF_PREPEND(&ifp->if_snd, m);
			return;
		}

		ifp->if_opackets++;
	}

}

static void
netdock_reset(struct netdock_softc *sc)
{

	netdock_stop(sc);
	netdock_init(sc);
}

static int
netdock_init(struct netdock_softc *sc)
{
	int s;
	int saveisr;
	int savetmp;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		return (0);

	s = splnet();

	/* 0606 */
	NIC_PUT_2(sc, REG_000E, 0x0200);
	NIC_PUT_2(sc, REG_ISR , 0);
	NIC_PUT_2(sc, REG_000E, 0);

	NIC_PUT_2(sc, REG_0000, 0x8104);
	NIC_PUT_2(sc, REG_0004, 0x0043);
	NIC_PUT_2(sc, REG_000E, 0x0100);
	NIC_PUT_2(sc, REG_0002, 0x6618);
	NIC_PUT_2(sc, REG_0000, 0x8010);

	NIC_PUT_1(sc, REG_0004 + 0, sc->sc_enaddr[0]);
	NIC_PUT_1(sc, REG_0004 + 1, sc->sc_enaddr[1]);
	NIC_PUT_1(sc, REG_0004 + 2, sc->sc_enaddr[2]);
	NIC_PUT_1(sc, REG_0004 + 3, sc->sc_enaddr[3]);
	NIC_PUT_1(sc, REG_0004 + 4, sc->sc_enaddr[4]);
	NIC_PUT_1(sc, REG_0004 + 5, sc->sc_enaddr[5]);

	NIC_PUT_2(sc, REG_ISR , 0x2008);
	NIC_PUT_2(sc, REG_000E, 0x0200);
	NIC_PUT_2(sc, REG_0000, 0x4000);
	NIC_PUT_2(sc, REG_ISR, ISR_MASK);


	/* 1320 */
	saveisr = NIC_GET_2(sc, REG_ISR);
	NIC_PUT_2(sc, REG_ISR , 0);
	savetmp = NIC_GET_2(sc, REG_000E);
	NIC_PUT_2(sc, REG_000E, 0x0100);
	NIC_ANDW(sc, REG_ISR, ~ISR_BIT_03);
	NIC_PUT_2(sc, REG_000E, savetmp);

	/* 1382 */
	savetmp = NIC_GET_2(sc, REG_000E);
	NIC_PUT_2(sc, REG_000E, 0x0100);
	NIC_ORW(sc, REG_ISR, ISR_BIT_03); 
	NIC_PUT_2(sc, REG_000E, savetmp);
	NIC_PUT_2(sc, REG_ISR , saveisr);


	sc->sc_if.if_flags |= IFF_RUNNING;
	sc->sc_if.if_flags &= ~IFF_OACTIVE;

	splx(s);
	return (0);
}

static int
netdock_stop(struct netdock_softc *sc)
{
	int s = splnet();

	sc->sc_if.if_timer = 0;
	sc->sc_if.if_flags &= ~(IFF_RUNNING | IFF_UP);

	splx(s);
	return (0);
}

static void
netdock_watchdog(struct ifnet *ifp)
{
	struct netdock_softc *sc = ifp->if_softc;
	int tmp;

	printf("netdock_watchdog: resetting chip\n");
	tmp = ifp->if_flags & IFF_UP;
	netdock_reset(sc);
	ifp->if_flags |= tmp;
}

static u_int
netdock_put(struct netdock_softc *sc, struct mbuf *m0)
{
	struct mbuf *m;
	u_int totlen = 0;
	u_int tmplen;
	int isr;
	int timeout;
	int tmp;
	u_int i;

	for (m = m0; m; m = m->m_next)
		totlen += m->m_len;

	if (totlen >= ETHER_MAX_LEN)
		panic("%s: netdock_put: packet overflow", sc->sc_dev.dv_xname);

	totlen += 6;
	tmplen = totlen;
	tmplen &= 0xff00;
	tmplen |= 0x2000;
	NIC_PUT_2(sc, REG_0000, tmplen);

	timeout = 0x3000;
	while ((((isr = NIC_GET_2(sc, REG_ISR)) & ISR_READY) == 0) &&
	    timeout--) {
		if (isr & ISR_TX)
			netdock_txint(sc);
	}
	if (timeout == 0)
		return (0);

	tmp = NIC_GET_2(sc, REG_0002);
	tmp <<= 8;
	NIC_PUT_2(sc, REG_0002, tmp);
	NIC_PUT_2(sc, REG_0006, 0x240);
	NIC_GET_2(sc, REG_ISR);
	tmplen = ((totlen << 8) & 0xfe00) | ((totlen >> 8) & 0x00ff);
	NIC_PUT_2(sc, REG_DATA, tmplen);

	for (m = m0; m; m = m->m_next) {
		u_char *data = mtod(m, u_char *);
		int len = m->m_len;
		int len4 = len >> 2;
		u_int32_t *data4 = (u_int32_t *)data;
		for (i = 0; i < len4; i++)
			NIC_PUT_4(sc, REG_DATA, data4[i]);
		for (i = len4 << 2; i < len; i++)
			NIC_PUT_1(sc, REG_DATA, data[i]);
	}

	if (totlen & 0x01)
		NIC_PUT_2(sc, REG_DATA, 0x2020);
	else
		NIC_PUT_2(sc, REG_DATA, 0);

	NIC_PUT_2(sc, REG_0000, 0xc000);

	m_freem(m0);
	/* sc->sc_if.if_timer = 5; */
	return (totlen);
}

void
netdock_intr(void *arg)
{
	struct netdock_softc *sc = (struct netdock_softc *)arg;
	int isr;
	int tmp;

	NIC_PUT_2(sc, REG_ISR, 0);
	while ((isr = (NIC_GET_2(sc, REG_ISR) & ISR_ALL)) != 0) {
		if (isr & ISR_TX)
			netdock_txint(sc);

		if (isr & ISR_RX)
			netdock_rxint(sc);

		if (isr & (ISR_BIT_0C | ISR_BIT_0D)) {
			if (isr & ISR_BIT_0C) {
				NIC_PUT_2(sc, REG_000E, 0);
				NIC_BSET(sc, REG_0004, 0x08);
				NIC_PUT_2(sc, REG_000E, 0x0200);
			}
			if (isr & ISR_BIT_0D) {
				NIC_PUT_2(sc, REG_000E, 0);
				tmp = NIC_GET_2(sc, REG_0002);
				if (tmp & REG_0002_BIT_04)
					NIC_GET_2(sc, REG_0006);
				tmp = NIC_GET_2(sc, REG_0000);
				if (tmp & REG_0000_BIT_08)
					NIC_BSET(sc, REG_0000, 0x08);
				NIC_PUT_2(sc, REG_000E, 0x0200);
			}
			NIC_PUT_2(sc, REG_ISR, isr);
		}
	}
	NIC_PUT_2(sc, REG_ISR, ISR_MASK);
}

static void
netdock_txint(struct netdock_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	int savereg0002;
	int reg0004;
	int regdata;

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;

	savereg0002 = NIC_GET_2(sc, REG_0002);

	while (((reg0004 = NIC_GET_2(sc, REG_0004)) & REG_0004_BIT_0F) == 0) {
		NIC_PUT_2(sc, REG_0002, reg0004);
		NIC_PUT_2(sc, REG_0006, 0x0060);
		NIC_GET_2(sc, REG_ISR);
		regdata = NIC_GET_2(sc, REG_DATA);
		if ((regdata & REG_DATA_BIT_08) == 0) {
			/* ifp->if_collisions++; */
			if (regdata & REG_DATA_BIT_07)
			/* ifp->if_oerrors++; */
			NIC_PUT_2(sc, REG_000E, 0);
			NIC_ORW(sc, REG_0000, 0x0100);
			NIC_PUT_2(sc, REG_000E, 0x0200);
		}
		NIC_GET_2(sc ,REG_DATA);

		if (regdata & REG_DATA_BIT_0F)
			NIC_GET_4(sc, REG_EFD00);
		NIC_PUT_2(sc, REG_0000, 0xa000);
		NIC_PUT_2(sc, REG_ISR, ISR_TX);
	}

	NIC_PUT_2(sc, REG_0002, savereg0002);
	NIC_PUT_2(sc, REG_000E, 0);
	NIC_GET_2(sc, REG_0006);
	NIC_PUT_2(sc, REG_000E, 0x0200);
	NIC_PUT_2(sc, REG_0006, 0);
}

static void
netdock_rxint(struct netdock_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	int regdata1;
	int regdata2;
	u_int len;
	int timeout;

	while ((NIC_GET_2(sc, REG_0004) & REG_0004_BIT_07) == 0) {
		NIC_GET_2(sc, REG_ISR);
		NIC_PUT_2(sc, REG_0006, 0x00e0);
		NIC_GET_2(sc, REG_ISR);
		regdata1 = NIC_GET_2(sc, REG_DATA);
		regdata2 = NIC_GET_2(sc, REG_DATA);
		len = ((regdata2 << 8) & 0x0700) | ((regdata2 >> 8) & 0x00ff);

#if 0
		printf("netdock_rxint: r1=0x%04x, r2=0x%04x, len=%d\n",
		       regdata1, regdata2, len);
#endif

		if ((regdata1 & REG_DATA_BIT_04) == 0)
			len -= 2;

		/* CRC is included with the packet; trim it off. */
		len -= ETHER_CRC_LEN;

		if ((regdata1 & 0x00ac) == 0) {
			if (netdock_read(sc, len))
				ifp->if_ipackets++;
			else
				ifp->if_ierrors++;
		} else {
			ifp->if_ierrors++;

			if (regdata1 & REG_DATA_BIT_02)
				NIC_GET_4(sc, REG_EFD00);
			if (regdata1 & REG_DATA_BIT_03)
				;
			if (regdata1 & REG_DATA_BIT_05)
				NIC_GET_4(sc, REG_EFD00);
			if (regdata1 & REG_DATA_BIT_07)
				;
		}

		timeout = 0x14;
		while ((NIC_GET_2(sc, REG_0000) & REG_0000_BIT_08) && timeout--)
			;
		if (timeout == 0)
			;

		NIC_PUT_2(sc, REG_0000, 0x8000);
	}
	NIC_PUT_2(sc, REG_0006, 0);
}

static int
netdock_read(struct netdock_softc *sc, int len)
{
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;

	m = netdock_get(sc, len);
	if (m == 0)
		return (0);

#if NBPFILTER > 0
	if (ifp->if_bpf) 
		bpf_mtap(ifp->if_bpf, m);
#endif

	(*ifp->if_input)(ifp, m);

	return (1);
}

static struct mbuf *
netdock_get(struct netdock_softc *sc, int datalen)
{
	struct mbuf *m, *top, **mp;
	u_char *data;
	int i;
	int len;
	int len4;
	u_int32_t *data4;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (NULL);
	m->m_pkthdr.rcvif = &sc->sc_if;
	m->m_pkthdr.len = datalen;
	len = MHLEN;
	top = NULL;
	mp = &top;

	while (datalen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (NULL);
			}
			len = MLEN;
		}
		if (datalen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				if (top)
					m_freem(top);
				return (NULL);
			}
			len = MCLBYTES;
		}

		if (mp == &top) {
			caddr_t newdata = (caddr_t)
			    ALIGN(m->m_data + sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data;
			m->m_data = newdata;
		}

		m->m_len = len = min(datalen, len);

		data = mtod(m, u_char *);
		len4 = len >> 2;
		data4 = (u_int32_t *)data;
		for (i = 0; i < len4; i++)
			data4[i] = NIC_GET_4(sc, REG_DATA);
		for (i = len4 << 2; i < len; i++)
			data[i] = NIC_GET_1(sc, REG_DATA);

		datalen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return (top);
}
