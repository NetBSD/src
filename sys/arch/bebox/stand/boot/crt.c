/*	$Id: crt.c,v 1.1 1998/01/16 04:17:43 sakamoto Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * CRT 'glass TTY' emulator
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
typedef unsigned short u_short;
typedef unsigned char  u_char;

#define	COL		80
#define	ROW		25
#define	CHR		2
#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000

unsigned char background = 0;  /* Black */
unsigned char foreground = 7;  /* White */

unsigned int addr_6845;
unsigned short *Crtat;
int lastpos;
int scroll;

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

unsigned short	pccolor;		/* color/attributes for tty output */
unsigned short	pccolor_so;		/* color/attributes, standout mode */

/*
 * Just a way to force I/O
 */

static NOP() {}
 

/*
 * cursor() sets an offset (0-1999) into the 80x25 text area   
 */
static void
cursor()
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
initscreen()
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

fillw(unsigned short val, unsigned short *buf, int num)
{
	/* Need to byte swap value */
	unsigned short tmp;
	tmp = val;
	while (num-- > 0)
	{
		*buf++ = tmp;
	}
}

/*
 * CRT_putc (nee sput) has support for emulation of the 'ibmpc' termcap entry.
 * This is a bare-bones implementation of a bare-bones entry
 * One modification: Change li#24 to li#25 to reflect 25 lines
 * "ca" is the color/attributes value (left-shifted by 8)
 * or 0 if the current regular color for that screen is to be used.
 */
void 
CRT_putc(u_char c)
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
			pp = base + COL * (ROW-2);
			for (j = ROW - 1 - i; j--; pp -= COL)
				bcopy(pp, pp + COL, COL * CHR);
			fillw(d->color|(' '<<8), base + i * COL, COL);
			break;
			
		case 'M':	/* Delete line */
			i = (d->cp - base) / COL;
			pp = base + i * COL;
			bcopy(pp + COL, pp, (ROW-1 - i)*COL*CHR);
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
		bcopy(base + COL, base, COL * (ROW - 1) * CHR);
		fillw(d->color|(' '<<8), base + COL * (ROW - 1), COL);
		d->cp -= COL;
	}	
	cursor();
}

void
CRT_puts(char *s)
{
	char c;
	while (c = *s++)
	{
		CRT_putc(c);
	}
}

video_on()
{ /* Enable video */
	outb(0x3C4, 0x01);
	outb(0x3C5, inb(0x3C5)&~0x20);
}

video_off()
{ /* Disable video */
	outb(0x3C4, 0x01);
	outb(0x3C5, inb(0x3C5)|0x20);
}

CRT_init(unsigned char *ISA_mem)
{
	struct screen *d = &screen;
	bzero(d, sizeof(screen));
	video_on();
	d->cp = Crtat = (u_short *)&ISA_mem[0x0B8000];
	addr_6845 = CGA_BASE;
	initscreen();
	fillw(pccolor|(' '<<8), d->cp, COL * ROW);
	return (1);
}

/* Keyboard handler */

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

unsigned char shfts, ctls, alts, caps, num, stp;

#define	KBDATAP		0x60	/* kbd data port */
#define	KBSTATUSPORT	0x61	/* kbd status */
#define	KBSTATP		0x64	/* kbd status port */
#define	KBINRDY		0x01
#define	KBOUTRDY	0x02

#define _x__ 0x00  /* Unknown / unmapped */

const unsigned char keycode[] = {
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
kbd(noblock)
	int noblock;
{
	unsigned char dt, brk, act;
	int first = 1;	
loop:
	if (noblock) {
		if ((inb(KBSTATP) & KBINRDY) == 0)
			return (-1);
	} else while((inb(KBSTATP) & KBINRDY) == 0) ;

	dt = inb(KBDATAP);

	brk = dt & 0x80;	/* brk == 1 on key release */
	dt = dt & 0x7f;		/* keycode */

	act = action[dt];
	if (act&SHF)
		shfts = brk ? 0 : 1;
	if (act&ALT)
		alts = brk ? 0 : 1;
	if (act&NUM)
		if (act&L) {
			/* NUM lock */
			if(!brk)
				num = !num;
		} else
			num = brk ? 0 : 1;
	if (act&CTL)
		ctls = brk ? 0 : 1;
	if (act&CPS)
		if (act&L) {
			/* CAPS lock */
			if(!brk)
				caps = !caps;
		} else
			caps = brk ? 0 : 1;
	if (act&STP)
		if (act&L) {
			if(!brk)
				stp = !stp;
		} else
			stp = brk ? 0 : 1;

	if ((act&ASCII) && !brk) {
		unsigned char chr;

		if (shfts)
			chr = shift[dt];
		else if (ctls)
			chr = ctl[dt];
		else
			chr = unshift[dt];

		if (alts)
			chr |= 0x80;

		if (caps && (chr >= 'a' && chr <= 'z'))
			chr -= 'a' - 'A' ;
#define CTRL(s) (s & 0x1F)			
		if ((chr == '\r') || (chr == '\n') || (chr == CTRL('A')) || (chr == CTRL('S')))
		{
			/* Wait for key up */
			while (1)
			{
				while((inb(KBSTATP) & KBINRDY) == 0) ;
				dt = inb(KBDATAP);
				if (dt & 0x80) /* key up */ break;
			}
		}
		return (chr);
	}
	if (first && brk) return (0);  /* Ignore initial 'key up' codes */
	goto loop;
}

kbdreset()
{
	unsigned char c;
	int i;

#if 0

	/* Enable interrupts and keyboard controller */
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBSTATP,0x60);
	
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBDATAP,0x4D);

	/* Start keyboard stuff RESET */
	while (inb(KBSTATP) & KBOUTRDY) ;	/* wait input ready */
	outb(KBDATAP,0xFF);	/* RESET */

	while ((c = inb(KBDATAP)) != 0xFA) ;

#else	

	/* Send self-test */
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBSTATP,0xAA);
	while ((inb(KBSTATP) & KBINRDY) == 0) ;	/* wait input ready */
	if ((c = inb(KBDATAP)) != 0x55)
	{
		printf("Keyboard self test failed - result: %x\n", c);
	}
	/* Enable interrupts and keyboard controller */
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBSTATP,0x60);	
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBDATAP,0x45);
	for (i = 0;  i < 10000;  i++) NOP();
	while (inb(KBSTATP) & KBOUTRDY) ;
	outb(KBSTATP,0xAE);
#endif	
}

CRT_getc()
{
	int c;
	while ((c = kbd(0)) == 0) ;
	return(c);
}

CRT_test()
{
	return ((inb(KBSTATP) & KBINRDY) != 0);
}
#endif /* CONS_VGA */
