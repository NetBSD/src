/*	$NetBSD: if_xi.c,v 1.4 2000/07/31 21:49:47 gmcgarry Exp $	*/
/*	OpenBSD: if_xe.c,v 1.9 1999/09/16 11:28:42 niklas Exp 	*/

/*
 * Copyright (c) 1999 Niklas Hallqvist, Brandon Creighton, Job de Haas
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
 *	This product includes software developed by Niklas Hallqvist,
 *	Brandon Creighton and Job de Haas.
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
 * A driver for Xircom CreditCard PCMCIA Ethernet adapters.
 */

/*
 * Known Bugs:
 *
 * 1) Promiscuous mode doesn't work on at least the CE2.
 * 2) Slow. ~450KB/s.  Memory access would be better.
 */

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef IPX
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#define ETHER_MIN_LEN 64
#define ETHER_CRC_LEN 4

/*
 * Maximum number of bytes to read per interrupt.  Linux recommends
 * somewhere between 2000-22000.
 * XXX This is currently a hard maximum.
 */
#define MAX_BYTES_INTR 12000

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#define XI_IOSIZ	16

#include <dev/pcmcia/if_xireg.h>

#ifdef __GNUC__
#define INLINE	__inline
#else
#define INLINE
#endif	/* __GNUC__ */

#ifdef XIDEBUG
#define DPRINTF(cat, x) if (xidebug & (cat)) printf x

#define XID_CONFIG	0x1
#define XID_MII		0x2
#define XID_INTR	0x4
#define XID_FIFO	0x8

#ifdef XIDEBUG_VALUE
int xidebug = XIDEBUG_VALUE;
#else
int xidebug = 0;
#endif
#else
#define DPRINTF(cat, x) (void)0
#endif

int	xi_pcmcia_match __P((struct device *, struct cfdata *, void *));
void	xi_pcmcia_attach __P((struct device *, struct device *, void *));
int	xi_pcmcia_detach __P((struct device *, int));
int	xi_pcmcia_activate __P((struct device *, enum devact));

/*
 * In case this chipset ever turns up out of pcmcia attachments (very
 * unlikely) do the driver splitup.
 */
struct xi_softc {
	struct device sc_dev;			/* Generic device info */
	struct ethercom sc_ethercom;		/* Ethernet common part */

	struct mii_data sc_mii;			/* MII media information */

	bus_space_tag_t		sc_bst;		/* Bus cookie */
	bus_space_handle_t	sc_bsh;		/* Bus I/O handle */
	bus_addr_t		sc_offset;	/* Offset of registers */

	u_int8_t	sc_rev;			/* Chip revision */
	u_int32_t	sc_flags;		/* Misc. flags */
	int		sc_all_mcasts;		/* Receive all multicasts */
	u_int8_t 	sc_enaddr[ETHER_ADDR_LEN];
};

struct xi_pcmcia_softc {
	struct	xi_softc sc_xi;			/* Generic device info */

	/* PCMCIA-specific goo */
	struct	pcmcia_function *sc_pf;		/* PCMCIA function */
	struct	pcmcia_io_handle sc_pcioh;	/* iospace info */
	int	sc_io_window;			/* io window info */
	void	*sc_ih;				/* Interrupt handler */

	int	sc_resource;			/* resource allocated */
#define XI_RES_PCIC	1
#define XI_RES_IO	2
#define XI_RES_MI	8
};

struct cfattach xi_pcmcia_ca = {
	sizeof(struct xi_pcmcia_softc), xi_pcmcia_match, xi_pcmcia_attach,
	xi_pcmcia_detach, xi_pcmcia_activate
};

static int xi_pcmcia_cis_quirks __P((struct pcmcia_function *));
static void xi_cycle_power __P((struct xi_softc *));
static int xi_ether_ioctl __P((struct ifnet *, u_long cmd, caddr_t));
static void xi_full_reset __P((struct xi_softc *));
static void xi_init __P((struct xi_softc *));
static int xi_intr __P((void *));
static int xi_ioctl __P((struct ifnet *, u_long, caddr_t));
static int xi_mdi_read __P((struct device *, int, int));
static void xi_mdi_write __P((struct device *, int, int, int));
static int xi_mediachange __P((struct ifnet *));
static void xi_mediastatus __P((struct ifnet *, struct ifmediareq *));
static int xi_pcmcia_funce_enaddr __P((struct device *, u_int8_t *));
static int xi_pcmcia_lan_nid_ciscallback __P((struct pcmcia_tuple *, void *));
static int xi_pcmcia_manfid_ciscallback __P((struct pcmcia_tuple *, void *));
static u_int16_t xi_get __P((struct xi_softc *));
static void xi_reset __P((struct xi_softc *));
static void xi_set_address __P((struct xi_softc *));
static void xi_start __P((struct ifnet *));
static void xi_statchg __P((struct device *));
static void xi_stop __P((struct xi_softc *));
static void xi_watchdog __P((struct ifnet *));
struct xi_pcmcia_product *xi_pcmcia_identify __P((struct device *,
						struct pcmcia_attach_args *));

/* flags */
#define XIFLAGS_MOHAWK	0x001		/* 100Mb capabilities (has phy) */
#define XIFLAGS_DINGO	0x002		/* realport cards ??? */
#define XIFLAGS_MODEM	0x004		/* modem also present */

