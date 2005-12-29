/*	$NetBSD: cons_zskbd.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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
#include <lib/libkern/libkern.h>

#include <machine/sbd.h>

#include "console.h"

struct zskbd zskbd;

int zskbd_common_getc(int);
void zskbd_busy(void);

static const uint8_t map_normal[] = {
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	'-',	'^',	'\\',	':',	'.',	'/',
	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
	'x',	'y',	'z',	'[',	',',	']',	';',	0x00,
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	' ',	',',	0x00,	0x00,	0x00,	0x00,
	0x0d,	0x0d,	0x00,	0x00,	0x0d,	0x00,	'-',	'.',
	0xa3,	0xa2,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x08,	0x00,	0x0c,	0x7f,	0x12,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0xa0,	0xa1,	0x00,	0x00,	0x00,	0x00,
	'+',	'*',	'/',	0x1b,	0x01,	'=',	0x09,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};

static const uint8_t map_shift[] = {
	'0',	'!',	'\"',	'#',	'$',	'%',	'&',	'\'',
	'(',	')',	'=',	'^',	'|',	'*',	'>',	'?',
	'~',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
	'X',	'Y',	'Z',	'{',	'<',	'}',	'+',	'_',
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	' ',	',',	0x00,	0x00,	0x00,	0x00,
	0x0d,	0x0d,	'B',	'C',	0x0d,	'E',	'-',	'.',
	0xa3,	0xa2,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x08,	0x00,	0x0b,	0x7f,	0x12,	0x09,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0xa0,	0xa1,	0x00,	0x00,	0x00,	0x00,
	'+',	'*',	'/',	0x1b,	0x01,	'=',	0x09,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};

static const uint8_t map_ctrl[] = {
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0xa4,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
	0x08,	0x09,	0x0a,	0x0b,	0x0c,	0x0d,	0x0e,	0x0f,
	0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,
	0x18,	0x19,	0x1a,	0x1b,	0x1c,	0x1d,	0x1e,	0x1f,
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	' ',	',',	0x00,	0x00,	0x00,	0x00,
	0x0d,	0x0d,	0x00,	0x00,	0x0d,	0x00,	'-',	'.',
	0xa3,	0xa2,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x08,	0x00,	0x00,	0x7f,	0x12,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0xa0,	0xa1,	0x00,	0x00,	0x00,	0x00,
	'+',	'*',	'/',	0x1b,	0x01,	'=',	0x09,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};

static const uint8_t map_capslock[] = {
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	'-',	'^',	'\\',	':',	'.',	'/',
	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
	'X',	'Y',	'Z',	'[',	',',	']',	';',	0x00,
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	' ',	',',	0x00,	0x00,	0x00,	0x00,
	0x0d,	0x0d,	0x00,	0x00,	0x0d,	0x00,	'-',	'.',
	0xa3,	0xa2,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x08,	0x00,	0x0c,	0x7f,	0x12,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0x00,	0x00,	0xa0,	0xa1,	0x00,	0x00,	0x00,	0x00,
	'+',	'*',	'/',	0x1b,	0x01,	'=',	0x09,	0x00,
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};

void
zskbd_set_addr(uint32_t status, uint32_t data)
{
	zskbd.status = (volatile uint8_t *)status;
	zskbd.data = (volatile uint8_t *)data;
	zskbd.normal = map_normal;
	zskbd.shift = map_shift;
	zskbd.ctrl = map_ctrl;
	zskbd.capslock = map_capslock;

	cons.getc = zskbd_getc;
	cons.scan = zskbd_scan;
}

void
zskbd_print_keyscan(int on)
{

	zskbd.print = on;
}

int
zskbd_getc(void)
{

	return zskbd_common_getc(0);
}

int
zskbd_scan(void)
{

	return zskbd_common_getc(1);
}

int
zskbd_common_getc(int scan)
{
#ifdef CHECK_KEY_RELEASE
	static int released = 1;
	int push;
#endif
	int v, c;

	for (;;) {
		if (scan) {
			if ((*zskbd.status & 0x01) != 0x01)
				return -1;
		} else {
			while ((*zskbd.status & 0x01) != 0x01)
				continue;
		}

		v = *zskbd.data;
		if (zskbd.print)
			printf("scancode = 0x%x\n", v);

		switch (v) {
		case 123:	/* Shift-L */
			/* FALLTHROUGH */
		case 124:	/* Shift-R */
			zskbd.keymap  |= 0x01;
			break;
		case 251:
			/* FALLTHROUGH */
		case 252:
			zskbd.keymap &= ~0x01;
			break;
		case 120:	/* Ctrl-L */
			zskbd.keymap |= 0x02;
			break;
		case 248:
			zskbd.keymap &= ~0x02;
			break;
		case 121:	/* CapsLock */
			if (zskbd.keymap & 0x04) {
				zskbd.keymap  &= ~0x04;
				zskbd_busy();
				*zskbd.data = 0x90; /* LED */
			} else {
				zskbd.keymap  |= 0x04;
				zskbd_busy();
				*zskbd.data = 0x92; /* LED */
			}
			break;
		default:
#ifdef CHECK_KEY_RELEASE
			push = (v & 0x80) == 0;
			if (push && released) {
				released = 0;
				goto exit_loop;
			}
			if (!push)
				released = 1;
#else /* CHECK_KEY_RELEASE */
			if ((v & 0x80) == 0)
				goto exit_loop;
#endif /* CHECK_KEY_RELEASE */
			break;
		}
	}

 exit_loop:
	c = v & 0x7f;
	if (zskbd.keymap & 0x01)	/* Shift */
		return *(zskbd.shift + c);
	if (zskbd.keymap & 0x02)	/* Ctrl */
		return *(zskbd.ctrl + c);
	if (zskbd.keymap & 0x04)	/* CapsLock */
		return *(zskbd.capslock + c);
				/* Normal */
	return *(zskbd.normal + c);
}

void
zskbd_busy(void)
{

#if 0 /* I misunderstand??? -uch */
	do {
		while ((*zskbd.status & 0x20) != 0x20)
			;
	} while ((*zskbd.status & 0x4) != 0x4);
#endif
}
