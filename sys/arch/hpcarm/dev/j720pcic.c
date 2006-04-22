/*      $NetBSD: j720pcic.c,v 1.2.6.1 2006/04/22 11:37:28 simonb Exp $        */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/* Jornada 720 PCMCIA support. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720pcic.c,v 1.2.6.1 2006/04/22 11:37:28 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>
#include <arm/sa11x0/sa11x1_pcicreg.h>
#include <arm/sa11x0/sa11xx_pcicvar.h>
#include <arm/sa11x0/sa11x1_pcicvar.h>

#include "sacpcic.h"

static int	sacpcic_match(struct device *, struct cfdata *, void *);
static void	sacpcic_attach(struct device *, struct device *, void *);

static void	j720_socket_setup(struct sapcic_socket *);
static void	j720_set_power(struct sapcic_socket *, int);

static struct sapcic_tag j720_sacpcic_functions = {
	sacpcic_read,
	sacpcic_write,
	j720_set_power,
	sacpcic_clear_intr,
	sacpcic_intr_establish,
	sacpcic_intr_disestablish
};

static int j720_power_capability[] = {
	SAPCIC_POWER_5V | SAPCIC_POWER_3V, SAPCIC_POWER_3V
};

static struct platid_data sacpcic_platid_table[] = {
	{ &platid_mask_MACH_HP_JORNADA_7XX, j720_power_capability },
	{ NULL, NULL }
};

CFATTACH_DECL(sacpcic, sizeof(struct sacpcic_softc),
    sacpcic_match, sacpcic_attach, NULL, NULL);


static int
sacpcic_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

static void
sacpcic_attach(struct device *parent, struct device *self, void *aux)
{

	sacpcic_attach_common((struct sacc_softc *)parent,
	    (struct sacpcic_softc *)self, aux, j720_socket_setup);
}

static void
j720_socket_setup(struct sapcic_socket *sp)
{
	int *ip;
	struct platid_data *p;
	int socket = sp->socket;

	p = platid_search_data(&platid, sacpcic_platid_table);

	if (p == NULL) {
		sp->power_capability = SAPCIC_POWER_5V;
	} else {
		ip = (int *)p->data;
		sp->power_capability = ip[socket];
	}

	sp->pcictag = &j720_sacpcic_functions;
}

static void
j720_set_power(struct sapcic_socket *so, int arg)
{
	int newval, oldval, s;
	struct sacc_softc *sc = so->pcictag_cookie;

	/* XXX this isn't well confirmed. DANGER DANGER */
	switch (arg) {
	case SAPCIC_POWER_OFF:
		newval = 0;
		break;
	case SAPCIC_POWER_3V:
		newval = 2;
		break;
	case SAPCIC_POWER_5V:
		newval = 1;
		break;
	default:
		panic("j720_set_power: bogus arg (%d)", arg);
	}

	s = splbio();
	oldval = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCGPIOA_DVR);

	switch (so->socket) {
	case 0:
		newval = newval | (oldval & 0xc);
		break;
	case 1:
		newval = (newval << 2) | (oldval & 3);
		break;
	default:
		splx(s);
		panic("j720_set_power: bogus socket (%d)", so->socket);
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCGPIOA_DVR, newval);
	splx(s);
}