struct xi_pcmcia_product {
	u_int32_t	xpp_vendor;	/* vendor ID */
	u_int32_t	xpp_product;	/* product ID */
	int		xpp_expfunc;	/* expected function number */
	int		xpp_flags;	/* initial softc flags */
	const char	*xpp_name;	/* device name */
} xi_pcmcia_products[] = {
#ifdef NOT_SUPPORTED
	{ PCMCIA_VENDOR_XIRCOM,		0x0141,
	  0,				0,
	  PCMCIA_STR_XIRCOM_CE },
#endif
	{ PCMCIA_VENDOR_XIRCOM,		0x0141,
	  0,				0,
	  PCMCIA_STR_XIRCOM_CE2 },
	{ PCMCIA_VENDOR_XIRCOM,		0x0142,
	  0,				0,
	  PCMCIA_STR_XIRCOM_CE2 },
	{ PCMCIA_VENDOR_XIRCOM,		0x0143,
	  0,				XIFLAGS_MOHAWK,
	  PCMCIA_STR_XIRCOM_CE3 },
	{ PCMCIA_VENDOR_COMPAQ2,	0x0143,
	  0,				XIFLAGS_MOHAWK,
	  PCMCIA_STR_COMPAQ2_CPQ_10_100 },
	{ PCMCIA_VENDOR_INTEL,		0x0143,
	  0,				XIFLAGS_MOHAWK | XIFLAGS_MODEM,
	  PCMCIA_STR_INTEL_EEPRO100 },
#ifdef NOT_SUPPORTED
	{ PCMCIA_VENDOR_XIRCOM,		0x1141,
	  0,				XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_CEM },
#endif
	{ PCMCIA_VENDOR_XIRCOM,		0x1142,
	  0,				XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_CEM },
	{ PCMCIA_VENDOR_XIRCOM,		0x1143,
	  0,				XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_CEM },
	{ PCMCIA_VENDOR_XIRCOM,		0x1144,
	  0,				XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_CEM33 },
	{ PCMCIA_VENDOR_XIRCOM,		0x1145,
	  0,				XIFLAGS_MOHAWK | XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_CEM56 },
	{ PCMCIA_VENDOR_XIRCOM,		0x1146,
	  0,				XIFLAGS_MOHAWK | XIFLAGS_DINGO | XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_REM56 },
	{ PCMCIA_VENDOR_XIRCOM,		0x1147,
	  0,				XIFLAGS_MOHAWK | XIFLAGS_DINGO | XIFLAGS_MODEM,
	  PCMCIA_STR_XIRCOM_REM56 },
	{ 0,				0,
	  0,				0,
	  NULL },
};


struct xi_pcmcia_product *
xi_pcmcia_identify(dev, pa)
	struct device *dev;
        struct pcmcia_attach_args *pa;
{
	struct xi_pcmcia_product *xpp;
        u_int8_t id;
	u_int32_t prod;

	/*
	 * The Xircom ethernet cards swap the revision and product fields
	 * inside the CIS, which makes identification just a little
	 * bit different.
	 */

        pcmcia_scan_cis(dev, xi_pcmcia_manfid_ciscallback, &id);

	prod = (pa->product & ~0xff) | id;

	DPRINTF(XID_CONFIG, ("product=0x%x\n", prod));

	for (xpp = xi_pcmcia_products; xpp->xpp_name != NULL; xpp++)
		if (pa->manufacturer == xpp->xpp_vendor &&
			prod == xpp->xpp_product &&
			pa->pf->number == xpp->xpp_expfunc)
			return (xpp);
	return (NULL);
}

/*
 * If someone can determine which manufacturers/products require cis_quirks,
 * then the proper infrastucture can be used.  Until then...
 * This also becomes a pain with detaching.
 */
static int
xi_pcmcia_cis_quirks(pf)
	struct pcmcia_function *pf;
{
	struct pcmcia_config_entry *cfe;

	/* Tell the pcmcia framework where the CCR is. */
	pf->ccr_base = 0x800;
	pf->ccr_mask = 0x67;

	/* Fake a cfe. */
	SIMPLEQ_FIRST(&pf->cfe_head) = cfe = (struct pcmcia_config_entry *)
	    malloc(sizeof(*cfe), M_DEVBUF, M_NOWAIT);

	if (cfe == NULL)
		return -1;
	bzero(cfe, sizeof(*cfe));

	/*
	 * XXX Use preprocessor symbols instead.
	 * Enable ethernet & its interrupts, wiring them to -INT
	 * No I/O base.
	 */
	cfe->number = 0x5;
	cfe->flags = 0;		/* XXX Check! */
	cfe->iftype = PCMCIA_IFTYPE_IO;
	cfe->num_iospace = 0;
	cfe->num_memspace = 0;
	cfe->irqmask = 0x8eb0;

	return 0;
}

int
xi_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;
	
	if (pa->pf->function != PCMCIA_FUNCTION_NETWORK)
		return (0);

	if (pa->manufacturer == PCMCIA_VENDOR_COMPAQ2 &&
	    pa->product == PCMCIA_PRODUCT_COMPAQ2_CPQ_10_100)
		return (1);

	if (pa->manufacturer == PCMCIA_VENDOR_INTEL &&
	   pa->product == PCMCIA_PRODUCT_INTEL_EEPRO100)
		return (1);

	if (pa->manufacturer == PCMCIA_VENDOR_XIRCOM &&
	    ((pa->product >> 8) == XIMEDIA_ETHER ||
	    (pa->product >> 8) == (XIMEDIA_ETHER | XIMEDIA_MODEM)))
		return (1);

	return (0);
}

