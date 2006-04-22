/*	$NetBSD: kbd.c,v 1.1.62.1 2006/04/22 11:37:54 simonb Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifdef CONS_VGA
#include <lib/libsa/stand.h>
#include "boot.h"

/*
 * Keyboard handler
 */
#define	L		0x0001	/* locking function */
#define	SHF		0x0002	/* keyboard shift */
#define	ALT		0x0004	/* alternate shift -- alternate chars */
#define	NUM		0x0008	/* numeric shift  cursors vs. numeric */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	CPS		0x0020	/* caps shift -- swaps case of letter */
#define	ASCII		0x0040	/* ascii code for this key */
#define	STP		0x0080	/* stop output */
#define	FUNC		0x0100	/* function key */
#define	SCROLL		0x0200	/* scroll lock key */

#include "pcconstab.US"

u_char shfts, ctls, alts, caps, num, stp;

#define	KBDATAP		0x60	/* kbd data port */
#define	KBSTATUSPORT	0x61	/* kbd status */
#define	KBSTATP		0x64	/* kbd status port */
#define	KBINRDY		0x01
#define	KBOUTRDY	0x02

#define	_x__ 0x00  /* Unknown / unmapped */

const u_char keycode[] = {
	_x__, 0x43, 0x41, 0x3F, 0x3D, 0x3B, 0x3C, _x__, /* 0x00-0x07 */
	_x__, 0x44, 0x42, 0x40, 0x3E, 0x0F, 0x29, _x__, /* 0x08-0x0F */
	_x__, 0x38, 0x2A, _x__, 0x1D, 0x10, 0x02, _x__, /* 0x10-0x17 */
	_x__, _x__, 0x2C, 0x1F, 0x1E, 0x11, 0x03, _x__, /* 0x18-0x1F */
	_x__, 0x2E, 0x2D, 0x20, 0x12, 0x05, 0x04, _x__, /* 0x20-0x27 */
	_x__, 0x39, 0x2F, 0x21, 0x14, 0x13, 0x06, _x__, /* 0x28-0x2F */
	_x__, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, _x__, /* 0x30-0x37 */
	_x__, _x__, 0x32, 0x24, 0x16, 0x08, 0x09, _x__, /* 0x38-0x3F */
	_x__, 0x33, 0x25, 0x17, 0x18, 0x0B, 0x0A, _x__, /* 0x40-0x47 */
	_x__, 0x34, 0x35, 0x26, 0x27, 0x19, 0x0C, _x__, /* 0x48-0x4F */
	_x__, _x__, 0x28, _x__, 0x1A, 0x0D, _x__, _x__, /* 0x50-0x57 */
	0x3A, 0x36, 0x1C, 0x1B, _x__, 0x2B, _x__, _x__, /* 0x58-0x5F */
	_x__, _x__, _x__, _x__, _x__, _x__, 0x0E, _x__, /* 0x60-0x67 */
	_x__, 0x4F, _x__, 0x4B, 0x47, _x__, _x__, _x__, /* 0x68-0x6F */
	0x52, 0x53, 0x50, 0x4C, 0x4D, 0x48, 0x01, 0x45, /* 0x70-0x77 */
	_x__, 0x4E, 0x51, 0x4A, _x__, 0x49, 0x46, 0x54, /* 0x78-0x7F */
};

int
kbd(int noblock)
{
	u_char dt, brk, act;
	int first = 1;

loop:
	if (noblock) {
		if ((inb(KBSTATP) & KBINRDY) == 0)
			return (-1);
	} else while ((inb(KBSTATP) & KBINRDY) == 0)
		;

	dt = inb(KBDATAP);

	brk = dt & 0x80;	/* brk == 1 on key release */
	dt = dt & 0x7f;		/* keycode */

	act = action[dt];
	if (act & SHF)
		shfts = brk ? 0 : 1;
	if (act & ALT)
		alts = brk ? 0 : 1;
	if (act & NUM) {
		if (act & L) {
			/* NUM lock */
			if (!brk)
				num = !num;
		} else {
			num = brk ? 0 : 1;
		}
	}
	if (act & CTL)
		ctls = brk ? 0 : 1;
	if (act & CPS) {
		if (act & L) {
			/* CAPS lock */
			if (!brk)
				caps = !caps;
		} else {
			caps = brk ? 0 : 1;
		}
	}
	if (act & STP) {
		if (act & L) {
			if (!brk)
				stp = !stp;
		} else {
			stp = brk ? 0 : 1;
		}
	}

	if ((act & ASCII) && !brk) {
		u_char chr;

		if (shfts)
			chr = shift[dt];
		else if (ctls)
			chr = ctl[dt];
		else
			chr = unshift[dt];

		if (alts)
			chr |= 0x80;

		if (caps && (chr >= 'a' && chr <= 'z'))
			chr -= 'a' - 'A';
#define	CTRL(s) (s & 0x1F)
		if ((chr == '\r') || (chr == '\n') ||
		    (chr == CTRL('A')) || (chr == CTRL('S'))) {

			/* Wait for key up */
			while (1)
			{
				while ((inb(KBSTATP) & KBINRDY) == 0)
					;
				dt = inb(KBDATAP);
				if (dt & 0x80) /* key up */ break;
			}
		}
		return (chr);
	}

	if (first && brk)
		return (0);	/* Ignore initial 'key up' codes */
	goto loop;
}

void
kbdreset(void)
{
	u_char c;
	int i;

	/* Send self-test */
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBSTATP, 0xAA);
	while ((inb(KBSTATP) & KBINRDY) == 0)	/* wait input ready */
		;
	if ((c = inb(KBDATAP)) != 0x55) {
		printf("Keyboard self test failed - result: %x\n", c);
	}
	/* Enable interrupts and keyboard controller */
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBSTATP, 0x60);
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBDATAP, 0x45);
	for (i = 0;  i < 10000;  i++)
		;
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBSTATP, 0xAE);
}

int
kbd_getc(void)
{
	int c;
	while ((c = kbd(0)) == 0)
		;
	return (c);
}
#endif /* CONS_VGA */
