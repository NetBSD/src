/*	$NetBSD: if_sn_ap.c,v 1.1 1999/12/22 05:55:24 tsubai Exp $	*/

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

#define SONIC_MACROM_OFFSET	0x40

#define SONIC_APBUS_REG_OFFSET	0x00010000
#define SONIC_APBUS_MEM_OFFSET	0x00020000
#define SONIC_APBUS_CTL_OFFSET	(-0x00100000)

static int	sn_ap_match __P((struct device *, struct cfdata *, void *));
static void	sn_ap_attach __P((struct device *, struct device *, void *));
static int	sn_ap_getaddr __P((struct sn_softc *, u_int8_t *));

struct cfattach sn_ap_ca = {
	sizeof(struct sn_softc), sn_ap_match, sn_ap_attach
};

static int
sn_ap_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
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
sn_ap_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;
{
	struct sn_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;
	u_int8_t myaddr[ETHER_ADDR_LEN];
	u_int intrmask;

	sc->sc_hwbase = (caddr_t)apa->apa_hwbase;
	sc->sc_regbase = (void *)(apa->apa_hwbase + SONIC_APBUS_REG_OFFSET);
	sc->space = (void *)(apa->apa_hwbase + SONIC_APBUS_MEM_OFFSET);

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);

	sc->snr_dcr = DCR_WAIT0 | DCR_DMABLOCK | DCR_RFT16 | DCR_TFT16;
	sc->snr_dcr2 = 0;
	sc->snr_dcr |= DCR_EXBUS;
	sc->bitmode = 1;

	if (sn_ap_getaddr(sc, myaddr)) {
		printf(": failed to get MAC address\n");
		return;
	}

	printf("\n");

	/* snsetup returns 1 if something fails */
	if (snsetup(sc, myaddr))
		return;

	intrmask = (apa->apa_slotno == 0) ?
		NEWS5000_INT0_SONIC : SLOTTOMASK(apa->apa_slotno);

	apbus_intr_establish(0, /* interrupt level (0 or 1) */
			     intrmask,
			     0, /* priority */
			     snintr, sc, apa->apa_name, apa->apa_ctlnum);
}

int
sn_ap_getaddr(sc, lladdr)
	struct sn_softc	*sc;
	u_int8_t *lladdr;
{
	u_int *p = (u_int *)(sc->sc_hwbase + SONIC_MACROM_OFFSET);
	int i;

	for (i = 0; i < 6; i++) {
		int h = *p++ & 0x0f;
		int l = *p++ & 0x0f;
		*lladdr++ = (h << 4) + l;
	}

	return 0;
}

#define APSONIC_REG	0x00
#define APSONIC_NREGS	0x10

#define APSONIC_ENDIAN		0x80000000 /* Endian Display bit	*/
#define APSONIC_RDERR		0x00020000 /* DMA Error occur in RDMAC	*/
#define APSONIC_TDERR		0x00010000 /* DMA Error occur in TDMAC	*/
#define APSONIC_RDMACEN		0x00004000 /* RDMAC done Interrupt enable */
#define APSONIC_TDMACEN		0x00002000 /* TDMAC done Interrupt enable */
#define APSONIC_DERREN		0x00001000 /* DMA Error Interrupt enable */
#define APSONIC_CH3_INTEN	0x00000800 /* Channel 3 Interrupt enable */
#define APSONIC_CH2_INTEN	0x00000400 /* Channel 2 Interrupt enable */
#define APSONIC_CH1_INTEN	0x00000200 /* Channel 1 Interrupt enable */
#define APSONIC_CH0_INTEN	0x00000100 /* Channel 0 Interrupt enable */
#define APSONIC_RDMAC		0x00000040 /* RDMAC done Interrupt	*/
#define APSONIC_TDMAC		0x00000020 /* TDMAC done Interrupt	*/
#define APSONIC_DERR		0x00000010 /* DMA Error Interrupt	*/
#define APSONIC_CH3_INT		0x00000008 /* Channel 3 Interrupt	*/
#define APSONIC_CH2_INT		0x00000004 /* Channel 2 Interrupt	*/
#define APSONIC_CH1_INT		0x00000002 /* Channel 1 Interrupt	*/
#define APSONIC_CH0_INT		0x00000001 /* Channel 0 Interrupt	*/

#define APSONIC_INT_MASK (APSONIC_CH0_INTEN | \
			  APSONIC_CH1_INTEN | \
			  APSONIC_CH2_INTEN | \
			  APSONIC_CH3_INTEN | \
			  APSONIC_RDMACEN | \
			  APSONIC_TDMACEN | \
			  APSONIC_DERREN)

void
sn_md_init(sc)
	struct sn_softc *sc;
{
	u_int *reg = (u_int *)(sc->sc_hwbase - 0x00100000);

	reg[APSONIC_REG] = APSONIC_INT_MASK;
	wbflush();
	apbus_wbflush();
	delay(10000);
}
