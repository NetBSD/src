/*	$NetBSD: uda1341.c,v 1.6 2005/06/30 17:03:53 drochner Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uda1341.c,v 1.6 2005/06/30 17:03:53 drochner Exp $");

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

#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_gpioreg.h>
#include <hpcarm/dev/uda1341.h>
#include <hpcarm/sa11x0/sa11x0_gpioreg.h>
#include <hpcarm/sa11x0/sa11x0_sspreg.h>

struct uda1341_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct ipaq_softc	*sc_parent;
};

static	int	uda1341_match(struct device *, struct cfdata *, void *);
static	void	uda1341_attach(struct device *, struct device *, void *);
static	int	uda1341_print(void *, const char *);
static	int	uda1341_search(struct device *, struct cfdata *,
			       const locdesc_t *, void *);

static	void	uda1341_output_high(struct uda1341_softc *);
static	void	uda1341_output_low(struct uda1341_softc *);
static	void	uda1341_L3_init(struct uda1341_softc *);
static	void	uda1341_init(struct uda1341_softc *);
static	void	uda1341_reset(struct uda1341_softc *);
static	void	uda1341_reginit(struct uda1341_softc *);

static	int	L3_getbit(struct uda1341_softc *);
static	void	L3_sendbit(struct uda1341_softc *, int);
static	u_int8_t L3_getbyte(struct uda1341_softc *, int);
static	void	L3_sendbyte(struct uda1341_softc *, u_int8_t, int);
static	int	L3_read(struct uda1341_softc *, u_int8_t, u_int8_t *, int);
static	int	L3_write(struct uda1341_softc *, u_int8_t, u_int8_t *, int);

CFATTACH_DECL(uda, sizeof(struct uda1341_softc),
    uda1341_match, uda1341_attach, NULL, NULL);

/*
 * Philips L3 bus support.
 * GPIO lines are used for clock, data and mode pins.
 */
#define L3_DATA		GPIO_H3600_L3_DATA
#define L3_MODE		GPIO_H3600_L3_MODE
#define L3_CLK		GPIO_H3600_L3_CLK

static struct {
	u_int8_t data0;	/* direct addressing register */
} DIRECT_REG = {0};

static struct {
	u_int8_t data0;	/* extended addressing register 1 */
	u_int8_t data1;	/* extended addressing register 2 */
} EXTEND_REG = {0, 0};

/*
 * register space access macros
 */
#define GPIO_WRITE(sc, reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_parent->sc_gpioh, reg, val)
#define GPIO_READ(sc, reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_parent->sc_gpioh, reg)
#define EGPIO_WRITE(sc) \
	bus_space_write_2(sc->sc_iot, sc->sc_parent->sc_egpioh, \
			  0, sc->sc_parent->ipaq_egpio)
#define SSP_WRITE(sc, reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_parent->sc_ssph, reg, val)

static int
uda1341_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

static void
uda1341_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct uda1341_softc *sc = (struct uda1341_softc *)self;
	struct ipaq_softc *psc = (struct ipaq_softc *)parent;

	printf("\n");
	printf("%s: UDA1341 CODEC\n",  sc->sc_dev.dv_xname);

	sc->sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_parent = (struct ipaq_softc *)parent;

	uda1341_L3_init(sc);
	uda1341_init(sc);

	uda1341_reset(sc);

	uda1341_reginit(sc);
	

	/*
	 *  Attach each devices
	 */

	config_search_ia(uda1341_search, self, "udaif", NULL);
}

static int
uda1341_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const locdesc_t *ldesc;
	void *aux;
{
	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, uda1341_print);
	return 0;
}


static int
uda1341_print(aux, name)
	void *aux;
	const char *name;
{
	return (UNCONF);
}

static void
uda1341_output_high(sc)
	struct uda1341_softc *sc;
{
	int cr;

	GPIO_WRITE(sc, SAGPIO_PSR, (L3_DATA | L3_MODE | L3_CLK));
	cr = GPIO_READ(sc, SAGPIO_PDR) | (L3_DATA | L3_MODE | L3_CLK);
	GPIO_WRITE(sc, SAGPIO_PDR, cr);
}

static void
uda1341_output_low(sc)
	struct uda1341_softc *sc;
{
	int cr;

	cr = GPIO_READ(sc, SAGPIO_PDR);
	cr &= ~(L3_DATA | L3_MODE | L3_CLK);
	GPIO_WRITE(sc, SAGPIO_PDR, cr);
}

static void
uda1341_L3_init(sc)
	struct uda1341_softc *sc;
{
	int cr;

