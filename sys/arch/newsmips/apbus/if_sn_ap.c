/*	$NetBSD: if_sn_ap.c,v 1.11.90.1 2018/10/20 06:58:29 pgoyette Exp $	*/

/*
 * Copyright (C) 1997 Allen Briggs
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Allen Briggs
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sn_ap.c,v 1.11.90.1 2018/10/20 06:58:29 pgoyette Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/cpu.h>
#include <machine/adrsmap.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/apbus/if_snreg.h>
#include <newsmips/apbus/if_snvar.h>

#include <newsmips/newsmips/machid.h>

#define SONIC_MACROM_OFFSET	0x40

#define SONIC_APBUS_REG_OFFSET	0x00010000
#define SONIC_APBUS_MEM_OFFSET	0x00020000
#define SONIC_APBUS_CTL_OFFSET	(-0x00100000)

static int	sn_ap_match(device_t, cfdata_t, void *);
static void	sn_ap_attach(device_t, device_t, void *);
static int	sn_ap_getaddr(struct sn_softc *, uint8_t *);

CFATTACH_DECL_NEW(sn_ap, sizeof(struct sn_softc),
    sn_ap_match, sn_ap_attach, NULL, NULL);

static int
sn_ap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "sonic") != 0)
		return 0;

	return 1;
}

/*
 * Install interface into kernel networking data structures
 */
static void
sn_ap_attach(device_t parent, device_t self, void *aux)
{
	struct sn_softc *sc = device_private(self);
	struct apbus_attach_args *apa = aux;
	uint8_t myaddr[ETHER_ADDR_LEN];
	uint32_t intrmask = 0;

	sc->sc_dev = self;
	sc->slotno = apa->apa_slotno;
	if (systype == NEWS4000 && sc->slotno == 0) {
		sc->sc_flags |= F_NWS40S0;
	}
	if ((sc->sc_flags & F_NWS40S0) != 0) {
		sc->sc_hwbase = (void *)apa->apa_hwbase;
		sc->sc_regbase = (void *)apa->apa_hwbase;
		sc->memory = (void *)NEWS4000_SONIC_MEMORY;
		sc->buffer = (void *)NEWS4000_SONIC_BUFFER;
	} else {
		sc->sc_hwbase = (void *)apa->apa_hwbase;
		sc->sc_regbase =
		    (void *)(apa->apa_hwbase + SONIC_APBUS_REG_OFFSET);
		sc->memory =
		    (void *)(apa->apa_hwbase + SONIC_APBUS_MEM_OFFSET);
	}

	aprint_normal(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);

	if ((sc->sc_flags & F_NWS40S0) != 0) {
		sc->snr_dcr = DCR_STERM | DCR_TFT28;
		sc->snr_dcr2 = 0x0100;
	} else {
		sc->snr_dcr = DCR_WAIT0 | DCR_DMABLOCK | DCR_RFT16 | DCR_TFT16;
		sc->snr_dcr2 = 0;
		sc->snr_dcr |= DCR_EXBUS;
	}
	sc->bitmode = 1;

	if (sn_ap_getaddr(sc, myaddr)) {
		aprint_error(": failed to get MAC address\n");
		return;
	}

	aprint_normal("\n");

	/* snsetup returns 1 if something fails */
	if (snsetup(sc, myaddr))
		return;

	if (systype == NEWS5000) {
		intrmask = (apa->apa_slotno == 0) ?
		    NEWS5000_INT0_SONIC : NEWS5000_SLOTTOMASK(apa->apa_slotno);
	}
	if (systype == NEWS4000) {
		intrmask = (apa->apa_slotno == 0) ?
		    NEWS4000_INT0_SONIC : NEWS4000_SLOTTOMASK(apa->apa_slotno);
	}

	apbus_intr_establish(0, /* interrupt level (0 or 1) */
	    intrmask,
	    0, /* priority */
	    snintr, sc, device_xname(self), apa->apa_ctlnum);
}

int
sn_ap_getaddr(struct sn_softc *sc, uint8_t *lladdr)
{

	if ((sc->sc_flags & F_NWS40S0) != 0) {
		extern struct idrom idrom;
		memcpy(lladdr, idrom.id_macadrs, sizeof(idrom.id_macadrs));
	} else {
		uint32_t *p;
		int i;

		p = (uint32_t *)((uint8_t *)sc->sc_hwbase +
		    SONIC_MACROM_OFFSET);
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			int h = *p++ & 0x0f;
			int l = *p++ & 0x0f;
			*lladdr++ = (h << 4) + l;
		}
	}

	return 0;
}

#define APSONIC_INT_MASK	0x00007f00	/* XXX */
#define	APSONIC_INT_REG(base)	(((u_long)(base) & 0xffc00000) | 0x00100000)

void
sn_md_init(struct sn_softc *sc)
{

	if ((sc->sc_flags & F_NWS40S0) != 0) {
		*(volatile uint32_t *)0xbf700000 = 1;
	} else {
		volatile uint32_t *reg =
		    (uint32_t *)APSONIC_INT_REG(sc->sc_hwbase);

		*reg = APSONIC_INT_MASK;
	}
	wbflush();
	apbus_wbflush();
	delay(10000);
}
