/*	$NetBSD: ethfoo_lkm.c,v 1.1 2003/11/24 21:58:45 cube Exp $	*/

/*
 *  Copyright (c) 2003, Quentin Garnier.  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ethfoo is a NetBSD Loadable Kernel Module that demonstrates the use of
 * several kernel mechanisms, mostly in the networking subsytem.
 *
 * First, it is sample LKM, with the standard LKM management routines and
 * Second, sample Ethernet driver.
 * Third, sample of use of autoconf stuff inside a LKM.
 * Fourth, sample clonable interface
 *
 * XXX Hacks
 * 1. NetBSD doesn't offer a way to change an Ethernet address, so I chose
 *    to use the SIOCSIFPHYADDR ioctl() normally used by gif(4).
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#if __NetBSD_Version__ > 106200000
#include <sys/ksyms.h>
#endif
#include <sys/lkm.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

/* autoconf(9) structures */

CFDRIVER_DECL(ethfoo, DV_DULL, NULL);

static struct cfdata ethfoo_cfdata = {
	"ethfoo", "ethfoo", 0, FSTATE_STAR,
};

/*
 * Since we're an Ethernet device, we need the 3 following
 * components: a leading struct device, a struct ethercom,
 * and also a struct ifmedia since we don't attach a PHY to
 * ourselves. We could emulate one, but there's no real
 * point.
 *
 * sc_bpf_mtap is here to optionnally depend on BPF, to make
 * our faked interface available to BPF listeners. Here, ksyms
 * is used to check if BPF was compiled in, so it is only
 * available in -current.
 */

struct ethfoo_softc {
	struct device	sc_dev;
	struct ifmedia	sc_im;
	struct ethercom	sc_ec;
	void		(*sc_bpf_mtap)(caddr_t, struct mbuf *);
};

/* LKM management routines */

int		ethfoo_lkmentry(struct lkm_table *, int, int);
static int	ethfoo_lkmload(struct lkm_table *, int);
static int	ethfoo_lkmunload(struct lkm_table *, int);

/* autoconf(9) glue */

static int	ethfoo_match(struct device *, struct cfdata *, void *);
static void	ethfoo_attach(struct device *, struct device *, void *);
static int	ethfoo_detach(struct device*, int);

CFATTACH_DECL(ethfoo, sizeof(struct ethfoo_softc),
    ethfoo_match, ethfoo_attach, ethfoo_detach, NULL);

/*
 * We don't need any particular functionality available to LKMs,
 * such as a device number or a syscall number, thus MOD_MISC is
 * enough.
 */

MOD_MISC("ethfoo");

/*
 * Those are needed by the if_media interface.
 */

static int	ethfoo_mediachange(struct ifnet *);
static void	ethfoo_mediastatus(struct ifnet *, struct ifmediareq *);

/*
 * Those are needed by the ifnet interface, and would typically be
 * there for any network interface driver.
 * Some other routines are optional: watchdog and drain.
 */

static void	ethfoo_start(struct ifnet *);
static void	ethfoo_stop(struct ifnet *, int);
static int	ethfoo_init(struct ifnet *);
static int	ethfoo_ioctl(struct ifnet *, u_long, caddr_t);

/* This is an internal function to keep ethfoo_ioctl readable */
static int	ethfoo_lifaddr(struct ifnet *, u_long, struct ifaliasreq *);

/*
 * ethfoo is a clonable interface, although it is highly unrealistic for
 * an Ethernet device.
 *
 * Here are the bits needed for a clonable interface.
 */
static int	ethfoo_clone_create(struct if_clone *, int);
static void	ethfoo_clone_destroy(struct ifnet *);

struct if_clone ethfoo_cloners = IF_CLONE_INITIALIZER("ethfoo",
					ethfoo_clone_create,
					ethfoo_clone_destroy);

/* We don't have anything to do on 'modstat' */
int
ethfoo_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, ethfoo_lkmload, ethfoo_lkmunload, lkm_nofunc);
}

/*
 * autoconf(9) is a rather complicated piece of work, but in the end
 * it is rather flexible, so you can easily add a device somewhere in
 * the tree, and make almost anything attach to something known.
 *
 * Here the idea is taken from Jason R. Thorpe's ataraid(4) pseudo-
 * device.  Instead of needing a declaration in the kernel
 * configuration, we teach autoconf(9) the availability of the
 * pseudo-device at run time.
 *
 * Once our autoconf(9) structures are committed to the kernel's
 * arrays, we can attach a device.  It is done through config_attach
 * for a real device, but for a pseudo-device it is a bit different
 * and one has to use config_pseudo_attach.
 *
 * And since we want the user to be responsible for creating device,
 * we use the interface cloning mechanism, and advertise our interface
 * to the kernel.
 */
