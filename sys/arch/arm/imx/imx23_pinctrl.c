/* $Id: imx23_pinctrl.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $ */

/*
* Copyright (c) 2013 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/imx/imx23_pinctrlreg.h>
#include <arm/imx/imx23_pinctrlvar.h>
#include <arm/imx/imx23var.h>

#define GPIO_PINS 96

typedef struct pinctrl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
	struct gpio_chipset_tag gc;
	gpio_pin_t pins[GPIO_PINS];
} *pinctrl_softc_t;

static int	pinctrl_match(device_t, cfdata_t, void *);
static void	pinctrl_attach(device_t, device_t, void *);
static int	pinctrl_activate(device_t, enum devact);

#if notyet
static void     pinctrl_reset(struct pinctrl_softc *);
#endif
static void     pinctrl_init(struct pinctrl_softc *);

static	int	pinctrl_gp_gc_open(void *, device_t);
static	void	pinctrl_gp_gc_close(void *, device_t);
static	int	pinctrl_gp_pin_read(void *, int);
static	void	pinctrl_gp_pin_write(void *, int, int);
static	void	pinctrl_gp_pin_ctl(void *, int, int);

static pinctrl_softc_t _sc = NULL;

CFATTACH_DECL3_NEW(pinctrl,
        sizeof(struct pinctrl_softc),
        pinctrl_match,
        pinctrl_attach,
        NULL,
        pinctrl_activate,
        NULL,
        NULL,
        0
);

#define GPIO_PIN_CAP (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_INOUT | \
		GPIO_PIN_PULLUP | GPIO_PIN_SET)

/*
 * Supported capabilities for each GPIO pin.
 */
