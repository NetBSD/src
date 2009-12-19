/*	$NetBSD: psh3lcd.c,v 1.5 2009/12/19 07:08:55 kiyohara Exp $	*/
/*
 * Copyright (c) 2005 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/config_hook.h>

#include <sh3/pfcreg.h>
#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461gpioreg.h>


/*
 * LCD contrast INC#: controlled by pin 0 in HD64461 GPIO port A.
 * LCD contrast  CS#: controlled by pin 1 in HD64461 GPIO port A.
 * LCD contrast U/D#: controlled by pin 0 in SH7709 GPIO port D.
 *   0 - down
 *   1 - up
 */
#define PSH3LCD_CONTRAST_INC		0x01
#define PSH3LCD_CONTRAST_CS		0x02
#define PSH3LCD_CONTRAST_UD		0x01

/*
 * LCD brightness: controled by HG71C105FE at AREA2.
 *	XXXX: That is custom IC. We don't know spec.  X-<
 */
#define PSH3LCD_BRIGHTNESS_REG0		0xaa000072
#define PSH3LCD_BRIGHTNESS_REG1		0xaa000150
#define PSH3LCD_BRIGHTNESS_REG2		0xaa000152

/* brightness control data */
static const struct psh3lcd_x0_bcd {	/* 50PA, 30PA */
	uint8_t reg0;
	uint8_t reg1;
	uint8_t reg2;
} psh3lcd_x0_bcd[] = {
	{ 0x05, 0x08, 0x1e },
	{ 0x05, 0x1d, 0x09 },
	{ 0x04, 0x1e, 0x1e },
	{ 0x06, 0x1e, 0x1e },
	{ 0x07, 0x1e, 0x1e },
	{ 0x00, 0x00, 0x00 }
};
static const struct psh3lcd_xx0_bcd {	/* 200JC */
	uint8_t reg1;
	uint8_t reg2;
} psh3lcd_xx0_bcd[] = {
	{ 0x33, 0x0c },
	{ 0x2d, 0x12 },
	{ 0x26, 0x19 },
	{ 0x20, 0x1f },
	{ 0x1a, 0x25 },
	{ 0x13, 0x2c },
	{ 0x0d, 0x32 },
	{ 0x07, 0x38 },
	{ 0x01, 0x00 },
	{ 0x00, 0x00 }
};

#define PSH3LCD_XX0_BRIGHTNESS_MAX	8
#define PSH3LCD_X0_BRIGHTNESS_MAX	4

#define PSH3LCD_CONTRAST_MAX		2	/* XXX */
#define PSH3LCD_CONTRAST		1
#define PSH3LCD_CONTRAST_UP		2
#define PSH3LCD_CONTRAST_DOWN		0

#define BCD_NO_MATCH	(-1)


struct psh3lcd_softc {
	device_t sc_dev;
	int sc_brightness;
	int sc_brightness_max;
	void (*sc_set_brightness)(int);
};

static int psh3lcd_match(device_t, struct cfdata *, void *);
static void psh3lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(psh3lcd, sizeof(struct psh3lcd_softc),
    psh3lcd_match, psh3lcd_attach, NULL, NULL);


static inline int psh3lcd_x0_bcd_get(void);
static inline int psh3lcd_xx0_bcd_get(void);
static void psh3lcd_x0_set_brightness(int);
static void psh3lcd_xx0_set_brightness(int);
static void psh3lcd_set_contrast(int);
static int psh3lcd_param(void *, int, long, void *);
static int psh3lcd_power(void *, int, long, void *);


static inline int
psh3lcd_x0_bcd_get()
{
	int i;
	uint8_t bcr0, bcr1, bcr2;

	bcr0 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG0);
	bcr1 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG1);
	bcr2 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG2);

	for (i = 0; psh3lcd_x0_bcd[i].reg0 != 0; i++)
		if (bcr0 == psh3lcd_x0_bcd[i].reg0 &&
		    bcr1 == psh3lcd_x0_bcd[i].reg1 &&
		    bcr2 == psh3lcd_x0_bcd[i].reg2) 
			break;
	if (psh3lcd_x0_bcd[i].reg0 == 0)
		return BCD_NO_MATCH;
	return i;
}