void
xi_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct xi_pcmcia_softc *psc = (struct xi_pcmcia_softc *)self;
	struct xi_softc *sc = &psc->sc_xi;
	struct pcmcia_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct xi_pcmcia_product *xpp;

	if (xi_pcmcia_cis_quirks(pa->pf) < 0) {
		printf(": function enable failed\n");
		return;
	}

	/* Enable the card */
	psc->sc_pf = pa->pf;
	pcmcia_function_init(psc->sc_pf, psc->sc_pf->cfe_head.sqh_first);
	if (pcmcia_function_enable(psc->sc_pf)) {
		printf(": function enable failed\n");
		goto fail;
	}
	psc->sc_resource |= XI_RES_PCIC;

	/* allocate/map ISA I/O space */
	if (pcmcia_io_alloc(psc->sc_pf, 0, XI_IOSIZ, XI_IOSIZ,
		&psc->sc_pcioh) != 0) {
		printf(": i/o allocation failed\n");
		goto fail;
	}
	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_IO16, 0, XI_IOSIZ,
		&psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		goto fail;
	}
	sc->sc_bst = psc->sc_pcioh.iot;
	sc->sc_bsh = psc->sc_pcioh.ioh;
	sc->sc_offset = 0;
	psc->sc_resource |= XI_RES_IO;

	xpp = xi_pcmcia_identify(parent,pa);
	if (xpp == NULL) {
		printf(": unrecognised model\n");
		return;
	}
	sc->sc_flags = xpp->xpp_flags;

	printf(": %s\n", xpp->xpp_name);

	/*
	 * Configuration as adviced by DINGO documentation.
	 * Dingo has some extra configuration registers in the CCR space.
	 */
	if (sc->sc_flags & XIFLAGS_DINGO) {
		struct pcmcia_mem_handle pcmh;
		int ccr_window;
		bus_addr_t ccr_offset;

		if (pcmcia_mem_alloc(psc->sc_pf, PCMCIA_CCR_SIZE_DINGO,
			&pcmh)) {
			DPRINTF(XID_CONFIG, ("xi: bad mem alloc\n"));
			goto fail;
		}

		if (pcmcia_mem_map(psc->sc_pf, PCMCIA_MEM_ATTR,
			psc->sc_pf->ccr_base, PCMCIA_CCR_SIZE_DINGO,
			&pcmh, &ccr_offset, &ccr_window)) {
			DPRINTF(XID_CONFIG, ("xi: bad mem map\n"));
			pcmcia_mem_free(psc->sc_pf, &pcmh);
			goto fail;
		}

		bus_space_write_1(pcmh.memt, pcmh.memh,
		    ccr_offset + PCMCIA_CCR_DCOR0, PCMCIA_CCR_DCOR0_SFINT);
		bus_space_write_1(pcmh.memt, pcmh.memh,
		    ccr_offset + PCMCIA_CCR_DCOR1,
		    PCMCIA_CCR_DCOR1_FORCE_LEVIREQ | PCMCIA_CCR_DCOR1_D6);
		bus_space_write_1(pcmh.memt, pcmh.memh,
		    ccr_offset + PCMCIA_CCR_DCOR2, 0);
		bus_space_write_1(pcmh.memt, pcmh.memh,
		    ccr_offset + PCMCIA_CCR_DCOR3, 0);
		bus_space_write_1(pcmh.memt, pcmh.memh,
		    ccr_offset + PCMCIA_CCR_DCOR4, 0);

		/* We don't need them anymore and can free them (I think). */
		pcmcia_mem_unmap(psc->sc_pf, ccr_window);
		pcmcia_mem_free(psc->sc_pf, &pcmh);
	}

	/*
	 * Try to get the ethernet address from FUNCE/LAN_NID tuple.
	 */
	xi_pcmcia_funce_enaddr(parent, sc->sc_enaddr);
	if (!sc->sc_enaddr) {
		printf("%s: unable to get ethernet address\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(sc->sc_enaddr));

	ifp = &sc->sc_ethercom.ec_if;
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = xi_start;
	ifp->if_ioctl = xi_ioctl;
	ifp->if_watchdog = xi_watchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_NOTRAILERS | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;

	/* Reset and initialize the card. */
	xi_full_reset(sc);

	/*
	 * Initialize our media structures and probe the MII.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = xi_mdi_read;
	sc->sc_mii.mii_writereg = xi_mdi_write;
	sc->sc_mii.mii_statchg = xi_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, xi_mediachange,
	    xi_mediastatus);
	DPRINTF(XID_MII | XID_CONFIG,
	    ("xi: bmsr %x\n", xi_mdi_read(&sc->sc_dev, 0, 1)));
	mii_attach(self, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL)
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER | IFM_AUTO, 0,
		    NULL);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_AUTO);

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
	psc->sc_resource |= XI_RES_MI;

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif	/* NBPFILTER > 0 */

	/*
	 * Reset and initialize the card again for DINGO (as found in Linux
	 * driver).  Without this Dingo will get a watchdog timeout the first
	 * time.  The ugly media tickling seems to be necessary for getting
	 * autonegotiation to work too.
	 */
	if (sc->sc_flags & XIFLAGS_DINGO) {
		xi_full_reset(sc);
		xi_init(sc);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_AUTO);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_NONE);
		xi_stop(sc);
	}

	/* Establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, xi_intr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	return;

fail:
	if ((psc->sc_resource & XI_RES_IO) != 0) {
		/* Unmap our i/o windows. */
		pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
                pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
        }
        psc->sc_resource &= ~XI_RES_IO;
	if (psc->sc_resource & XI_RES_PCIC) {
		pcmcia_function_disable(pa->pf);
		psc->sc_resource &= ~XI_RES_PCIC;
	}
	free(SIMPLEQ_FIRST(&psc->sc_pf->cfe_head), M_DEVBUF);
}

int
xi_pcmcia_detach(self, flags)
     struct device *self;
     int flags;
{
	struct xi_pcmcia_softc *psc = (struct xi_pcmcia_softc *)self;
	struct xi_softc *sc = &psc->sc_xi;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	DPRINTF(XID_CONFIG, ("xi_pcmcia_detach()\n"));

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		xi_stop(sc);
	}

	pcmcia_function_disable(psc->sc_pf);
	psc->sc_resource &= ~XI_RES_PCIC;
	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
	ifp->if_flags &= ~IFF_RUNNING;
	ifp->if_timer = 0;

	if ((psc->sc_resource & XI_RES_MI) != 0) {

		mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);

		ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);
