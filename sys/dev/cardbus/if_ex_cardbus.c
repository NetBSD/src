/*	$NetBSD: if_ex_cardbus.c,v 1.9 1999/11/17 08:49:16 thorpej Exp $	*/

/*
 * CardBus specific routines for 3Com 3C575-family CardBus ethernet adapter
 *
 * Copyright (c) 1998 and 1999
 *       HAYAKAWA Koichi.  All rights reserved.
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
 *      This product includes software developed by the author.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HAYAKAWA KOICHI ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL TAKESHI OHASHI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */

/* #define EX_DEBUG 4 */	/* define to report infomation for debugging */

#define EX_POWER_STATIC		/* do not use enable/disable functions */
				/* I'm waiting elinkxl.c uses
                                   sc->enable and sc->disable
                                   functions. */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h> 
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>

#include <dev/mii/miivar.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>
#include <dev/ic/elinkxlreg.h>
#include <dev/ic/elinkxlvar.h>

#if defined DEBUG && !defined EX_DEBUG
#define EX_DEBUG
#endif

#if defined EX_DEBUG
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

#define CARDBUS_3C575BTX_FUNCSTAT_PCIREG  CARDBUS_BASE2_REG  /* means 0x18 */
#define EX_CB_INTR 4		/* intr acknowledge reg. CardBus only */
#define EX_CB_INTR_ACK 0x8000 /* intr acknowledge bit */

int ex_cardbus_match __P((struct device *, struct cfdata *, void *));
void ex_cardbus_attach __P((struct device *, struct device *,void *));
int ex_cardbus_detach __P((struct device *, int));
void ex_cardbus_intr_ack __P((struct ex_softc *));

#if !defined EX_POWER_STATIC
int ex_cardbus_enable __P((struct ex_softc *sc));
void ex_cardbus_disable __P((struct ex_softc *sc));
#endif /* !defined EX_POWER_STATIC */

struct ex_cardbus_softc {
	struct ex_softc sc_softc;

	cardbus_devfunc_t sc_ct;
	int sc_intrline;
	u_int8_t sc_cardbus_flags;
#define EX_REATTACH		0x01
#define EX_ABSENT		0x02
	u_int8_t sc_cardtype;
#define EX_3C575		1
#define EX_3C575B		2

	/* CardBus function status space.  575B requests it. */
	bus_space_tag_t sc_funct;
	bus_space_handle_t sc_funch;
};

struct cfattach ex_cardbus_ca = {
	sizeof(struct ex_cardbus_softc), ex_cardbus_match,
	    ex_cardbus_attach, ex_cardbus_detach
};

const struct ex_cardbus_product {
	u_int32_t	ecp_prodid;	/* CardBus product ID */
	int		ecp_flags;	/* initial softc flags */
	pcireg_t	ecp_csr;	/* PCI CSR flags */
	int		ecp_cardtype;	/* card type */
	const char	*ecp_name;	/* device name */
} ex_cardbus_products[] = {
	{ CARDBUS_PRODUCT_3COM_3C575TX,
	  EX_CONF_MII,
	  CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE,
	  EX_3C575,
	  "3c575-TX Ethernet" },

	{ CARDBUS_PRODUCT_3COM_3C575BTX,
	  EX_CONF_90XB|EX_CONF_MII,
	  CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MEM_ENABLE |
	      CARDBUS_COMMAND_MASTER_ENABLE,
	  EX_3C575B,
	  "3c575B-TX Ethernet" },

	{ 0,
	  0,
	  0,
	  NULL },
};

const struct ex_cardbus_product *ex_cardbus_lookup
    __P((const struct cardbus_attach_args *));

const struct ex_cardbus_product *
ex_cardbus_lookup(ca)
	const struct cardbus_attach_args *ca;
{
	const struct ex_cardbus_product *ecp;

	if (CARDBUS_VENDOR(ca->ca_id) != CARDBUS_VENDOR_3COM)
		return (NULL);

	for (ecp = ex_cardbus_products; ecp->ecp_name != NULL; ecp++)
		if (CARDBUS_PRODUCT(ca->ca_id) == ecp->ecp_prodid)
			return (ecp);
	return (NULL);
}

int
ex_cardbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct cardbus_attach_args *ca = aux;

	if (ex_cardbus_lookup(ca) != NULL)
		return (1);

	return (0);
}

void
ex_cardbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ex_cardbus_softc *psc = (void *)self;
	struct ex_softc *sc = &psc->sc_softc;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbusreg_t iob, command, bhlc;
	const struct ex_cardbus_product *ecp;
	bus_space_handle_t ioh;
	bus_addr_t adr;

	if (Cardbus_mapreg_map(ct, CARDBUS_BASE0_REG, CARDBUS_MAPREG_TYPE_IO, 0,
	    &(sc->sc_iot), &ioh, &adr, NULL)) {
		printf(": can't map i/o space\n");
		return;
	}

	ecp = ex_cardbus_lookup(ca);
	if (ecp == NULL) {
		printf("\n");
		panic("ex_cardbus_attach: impossible");
	}

	printf(": 3Com %s\n", ecp->ecp_name);