const static int pin_caps[GPIO_PINS] = {
	/*
	 * HW_PINCTRL_MUXSEL0
	 */
	/* PIN 0 */
	GPIO_PIN_CAP,
	/* PIN 1 */
	GPIO_PIN_CAP,
	/* PIN 2 */
	GPIO_PIN_CAP,
	/* PIN 3 */
	GPIO_PIN_CAP,
	/* PIN 4 */
	GPIO_PIN_CAP,
	/* PIN 5 */
	GPIO_PIN_CAP,
	/* PIN 6 */
	GPIO_PIN_CAP,
	/* PIN 7 */
	GPIO_PIN_CAP,
	/* PIN 8 */
	GPIO_PIN_CAP,
	/* PIN 9 */
	GPIO_PIN_CAP,
	/* PIN 10 */
	GPIO_PIN_CAP,
	/* PIN 11 */
	GPIO_PIN_CAP,
	/* PIN 12 */
	GPIO_PIN_CAP,
	/* PIN 13 */
	GPIO_PIN_CAP,
	/* PIN 14 */
	GPIO_PIN_CAP,
	/* PIN 15 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL1
	 */
	/* PIN 16 */
	GPIO_PIN_CAP,
	/* PIN 17 */
	0,		/* Reserved for powering OLinuXino MAXI/MINI USB hub. */
	/* PIN 18 */
	GPIO_PIN_CAP,
	/* PIN 19 */
	GPIO_PIN_CAP,
	/* PIN 20 */
	GPIO_PIN_CAP,
	/* PIN 21 */
	GPIO_PIN_CAP,
	/* PIN 22 */
	GPIO_PIN_CAP,
	/* PIN 23 */
	GPIO_PIN_CAP,
	/* PIN 24 */
	GPIO_PIN_CAP,
	/* PIN 25 */
	GPIO_PIN_CAP,
	/* PIN 26 */
	GPIO_PIN_CAP,
	/* PIN 27 */
	GPIO_PIN_CAP,
	/* PIN 28 */
	GPIO_PIN_CAP,
	/* PIN 29 */
	GPIO_PIN_CAP,
	/* PIN 30 */
	GPIO_PIN_CAP,
	/* PIN 31 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL2
	 */
	/* PIN 32 */
	GPIO_PIN_CAP,
	/* PIN 33 */
	GPIO_PIN_CAP,
	/* PIN 34 */
	GPIO_PIN_CAP,
	/* PIN 35 */
	GPIO_PIN_CAP,
	/* PIN 36 */
	GPIO_PIN_CAP,
	/* PIN 37 */
	GPIO_PIN_CAP,
	/* PIN 38 */
	GPIO_PIN_CAP,
	/* PIN 39 */
	GPIO_PIN_CAP,
	/* PIN 40 */
	GPIO_PIN_CAP,
	/* PIN 41 */
	GPIO_PIN_CAP,
	/* PIN 42 */
	GPIO_PIN_CAP,
	/* PIN 43 */
	GPIO_PIN_CAP,
	/* PIN 44 */
	GPIO_PIN_CAP,
	/* PIN 45 */
	GPIO_PIN_CAP,
	/* PIN 46 */
	GPIO_PIN_CAP,
	/* PIN 47 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL3
	 */
	/* PIN 48 */
	GPIO_PIN_CAP,
	/* PIN 49 */
	GPIO_PIN_CAP,
	/* PIN 50 */
	GPIO_PIN_CAP,
	/* PIN 51 */
	GPIO_PIN_CAP,
	/* PIN 52 */
	GPIO_PIN_CAP,
	/* PIN 53 */
	GPIO_PIN_CAP,
	/* PIN 54 */
	GPIO_PIN_CAP,
	/* PIN 55 */
	GPIO_PIN_CAP,
	/* PIN 56 */
	GPIO_PIN_CAP,
	/* PIN 57 */
	GPIO_PIN_CAP,
	/* PIN 58 */
	GPIO_PIN_CAP,
	/* PIN 59 */
	GPIO_PIN_CAP,
	/* PIN 60 */
	GPIO_PIN_CAP,
	/* PIN 61 */
	GPIO_PIN_CAP,
	/* PIN 62 */
	GPIO_PIN_CAP,
	/* PIN 63 */
	0,		/* Reserved. */
	/*
	 * HW_PINCTRL_MUXSEL4
	 */
	/* PIN 64 */
	GPIO_PIN_CAP,
	/* PIN 65 */
	GPIO_PIN_CAP,
	/* PIN 66 */
	GPIO_PIN_CAP,
	/* PIN 67 */
	GPIO_PIN_CAP,
	/* PIN 68 */
	GPIO_PIN_CAP,
	/* PIN 69 */
	GPIO_PIN_CAP,
	/* PIN 70 */
	GPIO_PIN_CAP,
	/* PIN 71 */
	GPIO_PIN_CAP,
	/* PIN 72 */
	GPIO_PIN_CAP,
	/* PIN 73 */
	0,		/* From this on reserved for EMI (DRAM) pins. */
	/* PIN 74 */
	0,
	/* PIN 75 */
	0,
	/* PIN 76 */
	0,
	/* PIN 77 */
	0,
	/* PIN 78 */
	0,
	/* PIN 79 */
	0,
	/*
	 * HW_PINCTRL_MUXSEL5
	 */
	/* PIN 80 */
	0,
	/* PIN 81 */
	0,
	/* PIN 82 */
	0,
	/* PIN 83 */
	0,
	/* PIN 84 */
	0,
	/* PIN 85 */
	0,
	/* PIN 86 */
	0,
	/* PIN 87 */
	0,
	/* PIN 88 */
	0,
	/* PIN 89 */
	0,
	/* PIN 90 */
	0,
	/* PIN 91 */
	0,
	/* PIN 92 */
	0,
	/* PIN 93 */
	0,
	/* PIN 94 */
	0,
	/* PIN 95 */
	0
};

#define PINCTRL_RD(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define PINCTRL_WR(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

/*
 * Macros to map pin numbers to registers and bit fields.
 */
#define MUXSEL_REG_SIZE	0x10
#define PIN2MUXSEL_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0)
#define PIN2MUXSEL_SET_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0_SET)
#define PIN2MUXSEL_CLR_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0_CLR)
#define PIN2MUXSEL_MASK(pin)	(3<<(pin % 16 * 2))

#define DRIVE_REG_SIZE	0x10
#define PIN2DRIVE_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0)
#define PIN2DRIVE_SET_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0_SET)
#define PIN2DRIVE_CLR_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0_CLR)
#define PIN2DRIVE_MASK(pin)	(3<<(pin % 8 * 4))

#define PULL_REG_SIZE	0x10
#define PIN2PULL_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0)
#define PIN2PULL_SET_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0_SET)
#define PIN2PULL_CLR_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0_CLR)
#define PIN2PULL_MASK(pin)	(1<<(pin % 32))

#define DOUT_REG_SIZE	0x10
#define PIN2DOUT_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0)
#define PIN2DOUT_SET_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0_SET)
#define PIN2DOUT_CLR_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0_CLR)
#define PIN2DOUT_MASK(pin)	(1<<(pin % 32))