#if NBPFILTER > 0
		bpfdetach(ifp);
#endif
		ether_ifdetach(ifp);
		if_detach(ifp);
		psc->sc_resource &= ~XI_RES_MI;
	}

	if ((psc->sc_resource & XI_RES_IO) != 0) {
		/* Unmap our i/o windows. */
		pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
                pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
        }
        free(SIMPLEQ_FIRST(&psc->sc_pf->cfe_head), M_DEVBUF);
	psc->sc_resource &= ~XI_RES_IO;

	return 0;
}

int
xi_pcmcia_activate(self, act)
     struct device *self;
     enum devact act;
{
	struct xi_pcmcia_softc *psc = (struct xi_pcmcia_softc *)self;
	struct xi_softc *sc = &psc->sc_xi;
	int s, rv=0;

	DPRINTF(XID_CONFIG, ("xi_pcmcia_activate()\n"));

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ethercom.ec_if);
		break;
	}
	splx(s);
	return (rv);
}

/*
 * XXX These two functions might be OK to factor out into pcmcia.c since
 * if_sm_pcmcia.c uses similar ones.
 */
static int
xi_pcmcia_funce_enaddr(parent, myla)
	struct device *parent;
	u_int8_t *myla;
{
	/* XXX The Linux driver has more ways to do this in case of failure. */
	return (pcmcia_scan_cis(parent, xi_pcmcia_lan_nid_ciscallback, myla));
}

static int
xi_pcmcia_lan_nid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int8_t *myla = arg;
	int i;

	DPRINTF(XID_CONFIG, ("xi_pcmcia_lan_nid_ciscallback()\n"));

	if (tuple->code == PCMCIA_CISTPL_FUNCE) {
		if (tuple->length < 2)
			return (0);

		switch (pcmcia_tuple_read_1(tuple, 0)) {
		case PCMCIA_TPLFE_TYPE_LAN_NID:
			if (pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN)
				return (0);
			break;

		case 0x02:
			/*
			 * Not sure about this, I don't have a CE2
			 * that puts the ethernet addr here.
			 */
		 	if (pcmcia_tuple_read_1(tuple, 1) != 13)
				return (0);
			break;

		default:
			return (0);
		}

		for (i = 0; i < ETHER_ADDR_LEN; i++)
			myla[i] = pcmcia_tuple_read_1(tuple, i + 2);
		return (1);
	}

	/* Yet another spot where this might be. */
	if (tuple->code == 0x89) {
		pcmcia_tuple_read_1(tuple, 1);
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			myla[i] = pcmcia_tuple_read_1(tuple, i + 2);
		return (1);
	}
	return (0);
}

int
xi_pcmcia_manfid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int8_t *id = arg;

	DPRINTF(XID_CONFIG, ("xi_pcmcia_manfid_callback()\n"));

	if (tuple->code != PCMCIA_CISTPL_MANFID)
		return (0);

	if (tuple->length < 2)
		return (0);

	*id = pcmcia_tuple_read_1(tuple, 4);
	return (1);
}



static int
xi_intr(arg)
	void *arg;
{
	struct xi_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int8_t esr, rsr, isr, rx_status, savedpage;
	u_int16_t tx_status, recvcount = 0, tempint;

	DPRINTF(XID_CONFIG, ("xi_intr()\n"));

#if 0
	if (!(ifp->if_flags & IFF_RUNNING))
		return (0);
#endif

	ifp->if_timer = 0;	/* turn watchdog timer off */

	if (sc->sc_flags & XIFLAGS_MOHAWK) {
		/* Disable interrupt (Linux does it). */
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + CR,
		    0);
	}

	savedpage =
	    bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + PR);

	PAGE(sc, 0);
	esr = bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + ESR);
	isr = bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + ISR0);
	rsr = bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + RSR);
				
	/* Check to see if card has been ejected. */
	if (isr == 0xff) {
#ifdef DIAGNOSTIC
		printf("%s: interrupt for dead card\n", sc->sc_dev.dv_xname);
#endif
		goto end;
	}

	PAGE(sc, 40);
	rx_status =
	    bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + RXST0);
	tx_status =
	    bus_space_read_2(sc->sc_bst, sc->sc_bsh, sc->sc_offset + TXST0);

	/*
	 * XXX Linux writes to RXST0 and TXST* here.  My CE2 works just fine
	 * without it, and I can't see an obvious reason for it.
	 */

	PAGE(sc, 0);
	while (esr & FULL_PKT_RCV) {
		if (!(rsr & RSR_RX_OK))
			break;

		/* Compare bytes read this interrupt to hard maximum. */
		if (recvcount > MAX_BYTES_INTR) {
			DPRINTF(XID_INTR,
			    ("xi: too many bytes this interrupt\n"));
			ifp->if_iqdrops++;
			/* Drop packet. */
			bus_space_write_2(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + DO0, DO_SKIP_RX_PKT);
		}
		tempint = xi_get(sc);	/* XXX doesn't check the error! */
		recvcount += tempint;
		ifp->if_ibytes += tempint;
		esr = bus_space_read_1(sc->sc_bst, sc->sc_bsh,
		    sc->sc_offset + ESR);
		rsr = bus_space_read_1(sc->sc_bst, sc->sc_bsh,
		    sc->sc_offset + RSR);
	}
	
	/* Packet too long? */
	if (rsr & RSR_TOO_LONG) {
		ifp->if_ierrors++;
		DPRINTF(XID_INTR, ("xi: packet too long\n"));
	}

	/* CRC error? */
	if (rsr & RSR_CRCERR) {
		ifp->if_ierrors++;
		DPRINTF(XID_INTR, ("xi: CRC error detected\n"));
	}

	/* Alignment error? */
	if (rsr & RSR_ALIGNERR) {
		ifp->if_ierrors++;
		DPRINTF(XID_INTR, ("xi: alignment error detected\n"));
	}

	/* Check for rx overrun. */
	if (rx_status & RX_OVERRUN) {
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + CR,
		    CLR_RX_OVERRUN);
		DPRINTF(XID_INTR, ("xi: overrun cleared\n"));
	}
			
	/* Try to start more packets transmitting. */
	if (ifp->if_snd.ifq_head)
		xi_start(ifp);

	/* Detected excessive collisions? */
	if ((tx_status & EXCESSIVE_COLL) && ifp->if_opackets > 0) {
		DPRINTF(XID_INTR, ("xi: excessive collisions\n"));
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + CR,
		    RESTART_TX);
		ifp->if_oerrors++;
	}
	
	if ((tx_status & TX_ABORT) && ifp->if_opackets > 0)
		ifp->if_oerrors++;