	cr = GPIO_READ(sc, SAGPIO_AFR);
	cr &= ~(L3_DATA | L3_MODE | L3_CLK);
	GPIO_WRITE(sc, SAGPIO_AFR, cr);

	uda1341_output_low(sc);
}

static void
uda1341_init(sc)
	struct uda1341_softc *sc;
{
	int cr; 

	/* GPIO initialize */
	cr = GPIO_READ(sc, SAGPIO_AFR);
	cr &= ~(GPIO_ALT_SSP_TXD | GPIO_ALT_SSP_RXD | GPIO_ALT_SSP_SCLK |
		GPIO_ALT_SSP_SFRM);
	cr |= GPIO_ALT_SSP_CLK;
	GPIO_WRITE(sc, SAGPIO_AFR, cr);

	cr = GPIO_READ(sc, SAGPIO_PDR);
	cr &= ~GPIO_ALT_SSP_CLK;
	GPIO_WRITE(sc, SAGPIO_PDR, cr);

	/* SSP initialize & enable */
	SSP_WRITE(sc, SASSP_CR1, CR1_ECS);
	cr = 0xF | (CR0_FRF_MASK & (1<<4)) | (CR0_SCR_MASK & (3<<8)) | CR0_SSE;
	SSP_WRITE(sc, SASSP_CR0, cr);

	/* Enable the audio power */
	sc->sc_parent->ipaq_egpio |= 
			(EGPIO_H3600_AUD_PWRON | EGPIO_H3600_AUD_ON);
	sc->sc_parent->ipaq_egpio &=
			~(EGPIO_H3600_CODEC_RESET | EGPIO_H3600_QMUTE);
	EGPIO_WRITE(sc);

	/* external clock configured for 44100 samples/sec */
	cr = GPIO_READ(sc, SAGPIO_PDR);
	cr |= (GPIO_H3600_CLK_SET0 | GPIO_H3600_CLK_SET1);
	GPIO_WRITE(sc, SAGPIO_PDR, cr);
	GPIO_WRITE(sc, SAGPIO_PSR, GPIO_H3600_CLK_SET0);
	GPIO_WRITE(sc, SAGPIO_PCR, GPIO_H3600_CLK_SET1);

	/* wait for power on */
	delay(100*1000);
	sc->sc_parent->ipaq_egpio |= EGPIO_H3600_CODEC_RESET;
	EGPIO_WRITE(sc);

	/* Wait for the UDA1341 to wake up */
	delay(100*1000);
}

static void
uda1341_reset(sc)
	struct uda1341_softc *sc;
{       
	u_int8_t command;

	command = (L3_ADDRESS_COM << 2) | L3_ADDRESS_STATUS;
	DIRECT_REG.data0 = STATUS0_RST | STATUS0_SC_256 | STATUS0_IF_LSB16;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	sc->sc_parent->ipaq_egpio &= ~EGPIO_H3600_CODEC_RESET;
	EGPIO_WRITE(sc);
	sc->sc_parent->ipaq_egpio |= EGPIO_H3600_CODEC_RESET;
	EGPIO_WRITE(sc);

	DIRECT_REG.data0 &= ~STATUS0_RST;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);
}

static void
uda1341_reginit(sc)
	struct uda1341_softc *sc;
{
	u_int8_t command;

	/* STATUS 0 */
	command = (L3_ADDRESS_COM << 2) | L3_ADDRESS_STATUS;
	DIRECT_REG.data0 = STATUS0_SC_256 | STATUS0_IF_LSB16;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	/* STATUS 1 */
	DIRECT_REG.data0 = STATUS1_OGS | STATUS1_IGS | (1<<7);
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	/* DATA 0 */
	command = (L3_ADDRESS_COM << 2) | L3_ADDRESS_DATA0;
	DIRECT_REG.data0 = DATA0_VC(100) | DATA0_COMMON;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	/* DATA 1 */ 
	DIRECT_REG.data0 = DATA1_BB(0) | DATA1_TR(0) | DATA1_COMMON;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	/* DATA 2*/
	DIRECT_REG.data0 = DATA2_PP | DATA2_COMMON;
	L3_write(sc, command, (u_int8_t *) &DIRECT_REG, 1);

