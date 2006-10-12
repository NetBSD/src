/*	$NetBSD: xirc.c,v 1.19 2006/10/12 01:31:50 christos Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xirc.c,v 1.19 2006/10/12 01:31:50 christos Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif


#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include "xirc.h"

#if NCOM_XIRC > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#if NXI_XIRC > 0
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pcmcia/if_xivar.h>
#endif
#include <dev/pcmcia/if_xireg.h>

struct xirc_softc {
	struct device sc_dev;		/* generic device glue */

	struct pcmcia_function *sc_pf;	/* our PCMCIA function */
	void *sc_ih;			/* interrupt handle */

	u_int16_t sc_id;
	u_int8_t sc_mako_intmask;
	int sc_chipset;

	/*
	 * Data for the Modem portion.
	 */
	struct device *sc_modem;
	struct pcmcia_io_handle sc_modem_pcioh;
	int sc_modem_io_window;

	/*
	 * Data for the Ethernet portion.
	 */
	struct device *sc_ethernet;
	struct pcmcia_io_handle sc_ethernet_pcioh;
	int sc_ethernet_io_window;

	int sc_flags;
#define	XIRC_MODEM_MAPPED	0x01
#define	XIRC_ETHERNET_MAPPED	0x02
#define	XIRC_MODEM_ENABLED	0x04
#define	XIRC_ETHERNET_ENABLED	0x08
#define	XIRC_MODEM_ALLOCED	0x10
#define	XIRC_ETHERNET_ALLOCED	0x20
};

int	xirc_match(struct device *, struct cfdata *, void *);
void	xirc_attach(struct device *, struct device *, void *);
int	xirc_detach(struct device *, int);
int	xirc_activate(struct device *, enum devact);

CFATTACH_DECL(xirc, sizeof(struct xirc_softc),
    xirc_match, xirc_attach, xirc_detach, xirc_activate);

int	xirc_print(void *, const char *);

int	xirc_manfid_ciscallback(struct pcmcia_tuple *, void *);
struct pcmcia_config_entry *
	xirc_mako_alloc(struct xirc_softc *);
struct pcmcia_config_entry *
	xirc_dingo_alloc_modem(struct xirc_softc *);
struct pcmcia_config_entry *
	xirc_dingo_alloc_ethernet(struct xirc_softc *);

int	xirc_enable(struct xirc_softc *, int, int);
void	xirc_disable(struct xirc_softc *, int, int);

int	xirc_intr(void *);

int
xirc_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	/* XXX Toshiba, Accton */

	if (pa->manufacturer == PCMCIA_VENDOR_COMPAQ2 &&
	    pa->product == PCMCIA_PRODUCT_COMPAQ2_CPQ_10_100)
		return (1);

	if (pa->manufacturer == PCMCIA_VENDOR_INTEL &&
	    pa->product == PCMCIA_PRODUCT_INTEL_EEPRO100)
		return (1);

	if (pa->manufacturer == PCMCIA_VENDOR_XIRCOM &&
	    (pa->product & (XIMEDIA_ETHER << 8)) != 0)
		return (2);

	return (0);
}