end:
	/* Reenable interrupts. */
	PAGE(sc, savedpage);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + CR,
	    ENABLE_INT);

	return (1);
}

/*
 * Pull a packet from the card into an mbuf chain.
 */
static u_int16_t
xi_get(sc)
	struct xi_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *top, **mp, *m;
	u_int16_t pktlen, len, recvcount = 0;
	u_int8_t *data;
	u_int8_t rsr;
	
	DPRINTF(XID_CONFIG, ("xi_get()\n"));

	PAGE(sc, 0);
	rsr = bus_space_read_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + RSR);

	pktlen =
	    bus_space_read_2(sc->sc_bst, sc->sc_bsh, sc->sc_offset + RBC0) &
	    RBC_COUNT_MASK;

	DPRINTF(XID_CONFIG, ("xi_get: pktlen=%d\n", pktlen));

	if (pktlen == 0) {
		/*
		 * XXX At least one CE2 sets RBC0 == 0 occasionally, and only
		 * when MPE is set.  It is not known why.
		 */
		return (0);
	}

	/* XXX should this be incremented now ? */
	recvcount += pktlen;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (recvcount);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = pktlen;
	len = MHLEN;
	top = 0;
	mp = &top;
	
	while (pktlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (recvcount);
			}
			len = MLEN;
		}
		if (pktlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (!(m->m_flags & M_EXT)) {
				m_freem(m);
				m_freem(top);
				return (recvcount);
			}
			len = MCLBYTES;
		}
		if (!top) {
			caddr_t newdata = (caddr_t)ALIGN(m->m_data +
			    sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data;
			m->m_data = newdata;
		}
		len = min(pktlen, len);
		data = mtod(m, u_int8_t *);
		if (len > 1) {
		        len &= ~1;
			bus_space_read_multi_2(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + EDP, data, len>>1);
		} else
			*data = bus_space_read_1(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + EDP);
		m->m_len = len;
		pktlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	/* Skip Rx packet. */
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, sc->sc_offset + DO0,
	    DO_SKIP_RX_PKT);
	
	ifp->if_ipackets++;
	
#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, top);
#endif
	
	(*ifp->if_input)(ifp, top);
	return (recvcount);
}

/*
 * Serial management for the MII.
 * The DELAY's below stem from the fact that the maximum frequency
 * acceptable on the MDC pin is 2.5 MHz and fast processors can easily
 * go much faster than that.
 */

/* Let the MII serial management be idle for one period. */
static INLINE void xi_mdi_idle __P((struct xi_softc *));
static INLINE void
xi_mdi_idle(sc)
	struct xi_softc *sc;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;

	/* Drive MDC low... */
	bus_space_write_1(bst, bsh, offset + GP2, MDC_LOW);
	DELAY(1);

	/* and high again. */
	bus_space_write_1(bst, bsh, offset + GP2, MDC_HIGH);
	DELAY(1);
}

/* Pulse out one bit of data. */
static INLINE void xi_mdi_pulse __P((struct xi_softc *, int));
static INLINE void
xi_mdi_pulse(sc, data)
	struct xi_softc *sc;
	int data;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;
	u_int8_t bit = data ? MDIO_HIGH : MDIO_LOW;

	/* First latch the data bit MDIO with clock bit MDC low...*/
	bus_space_write_1(bst, bsh, offset + GP2, bit | MDC_LOW);
	DELAY(1);

	/* then raise the clock again, preserving the data bit. */
	bus_space_write_1(bst, bsh, offset + GP2, bit | MDC_HIGH);
	DELAY(1);
}

/* Probe one bit of data. */
static INLINE int xi_mdi_probe __P((struct xi_softc *sc));
static INLINE int
xi_mdi_probe(sc)
	struct xi_softc *sc;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;
	u_int8_t x;

	/* Pull clock bit MDCK low... */
	bus_space_write_1(bst, bsh, offset + GP2, MDC_LOW);
	DELAY(1);

	/* Read data and drive clock high again. */
	x = bus_space_read_1(bst, bsh, offset + GP2) & MDIO;
	bus_space_write_1(bst, bsh, offset + GP2, MDC_HIGH);
	DELAY(1);

	return (x);
}

/* Pulse out a sequence of data bits. */
static INLINE void xi_mdi_pulse_bits __P((struct xi_softc *, u_int32_t, int));
static INLINE void
xi_mdi_pulse_bits(sc, data, len)
	struct xi_softc *sc;
	u_int32_t data;
	int len;
{
	u_int32_t mask;

	for (mask = 1 << (len - 1); mask; mask >>= 1)
		xi_mdi_pulse(sc, data & mask);
}

