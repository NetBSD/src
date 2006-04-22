/*	$NetBSD: depca_eisa.c,v 1.8.6.1 2006/04/22 11:38:52 simonb Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * EISA bus front-end for the Digital DEPCA Ethernet controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: depca_eisa.c,v 1.8.6.1 2006/04/22 11:38:52 simonb Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/eisa/eisareg.h>
#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>
#include <dev/ic/depcareg.h>
#include <dev/ic/depcavar.h>

static int	depca_eisa_match(struct device *, struct cfdata *, void *);
static void	depca_eisa_attach(struct device *, struct device *, void *);

struct depca_eisa_softc {
	struct depca_softc sc_depca;

	eisa_chipset_tag_t sc_ec;
	int sc_irq;
	int sc_ist;
};

CFATTACH_DECL(depca_eisa, sizeof(struct depca_eisa_softc),
    depca_eisa_match, depca_eisa_attach, NULL, NULL);

static void	*depca_eisa_intr_establish(struct depca_softc *,
					   struct lance_softc *);

static int
depca_eisa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct eisa_attach_args *ea = aux;

	return (strcmp(ea->ea_idstring, "DEC4220") == 0);
}

#define	DEPCA_ECU_FUNC_NETINTR	0
#define	DEPCA_ECU_FUNC_NETBUF	1

static void
depca_eisa_attach(struct device *parent, struct device *self, void *aux)
{
	struct depca_softc *sc = device_private(self);
	struct depca_eisa_softc *esc = device_private(self);
	struct eisa_attach_args *ea = aux;
	struct eisa_cfg_mem ecm;
	struct eisa_cfg_irq eci;

	printf(": DEC DE422 Ethernet\n");

	sc->sc_iot = ea->ea_iot;
	sc->sc_memt = ea->ea_memt;

	esc->sc_ec = ea->ea_ec;

	sc->sc_intr_establish = depca_eisa_intr_establish;

	if (eisa_conf_read_mem(ea->ea_ec, ea->ea_slot,
	    DEPCA_ECU_FUNC_NETBUF, 0, &ecm) != 0) {
		printf("%s: unable to find network buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: shared memory at 0x%lx-0x%lx\n", sc->sc_dev.dv_xname,
	    ecm.ecm_addr, ecm.ecm_addr + ecm.ecm_size - 1);

	sc->sc_memsize = ecm.ecm_size;

	if (bus_space_map(sc->sc_iot, EISA_SLOT_ADDR(ea->ea_slot) + 0xc00, 16,
	    0, &sc->sc_ioh) != 0) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_memt, ecm.ecm_addr, sc->sc_memsize,
	    0, &sc->sc_memh) != 0) {
		printf("%s: unable to map memory space\n", sc->sc_dev.dv_xname);
		return;
	}

	if (eisa_conf_read_irq(ea->ea_ec, ea->ea_slot,
	    DEPCA_ECU_FUNC_NETINTR, 0, &eci) != 0) {
		printf("%s: unable to determine IRQ\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	esc->sc_irq = eci.eci_irq;
	esc->sc_ist = eci.eci_ist;

	depca_attach(sc);
}

static void *
depca_eisa_intr_establish(struct depca_softc *parent, struct lance_softc *child)
{
	struct depca_eisa_softc *esc = (void *) parent;
	eisa_intr_handle_t ih;
	const char *intrstr;
	void *rv;

	if (eisa_intr_map(esc->sc_ec, esc->sc_irq, &ih)) {
		printf("%s: unable to map interrupt (%d)\n",
		    parent->sc_dev.dv_xname, esc->sc_irq);
		return (NULL);
	}
	intrstr = eisa_intr_string(esc->sc_ec, ih);
	rv = eisa_intr_establish(esc->sc_ec, ih, esc->sc_ist, IPL_NET,
	    (esc->sc_ist == IST_LEVEL) ? am7990_intr : depca_intredge, child);
	if (rv == NULL) {
		printf("%s: unable to establish interrupt",
		    parent->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return (NULL);
	}
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", parent->sc_dev.dv_xname,
		    intrstr);

	return (rv);
}
