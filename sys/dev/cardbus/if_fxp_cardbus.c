/*	$NetBSD: if_fxp_cardbus.c,v 1.19.6.4 2008/01/21 09:42:40 yamt Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Johan Danielsson.
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
 * CardBus front-end for the Intel i82557 family of Ethernet chips.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_fxp_cardbus.c,v 1.19.6.4 2008/01/21 09:42:40 yamt Exp $");

#include "opt_inet.h"
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

#include <machine/endian.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif


#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/i82557reg.h>
#include <dev/ic/i82557var.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

static int fxp_cardbus_match(struct device *, struct cfdata *, void *);
static void fxp_cardbus_attach(struct device *, struct device *, void *);
static int fxp_cardbus_detach(struct device * self, int flags);
static void fxp_cardbus_setup(struct fxp_softc * sc);
static int fxp_cardbus_enable(struct fxp_softc * sc);
static void fxp_cardbus_disable(struct fxp_softc * sc);

struct fxp_cardbus_softc {
	struct fxp_softc sc;
	cardbus_devfunc_t ct;
	pcireg_t base0_reg;
	pcireg_t base1_reg;
	bus_size_t size;
};

CFATTACH_DECL(fxp_cardbus, sizeof(struct fxp_cardbus_softc),
    fxp_cardbus_match, fxp_cardbus_attach, fxp_cardbus_detach, fxp_activate);

#ifdef CBB_DEBUG
#define DPRINTF(X) printf X
#else
#define DPRINTF(X)
#endif

static int
fxp_cardbus_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (CARDBUS_VENDOR(ca->ca_id) == PCI_VENDOR_INTEL &&
	    CARDBUS_PRODUCT(ca->ca_id) == PCI_PRODUCT_INTEL_82557)
		return (1);

	return (0);
}

static void
fxp_cardbus_attach(struct device *parent, struct device *self,
    void *aux)
{
	static const char thisfunc[] = "fxp_cardbus_attach";

	struct fxp_softc *sc = device_private(self);
	struct fxp_cardbus_softc *csc = device_private(self);
	struct cardbus_attach_args *ca = aux;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;

	bus_addr_t adr;
	bus_size_t size;

	csc->ct = ca->ca_ct;

	/*
         * Map control/status registers.
         */
	if (Cardbus_mapreg_map(csc->ct, CARDBUS_BASE1_REG,
	    PCI_MAPREG_TYPE_IO, 0, &iot, &ioh, &adr, &size) == 0) {
		csc->base1_reg = adr | 1;
		sc->sc_st = iot;
		sc->sc_sh = ioh;
		csc->size = size;
	} else if (Cardbus_mapreg_map(csc->ct, CARDBUS_BASE0_REG,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
	    0, &memt, &memh, &adr, &size) == 0) {
		csc->base0_reg = adr;
		sc->sc_st = memt;
		sc->sc_sh = memh;
		csc->size = size;
	} else
		panic("%s: failed to allocate mem and io space", thisfunc);


	if (ca->ca_cis.cis1_info[0] && ca->ca_cis.cis1_info[1])
		printf(": %s %s\n", ca->ca_cis.cis1_info[0],
		    ca->ca_cis.cis1_info[1]);
	else
		printf("\n");

	sc->sc_dmat = ca->ca_dmat;
	sc->sc_enable = fxp_cardbus_enable;
	sc->sc_disable = fxp_cardbus_disable;
	sc->sc_enabled = 0;

	fxp_enable(sc);
	fxp_attach(sc);
	fxp_disable(sc);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, &sc->sc_ethercom.ec_if);
}

static void
fxp_cardbus_setup(struct fxp_softc * sc)
{
	struct fxp_cardbus_softc *csc = (struct fxp_cardbus_softc *) sc;
	struct cardbus_softc *psc =
	    (struct cardbus_softc *) device_parent(&sc->sc_dev);
	cardbus_chipset_tag_t cc = psc->sc_cc;
	cardbus_function_tag_t cf = psc->sc_cf;
	pcireg_t command;

	cardbustag_t tag = cardbus_make_tag(cc, cf, csc->ct->ct_bus,
	    csc->ct->ct_func);

	command = Cardbus_conf_read(csc->ct, tag, CARDBUS_COMMAND_STATUS_REG);
	if (csc->base0_reg) {
		Cardbus_conf_write(csc->ct, tag,
		    CARDBUS_BASE0_REG, csc->base0_reg);
		(cf->cardbus_ctrl) (cc, CARDBUS_MEM_ENABLE);
		command |= CARDBUS_COMMAND_MEM_ENABLE |
		    CARDBUS_COMMAND_MASTER_ENABLE;
	} else if (csc->base1_reg) {
		Cardbus_conf_write(csc->ct, tag,
		    CARDBUS_BASE1_REG, csc->base1_reg);
		(cf->cardbus_ctrl) (cc, CARDBUS_IO_ENABLE);
		command |= (CARDBUS_COMMAND_IO_ENABLE |
		    CARDBUS_COMMAND_MASTER_ENABLE);
	}

	(cf->cardbus_ctrl) (cc, CARDBUS_BM_ENABLE);

	/* enable the card */
	Cardbus_conf_write(csc->ct, tag, CARDBUS_COMMAND_STATUS_REG, command);
}

static int
fxp_cardbus_enable(struct fxp_softc * sc)
{
	struct fxp_cardbus_softc *csc = (struct fxp_cardbus_softc *) sc;
	struct cardbus_softc *psc =
	    (struct cardbus_softc *) device_parent(&sc->sc_dev);
	cardbus_chipset_tag_t cc = psc->sc_cc;
	cardbus_function_tag_t cf = psc->sc_cf;

	Cardbus_function_enable(csc->ct);

	fxp_cardbus_setup(sc);

	/* Map and establish the interrupt. */

	sc->sc_ih = cardbus_intr_establish(cc, cf, psc->sc_intrline, IPL_NET,
	    fxp_intr, sc);
	if (NULL == sc->sc_ih) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return 1;
	}

	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
	    psc->sc_intrline);

	return 0;
}

static void
fxp_cardbus_disable(struct fxp_softc * sc)
{
	struct cardbus_softc *psc =
	    (struct cardbus_softc *) device_parent(&sc->sc_dev);
	cardbus_chipset_tag_t cc = psc->sc_cc;
	cardbus_function_tag_t cf = psc->sc_cf;

	/* Remove interrupt handler. */
	cardbus_intr_disestablish(cc, cf, sc->sc_ih);

	Cardbus_function_disable(((struct fxp_cardbus_softc *) sc)->ct);
}

static int
fxp_cardbus_detach(struct device *self, int flags)
{
	struct fxp_softc *sc = device_private(self);
	struct fxp_cardbus_softc *csc = device_private(self);
	struct cardbus_devfunc *ct = csc->ct;
	int rv, reg;

#ifdef DIAGNOSTIC
	if (ct == NULL)
		panic("%s: data structure lacks", sc->sc_dev.dv_xname);
#endif

	rv = fxp_detach(sc);
	if (rv == 0) {
		/*
		 * Unhook the interrupt handler.
		 */
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, sc->sc_ih);

		/*
		 * release bus space and close window
		 */
		if (csc->base0_reg)
			reg = CARDBUS_BASE0_REG;
		else
			reg = CARDBUS_BASE1_REG;
		Cardbus_mapreg_unmap(ct, reg, sc->sc_st, sc->sc_sh, csc->size);
	}
	return (rv);
}