/* Read a PHY register. */
static int
xi_mdi_read(self, phy, reg)
	struct device *self;
	int phy;
	int reg;
{
	struct xi_softc *sc = (struct xi_softc *)self;
	int i;
	u_int32_t mask;
	u_int32_t data = 0;

	PAGE(sc, 2);
	for (i = 0; i < 32; i++)	/* Synchronize. */
		xi_mdi_pulse(sc, 1);
	xi_mdi_pulse_bits(sc, 0x06, 4); /* Start + Read opcode */
	xi_mdi_pulse_bits(sc, phy, 5);	/* PHY address */
	xi_mdi_pulse_bits(sc, reg, 5);	/* PHY register */
	xi_mdi_idle(sc);		/* Turn around. */
	xi_mdi_probe(sc);		/* Drop initial zero bit. */

	for (mask = 1 << 15; mask; mask >>= 1) {
		if (xi_mdi_probe(sc))
			data |= mask;
	}
	xi_mdi_idle(sc);

	DPRINTF(XID_MII,
	    ("xi_mdi_read: phy %d reg %d -> %x\n", phy, reg, data));

	return (data);
}

/* Write a PHY register. */
static void
xi_mdi_write(self, phy, reg, value)
	struct device *self;
	int phy;
	int reg;
	int value;
{
	struct xi_softc *sc = (struct xi_softc *)self;
	int i;

	PAGE(sc, 2);
	for (i = 0; i < 32; i++)	/* Synchronize. */
		xi_mdi_pulse(sc, 1);
	xi_mdi_pulse_bits(sc, 0x05, 4); /* Start + Write opcode */
	xi_mdi_pulse_bits(sc, phy, 5);	/* PHY address */
	xi_mdi_pulse_bits(sc, reg, 5);	/* PHY register */
	xi_mdi_pulse_bits(sc, 0x02, 2); /* Turn around. */
	xi_mdi_pulse_bits(sc, value, 16);	/* Write the data */
	xi_mdi_idle(sc);		/* Idle away. */

	DPRINTF(XID_MII,
	    ("xi_mdi_write: phy %d reg %d val %x\n", phy, reg, value));
}

static void
xi_statchg(self)
	struct device *self;
{
	/* XXX Update ifp->if_baudrate */
}

/*
 * Change media according to request.
 */
static int
xi_mediachange(ifp)
	struct ifnet *ifp;
{
	DPRINTF(XID_CONFIG, ("xi_mediachange()\n"));

	if (ifp->if_flags & IFF_UP)
		xi_init(ifp->if_softc);
	return (0);
}

/*
 * Notify the world which media we're using.
 */
static void
xi_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct xi_softc *sc = ifp->if_softc;

	DPRINTF(XID_CONFIG, ("xi_mediastatus()\n"));

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

static void
xi_reset(sc)
	struct xi_softc *sc;
{
	int s;

	DPRINTF(XID_CONFIG, ("xi_reset()\n"));

	s = splnet();
	xi_stop(sc);
	xi_full_reset(sc);
	xi_init(sc);
	splx(s);
}

static void
xi_watchdog(ifp)
	struct ifnet *ifp;
{
	struct xi_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;

	xi_reset(sc);
}

static void
xi_stop(sc)
	register struct xi_softc *sc;
{
	DPRINTF(XID_CONFIG, ("xi_stop()\n"));

	/* Disable interrupts. */
	PAGE(sc, 0);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + CR, 0);

	PAGE(sc, 1);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + IMR0, 0);
	
	/* Power down, wait. */
	PAGE(sc, 4);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, sc->sc_offset + GP1, 0);
	DELAY(40000);
	
	/* Cancel watchdog timer. */
	sc->sc_ethercom.ec_if.if_timer = 0;
}

static void
xi_init(sc)
	struct xi_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	DPRINTF(XID_CONFIG, ("xi_init()\n"));

	s = splimp();

	xi_set_address(sc);

	/* Set current media. */
	mii_mediachg(&sc->sc_mii);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);
}

/*
 * Start outputting on the interface.
 * Always called as splnet().
 */
static void
xi_start(ifp)
	struct ifnet *ifp;
{
	struct xi_softc *sc = ifp->if_softc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;
	unsigned int s, len, pad = 0;
	struct mbuf *m0, *m;
	u_int16_t space;

	DPRINTF(XID_CONFIG, ("xi_start()\n"));

	/* Don't transmit if interface is busy or not running. */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING) {
		DPRINTF(XID_CONFIG, ("xi: interface busy or not running\n"));
		return;
	}

	/* Peek at the next packet. */
	m0 = ifp->if_snd.ifq_head;
	if (m0 == 0)
		return;

	/* We need to use m->m_pkthdr.len, so require the header. */
	if (!(m0->m_flags & M_PKTHDR))
		panic("xi_start: no header mbuf");

	len = m0->m_pkthdr.len;

	/* Pad to ETHER_MIN_LEN - ETHER_CRC_LEN. */
	if (len < ETHER_MIN_LEN - ETHER_CRC_LEN)
		pad = ETHER_MIN_LEN - ETHER_CRC_LEN - len;

	PAGE(sc, 0);
	space = bus_space_read_2(bst, bsh, offset + TSO0) & 0x7fff;
	if (len + pad + 2 > space) {
		DPRINTF(XID_FIFO,
		    ("xi: not enough space in output FIFO (%d > %d)\n",
		    len + pad + 2, space));
		return;
	}

	IF_DEQUEUE(&ifp->if_snd, m0);

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	/*
	 * Do the output at splhigh() so that an interrupt from another device
	 * won't cause a FIFO underrun.
	 */
	s = splhigh();

	bus_space_write_2(bst, bsh, offset + TSO2, (u_int16_t)len + pad + 2);
	bus_space_write_2(bst, bsh, offset + EDP, (u_int16_t)len + pad);
	for (m = m0; m; ) {
		if (m->m_len > 1)
			bus_space_write_multi_2(bst, bsh, offset + EDP,
			    mtod(m, u_int8_t *), m->m_len>>1);
		if (m->m_len & 1)
			bus_space_write_1(bst, bsh, offset + EDP,
			    *(mtod(m, u_int8_t *) + m->m_len - 1));
		MFREE(m, m0);
		m = m0;
	}
	if (sc->sc_flags & XIFLAGS_MOHAWK)
		bus_space_write_1(bst, bsh, offset + CR, TX_PKT | ENABLE_INT);
	else {
		for (; pad > 1; pad -= 2)
			bus_space_write_2(bst, bsh, offset + EDP, 0);
		if (pad == 1)
			bus_space_write_1(bst, bsh, offset + EDP, 0);
	}

	splx(s);

	ifp->if_timer = 5;
	++ifp->if_opackets;
}

