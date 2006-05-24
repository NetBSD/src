/*	$NetBSD: optpoint.c,v 1.3.12.1 2006/05/24 15:47:56 tron Exp $ */

/*-
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * OptOpint on Telios HC-AJ2
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: optpoint.c,v 1.3.12.1 2006/05/24 15:47:56 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/config_hook.h>
#include <dev/hpc/hpciovar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39spivar.h>
#include <hpcmips/tx/tx39icureg.h>

#undef	OPTPOINTDEBUG

#ifdef OPTPOINTDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

struct optpoint_softc {
	struct device sc_dev;
	tx_chipset_tag_t sc_tc;
	struct tx39spi_softc *sc_spi;
	struct hpcio_chip *sc_hc;
	struct device *sc_wsmousedev;
	void *sc_powerhook;	/* power management hook */
	char packet[4];
	int index;	/* number of bytes received for this packet */
	u_int buttons;	/* mouse button status */
	int enabled;
};

static int optpoint_match(struct device *, struct cfdata *, void *);
static void optpoint_attach(struct device *, struct device *, void *);
static int optpoint_intr(void *);
static int optpoint_enable(void *);
static void optpoint_disable(void *);
static int optpoint_ioctl(void *, u_long, caddr_t, int, struct lwp *);
static int optpoint_initialize(void *);
static void optpoint_send(struct optpoint_softc *, int);
static int optpoint_recv(struct optpoint_softc *);
static int optpoint_power(void *, int, long, void *);

#define LBUTMASK 0x01
#define RBUTMASK 0x02

#define TX39_INTRSTATUS4_OPTPOINTINT	TX39_INTRSTATUS4_CARDIORDNEGINT
#define TX39_IO_MFIO_CARDREG	11
#define TX39_IO_MFIO_CARDIOWR	10
#define TX39_IO_MFIO_CARDIORD	9
#define TELIOS_MFIO_OPTP_T_RDY		TX39_IO_MFIO_CARDREG
#define TELIOS_MFIO_OPTP_C_REQ		TX39_IO_MFIO_CARDIOWR
#define TELIOS_MFIO_OPTP_S_ENB_N	TX39_IO_MFIO_CARDIORD

CFATTACH_DECL(optpoint, sizeof(struct optpoint_softc),
    optpoint_match, optpoint_attach, NULL, NULL);

const struct wsmouse_accessops optpoint_accessops = {
	optpoint_enable,
	optpoint_ioctl,
	optpoint_disable,
};

int
optpoint_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_NORMAL);
}

void
optpoint_attach(struct device *parent, struct device *self, void *aux)
{
	struct txspi_attach_args *ta = aux;
	struct optpoint_softc *sc = (void*)self;
	struct tx39spi_softc *spi = sc->sc_spi = (void*)parent;
	tx_chipset_tag_t tc = sc->sc_tc = ta->sa_tc;
	struct wsmousedev_attach_args wsmaa;

	sc->sc_hc = tc->tc_iochip[MFIO];
	sc->enabled = 0;

	/* Specific SPI settings for OptPoint of HC-AJ2 */
	tx39spi_delayval(spi, 0);
	tx39spi_baudrate(spi, 4);	/* SPICLK Rate = 737.3 kHz */
	tx39spi_word(spi, 0);		/* Use 8bits of data */
	tx39spi_phapol(spi, 0);
	tx39spi_clkpol(spi, 1);
	tx39spi_lsb(spi, 0);		/* MSB first */

	optpoint_enable(sc);
	printf("\n");

	wsmaa.accessops = &optpoint_accessops;
	wsmaa.accesscookie = sc;
	/* attach the wsmouse */
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);

	/* Add a hard power hook to power saving */
	sc->sc_powerhook = config_hook(CONFIG_HOOK_PMEVENT,
				       CONFIG_HOOK_PMEVENT_HARDPOWER,
				       CONFIG_HOOK_SHARE,
				       optpoint_power, sc);
#ifdef DIAGNOSTIC
	if (sc->sc_powerhook == 0)
		printf("%s: unable to establish hard power hook",
		       sc->sc_dev.dv_xname);
#endif
}

