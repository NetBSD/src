/*	$NetBSD: ite_gb.c,v 1.6.106.2 2011/02/17 11:59:41 bouyer Exp $	*/

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
 * from: Utah $Hdr: ite_gb.c 1.9 92/01/20$
 *
 *	@(#)ite_gb.c	8.1 (Berkeley) 6/10/93
 */

#ifdef ITECONSOLE

#include <sys/param.h>

#include <hp300/stand/common/itereg.h>
#include <hp300/stand/common/grf_gbreg.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/itevar.h>

void gbox_windowmove(struct ite_data *, int, int, int, int, int, int, int);

void
gbox_init(struct ite_data *ip)
{
	struct gboxfb *regbase = (void *)ip->regbase;

	ip->bmv = gbox_windowmove;

	regbase->write_protect = 0x0;
	regbase->interrupt = 0x4;
	regbase->rep_rule = RR_COPY;
	regbase->blink1 = 0xff;
	regbase->blink2 = 0xff;
	regbase->sec_interrupt = 0x01;

	/*
	 * Set up the color map entries. We use three entries in the
	 * color map. The first, is for black, the second is for
	 * white, and the very last entry is for the inverted cursor.
	 */
	regbase->creg_select = 0x00;
	regbase->cmap_red    = 0x00;
	regbase->cmap_grn    = 0x00;
	regbase->cmap_blu    = 0x00;
	regbase->cmap_write  = 0x00;
	gbcm_waitbusy(regbase);

	regbase->creg_select = 0x01;
	regbase->cmap_red    = 0xFF;
	regbase->cmap_grn    = 0xFF;
	regbase->cmap_blu    = 0xFF;
	regbase->cmap_write  = 0x01;
	gbcm_waitbusy(regbase);

	regbase->creg_select = 0xFF;
	regbase->cmap_red    = 0xFF;
	regbase->cmap_grn    = 0xFF;
	regbase->cmap_blu    = 0xFF;
	regbase->cmap_write  = 0x01;
	gbcm_waitbusy(regbase);

	ite_fontinfo(ip);
	ite_fontinit8bpp(ip);

	/*
	 * Clear the display. This used to be before the font unpacking
	 * but it crashes. Figure it out later.
	 */
	gbox_windowmove(ip, 0, 0, 0, 0, ip->dheight, ip->dwidth, RR_CLEAR);
	tile_mover_waitbusy(regbase);

	/*
	 * Stash the inverted cursor.
	 */
	gbox_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			ip->cblanky, ip->cblankx, ip->ftheight,
			ip->ftwidth, RR_COPYINVERTED);
}

void
gbox_scroll(struct ite_data *ip)
{
	struct gboxfb *regbase = (void *)ip->regbase;

	tile_mover_waitbusy(regbase);
	regbase->write_protect = 0x0;

	ite_dio_scroll(ip);
}

void
gbox_windowmove(struct ite_data *ip, int sy, int sx, int dy, int dx, int h,
    int w, int mask)
{
	struct gboxfb *regbase = (void *)ip->regbase;
	int src, dest;

	src  = (sy * 1024) + sx;	/* upper left corner in pixels */
	dest = (dy * 1024) + dx;

	tile_mover_waitbusy(regbase);
	regbase->width = -(w / 4);
	regbase->height = -(h / 4);
	if (src < dest)
		regbase->rep_rule = MOVE_DOWN_RIGHT|mask;
	else {
		regbase->rep_rule = MOVE_UP_LEFT|mask;
		/*
		 * Adjust to top of lower right tile of the block.
		 */
		src = src + ((h - 4) * 1024) + (w - 4);
		dest= dest + ((h - 4) * 1024) + (w - 4);
	}
	FBBASE[dest] = FBBASE[src];
}
#endif