static int
xi_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct xi_softc *sc = ifp->if_softc;


	DPRINTF(XID_CONFIG, ("xi_ether_ioctl()\n"));

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			xi_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif	/* INET */

#ifdef NS
		case AF_NS:
		{
			struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
					LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host,
					LLADDR(ifp->if_sadl),
					ifp->if_addrlen);
			/* Set new address. */
			xi_init(sc);
			break;
		}
#endif  /* NS */

		default:
			xi_init(sc);
			break;
		}
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

static int
xi_ioctl(ifp, command, data)
	struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct xi_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	DPRINTF(XID_CONFIG, ("xi_ioctl()\n"));

	s = splimp();

	switch (command) {
	case SIOCSIFADDR:
		error = xi_ether_ioctl(ifp, command, data);
		break;

	case SIOCSIFFLAGS:
		sc->sc_all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;
				
		PAGE(sc, 0x42);
		if ((ifp->if_flags & IFF_PROMISC) ||
		    (ifp->if_flags & IFF_ALLMULTI))
			bus_space_write_1(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + SWC1,
			    SWC1_PROMISC | SWC1_MCAST_PROM);
		else
			bus_space_write_1(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + SWC1, 0);

		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */
		if (ifp->if_flags & IFF_UP) {
			xi_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING) {
				xi_stop(sc);
				ifp->if_flags &= ~IFF_RUNNING;
			}
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		sc->sc_all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware
			 * filter accordingly.
			 */
			if (!sc->sc_all_mcasts &&
			    !(ifp->if_flags & IFF_PROMISC))
				xi_set_address(sc);

			/*
			 * xi_set_address() can turn on all_mcasts if we run
			 * out of space, so check it again rather than else {}.
			 */
			if (sc->sc_all_mcasts)
				xi_init(sc);
			error = 0;
		}
		break;

	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error =
		    ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, command);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

static void
xi_set_address(sc)
	struct xi_softc *sc;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;
	struct ethercom *ether = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i, page, pos, num;

	DPRINTF(XID_CONFIG, ("xi_set_address()\n"));

	PAGE(sc, 0x50);
	for (i = 0; i < 6; i++) {
		bus_space_write_1(bst, bsh, offset + IA + i,
		    sc->sc_enaddr[(sc->sc_flags & XIFLAGS_MOHAWK) ?  5-i : i]);
	}
		
	if (ether->ec_multicnt > 0) {
		if (ether->ec_multicnt > 9) {
			PAGE(sc, 0x42);
			bus_space_write_1(sc->sc_bst, sc->sc_bsh,
			    sc->sc_offset + SWC1,
			    SWC1_PROMISC | SWC1_MCAST_PROM);
			return;
		}

		ETHER_FIRST_MULTI(step, ether, enm);

		pos = IA + 6;
		for (page = 0x50, num = ether->ec_multicnt; num > 0 && enm;
		    num--) {
			if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
			    sizeof(enm->enm_addrlo)) != 0) {
				/*
				 * The multicast address is really a range;
				 * it's easier just to accept all multicasts.
				 * XXX should we be setting IFF_ALLMULTI here?
				 */
				ifp->if_flags |= IFF_ALLMULTI;
				sc->sc_all_mcasts=1;
				break;
			}

			for (i = 0; i < 6; i++) {
				bus_space_write_1(bst, bsh, offset + pos,
				    enm->enm_addrlo[
				    (sc->sc_flags & XIFLAGS_MOHAWK) ? 5-i : i]);

				if (++pos > 15) {
					pos = IA;
					page++;
					PAGE(sc, page);
				}
			}
		}
	}
}

static void
xi_cycle_power(sc)
	struct xi_softc *sc;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;

	DPRINTF(XID_CONFIG, ("xi_cycle_power()\n"));

	PAGE(sc, 4);
	DELAY(1);
	bus_space_write_1(bst, bsh, offset + GP1, 0);
	DELAY(40000);
	if (sc->sc_flags & XIFLAGS_MOHAWK)
		bus_space_write_1(bst, bsh, offset + GP1, POWER_UP);
	else
		/* XXX What is bit 2 (aka AIC)? */
		bus_space_write_1(bst, bsh, offset + GP1, POWER_UP | 4);
	DELAY(20000);
}

