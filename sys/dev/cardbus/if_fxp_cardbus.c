/*	$NetBSD: if_fxp_cardbus.c,v 1.2 1999/10/15 06:42:22 haya Exp $	*/

/*
 * Copyright (c) 1999
 *       Johan Danielsson <joda@pdc.kth.se>.  All rights reserved.
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
 *	This product includes software developed by the author.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/pci/if_fxpreg.h>
#include <dev/pci/if_fxpvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>

static int fxp_cardbus_match __P((struct device *, struct cfdata *, void *));
static void fxp_cardbus_attach __P((struct device *, struct device *, void *));

struct fxp_cardbus_softc {
    struct fxp_softc sc;
    cardbustag_t tag;
    pcireg_t base1_reg;
};

struct cfattach fxp_cardbus_ca = {
    sizeof(struct fxp_cardbus_softc), fxp_cardbus_match, fxp_cardbus_attach
};

#ifdef CBB_DEBUG
#define DPRINTF(X) printf X
#else
#define DPRINTF(X)
#endif

/*
 * Check if a device is an 82557.
 */
static int
fxp_cardbus_match(parent, match, aux)
     struct device *parent;
     struct cfdata *match;
     void *aux;
{
    /* should check CIS */
    struct cardbus_attach_args *ca = aux;
    
    if (CARDBUS_VENDOR(ca->ca_id) != CARDBUS_VENDOR_INTEL)
	return (0);

    switch (CARDBUS_PRODUCT(ca->ca_id)) {
    case CARDBUS_PRODUCT_INTEL_82557:
	return (1);
    }

    return (0);
}

int
fxp_enable(struct fxp_softc *sc);

static int
fxp_cardbus_enable(struct fxp_softc *sc);
static void
fxp_cardbus_disable(struct fxp_softc *sc);

static void
fxp_cardbus_setup(struct fxp_softc *sc);

static void
fxp_cardbus_attach(parent, self, aux)
     struct device *parent, *self;
     void *aux;
{
    struct fxp_softc *sc = (struct fxp_softc*)self;
    struct fxp_cardbus_softc *csc = (struct fxp_cardbus_softc*)self;
    struct cardbus_attach_args *ca = aux;
    cardbus_function_tag_t cf = ca->ca_cf;
    bus_space_handle_t ioh, memh;
	
    bus_addr_t adr;

    /*
     * Map control/status registers.
     */
#if rbus
    cardbus_function_tag_t cf = ct->ct_cf;

    if (cardbus_mapreg_map(ct, CARDBUS_BASE1_REG, CARDBUS_MAPREG_TYPE_IO, 0,
			   &(sc->sc_iot), &ioh, &adr, NULL)) {
      panic("io alloc in ex_attach_cardbus\n");
    }
    csc->base1_reg = adr | 1;
    sc->sc_sh = ioh;

#elif unibus
    printf("fxp_cardbus_attach: uni %p\n", ca->ca_uni);
    if ((*cf->cf_ub.unibus_space_alloc)(ca->ca_uni, ca->bar_info[1].tag, 0, 
					ca->bar_info[1].size,
					ca->bar_info[1].size - 1,
					ca->bar_info[1].size,
					0, 0, &adr, &ioh))
	panic("io alloc");


    csc->base1_reg = adr | 1;
    sc->sc_st = ca->bar_info[1].tag;
    sc->sc_sh = ioh;

    if (0) {
	/*
	 * Map control/status registers.
	 */
	if ((*cf->cf_ub.unibus_space_alloc)(ca->ca_uni, ca->bar_info[0].tag, 0, 
					    ca->bar_info[0].size,
					    ca->bar_info[0].size - 1,
					    ca->bar_info[0].size,
					    0, 0, &adr, &memh))
	    panic("mem alloc");


	sc->sc_st = ca->bar_info[0].tag;
	sc->sc_sh = memh;
    }
#endif

    csc->tag = ca->ca_tag;

    sc->sc_dmat = ca->ca_dmat;
    sc->enable = fxp_cardbus_enable;
    sc->disable = fxp_cardbus_disable;
    sc->enabled = 0;
    
    fxp_cardbus_setup(sc);
    printf(": Intel EtherExpress Pro 10+/100B Ethernet\n");
    
    fxp_attach_common(sc);
}


static void
fxp_cardbus_setup(struct fxp_softc *sc)
{
    struct fxp_cardbus_softc *csc = (struct fxp_cardbus_softc*)sc;
    struct cardbus_softc *psc = (struct cardbus_softc *)sc->sc_dev.dv_parent;
    cardbus_chipset_tag_t cc = psc->sc_cc;
    cardbus_function_tag_t cf = psc->sc_cf;
    pcireg_t command;

    printf("fxp_cardbus_setup\n");

    cardbus_conf_write(cc, cf, csc->tag, CARDBUS_BASE1_REG, csc->base1_reg);

    (cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);

    /* enable the card */
    command = cardbus_conf_read(cc, cf, csc->tag, CARDBUS_COMMAND_STATUS_REG);
    command |= (CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE);
    cardbus_conf_write(cc, cf, csc->tag, CARDBUS_COMMAND_STATUS_REG, command);
#if 0
    cardbus_conf_write(cc, cf, ca->tag, CARDBUS_BASE0_REG, adr);

    (ca->ca_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);

    /* enable card mem */
    command = cardbus_conf_read(cc, cf, ca->tag, CARDBUS_COMMAND_STATUS_REG);
    command |= CARDBUS_COMMAND_MEM_ENABLE;
    cardbus_conf_write(cc, cf, ca->tag, CARDBUS_COMMAND_STATUS_REG, command);
#endif
}

static int
fxp_cardbus_enable(struct fxp_softc *sc)
{
    struct fxp_cardbus_softc *csc = (struct fxp_cardbus_softc*)sc;
    struct cardbus_softc *psc = (struct cardbus_softc *)sc->sc_dev.dv_parent;
    cardbus_chipset_tag_t cc = psc->sc_cc;
    cardbus_function_tag_t cf = psc->sc_cf;

    cardbus_function_enable(psc, csc->tag);

    fxp_cardbus_setup(sc);

    /* Map and establish the interrupt. */

    sc->sc_ih = cardbus_intr_establish(cc, cf, psc->sc_intrline, IPL_NET, 
				       fxp_intr, sc);
    if (NULL == sc->sc_ih) {
	printf("%s: couldn't establish interrupt\n", sc->sc_dev.dv_xname);
	return 1;
    }

    printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname, psc->sc_intrline);

    return 0;
}

static void
fxp_cardbus_disable(struct fxp_softc *sc)
{
    struct cardbus_softc *psc = (struct cardbus_softc *)sc->sc_dev.dv_parent;
    cardbus_chipset_tag_t cc = psc->sc_cc;
    cardbus_function_tag_t cf = psc->sc_cf;

    fxp_stop(sc);
    cardbus_intr_disestablish(cc, cf, sc->sc_ih); /* remove intr handler */
    
    cardbus_function_disable(psc);
}