void
xirc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct xirc_softc *sc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	int rv;
	int error;

	sc->sc_pf = pa->pf;

	pcmcia_socket_enable(parent);
	rv = pcmcia_scan_cis(parent, xirc_manfid_ciscallback, &sc->sc_id);
	pcmcia_socket_disable(parent);
	if (!rv) {
		aprint_error("%s: failed to find ID\n", self->dv_xname);
		return;
	}

	switch (sc->sc_id & 0x100f) {
	case 0x0001:	/* CE */
	case 0x0002:	/* CE2 */
		sc->sc_chipset = XI_CHIPSET_SCIPPER;
		break;
	case 0x0003:	/* CE3 */
		sc->sc_chipset = XI_CHIPSET_MOHAWK;
		break;
	case 0x1001:
	case 0x1002:
	case 0x1003:
	case 0x1004:
		sc->sc_chipset = XI_CHIPSET_SCIPPER;
		break;
	case 0x1005:
		sc->sc_chipset = XI_CHIPSET_MOHAWK;
		break;
	case 0x1006:
	case 0x1007:
		sc->sc_chipset = XI_CHIPSET_DINGO;
		break;
	default:
		aprint_error("%s: unknown ID %04x\n", self->dv_xname,
		    sc->sc_id);
		return;
	}

	aprint_normal("%s: id=%04x\n", self->dv_xname, sc->sc_id);

	if (sc->sc_id & (XIMEDIA_MODEM << 8)) {
		if (sc->sc_chipset >= XI_CHIPSET_DINGO) {
			cfe = xirc_dingo_alloc_modem(sc);
			if (cfe && sc->sc_id & (XIMEDIA_ETHER << 8)) {
				if (!xirc_dingo_alloc_ethernet(sc)) {
					pcmcia_io_free(pa->pf,
					    &sc->sc_modem_pcioh);
					cfe = 0;
				}
			}
		} else
			cfe = xirc_mako_alloc(sc);
	} else
		cfe = xirc_dingo_alloc_ethernet(sc);
	if (!cfe) {
		aprint_error("%s: failed to allocate I/O space\n",
		    self->dv_xname);
		goto fail;
	}

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);

	if (sc->sc_id & (XIMEDIA_MODEM << 8)) {
		if (pcmcia_io_map(sc->sc_pf, PCMCIA_WIDTH_IO8,
		    &sc->sc_modem_pcioh, &sc->sc_modem_io_window)) {
			aprint_error("%s: unable to map I/O space\n",
			    self->dv_xname);
			goto fail;
		}
		sc->sc_flags |= XIRC_MODEM_MAPPED;
	}

	if (sc->sc_id & (XIMEDIA_ETHER << 8)) {
		if (pcmcia_io_map(sc->sc_pf, PCMCIA_WIDTH_AUTO,
		    &sc->sc_ethernet_pcioh, &sc->sc_ethernet_io_window)) {
			aprint_error("%s: unable to map I/O space\n",
			    self->dv_xname);
			goto fail;
		}
		sc->sc_flags |= XIRC_ETHERNET_MAPPED;
	}

	error = xirc_enable(sc, XIRC_MODEM_ENABLED|XIRC_ETHERNET_ENABLED,
	    sc->sc_id & (XIMEDIA_MODEM|XIMEDIA_ETHER));
	if (error)
		goto fail;

	sc->sc_mako_intmask = 0xee;

	if (sc->sc_id & (XIMEDIA_MODEM << 8))
		/*XXXUNCONST*/
		sc->sc_modem = config_found(self, __UNCONST("com"), xirc_print);
	if (sc->sc_id & (XIMEDIA_ETHER << 8))
		/*XXXUNCONST*/
		sc->sc_ethernet = config_found(self, __UNCONST("xi"),
		    xirc_print);

	xirc_disable(sc, XIRC_MODEM_ENABLED|XIRC_ETHERNET_ENABLED,
	    sc->sc_id & (XIMEDIA_MODEM|XIMEDIA_ETHER));
	return;

fail:
	/* I/O spaces will be freed by detach. */
	;
}

int
xirc_manfid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int16_t *id = arg;

	if (tuple->code != PCMCIA_CISTPL_MANFID)
		return (0);

	if (tuple->length < 5)
		return (0);

	*id = (pcmcia_tuple_read_1(tuple, 3) << 8) |
	      pcmcia_tuple_read_1(tuple, 4);
	return (1);
}

struct pcmcia_config_entry *
xirc_mako_alloc(sc)
	struct xirc_softc *sc;
{
	struct pcmcia_config_entry *cfe;

	SIMPLEQ_FOREACH(cfe, &sc->sc_pf->cfe_head, cfe_list) {
		if (cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(sc->sc_pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, cfe->iospace[0].length,
		    &sc->sc_modem_pcioh))
			continue;

		cfe->iospace[1].start = cfe->iospace[0].start+8;
		cfe->iospace[1].length = 18;
		if (pcmcia_io_alloc(sc->sc_pf, cfe->iospace[1].start,
		    cfe->iospace[1].length, 0x20,
		    &sc->sc_ethernet_pcioh)) {
			cfe->iospace[1].start = cfe->iospace[0].start-24;
			if (pcmcia_io_alloc(sc->sc_pf, cfe->iospace[1].start,
			    cfe->iospace[1].length, 0x20,
			    &sc->sc_ethernet_pcioh))
				continue;
		}

		/* Found one! */
		sc->sc_flags |= XIRC_MODEM_ALLOCED;
		sc->sc_flags |= XIRC_ETHERNET_ALLOCED;
		return (cfe);
	}

