/*	$NetBSD: j6x0lcd.c,v 1.9 2005/12/18 18:59:48 uwe Exp $ */

/*
 * Copyright (c) 2004, 2005 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: j6x0lcd.c,v 1.9 2005/12/18 18:59:48 uwe Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/config_hook.h>

#include <sh3/dacreg.h>
#include <hpcsh/dev/hd64461/hd64461var.h> /* XXX: for hd64461_reg_read_2 &c */
#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461gpioreg.h>

#define arraysize(ary) (sizeof(ary) / sizeof(ary[0]))


/*
 * LCD power: controlled by pin 0 in HD64461 GPIO port B.
 *   0 - power on
 *   1 - power off
 */
#define HD64461_GPBDR_J6X0_LCD_OFF	0x01

#define HD64461_GPBCR_J6X0_LCD_OFF_MASK	0xfffc
#define HD64461_GPBCR_J6X0_LCD_OFF_BITS	0x0001


/*
 * LCD brightness: controlled by DAC channel 0.  Larger channel values
 * mean dimmer.  Values smaller (i.e. brighter) then 0x5e seems to
 * result in no visible changes.
 */
#define J6X0LCD_BRIGHTNESS_DA_MAX	0x5e
#define J6X0LCD_BRIGHTNESS_DA_MIN	0xff

#define J6X0LCD_DA_TO_BRIGHTNESS(da) \
	(J6X0LCD_BRIGHTNESS_DA_MIN - (da))

#define J6X0LCD_BRIGHTNESS_TO_DA(br) \
	(J6X0LCD_BRIGHTNESS_DA_MIN - (br))

#define J6X0LCD_BRIGHTNESS_MAX \
	J6X0LCD_DA_TO_BRIGHTNESS(J6X0LCD_BRIGHTNESS_DA_MAX)

/* convenience macro to accesses DAC registers */
#define DAC_(x)    (*((volatile uint8_t *)SH7709_DA ## x))


/*
 * LCD contrast in 680 is controlled by pins 6..3 of HD64461 GPIO
 * port B.  6th pin is the least significant bit, 3rd pin is the most
 * significant.  The bits are inverted: 0 = .1111...; 1 = .0111...;
 * etc.  Larger values mean "blacker".
 *
 * The contrast value is programmed by setting bits in the data
 * register to all ones, and changing the mode of the pins in the
 * control register, setting logical "ones" to GPIO output mode (1),
 * and switching "zeroes" to input mode (3).
 */
#define HD64461_GPBDR_J680_CONTRAST_BITS	0x78	/* set */
#define HD64461_GPBCR_J680_CONTRAST_MASK	0xc03f

static const uint8_t j6x0lcd_contrast680_pins[] = { 6, 5, 4, 3 };

static const uint16_t j6x0lcd_contrast680_control_bits[] = {
	0x1540, 0x3540, 0x1d40, 0x3d40, 0x1740, 0x3740, 0x1f40, 0x3f40,
	0x15c0, 0x35c0, 0x1dc0, 0x3dc0, 0x17c0, 0x37c0, 0x1fc0, 0x3fc0
};


/*
 * LCD contrast in 620lx is controlled by pins 7,6,3,4,5 of HD64461
 * GPIO port B (in the order from the least significant to the most
 * significant).  The bits are inverted: 0 = 11111...; 5 = 01110...;
 * etc.  Larger values mean "whiter".
 *
 * The contrast value is programmed by setting bits in the data
 * register to all zeroes, and changing the mode of the pins in the
 * control register, setting logical "ones" to GPIO output mode (1),
 * and switching "zeroes" to input mode (3).
 */
#define HD64461_GPBDR_J620LX_CONTRAST_BITS	0xf8	/* clear */
#define HD64461_GPBCR_J620LX_CONTRAST_MASK	0x003f

static const uint8_t j6x0lcd_contrast620lx_pins[] = { 7, 6, 3, 4, 5 };