#define DIN_REG_SIZE	0x10
#define PIN2DIN_REG(pin)	((pin / 32) * DIN_REG_SIZE + HW_PINCTRL_DIN0)
#define PIN2DIN_MASK(pin)	(1<<(pin % 32))

#define DOE_REG_SIZE	0x10
#define PIN2DOE_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0)
#define PIN2DOE_SET_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0_SET)
#define PIN2DOE_CLR_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0_CLR)
#define PIN2DOE_MASK(pin)	(1<<(pin % 32))

#define DRIVE_STRENGTH_4MA	0
#define DRIVE_STRENGTH_8MA	1
#define DRIVE_STRENGTH_12MA	2

#define MUXEL_GPIO_MODE	3

#define PINCTRL_SOFT_RST_LOOP 455 /* At least 1 us ... */

static int
pinctrl_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_PINCTRL_BASE) &&
	    (aa->aa_size == HW_PINCTRL_SIZE))
		return 1;

	return 0;
}

static void
pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct pinctrl_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int pinctrl_attached = 0;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	
	if (pinctrl_attached) {
		aprint_error_dev(sc->sc_dev, "already attached\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl))
	{
		aprint_error_dev(sc->sc_dev, "Unable to map bus space\n");
		return;
	}

#if notyet
	pinctrl_reset(sc);
#endif

	pinctrl_init(sc);

	aprint_normal(": PIN MUX & GPIO\n");

	/* Set pin capabilities. */
	int i;
	for(i = 0; i < GPIO_PINS; i++) {
		sc->pins[i].pin_caps = pin_caps[i];
	}

	pinctrl_attached = 1;

	sc->gc.gp_cookie = sc;
	sc->gc.gp_gc_open = pinctrl_gp_gc_open;
	sc->gc.gp_gc_close = pinctrl_gp_gc_close;
	sc->gc.gp_pin_read = pinctrl_gp_pin_read;
	sc->gc.gp_pin_write = pinctrl_gp_pin_write;
	sc->gc.gp_pin_ctl = pinctrl_gp_pin_ctl;

	struct gpiobus_attach_args gpiobus_aa;
	gpiobus_aa.gba_gc = &sc->gc;
	gpiobus_aa.gba_npins = GPIO_PINS;
	gpiobus_aa.gba_pins = sc->pins;

	config_found_ia(self, "gpiobus", &gpiobus_aa, gpiobus_print);
	
	return;
}

static int
pinctrl_activate(device_t self, enum devact act)
{

	return EOPNOTSUPP;
}

static void    
pinctrl_init(struct pinctrl_softc *sc)
{
	_sc = sc;
	return;
}

#if notyet
/*
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
pinctrl_reset(struct pinctrl_softc *sc)
{
        unsigned int loop;

        /* Prepare for soft-reset by making sure that SFTRST is not currently
         * asserted. Also clear CLKGATE so we can wait for its assertion below.
         */
        PINCTRL_WR(sc, HW_PINCTRL_CTRL_CLR, HW_PINCTRL_CTRL_SFTRST);

        /* Wait at least a microsecond for SFTRST to deassert. */
        loop = 0;
        while ((PINCTRL_RD(sc, HW_PINCTRL_CTRL) & HW_PINCTRL_CTRL_SFTRST) ||
            (loop < PINCTRL_SOFT_RST_LOOP))
                loop++;

        /* Clear CLKGATE so we can wait for its assertion below. */
        PINCTRL_WR(sc, HW_PINCTRL_CTRL_CLR, HW_PINCTRL_CTRL_CLKGATE);

        /* Soft-reset the block. */
        PINCTRL_WR(sc, HW_PINCTRL_CTRL_SET, HW_PINCTRL_CTRL_SFTRST);

        /* Wait until clock is in the gated state. */
        while (!(PINCTRL_RD(sc, HW_PINCTRL_CTRL) & HW_PINCTRL_CTRL_CLKGATE));

        /* Bring block out of reset. */
        PINCTRL_WR(sc, HW_PINCTRL_CTRL_CLR, HW_PINCTRL_CTRL_SFTRST);

        loop = 0;
        while ((PINCTRL_RD(sc, HW_PINCTRL_CTRL) & HW_PINCTRL_CTRL_SFTRST) ||
            (loop < PINCTRL_SOFT_RST_LOOP))
                loop++;

        PINCTRL_WR(sc, HW_PINCTRL_CTRL_CLR, HW_PINCTRL_CTRL_CLKGATE);

        /* Wait until clock is in the NON-gated state. */
        while (PINCTRL_RD(sc, HW_PINCTRL_CTRL) & HW_PINCTRL_CTRL_CLKGATE);

        return;
}
#endif