static inline int
psh3lcd_xx0_bcd_get()
{
	int i;
	uint8_t bcr1, bcr2;

	bcr1 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG1);
	bcr2 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG2);

	for (i = 0; psh3lcd_xx0_bcd[i].reg1 != 0; i++)
		if (bcr1 == psh3lcd_xx0_bcd[i].reg1 &&
		    bcr2 == psh3lcd_xx0_bcd[i].reg2)
			break;
	if (psh3lcd_xx0_bcd[i].reg1 == 0)
		return BCD_NO_MATCH;
	return i;
}

static void
psh3lcd_xx0_set_brightness(int index)
{

	_reg_write_1(PSH3LCD_BRIGHTNESS_REG1, psh3lcd_xx0_bcd[index].reg1);
	_reg_write_1(PSH3LCD_BRIGHTNESS_REG2, psh3lcd_xx0_bcd[index].reg2);
}

static void
psh3lcd_x0_set_brightness(int index)
{

	_reg_write_1(PSH3LCD_BRIGHTNESS_REG0, psh3lcd_x0_bcd[index].reg0);
	_reg_write_1(PSH3LCD_BRIGHTNESS_REG1, psh3lcd_x0_bcd[index].reg1);
	_reg_write_1(PSH3LCD_BRIGHTNESS_REG2, psh3lcd_x0_bcd[index].reg2);
}

/*
 * contrast control function.  controlled by IC (X9313W).
 */
inline void
psh3lcd_set_contrast(int value)
{
	uint16_t gpadr;
	uint8_t pddr;

	/* CS assert */
	gpadr = hd64461_reg_read_2(HD64461_GPADR_REG16);
	gpadr &= ~PSH3LCD_CONTRAST_CS;
	hd64461_reg_write_2(HD64461_GPADR_REG16, gpadr);
	delay(1); 

	/* set U/D# */
	pddr = _reg_read_1(SH7709_PDDR);
	if (value == PSH3LCD_CONTRAST_UP)
		pddr |= PSH3LCD_CONTRAST_UD;
	else if (value == PSH3LCD_CONTRAST_DOWN)
		pddr &= ~PSH3LCD_CONTRAST_UD;
	_reg_write_1(SH7709_PDDR, pddr);
	delay(3);

	/* INCrement */
	hd64461_reg_write_2(HD64461_GPADR_REG16, gpadr & ~PSH3LCD_CONTRAST_INC);
	delay(1); 
	hd64461_reg_write_2(HD64461_GPADR_REG16, gpadr);
	delay(1); 

	/* CS deassert */
	gpadr |= PSH3LCD_CONTRAST_CS;
	hd64461_reg_write_2(HD64461_GPADR_REG16, gpadr);
	delay(1); 
}

/* ARGSUSED */
static int
psh3lcd_match(device_t parent __unused, struct cfdata *cfp, void *aux __unused)
{
	uint8_t bcr0;

	if (!platid_match(&platid, &platid_mask_MACH_HITACHI_PERSONA))
		return 0;

	if (strcmp(cfp->cf_name, "psh3lcd") != 0)
		return 0;

	bcr0 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG0);
	if (bcr0 == 0) {
		if (psh3lcd_xx0_bcd_get() == BCD_NO_MATCH)
			return 0;
	} else {
		if (psh3lcd_x0_bcd_get() == BCD_NO_MATCH)
			return 0;
	}

	return 1;
}