static const uint16_t j6x0lcd_contrast620lx_control_bits[] = {
	0xffc0, 0x7fc0, 0xdfc0, 0x5fc0, 0xff40, 0x7f40, 0xdf40, 0x5f40,
	0xfdc0, 0x7dc0, 0xddc0, 0x5dc0, 0xfd40, 0x7d40, 0xdd40, 0x5d40,
	0xf7c0, 0x77c0, 0xd7c0, 0x57c0, 0xf740, 0x7740, 0xd740, 0x5740,
	0xf5c0, 0x75c0, 0xd5c0, 0x55c0, 0xf540, 0x7540, 0xd540, 0x5540
};



struct j6x0lcd_softc {
	struct device sc_dev;
	int sc_brightness;
	int sc_contrast;

	int sc_contrast_max;
	uint16_t sc_contrast_mask;
	const uint16_t *sc_contrast_control_bits;
};

static int	j6x0lcd_match(struct device *, struct cfdata *, void *);
static void	j6x0lcd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(j6x0lcd, sizeof(struct j6x0lcd_softc),
    j6x0lcd_match, j6x0lcd_attach, NULL, NULL);


static int	j6x0lcd_param(void *, int, long, void *);
static int	j6x0lcd_power(void *, int, long, void *);

static int	j6x0lcd_contrast_raw(uint16_t, int, const uint8_t *);
static void	j6x0lcd_contrast_set(struct j6x0lcd_softc *, int);



static int
j6x0lcd_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/*
	 * XXX: platid_mask_MACH_HP_LX also matches 360LX.  It's not
	 * confirmed whether touch panel in 360LX is connected this
	 * way.  We may need to regroup platid masks.
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX)
	    && !platid_match(&platid, &platid_mask_MACH_HP_LX))
		return (0);

	if (strcmp(cfp->cf_name, "j6x0lcd") != 0)
		return (0);

	return (1);
}


static void
j6x0lcd_attach(struct device *parent, struct device *self, void *aux)
{
	struct j6x0lcd_softc *sc = (struct j6x0lcd_softc *)self;
	uint16_t bcr, bdr;
	uint8_t dcr, ddr;

	/*
	 * Brightness is controlled by DAC channel 0.
	 */
	dcr = DAC_(CR);
	dcr &= ~SH7709_DACR_DAE; /* want to control each channel separately */
	dcr |= SH7709_DACR_DAOE0; /* enable channel 0 */
	DAC_(CR) = dcr;

	ddr = DAC_(DR0);
	sc->sc_brightness = J6X0LCD_DA_TO_BRIGHTNESS(ddr);

	/*
	 * Contrast and power are controlled by HD64461 GPIO port B.
	 */
	bcr = hd64461_reg_read_2(HD64461_GPBCR_REG16);
	bdr = hd64461_reg_read_2(HD64461_GPBDR_REG16);

	/*
	 * Make sure LCD is turned on.
	 */
	bcr &= HD64461_GPBCR_J6X0_LCD_OFF_MASK;
	bcr |= HD64461_GPBCR_J6X0_LCD_OFF_BITS; /* output mode */

	bdr &= ~HD64461_GPBDR_J6X0_LCD_OFF;

	/*
	 * 620LX and 680 have different contrast control.
	 */
	if (platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX)) {
		bdr |= HD64461_GPBDR_J680_CONTRAST_BITS;

		sc->sc_contrast_mask =
			HD64461_GPBCR_J680_CONTRAST_MASK;
		sc->sc_contrast_control_bits =
			j6x0lcd_contrast680_control_bits;
		sc->sc_contrast_max =
			arraysize(j6x0lcd_contrast680_control_bits) - 1;

		sc->sc_contrast = sc->sc_contrast_max
			- j6x0lcd_contrast_raw(bcr,
				arraysize(j6x0lcd_contrast680_pins),
				j6x0lcd_contrast680_pins);
	} else {
		bdr &= ~HD64461_GPBDR_J620LX_CONTRAST_BITS;

		sc->sc_contrast_mask =
			HD64461_GPBCR_J620LX_CONTRAST_MASK;
		sc->sc_contrast_control_bits =
			j6x0lcd_contrast620lx_control_bits;
		sc->sc_contrast_max =
			arraysize(j6x0lcd_contrast620lx_control_bits) - 1;

		sc->sc_contrast =
			j6x0lcd_contrast_raw(bcr,
				arraysize(j6x0lcd_contrast620lx_pins),
				j6x0lcd_contrast620lx_pins);
	}

	hd64461_reg_write_2(HD64461_GPBCR_REG16, bcr);
	hd64461_reg_write_2(HD64461_GPBDR_REG16, bdr);

	printf(": brightness %d, contrast %d\n",
	       sc->sc_brightness, sc->sc_contrast);


	/* LCD brightness hooks */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS_MAX,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);

	/* LCD contrast hooks */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST_MAX,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_param, sc);

	/* LCD on/off hook */
	config_hook(CONFIG_HOOK_POWERCONTROL,
		    CONFIG_HOOK_POWERCONTROL_LCD,
		    CONFIG_HOOK_SHARE,
		    j6x0lcd_power, sc);
}


