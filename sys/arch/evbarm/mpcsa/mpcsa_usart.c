/*	$Id: mpcsa_usart.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $	*/
/*	$NetBSD: mpcsa_usart.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcsa_usart.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91usartvar.h>
#include <arm/at91/at91piovar.h>
#include <arm/at91/at91rm9200reg.h>
#include <evbarm/mpcsa/mpcsa_io.h>
#include <evbarm/mpcsa/mpcsa_leds_var.h>
#include <sys/unistd.h>

#ifdef MPCSA_USART_DEBUG
int mpcsa_usart_debug = MPCSA_USART_DEBUG;
#define DPRINTFN(n,x)	if (mpcsa_usart_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)   
#endif  

struct at91pio_softc;

struct mpcsa_usart_softc {
	struct at91usart_softc	sc_dev;
	struct at91pio_softc	*sc_pioa, *sc_piob, *sc_piod;
	void *sc_cts_ih;
	int sc_tx_busy, sc_rx_busy;
};

static int mpcsa_usart_match(struct device *, struct cfdata *, void *);
static void mpcsa_usart_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mpcsa_usart, sizeof(struct mpcsa_usart_softc),
	      mpcsa_usart_match, mpcsa_usart_attach, NULL, NULL);

static int mpcsa_usart_enable(struct at91usart_softc *sc);
static int mpcsa_usart_disable(struct at91usart_softc *sc);
static void mpcsa_usart_hwflow(struct at91usart_softc *sc, int cflags);
static void mpcsa_usart_start_tx(struct at91usart_softc *sc);
static void mpcsa_usart_stop_tx(struct at91usart_softc *sc);
static void mpcsa_usart_rx_started(struct at91usart_softc *sc);
static void mpcsa_usart_rx_stopped(struct at91usart_softc *sc);
static void mpcsa_usart_rx_rts_ctl(struct at91usart_softc *sc, int enabled);

static int mpcsa_gsm_cts_intr(void *);

static __inline int
led_num(struct mpcsa_usart_softc *mpsc)
{
	return (mpsc->sc_dev.sc_pid == PID_US3 ? LED_GSM : LED_SER1 + mpsc->sc_dev.sc_pid - PID_US0);
}

static __inline void
comm_led(struct mpcsa_usart_softc *mpsc, int count)
{
	mpcsa_comm_led(led_num(mpsc), count);
}

static __inline void
conn_led(struct mpcsa_usart_softc *mpsc, int count)
{
	mpcsa_conn_led(led_num(mpsc), count);
}

static int
mpcsa_usart_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "at91usart") == 0 && strcmp(match->cf_atname, "mpcsa_usart") == 0)
		return 2;
	return 0;
}


static void
mpcsa_usart_attach(struct device *parent, struct device *self, void *aux)
{
	struct mpcsa_usart_softc *sc = (struct mpcsa_usart_softc *)self;
	struct at91bus_attach_args *sa = aux;

	// initialize softc
	if ((sc->sc_pioa = at91pio_sc(AT91_PIOA)) == NULL) {
		printf("no PIOA!\n");
		return;
	}
	if ((sc->sc_piob = at91pio_sc(AT91_PIOB)) == NULL) {
		printf("no PIOB!\n");
		return;
	}
	if ((sc->sc_piod = at91pio_sc(AT91_PIOD)) == NULL) {
		printf("no PIOD!\n");
		return;
	}

	// calculate unit number...
	switch (sa->sa_pid) {
	case PID_US0:
	case PID_US1:
	case PID_US2:
	case PID_US3:
		sc->sc_dev.enable = mpcsa_usart_enable;
		sc->sc_dev.disable = mpcsa_usart_disable;
		sc->sc_dev.hwflow = mpcsa_usart_hwflow;
		sc->sc_dev.start_tx = mpcsa_usart_start_tx;
		sc->sc_dev.stop_tx = mpcsa_usart_stop_tx;
		sc->sc_dev.rx_started = mpcsa_usart_rx_started;
		sc->sc_dev.rx_stopped = mpcsa_usart_rx_stopped;
		sc->sc_dev.rx_rts_ctl = mpcsa_usart_rx_rts_ctl;
		break;
	}

	/* configure pins */
	switch (sa->sa_pid) {
	case PID_US0:
		at91pio_set(sc->sc_piob, PB_RTS1);
		at91pio_set(sc->sc_piod, PD_DTR1);
		at91pio_in(sc->sc_piob, PB_CTS1);
		at91pio_out(sc->sc_piob, PB_RTS1);
		at91pio_in(sc->sc_piod, PD_DSR1);
		at91pio_out(sc->sc_piod, PD_DTR1);
		at91pio_per(sc->sc_piob, PB_CTS1, -1);
		at91pio_per(sc->sc_piob, PB_RTS1, -1);
		at91pio_per(sc->sc_piod, PD_DSR1, -1);
		at91pio_per(sc->sc_piod, PD_DTR1, -1);
		break;
	case PID_US1:
		at91pio_set(sc->sc_piob, PB_RTS2);
		at91pio_set(sc->sc_piod, PD_DTR2);
		at91pio_in(sc->sc_piob, PB_CTS2);
		at91pio_out(sc->sc_piob, PB_RTS2);
		at91pio_in(sc->sc_piod, PD_DSR2);
		at91pio_out(sc->sc_piod, PD_DTR2);
		at91pio_per(sc->sc_piob, PB_CTS2, -1);
		at91pio_per(sc->sc_piob, PB_RTS2, -1);
		at91pio_per(sc->sc_piod, PD_DSR2, -1);
		at91pio_per(sc->sc_piod, PD_DTR2, -1);
		break;
	case PID_US2:
		at91pio_set(sc->sc_piob, PB_RTS3);
		at91pio_set(sc->sc_piod, PD_DTR3);
		at91pio_in(sc->sc_piob, PB_CTS3);
		at91pio_out(sc->sc_piob, PB_RTS3);
		at91pio_in(sc->sc_piod, PD_DSR3);
		at91pio_out(sc->sc_piod, PD_DTR3);
		at91pio_per(sc->sc_piob, PB_CTS3, -1);
		at91pio_per(sc->sc_piob, PB_RTS3, -1);
		at91pio_per(sc->sc_piod, PD_DSR3, -1);
		at91pio_per(sc->sc_piod, PD_DTR3, -1);
		break;
	case PID_US3:
		/* configure pin states... */
		at91pio_clear(sc->sc_pioa, PA_GSMON);
		at91pio_set(sc->sc_pioa, PA_GSMOFF);
		at91pio_set(sc->sc_piob, PB_RTS4);
		at91pio_set(sc->sc_piod, PD_DTR4);

		/* configure pin directions.. */
		at91pio_out(sc->sc_pioa, PA_GSMOFF);
		at91pio_out(sc->sc_pioa, PA_GSMON);
		at91pio_in(sc->sc_pioa, PA_TXD4);
		at91pio_in(sc->sc_piob, PB_RTS4);
		at91pio_in(sc->sc_piob, PB_CTS4);
		at91pio_in(sc->sc_piod, PD_DTR4);
		at91pio_in(sc->sc_piod, PD_DSR4);
		at91pio_in(sc->sc_piod, PD_DCD4);

		/* make sure all related pins are configured as PIO */
		at91pio_per(sc->sc_pioa, PA_GSMOFF, -1);
		at91pio_per(sc->sc_pioa, PA_GSMON, -1);
		at91pio_per(sc->sc_pioa, PA_TXD4, -1);
		at91pio_per(sc->sc_piob, PB_CTS4, -1);
		at91pio_per(sc->sc_piob, PB_RTS4, -1);
		at91pio_per(sc->sc_piod, PD_DSR4, -1);
		at91pio_per(sc->sc_piod, PD_DTR4, -1);
		at91pio_per(sc->sc_piod, PD_DCD4, -1);
		break;
	}

	// and call common routine
	at91usart_attach_subr(&sc->sc_dev, sa);
}