/* ARGSUSED */
static void
psh3lcd_attach(device_t parent __unused, device_t self, void *aux __unused)
{
	struct psh3lcd_softc *sc = device_private(self);
	uint8_t bcr0, bcr1, bcr2;

	sc->sc_dev = self;

	bcr0 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG0);
	bcr1 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG1);
	bcr2 = _reg_read_1(PSH3LCD_BRIGHTNESS_REG2);
	if (bcr0 == 0) {
		sc->sc_set_brightness = psh3lcd_xx0_set_brightness;
		sc->sc_brightness = psh3lcd_xx0_bcd_get();
		sc->sc_brightness_max = PSH3LCD_XX0_BRIGHTNESS_MAX;
	} else {
		sc->sc_set_brightness = psh3lcd_x0_set_brightness;
		sc->sc_brightness = psh3lcd_x0_bcd_get();
		sc->sc_brightness_max = PSH3LCD_X0_BRIGHTNESS_MAX;
	}
	aprint_naive("\n");
	aprint_normal(": brightness %d\n", sc->sc_brightness);

	/* LCD contrast hooks */
        config_hook(CONFIG_HOOK_GET,
	    CONFIG_HOOK_CONTRAST_MAX, CONFIG_HOOK_SHARE, psh3lcd_param, sc);
        config_hook(CONFIG_HOOK_GET,
	    CONFIG_HOOK_CONTRAST, CONFIG_HOOK_SHARE, psh3lcd_param, sc);
        config_hook(CONFIG_HOOK_SET,
	    CONFIG_HOOK_CONTRAST, CONFIG_HOOK_SHARE, psh3lcd_param, sc);

	/* LCD brightness hooks */
        config_hook(CONFIG_HOOK_GET,
	    CONFIG_HOOK_BRIGHTNESS_MAX, CONFIG_HOOK_SHARE, psh3lcd_param, sc);
        config_hook(CONFIG_HOOK_GET,
	    CONFIG_HOOK_BRIGHTNESS, CONFIG_HOOK_SHARE, psh3lcd_param, sc);
        config_hook(CONFIG_HOOK_SET,
	    CONFIG_HOOK_BRIGHTNESS, CONFIG_HOOK_SHARE, psh3lcd_param, sc);

	/* LCD on/off hook */
	config_hook(CONFIG_HOOK_POWERCONTROL,
	    CONFIG_HOOK_POWERCONTROL_LCD, CONFIG_HOOK_SHARE, psh3lcd_power, sc);

 	/* XXX: TODO: don't rely on CONFIG_HOOK_POWERCONTROL_LCD */
 	if (!pmf_device_register(self, NULL, NULL))
 		aprint_error_dev(self, "unable to establish power handler\n");
}


static int
psh3lcd_param(void *ctx, int type, long id, void *msg)
{
	struct psh3lcd_softc *sc = ctx;
	int value;

	switch (type) {
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_CONTRAST:
			*(int *)msg = PSH3LCD_CONTRAST;
			return 0;

		case CONFIG_HOOK_CONTRAST_MAX:
			*(int *)msg = PSH3LCD_CONTRAST_MAX;
			return 0;

		case CONFIG_HOOK_BRIGHTNESS:
			*(int *)msg = sc->sc_brightness;
			return 0;

		case CONFIG_HOOK_BRIGHTNESS_MAX:
			*(int *)msg = sc->sc_brightness_max;
			return 0;
		}
		break;

	case CONFIG_HOOK_SET:
		value = *(int *)msg;

		switch (id) {
		case CONFIG_HOOK_CONTRAST:
			if (value != PSH3LCD_CONTRAST_UP &&
			    value != PSH3LCD_CONTRAST_DOWN)
				return EINVAL;
			psh3lcd_set_contrast(value);
			return 0;

		case CONFIG_HOOK_BRIGHTNESS:
			if (value < 0)
				value = 0;
			if (value > sc->sc_brightness_max)
				value = sc->sc_brightness_max;
			sc->sc_brightness = value;
			sc->sc_set_brightness(sc->sc_brightness);
			return 0;
		}
		break;
	}

	return EINVAL;
}


static int
psh3lcd_power(void *ctx, int type, long id, void *msg)
{
	struct psh3lcd_softc *sc = ctx;
	int on;

	if (type != CONFIG_HOOK_POWERCONTROL ||
	    id != CONFIG_HOOK_POWERCONTROL_LCD)
		return EINVAL;

	on = (int)msg;
	if (on)
		sc->sc_set_brightness(sc->sc_brightness);
	else
		sc->sc_set_brightness(sc->sc_brightness_max + 1);

	return 0;
}