static void
xi_full_reset(sc)
	struct xi_softc *sc;
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_addr_t offset = sc->sc_offset;

	DPRINTF(XID_CONFIG, ("xi_full_reset()\n"));

	/* Do an as extensive reset as possible on all functions. */
	xi_cycle_power(sc);
	bus_space_write_1(bst, bsh, offset + CR, SOFT_RESET);
	DELAY(20000);
	bus_space_write_1(bst, bsh, offset + CR, 0);
	DELAY(20000);
	if (sc->sc_flags & XIFLAGS_MOHAWK) {
		PAGE(sc, 4);
		/*
		 * Drive GP1 low to power up ML6692 and GP2 high to power up
		 * the 10Mhz chip.  XXX What chip is that?  The phy?
		 */
		bus_space_write_1(bst, bsh, offset + GP0,
		    GP1_OUT | GP2_OUT | GP2_WR);
	}
	DELAY(500000);

	/* Get revision information.  XXX Symbolic constants. */
	sc->sc_rev = bus_space_read_1(bst, bsh, offset + BV) &
	    ((sc->sc_flags & XIFLAGS_MOHAWK) ? 0x70 : 0x30) >> 4;

	/* Media selection.  XXX Maybe manual overriding too? */
	if (!(sc->sc_flags & XIFLAGS_MOHAWK)) {
		PAGE(sc, 4);
		/*
		 * XXX I have no idea what this really does, it is from the
		 * Linux driver.
		 */
		bus_space_write_1(bst, bsh, offset + GP0, GP1_OUT);
	}
	DELAY(40000);

	/* Setup the ethernet interrupt mask. */
	PAGE(sc, 1);
	bus_space_write_1(bst, bsh, offset + IMR0,
	    ISR_TX_OFLOW | ISR_PKT_TX | ISR_MAC_INT | /* ISR_RX_EARLY | */
	    ISR_RX_FULL | ISR_RX_PKT_REJ | ISR_FORCED_INT);
#if 0
	bus_space_write_1(bst, bsh, offset + IMR0, 0xff);
#endif
	if (!(sc->sc_flags & XIFLAGS_DINGO)) {
		/* XXX What is this?  Not for Dingo at least. */
		bus_space_write_1(bst, bsh, offset + IMR1, 1);
	}

	/*
	 * Disable source insertion.
	 * XXX Dingo does not have this bit, but Linux does it unconditionally.
	 */
	if (!(sc->sc_flags & XIFLAGS_DINGO)) {
		PAGE(sc, 0x42);
		bus_space_write_1(bst, bsh, offset + SWC0, 0x20);
	}

	/* Set the local memory dividing line. */
	if (sc->sc_rev != 1) {
		PAGE(sc, 2);
		/* XXX Symbolic constant preferrable. */
		bus_space_write_2(bst, bsh, offset + RBS0, 0x2000);
	}

	xi_set_address(sc);

	/*
	 * Apparently the receive byte pointer can be bad after a reset, so
	 * we hardwire it correctly.
	 */
	PAGE(sc, 0);
	bus_space_write_2(bst, bsh, offset + DO0, DO_CHG_OFFSET);

	/* Setup ethernet MAC registers. XXX Symbolic constants. */
	PAGE(sc, 0x40);
	bus_space_write_1(bst, bsh, offset + RX0MSK,
	    PKT_TOO_LONG | CRC_ERR | RX_OVERRUN | RX_ABORT | RX_OK);
	bus_space_write_1(bst, bsh, offset + TX0MSK,
	    CARRIER_LOST | EXCESSIVE_COLL | TX_UNDERRUN | LATE_COLLISION |
	    SQE | TX_ABORT | TX_OK);
	if (!(sc->sc_flags & XIFLAGS_DINGO))
		/* XXX From Linux, dunno what 0xb0 means. */
		bus_space_write_1(bst, bsh, offset + TX1MSK, 0xb0);
	bus_space_write_1(bst, bsh, offset + RXST0, 0);
	bus_space_write_1(bst, bsh, offset + TXST0, 0);
	bus_space_write_1(bst, bsh, offset + TXST1, 0);

	/* Enable MII function if available. */
	if (LIST_FIRST(&sc->sc_mii.mii_phys)) {
		PAGE(sc, 2);
		bus_space_write_1(bst, bsh, offset + MSR,
		    bus_space_read_1(bst, bsh, offset + MSR) | SELECT_MII);
		DELAY(20000);
	} else {
		PAGE(sc, 0);
				
		/* XXX Do we need to do this? */
		PAGE(sc, 0x42);
		bus_space_write_1(bst, bsh, offset + SWC1, SWC1_AUTO_MEDIA);
		DELAY(50000);

		/* XXX Linux probes the media here. */
	}

	/* Configure the LED registers. */
	PAGE(sc, 2);

	/* XXX This is not good for 10base2. */
	bus_space_write_1(bst, bsh, offset + LED,
	    LED_TX_ACT << LED1_SHIFT | LED_10MB_LINK << LED0_SHIFT);
	if (sc->sc_flags & XIFLAGS_DINGO)
		bus_space_write_1(bst, bsh, offset + LED3,
		    LED_100MB_LINK << LED3_SHIFT);

	/* Enable receiver and go online. */
	PAGE(sc, 0x40);
	bus_space_write_1(bst, bsh, offset + CMD0, ENABLE_RX | ONLINE);

#if 0
	/* XXX Linux does this here - is it necessary? */
	PAGE(sc, 1);
	bus_space_write_1(bst, bsh, offset + IMR0, 0xff);
	if (!(sc->sc_flags & XIFLAGS_DINGO)) {
		/* XXX What is this?  Not for Dingo at least. */
		bus_space_write_1(bst, bsh, offset + IMR1, 1);
	}
#endif

       /* Enable interrupts. */
	PAGE(sc, 0);
	bus_space_write_1(bst, bsh, offset + CR, ENABLE_INT);

	/* XXX This is pure magic for me, found in the Linux driver. */
	if ((sc->sc_flags & (XIFLAGS_DINGO | XIFLAGS_MODEM)) == XIFLAGS_MODEM) {
		if ((bus_space_read_1(bst, bsh, offset + 0x10) & 0x01) == 0)
			/* Unmask the master interrupt bit. */
			bus_space_write_1(bst, bsh, offset + 0x10, 0x11);
	}

	/*
	 * The Linux driver says this:
	 * We should switch back to page 0 to avoid a bug in revision 0
	 * where regs with offset below 8 can't be read after an access
	 * to the MAC registers.
	 */
	PAGE(sc, 0);
}
