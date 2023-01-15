/* $NetBSD: lcd.c,v 1.13 2023/01/15 05:08:33 tsutsui Exp $ */
/* $OpenBSD: lcd.c,v 1.7 2015/02/10 22:42:35 miod Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>		/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: lcd.c,v 1.13 2023/01/15 05:08:33 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

#include <machine/autoconf.h>
#include <machine/board.h>
#include <machine/cpu.h>
#include <machine/lcd.h>

#include "ioconf.h"

#define PIO1_MODE_OUTPUT	0x84
#define PIO1_MODE_INPUT		0x94

#define POWER	0x10

#define ENABLE	0x80
#define DISABLE	0x00

#define WRITE_CMD	(0x00 | 0x00)
#define WRITE_DATA	(0x00 | 0x40)
#define READ_BUSY	(0x20 | 0x00)
#define READ_DATA	(0x20 | 0x40)

#define LCD_INIT	0x38
#define LCD_ENTRY	0x06
#define LCD_ON		0x0c
#define LCD_CLS		0x01
#define LCD_HOME	0x02
#define LCD_LOCATE(X, Y)	(((Y) & 1 ? 0xc0 : 0x80) | ((X) & 0x0f))

#define LCD_MAXBUFLEN	80

struct pio {
	volatile uint8_t portA;
	volatile uint8_t portB;
	volatile uint8_t portC;
	volatile uint8_t cntrl;
};

/* Autoconf stuff */
static int  lcd_match(device_t, cfdata_t, void *);
static void lcd_attach(device_t, device_t, void *);

static dev_type_open(lcdopen);
static dev_type_close(lcdclose);
static dev_type_write(lcdwrite);
static dev_type_ioctl(lcdioctl);

const struct cdevsw lcd_cdevsw = {
	.d_open     = lcdopen,
	.d_close    = lcdclose,
	.d_read     = noread,
	.d_write    = lcdwrite,
	.d_ioctl    = lcdioctl,
	.d_stop     = nostop,
	.d_tty      = notty,
	.d_poll     = nopoll,
	.d_mmap     = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard  = nodiscard,
	.d_flag     = 0
};

struct lcd_softc {
	device_t sc_dev;

	bool sc_opened;
};

CFATTACH_DECL_NEW(lcd, sizeof(struct lcd_softc),
    lcd_match, lcd_attach, NULL, NULL);

static void lcdbusywait(void);
static void lcdput(int);
static void lcdctrl(int);
static void lcdshow(char *);
static void greeting(void);
			       /* "1234567890123456" */
static char lcd_boot_message1[] = " NetBSD/luna68k ";
static char lcd_boot_message2[] = "   SX-9100/DT   ";

/*
 * Autoconf functions
 */
static int
lcd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, lcd_cd.cd_name))
		return 0;
	if (badaddr((void *)ma->ma_addr, 4))
		return 0;
	return 1;
}

static void
lcd_attach(device_t parent, device_t self, void *aux)
{

	printf("\n");

	/* Say hello to the world on LCD. */
	greeting();
}

/*
 * open/close/write/ioctl
 */
static int
lcdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	int unit;
	struct lcd_softc *sc;

	unit = minor(dev);
	sc = device_lookup_private(&lcd_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_opened)
		return EBUSY;
	sc->sc_opened = true;

	return 0;
}

static int
lcdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	int unit;
	struct lcd_softc *sc;

	unit = minor(dev);
	sc = device_lookup_private(&lcd_cd, unit);
	sc->sc_opened = false;

	return 0;
}

static int
lcdwrite(dev_t dev, struct uio *uio, int flag)
{
	int error;
	size_t len, i;
	char buf[LCD_MAXBUFLEN];

	len = uio->uio_resid;

	if (len > LCD_MAXBUFLEN)
		return EIO;

	error = uiomove(buf, len, uio);
	if (error)
		return EIO;

	for (i = 0; i < len; i++) {
		lcdput((int)buf[i]);
	}

	return 0;
}

static int
lcdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	int val;

	/* check if the device opened with write mode */
	switch (cmd) {
	case LCDCLS:
	case LCDHOME:
	case LCDMODE:
	case LCDDISP:
	case LCDMOVE:
	case LCDSEEK:
	case LCDRESTORE:
		if ((flag & FWRITE) == 0)
			return EACCES;
	}

	switch (cmd) {
	case LCDCLS:
		lcdctrl(LCD_CLS);
		break;

	case LCDHOME:
		lcdctrl(LCD_HOME);
		break;

	case LCDMODE:
		val = *(int *)addr;
		switch (val) {
		case LCDMODE_C_LEFT:
		case LCDMODE_C_RIGHT:
		case LCDMODE_D_LEFT:
		case LCDMODE_D_RIGHT:
			lcdctrl(val);
			break;
		default:
			return EINVAL;
		}
		break;

	case LCDDISP:
		val = *(int *)addr;
		if ((val & 0x7) != val)
			return EINVAL;
		lcdctrl(val | 0x8);
		break;

	case LCDMOVE:
		val = *(int *)addr;
		switch (val) {
		case LCDMOVE_C_LEFT:
		case LCDMOVE_C_RIGHT:
		case LCDMOVE_D_LEFT:
		case LCDMOVE_D_RIGHT:
			lcdctrl(val);
			break;
		default:
			return EINVAL;
		}
		break;

	case LCDSEEK:
		val = *(int *)addr & 0x7f;
		lcdctrl(val | 0x80);
		break;

	case LCDRESTORE:
		greeting();
		break;

	default:
		return ENOTTY;
	}
	return EPASSTHROUGH;
}

static void
lcdbusywait(void)
{
	struct pio *p1 = (struct pio *)OBIO_PIO1_BASE;
	int msb, s;

	s = splhigh();
	p1->cntrl = PIO1_MODE_INPUT;
	p1->portC = POWER | READ_BUSY | ENABLE;
	splx(s);

	do {
		msb = p1->portA & ENABLE;
		delay(5);
	} while (msb != 0);

	s = splhigh();
	p1->portC = POWER | READ_BUSY | DISABLE;
	splx(s);
}

static void
lcdput(int cc)
{
	struct pio *p1 = (struct pio *)OBIO_PIO1_BASE;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_DATA | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_DATA | DISABLE;
	splx(s);
}

static void
lcdctrl(int cc)
{
	struct pio *p1 = (struct pio *)OBIO_PIO1_BASE;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_CMD | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_CMD | DISABLE;
	splx(s);
}

static void
lcdshow(char *s)
{
	int cc;

	while ((cc = *s++) != '\0')
		lcdput(cc);
}

static void
greeting(void)
{

	lcdctrl(LCD_INIT);
	lcdctrl(LCD_ENTRY);
	lcdctrl(LCD_ON);

	lcdctrl(LCD_CLS);
	lcdctrl(LCD_HOME);

	lcdctrl(LCD_LOCATE(0, 0));
	lcdshow(lcd_boot_message1);
	lcdctrl(LCD_LOCATE(0, 1));
	if (machtype == LUNA_II)
		lcd_boot_message2[13] = '2';
	lcdshow(lcd_boot_message2);
}
