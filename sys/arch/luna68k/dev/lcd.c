/* $NetBSD: lcd.c,v 1.6.12.1 2012/04/17 00:06:35 yamt Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: lcd.c,v 1.6.12.1 2012/04/17 00:06:35 yamt Exp $");

/*
 * XXX
 * Following code segments are subject to change.
 * XXX
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>

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

struct pio {
	volatile u_int8_t portA;
	volatile u_int8_t portB;
	volatile u_int8_t portC;
	volatile u_int8_t cntrl;
};

void lcdbusywait(void);
void lcdput(int);
void lcdctrl(int);
void lcdshow(char *);
void greeting(void);
			       /* "1234567890123456" */
static char lcd_boot_message1[] = " NetBSD/luna68k ";
static char lcd_boot_message2[] = "   SX-9100/DT   ";

void
lcdbusywait(void)
{
	struct pio *p1 = (struct pio *)0x4D000000;
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

void
lcdput(int cc)
{
	struct pio *p1 = (struct pio *)0x4D000000;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_DATA | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_DATA | DISABLE;
	splx(s);
}

void
lcdctrl(int cc)
{
	struct pio *p1 = (struct pio *)0x4D000000;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_CMD | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_CMD | DISABLE;
	splx(s);
}

void
lcdshow(char *s)
{
	int cc;

	while ((cc = *s++) != '\0')
		lcdput(cc);
}

void
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
