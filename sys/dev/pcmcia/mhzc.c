/*	$NetBSD: mhzc.c,v 1.8.8.1 2002/06/20 16:33:57 gehenna Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Device driver for the Megaherz X-JACK Ethernet/Modem combo cards.
 *
 * Many thanks to Chuck Cranor for having the patience to sift through
 * the Linux smc91c92_cs.c driver to find the magic details to get this
 * working!
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mhzc.c,v 1.8.8.1 2002/06/20 16:33:57 gehenna Exp $");

#include "opt_inet.h" 
#include "opt_ns.h"
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
        
#ifdef NS 
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif  

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include "mhzc.h"

struct mhzc_softc {
	struct device sc_dev;		/* generic device glue */

	struct pcmcia_function *sc_pf;	/* our PCMCIA function */
	void *sc_ih;			/* interrupt handle */

	const struct mhzc_product *sc_product;

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
};

/* sc_flags */
#define	MHZC_MODEM_MAPPED	0x01
#define	MHZC_ETHERNET_MAPPED	0x02
#define	MHZC_MODEM_ENABLED	0x04
#define	MHZC_ETHERNET_ENABLED	0x08
#define	MHZC_IOSPACE_ALLOCED	0x10

int	mhzc_match __P((struct device *, struct cfdata *, void *));
void	mhzc_attach __P((struct device *, struct device *, void *));
int	mhzc_detach __P((struct device *, int));
int	mhzc_activate __P((struct device *, enum devact));

struct cfattach mhzc_ca = {
	sizeof(struct mhzc_softc), mhzc_match, mhzc_attach,
	    mhzc_detach, mhzc_activate
};

int	mhzc_em3336_enaddr __P((struct mhzc_softc *, u_int8_t *));
int	mhzc_em3336_enable __P((struct mhzc_softc *));

const struct mhzc_product {
	struct pcmcia_product mp_product;

	/* Get the Ethernet address for this card. */
	int		(*mp_enaddr) __P((struct mhzc_softc *, u_int8_t *));

	/* Perform any special `enable' magic. */
	int		(*mp_enable) __P((struct mhzc_softc *));
} mhzc_products[] = {
	{ { PCMCIA_STR_MEGAHERTZ_XJEM3336,	PCMCIA_VENDOR_MEGAHERTZ,
	    PCMCIA_PRODUCT_MEGAHERTZ_XJEM3336,	0 },
	  mhzc_em3336_enaddr,		mhzc_em3336_enable },

	/*
	 * Eventually we could add support for other Ethernet/Modem
	 * combo cards, even if they're aren't Megahertz, because
	 * most of them work more or less the same way.
	 */

	{ { NULL } }
};

int	mhzc_print __P((void *, const char *));

int	mhzc_check_cfe __P((struct mhzc_softc *, struct pcmcia_config_entry *));
int	mhzc_alloc_ethernet __P((struct mhzc_softc *, struct pcmcia_config_entry *));

int	mhzc_enable __P((struct mhzc_softc *, int));
void	mhzc_disable __P((struct mhzc_softc *, int));

int	mhzc_intr __P((void *));

int
mhzc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa,
	    (const struct pcmcia_product *)mhzc_products,
	    sizeof mhzc_products[0], NULL) != NULL)
		return (10);		/* beat `com' */

	return (0);
}