static int
mpcsa_usart_enable(struct at91usart_softc *dev)
{
	struct mpcsa_usart_softc *sc = (struct mpcsa_usart_softc *)dev;
	conn_led(sc, 1);
	switch (sc->sc_dev.sc_pid) {
	case PID_US3:
		/* turn gsm on */
		at91pio_clear(sc->sc_pioa, PA_GSMOFF);
		ltsleep(sc, 0, "gsmond", 4 * hz, NULL);
		at91pio_set(sc->sc_pioa, PA_GSMON);
		ltsleep(sc, 0, "gsmon", 2 * hz, NULL);
		at91pio_clear(sc->sc_pioa, PA_GSMON);
		/* then attach pins to devices etc */
		at91pio_per(sc->sc_pioa, PA_TXD4, 1);
		at91pio_clear(sc->sc_piob, PB_RTS4);
		at91pio_clear(sc->sc_piod, PD_DTR4);
		at91pio_out(sc->sc_piob, PB_RTS4);
		at91pio_out(sc->sc_piod, PD_DTR4);
		/* catch CTS interrupt */
		sc->sc_cts_ih = at91pio_intr_establish(sc->sc_piob, PB_CTS4,
						       IPL_TTY, mpcsa_gsm_cts_intr,
						       sc);
		break;
	}
	return 0;
}

static int
mpcsa_usart_disable(struct at91usart_softc *dev)
{
	struct mpcsa_usart_softc *sc = (struct mpcsa_usart_softc *)dev;
	if (sc->sc_tx_busy || sc->sc_rx_busy) {
		sc->sc_tx_busy = sc->sc_rx_busy = 0;
		comm_led(sc, 1);
	}
	switch (sc->sc_dev.sc_pid) {
	case PID_US3:
		at91pio_intr_disestablish(sc->sc_piob, PB_CTS4, sc->sc_cts_ih);

		at91pio_clear(sc->sc_pioa, PA_GSMON);
		ltsleep(sc, 0, "gsmoffd", (hz * 350 + 999) / 1000, NULL);

		at91pio_per(sc->sc_pioa, PA_TXD4, -1);
		at91pio_in(sc->sc_piob, PB_RTS4);
		at91pio_in(sc->sc_piod, PD_DTR4);
		
		at91pio_set(sc->sc_pioa, PA_GSMOFF);
		ltsleep(sc, 0, "gsmoff", hz * 4, NULL);
		at91pio_clear(sc->sc_pioa, PA_GSMOFF);

		break;
	}
	conn_led(sc, 0);
	return 0;
}

