/*	$NetBSD: if_sn_jazzio.c,v 1.7.30.1 2007/07/15 13:15:35 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * Microsoft JAZZ front-end for for the National Semiconductor DP83932
 * Systems-Oriented Network Interface Controller (SONIC).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sn_jazzio.c,v 1.7.30.1 2007/07/15 13:15:35 ad Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/kcore.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/dp83932reg.h>
#include <dev/ic/dp83932var.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/jazziovar.h>

int	sonic_jazzio_match(struct device *, struct cfdata *, void *);
void	sonic_jazzio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sn_jazzio, sizeof(struct sonic_softc),
    sonic_jazzio_match, sonic_jazzio_attach, NULL, NULL);

int
sonic_jazzio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "SONIC") != 0)
		return 0;

	return 1;
}

void
sonic_jazzio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sonic_softc *sc = (void *) self;
	struct jazzio_attach_args *ja = aux;
	int i;
	uint8_t enaddr[ETHER_ADDR_LEN];

	sc->sc_st = ja->ja_bust;
	sc->sc_dmat = ja->ja_dmat;

	printf(": SONIC Ethernet\n");

	/* Map the device. */
	if (bus_space_map(sc->sc_st, ja->ja_addr, SONIC_NREGS * 4,
	    0, &sc->sc_sh) != 0) {
		printf("%s: unable to map SONIC registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* We run in 32-bit mode. */
	sc->sc_32bit = 1;

	/* BMODE is set for little-endian. */
	sc->sc_bigendian = 0;

	/* Regs are 16-bit, plus 16-bit pad. */
	for (i = 0; i < SONIC_NREGS; i++)
		sc->sc_regmap[i] = i * 4;

	/*
	 * Configure DCR:
	 *
	 *	- Latched bug retry
	 *	- Synchronous bus (memory cycle 2 clocks)
	 *	- 0 wait states added (WC0,WC1 == 0,0)
	 *	- 4 byte Rx DMA threshold (RFT0,RFT1 == 0,0)
	 *	- 28 byte Tx DMA threshold (TFT0,TFT1 == 1,1)
	 *	  XXX There was a comment
	 *	  	"XXX RFT & TFT according to MIPS manual"
	 *	      in old MD sys/arch/arc/dev/if_sn.c in Attic.
	 */
	sc->sc_dcr = DCR_LBR | DCR_SBUS | DCR_TFT0 | DCR_TFT1;
	sc->sc_dcr2 = 0;

	/* Hook up our interrupt handler. */
	jazzio_intr_establish(ja->ja_intr, sonic_intr, sc);

	/* The Ethernet address is from the product ID. */
	memcpy(enaddr, arc_product_id, sizeof(enaddr));

	/* Finish off the attach. */
	sonic_attach(sc, enaddr);
}
