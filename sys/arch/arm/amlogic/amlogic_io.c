/* $NetBSD: amlogic_io.c,v 1.13.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_amlogic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_io.c,v 1.13.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/mainbus/mainbus.h>
#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>

#include "locators.h"

static int	amlogicio_match(device_t, cfdata_t, void *);
static void	amlogicio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(amlogic_io, 0,
    amlogicio_match, amlogicio_attach, NULL, NULL);

static int	amlogicio_print(void *, const char *);
static int	amlogicio_find(device_t, cfdata_t, const int *, void *);

static bool amlogicio_found = false;

#define NOPORT	AMLOGICIOCF_PORT_DEFAULT
#define NOINTR	AMLOGICIO_INTR_DEFAULT

static const struct amlogic_locators amlogic_locators[] = {
  { "amlogiccom",
    AMLOGIC_UART0AO_OFFSET, AMLOGIC_UART_SIZE, 0, AMLOGIC_INTR_UART0AO },
  { "amlogiccom",
    AMLOGIC_UART2AO_OFFSET, AMLOGIC_UART_SIZE, 2, AMLOGIC_INTR_UART2AO },
  { "amlogicgpio",
    0, 0, NOPORT, NOINTR },
  { "genfb",
    AMLOGIC_DMC_OFFSET, AMLOGIC_DMC_SIZE, NOPORT, NOINTR },
  { "amlogicrng",
    AMLOGIC_RAND_OFFSET, AMLOGIC_RAND_SIZE, NOPORT, NOINTR },
  { "dwctwo",
    AMLOGIC_USB0_OFFSET, AMLOGIC_USB_SIZE, 0, AMLOGIC_INTR_USB0 },
  { "dwctwo",
    AMLOGIC_USB1_OFFSET, AMLOGIC_USB_SIZE, 1, AMLOGIC_INTR_USB1 },
  { "awge",
    AMLOGIC_GMAC_OFFSET, AMLOGIC_GMAC_SIZE, NOPORT, AMLOGIC_INTR_GMAC },
  { "amlogicsdio",
    AMLOGIC_SDIO_OFFSET, AMLOGIC_SDIO_SIZE, NOPORT, AMLOGIC_INTR_SDIO },
  { "amlogicsdhc",
    AMLOGIC_SDHC_OFFSET, AMLOGIC_SDHC_SIZE, NOPORT, AMLOGIC_INTR_SDHC },
  { "amlogicrtc",
    AMLOGIC_RTC_OFFSET, AMLOGIC_RTC_SIZE, NOPORT, AMLOGIC_INTR_RTC },
};

int
amlogicio_match(device_t parent, cfdata_t cf, void *aux)
{
	if (amlogicio_found)
		return 0;
	return 1;
}

void
amlogicio_attach(device_t parent, device_t self, void *aux)
{
	amlogicio_found = true;

	aprint_naive("\n");
	aprint_normal("\n");

	amlogic_wdog_init();
	amlogic_usbphy_init(0);
	amlogic_usbphy_init(1);

	const struct amlogic_locators * const eloc =
	    amlogic_locators + __arraycount(amlogic_locators);
	for (const struct amlogic_locators *loc = amlogic_locators;
	     loc < eloc;
	     loc++) {
		struct amlogicio_attach_args aio = {
			.aio_loc = *loc,
			.aio_core_bst = &armv7_generic_bs_tag,
			.aio_core_a4x_bst = &armv7_generic_a4x_bs_tag,
			.aio_bsh = amlogic_core_bsh,
			.aio_dmat = &amlogic_dma_tag,
		};
		cfdata_t cf = config_search_ia(amlogicio_find, self,
		    "amlogicio", &aio);
		if (cf != NULL)
			config_attach(self, cf, &aio, amlogicio_print);
	}
}

int
amlogicio_print(void *aux, const char *pnp)
{
	const struct amlogicio_attach_args * const aio = aux;

	if (aio->aio_loc.loc_port != AMLOGICIOCF_PORT_DEFAULT)
		aprint_normal(" port %d", aio->aio_loc.loc_port);

	return UNCONF;
}

static int
amlogicio_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	const struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AMLOGICIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != AMLOGICIOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	return config_match(parent, cf, aux);
}
