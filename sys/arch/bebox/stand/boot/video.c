/*	$NetBSD: video.c,v 1.4 1999/06/28 01:20:45 sakamoto Exp $	*/

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

#ifdef CONS_BE
#include <stand.h>
#include "boot.h"
#include "iso_font.h"

#define	FONT_WIDTH	8
#define	FONT_HEIGHT	16
#define	VRAM_WIDTH	640
#define	VRAM_HEIGHT	480
#define	VRAM_SIZE	(VRAM_WIDTH * VRAM_HEIGHT)
#define	ROW	(VRAM_WIDTH / FONT_WIDTH)
#define	COL	(VRAM_HEIGHT / FONT_HEIGHT)

u_char *VramBase;
u_char save_cursor[FONT_WIDTH];

/*
 * The current state of virtual displays
 */
struct screen {
	enum state {
		NORMAL,		/* no pending escape */
		ESC,		/* saw ESC */
		EBRAC,		/* saw ESC[ */
		EBRACEQ		/* saw ESC[= */
	} state;		/* command parser state */
	int	cx;		/* the first escape seq argument */
	int	cy;		/* the second escap seq argument */
	int	*accp;		/* pointer to the current processed argument */
	int	row;
	int	col;
	u_char fgcolor;
	u_char bgcolor;
} screen;


void
video_init(buffer)
	u_char *buffer;
{
	struct screen *d = &screen;

	VramBase = buffer;
	d->fgcolor = 0x1f;	/* WHITE */
	d->bgcolor = 0x00;	/* BLACK */
	d->row = 0;
	d->col = 0;
	d->state = NORMAL;
	memset(VramBase, d->bgcolor, VRAM_SIZE);
	memset(save_cursor, d->bgcolor, FONT_WIDTH);
}

static void
wrtchar(c, d)
	u_char c;
	struct screen *d;
{
	u_char *p = VramBase +
		(d->col * VRAM_WIDTH * FONT_HEIGHT) + (d->row * FONT_WIDTH);
	int fontbase = c * 16;
	int x, y;

	for (y = 0; y < FONT_HEIGHT; y++) {
		for (x = 0; x < FONT_WIDTH; x++) {
			if ((font[fontbase + y] >> (FONT_WIDTH - x)) & 1)
				*(p + x + (y * VRAM_WIDTH)) = d->fgcolor;
			else
				*(p + x + (y * VRAM_WIDTH)) = d->bgcolor;
		}
	}
	d->row++;
}

static void
cursor(d, flag)
	struct screen *d;
	int flag;
{
	int x;
	int y = FONT_HEIGHT - 1;

	u_char *p = VramBase +
		(d->col * VRAM_WIDTH * FONT_HEIGHT) + (d->row * FONT_WIDTH);
	for (x = 0; x < FONT_WIDTH - 1; x++) {
		if (flag) {
			save_cursor[x] = *(p + x + (y * VRAM_WIDTH));
			*(p + x + (y * VRAM_WIDTH)) = d->fgcolor;
		} else {
			*(p + x + (y * VRAM_WIDTH)) = save_cursor[x];
		}
	}
}

/*
 * vga_putc (nee sput) has support for emulation of the 'ibmpc' termcap entry.
 * This is a bare-bones implementation of a bare-bones entry
 * One modification: Change li#24 to li#25 to reflect 25 lines
 * "ca" is the color/attributes value (left-shifted by 8)
 * or 0 if the current regular color for that screen is to be used.
 */
void 
video_putc(c)
	int c;
{
	struct screen *d = &screen;

	cursor(d, 0);

	switch (d->state) {
	case NORMAL:
		switch (c) {
		case 0x0:		/* Ignore pad characters */
			return;

		case 0x1B:
			d->state = ESC;
			break;

		case '\t':
			do {
				wrtchar(' ', d);
			} while (d->row % 8);
			break;

		case '\b':  /* non-destructive backspace */
			d->row--;
			if (d->row < 0) {
				if (d->col > 0) {
					d->col--;
					d->row += ROW;	/* prev column */
				} else {
					d->row = 0;
				}
			}
			break;

		case '\n':
			d->col++;
		case '\r':
			d->row = 0;
			break;

		case '\007':
			break;

		default:
			wrtchar(c, d);
			if (d->row >= ROW) {
				d->row = 0;
				d->col++;
			}
			break;
		}
		break;

	case ESC:
	case EBRAC:
	case EBRACEQ:
		break;
	}
	if (d->col >= COL) {		/* scroll check */
		memcpy(VramBase, VramBase + VRAM_WIDTH * FONT_HEIGHT,
			VRAM_SIZE - VRAM_WIDTH * FONT_WIDTH);
		memset(VramBase + VRAM_SIZE - VRAM_WIDTH * FONT_HEIGHT,
			d->bgcolor, VRAM_WIDTH * FONT_HEIGHT);
		d->col = COL - 1;
	}	
	cursor(d, 1);
}
#endif