	return (0);
}

struct pcmcia_config_entry *
xirc_dingo_alloc_modem(sc)
	struct xirc_softc *sc;
{
	struct pcmcia_config_entry *cfe;

	SIMPLEQ_FOREACH(cfe, &sc->sc_pf->cfe_head, cfe_list) {
		if (cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(sc->sc_pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, cfe->iospace[0].length,
		    &sc->sc_modem_pcioh))
			continue;

		/* Found one! */
		sc->sc_flags |= XIRC_MODEM_ALLOCED;
		return (cfe);
	}

	return (0);
}

struct pcmcia_config_entry *
xirc_dingo_alloc_ethernet(sc)
	struct xirc_softc *sc;
{
	struct pcmcia_config_entry *cfe;
	bus_addr_t port;

	for (port = 0x300; port < 0x400; port += XI_IOSIZE) {
		if (pcmcia_io_alloc(sc->sc_pf, port,
		    XI_IOSIZE, XI_IOSIZE, &sc->sc_ethernet_pcioh))
			continue;

		/* Found one for the ethernet! */
		sc->sc_flags |= XIRC_ETHERNET_ALLOCED;
		cfe = SIMPLEQ_FIRST(&sc->sc_pf->cfe_head);
		return (cfe);
	}

	return (0);
}

int
xirc_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	const char *name = aux;

	if (pnp)
		aprint_normal("%s at %s(*)",  name, pnp);

	return (UNCONF);
}

int
xirc_detach(self, flags)
	struct device *self;
	int flags;
{
	struct xirc_softc *sc = (void *)self;
	int rv;

	if (sc->sc_ethernet != NULL) {
		rv = config_detach(sc->sc_ethernet, flags);
		if (rv != 0)
			return (rv);
		sc->sc_ethernet = NULL;
	}

	if (sc->sc_modem != NULL) {
		rv = config_detach(sc->sc_modem, flags);
		if (rv != 0)
			return (rv);
		sc->sc_modem = NULL;
	}

	/* Unmap our i/o windows. */
	if (sc->sc_flags & XIRC_ETHERNET_MAPPED)
		pcmcia_io_unmap(sc->sc_pf, sc->sc_ethernet_io_window);
	if (sc->sc_flags & XIRC_MODEM_MAPPED)
		pcmcia_io_unmap(sc->sc_pf, sc->sc_modem_io_window);

	/* Free our i/o spaces. */
	if (sc->sc_flags & XIRC_ETHERNET_ALLOCED)
		pcmcia_io_free(sc->sc_pf, &sc->sc_ethernet_pcioh);
	if (sc->sc_flags & XIRC_MODEM_ALLOCED)
		pcmcia_io_free(sc->sc_pf, &sc->sc_modem_pcioh);
	sc->sc_flags = 0;

	return (0);
}

int
xirc_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct xirc_softc *sc = (void *)self;
	int s, rv = 0;

	s = splhigh();
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_ethernet != NULL) {
			rv = config_deactivate(sc->sc_ethernet);
			if (rv != 0)
				goto out;
		}

		if (sc->sc_modem != NULL) {
			rv = config_deactivate(sc->sc_modem);
			if (rv != 0)
				goto out;
		}
		break;
	}
 out:
	splx(s);
	return (rv);
}

int
xirc_intr(arg)
	void *arg;
{
	struct xirc_softc *sc = arg;
	int rval = 0;

#if NCOM_XIRC > 0
	if (sc->sc_modem != NULL &&
	    (sc->sc_flags & XIRC_MODEM_ENABLED) != 0)
		rval |= comintr(sc->sc_modem);
#endif

#if NXI_XIRC > 0
	if (sc->sc_ethernet != NULL &&
	    (sc->sc_flags & XIRC_ETHERNET_ENABLED) != 0)
		rval |= xi_intr(sc->sc_ethernet);
#endif

	return (rval);
}

