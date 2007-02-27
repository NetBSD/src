/*	$NetBSD: console.c,v 1.1.28.1 2007/02/27 16:50:33 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/systm.h>

#include <lib/libsa/stand.h>

#include "local.h"

void __putchar(int);

#define	PUTCHAR		__putchar	/* "boot" hooks log staff. */

#include <machine/sbd.h>

#include "console.h"

struct cons cons;
void nullcursor(int, int);
void nullscroll(void);


void
console_init(void)
{
	uint8_t nvsram_cons;

	cons.cursor = nullcursor;
	cons.scroll = nullscroll;
	cons.cursor_enable = false;
	cons.erace_previous_cursor = false;

	switch (SBD_INFO->machine) {
	case MACHINE_TR2:
		/* untested */
		nvsram_cons = *(volatile uint8_t *)0xbb023010;

		switch (nvsram_cons) {
		case 0x00:
			cons.type = CONS_FB_KSEG2;
			fb_set_addr(0xf0000000, 0x01000000, 0xbfc0ec00);
			zskbd_set_addr(0xbb010000, 0xbb010004);
			cons.init();
			break;
		case 0x01:
			/* sio 1 (zs channel A) */
			cons.type = CONS_SIO1;
			zs_set_addr(0xbb011008, 0xbb01100c, 4915200);
			cons.init();
			break;
		case 0x02:
			/* sio 2 (zs channel B) */
			cons.type = CONS_SIO2;
			zs_set_addr(0xbb011000, 0xbb011004, 4915200);
			cons.init();
			break;
		default:
			goto rom_cons;
		}
		break;
	case MACHINE_TR2A:
		nvsram_cons = *(volatile uint8_t *)0xbe4932a0;

		switch (nvsram_cons) {
		case 0x80:
			/* on-board FB on 360AD, 360ADII */
			cons.type = CONS_FB_KSEG2;
			fb_set_addr(0xf0000000, 0x01000000, 0xbfc0ec00);
			zskbd_set_addr(0xbe480000, 0xbe480004);
			cons.init();
			break;
		case 0x01:
			/* sio 1 (zs channel A) */
			cons.type = CONS_SIO1;
			zs_set_addr(0xbe440008, 0xbe44000c, 4915200);
			cons.init();
			break;
		case 0x02:
			/* sio 2 (zs channel B) */
			cons.type = CONS_SIO2;
			zs_set_addr(0xbe440000, 0xbe440004, 4915200);
			cons.init();
			break;
		default:
			goto rom_cons;
		}
		break;
	default:
 rom_cons:
		cons.getc = ROM_GETC;
		cons.scan = rom_scan;
		cons.putc = ROM_PUTC;
		cons.init = cons_rom_init;
		cons.scroll = cons_rom_scroll;
		cons.type = CONS_ROM;
		cons.init();

		break;
	}
}

void
console_cursor(bool on)
{

	cons.cursor_enable = on;
}

enum console_type
console_type(void)
{

	return cons.type;
}

void
PUTCHAR(int c)
{
	int i;

	if (cons.type == CONS_SIO1 || cons.type == CONS_SIO2) {
		if (c == '\n')
			cons.putc(0, 0, '\r');
		cons.putc(0, 0, c);
		return;
	}

	if (cons.cursor_enable && cons.erace_previous_cursor)
		cons.cursor(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT);

	switch (c) {
	default:
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		if (++cons.x == CONS_WIDTH) {
			cons.x = X_INIT;
			cons.y++;
		}
		break;
	case '\b':
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = cons.x == X_INIT ? X_INIT : cons.x - 1;
		if (cons.cursor_enable)
			cons.putc(cons.x * ROM_FONT_WIDTH,
			    cons.y * ROM_FONT_HEIGHT, ' ');
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		break;
	case '\t':
		for (i = cons.x % 8; i < 8; i++) {
			cons.putc(cons.x * ROM_FONT_WIDTH,
			    cons.y * ROM_FONT_HEIGHT, ' ');
			if (++cons.x == CONS_WIDTH) {
				cons.x = X_INIT;
				if (++cons.y == CONS_HEIGHT)
					cons.scroll();
			}
		}
		break;
	case '\r':
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = X_INIT;
		break;
	case '\n':
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT,
		    '\r');
		cons.putc(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = X_INIT;
		cons.y++;
		break;
	case 2:	/* Ctrl-b */
		if (--cons.x < X_INIT)
			cons.x = X_INIT;
		break;
	case 6:	/* Ctrl-f */
		if (++cons.x >= CONS_WIDTH)
			cons.x = CONS_WIDTH;
		break;
	case 11:/* Ctrl-k */
		for (i = cons.x; i < CONS_WIDTH; i++)
			cons.putc(i * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT,
			     ' ');
		break;
	}

	if (cons.y == CONS_HEIGHT)
		cons.scroll();

	if (cons.cursor_enable) {
		cons.cursor(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT);
		cons.erace_previous_cursor = true;
	} else {
		cons.erace_previous_cursor = false;
	}
}

int
getchar(void)
{

	return cons.getc();
}

int
cnscan(void)
{

	return cons.scan();
}

void
nullcursor(int x, int y)
{
	/* place holder */
}

void
nullscroll(void)
{
	/* place holder */
}
