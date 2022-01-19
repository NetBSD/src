/*	$Id: mpcsa_leds.c,v 1.9 2022/01/19 05:21:44 thorpej Exp $	*/
/*	$NetBSD: mpcsa_leds.c,v 1.9 2022/01/19 05:21:44 thorpej Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/arm/ep93xx/epgpio.c,
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcsa_leds.c,v 1.9 2022/01/19 05:21:44 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/gpio.h>
#include <dev/spi/spivar.h>
#include <dev/gpio/gpiovar.h>
#include <evbarm/mpcsa/mpcsa_leds_var.h>
#include <evbarm/mpcsa/mpcsa_io.h>
#include "gpio.h"
#if NGPIO > 0
#include <sys/gpio.h>
#endif
#include "locators.h"

#ifdef MPCSA_LEDS_DEBUG
int mpcsa_leds_debug = MPCSA_LEDS_DEBUG;
#define DPRINTFN(n,x)		if (mpcsa_leds_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

#define	MPCSA_LEDS_NPINS		16
#define	LEDS_UPDATE_INTERVAL		100	/* update interval in milliseconds */

enum {
  LMODE_NORMAL,					/* normal mode */
  LMODE_COMM,					/* communication mode */
  LMODE_BLINK					/* blink mode */
};

typedef struct led_state {
	int			l_mode;		/* current command */
	union {
		struct {
			int	l_conn_cnt;	/* connection count */
			int	l_comm_cnt;	/* comm/pkt count */
		};
		struct {
			int	l_blink_int;	/* blink interval */
			int	l_blink_cnt;	/* blink counter */
		};
	};
} led_state_t;

struct mpcsa_leds_softc {
	struct spi_handle	*sc_sh;

#if NGPIO > 0
	struct gpio_chipset_tag	sc_gpio_chipset;
	gpio_pin_t		sc_pins[MPCSA_LEDS_NPINS];
#endif

	callout_t		sc_c;
	led_state_t		sc_leds[MPCSA_LEDS_NPINS];
	uint16_t		sc_pinstate;

	struct spi_chunk	sc_spi_chunk;
	struct spi_transfer	sc_spi_transfer;
};

static int mpcsa_leds_match(device_t , cfdata_t , void *);
static void mpcsa_leds_attach(device_t , device_t , void *);
static void mpcsa_leds_timer(void *aux);

#if NGPIO > 0
static int mpcsa_ledsbus_print(void *, const char *);
static int mpcsa_leds_pin_read(void *, int);
static void mpcsa_leds_pin_write(void *, int, int);
static void mpcsa_leds_pin_ctl(void *, int, int);
#endif

#if 0
static int mpcsa_leds_search(device_t , cfdata_t , const int *, void *);
static int mpcsa_leds_print(void *, const char *);
#endif

CFATTACH_DECL_NEW(mpcsa_leds, sizeof(struct mpcsa_leds_softc),
	      mpcsa_leds_match, mpcsa_leds_attach, NULL, NULL);

static struct mpcsa_leds_softc *mpcsa_leds_sc;

static int
mpcsa_leds_match(device_t parent, cfdata_t match, void *aux)
{

	if (strcmp(match->cf_name, "mpcsa_leds"))
		return 0;

	return 2;
}

static void
mpcsa_leds_attach(device_t parent, device_t self, void *aux)
{
	struct mpcsa_leds_softc *sc = device_private(self);
	struct spi_attach_args *sa = aux;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
#endif
	int n;
	int error;

	aprint_naive(": output buffer\n");
	aprint_normal(": 74HC595 or compatible shift register(s)\n");

	error = spi_configure(self, sa->sa_handle, SPI_MODE_0, 10000000);
	if (error) {
		return;
	}

	sc->sc_sh = sa->sa_handle;
	sc->sc_pinstate = 0xffff;
	callout_init(&sc->sc_c, 0);

#if NGPIO > 0
	/* initialize and attach gpio(4) */
	for (n = 0; n < MPCSA_LEDS_NPINS; n++) {
		sc->sc_pins[n].pin_num = n;
		sc->sc_pins[n].pin_caps = (GPIO_PIN_OUTPUT
					   | GPIO_PIN_PUSHPULL);
		sc->sc_pins[n].pin_flags = GPIO_PIN_OUTPUT | GPIO_PIN_LOW;
	}

	sc->sc_gpio_chipset.gp_cookie = sc;
	sc->sc_gpio_chipset.gp_pin_read = mpcsa_leds_pin_read;
	sc->sc_gpio_chipset.gp_pin_write = mpcsa_leds_pin_write;
	sc->sc_gpio_chipset.gp_pin_ctl = mpcsa_leds_pin_ctl;
	gba.gba_gc = &sc->sc_gpio_chipset;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = MPCSA_LEDS_NPINS;
	config_found(self, &gba, mpcsa_ledsbus_print, CFARGS_NONE);
#endif

	/* attach device */
//	config_search_ia(mpcsa_leds_search, self, "mpcsa_leds", mpcsa_leds_print);

	/* update leds ten times a second or so */
	mpcsa_leds_sc = sc; // @@@@
	sc->sc_spi_transfer.st_flags = SPI_F_DONE;
	callout_reset(&sc->sc_c, mstohz(LEDS_UPDATE_INTERVAL), mpcsa_leds_timer, sc);

	mpcsa_blink_led(LED_HB, 500);
}