	/* Extended DATA 0 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E0;
	EXTEND_REG.data1 = EXT_DATA_COMMN | 0x4 ;
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);

	/* Extended DATA 1 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E1;
	EXTEND_REG.data1 = EXT_DATA_COMMN | 0x4 ;
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);

	/* Extended DATA 2 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E2;
	EXTEND_REG.data1 = EXT_DATA_COMMN | DATA_E2_MS(30);
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);

	/* Extended DATA 3 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E3;
	EXTEND_REG.data1 = EXT_DATA_COMMN | DATA_E3_IG_L(0);
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);

	/* Extended DATA 4 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E4;
	EXTEND_REG.data1 = EXT_DATA_COMMN | DATA_E4_IG_H(0);
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);

	/* Extended DATA 5 */
	EXTEND_REG.data0 = EXT_ADDR_COMMON | EXT_ADDR_E5;
	EXTEND_REG.data1 = EXT_DATA_COMMN;
	L3_write(sc, command, (u_int8_t *) &EXTEND_REG, 2);
}

static int
L3_getbit(sc)
	struct uda1341_softc *sc;
{
	int cr, data;

	GPIO_WRITE(sc, SAGPIO_PCR, L3_CLK);	/* Clock down */
	delay(L3_CLK_LOW);

	cr = GPIO_READ(sc, SAGPIO_PLR);
	data = (cr & L3_DATA) ? 1 : 0;

	GPIO_WRITE(sc, SAGPIO_PSR, L3_CLK);	/* Clock up */
	delay(L3_CLK_HIGH);

	return (data);
}

static void
L3_sendbit(sc, bit)
	struct uda1341_softc *sc;
	int bit;
{
	GPIO_WRITE(sc, SAGPIO_PCR, L3_CLK);	/* Clock down */
	
	if (bit & 0x01)
		GPIO_WRITE(sc, SAGPIO_PSR, L3_DATA);
	else
		GPIO_WRITE(sc, SAGPIO_PCR, L3_DATA);

	delay(L3_CLK_LOW);
	GPIO_WRITE(sc, SAGPIO_PSR, L3_CLK);     /* Clock up */
	delay(L3_CLK_HIGH);
}

static u_int8_t
L3_getbyte(sc, mode)
	struct uda1341_softc *sc;
	int mode;
{
	int i;
	u_int8_t data;

	switch (mode) {
	case 0:		/* Address mode */
	case 1:		/* First data byte */
		break;
	default:	/* second data byte via halt-Time */
		GPIO_WRITE(sc, SAGPIO_PCR, L3_CLK);     /* Clock down */
		delay(L3_HALT);
		GPIO_WRITE(sc, SAGPIO_PSR, L3_CLK);	/* Clock up */
		break;
	}

	delay(L3_MODE_SETUP);

	for (i = 0; i < 8; i++)
		data |= (L3_getbit(sc) << i);

	delay(L3_MODE_HOLD);

	return (data);
}

static void
L3_sendbyte(sc, data, mode)
	struct uda1341_softc *sc;
	u_int8_t data;
	int mode;
{
	int i;

	switch (mode) {
	case 0:		/* Address mode */
		GPIO_WRITE(sc, SAGPIO_PCR, L3_CLK);	/* Clock down */
		break;
	case 1:		/* First data byte */
		break;
	default:	/* second data byte via halt-Time */
		GPIO_WRITE(sc, SAGPIO_PCR, L3_CLK);     /* Clock down */
		delay(L3_HALT);
		GPIO_WRITE(sc, SAGPIO_PSR, L3_CLK);	/* Clock up */
		break;
	}

	delay(L3_MODE_SETUP);

	for (i = 0; i < 8; i++)
		L3_sendbit(sc, data >> i);

	if (mode == 0)		/* Address mode */
		GPIO_WRITE(sc, SAGPIO_PSR, L3_CLK);	/* Clock up */

	delay(L3_MODE_HOLD);
}

static int
L3_read(sc, addr, data, len)
	struct uda1341_softc *sc;
	u_int8_t addr, *data;
        int len;
{
	int cr, mode;
	mode = 0;

	uda1341_output_high(sc);
	L3_sendbyte(sc, addr, mode++);

	cr = GPIO_READ(sc, SAGPIO_PDR);
	cr &= ~(L3_DATA);
	GPIO_WRITE(sc, SAGPIO_PDR, cr);

	while(len--)
		*data++ = L3_getbyte(sc, mode++);
	uda1341_output_low(sc);

	return len;
}

static int
L3_write(sc, addr, data, len)
	struct uda1341_softc *sc;
	u_int8_t addr, *data;
	int len;
{
	int mode = 0;

	uda1341_output_high(sc);
	L3_sendbyte(sc, addr, mode++);
	while(len--)
		L3_sendbyte(sc, *data++, mode++);
	uda1341_output_low(sc);

	return len;
}