#if !defined EX_POWER_STATIC
	sc->enable = ex_cardbus_enable;
	sc->disable = ex_cardbus_disable;
#else
	sc->enable = NULL;
	sc->disable = NULL;
#endif  
	sc->enabled = 1;

	sc->sc_dmat = ca->ca_dmat;

	sc->ex_bustype = EX_BUS_CARDBUS;
	sc->ex_conf = ecp->ecp_flags;

	iob = adr;
	sc->sc_ioh = ioh;

#if rbus
#else
	(ct->ct_cf->cardbus_io_open)(cc, 0, iob, iob + 0x40);
#endif
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);

	command = cardbus_conf_read(cc, cf, ca->ca_tag,
	    CARDBUS_COMMAND_STATUS_REG);
	command |= ecp->ecp_csr;
	psc->sc_cardtype = ecp->ecp_cardtype;

	if (psc->sc_cardtype == EX_3C575B) {
		/* Map CardBus function status window. */
		if (Cardbus_mapreg_map(ct, CARDBUS_3C575BTX_FUNCSTAT_PCIREG,
		    CARDBUS_MAPREG_TYPE_MEM, 0, &psc->sc_funct,
		    &psc->sc_funch, 0, NULL)) {
			printf("%s: unable to map function status window\n",
			    self->dv_xname);
			return;
		}

		/*
		 * Make sure CardBus brigde can access memory space.  Usually
		 * memory access is enabled by BIOS, but some BIOSes do not
		 * enable it.
		 */
		(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);

		/* Setup interrupt acknowledge hook */
		sc->intr_ack = ex_cardbus_intr_ack;
	}

	cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG,
	    command);
  
 	/*
	 * set latency timmer
	 */
	bhlc = cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG);
	if (CARDBUS_LATTIMER(bhlc) < 0x20) {
		/* at least the value of latency timer should 0x20. */
		DPRINTF(("if_ex_cardbus: lattimer 0x%x -> 0x20\n",
		    CARDBUS_LATTIMER(bhlc)));
		bhlc &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
		bhlc |= (0x20 << CARDBUS_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG, bhlc);
	}

	psc->sc_ct = ca->ca_ct;
	psc->sc_intrline = ca->ca_intrline;

#if defined EX_POWER_STATIC
	/* Map and establish the interrupt. */

	sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_NET,
	    ex_intr, psc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		printf(" at %d", ca->ca_intrline);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
	    ca->ca_intrline);
#endif

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ELINK_COMMAND, GLOBAL_RESET);
	delay(400);
	{
		int i = 0;
		while (bus_space_read_2(sc->sc_iot, sc->sc_ioh, ELINK_STATUS) &
		    S_COMMAND_IN_PROGRESS) {
			if (++i > 10000) {
				printf("ex: timeout %x\n",
				    bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				        ELINK_STATUS));
				printf("ex: addr %x\n",
				    cardbus_conf_read(cc, cf, ca->ca_tag,
				    CARDBUS_BASE0_REG));
				return;		/* emergency exit */
			}
		}
	}

	ex_config(sc);

	if (psc->sc_cardtype == EX_3C575B)
		bus_space_write_4(psc->sc_funct, psc->sc_funch,
		    EX_CB_INTR, EX_CB_INTR_ACK);

#if !defined EX_POWER_STATIC
	cardbus_function_disable(psc->sc_ct);  
	sc->enabled = 0;
#endif
}

void
ex_cardbus_intr_ack(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *psc = (struct ex_cardbus_softc *)sc;

	bus_space_write_4(psc->sc_funct, psc->sc_funch, EX_CB_INTR,
	    EX_CB_INTR_ACK);
}

int
ex_cardbus_detach(self, arg)
	struct device *self;
	int arg;
{
	struct ex_cardbus_softc *psc = (void *)self;
	struct ex_softc *sc = &psc->sc_softc;
	cardbus_function_tag_t cf = psc->sc_ct->ct_cf;
	cardbus_chipset_tag_t cc = psc->sc_ct->ct_cc;

	/*
	 * XXX Currently, no detach.
	 */
	printf("- ex_cardbus_detach\n");

	cardbus_intr_disestablish(cc, cf, sc->sc_ih);

	sc->enabled = 0;

	return (EBUSY);
}

#if !defined EX_POWER_STATIC
int
ex_cardbus_enable(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
	cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
	cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

	Cardbus_function_enable(csc->sc_ct);
	cardbus_restore_bar(csc->sc_ct);

	sc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline,
	    IPL_NET, ex_intr, sc);
	if (NULL == sc->sc_ih) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (0);
}

void
ex_cardbus_disable(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
	cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
	cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

	cardbus_save_bar(csc->sc_ct);
  
 	Cardbus_function_disable(csc->sc_ct);

	cardbus_intr_disestablish(cc, cf, sc->sc_ih);
}
#endif /* EX_POWER_STATIC */
