/* $NetBSD: upc.c,v 1.1.6.1 2002/01/10 19:55:08 thorpej Exp $ */
/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: upc.c,v 1.1.6.1 2002/01/10 19:55:08 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/ata/atavar.h> /* XXX needed by wdcvar.h */
#include <dev/ic/comreg.h>
#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>
#include <dev/ic/wdcvar.h>
#include <dev/ic/upcreg.h>
#include <dev/ic/upcvar.h>

#include "locators.h"

static void upc_found(struct upc_softc *, char const *, int, int,
		      struct upc_irqhandle *);
static void upc_found2(struct upc_softc *, char const *, int, int, int, int,
		       struct upc_irqhandle *);
static int upc_print(void *, char const *);
static int upc_submatch(struct device *, struct cfdata *, void *);
static int upc_com3_addr(int);
static int upc_com4_addr(int);

void
upc_attach(struct upc_softc *sc)
{
	u_int8_t cr[5];
	int i;

	/* Dump configuration */
	for (i = 0; i < 5; i++)
		cr[i] = upc_read_config(sc, i);

	/* Leave configuration mode. */
	printf(": config state %02x %02x %02x %02x %02x",
	       cr[0], cr[1], cr[2], cr[3], cr[4]);
	printf("\n");

	/* "Find" the attached devices */
	/* FDC */
	if (cr[0] & UPC_CR0_FDC_ENABLE)
		upc_found(sc, "fdc", UPC_PORT_FDCBASE, 2, &sc->sc_fintr);
	/* IDE */
	if (cr[0] & UPC_CR0_IDE_ENABLE)
		upc_found2(sc, "wdc", UPC_PORT_IDECMDBASE, 8,
			   UPC_PORT_IDECTLBASE, 2, &sc->sc_wintr);
	/* Parallel */
	switch (cr[1] & UPC_CR1_LPT_MASK) {
	case UPC_CR1_LPT_3BC:
		upc_found(sc, "lpt", 0x3bc, LPT_NPORTS, &sc->sc_pintr);
		break;
	case UPC_CR1_LPT_378:
		upc_found(sc, "lpt", 0x378, LPT_NPORTS, &sc->sc_pintr);
		break;
	case UPC_CR1_LPT_278:
		upc_found(sc, "lpt", 0x278, LPT_NPORTS, &sc->sc_pintr);
		break;
	}
	/* UART1 */
	if (cr[2] & UPC_CR2_UART1_ENABLE) {
		switch (cr[2] & UPC_CR2_UART1_MASK) {
		case UPC_CR2_UART1_3F8:
			upc_found(sc, "com", 0x3f8, COM_NPORTS, &sc->sc_irq4);
			break;
		case UPC_CR2_UART1_2F8:
			upc_found(sc, "com", 0x2f8, COM_NPORTS, &sc->sc_irq3);
			break;
		case UPC_CR2_UART1_COM3:
			upc_found(sc, "com", upc_com3_addr(cr[1]), COM_NPORTS,
				  &sc->sc_irq4);
			break;
		case UPC_CR2_UART1_COM4:
			upc_found(sc, "com", upc_com4_addr(cr[1]), COM_NPORTS,
				  &sc->sc_irq3);
			break;
		}
	}
	/* UART2 */
	if (cr[2] & UPC_CR2_UART2_ENABLE) {
		switch (cr[2] & UPC_CR2_UART2_MASK) {
		case UPC_CR2_UART2_3F8:
			upc_found(sc, "com", 0x3f8, COM_NPORTS, &sc->sc_irq4);
			break;
		case UPC_CR2_UART2_2F8:
			upc_found(sc, "com", 0x2f8, COM_NPORTS, &sc->sc_irq3);
			break;
		case UPC_CR2_UART2_COM3:
			upc_found(sc, "com", upc_com3_addr(cr[1]), COM_NPORTS,
				  &sc->sc_irq4);
			break;
		case UPC_CR2_UART2_COM4:
			upc_found(sc, "com", upc_com4_addr(cr[1]), COM_NPORTS,
				  &sc->sc_irq3);
			break;
		}
	}

}

