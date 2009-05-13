/*	$NetBSD: vga.c,v 1.8.12.1 2009/05/13 17:16:34 jym Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * VGA 'glass TTY' emulator
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

#define	COL		80
#define	ROW		25
#define	CHR		2
#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000

static u_char background = 0;  /* Black */
static u_char foreground = 7;  /* White */

u_int addr_6845;
u_short *Crtat;
int lastpos;

/*
 * The current state of virtual displays
 */
struct screen {
	u_short *cp;		/* the current character address */
	enum state {
		NORMAL,			/* no pending escape */
		ESC,			/* saw ESC */
		EBRAC,			/* saw ESC[ */
		EBRACEQ			/* saw ESC[= */
	} state;		/* command parser state */
	int	cx;		/* the first escape seq argument */
	int	cy;		/* the second escap seq argument */
	int	*accp;		/* pointer to the current processed argument */
	int	row;		/* current column */
	int	so;		/* standout mode */
	u_short color;		/* normal character color */
	u_short color_so;	/* standout color */
	u_short save_color;	/* saved normal color */
	u_short save_color_so;	/* saved standout color */
} screen;

/*
 * Color and attributes for normal, standout and kernel output
 * are stored in the least-significant byte of a u_short
 * so they don't have to be shifted for use.
 * This is all byte-order dependent.
 */
#define	CATTR(x) (x)		/* store color/attributes un-shifted */
#define	ATTR_ADDR(which) (((u_char *)&(which))+1) /* address of attributes */

u_short	pccolor;		/* color/attributes for tty output */
u_short	pccolor_so;		/* color/attributes, standout mode */

static void cursor(void);
static void initscreen(void);
void fillw(u_short, u_short *, int);
void video_on(void);
void video_off(void);

/*
 * cursor() sets an offset (0-1999) into the 80x25 text area
 */
static void
cursor(void)
{
 	int pos = screen.cp - Crtat;

	if (lastpos != pos) {
		outb(addr_6845, 14);
		outb(addr_6845+1, pos >> 8);
		outb(addr_6845, 15);
		outb(addr_6845+1, pos);
		lastpos = pos;
	}
}

static void
initscreen(void)
{
	struct screen *d = &screen;

	pccolor = CATTR((background<<4)|foreground);
	pccolor_so = CATTR((foreground<<4)|background);
	d->color = pccolor;
	d->save_color = pccolor;
	d->color_so = pccolor_so;
	d->save_color_so = pccolor_so;
}


#define	wrtchar(c, d) { \
	*(d->cp) = c; \
	d->cp++; \
	d->row++; \
}

void
fillw(u_short val, u_short *buf, int num)
{
	/* Need to byte swap value */
	u_short tmp;

	tmp = val;
	while (num-- > 0)
		*buf++ = tmp;
}

/*
 * vga_putc (nee sput) has support for emulation of the 'ibmpc' termcap entry.
 * This is a bare-bones implementation of a bare-bones entry
 * One modification: Change li#24 to li#25 to reflect 25 lines
 * "ca" is the color/attributes value (left-shifted by 8)
 * or 0 if the current regular color for that screen is to be used.
 */
