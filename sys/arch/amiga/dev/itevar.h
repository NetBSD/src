/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: itevar.h 1.1 90/07/09$
 *
 *	@(#)itevar.h	7.2 (Berkeley) 11/4/90
 */

#define UNIT(dev)       minor(dev)

struct itesw {
	int	(*ite_init)();
	int	(*ite_deinit)();
	int	(*ite_clear)();
	int	(*ite_putc)();
	int	(*ite_cursor)();
	int	(*ite_scroll)();
};

/* maximum number of argument characters (<CSI>33;34;3m for example) */
#define ARGBUF_SIZE 256

struct ite_softc {
	int	flags;
	int	type;
	struct grf_softc *grf;
	void	*priv;
	short	curx, cury;
	short   cursorx, cursory;
	u_char	*font;
	u_char	*cursor;
	u_char	font_lo, font_hi;
	short	rows, cols;
	short   cpl;
	short	ftheight, ftwidth;
	short   attribute;
	u_char	*attrbuf;
	short	planemask;
	short	pos;
	char	imode, fpd, hold;
	u_char  escape;
	/* not currently used, but maintained */
	char	GL, GR, G0, G1, G2, G3;
	char	linefeed_newline;
	char	argbuf[ARGBUF_SIZE], *ap;
	char	emul_level, eightbit_C1;
	int	top_margin, bottom_margin, inside_margins;
	short	save_curx, save_cury;
};

/* emulation levels: */
#define EMUL_VT100	1
#define EMUL_VT300_8	2
#define EMUL_VT300_7	3

/* Flags */
#define ITE_ALIVE	0x01	/* hardware exists */
#define ITE_INITED	0x02	/* device has been initialized */
#define ITE_CONSOLE	0x04	/* device can be console */
#define ITE_ISCONS	0x08	/* device is console */
#define ITE_ACTIVE	0x10	/* device is being used as ITE */
#define ITE_INGRF	0x20	/* device in use as non-ITE */

/* Types - indices into itesw */
#define ITE_CUSTOMCHIPS	0
#define ITE_TIGA_A2410	1

#define attrloc(ip, y, x) \
	(ip->attrbuf + ((y) * ip->cols) + (x))

#define attrclr(ip, sy, sx, h, w) \
	bzero(ip->attrbuf + ((sy) * ip->cols) + (sx), (h) * (w))
  
#define attrmov(ip, sy, sx, dy, dx, h, w) \
	bcopy(ip->attrbuf + ((sy) * ip->cols) + (sx), \
	      ip->attrbuf + ((dy) * ip->cols) + (dx), \
	      (h) * (w))

#define attrtest(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) & attr)

#define attrset(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) = attr)
  
/*
 * X and Y location of character 'c' in the framebuffer, in pixels.
 */
#define	charX(ip,c)	\
	(((c) % (ip)->cpl) * (ip)->ftwidth + (ip)->fontx)

#define	charY(ip,c)	\
	(((c) / (ip)->cpl) * (ip)->ftheight + (ip)->fonty)

/* Character attributes */
#define ATTR_NOR        0x0             /* normal */
#define	ATTR_INV	0x1		/* inverse */
#define	ATTR_UL		0x2		/* underline */
#define ATTR_BOLD	0x4		/* bold */
#define ATTR_BLINK	0x8		/* blink */
#define ATTR_ALL	(ATTR_INV | ATTR_UL)

/* Keyboard attributes */
#define ATTR_KPAD	0x80		/* keypad transmit */
  
/* Replacement Rules */
#define RR_CLEAR		0x0
#define RR_COPY			0x3
#define RR_XOR			0x6
#define RR_COPYINVERTED  	0xc

#define SCROLL_UP	0x01
#define SCROLL_DOWN	0x02
#define SCROLL_LEFT	0x03
#define SCROLL_RIGHT	0x04
#define DRAW_CURSOR	0x05
#define ERASE_CURSOR    0x06
#define MOVE_CURSOR	0x07


/* special key codes */
#define KBD_LEFT_SHIFT	0x60
#define KBD_RIGHT_SHIFT	0x61
#define KBD_CAPS_LOCK	0x62
#define KBD_CTRL	0x63
#define KBD_LEFT_ALT	0x64
#define KBD_RIGHT_ALT	0x65
#define KBD_LEFT_META	0x66
#define KBD_RIGHT_META	0x67

/* modifier map for use in itefilter() */
#define KBD_MOD_LSHIFT	(1<<0)
#define KBD_MOD_RSHIFT	(1<<1)
#define KBD_MOD_SHIFT	(KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
#define KBD_MOD_CTRL	(1<<2)
#define KBD_MOD_LALT	(1<<3)
#define KBD_MOD_RALT	(1<<4)
#define KBD_MOD_ALT	(KBD_MOD_LALT | KBD_MOD_RALT)
#define KBD_MOD_LMETA	(1<<5)
#define KBD_MOD_RMETA	(1<<6)
#define KBD_MOD_META	(KBD_MOD_LMETA | KBD_MOD_RMETA)
#define KBD_MOD_CAPS	(1<<7)

/* type for the second argument to itefilter(). Note that the 
   driver doesn't support key-repeat for console-mode, since it can't use
   timeout() for polled I/O. */
   
enum caller { ITEFILT_TTY, ITEFILT_CONSOLE, ITEFILT_REPEATER };

#define	TABSIZE		8
#define	TABEND(u)	(ite_tty[u]->t_winsize.ws_col - TABSIZE)

#ifdef KERNEL
extern	struct ite_softc ite_softc[];
#endif
