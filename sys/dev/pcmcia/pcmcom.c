/*	$NetBSD: pcmcom.c,v 1.6.8.2 2002/02/11 20:10:09 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks, Inc.
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
 * Device driver for multi-port PCMCIA serial cards, written by
 * Jason R. Thorpe for RedBack Networks, Inc.
 *
 * Most of these cards are simply multiple UARTs sharing a single interrupt
 * line, and rely on the fact that PCMCIA level-triggered interrupts can
 * be shared.  There are no special interrupt registers on them, as there
 * are on most ISA multi-port serial cards.
 *
 * If there are other cards that have interrupt registers, they should not
 * be glued into this driver.  Rather, separate drivers should be written
 * for those devices, as we have in the ISA multi-port serial card case.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcmcom.c,v 1.6.8.2 2002/02/11 20:10:09 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h> 
#include <sys/termios.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h> 

#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciadevs.h>

#include "com.h"
#include "pcmcom.h"

#include "locators.h"

struct pcmcom_slave_info {
	struct pcmcia_io_handle psi_pcioh;	/* PCMCIA i/o space info */
	int psi_io_window;			/* our i/o window */
	struct device *psi_child;		/* child's device */
};

struct pcmcom_softc {
	struct device sc_dev;			/* generic device glue */

	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handle */
	int sc_enabled_count;			/* enabled count */

	struct pcmcom_slave_info *sc_slaves;	/* slave info */
	int sc_nslaves;				/* slave count */
};

struct pcmcom_attach_args {
	bus_space_tag_t pca_iot;		/* I/O tag */
	bus_space_handle_t pca_ioh;		/* I/O handle */
	int pca_slave;				/* slave # */
};

int	pcmcom_match __P((struct device *, struct cfdata *, void *));
void	pcmcom_attach __P((struct device *, struct device *, void *));
int	pcmcom_detach __P((struct device *, int));
int	pcmcom_activate __P((struct device *, enum devact));

struct cfattach pcmcom_ca = {
	sizeof(struct pcmcom_softc), pcmcom_match, pcmcom_attach,
	    pcmcom_detach, pcmcom_activate
};

const struct pcmcom_product {
	struct pcmcia_product pp_product;
	int		pp_nslaves;		/* number of slaves */
} pcmcom_products[] = {
	{ { PCMCIA_STR_SOCKET_DUAL_RS232,	PCMCIA_VENDOR_SOCKET,
	    PCMCIA_PRODUCT_SOCKET_DUAL_RS232,	0 },
	  2 },

	{ { NULL } }
};

int	pcmcom_print __P((void *, const char *));
int	pcmcom_submatch __P((struct device *, struct cfdata *, void *));

int	pcmcom_check_cfe __P((struct pcmcom_softc *,
	    struct pcmcia_config_entry *));
void	pcmcom_attach_slave __P((struct pcmcom_softc *, int));

int	pcmcom_enable __P((struct pcmcom_softc *));
void	pcmcom_disable __P((struct pcmcom_softc *));

int	pcmcom_intr __P((void *));

int
pcmcom_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa,
	    (const struct pcmcia_product *)pcmcom_products,
	    sizeof pcmcom_products[0], NULL) != NULL)
		return (10);	/* beat com_pcmcia */
	return (0);
}