/*
 * Get raw contrast value programmed in GPIO port B control register.
 * Used only at attach time to get initial contrast.
 */
static int
j6x0lcd_contrast_raw(uint16_t bcr, int width, const uint8_t *pin)
{
	int contrast;
	int bit;

	contrast = 0;
	for (bit = 0; bit < width; ++bit) {
		unsigned int c, v;

		c = (bcr >> (pin[bit] << 1)) & 0x3;
		if (c == 1)	/* output mode? */
			v = 1;
		else
			v = 0;
		contrast |= (v << bit);
	}

	return contrast;
}


/*
 * Set contrast by programming GPIO port B control register.
 * Data register has been initialized at attach time.
 */
static void
j6x0lcd_contrast_set(struct j6x0lcd_softc *sc, int contrast)
{
	uint16_t bcr;

	sc->sc_contrast = contrast;

	bcr = hd64461_reg_read_2(HD64461_GPBCR_REG16);

	bcr &= sc->sc_contrast_mask;
	bcr |= sc->sc_contrast_control_bits[contrast];

	hd64461_reg_write_2(HD64461_GPBCR_REG16, bcr);
}


static int
j6x0lcd_param(void *ctx, int type, long id, void *msg)
{
	struct j6x0lcd_softc *sc = ctx;
	int value;
	uint8_t dr;

	switch (type) {
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_CONTRAST:
			*(int *)msg = sc->sc_contrast;
			return (0);

		case CONFIG_HOOK_CONTRAST_MAX:
			*(int *)msg = sc->sc_contrast_max;
			return (0);

		case CONFIG_HOOK_BRIGHTNESS:
			*(int *)msg = sc->sc_brightness;
			return (0);

		case CONFIG_HOOK_BRIGHTNESS_MAX:
			*(int *)msg = J6X0LCD_BRIGHTNESS_MAX;
			return (0);
		}
		break;

	case CONFIG_HOOK_SET:
		value = *(int *)msg;
		if (value < 0)
			value = 0;

		switch (id) {
		case CONFIG_HOOK_CONTRAST:
			if (value > sc->sc_contrast_max)
				value = sc->sc_contrast_max;
			j6x0lcd_contrast_set(sc, value);
			return (0);

		case CONFIG_HOOK_BRIGHTNESS:
			if (value > J6X0LCD_BRIGHTNESS_MAX)
				value = J6X0LCD_BRIGHTNESS_MAX;
			sc->sc_brightness = value;

			dr = J6X0LCD_BRIGHTNESS_TO_DA(value);
			DAC_(DR0) = dr;
			return (0);
		}
		break;
	}

	return (EINVAL);
}


static int
j6x0lcd_power(void *ctx, int type, long id, void *msg)
{
	int on;
	uint16_t r;

	if (type != CONFIG_HOOK_POWERCONTROL
	    || id != CONFIG_HOOK_POWERCONTROL_LCD)
		return (EINVAL);

	on = (int)msg;

	r = hd64461_reg_read_2(HD64461_GPBDR_REG16);
	if (on)
		r &= ~HD64461_GPBDR_J6X0_LCD_OFF;
	else
		r |= HD64461_GPBDR_J6X0_LCD_OFF;
	hd64461_reg_write_2(HD64461_GPBDR_REG16, r);

	return (0);
}
