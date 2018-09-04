/*	$NetBSD: lcd.c,v 1.3 2018/09/04 15:08:30 riastradh Exp $	*/

/*-
 * Copyright (c) 2008 Izumi Tsutsui.  All rights reserved.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/hd44780reg.h>

#include <machine/cpu.h>

#include "boot.h"

#define IREG	0x00
#define DREG	0x10

#if 0
#define CSR_READ(base, reg)						\
	(((*(volatile uint32_t *)((base) + (reg))) >> 24), delay(10))
#endif

#define CSR_WRITE(base, reg, val)					\
	do {								\
		*(volatile uint32_t *)((base) + (reg)) = ((val) << 24);	\
		delay(10);						\
	} while (/* CONSTCOND */ 0)

#define NCOLS	16

struct lcd_message {
	char row1[NCOLS];
	char row2[NCOLS];
};

static uint32_t lcd_base;
static const struct lcd_message banner_message = {
	"NetBSD/cobalt   ",
	"Bootloader      "
};
static const struct lcd_message failed_message = {
	"Boot failed!    ",
	"Rebooting...    "
};
static struct lcd_message loadfile_message = {
	"Loading:        ",
	"                " 
};

static void lcd_puts(const struct lcd_message *);

void
lcd_init(void)
{

	lcd_base = MIPS_PHYS_TO_KSEG1(LCD_BASE);
}

void
lcd_banner(void)
{

	lcd_puts(&banner_message);
}

void
lcd_loadfile(const char *file)
{

	memcpy(loadfile_message.row2, file, uimin(NCOLS, strlen(file)));

	lcd_puts(&loadfile_message);
}

void
lcd_failed(void)
{

	lcd_puts(&failed_message);
}

static void
lcd_puts(const struct lcd_message *message)
{
	int i;

	for (i = 0; i < NCOLS; i++) {
		CSR_WRITE(lcd_base, IREG, cmd_ddramset(HD_ROW1_ADDR + i));
		CSR_WRITE(lcd_base, DREG, message->row1[i]);
	}
	for (i = 0; i < NCOLS; i++) {
		CSR_WRITE(lcd_base, IREG, cmd_ddramset(HD_ROW2_ADDR + i));
		CSR_WRITE(lcd_base, DREG, message->row2[i]);
	}
}
