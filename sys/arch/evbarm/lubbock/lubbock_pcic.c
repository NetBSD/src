/*      $NetBSD: lubbock_pcic.c,v 1.3.72.2 2009/06/20 07:20:01 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: lubbock_pcic.c,v 1.3.72.2 2009/06/20 07:20:01 yamt Exp $");

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

#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>
#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>
#include <arm/sa11x0/sa11x1_pcicreg.h>
#include <arm/sa11x0/sa11xx_pcicvar.h>
#include <arm/sa11x0/sa11x1_pcicvar.h>

#include <evbarm/lubbock/lubbock_reg.h>
#include <evbarm/lubbock/lubbock_var.h>

static	int	sacpcic_match(device_t, cfdata_t, void *);
static	void	sacpcic_attach(device_t, device_t, void *);
static	void	lubbock_set_power(struct sapcic_socket *so, int arg);
static	void	lubbock_socket_setup(struct sapcic_socket *sp);

static struct sapcic_tag lubbock_sacpcic_functions = {
	sacpcic_read,
	sacpcic_write,
	lubbock_set_power,
	sacpcic_clear_intr,
	sacpcic_intr_establish,
	sacpcic_intr_disestablish
};

CFATTACH_DECL_NEW(sacpcic, sizeof(struct sacpcic_softc),
    sacpcic_match, sacpcic_attach, NULL, NULL);

static int
sacpcic_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

static void
lubbock_socket_setup(struct sapcic_socket *sp)
{
	sp->power_capability = SAPCIC_POWER_5V | SAPCIC_POWER_3V;
	sp->pcictag = &lubbock_sacpcic_functions;
}

static void
sacpcic_attach(device_t parent, device_t self, void *aux)
{
	sacpcic_attach_common(device_private(parent),
	    device_private(self), aux, lubbock_socket_setup);
}


static void
lubbock_set_power(struct sapcic_socket *so, int arg)
{
	struct sacc_softc *sc = so->pcictag_cookie;
	struct obio_softc *bsc = device_private(device_parent(sc->sc_dev));
	int s;
	uint16_t tmp;

	static const uint8_t vval_socket0[] = {
		/* for socket0 (pcmcia) */
		0x00,		/* OFF */
		0x08,		/* 3.3V */
		0x05,		/* 5V */
	};
	static const uint16_t vval_socket1[] = {
		/* for socket1 (CF) */
		0x0000,		/* OFF */
		0x8000,		/* 3.3V */
		0x4000,		/* 5V */
	};

	if( arg < 0 || SAPCIC_POWER_5V < arg )
		panic("sacpcic_set_power: bogus arg\n");
			
	switch( so->socket ){
	case 0:
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCGPIOA_DVR, 
				  vval_socket0[arg]);
		break;
	case 1:
		s = splhigh();
		tmp = bus_space_read_2(bsc->sc_iot, bsc->sc_obioreg_ioh,
		    LUBBOCK_MISCWR);
		bus_space_write_2(bsc->sc_iot, bsc->sc_obioreg_ioh,
		    LUBBOCK_MISCWR, (tmp & 0x3fff) | vval_socket1[arg] );
		splx(s);
		break;
	default:
		aprint_normal("unknown socket number: %d\n", so->socket);
	}
}
