/*	$NetBSD: if_ex_cardbus.c,v 1.19.2.2 2001/08/24 00:09:06 nathanw Exp $	*/

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

/* #define EX_DEBUG 4 */	/* define to report information for debugging */

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

int ex_cardbus_enable __P((struct ex_softc *));
void ex_cardbus_disable __P((struct ex_softc *));
void ex_cardbus_power __P((struct ex_softc *, int));

struct ex_cardbus_softc {
	struct ex_softc sc_softc;

	cardbus_devfunc_t sc_ct;
	int sc_intrline;
	u_int8_t sc_cardbus_flags;
#define EX_REATTACH		0x01
#define EX_ABSENT		0x02
	u_int8_t sc_cardtype;
#define EX_CB_BOOMERANG		1
#define EX_CB_CYCLONE		2

	/* CardBus function status space.  575B requests it. */
	bus_space_tag_t sc_funct;
	bus_space_handle_t sc_funch;
	bus_size_t sc_funcsize;

	bus_size_t sc_mapsize;		/* the size of mapped bus space region */
	
	cardbustag_t sc_tag;

	int	sc_csr;			/* CSR bits */
	int	sc_bar_reg;		/* which BAR to use */
	pcireg_t sc_bar_val;		/* value of the BAR */
	int	sc_bar_reg1;		/* which BAR to use */
	pcireg_t sc_bar_val1;		/* value of the BAR */
	
};

struct cfattach ex_cardbus_ca = {
	sizeof(struct ex_cardbus_softc), ex_cardbus_match,
	    ex_cardbus_attach, ex_cardbus_detach, ex_activate
};

const struct ex_cardbus_product {
	u_int32_t	ecp_prodid;	/* CardBus product ID */
	int		ecp_flags;	/* initial softc flags */
	pcireg_t	ecp_csr;	/* PCI CSR flags */
	int		ecp_cardtype;	/* card type */
	const char	*ecp_name;	/* device name */
} ex_cardbus_products[] = {
	{ CARDBUS_PRODUCT_3COM_3C575TX,
	  EX_CONF_MII | EX_CONF_EEPROM_OFF | EX_CONF_EEPROM_8BIT,
	  CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE,
	  EX_CB_BOOMERANG,
	  "3c575-TX Ethernet" },

	{ CARDBUS_PRODUCT_3COM_3C575BTX,
	  EX_CONF_90XB|EX_CONF_MII|EX_CONF_INV_LED_POLARITY |
	    EX_CONF_EEPROM_OFF | EX_CONF_EEPROM_8BIT,
	  CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MEM_ENABLE |
	      CARDBUS_COMMAND_MASTER_ENABLE,
	  EX_CB_CYCLONE,
	  "3c575B-TX Ethernet" },

	{ CARDBUS_PRODUCT_3COM_3C575CTX,
	  EX_CONF_90XB | EX_CONF_PHY_POWER | EX_CONF_EEPROM_OFF |
	    EX_CONF_EEPROM_8BIT,
	  CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MEM_ENABLE |
	      CARDBUS_COMMAND_MASTER_ENABLE,
	  EX_CB_CYCLONE,
	  "3c575CT Ethernet" },

	{ 0,
	  0,
	  0,
	  0,
	  NULL },
};


void ex_cardbus_setup __P((struct ex_cardbus_softc *));

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
	struct ex_cardbus_softc *csc = (void *)self;
	struct ex_softc *sc = &csc->sc_softc;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
#if rbus
#else
	cardbus_chipset_tag_t cc = ct->ct_cc;
#endif
	const struct ex_cardbus_product *ecp;
	bus_addr_t adr, adr1;

	sc->ex_bustype = EX_BUS_CARDBUS;
	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ca->ca_ct;
	csc->sc_intrline = ca->ca_intrline;
	csc->sc_tag = ca->ca_tag;

	ecp = ex_cardbus_lookup(ca);
	if (ecp == NULL) {
		printf("\n");
		panic("ex_cardbus_attach: impossible");
	}

	printf(": 3Com %s\n", ecp->ecp_name);

	sc->ex_conf = ecp->ecp_flags;
	csc->sc_cardtype = ecp->ecp_cardtype;
	csc->sc_csr = ecp->ecp_csr;

	if (Cardbus_mapreg_map(ct, CARDBUS_BASE0_REG, CARDBUS_MAPREG_TYPE_IO, 0,
		&sc->sc_iot, &sc->sc_ioh, &adr, &csc->sc_mapsize) == 0) {
#if rbus
#else
		(*ct->ct_cf->cardbus_io_open)(cc, 0, adr, adr + csc->sc_mapsize);
#endif
		csc->sc_bar_reg = CARDBUS_BASE0_REG;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_IO;

		if (csc->sc_cardtype == EX_CB_CYCLONE) {
			/* Map CardBus function status window. */
			if (Cardbus_mapreg_map(ct, 
				CARDBUS_3C575BTX_FUNCSTAT_PCIREG,
		    		CARDBUS_MAPREG_TYPE_MEM, 0,
				 &csc->sc_funct, &csc->sc_funch,
				 &adr1, &csc->sc_funcsize) == 0) {

				csc->sc_bar_reg1 =
					CARDBUS_3C575BTX_FUNCSTAT_PCIREG;
				csc->sc_bar_val1 =
					adr1 | CARDBUS_MAPREG_TYPE_MEM;

			} else {
				printf("%s: unable to map function "
					"status window\n", self->dv_xname);
				return;
			}

			/* Setup interrupt acknowledge hook */
			sc->intr_ack = ex_cardbus_intr_ack;
		}
	}
	else {
		printf(": can't map i/o space\n");
		return;
	}

	/* Power management hooks. */
	sc->enable = ex_cardbus_enable;
	sc->disable = ex_cardbus_disable;
	sc->power = ex_cardbus_power;

	/*
	 *  Handle power management nonsense and
	 * initialize the configuration registers.
	 */
	ex_cardbus_setup(csc);

	ex_config(sc);

	if (csc->sc_cardtype == EX_CB_CYCLONE)
		bus_space_write_4(csc->sc_funct, csc->sc_funch,
		    EX_CB_INTR, EX_CB_INTR_ACK);

	Cardbus_function_disable(csc->sc_ct);  
}