int
xirc_enable(sc, flag, media)
	struct xirc_softc *sc;
	int flag, media;
{
	int error;

	if ((sc->sc_flags & flag) == flag) {
		printf("%s: already enabled\n", sc->sc_dev.dv_xname);
		return (0);
	}

	if ((sc->sc_flags & (XIRC_MODEM_ENABLED|XIRC_ETHERNET_ENABLED)) != 0) {
		sc->sc_flags |= flag;
		return (0);
	}

	/*
	 * Establish our interrupt handler.
	 *
	 * XXX Note, we establish this at IPL_NET.  This is suboptimal
	 * XXX the Modem portion, but is necessary to make the Ethernet
	 * XXX portion have the correct interrupt level semantics.
	 *
	 * XXX Eventually we should use the `enabled' bits in the
	 * XXX flags word to determine which level we should be at.
	 */
	sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_NET, xirc_intr, sc);
	if (!sc->sc_ih)
		return (EIO);

	error = pcmcia_function_enable(sc->sc_pf);
	if (error) {
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
		sc->sc_ih = 0;
		return (error);
	}

	sc->sc_flags |= flag;

	if (sc->sc_chipset < XI_CHIPSET_DINGO &&
	    sc->sc_id & (XIMEDIA_MODEM << 8)) {
		sc->sc_mako_intmask |= media;
		bus_space_write_1(sc->sc_ethernet_pcioh.iot,
		    sc->sc_ethernet_pcioh.ioh, 0x10, sc->sc_mako_intmask);
	}

	return (0);
}