#if NGPIO > 0
static int
mpcsa_ledsbus_print(void *aux, const char *name)
{
	gpiobus_print(aux, name);
	return (UNCONF);
}

    
static int
mpcsa_leds_pin_read(void *arg, int pin)
{
	struct mpcsa_leds_softc *sc = arg;
	pin %= MPCSA_LEDS_NPINS;
	return (sc->sc_pinstate & htobe16(1U << pin)) ? 1 : 0;
}

static void
mpcsa_leds_pin_write(void *arg, int pin, int val)
{
	struct mpcsa_leds_softc *sc = arg;
	int s;

	pin %= MPCSA_LEDS_NPINS;
	if (!sc->sc_pins[pin].pin_caps)
		return;

	s = splserial();
	if (val)
		sc->sc_pinstate |= htobe16(1U << pin);
	else
		sc->sc_pinstate &= htobe16(~(1U << pin));
	splx(s);

	if (spi_send(sc->sc_sh, 2, (const void *)&sc->sc_pinstate) != 0) {
		// @@@ do what? @@@
	}
}
	

static void
mpcsa_leds_pin_ctl(void *arg, int pin, int flags)
{
	struct mpcsa_leds_softc *sc = arg;

	pin %= MPCSA_LEDS_NPINS;
	if (!sc->sc_pins[pin].pin_caps)
		return;

	// hmm, I think there's nothing to do
}
#endif

static void mpcsa_leds_timer(void *aux)
{
	int n, s;
	struct mpcsa_leds_softc *sc = aux;
	uint16_t pins;

	callout_schedule(&sc->sc_c, mstohz(LEDS_UPDATE_INTERVAL));

	s = splserial();
	if (!(sc->sc_spi_transfer.st_flags & SPI_F_DONE)) {
		splx(s);
		return;
	}


	pins = be16toh(sc->sc_pinstate);

	for (n = 0; n < MPCSA_LEDS_NPINS; n++) {
		switch (sc->sc_leds[n].l_mode) {
		default:
			continue;

		case LMODE_COMM:
			if (sc->sc_leds[n].l_comm_cnt > 0) {
				if (sc->sc_leds[n].l_comm_cnt < INFINITE_BLINK)
					sc->sc_leds[n].l_comm_cnt--;
				else
					sc->sc_leds[n].l_comm_cnt ^= 1;
			}
			if ((sc->sc_leds[n].l_conn_cnt > 0) ^ (sc->sc_leds[n].l_comm_cnt & 1))
				pins &= ~(1U << n);
			else
				pins |= (1U << n);
			break;

		case LMODE_BLINK:
			if (--sc->sc_leds[n].l_blink_cnt <= 0) {
				pins ^= (1U << n);
				sc->sc_leds[n].l_blink_cnt = sc->sc_leds[n].l_blink_int;
			}
			break;
		}
	}

	HTOBE16(pins);
	sc->sc_pinstate = pins;
	splx(s);

	spi_transfer_init(&sc->sc_spi_transfer);
	spi_chunk_init(&sc->sc_spi_chunk, 2, (const void *)&sc->sc_pinstate, NULL);
	spi_transfer_add(&sc->sc_spi_transfer, &sc->sc_spi_chunk);
	if (spi_transfer(sc->sc_sh, &sc->sc_spi_transfer) != 0) {
		/* an error occurred! */
	}
}

void mpcsa_comm_led(int num, int count)
{
	struct mpcsa_leds_softc *sc = mpcsa_leds_sc;
	if (!sc || num < 1 || num > MPCSA_LEDS_NPINS) {
		return;
	}
	num--;
	count *= 2;
	int s = splserial();
	if (sc->sc_leds[num].l_mode != LMODE_COMM) {
		sc->sc_leds[num].l_mode = LMODE_COMM;
		sc->sc_leds[num].l_conn_cnt = 0;
		sc->sc_leds[num].l_comm_cnt = 0;
	}
	if (sc->sc_leds[num].l_comm_cnt < (count * 2 - 1))
		sc->sc_leds[num].l_comm_cnt += count;
	else if ((count * 2) < sc->sc_leds[num].l_comm_cnt)
		sc->sc_leds[num].l_comm_cnt = count * 2 - 2 - (sc->sc_leds[num].l_comm_cnt % 2);
	splx(s);
}

void mpcsa_conn_led(int num, int ok)
{
	struct mpcsa_leds_softc *sc = mpcsa_leds_sc;
	if (!sc || num < 1 || num > MPCSA_LEDS_NPINS)
		return;
	num--;
	int s = splserial();
	if (sc->sc_leds[num].l_mode != LMODE_COMM) {
		sc->sc_leds[num].l_mode = LMODE_COMM;
		sc->sc_leds[num].l_conn_cnt = 0;
		sc->sc_leds[num].l_comm_cnt = 0;
	}
	if (ok)
		sc->sc_leds[num].l_conn_cnt++;
	else
		sc->sc_leds[num].l_conn_cnt--;
	splx(s);
}

void mpcsa_blink_led(int num, int interval)
{
	struct mpcsa_leds_softc *sc = mpcsa_leds_sc;
	if (!sc || num < 1 || num > MPCSA_LEDS_NPINS) {
		return;
	}
	interval = (interval + LEDS_UPDATE_INTERVAL + 1) / LEDS_UPDATE_INTERVAL;
	num--;
	int s = splserial();
	if (sc->sc_leds[num].l_mode != LMODE_COMM) {
		sc->sc_leds[num].l_mode = LMODE_BLINK;
		sc->sc_leds[num].l_blink_cnt = interval;
	}
	sc->sc_leds[num].l_blink_int = interval;
	splx(s);
}
