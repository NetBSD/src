/*	$NetBSD: mediabay.c,v 1.1 1999/07/21 19:20:04 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

struct mediabay_softc {
	struct device sc_dev;
	int sc_node;
	u_int *sc_addr;
	u_int *sc_fcr;
};

static void mediabay_attach __P((struct device *, struct device *, void *));
static int mediabay_match __P((struct device *, struct cfdata *, void *));
static int mediabay_print __P((void *, const char *));

struct cfattach mediabay_ca = {
	sizeof(struct mediabay_softc), mediabay_match, mediabay_attach
};

int mediabay_intr __P((void *));

#define FCR_MEDIABAY_RESET	0x00000002
#define FCR_MEDIABAY_IDE_ENABLE	0x00000008
#define FCR_MEDIABAY_FD_ENABLE	0x00000010
#define FCR_MEDIABAY_ENABLE	0x00000080
#define FCR_MEDIABAY_CD_POWER	0x00800000

#define MEDIABAY_ID(x)		((x >> 12) & 0xf)
#define MEDIABAY_ID_FD		0
#define MEDIABAY_ID_CD		3
#define MEDIABAY_ID_NONE	7

int
mediabay_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "media-bay") == 0)
		return 1;

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
mediabay_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mediabay_softc *sc = (struct mediabay_softc *)self;
	struct confargs ca;
	int irq, child;
	u_int fcr;
	u_int reg[20], intr[5];
	char name[32];

	bcopy(aux, &ca, sizeof(ca));
	ca.ca_reg[0] += ca.ca_baseaddr;
	sc->sc_addr = mapiodev(ca.ca_reg[0], NBPG);
	sc->sc_fcr = sc->sc_addr + 1;
	sc->sc_node = ca.ca_node;
	irq = ca.ca_intr[0];

	printf(" irq %d\n", irq);
	intr_establish(irq, IST_LEVEL, IPL_BIO, mediabay_intr, sc);

	fcr = in32rb(sc->sc_fcr);
	fcr |= FCR_MEDIABAY_ENABLE | FCR_MEDIABAY_RESET;
	out32rb(sc->sc_fcr, fcr);
	delay(50000);

	fcr &= ~FCR_MEDIABAY_RESET;
	out32rb(sc->sc_fcr, fcr);
	delay(50000);

	fcr |= FCR_MEDIABAY_IDE_ENABLE | FCR_MEDIABAY_CD_POWER;
	out32rb(sc->sc_fcr, fcr);
	delay(50000);

	/* XXX */
	if (MEDIABAY_ID(in32rb(sc->sc_addr)) == MEDIABAY_ID_NONE)
		return;

	for (child = OF_child(sc->sc_node); child; child = OF_peer(child)) {
		bzero(name, sizeof(name));
		if (OF_getprop(child, "name", name, sizeof(name)) == -1)
			continue;
		ca.ca_name = name;
		ca.ca_node = child;

		ca.ca_nreg  = OF_getprop(child, "reg", reg, sizeof(reg));
		ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
				sizeof(intr));
		if (ca.ca_nintr == -1)
			ca.ca_nintr = OF_getprop(child, "interrupts", intr,
					sizeof(intr));
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		config_found(self, &ca, mediabay_print);
	}
}

int
mediabay_print(aux, mediabay)
	void *aux;
	const char *mediabay;
{
	struct confargs *ca = aux;

	if (mediabay)
		printf("%s at %s", ca->ca_name, mediabay);

	if (ca->ca_nreg > 0)
		printf(" offset 0x%x", ca->ca_reg[0]);

	return UNCONF;
}

int
mediabay_intr(v)
	void *v;
{
	struct mediabay_softc *sc = v;
	u_int x;

	printf("%s: ", sc->sc_dev.dv_xname);
	x = in32rb(sc->sc_addr);

	switch (MEDIABAY_ID(x)) {
	case MEDIABAY_ID_NONE:
		printf("removed\n");
		break;
	case MEDIABAY_ID_FD:
		printf("FD inserted\n");
		break;
	case MEDIABAY_ID_CD:
		printf("CD inserted\n");
		break;
	default:
		printf("unknown event (0x%x)\n", x);
	}

	return 1;
}

/* PBG3: 0x7025X0c0 */
/* 2400: 0x0070X0a8 */