void
mhzc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mhzc_softc *sc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	sc->sc_pf = pa->pf;

	sc->sc_product = (const struct mhzc_product *)pcmcia_product_lookup(pa,
            (const struct pcmcia_product *)mhzc_products,
            sizeof mhzc_products[0], NULL);
	if (sc->sc_product == NULL) {
		printf("\n");
		panic("mhzc_attach: impossible");
	}

	printf(": %s\n", sc->sc_product->mp_product.pp_name);

	/*
	 * The address decoders on these cards are wacky.  The configuration
	 * entries are set up to look like serial ports, and have no
	 * information about the Ethernet portion.  In order to talk to
	 * the Modem portion, the I/O address must have bit 0x80 set.
	 * In order to talk to the Ethernet portion, the I/O address must
	 * have the 0x80 bit clear.
	 *
	 * The standard configuration entries conveniently have 0x80 set
	 * in them, and have a length of 8 (a 16550's size, convenient!),
	 * so we use those to set up the Modem portion.
	 *
	 * Once we have the Modem's address established, we search for
	 * an address suitable for the Ethernet portion.  We do this by
	 * rounding up to the next 16-byte aligned address where 0x80
	 * isn't set (the SMC Ethernet chip has a 16-byte address size)
	 * and attemping to allocate a 16-byte region until we succeed.
	 *
	 * Sure would have been nice if Megahertz had made the card a
	 * proper multi-function device.
	 */
	SIMPLEQ_FOREACH(cfe, &pa->pf->cfe_head, cfe_list) {
		if (mhzc_check_cfe(sc, cfe)) {
			/* Found one! */
			break;
		}
	}
	if (cfe == NULL) {
		printf("%s: unable to find suitable config table entry\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (mhzc_alloc_ethernet(sc, cfe) == 0) {
		printf("%s: unable to allocate space for Ethernet portion\n",
		    sc->sc_dev.dv_xname);
		goto alloc_ethernet_failed;
	}

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		goto enable_failed;
	}
	sc->sc_flags |= MHZC_IOSPACE_ALLOCED;

	if (sc->sc_product->mp_enable != NULL)
		(*sc->sc_product->mp_enable)(sc);

	sc->sc_modem = config_found(&sc->sc_dev, "com", mhzc_print);
	sc->sc_ethernet = config_found(&sc->sc_dev, "sm", mhzc_print);

	pcmcia_function_disable(pa->pf);
	return;

 enable_failed:
	/* Free the Ethernet's I/O space. */
	pcmcia_io_free(sc->sc_pf, &sc->sc_ethernet_pcioh);

 alloc_ethernet_failed:
	/* Free the Modem's I/O space. */
	pcmcia_io_free(sc->sc_pf, &sc->sc_modem_pcioh);
}

int
mhzc_check_cfe(sc, cfe)
	struct mhzc_softc *sc;
	struct pcmcia_config_entry *cfe;
{

	if (cfe->num_memspace != 0)
		return (0);

	if (cfe->num_iospace != 1)
		return (0);

	if (pcmcia_io_alloc(sc->sc_pf,
	    cfe->iospace[0].start,
	    cfe->iospace[0].length,
	    cfe->iospace[0].length,
	    &sc->sc_modem_pcioh) == 0) {
		/* Found one for the modem! */
		return (1);
	}

	return (0);
}

int
mhzc_alloc_ethernet(sc, cfe)
	struct mhzc_softc *sc;
	struct pcmcia_config_entry *cfe;
{
	bus_addr_t addr, maxaddr;

	addr = cfe->iospace[0].start + cfe->iospace[0].length;
	maxaddr = 0x1000;

	/*
	 * Now round it up so that it starts on a 16-byte boundary.
	 */
	addr = roundup(addr, 0x10);

	for (; (addr + 0x10) < maxaddr; addr += 0x10) {
		if (addr & 0x80)
			continue;
		if (pcmcia_io_alloc(sc->sc_pf, addr, 0x10, 0x10,
		    &sc->sc_ethernet_pcioh) == 0) {
			/* Found one for the ethernet! */
			return (1);
		}
	}

	return (0);
}

int
mhzc_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	const char *name = aux;

	if (pnp)
		printf("%s at %s(*)",  name, pnp);

	return (UNCONF);
}