int
optpoint_intr(void *self)
{
	struct optpoint_softc *sc = (void*)self;
	tx_chipset_tag_t tc = sc->sc_tc;
	char data = optpoint_recv(sc) & 0xff;

#ifdef DIAGNOSTIC
	if (sc->index >= 3){
		printf("%s: Receive buffer overflow\n", sc->sc_dev.dv_xname);
		sc->index = 0;
		memset(sc->packet, 0, 3);
	}
#endif
	if ((sc->index == 1) && (data & 0xcc) != 0x08){
		DPRINTF(("%s: Bad second byte (0x%02x)\n",
			 sc->sc_dev.dv_xname, data));
		tx_conf_write(tc, TX39_INTRCLEAR4_REG,
				  TX39_INTRSTATUS4_OPTPOINTINT);
		return 0;
	}

	sc->packet[sc->index++] = data;
	if (sc->index >= 3){
		u_int newbuttons = ((sc->packet[1] & LBUTMASK) ? 0x1 : 0)
				 | ((sc->packet[1] & RBUTMASK) ? 0x2 : 0);
		int dx = sc->packet[2];
		int dy = sc->packet[0];
		u_int changed = (sc->buttons ^ newbuttons);

		if (dx || dy || changed){
			DPRINTF(("%s: buttons=0x%x, dx=%d, dy=%d\n",
				 sc->sc_dev.dv_xname, newbuttons, dx, dy));
			wsmouse_input(sc->sc_wsmousedev, newbuttons, dx, dy, 0,
				      WSMOUSE_INPUT_DELTA);
		}
		sc->buttons = newbuttons;
		sc->index = 0;
		memset(sc->packet, 0, 3);
	}
	tx_conf_write(tc, TX39_INTRCLEAR4_REG, TX39_INTRSTATUS4_OPTPOINTINT);

	return 0;
}

int
optpoint_enable(void *self)
{
	struct optpoint_softc *sc = (void*)self;

	if (!sc->enabled){
		tx_chipset_tag_t tc = sc->sc_tc;
		struct hpcio_chip *hc = sc->sc_hc;
		int s = spltty();

		DPRINTF(("%s: enable\n", sc->sc_dev.dv_xname));

		sc->enabled = 1;
		sc->index = 0;
		sc->buttons = 0;
		sc->packet[0] = 0xf5;	/* Disable */
		sc->packet[1] = 0xf2;	/* Set Stream Mode */
		sc->packet[2] = 0xf8;	/* Stanby */
		sc->packet[3] = 0xf4;	/* Enable */

		tx39spi_enable(sc->sc_spi, 1);
		tx_intr_establish(tc, MAKEINTR(4, TX39_INTRSTATUS4_OPTPOINTINT),
				  IST_EDGE, IPL_TTY, optpoint_initialize, sc);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_C_REQ, 1);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_T_RDY, 1);
		splx(s);
	}

        return 0;
}

void
optpoint_disable(void *self)
{
	struct optpoint_softc *sc = (void*)self;

	if (sc->enabled){
		tx_chipset_tag_t tc = sc->sc_tc;
		struct hpcio_chip *hc = sc->sc_hc;
		int s = spltty();

		DPRINTF(("%s: disable\n", sc->sc_dev.dv_xname));

		sc->enabled = 0;
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_C_REQ, 0);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_T_RDY, 0);
		tx_intr_disestablish(tc,
			    (void *)MAKEINTR(4, TX39_INTRSTATUS4_OPTPOINTINT));
		tx39spi_enable(sc->sc_spi, 0);
		splx(s);
	}
}

int
optpoint_ioctl(void *self, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;
		
	default:
		return (EPASSTHROUGH);
	}
        return 0;
}

int
optpoint_initialize(void *self)
{
	struct optpoint_softc *sc = (void*)self;
	struct hpcio_chip *hc = sc->sc_hc;
	tx_chipset_tag_t tc = sc->sc_tc;

	if (sc->index < 4){
		optpoint_send(sc, sc->packet[sc->index++]);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_C_REQ, 1);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_T_RDY, 1);
	} else {
		tx_intr_disestablish(tc,
			    (void *)MAKEINTR(4, TX39_INTRSTATUS4_OPTPOINTINT));
		sc->index = 0;
		memset(sc->packet, 0, 3);
		tx_intr_establish(tc,
				  MAKEINTR(4, TX39_INTRSTATUS4_OPTPOINTINT),
				  IST_EDGE, IPL_TTY, optpoint_intr, sc);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_C_REQ, 0);
		(*hc->hc_portwrite)(hc, TELIOS_MFIO_OPTP_T_RDY, 1);
	}
	tx_conf_write(tc, TX39_INTRCLEAR4_REG, TX39_INTRSTATUS4_OPTPOINTINT);

	return 0;
}

void
optpoint_send(struct optpoint_softc *sc, int data)
{
	struct hpcio_chip *hc = sc->sc_hc;

	while ((*hc->hc_portread)(hc, TELIOS_MFIO_OPTP_S_ENB_N))
		;
	tx39spi_put_word(sc->sc_spi, data & 0xff);
}

int
optpoint_recv(struct optpoint_softc *sc)
{
	struct hpcio_chip *hc = sc->sc_hc;

	optpoint_send(sc, 0x00);
	while ((*hc->hc_portread)(hc, TELIOS_MFIO_OPTP_S_ENB_N))
		;
	return tx39spi_get_word(sc->sc_spi) & 0xff;
}

int
optpoint_power(void *self, int type, long id, void *msg)
{
	struct optpoint_softc *sc = (void *)self;
	int why = (int)msg;

	switch (why) {
	case PWR_RESUME:
		/* power on */
		optpoint_enable(sc);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		/* power off */
		optpoint_disable(sc);
		break;
	}

	return 0;
}
