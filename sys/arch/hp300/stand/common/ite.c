/*	$NetBSD: ite.c,v 1.16.28.1 2014/08/10 06:53:58 tls Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: ite.c 1.24 93/06/25$
 *
 *	@(#)ite.c	8.1 (Berkeley) 7/8/93
 */

/*
 * Standalone Internal Terminal Emulator (CRT and keyboard)
 */

#ifdef ITECONSOLE

#include <sys/param.h>
#include <dev/cons.h>

#include <hp300/stand/common/grfreg.h>
#include <hp300/dev/intioreg.h>
#include <hp300/dev/sgcreg.h>
#include <dev/ic/stireg.h>

#include <hp300/stand/common/device.h>
#include <hp300/stand/common/itevar.h>
#include <hp300/stand/common/kbdvar.h>
#include <hp300/stand/common/consdefs.h>
#include <hp300/stand/common/samachdep.h>

static void iteconfig(void);
static void ite_clrtoeol(struct ite_data *, struct itesw *, int, int);
static void itecheckwrap(struct ite_data *, struct itesw *);

#define GID_STI		0x100	/* any value which is not a DIO fb, really */

struct itesw itesw[] = {
	{ GID_TOPCAT,
	topcat_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_GATORBOX,
	gbox_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	gbox_scroll },

	{ GID_RENAISSANCE,
	rbox_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_LRCATSEYE,
	topcat_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_HRCCATSEYE,
	topcat_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_HRMCATSEYE,
	topcat_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_DAVINCI,
      	dvbox_init,	ite_dio_clear,	ite_dio_putc8bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_HYPERION,
	hyper_init,	ite_dio_clear,	ite_dio_putc1bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_TIGER,
	tvrx_init,	ite_dio_clear,	ite_dio_putc1bpp,
	ite_dio_cursor,	ite_dio_scroll },

	{ GID_A1474MID,
	dumb_init,	dumb_clear,	dumb_putc,
	dumb_cursor,	dumb_scroll },

	{ GID_A147xVGA,
	dumb_init,	dumb_clear,	dumb_putc,
	dumb_cursor,	dumb_scroll },

	{ GID_STI,
	sti_iteinit_sgc, sti_clear,	sti_putc,
	sti_cursor,	sti_scroll },
};
int	nitesw = sizeof(itesw) / sizeof(itesw[0]);

/* these guys need to be in initialized data */
int itecons = -1;
struct  ite_data ite_data[NITE] = { { 0 } };
int	ite_scode[NITE] = { 0 };

/*
 * Locate all bitmapped displays
 */
static void
iteconfig(void)
{
	int dtype, fboff, slotno, i;
	uint8_t *va;
	struct hp_hw *hw;
	struct grfreg *gr;
	struct ite_data *ip;

	i = 0;
	for (hw = sc_table; hw < &sc_table[MAXCTLRS]; hw++) {
	        if (!HW_ISDEV(hw, D_BITMAP))
			continue;
		gr = (struct grfreg *) hw->hw_kva;
		/* XXX: redundent but safe */
		if (badaddr((void *)gr) || gr->gr_id != GRFHWID)
			continue;
		for (dtype = 0; dtype < nitesw; dtype++)
			if (itesw[dtype].ite_hwid == gr->gr_id2)
				break;
		if (dtype == nitesw)
			continue;
		if (i >= NITE)
			break;
		ite_scode[i] = hw->hw_sc;
		ip = &ite_data[i];
		ip->isw = &itesw[dtype];
		ip->regbase = (void *) gr;
		fboff = (gr->gr_fbomsb << 8) | gr->gr_fbolsb;
		ip->fbbase = (void *)(*((u_char *)ip->regbase + fboff) << 16);
		/* DIO II: FB offset is relative to select code space */
		if (ip->regbase >= (void *)DIOIIBASE)
			ip->fbbase = (char*)ip->fbbase + (int)ip->regbase;
		ip->fbwidth  = gr->gr_fbwidth_h << 8 | gr->gr_fbwidth_l;
		ip->fbheight = gr->gr_fbheight_h << 8 | gr->gr_fbheight_l;
		ip->dwidth   = gr->gr_dwidth_h << 8 | gr->gr_dwidth_l;
		ip->dheight  = gr->gr_dheight_h << 8 | gr->gr_dheight_l;
		/*
		 * XXX some displays (e.g. the davinci) appear
		 * to return a display height greater than the
		 * returned FB height.  Guess we should go back
		 * to getting the display dimensions from the
		 * fontrom...
		 */
		if (ip->dwidth > ip->fbwidth)
			ip->dwidth = ip->fbwidth;
		if (ip->dheight > ip->fbheight)
			ip->dheight = ip->fbheight;
		ip->alive = 1;
		i++;
	}

	/*
	 * Now probe for SGC frame buffers.
	 */
	switch (machineid) {
	case HP_400:
	case HP_425:
	case HP_433:
		break;
	default:
		return;
	}

	/* SGC frame buffers can only be STI... */
	for (dtype = 0; dtype < __arraycount(itesw); dtype++) {
		if (itesw[dtype].ite_hwid == GID_STI)
			break;
	}
	if (dtype == __arraycount(itesw))
		return;

	for (slotno = 0; slotno < SGC_NSLOTS; slotno++) {
		va = (uint8_t *)IIOV(SGC_BASE + (slotno * SGC_DEVSIZE));

		/* Check to see if hardware exists. */
		if (badaddr(va) != 0)
			continue;

		/* Check hardware. */
		if (va[3] == STI_DEVTYPE1) {
			if (i >= NITE)
				break;
			ip = &ite_data[i];
			ip->scode = slotno;
			ip->isw = &itesw[dtype];
			/* to get CN_MIDPRI */
			ip->regbase = (uint8_t *)(INTIOBASE + FB_BASE);
			/* ...and do not need an ite_probe() check */
			ip->alive = 1;
			i++;
			/* we only support one SGC frame buffer at the moment */
			break;
		}
	}
}