/*
 * Enable external USB transceiver/HUB.
 *
 * PIN18/LCD_D17/USB_EN controls reset line of external USB chip on MINI and
 * MAXI boards. We configure this pin to logic 1.
 */
void
pinctrl_en_usb(void)    
{
	struct pinctrl_softc *sc = _sc;

        if (sc == NULL) {
                aprint_error("pinctrl is not initalized");
                return;
        }

	pinctrl_gp_pin_ctl(sc, 17, GPIO_PIN_OUTPUT);
	delay(1000);
	pinctrl_gp_pin_write(sc, 17, 1);

	return;
}

static	int
pinctrl_gp_gc_open(void *cookie, device_t dev)
{
	return 0;
}

static	void
pinctrl_gp_gc_close(void *cookie, device_t dev)
{
	return;
}

static	int
pinctrl_gp_pin_read(void *cookie, int pin)
{
	int value;
	pinctrl_softc_t sc = (pinctrl_softc_t) cookie;

	if (PINCTRL_RD(sc, PIN2DIN_REG(pin)) & PIN2DIN_MASK(pin))
		value = 1;
	else
		value = 0;
	
	return value;
}

static	void
pinctrl_gp_pin_write(void *cookie, int pin, int value)
{
	pinctrl_softc_t sc = (pinctrl_softc_t) cookie;

	if (value)
		PINCTRL_WR(sc, PIN2DOUT_SET_REG(pin), PIN2DOUT_MASK(pin));
	else
		PINCTRL_WR(sc, PIN2DOUT_CLR_REG(pin), PIN2DOUT_MASK(pin));

	return;
}

/*
 * Configure pin as requested in flags.
 */
static	void
pinctrl_gp_pin_ctl(void *cookie, int pin, int flags)
{
	pinctrl_softc_t sc = (pinctrl_softc_t) cookie;
	uint32_t tmpr;

	/* Enable GPIO pin. */
	tmpr = PINCTRL_RD(sc, PIN2MUXSEL_REG(pin));
	tmpr &= ~PIN2MUXSEL_MASK(pin);
	tmpr |= __SHIFTIN(MUXEL_GPIO_MODE, PIN2MUXSEL_MASK(pin));
	PINCTRL_WR(sc, PIN2MUXSEL_REG(pin), tmpr);

	/* Configure pin drive strength. */
	tmpr = PINCTRL_RD(sc, PIN2DRIVE_REG(pin));
	tmpr &= ~PIN2DRIVE_MASK(pin);
	tmpr |= __SHIFTIN(DRIVE_STRENGTH_4MA, PIN2DRIVE_MASK(pin));
	PINCTRL_WR(sc, PIN2DRIVE_REG(pin), tmpr);

	if ((flags & (GPIO_PIN_OUTPUT | GPIO_PIN_INOUT))) {
		/* Configure pullup resistor or gate keeper. */
		if (flags & GPIO_PIN_PULLUP)
			PINCTRL_WR(sc, PIN2PULL_SET_REG(pin),
				PIN2PULL_MASK(pin));
		else
			PINCTRL_WR(sc, PIN2PULL_CLR_REG(pin),
				PIN2PULL_MASK(pin));

		/* Set initial pin value to logic zero. */
		PINCTRL_WR(sc, PIN2DOUT_CLR_REG(pin), PIN2DOUT_MASK(pin));

		/* Enable pin output. */
		PINCTRL_WR(sc, PIN2DOE_SET_REG(pin), PIN2DOE_MASK(pin));
	}

	if (flags & GPIO_PIN_INPUT) {
		/* Disable pin output. */
		PINCTRL_WR(sc, PIN2DOE_CLR_REG(pin), PIN2DOE_MASK(pin));

		/* Configure pullup resistor or gate keeper. */
		if (flags & GPIO_PIN_PULLUP)
			PINCTRL_WR(sc, PIN2PULL_SET_REG(pin),
				PIN2PULL_MASK(pin));
		else
			PINCTRL_WR(sc, PIN2PULL_CLR_REG(pin),
				PIN2PULL_MASK(pin));
	}

	return;
}