static int mpcsa_gsm_cts_intr(void *cookie)
{
	struct mpcsa_usart_softc *sc = (struct mpcsa_usart_softc *)cookie;
	if (ISSET(sc->sc_dev.sc_swflags, TIOCFLAG_CRTSCTS)) {
		/* hardware flow control is enabled */
		if (!(PIOB_READ(PIO_PDSR) & (1U << PB_CTS4))) {
			if (bus_space_read_4(sc->sc_dev.sc_iot, sc->sc_dev.sc_ioh,
					     US_PDC + PDC_TCR) && !sc->sc_tx_busy) {
				sc->sc_tx_busy = 1;
				if (!sc->sc_rx_busy)
					comm_led(sc, INFINITE_BLINK);
			}

			bus_space_write_4(sc->sc_dev.sc_iot, sc->sc_dev.sc_ioh,
					  US_PDC + PDC_PTCR, PDC_PTCR_TXTEN);
			SET(sc->sc_dev.sc_ier, US_CSR_TXEMPTY | US_CSR_ENDTX);
			bus_space_write_4(sc->sc_dev.sc_iot, sc->sc_dev.sc_ioh,
					  US_IER, US_CSR_ENDTX);
		} else {
			bus_space_write_4(sc->sc_dev.sc_iot, sc->sc_dev.sc_ioh,
					  US_PDC + PDC_PTCR, PDC_PTCR_TXTDIS);
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				if (!sc->sc_rx_busy)
					comm_led(sc, 1);
			}
		}
	}
	return 0;
}

static void
mpcsa_usart_hwflow(struct at91usart_softc *dev, int flags)
{
}

static void
mpcsa_usart_start_tx(struct at91usart_softc *sc)
{
	if (!ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS)
	    || bus_space_read_4(sc->sc_iot, sc->sc_ioh, US_PDC + PDC_PTSR) & PDC_PTSR_TXTEN) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  US_PDC + PDC_PTCR, PDC_PTCR_TXTEN);
		struct mpcsa_usart_softc *mpsc = (void*)sc;
		if (!mpsc->sc_tx_busy) {
			mpsc->sc_tx_busy = 1;
			if (!mpsc->sc_rx_busy)
				comm_led(mpsc, INFINITE_BLINK);
		}
		return;
	}
}

static void
mpcsa_usart_stop_tx(struct at91usart_softc *sc)
{
	struct mpcsa_usart_softc *mpsc = (void*)sc;
	mpsc->sc_tx_busy = 0;
	if (!mpsc->sc_rx_busy)
		comm_led(mpsc, 1);
	if (!ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS)) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  US_PDC + PDC_PTCR, PDC_PTCR_TXTDIS);
	}
}

static void
mpcsa_usart_rx_started(struct at91usart_softc *sc)
{
	struct mpcsa_usart_softc *mpsc = (void*)sc;
	if (!mpsc->sc_rx_busy) {
		mpsc->sc_rx_busy = 1;
		if (!mpsc->sc_tx_busy)
			comm_led(mpsc, INFINITE_BLINK);
	}
}

static void
mpcsa_usart_rx_stopped(struct at91usart_softc *sc)
{
	struct mpcsa_usart_softc *mpsc = (void*)sc;
	mpsc->sc_rx_busy = 0;
	if (!mpsc->sc_tx_busy)
		comm_led(mpsc, 1);
}

static void
mpcsa_usart_rx_rts_ctl(struct at91usart_softc *sc, int enabled)
{
	struct mpcsa_usart_softc *mpsc = (void*)sc;

	switch (mpsc->sc_dev.sc_pid) {
	case PID_US0:
		if (enabled)
			at91pio_set(mpsc->sc_piob, PB_RTS1);
		else
			at91pio_clear(mpsc->sc_piob, PB_RTS1);
		break;

	case PID_US1:
		if (enabled)
			at91pio_set(mpsc->sc_piob, PB_RTS2);
		else
			at91pio_clear(mpsc->sc_piob, PB_RTS2);
		break;

	case PID_US2:
		if (enabled)
			at91pio_set(mpsc->sc_piob, PB_RTS3);
		else
			at91pio_clear(mpsc->sc_piob, PB_RTS3);
		break;

	case PID_US3:
		if (enabled)
			at91pio_set(mpsc->sc_piob, PB_RTS4);
		else
			at91pio_clear(mpsc->sc_piob, PB_RTS4);
		break;

	}
	
}