static int
ethfoo_lkmload(struct lkm_table *lkmtp, int cmd)
{
	int error = 0;

	error = config_cfdriver_attach(&ethfoo_cd);
	if (error) {
		aprint_error("%s: unable to register cfdriver\n",
		    ethfoo_cd.cd_name);
		goto out;
	}

	error = config_cfattach_attach(ethfoo_cd.cd_name, &ethfoo_ca);
	if (error) {
		aprint_error("%s: unable to register cfattach\n",
		    ethfoo_cd.cd_name);
		(void)config_cfdriver_detach(&ethfoo_cd);
		goto out;
	}

	if_clone_attach(&ethfoo_cloners);

out:
	return error;
}

/*
 * Cleaning up is the most critical part of a LKM, since a module is not
 * actually made to be loadable, but rather "unloadable".  If it is only
 * to be loaded, you'd better link it to the kernel in the first place,
 * either through compilation or through static linking (I think modload
 * allows that).
 *
 * The interface cloning mechanism is really simple, with only two void
 * returning functions.  It will always do its job. You should note though
 * that if an instance of ethfoo can't be detached, the module won't
 * unload and you won't be able to create interfaces anymore.
 *
 * We have to make sure the devices really exist, because they can be
 * destroyed through ifconfig, hence the test whether cd_devs[i] is NULL
 * or not.
 *
 * The cd_devs array is somehow the downside of the whole autoconf(9)
 * mechanism, since if you only create 'ethfoo150', you'll get an array of
 * 150 elements which 149 of them are NULL.
 */
static int
ethfoo_lkmunload(struct lkm_table *lkmtp, int cmd)
{
	int error, i;

	if_clone_detach(&ethfoo_cloners);

	for (i = 0; i < ethfoo_cd.cd_ndevs; i++)
		if (ethfoo_cd.cd_devs[i] != NULL &&
		    (error = config_detach(ethfoo_cd.cd_devs[i], 0)) != 0) {
			aprint_error("%s: unable to detach instance\n",
			    ((struct device *)ethfoo_cd.cd_devs[i])->dv_xname);
			return error;
		}

	if ((error = config_cfattach_detach(ethfoo_cd.cd_name, &ethfoo_ca)) != 0) {
		aprint_error("%s: unable to deregister cfattach\n",
		    ethfoo_cd.cd_name);
		return error;
	}

	if ((error = config_cfdriver_detach(&ethfoo_cd)) != 0) {
		aprint_error("%s: unable to deregister cfdriver\n",
		    ethfoo_cd.cd_name);
		return error;
	}

	return (0);
}

/* Pretty much useless for a pseudo-device */
static int
ethfoo_match(struct device *self, struct cfdata *cfdata, void *arg)
{
	return (1);
}

static void
ethfoo_attach(struct device *parent, struct device *self, void *aux)
{
	struct ethfoo_softc *sc = (struct ethfoo_softc *)self;
	struct ifnet *ifp;
	u_int8_t enaddr[ETHER_ADDR_LEN] = { 0xf0, 0x0b, 0xa4, 0xff, 0xff, 0xff };
	unsigned long u;
	uint32_t ui;

	aprint_normal("%s: faking Ethernet device\n",
	    self->dv_xname);

	/*
	 * In order to obtain unique initial Ethernet address on a host,
	 * do some randomisation using mono_time.  It's not meant for anything
	 * but avoiding hard-coding an address.
	 */
	ui = (mono_time.tv_sec ^ mono_time.tv_usec) & 0xffffff;
	memcpy(enaddr+3, (u_int8_t *)&ui, 3);

	aprint_normal("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/* ksyms interface is only available since mid-1.6R, so require
	 * at least 1.6T
	 */
#if __NetBSD_Version__ > 106200000
# ifndef ksyms_getval_from_kernel
#  define ksyms_getval_from_kernel ksyms_getval
#endif
	if (ksyms_getval_from_kernel(NULL, "bpf_mtap", &u, KSYMS_PROC) != ENOENT)
		sc->sc_bpf_mtap = (void *)u;
	else
#endif
		sc->sc_bpf_mtap = NULL;

	/*
	 * Why 1000baseT? Why not? You can add more.
	 *
	 * Note that there are 3 steps: init, one or several additions to
	 * list of supported media, and in the end, the selection of one
	 * of them.
	 */
	ifmedia_init(&sc->sc_im, 0, ethfoo_mediachange, ethfoo_mediastatus);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_1000_T, 0, NULL);
	ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_1000_T);

	/*
	 * One should note that an interface must do multicast in order
	 * to support IPv6.
	 */
	ifp = &sc->sc_ec.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc	= sc;
	ifp->if_flags	= IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl	= ethfoo_ioctl;
	ifp->if_start	= ethfoo_start;
	ifp->if_stop	= ethfoo_stop;
	ifp->if_init	= ethfoo_init;
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ec.ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;

	/* Those steps are mandatory for an Ethernet driver, the fisrt call
	 * being common to all network interface drivers. */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
}

/*
 * When detaching, we do the inverse of what is done in the attach
 * routine, in reversed order.
 */
