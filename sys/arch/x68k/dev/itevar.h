/*	$NetBSD: itevar.h,v 1.7.8.1 2002/05/19 08:02:51 gehenna Exp $	*/

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

struct ite_softc;

struct itesw {
	int	(*ite_cnprobe) __P((int minor));
	void	(*ite_init) __P((struct ite_softc *));
	void	(*ite_deinit) __P((struct ite_softc *));
	void	(*ite_clear) __P((struct ite_softc *,int,int,int,int));
	void	(*ite_putc) __P((struct ite_softc *,int,int,int,int));
	void	(*ite_cursor) __P((struct ite_softc *,int));
	void	(*ite_scroll) __P((struct ite_softc *,int,int,int,int));
};

enum ite_arraymaxs {
	MAX_ARGSIZE = 256,
	MAX_TABS = 256,
};

/* maximum number of argument characters (<CSI>33;34;3m for example) */
#define ARGBUF_SIZE 256

struct ite_softc {
	struct	device device;
	struct grf_softc *grf;
	struct	itesw *isw;
	int	flags;
	int	type;
	int	open_cnt;
	void	*priv;
	short	curx, cury;
	short   cursorx, cursory;
	u_char	*font;
	u_char	*cursor;
	u_char	font_lo, font_hi;
	short	rows, cols;
	short   cpl;
	short	ftheight, ftwidth, ftbaseline, ftboldsmear;
	short   attribute;
	u_char	*attrbuf;
	short	planemask;
	short	pos;
	char	imode, fpd, hold;
	u_char  escape, cursor_opt, key_repeat;
	char	*GL, *GR, *save_GL;
	char	G0, G1, G2, G3;
	char	fgcolor, bgcolor;
	char	linefeed_newline, auto_wrap;
	char	cursor_appmode, keypad_appmode;
	char	argbuf[ARGBUF_SIZE], *ap, *tabs;
	char	emul_level, eightbit_C1;
	int	top_margin, bottom_margin;
	char	inside_margins, sc_om;
	short	save_curx, save_cury, save_attribute, save_char;
	char	sc_G0, sc_G1, sc_G2, sc_G3;
	char	*sc_GL, *sc_GR;
};

enum emul_level {
	EMUL_VT100 = 1,
	EMUL_VT300_8,
	EMUL_VT300_7
};

/* Flags */
#define ITE_ALIVE	0x01	/* hardware exists */
#define ITE_INITED	0x02	/* device has been initialized */
#define ITE_CONSOLE	0x04	/* device can be console */
#define ITE_ISCONS	0x08	/* device is console */
#define ITE_ACTIVE	0x10	/* device is being used as ITE */
#define ITE_INGRF	0x20	/* device in use as non-ITE */

#ifdef DO_WEIRD_ATTRIBUTES
#define attrloc(ip, y, x) \
	(ip->attrbuf + ((y) * ip->cols) + (x))

#define attrclr(ip, sy, sx, h, w) \
	memset(ip->attrbuf + ((sy) * ip->cols) + (sx), 0, (h) * (w))
  
#define attrmov(ip, sy, sx, dy, dx, h, w) \
	memcpy(ip->attrbuf + ((dy) * ip->cols) + (dx), \
	      ip->attrbuf + ((sy) * ip->cols) + (sx), \
	      (h) * (w))

#define attrtest(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) & attr)

#define attrset(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) = attr)
#else
#define attrloc(ip, y, x) 0
#define attrclr(ip, sy, sx, h, w)
#define attrmov(ip, sy, sx, dy, dx, h, w)
#define attrtest(ip, attr) 0
#define attrset(ip, attr)
#endif

  
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
#define ATTR_ALL	(ATTR_INV | ATTR_UL|ATTR_BOLD|ATTR_BLINK)

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
#define START_CURSOROPT	0x08	/* at start of output. May disable cursor */
#define END_CURSOROPT	0x09	/* at end, make sure cursor state is ok */

/* special key codes */
#define KBD_LEFT_SHIFT	0x70
#define KBD_RIGHT_SHIFT	0x70
#define KBD_CAPS_LOCK	0x5d
#define KBD_CTRL	0x71
#define KBD_LEFT_ALT	0x55
#define KBD_RIGHT_ALT	0x58
#define KBD_LEFT_META	0x56
#define KBD_RIGHT_META	0x57
#define KBD_OPT1	0x72
#define KBD_OPT2	0x73
#define KBD_RECONNECT	0x7f

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
#define KBD_MOD_OPT1	(1<<8)
#define KBD_MOD_OPT2	(1<<9)

/* type for the second argument to itefilter(). Note that the 
   driver doesn't support key-repeat for console-mode, since it can't use
   timeout() for polled I/O. */
   
enum tab_size { TABSIZE = 8 };
#define TABEND(u) (ite_tty[u]->t_windsize.ws_col - TABSIZE) /* XXX */

#define set_attr(ip, attr)	((ip)->attribute |= (attr))
#define clr_attr(ip, attr)	((ip)->attribute &= ~(attr))
#define attrloc(ip, y, x) 0
#define attrclr(ip, sy, sx, h, w)
#define attrmov(ip, sy, sx, dy, dx, h, w)
#define attrtest(ip, attr) 0
#define attrset(ip, attr)

/* character set */
#define CSET_MULTI	0x80 /* multibytes flag */
#define CSET_ASCII	0 /* ascii */
#define CSET_JISROMA	1 /* iso2022jp romaji */
#define CSET_JISKANA	2 /* iso2022jp kana */
#define CSET_JIS1978	(3|CSET_MULTI) /* iso2022jp old jis kanji */
#define CSET_JIS1983	(4|CSET_MULTI) /* iso2022jp new jis kanji */
#define CSET_JIS1990	(5|CSET_MULTI) /* iso2022jp hojo kanji */

struct consdev;

/* console related function */
void	itecnprobe __P((struct consdev *));
void	itecninit __P((struct consdev *));
int	itecngetc __P((dev_t));
void	itecnputc __P((dev_t, int));
void	itecnfinish __P((struct ite_softc *));

/* standard ite device entry points. */
void	iteinit __P((dev_t));
void	itestart __P((struct tty *));

/* ite functions */
int	iteon __P((dev_t, int));
void	iteoff __P((dev_t, int));
void	ite_reinit __P((dev_t));
void	ite_reset __P((struct ite_softc *));
int	ite_cnfilter __P((u_char));
void	ite_filter __P((u_char));

/* lower layer functions */
void	tv_init __P((struct ite_softc *));
void	tv_deinit __P((struct ite_softc *));

#ifdef _KERNEL
extern unsigned char kern_font[];

/* keyboard LED status variable */
extern unsigned char kbdled;
void ite_set_glyph __P((void));
void kbd_setLED __P((void));
#endif