int
mhzc_detach(self, flags)
	struct device *self;
	int flags;
{
	struct mhzc_softc *sc = (void *)self;
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
#ifdef not_necessary
		sc->sc_modem = NULL;
#endif
	}

	/* Unmap our i/o windows. */
	if (sc->sc_flags & MHZC_MODEM_MAPPED)
		pcmcia_io_unmap(sc->sc_pf, sc->sc_modem_io_window);
	if (sc->sc_flags & MHZC_ETHERNET_MAPPED)
		pcmcia_io_unmap(sc->sc_pf, sc->sc_ethernet_io_window);

	/* Free our i/o spaces. */
	if (sc->sc_flags & MHZC_IOSPACE_ALLOCED) {
		pcmcia_io_free(sc->sc_pf, &sc->sc_modem_pcioh);
		pcmcia_io_free(sc->sc_pf, &sc->sc_ethernet_pcioh);
	}

	return (0);
}

int
mhzc_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct mhzc_softc *sc = (void *)self;
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
mhzc_intr(arg)
	void *arg;
{
	struct mhzc_softc *sc = arg;
	int rval = 0;

#if NCOM_MHZC > 0
	if (sc->sc_modem != NULL &&
	    (sc->sc_flags & MHZC_MODEM_ENABLED) != 0)
		rval |= comintr(sc->sc_modem);
#endif

#if NSM_MHZC > 0
	if (sc->sc_ethernet != NULL &&
	    (sc->sc_flags & MHZC_ETHERNET_ENABLED) != 0)
		rval |= smc91cxx_intr(sc->sc_ethernet);
#endif

	return (rval);
}