static int
ethfoo_detach(struct device* self, int flags)
{
	struct ethfoo_softc *sc = (struct ethfoo_softc *)self;
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	ether_ifdetach(ifp);
	if_detach(ifp);
	ifmedia_delete_instance(&sc->sc_im, IFM_INST_ANY);

	if (!(flags & DETACH_QUIET))
		aprint_normal("%s detached\n",
		    self->dv_xname);

	return (0);
}

/*
 * This function is called by the ifmedia layer to notify the driver
 * that the user requested a media change.  A real driver would
 * reconfigure the hardware.
 */
static int
ethfoo_mediachange(struct ifnet *ifp)
{
	return (0);
}


/*
 * Here the user asks for the currently used media.
 */
static void
ethfoo_mediastatus(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct ethfoo_softc *sc = (struct ethfoo_softc *)ifp->if_softc;
	imr->ifm_active = sc->sc_im.ifm_cur->ifm_media;
}


/*
 * This is the function where we SEND packets.
 *
 * There is no 'receive' equivalent.  A typical driver will get
 * interrupts from the hardware, and from there will inject new packets
 * into the network stack.
 *
 * Once handled, a packet must be freed.  A real driver might not be able
 * to fit all the pending packets into the hardware, and is allowed to
 * return before having sent all the packets.  It should then use the
 * if_flags flag IFF_OACTIVE to notify the upper layer.
 *
 * There are also other flags one should check, such as IFF_PAUSE.
 *
 * It is our duty to make packets available to BPF listeners.
 */
static void
ethfoo_start(struct ifnet *ifp)
{
	struct ethfoo_softc *sc = (struct ethfoo_softc *)ifp->if_softc;
	struct mbuf *m0;

	for(;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			return;

		if (ifp->if_bpf && sc->sc_bpf_mtap != NULL)
			(sc->sc_bpf_mtap)(ifp->if_bpf, m0);

		m_freem(m0);
	}
}

/*
 * A typical driver will only contain the following handlers for
 * ioctl calls, except SIOCSIFPHYADDR.
 * The latter is a hack I used to set the Ethernet address of the
 * faked device.
 *
 * Note that both ifmedia_ioctl() and ether_ioctl() have to be
 * called under splnet().
 */
static int
ethfoo_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ethfoo_softc *sc = (struct ethfoo_softc *)ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_im, cmd);
		break;
	case SIOCSIFPHYADDR:
		error = ethfoo_lifaddr(ifp, cmd, (struct ifaliasreq *)data);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET)
			error = 0;
		break;
	}

	splx(s);

	return (error);
}

/*
 * Helper function to set Ethernet address.  This shouldn't be done there,
 * and should actually be available to all Ethernet drivers, real or not.
 */
static int
ethfoo_lifaddr(struct ifnet *ifp, u_long cmd, struct ifaliasreq *ifra)
{
	struct sockaddr *sa = (struct sockaddr *)&ifra->ifra_addr;

	if (sa->sa_family != AF_LINK)
		return (EINVAL);

	memcpy(LLADDR(ifp->if_sadl), sa->sa_data, ETHER_ADDR_LEN);

	return (0);
}

/*
 * _init() would typically be called when an interface goes up,
 * meaning it should configure itself into the state in which it
 * can send packets.
 */
static int
ethfoo_init(struct ifnet *ifp)
{
	ifp->if_flags |= IFF_RUNNING;

	ethfoo_start(ifp);

	return (0);
}

/*
 * _stop() is called when an interface goes down.  It is our
 * responsability to validate that state by clearing the
 * IFF_RUNNING flag.
 */
static void
ethfoo_stop(struct ifnet *ifp, int disable)
{
	ifp->if_flags &= ~IFF_RUNNING;
}

/*
 * The 'create' command of ifconfig can be used to create
 * any numbered instance of a given device.  Thus we have to
 * make sure we have enough room in cd_devs to create the
 * user-specified instance.
 *
 * config_attach_pseudo can be called with unit = -1 to have
 * autoconf(9) choose a unit number for us.
 */
static int
ethfoo_clone_create(struct if_clone *ifc, int unit)
{
	config_makeroom(unit, &ethfoo_cd); /* can panic */

	if (config_attach_pseudo(ethfoo_cd.cd_name, unit) == NULL) {
		aprint_error("%s%d: unable to attach an instance\n",
                    ethfoo_cd.cd_name, unit);
		return (ENXIO);
	}

	return (0);
}

/*
 * The clean design of if_clone and autoconf(9) makes that part
 * really straightforward.  The second argument of config_detach
 * means neither QUIET not FORCED.
 */
static void
ethfoo_clone_destroy(struct ifnet *ifp)
{
	struct device *dev = (struct device *)ifp->if_softc;

	if (config_detach(dev, 0) != 0)
		aprint_error("%s: unable to detach instance\n",
		    dev->dv_xname);
}
