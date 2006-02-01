/*	$NetBSD: ite_tc.c,v 1.5.2.1 2006/02/01 14:51:27 yamt Exp $	*/

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
 * from: Utah $Hdr: ite_tc.c 1.11 92/01/20$
 *
 *	@(#)ite_tc.c	8.1 (Berkeley) 6/10/93
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
 * from: Utah $Hdr: ite_tc.c 1.11 92/01/20$
 *
 *	@(#)ite_tc.c	8.1 (Berkeley) 6/10/93
 */

#ifdef ITECONSOLE

#include <sys/param.h>

#include <hp300/dev/itereg.h>
#include <hp300/dev/grfreg.h>
#include <hp300/dev/grf_tcreg.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/itevar.h>

#define WINDOWMOVER 	topcat_windowmove

void topcat_windowmove(struct ite_data *, int, int, int, int, int, int, int);

void
topcat_init(struct ite_data *ip)
{
	struct tcboxfb *regbase = (void *)ip->regbase;

	/*
	 * Catseye looks a lot like a topcat, but not completely.
	 * So, we set some bits to make it work.
	 */
	if (regbase->fbid != GID_TOPCAT) {
		while ((regbase->catseye_status & 1))
			;
		regbase->catseye_status = 0x0;
		regbase->vb_select      = 0x0;
		regbase->tcntrl         = 0x0;
		regbase->acntrl         = 0x0;
		regbase->pncntrl        = 0x0;
		regbase->rug_cmdstat    = 0x90;
	}

	/*
	 * Determine the number of planes by writing to the first frame
	 * buffer display location, then reading it back. 
	 */
	regbase->wen = ~0;
	regbase->fben = ~0;
	regbase->prr = RR_COPY;
	*FBBASE = 0xFF;
	ip->planemask = *FBBASE;

	/*
	 * Enable reading/writing of all the planes.
	 */
	regbase->fben = ip->planemask;
	regbase->wen  = ip->planemask;
	regbase->ren  = ip->planemask;
	regbase->prr  = RR_COPY;

	ite_fontinfo(ip);

	/*
	 * Clear the framebuffer on all planes.
	 */
	topcat_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);
	tc_waitbusy(regbase, ip->planemask);

	ite_fontinit(ip);

	/*
	 * Stash the inverted cursor.
	 */
	topcat_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			  ip->cblanky, ip->cblankx, ip->ftheight,
			  ip->ftwidth, RR_COPYINVERTED);
}

void
topcat_putc(struct ite_data *ip, int c, int dy, int dx, int mode)
{

	topcat_windowmove(ip, charY(ip, c), charX(ip, c),
			  dy * ip->ftheight, dx * ip->ftwidth,
			  ip->ftheight, ip->ftwidth, RR_COPY);
}

void
topcat_cursor(struct ite_data *ip, int flag)
{

	if (flag == DRAW_CURSOR)
		draw_cursor(ip)
	else if (flag == MOVE_CURSOR) {
		erase_cursor(ip)
		draw_cursor(ip)
	} else
		erase_cursor(ip)
}

void
topcat_clear(struct ite_data *ip, int sy, int sx, int h, int w)
{

	topcat_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			  sy * ip->ftheight, sx * ip->ftwidth, 
			  h  * ip->ftheight, w  * ip->ftwidth,
			  RR_CLEAR);
}

void
topcat_scroll(struct ite_data *ip, int sy, int sx, int count, int dir)
{
	int dy = sy - count;
	int height = ip->rows - sy;

	topcat_cursor(ip, ERASE_CURSOR);

	topcat_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			  dy * ip->ftheight, sx * ip->ftwidth,
			  height * ip->ftheight,
			  ip->cols  * ip->ftwidth, RR_COPY);
}

void
topcat_windowmove(struct ite_data *ip, int sy, int sx, int dy, int dx,
    int h, int w, int func)
{
	struct tcboxfb *rp = (void *)ip->regbase;
	
	if (h == 0 || w == 0)
		return;
	tc_waitbusy(rp, ip->planemask);
	rp->wmrr     = func;
	rp->source_y = sy;
	rp->source_x = sx;
	rp->dest_y   = dy;
	rp->dest_x   = dx;
	rp->wheight  = h;
	rp->wwidth   = w;
	rp->wmove    = ip->planemask;
}
#endif