void
vga_putc(int c)
{
	struct screen *d = &screen;
	u_short *base;
	int i, j;
	u_short *pp;

	base = Crtat;

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
				wrtchar(d->color | ' ', d);
			} while (d->row % 8);
			break;

		case '\b':  /* non-destructive backspace */
			if (d->cp > base) {
				d->cp--;
				d->row--;
				if (d->row < 0)
					d->row += COL;	/* prev column */
			}
			break;

		case '\n':
			d->cp += COL;
		case '\r':
			d->cp -= d->row;
			d->row = 0;
			break;

		case '\007':
			break;

		default:
			if (d->so) {
				wrtchar(d->color_so|(c<<8), d);
			} else {
				wrtchar(d->color | (c<<8), d);
			}
			if (d->row >= COL)
				d->row = 0;
			break;
		}
		break;

	case EBRAC:
		/*
		 * In this state, the action at the end of the switch
		 * on the character type is to go to NORMAL state,
		 * and intermediate states do a return rather than break.
		 */
		switch (c) {
		case 'm':
			d->so = d->cx;
			break;

		case 'A': /* back one row */
			if (d->cp >= base + COL)
				d->cp -= COL;
			break;

		case 'B': /* down one row */
			d->cp += COL;
			break;

		case 'C': /* right cursor */
			d->cp++;
			d->row++;
			break;

		case 'D': /* left cursor */
			if (d->cp > base) {
				d->cp--;
				d->row--;
				if (d->row < 0)
					d->row += COL;	/* prev column ??? */
			}
			break;

		case 'J': /* Clear to end of display */
			fillw(d->color|(' '<<8), d->cp, base + COL * ROW - d->cp);
			break;

		case 'K': /* Clear to EOL */
			fillw(d->color|(' '<<8), d->cp, COL - (d->cp - base) % COL);
			break;

		case 'H': /* Cursor move */
			if (d->cx > ROW)
				d->cx = ROW;
			if (d->cy > COL)
				d->cy = COL;
			if (d->cx == 0 || d->cy == 0) {
				d->cp = base;
				d->row = 0;
			} else {
				d->cp = base + (d->cx - 1) * COL + d->cy - 1;
				d->row = d->cy - 1;
			}
			break;

		case '_': /* set cursor */
			if (d->cx)
				d->cx = 1;		/* block */
			else
				d->cx = 12;	/* underline */
			outb(addr_6845, 10);
			outb(addr_6845+1, d->cx);
			outb(addr_6845, 11);
			outb(addr_6845+1, 13);
			break;

		case ';': /* Switch params in cursor def */
			d->accp = &d->cy;
			return;

		case '=': /* ESC[= color change */
			d->state = EBRACEQ;
			return;

		case 'L':	/* Insert line */
			i = (d->cp - base) / COL;
			/* avoid deficiency of bcopy implementation */
			/* XXX: comment and hack relevant? */
			pp = base + COL * (ROW-2);
			for (j = ROW - 1 - i; j--; pp -= COL)
				memmove(pp + COL, pp, COL * CHR);
			fillw(d->color|(' '<<8), base + i * COL, COL);
			break;

		case 'M':	/* Delete line */
			i = (d->cp - base) / COL;
			pp = base + i * COL;
			memmove(pp, pp + COL, (ROW-1 - i)*COL*CHR);
			fillw(d->color|(' '<<8), base + COL * (ROW - 1), COL);
			break;

		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				*(d->accp) *= 10;
				*(d->accp) += c - '0';
				return;
			} else
				break;
		}
		d->state = NORMAL;
		break;

	case EBRACEQ: {
		/*
		 * In this state, the action at the end of the switch
		 * on the character type is to go to NORMAL state,
		 * and intermediate states do a return rather than break.
		 */
		u_char *colp;

		/*
		 * Set foreground/background color
		 * for normal mode, standout mode
		 * or kernel output.
		 * Based on code from kentp@svmp03.
		 */
		switch (c) {
		case 'F':
			colp = ATTR_ADDR(d->color);
	do_fg:
			*colp = (*colp & 0xf0) | (d->cx);
			break;

		case 'G':
			colp = ATTR_ADDR(d->color);
	do_bg:
			*colp = (*colp & 0xf) | (d->cx << 4);
			break;

		case 'H':
			colp = ATTR_ADDR(d->color_so);
			goto do_fg;

		case 'I':
			colp = ATTR_ADDR(d->color_so);
			goto do_bg;

		case 'S':
			d->save_color = d->color;
			d->save_color_so = d->color_so;
			break;

		case 'R':
			d->color = d->save_color;
			d->color_so = d->save_color_so;
			break;

		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				d->cx *= 10;
				d->cx += c - '0';
				return;
			} else
				break;
		}
		d->state = NORMAL;
	    }
	    break;

	case ESC:
		switch (c) {
		case 'c':	/* Clear screen & home */
			fillw(d->color|(' '<<8), base, COL * ROW);
			d->cp = base;
			d->row = 0;
			d->state = NORMAL;
			break;
		case '[':	/* Start ESC [ sequence */
			d->state = EBRAC;
			d->cx = 0;
			d->cy = 0;
			d->accp = &d->cx;
			break;
		default: /* Invalid, clear state */
			d->state = NORMAL;
			break;
		}
		break;
	}
	if (d->cp >= base + (COL * ROW)) { /* scroll check */
		memmove(base, base + COL, COL * (ROW - 1) * CHR);
		fillw(d->color|(' '<<8), base + COL * (ROW - 1), COL);
		d->cp -= COL;
	}
	cursor();
}

void
vga_puts(char *s)
{
	char c;
	while ((c = *s++)) {
		vga_putc(c);
	}
}

void
video_on(void)
{

	/* Enable video */
	outb(0x3C4, 0x01);
	outb(0x3C5, inb(0x3C5) & ~0x20);
}

void
video_off(void)
{

	/* Disable video */
	outb(0x3C4, 0x01);
	outb(0x3C5, inb(0x3C5) | 0x20);
}

void
vga_init(u_char *ISA_mem)
{
	struct screen *d = &screen;

	memset(d, 0, sizeof (screen));
	video_on();

	d->cp = Crtat = (u_short *)&ISA_mem[0x0B8000];
	addr_6845 = CGA_BASE;
	initscreen();
	fillw(pccolor|(' '<<8), d->cp, COL * ROW);
}
#endif /* CONS_VGA */