void
ex_cardbus_intr_ack(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;

	bus_space_write_4(csc->sc_funct, csc->sc_funch, EX_CB_INTR,
	    EX_CB_INTR_ACK);
}

int
ex_cardbus_detach(self, arg)
	struct device *self;
	int arg;
{
	struct ex_cardbus_softc *csc = (void *)self;
	struct ex_softc *sc = &csc->sc_softc;
	struct cardbus_devfunc *ct = csc->sc_ct;
	int rv;

#if defined(DIAGNOSTIC)
	if (ct == NULL) {
		panic("%s: data structure lacks\n", sc->sc_dev.dv_xname);
	}
#endif

	rv = ex_detach(sc);
	if (rv == 0) {
		/*
		 * Unhook the interrupt handler.
		 */
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, sc->sc_ih);

		if (csc->sc_cardtype == EX_CB_CYCLONE) {
			Cardbus_mapreg_unmap(ct,
			    CARDBUS_3C575BTX_FUNCSTAT_PCIREG,
			    csc->sc_funct, csc->sc_funch, csc->sc_funcsize);
		}

		Cardbus_mapreg_unmap(ct, CARDBUS_BASE0_REG, sc->sc_iot,
		    sc->sc_ioh, csc->sc_mapsize);
	}
	return (rv);
}

int
ex_cardbus_enable(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
	cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
	cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

	Cardbus_function_enable(csc->sc_ct);
	ex_cardbus_setup(csc);

	sc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline,
	    IPL_NET, ex_intr, sc);
	if (NULL == sc->sc_ih) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
		csc->sc_intrline);

	return (0);
}

void
ex_cardbus_disable(sc)
	struct ex_softc *sc;
{
	struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
	cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
	cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

	cardbus_intr_disestablish(cc, cf, sc->sc_ih);
	sc->sc_ih = NULL;

 	Cardbus_function_disable(csc->sc_ct);

}

void 
ex_cardbus_power(sc, why)
	struct ex_softc *sc;
	int why;
{
	struct ex_cardbus_softc *csc = (void *) sc;

	if (why == PWR_RESUME) {
		/*
		 * Give the PCI configuration registers a kick
		 * in the head.
		 */
#ifdef DIAGNOSTIC
		if (sc->enabled == 0)
			panic("ex_cardbus_power");
#endif
		ex_cardbus_setup(csc);
	}
}

void 
ex_cardbus_setup(csc)
	struct ex_cardbus_softc *csc;
{
	struct ex_softc *sc = &csc->sc_softc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbusreg_t  reg;
	int pmreg;

	/* Get it out of power save mode if needed (BIOS bugs). */
	if (cardbus_get_capability(cc, cf, csc->sc_tag,
	    PCI_CAP_PWRMGMT, &pmreg, 0)) {
		reg = cardbus_conf_read(cc, cf, csc->sc_tag, pmreg + 4) & 0x03;
#if 1 /* XXX Probably not right for CardBus. */
		if (reg == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
#endif
		if (reg != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, reg);
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    pmreg + 4, 0);
		}
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);
	if (csc->sc_cardtype == EX_CB_CYCLONE) {
		/*
		 * Make sure CardBus brigde can access memory space.  Usually
		 * memory access is enabled by BIOS, but some BIOSes do not
		 * enable it.
		 */
		(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);

		/* Program the BAR */
		cardbus_conf_write(cc, cf, csc->sc_tag,
			csc->sc_bar_reg1, csc->sc_bar_val1); 
	}
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);


	/* Program the BAR */
	cardbus_conf_write(cc, cf, csc->sc_tag,
		csc->sc_bar_reg, csc->sc_bar_val); 

	/* Enable the appropriate bits in the CARDBUS CSR. */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag,
	    CARDBUS_COMMAND_STATUS_REG);
	reg |= csc->sc_csr;
	cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_COMMAND_STATUS_REG,
	    reg);
  
 	/*
	 * set latency timmer
	 */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG);
	if (CARDBUS_LATTIMER(reg) < 0x20) {
		/* at least the value of latency timer should 0x20. */
		DPRINTF(("if_ex_cardbus: lattimer 0x%x -> 0x20\n",
		    CARDBUS_LATTIMER(reg)));
		reg &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
		reg |= (0x20 << CARDBUS_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG, reg);
	}
}
