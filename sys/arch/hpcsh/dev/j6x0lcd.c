/*	$NetBSD: j6x0lcd.c,v 1.3 2004/04/04 17:49:38 uwe Exp $ */

/*
 * Copyright (c) 2004 Valeriy E. Ushakov
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
__KERNEL_RCSID(0, "$NetBSD: j6x0lcd.c,v 1.3 2004/04/04 17:49:38 uwe Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>
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


/*
 * LCD power: controlled by pin 0 in HD64461 GPIO port B.
 *   0 - power on
 *   1 - power off
 */
#define HD64461_GPBDR_J6X0LCD_OFF	0x01

#define HD64461_GPBCR_J6X0LCD_OFF_MASK	0xfffc
#define HD64461_GPBCR_J6X0LCD_OFF_BITS	0x0001


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
 * LCD contrast: controlled by pins 6,5,4,3 HD64461 GPIO port B.
 * 6th is the least significant bit, 3rd is the most significant.
 * The bits are inverted: .1111... = 0, .0111... = 1, etc.
 *
 * We control the contrast value by setting bits in the data register
 * to all ones, and changing the mode of the bits in the control
 * register, keeping "ones" in gpio output mode (1), and switching
 * "zeros" to input mode (3).  This is what WinCE also does.
 *
 * Alternative method is to set the mode of all bits to gpio output
 * mode and then change the bits in the data register.  This method
 * results in significantly less contrast screen for the same values.
 * E.g., WinCE default contrast value is 11 (in our numbering, 23 in
 * WinCE's) - which is fine in the "tweak the control register"
 * method, but is very blurry in the "tweak the data register" method.
 * Subjectively, 15 in the "tweak control" method looks like 7 in the
 * "tweak data" method.
 *
 * May be it's possible to control a wider range of contrast by
 * combining the two methods, but values above 7 in the "tweak data"
 * method are so blurry that they are next to unusable in practice.
 */
#define J6X0LCD_CONTRAST_MAX	15

#define HD64461_GPBDR_J6X0LCD_CONTRAST_MASK	0x87
#define HD64461_GPBDR_J6X0LCD_CONTRAST_BITS	0x78

#if 0
/* "tweak data" */
static uint8_t j6x0lcd_contrast_data_bits[] = {
	0x78, 0x38, 0x58, 0x18, 0x68, 0x28, 0x48, 0x08,
	0x70, 0x30, 0x50, 0x10, 0x60, 0x20, 0x40, 0x00
};
#endif

#define HD64461_GPBCR_J6X0LCD_CONTRAST_MASK	0xc03f
#define HD64461_GPBCR_J6X0LCD_CONTRAST_BITS	0x1540

/* "tweak control" */
static uint16_t j6x0lcd_contrast_control_bits[] = {
	0x1540, 0x3540, 0x1d40, 0x3d40, 0x1740, 0x3740, 0x1f40, 0x3f40,
	0x15c0, 0x35c0, 0x1dc0, 0x3dc0, 0x17c0, 0x37c0, 0x1fc0, 0x3fc0
};


struct j6x0lcd_softc {
	struct device sc_dev;
	int sc_brightness;
	int sc_contrast;
};

static int	j6x0lcd_match(struct device *, struct cfdata *, void *);
static void	j6x0lcd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(j6x0lcd, sizeof(struct j6x0lcd_softc),
    j6x0lcd_match, j6x0lcd_attach, NULL, NULL);


static int	j6x0lcd_param(void *, int, long, void *);
static int	j6x0lcd_power(void *, int, long, void *);


static int
j6x0lcd_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/*
	 * XXX: does platid_mask_MACH_HP_LX matches _JORNADA_6XX too?
	 * Is 620 wired similarly?
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX))
		return (0);

	if (strcmp(cfp->cf_name, "j6x0lcd") != 0)
		return (0);

	return (1);
}


static void
j6x0lcd_attach(struct device *parent, struct device *self, void *aux)
{
	struct j6x0lcd_softc *sc = (struct j6x0lcd_softc *)self;
	int contrast, i;
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

	contrast = 0xf;		/* bits are inverted */
	for (i = 0; i < 4; ++i) {
		unsigned int c, v;
		c = (bcr >> ((6 - i) << 1)) & 0x3;
		if (c == 1)	/* gpio mode? */
			v = 1;
		else
			v = 0;
		contrast &= ~(v << i);
	}

	sc->sc_contrast = contrast;

	bdr &= ~HD64461_GPBDR_J6X0LCD_OFF;
	bdr |= HD64461_GPBDR_J6X0LCD_CONTRAST_BITS;
	hd64461_reg_write_2(HD64461_GPBDR_REG16, bdr);

	bcr &= HD64461_GPBCR_J6X0LCD_OFF_MASK
		& HD64461_GPBCR_J6X0LCD_CONTRAST_MASK;
	bcr |= HD64461_GPBCR_J6X0LCD_OFF_BITS
		| j6x0lcd_contrast_control_bits[contrast];
	hd64461_reg_write_2(HD64461_GPBCR_REG16, bcr);

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


static int
j6x0lcd_param(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct j6x0lcd_softc *sc = ctx;
	int value;
	uint16_t bcr;
	uint8_t dr;

	switch (type) {
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_CONTRAST:
			*(int *)msg = sc->sc_contrast;
			return (0);

		case CONFIG_HOOK_CONTRAST_MAX:
			*(int *)msg = J6X0LCD_CONTRAST_MAX;
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
			if (value > J6X0LCD_CONTRAST_MAX)
				value = J6X0LCD_CONTRAST_MAX;
			sc->sc_contrast = value;

			bcr = hd64461_reg_read_2(HD64461_GPBCR_REG16);
			bcr &= HD64461_GPBCR_J6X0LCD_CONTRAST_MASK;
			bcr |= j6x0lcd_contrast_control_bits[value];
			hd64461_reg_write_2(HD64461_GPBCR_REG16, bcr);
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
j6x0lcd_power(ctx, type, id, msg)
	void *ctx; 
	int type; 
	long id; 
	void *msg;
{
	int on;
	uint16_t r;

	if (type != CONFIG_HOOK_POWERCONTROL
	    || id != CONFIG_HOOK_POWERCONTROL_LCD)
		return (EINVAL);

	on = (int)msg;

	r = hd64461_reg_read_2(HD64461_GPBDR_REG16);
	if (on)
		r &= ~HD64461_GPBDR_J6X0LCD_OFF;
	else
		r |= HD64461_GPBDR_J6X0LCD_OFF;
	hd64461_reg_write_2(HD64461_GPBDR_REG16, r);

	return (0);
}