#ifdef CONSDEBUG
/*
 * Allows us to cycle through all possible consoles (NITE ites and serial port)
 * by using SHIFT-RESET on the keyboard.
 */
int	whichconsole = -1;
#endif

void
iteprobe(struct consdev *cp)
{
	int ite;
	struct ite_data *ip;
	int unit, pri;

#ifdef CONSDEBUG
	whichconsole = ++whichconsole % (NITE+1);
#endif

	if (itecons != -1)
		return;

	iteconfig();
	unit = -1;
	pri = CN_DEAD;
	for (ite = 0; ite < NITE; ite++) {
#ifdef CONSDEBUG
		if (ite < whichconsole)
			continue;
#endif
		ip = &ite_data[ite];
		if (ip->alive == 0)
			continue;
		if ((int)ip->regbase == INTIOBASE + FB_BASE) {
			pri = CN_INTERNAL;
			unit = ite;
		} else if (unit < 0) {
			pri = CN_NORMAL;
			unit = ite;
		}
	}
	curcons_scode = ite_scode[unit];
	cp->cn_dev = unit;
	cp->cn_pri = pri;
}

void
iteinit(struct consdev *cp)
{
	int ite = cp->cn_dev;
	struct ite_data *ip;

	if (itecons != -1)
		return;

	ip = &ite_data[ite];

	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;

	(*ip->isw->ite_init)(ip);
	(*ip->isw->ite_cursor)(ip, DRAW_CURSOR);

	itecons = ite;
	kbdinit();
}

/* ARGSUSED */
void
iteputchar(dev_t dev, int c)
{
	struct ite_data *ip = &ite_data[itecons];
	struct itesw *sp = ip->isw;

	c &= 0x7F;
	switch (c) {

	case '\n':
		if (++ip->cury == ip->rows) {
			ip->cury--;
			(*sp->ite_scroll)(ip);
			ite_clrtoeol(ip, sp, ip->cury, 0);
		}
		else
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
		break;

	case '\r':
		ip->curx = 0;
		(*sp->ite_cursor)(ip, MOVE_CURSOR);
		break;

	case '\b':
		if (--ip->curx < 0)
			ip->curx = 0;
		else
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
		break;

	default:
		if (c < ' ' || c == 0177)
			break;
		(*sp->ite_putc)(ip, c, ip->cury, ip->curx);
		(*sp->ite_cursor)(ip, DRAW_CURSOR);
		itecheckwrap(ip, sp);
		break;
	}
}

static void
itecheckwrap(struct ite_data *ip, struct itesw *sp)
{
	if (++ip->curx == ip->cols) {
		ip->curx = 0;
		if (++ip->cury == ip->rows) {
			--ip->cury;
			(*sp->ite_scroll)(ip);
			ite_clrtoeol(ip, sp, ip->cury, 0);
			return;
		}
	}
	(*sp->ite_cursor)(ip, MOVE_CURSOR);
}

static void
ite_clrtoeol(struct ite_data *ip, struct itesw *sp, int y, int x)
{

	(*sp->ite_clear)(ip, y, x, 1, ip->cols - x);
	(*sp->ite_cursor)(ip, DRAW_CURSOR);
}

/* ARGSUSED */
int
itegetchar(dev_t dev)
{

#ifdef SMALL
	return 0;
#else
	return kbdgetc();
#endif
}
#endif