void
pcmcom_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcmcom_softc *sc = (struct pcmcom_softc *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	const struct pcmcom_product *pp;
	size_t size;
	int i;

	sc->sc_pf = pa->pf;

	pp = (const struct pcmcom_product *)pcmcia_product_lookup(pa,
            (const struct pcmcia_product *)pcmcom_products,
            sizeof pcmcom_products[0], NULL);
	if (pp == NULL) {
		printf("\n");
		panic("pcmcom_attach: impossible");
	}

	printf(": %s\n", pp->pp_product.pp_name);

	/* Allocate the slave info. */
	sc->sc_nslaves = pp->pp_nslaves;
	size = sizeof(struct pcmcom_slave_info) * sc->sc_nslaves;
	sc->sc_slaves = malloc(size, M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sc->sc_slaves == NULL) {
		printf("%s: unable to allocate slave info\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * The address decoders on these cards are stupid.  They decode
	 * (usually) 10 bits of address, so you need to allocate the
	 * regions they request in order for the card to differentiate
	 * between serial ports.
	 */
	for (cfe = pa->pf->cfe_head.sqh_first; cfe != NULL;
	     cfe = cfe->cfe_list.sqe_next) {
		if (pcmcom_check_cfe(sc, cfe)) {
			/* Found one! */
			break;
		}
	}
	if (cfe == NULL) {
		printf("%s: unable to find a suitable config table entry\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(sc->sc_pf)) {
		printf("%s: function enable failed\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_enabled_count = 1;

	/* Attach the children. */
	for (i = 0; i < sc->sc_nslaves; i++)
		pcmcom_attach_slave(sc, i);

	sc->sc_enabled_count = 0;
	pcmcia_function_disable(sc->sc_pf);
}

int
pcmcom_check_cfe(sc, cfe)
	struct pcmcom_softc *sc;
	struct pcmcia_config_entry *cfe;
{
	struct pcmcom_slave_info *psi;
	int slave;

	/* We need to have the same number of I/O spaces as we do slaves. */
	if (cfe->num_iospace != sc->sc_nslaves)
		return (0);

	for (slave = 0; slave < sc->sc_nslaves; slave++) {
		psi = &sc->sc_slaves[slave];
		if (pcmcia_io_alloc(sc->sc_pf,
		    cfe->iospace[slave].start,
		    cfe->iospace[slave].length,
		    cfe->iospace[slave].length,
		    &psi->psi_pcioh))
			goto release;
	}

	/* If we got here, we were able to allocate space for all slaves! */
	return (1);

 release:
	/* Release the i/o spaces we've allocated. */
	for (slave--; slave >= 0; slave--) {
		psi = &sc->sc_slaves[slave];
		pcmcia_io_free(sc->sc_pf, &psi->psi_pcioh);
	}
	return (0);
}

void
pcmcom_attach_slave(sc, slave)
	struct pcmcom_softc *sc;
	int slave;
{
	struct pcmcom_slave_info *psi = &sc->sc_slaves[slave];
	struct pcmcom_attach_args pca;

	printf("%s: slave %d", sc->sc_dev.dv_xname, slave);

	if (pcmcia_io_map(sc->sc_pf, PCMCIA_WIDTH_IO8, 0, psi->psi_pcioh.size,
	    &psi->psi_pcioh, &psi->psi_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	printf("\n");

	pca.pca_iot = psi->psi_pcioh.iot;
	pca.pca_ioh = psi->psi_pcioh.ioh;
	pca.pca_slave = slave;

	psi->psi_child = config_found_sm(&sc->sc_dev, &pca, pcmcom_print,
	    pcmcom_submatch);
}

int
pcmcom_detach(self, flags)
	struct device *self;
	int flags;
{
	struct pcmcom_softc *sc = (struct pcmcom_softc *)self;
	struct pcmcom_slave_info *psi;
	int slave, error;

	for (slave = sc->sc_nslaves - 1; slave >= 0; slave--) {
		psi = &sc->sc_slaves[slave];
		if (psi->psi_child == NULL)
			continue;

		/* Detach the child. */
		if ((error = config_detach(psi->psi_child, flags)) != 0)
			return (error);
		psi->psi_child = NULL;

		/* Unmap the i/o window. */
		pcmcia_io_unmap(sc->sc_pf, psi->psi_io_window);

		/* Free the i/o space. */
		pcmcia_io_free(sc->sc_pf, &psi->psi_pcioh);
	}
	return (0);
}

int
pcmcom_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct pcmcom_softc *sc = (struct pcmcom_softc *)self;
	struct pcmcom_slave_info *psi;
	int slave, error = 0, s;

	s = splserial();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (slave = sc->sc_nslaves - 1; slave >= 0; slave--) {
			psi = &sc->sc_slaves[slave];
			if (psi->psi_child == NULL)
				continue;

			/*
			 * Deactivate the child.  Doing so will cause our
			 * own enabled count to drop to 0, once all children
			 * are deactivated.
			 */
			if ((error = config_deactivate(psi->psi_child)) != 0)
				break;
		}
		break;
	}
	splx(s);
	return (error);
}

int
pcmcom_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcmcom_attach_args *pca = aux;

	/* only com's can attach to pcmcom's; easy... */
	if (pnp)
		printf("com at %s", pnp);

	printf(" slave %d", pca->pca_slave);

	return (UNCONF);
}

int
pcmcom_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcmcom_attach_args *pca = aux;

	if (cf->cf_loc[PCMCOMCF_SLAVE] != pca->pca_slave &&
	    cf->cf_loc[PCMCOMCF_SLAVE] != PCMCOMCF_SLAVE_DEFAULT)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
pcmcom_intr(arg)
	void *arg;
{
#if NCOM > 0
	struct pcmcom_softc *sc = arg;
	int i, rval = 0;

	if (sc->sc_enabled_count == 0)
		return (0);

	for (i = 0; i < sc->sc_nslaves; i++) {
		if (sc->sc_slaves[i].psi_child != NULL)
			rval |= comintr(sc->sc_slaves[i].psi_child);
	}

	return (rval);
#else
	return (0);
#endif /* NCOM > 0 */
}

int
pcmcom_enable(sc)
	struct pcmcom_softc *sc;
{

	sc->sc_enabled_count++;
	if (sc->sc_enabled_count > 1)
		return (0);

	/* Establish the interrupt. */
	sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_SERIAL,
	    pcmcom_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (pcmcia_function_enable(sc->sc_pf));
}

void
pcmcom_disable(sc)
	struct pcmcom_softc *sc;
{

	sc->sc_enabled_count--;
	if (sc->sc_enabled_count != 0)
		return;

	pcmcia_function_disable(sc->sc_pf);
	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
}

/****** Here begins the com attachment code. ******/

#if NCOM_PCMCOM > 0
int	com_pcmcom_match __P((struct device *, struct cfdata *, void *));
void	com_pcmcom_attach __P((struct device *, struct device *, void *));

/* No pcmcom-specific goo in the softc; it's all in the parent. */
struct cfattach com_pcmcom_ca = {
	sizeof(struct com_softc), com_pcmcom_match, com_pcmcom_attach,
	    com_detach, com_activate
};

int	com_pcmcom_enable __P((struct com_softc *));
void	com_pcmcom_disable __P((struct com_softc *));

int
com_pcmcom_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* Device is always present. */
	return (1);
}

void
com_pcmcom_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (struct com_softc *)self;
	struct pcmcom_attach_args *pca = aux;

	sc->sc_iot = pca->pca_iot;
	sc->sc_ioh = pca->pca_ioh;

	sc->enabled = 1;

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	sc->enable = com_pcmcom_enable;
	sc->disable = com_pcmcom_disable;

	com_attach_subr(sc);

	sc->enabled = 0;
}

int
com_pcmcom_enable(sc)
	struct com_softc *sc;
{

	return (pcmcom_enable((struct pcmcom_softc *)sc->sc_dev.dv_parent));
}

void
com_pcmcom_disable(sc)
	struct com_softc *sc;
{

	pcmcom_disable((struct pcmcom_softc *)sc->sc_dev.dv_parent);
}
#endif /* NCOM_PCMCOM > 0 */
