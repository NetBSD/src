/*	$NetBSD: if_cs_smdk24x0.c,v 1.4.2.2 2013/01/16 05:32:55 yamt Exp $ */

/*
 * Copyright (c) 2003  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* derived from sys/dev/isa/if_cs_isa.c */
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * driver for SMDK24[10]0's on-board CS8900A Ethernet
 *
 * CS8900A is located at CS3 area.
 *    A24=1: I/O access
 *    A24=0: Memory access
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs_smdk24x0.c,v 1.4.2.2 2013/01/16 05:32:55 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <sys/rnd.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arch/arm/s3c2xx0/s3c2410reg.h>
#include <arch/arm/s3c2xx0/s3c2410var.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

#include "locators.h"
#include "opt_smdk2xx0.h"		/* SMDK24X0_ETHER_ADDR_FIXED */

int	cs_ssextio_probe(device_t, cfdata_t, void *);
void	cs_ssextio_attach(device_t, device_t, void *);

/*
 * I/O access is done when A24==1.
 * default value for I/O base address is 0x300
 */
#define IOADDR(base)	(base + (1<<24) + 0x0300)

CFATTACH_DECL_NEW(cs_ssextio, sizeof(struct cs_softc),
    cs_ssextio_probe, cs_ssextio_attach, NULL, NULL);

int 
cs_ssextio_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct s3c2xx0_attach_args *sa = aux;
	bus_space_tag_t iot = sa->sa_iot;
	bus_space_handle_t ioh;
	struct cs_softc sc;
	int rv = 0, have_io = 0;
	vaddr_t ioaddr;

	if (sa->sa_intr == SSEXTIOCF_INTR_DEFAULT)
		sa->sa_intr = 9;
	if (sa->sa_addr == SSEXTIOCF_ADDR_DEFAULT)
		sa->sa_addr = S3C2410_BANK_START(3);

	/*
	 * Map the I/O space.
	 */
	ioaddr = IOADDR(sa->sa_addr);
	if (bus_space_map(iot, ioaddr, CS8900_IOSIZE, 0, &ioh))
		goto out;
	have_io = 1;

	memset(&sc, 0, sizeof sc);
	sc.sc_iot = iot;
	sc.sc_ioh = ioh;

	if (0) {
		int i;

		for (i=0; i <=PKTPG_IND_ADDR; i += 2) {
			if (i % 16 == 0)
				printf( "\n%04x: ", i);
			printf("%04x ", CS_READ_PACKET_PAGE_IO(&sc, i));
		}

	}

	/* Verify that it's a Crystal product. */
	if (CS_READ_PACKET_PAGE_IO(&sc, PKTPG_EISA_NUM) != EISA_NUM_CRYSTAL)
		goto out;

	/*
	 * Verify that it's a supported chip.
	 */
	switch (CS_READ_PACKET_PAGE_IO(&sc, PKTPG_PRODUCT_ID) & PROD_ID_MASK) {
	case PROD_ID_CS8900:
#ifdef notyet
	case PROD_ID_CS8920:
	case PROD_ID_CS8920M:
#endif
		rv = 1;
	}

 out:
	if (have_io)
		bus_space_unmap(iot, ioh, CS8900_IOSIZE);

	return (rv);
}

/* media selection: UTP only */
static int cs_media[] = {
	IFM_ETHER|IFM_10_T,
#if 0
	IFM_ETHER|IFM_10_T|IFM_FDX,
#endif
};

void 
cs_ssextio_attach(device_t parent, device_t self, void *aux)
{
	struct cs_softc *sc = device_private(self);
	struct s3c2xx0_attach_args *sa = aux;
	vaddr_t ioaddr;
#ifdef	SMDK24X0_ETHER_ADDR_FIXED
	static uint8_t enaddr[ETHER_ADDR_LEN] = {SMDK24X0_ETHER_ADDR_FIXED};
#else
#define enaddr NULL
#endif

	sc->sc_dev = self;
	sc->sc_iot = sc->sc_memt = sa->sa_iot;
	/* sc_irq is an IRQ number in ISA world. set 10 for INTRQ0 of CS8900A */
	sc->sc_irq = 10;

	/*
	 * Map the device.
	 */
	ioaddr = IOADDR(sa->sa_addr);
	if (bus_space_map(sc->sc_iot, ioaddr, CS8900_IOSIZE, 0, &sc->sc_ioh)) {
		aprint_error(": unable to map i/o space\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, sa->sa_addr, CS8900_MEMSIZE,
			  0, &sc->sc_memh))
		aprint_error(": unable to map memory space");
	else {
		sc->sc_cfgflags |= CFGFLG_MEM_MODE;
		sc->sc_pktpgaddr = sa->sa_addr;
	}

	/* CS8900A is very slow. (nOE->Data valid: 135ns max.)
	   We need to use IOCHRDY signal */
	sc->sc_cfgflags |= CFGFLG_IOCHRDY;

	sc->sc_ih = s3c2410_extint_establish(sa->sa_intr, IPL_NET, IST_EDGE_RISING,
	    cs_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		return;
	}

	aprint_normal("\n");

	/* SMDK24X0 doesn't have EEPRMO hooked to CS8900A */
	sc->sc_cfgflags |= CFGFLG_NOT_EEPROM;

	cs_attach(sc, enaddr, cs_media, 
	    sizeof(cs_media) / sizeof(cs_media[0]), IFM_ETHER|IFM_10_T);
}