static void
upc_found(struct upc_softc *sc, char const *devtype, int offset, int size,
	  struct upc_irqhandle *uih)
{
	struct upc_attach_args ua;

	ua.ua_devtype = devtype;
	ua.ua_offset = offset;
	ua.ua_iot = sc->sc_iot;
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, offset, size, &ua.ua_ioh);
	ua.ua_irqhandle = uih;
	config_found_sm(&sc->sc_dev, &ua, upc_print, upc_submatch);
}

static void 
upc_found2(struct upc_softc *sc, char const *devtype, int offset, int size,
	   int offset2, int size2, struct upc_irqhandle *uih)
{
	struct upc_attach_args ua;

	ua.ua_devtype = devtype;
	ua.ua_offset = offset;
	ua.ua_iot = sc->sc_iot;
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, offset, size, &ua.ua_ioh);
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, offset2, size2,
			    &ua.ua_ioh2);
	ua.ua_irqhandle = uih;
	config_found_sm(&sc->sc_dev, &ua, upc_print, upc_submatch);
}

void
upc_intr_establish(struct upc_irqhandle *uih, int level, int (*func)(void *),
		   void *arg) {

	uih->uih_level = level;
	uih->uih_func = func;
	uih->uih_arg = arg;
	/* Actual MD establishment will be handled later by bus attachment. */
}

static int
upc_com3_addr(int cr1)
{

	switch (cr1 & UPC_CR1_COM34_MASK) {
	case UPC_CR1_COM34_338_238:
		return 0x338;
	case UPC_CR1_COM34_3E8_2E8:
		return 0x3e8;
	case UPC_CR1_COM34_2E8_2E0:
		return 0x2e8;
	case UPC_CR1_COM34_220_228:
		return 0x220;
	}
	return -1;
}

static int
upc_com4_addr(int cr1)
{

	switch (cr1 & UPC_CR1_COM34_MASK) {
	case UPC_CR1_COM34_338_238:
		return 0x238;
	case UPC_CR1_COM34_3E8_2E8:
		return 0x2e8;
	case UPC_CR1_COM34_2E8_2E0:
		return 0x2e0;
	case UPC_CR1_COM34_220_228:
		return 0x228;
	}
	return -1;
}

static int
upc_print(void *aux, char const *pnp)
{
	struct upc_attach_args *ua = aux;

	if (pnp)
		printf("%s at %s", ua->ua_devtype, pnp);
	printf(" offset 0x%x", ua->ua_offset);
	return UNCONF;
}

static int
upc_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct upc_attach_args *ua = aux;

	if (strcmp(cf->cf_driver->cd_name, ua->ua_devtype) == 0 &&
	    (cf->cf_loc[UPCCF_OFFSET] == UPCCF_OFFSET_DEFAULT ||
	     cf->cf_loc[UPCCF_OFFSET] == ua->ua_offset))
		return (*cf->cf_attach->ca_match)(parent, cf, aux);
	return 0;
}

int
upc_read_config(struct upc_softc *sc, int reg)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int retval;

	/* Switch into configuration mode. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_ENTER);
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_ENTER);

	/* Read register. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, reg);
	retval = bus_space_read_1(iot, ioh, UPC_PORT_CFGDATA);

	/* Leave configuration mode. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_EXIT);
	return retval;
}

void
upc_write_config(struct upc_softc *sc, int reg, int val)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* Switch into configuration mode. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_ENTER);
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_ENTER);

	/* Write register. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, reg);
	bus_space_write_1(iot, ioh, UPC_PORT_CFGDATA, val);

	/* Leave configuration mode. */
	bus_space_write_1(iot, ioh, UPC_PORT_CFGADDR, UPC_CFGMAGIC_EXIT);
}
