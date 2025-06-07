/*	$NetBSD: ite.c,v 1.23 2025/05/30 19:19:26 tsutsui Exp $	*/

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

#include <hp300/dev/diofbreg.h>
#include <hp300/dev/intioreg.h>
#include <hp300/dev/dioreg.h>
#include <hp300/dev/sgcreg.h>
#include <dev/ic/stireg.h>

#include <hp300/stand/common/device.h>
#include <hp300/stand/common/itevar.h>
#include <hp300/stand/common/kbdvar.h>
#include <hp300/stand/common/consdefs.h>
#include <hp300/stand/common/samachdep.h>

static void iteconfig(void);
static void ite_scroll(struct ite_data *);
static void itecheckwrap(struct ite_data *);

#define GID_STI		0x100	/* any value which is not a DIO fb, really */

static const struct itesw itesw[] = {
	{
		.ite_hwid   = GID_TOPCAT,
		.ite_probe  = NULL,
		.ite_init   = topcat_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_GATORBOX,
		.ite_probe  = NULL,
		.ite_init   = gbox_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = gbox_scroll
	},

	{
		.ite_hwid   = GID_RENAISSANCE,
		.ite_probe  = NULL,
		.ite_init   = rbox_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_LRCATSEYE,
		.ite_probe  = NULL,
		.ite_init   = topcat_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_HRCCATSEYE,
		.ite_probe  = NULL,
		.ite_init   = topcat_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_HRMCATSEYE,
		.ite_probe  = NULL,
		.ite_init   = topcat_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_DAVINCI,
		.ite_probe  = NULL,
		.ite_init   = dvbox_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc8bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_HYPERION,
		.ite_probe  = NULL,
		.ite_init   = hyper_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc1bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_TIGER,
		.ite_probe  = NULL,
		.ite_init   = tvrx_init,
		.ite_clear  = ite_dio_clear,
		.ite_putc   = ite_dio_putc1bpp,
		.ite_cursor = ite_dio_cursor,
		.ite_scroll = ite_dio_scroll
	},

	{
		.ite_hwid   = GID_A1474MID,
		.ite_probe  = sti_dio_probe,
		.ite_init   = sti_iteinit_dio,
		.ite_clear  = sti_clear,
		.ite_putc   = sti_putc,
		.ite_cursor = sti_cursor,
		.ite_scroll = sti_scroll
	},

	{
		.ite_hwid   = GID_A147xVGA,
		.ite_probe  = sti_dio_probe,
		.ite_init   = sti_iteinit_dio,
		.ite_clear  = sti_clear,
		.ite_putc   = sti_putc,
		.ite_cursor = sti_cursor,
		.ite_scroll = sti_scroll
	},

	{
		.ite_hwid   = GID_STI,
		.ite_probe  = NULL,
		.ite_init   = sti_iteinit_sgc,
		.ite_clear  = sti_clear,
		.ite_putc   = sti_putc,
		.ite_cursor = sti_cursor,
		.ite_scroll = sti_scroll
	},
};

/* these guys need to be in initialized data */
static int itecons = -1;
static struct  ite_data ite_data[NITE] = { { 0 } };

/*
 * Locate all bitmapped displays
 */
static void
iteconfig(void)
{
	int dtype, fboff, slotno, i;
	uint8_t *va;
	struct hp_hw *hw;
	struct diofbreg *fb;
	struct ite_data *ip;

	i = 0;
	for (hw = sc_table; hw < &sc_table[MAXCTLRS]; hw++) {
	        if (!HW_ISDEV(hw, D_BITMAP))
			continue;
		fb = (struct diofbreg *)hw->hw_kva;
		/* XXX: redundent but safe */
		if (badaddr((void *)fb) || fb->id != GRFHWID)
			continue;
		for (dtype = 0; dtype < __arraycount(itesw); dtype++)
			if (itesw[dtype].ite_hwid == fb->fbid)
				break;
		if (dtype == __arraycount(itesw))
			continue;
		if (i >= NITE)
			break;
		ip = &ite_data[i];
		ip->scode = hw->hw_sc;
		ip->isw = &itesw[dtype];
		ip->regbase = (void *)fb;
		fboff = (fb->fbomsb << 8) | fb->fbolsb;
		ip->fbbase = (void *)(*((uint8_t *)ip->regbase + fboff) << 16);
		/* DIO II: FB offset is relative to select code space */
		if (DIO_ISDIOII(ip->scode))
			ip->fbbase = (uint8_t *)ip->fbbase + (int)ip->regbase;
		ip->fbwidth  = fb->fbwmsb << 8 | fb->fbwlsb;
		ip->fbheight = fb->fbhmsb << 8 | fb->fbhlsb;
		ip->dwidth   = fb->dwmsb  << 8 | fb->dwlsb;
		ip->dheight  = fb->dhmsb  << 8 | fb->dhlsb;
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
		/* confirm hardware is what we think it is */
		if (itesw[dtype].ite_probe != NULL &&
		    (*itesw[dtype].ite_probe)(ip) != 0)
			continue;
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
	for (dtype = 0; dtype < __arraycount(itesw); dtype++)
		if (itesw[dtype].ite_hwid == GID_STI)
			break;
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
	whichconsole = (whichconsole + 1) % (NITE + 1);
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
	ip = &ite_data[unit];
	curcons_scode = ip->scode;
	cp->cn_dev = unit;
	cp->cn_pri = pri;
}

void
iteinit(struct consdev *cp)
{
	int ite = cp->cn_dev;
	struct ite_data *ip;
	const struct itesw *sp;

	if (itecons != -1)
		return;

	ip = &ite_data[ite];
	sp = ip->isw;

	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;

	(*sp->ite_init)(ip);
	(*sp->ite_cursor)(ip, DRAW_CURSOR);

	itecons = ite;
	kbdinit();
}

void
iteputchar(dev_t dev, int c)
{
	struct ite_data *ip = &ite_data[itecons];
	const struct itesw *sp = ip->isw;

	c &= 0x7F;
	switch (c) {

	case '\n':
		if (++ip->cury == ip->rows) {
			ip->cury--;
			ite_scroll(ip);
		} else
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
		itecheckwrap(ip);
		break;
	}
}

static void
itecheckwrap(struct ite_data *ip)
{
	const struct itesw *sp = ip->isw;

	if (++ip->curx == ip->cols) {
		ip->curx = 0;
		if (++ip->cury == ip->rows) {
			--ip->cury;
			ite_scroll(ip);
			return;
		}
	}
	(*sp->ite_cursor)(ip, MOVE_CURSOR);
}

static void
ite_scroll(struct ite_data *ip)
{
	const struct itesw *sp = ip->isw;

	/* Erase the cursor before scrolling */
	(*sp->ite_cursor)(ip, ERASE_CURSOR);
	/* Scroll the screen up by one line */
	(*sp->ite_scroll)(ip);
	/* Clear the entire bottom line after scrolling */
	(*sp->ite_clear)(ip, ip->rows - 1, 0, 1, ip->cols);
	/* Redraw the cursor */
	(*sp->ite_cursor)(ip, DRAW_CURSOR);
}

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