void
xirc_disable(sc, flag, media)
	struct xirc_softc *sc;
	int flag, media;
{

	if ((sc->sc_flags & flag) == 0) {
		printf("%s: already disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	if (sc->sc_chipset < XI_CHIPSET_DINGO &&
	    sc->sc_id & (XIMEDIA_MODEM << 8)) {
		sc->sc_mako_intmask &= ~media;
		bus_space_write_1(sc->sc_ethernet_pcioh.iot,
		    sc->sc_ethernet_pcioh.ioh, 0x10, sc->sc_mako_intmask);
	}

	sc->sc_flags &= ~flag;
	if ((sc->sc_flags & (XIRC_MODEM_ENABLED|XIRC_ETHERNET_ENABLED)) != 0)
		return;

	pcmcia_function_disable(sc->sc_pf);
	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
	sc->sc_ih = 0;
}

/****** Here begins the com attachment code. ******/

#if NCOM_XIRC > 0
int	com_xirc_match(struct device *, struct cfdata *, void *);
void	com_xirc_attach(struct device *, struct device *, void *);
int	com_xirc_detach(struct device *, int);

/* No xirc-specific goo in the softc; it's all in the parent. */
CFATTACH_DECL(com_xirc, sizeof(struct com_softc),
    com_xirc_match, com_xirc_attach, com_detach, com_activate);

int	com_xirc_enable(struct com_softc *);
void	com_xirc_disable(struct com_softc *);

int
com_xirc_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	extern struct cfdriver com_cd;
	const char *name = aux;

	if (strcmp(name, com_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
com_xirc_attach(struct device *parent, struct device *self, void *aux __unused)
{
	struct com_softc *sc = (void *)self;
	struct xirc_softc *msc = (void *)parent;

	aprint_normal("\n");

	COM_INIT_REGS(sc->sc_regs, 
	    msc->sc_modem_pcioh.iot,
	    msc->sc_modem_pcioh.ioh,
	    -1);

	sc->enabled = 1;

	sc->sc_frequency = COM_FREQ;

	sc->enable = com_xirc_enable;
	sc->disable = com_xirc_disable;

	aprint_normal("%s", self->dv_xname);

	com_attach_subr(sc);

	sc->enabled = 0;
}

int
com_xirc_enable(sc)
	struct com_softc *sc;
{
	struct xirc_softc *msc =
	    (struct xirc_softc *)device_parent(&sc->sc_dev);

	return (xirc_enable(msc, XIRC_MODEM_ENABLED, XIMEDIA_MODEM));
}

void
com_xirc_disable(sc)
	struct com_softc *sc;
{
	struct xirc_softc *msc =
	    (struct xirc_softc *)device_parent(&sc->sc_dev);

	xirc_disable(msc, XIRC_MODEM_ENABLED, XIMEDIA_MODEM);
}

#endif /* NCOM_XIRC > 0 */

/****** Here begins the xi attachment code. ******/

#if NXI_XIRC > 0
int	xi_xirc_match(struct device *, struct cfdata *, void *);
void	xi_xirc_attach(struct device *, struct device *, void *);

/* No xirc-specific goo in the softc; it's all in the parent. */
CFATTACH_DECL(xi_xirc, sizeof(struct xi_softc),
    xi_xirc_match, xi_xirc_attach, xi_detach, xi_activate);

int	xi_xirc_enable(struct xi_softc *);
void	xi_xirc_disable(struct xi_softc *);
int	xi_xirc_lan_nid_ciscallback(struct pcmcia_tuple *, void *);

int
xi_xirc_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	extern struct cfdriver xi_cd;
	const char *name = aux;

	if (strcmp(name, xi_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
xi_xirc_attach(struct device *parent, struct device *self, void *aux __unused)
{
	struct xi_softc *sc = (void *)self;
	struct xirc_softc *msc = (void *)parent;
	u_int8_t myla[ETHER_ADDR_LEN];

	aprint_normal("\n");

	sc->sc_bst = msc->sc_ethernet_pcioh.iot;
	sc->sc_bsh = msc->sc_ethernet_pcioh.ioh;

	sc->sc_chipset = msc->sc_chipset;

	sc->sc_enable = xi_xirc_enable;
	sc->sc_disable = xi_xirc_disable;

	if (!pcmcia_scan_cis(device_parent(&msc->sc_dev),
	    xi_xirc_lan_nid_ciscallback, myla)) {
		aprint_error("%s: can't find MAC address\n", self->dv_xname);
		return;
	}

	/* Perform generic initialization. */
	xi_attach(sc, myla);
}

int
xi_xirc_enable(sc)
	struct xi_softc *sc;
{
	struct xirc_softc *msc =
	    (struct xirc_softc *)device_parent(&sc->sc_dev);

	return (xirc_enable(msc, XIRC_ETHERNET_ENABLED, XIMEDIA_ETHER));
}

void
xi_xirc_disable(sc)
	struct xi_softc *sc;
{
	struct xirc_softc *msc =
	    (struct xirc_softc *)device_parent(&sc->sc_dev);

	xirc_disable(msc, XIRC_ETHERNET_ENABLED, XIMEDIA_ETHER);
}

int
xi_xirc_lan_nid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int8_t *myla = arg;
	int i;

	if (tuple->length < 2)
		return (0);

	switch (tuple->code) {
	case PCMCIA_CISTPL_FUNCE:
		switch (pcmcia_tuple_read_1(tuple, 0)) {
		case PCMCIA_TPLFE_TYPE_LAN_NID:
			if (pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN)
				return (0);
			for (i = 0; i < ETHER_ADDR_LEN; i++)
				myla[i] = pcmcia_tuple_read_1(tuple, i + 2);
			return (1);

		case 0x02:
			/*
			 * Not sure about this, I don't have a CE2
			 * that puts the ethernet addr here.
			 */
		 	if (pcmcia_tuple_read_1(tuple, 1) != 0x01 ||
			    pcmcia_tuple_read_1(tuple, 2) != ETHER_ADDR_LEN)
				return (0);
			for (i = 0; i < ETHER_ADDR_LEN; i++)
				myla[i] = pcmcia_tuple_read_1(tuple, i + 3);
			return (1);
		}

	case 0x89:
		if (pcmcia_tuple_read_1(tuple, 0) != 0x04 ||
		    pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN)
			return (0);
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			myla[i] = pcmcia_tuple_read_1(tuple, i + 2);
		return (1);
	}

	return (0);
}

#endif /* NXI_XIRC > 0 */