int
mhzc_enable(sc, flag)
	struct mhzc_softc *sc;
	int flag;
{

	if (sc->sc_flags & flag) {
		printf("%s: %s already enabled\n", sc->sc_dev.dv_xname,
		    (flag & MHZC_MODEM_ENABLED) ? "modem" : "ethernet");
		panic("mhzc_enable");
	}

	if ((sc->sc_flags & (MHZC_MODEM_ENABLED|MHZC_ETHERNET_ENABLED)) != 0) {
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
	sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_NET,
	    mhzc_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	if (pcmcia_function_enable(sc->sc_pf)) {
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
		return (1);
	}

	/*
	 * Perform any special enable magic necessary.
	 */
	if (sc->sc_product->mp_enable != NULL &&
	    (*sc->sc_product->mp_enable)(sc) != 0) {
		pcmcia_function_disable(sc->sc_pf);
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
		return (1);
	}

	sc->sc_flags |= flag;
	return (0);
}

void
mhzc_disable(sc, flag)
	struct mhzc_softc *sc;
	int flag;
{

	if ((sc->sc_flags & flag) == 0) {
		printf("%s: %s already disabled\n", sc->sc_dev.dv_xname,
		    (flag & MHZC_MODEM_ENABLED) ? "modem" : "ethernet");
		panic("mhzc_disable");
	}

	sc->sc_flags &= ~flag;
	if ((sc->sc_flags & (MHZC_MODEM_ENABLED|MHZC_ETHERNET_ENABLED)) != 0)
		return;

	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
	pcmcia_function_disable(sc->sc_pf);
}

/*****************************************************************************
 * Megahertz EM3336 (and compatibles) support
 *****************************************************************************/

int	mhzc_em3336_lannid_ciscallback __P((struct pcmcia_tuple *, void *));
int	mhzc_em3336_ascii_enaddr __P((const char *cisstr, u_int8_t *));

int
mhzc_em3336_enaddr(sc, myla)
	struct mhzc_softc *sc;
	u_int8_t *myla;
{

	/* Get the station address from CIS tuple 0x81. */
	if (pcmcia_scan_cis(sc->sc_dev.dv_parent,
	    mhzc_em3336_lannid_ciscallback, myla) != 1) {
		printf("%s: unable to get Ethernet address from CIS\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	return (1);
}

int
mhzc_em3336_enable(sc)
	struct mhzc_softc *sc;
{
	struct pcmcia_mem_handle memh;
	bus_addr_t memoff;
	int memwin, reg;

	/*
	 * Bring the chip to live by touching its registers in the correct
	 * way (as per my reference... the Linux smc91c92_cs.c driver by
	 * David A. Hinds).
	 */
	
	/* Map the ISRPOWEREG. */
	if (pcmcia_mem_alloc(sc->sc_pf, 0x1000, &memh) != 0) {
		printf("%s: unable to allocate memory space\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	if (pcmcia_mem_map(sc->sc_pf, PCMCIA_MEM_ATTR, 0, 0x1000,
	    &memh, &memoff, &memwin)) {
		printf("%s: unable to map memory space\n",
		    sc->sc_dev.dv_xname);
		pcmcia_mem_free(sc->sc_pf, &memh);
		return (1);
	}

	/*
	 * The magic sequence:
	 *
	 *	- read/write the CCR option register.
	 *	- read the ISRPOWEREG 2 times.
	 *	- read/write the CCR option register again.
	 */

	reg = pcmcia_ccr_read(sc->sc_pf, PCMCIA_CCR_OPTION);
	pcmcia_ccr_write(sc->sc_pf, PCMCIA_CCR_OPTION, reg);

	reg = bus_space_read_1(memh.memt, memh.memh, 0x380);
	delay(5);
	reg = bus_space_read_1(memh.memt, memh.memh, 0x380);

	delay(200000);

	reg = pcmcia_ccr_read(sc->sc_pf, PCMCIA_CCR_OPTION);
	delay(5);
	pcmcia_ccr_write(sc->sc_pf, PCMCIA_CCR_OPTION, reg);

	pcmcia_mem_unmap(sc->sc_pf, memwin);
	pcmcia_mem_free(sc->sc_pf, &memh);

	return (0);
}

int
mhzc_em3336_lannid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int8_t *myla = arg, addr_str[ETHER_ADDR_LEN * 2];
	int i;

	if (tuple->code == 0x81) {
		/*
		 * We have a string-encoded address.  Length includes
		 * terminating 0xff.
		 */
		if (tuple->length != (ETHER_ADDR_LEN * 2) + 1)
			return (0);

		for (i = 0; i < tuple->length - 1; i++)
			addr_str[i] = pcmcia_tuple_read_1(tuple, i);

		/*
		 * Decode the string into `myla'.
		 */
		return (mhzc_em3336_ascii_enaddr(addr_str, myla));
	}
	return (0);
}

/* XXX This should be shared w/ if_sm_pcmcia.c */
int
mhzc_em3336_ascii_enaddr(cisstr, myla)
	const char *cisstr;
	u_int8_t *myla;
{
	u_int8_t digit;
	int i;

	memset(myla, 0, ETHER_ADDR_LEN);

	for (i = 0, digit = 0; i < (ETHER_ADDR_LEN * 2); i++) {
		if (cisstr[i] >= '0' && cisstr[i] <= '9')
			digit |= cisstr[i] - '0';
		else if (cisstr[i] >= 'a' && cisstr[i] <= 'f')
			digit |= (cisstr[i] - 'a') + 10;
		else if (cisstr[i] >= 'A' && cisstr[i] <= 'F')
			digit |= (cisstr[i] - 'A') + 10;
		else {
			/* Bogus digit!! */
			return (0);
		}

		/* Compensate for ordering of digits. */
		if (i & 1) {
			myla[i >> 1] = digit;
			digit = 0;
		} else
			digit <<= 4;
	}

	return (1);
}

/****** Here begins the com attachment code. ******/

#if NCOM_MHZC > 0
int	com_mhzc_match __P((struct device *, struct cfdata *, void *));
void	com_mhzc_attach __P((struct device *, struct device *, void *));
int	com_mhzc_detach __P((struct device *, int));

/* No mhzc-specific goo in the softc; it's all in the parent. */
struct cfattach com_mhzc_ca = {
	sizeof(struct com_softc), com_mhzc_match, com_mhzc_attach,
	    com_detach, com_activate
};

int	com_mhzc_enable __P((struct com_softc *));
void	com_mhzc_disable __P((struct com_softc *));

int
com_mhzc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	extern struct cfdriver com_cd;
	const char *name = aux;

	/* Device is always present. */
	if (strcmp(name, com_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
com_mhzc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct mhzc_softc *msc = (void *)parent;

	printf(":");
	if (pcmcia_io_map(msc->sc_pf, PCMCIA_WIDTH_IO8, 0,
	    msc->sc_modem_pcioh.size, &msc->sc_modem_pcioh,
	    &msc->sc_modem_io_window)) {
		printf("unable to map I/O space\n");
		return;
	}
	printf(" io 0x%x-0x%x",
	    (int)msc->sc_modem_pcioh.addr,
	    (int)(msc->sc_modem_pcioh.addr + msc->sc_modem_pcioh.size - 1));

	msc->sc_flags |= MHZC_MODEM_MAPPED;

	sc->sc_iot = msc->sc_modem_pcioh.iot;
	sc->sc_ioh = msc->sc_modem_pcioh.ioh;

	sc->enabled = 1;

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	sc->enable = com_mhzc_enable;
	sc->disable = com_mhzc_disable;

	com_attach_subr(sc);

	sc->enabled = 0;
}

int
com_mhzc_enable(sc)
	struct com_softc *sc;
{

	return (mhzc_enable((struct mhzc_softc *)sc->sc_dev.dv_parent,
	    MHZC_MODEM_ENABLED));
}

void
com_mhzc_disable(sc)
	struct com_softc *sc;
{

	mhzc_disable((struct mhzc_softc *)sc->sc_dev.dv_parent,
	    MHZC_MODEM_ENABLED);
}

#endif /* NCOM_MHZC > 0 */

/****** Here begins the sm attachment code. ******/

#if NSM_MHZC > 0
int	sm_mhzc_match __P((struct device *, struct cfdata *, void *));
void	sm_mhzc_attach __P((struct device *, struct device *, void *));

/* No mhzc-specific goo in the softc; it's all in the parent. */
struct cfattach sm_mhzc_ca = {
	sizeof(struct smc91cxx_softc), sm_mhzc_match, sm_mhzc_attach,
	    smc91cxx_detach, smc91cxx_activate
};

int	sm_mhzc_enable __P((struct smc91cxx_softc *));
void	sm_mhzc_disable __P((struct smc91cxx_softc *));

int
sm_mhzc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	extern struct cfdriver sm_cd;
	const char *name = aux;

	/* Device is always present. */
	if (strcmp(name, sm_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
sm_mhzc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct smc91cxx_softc *sc = (void *)self;
	struct mhzc_softc *msc = (void *)parent;
	u_int8_t myla[ETHER_ADDR_LEN];

	printf(":");
	if (pcmcia_io_map(msc->sc_pf, PCMCIA_WIDTH_IO16, 0,
	    msc->sc_ethernet_pcioh.size, &msc->sc_ethernet_pcioh,
	    &msc->sc_ethernet_io_window)) {
		printf("unable to map I/O space\n");
		return;
	}
	printf(" io 0x%x-0x%x\n",
	   (int)msc->sc_ethernet_pcioh.addr,
	   (int)(msc->sc_ethernet_pcioh.addr + msc->sc_modem_pcioh.size - 1));

	msc->sc_flags |= MHZC_ETHERNET_MAPPED;

	sc->sc_bst = msc->sc_ethernet_pcioh.iot;
	sc->sc_bsh = msc->sc_ethernet_pcioh.ioh;

	sc->sc_enable = sm_mhzc_enable;
	sc->sc_disable = sm_mhzc_disable;

	if ((*msc->sc_product->mp_enaddr)(msc, myla) != 1)
		return;

	/* Perform generic initialization. */
	smc91cxx_attach(sc, myla);
}

int
sm_mhzc_enable(sc)
	struct smc91cxx_softc *sc;
{

	return (mhzc_enable((struct mhzc_softc *)sc->sc_dev.dv_parent,
	    MHZC_ETHERNET_ENABLED));
}

void
sm_mhzc_disable(sc)
	struct smc91cxx_softc *sc;
{

	mhzc_disable((struct mhzc_softc *)sc->sc_dev.dv_parent,
	    MHZC_ETHERNET_ENABLED);
}

#endif /* NSM_MHZC > 0 */
