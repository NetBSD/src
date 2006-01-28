/*	$NetBSD: ite_dv.c,v 1.6 2006/01/28 12:00:56 tsutsui Exp $	*/

/*
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
 * from: Utah $Hdr: ite_dv.c 1.2 92/01/20$
 *
 *	@(#)ite_dv.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: ite_dv.c 1.2 92/01/20$
 *
 *	@(#)ite_dv.c	8.1 (Berkeley) 6/10/93
 */

#ifdef ITECONSOLE

#include <sys/param.h>

#include <hp300/dev/itereg.h>
#include <hp300/dev/grf_dvreg.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/itevar.h>

#define WINDOWMOVER	dvbox_windowmove

static void dv_reset(struct dvboxfb *);
static void dvbox_windowmove(struct ite_data *, int, int, int, int,
    int, int, int);

void
dvbox_init(struct ite_data *ip)
{
	struct dvboxfb *regbase;
	int i;
	
	regbase = (void *)ip->regbase;
	dv_reset(regbase);
	DELAY(4000);

	/*
	 * Turn on frame buffer, turn on overlay planes, set replacement
	 * rule, enable top overlay plane writes for ite, disable all frame
	 * buffer planes, set byte per pixel, and display frame buffer 0.
	 * Lastly, turn on the box.
	 */
	regbase->interrupt = 0x04;
	regbase->drive     = 0x10;		
	regbase->rep_rule  = RR_COPY << 4 | RR_COPY;
	regbase->opwen     = 0x01;
	regbase->fbwen     = 0x0;
	regbase->fold      = 0x01;
	regbase->vdrive    = 0x0;
	regbase->dispen    = 0x01;

	/*
	 * Video enable top overlay plane.
	 */
	regbase->opvenp = 0x01;
	regbase->opvens = 0x01;

	/*
	 * Make sure that overlay planes override frame buffer planes.
	 */
	regbase->ovly0p  = 0x0;
	regbase->ovly0s  = 0x0;
	regbase->ovly1p  = 0x0;
	regbase->ovly1s  = 0x0;
	regbase->fv_trig = 0x1;
	DELAY(400);

	/*
	 * Setup the overlay colormaps. Need to set the 0,1 (black/white)
	 * color for both banks.
	 */

	for (i = 0; i <= 1; i++) {
		regbase->cmapbank = i;
		regbase->rgb[0].red   = 0x00;
		regbase->rgb[0].green = 0x00;
		regbase->rgb[0].blue  = 0x00;
		regbase->rgb[1].red   = 0xFF;
		regbase->rgb[1].green = 0xFF;
		regbase->rgb[1].blue  = 0xFF;
	}

	regbase->cmapbank = 0;
	
	db_waitbusy(regbase);

	ite_fontinfo(ip);
	ite_fontinit(ip);

	/*
	 * Clear the (visible) framebuffer.
	 */
	dvbox_windowmove(ip, 0, 0, 0, 0, ip->dheight, ip->dwidth, RR_CLEAR);
	db_waitbusy(regbase);

	/*
	 * Stash the inverted cursor.
	 */
	dvbox_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			 ip->cblanky, ip->cblankx, ip->ftheight,
			 ip->ftwidth, RR_COPYINVERTED);
	db_waitbusy(regbase);
}

void
dvbox_putc(struct ite_data *ip, int c, int dy, int dx, int mode)
{

	dvbox_windowmove(ip, charY(ip, c), charX(ip, c),
			 dy * ip->ftheight, dx * ip->ftwidth,
			 ip->ftheight, ip->ftwidth, RR_COPY);
}

void
dvbox_cursor(struct ite_data *ip, int flag)
{
	if (flag == DRAW_CURSOR)
		draw_cursor(ip)
	else if (flag == MOVE_CURSOR) {
		erase_cursor(ip)
		draw_cursor(ip)
	}
	else
		erase_cursor(ip)
}

void
dvbox_clear(struct ite_data *ip, int sy, int sx, int h, int w)
{

	dvbox_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			 sy * ip->ftheight, sx * ip->ftwidth,
			 h  * ip->ftheight, w  * ip->ftwidth,
			 RR_CLEAR);
}

void
dvbox_scroll(struct ite_data *ip, int sy, int sx, int count, int dir)
{
	int dy = sy - count;
	int height = ip->rows - sy;

	dvbox_cursor(ip, ERASE_CURSOR);

	dvbox_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			 dy * ip->ftheight, sx * ip->ftwidth,
			 height * ip->ftheight,
			 ip->cols * ip->ftwidth, RR_COPY);
}

void
dvbox_windowmove(struct ite_data *ip, int sy, int sx, int dy, int dx,
    int h, int w, int func)
{
	struct dvboxfb *dp = (void *)ip->regbase;

	if (h == 0 || w == 0)
		return;
	
	db_waitbusy(dp);
	dp->rep_rule = func << 4 | func;
	dp->source_y = sy;
	dp->source_x = sx;
	dp->dest_y   = dy;
	dp->dest_x   = dx;
	dp->wheight  = h;
	dp->wwidth   = w;
	dp->wmove    = 1;
}

static void
dv_reset(struct dvboxfb *dbp)
{

	dbp->reset = 0x80;
	DELAY(400);

	dbp->interrupt = 0x04;
	dbp->en_scan   = 0x01;
	dbp->fbwen     = ~0;
	dbp->opwen     = ~0;
	dbp->fold      = 0x01;
	dbp->drive     = 0x01;
	dbp->rep_rule  = 0x33;
	dbp->alt_rr    = 0x33;
	dbp->zrr       = 0x33;

	dbp->fbvenp    = 0xFF;
	dbp->dispen    = 0x01;
	dbp->fbvens    = 0x0;
	dbp->fv_trig   = 0x01;
	DELAY(400);
	dbp->vdrive    = 0x0;
	dbp->zconfig   = 0x0;

	while (dbp->wbusy & 0x01)
		DELAY(400);

	/*
	 * Start of missing ROM code.
	 */
	dbp->cmapbank = 0;

	dbp->red0   = 0;
	dbp->red1   = 0;
	dbp->green0 = 0;
	dbp->green1 = 0;
	dbp->blue0  = 0;
	dbp->blue1  = 0;

	dbp->panxh   = 0;
	dbp->panxl   = 0;
	dbp->panyh   = 0;
	dbp->panyl   = 0;
	dbp->zoom    = 0;
	dbp->cdwidth = 0x50;
	dbp->chstart = 0x52;
	dbp->cvwidth = 0x22;
	dbp->pz_trig = 1;
	/*
	 * End of missing ROM code.
	 */
}

#endif
