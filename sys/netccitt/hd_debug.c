/*	$NetBSD: hd_debug.c,v 1.7 1996/10/10 23:02:20 christos Exp $	*/

/*
 * Copyright (c) University of British Columbia, 1984
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Laboratory for Computation Vision and the Computer Science Department
 * of the University of British Columbia.
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
 *	@(#)hd_debug.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>

#include <netccitt/hdlc.h>
#include <netccitt/hd_var.h>
#include <netccitt/x25.h>

#ifdef HDLCDEBUG
#define NTRACE		32

struct hdlctrace {
	struct hdcb    *ht_hdp;
	short           ht_dir;
	struct mbuf    *ht_frame;
	struct timeval  ht_time;
}               hdtrace[NTRACE];

int             lasttracelogged, freezetrace;
#endif

void
hd_trace(hdp, direction, frame)
	struct hdcb    *hdp;
	int direction;
	register struct Hdlc_frame *frame;
{
	register char  *s;
	register int    nr, pf, ns, i;
	struct Hdlc_iframe *iframe = (struct Hdlc_iframe *) frame;

#ifdef HDLCDEBUG
	hd_savetrace(hdp, direction, frame);
#endif
	if (hdp->hd_xcp->xc_ltrace) {
		if (direction == RX)
			kprintf("F-In:  ");
		else if (direction == 2)
			kprintf("F-Xmt: ");
		else
			kprintf("F-Out:   ");

		nr = iframe->nr;
		pf = iframe->pf;
		ns = iframe->ns;

		switch (hd_decode(hdp, frame)) {
		case SABM:
			kprintf("SABM   : PF=%d\n", pf);
			break;

		case DISC:
			kprintf("DISC   : PF=%d\n", pf);
			break;

		case DM:
			kprintf("DM     : PF=%d\n", pf);
			break;

		case FRMR:
			{
				register struct Frmr_frame *f = (struct Frmr_frame *) frame;

				kprintf("FRMR   : PF=%d, TEXT=", pf);
				for (s = (char *) frame, i = 0; i < 5; ++i, ++s)
					kprintf("%x ", (int) *s & 0xff);
				kprintf("\n");
				kprintf("control=%x v(s)=%d v(r)=%d w%d x%d y%d z%d\n",
				    f->frmr_control, f->frmr_ns, f->frmr_nr,
				f->frmr_w, f->frmr_x, f->frmr_y, f->frmr_z);
				break;
			}

		case UA:
			kprintf("UA     : PF=%d\n", pf);
			break;

		case RR:
			kprintf("RR     : N(R)=%d, PF=%d\n", nr, pf);
			break;

		case RNR:
			kprintf("RNR    : N(R)=%d, PF=%d\n", nr, pf);
			break;

		case REJ:
			kprintf("REJ    : N(R)=%d, PF=%d\n", nr, pf);
			break;

		case IFRAME:
			{
				register struct mbuf *m;
				register int    len = 0;

				for (m = dtom(frame); m; m = m->m_next)
					len += m->m_len;
				len -= HDHEADERLN;
				kprintf("IFRAME : N(R)=%d, PF=%d, N(S)=%d, DATA(%d)=",
				       nr, pf, ns, len);
				for (s = (char *) iframe->i_field, i = 0; i < 3; ++i, ++s)
					kprintf("%x ", (int) *s & 0xff);
				kprintf("\n");
				break;
			}

		default:
			kprintf("ILLEGAL: ");
			for (s = (char *) frame, i = 0; i < 5; ++i, ++s)
				kprintf("%x ", (int) *s & 0xff);
			kprintf("\n");
		}

	}
}

#ifdef HDLCDEBUG
static void
hd_savetrace(hdp, dir, frame)
	struct hdcb    *hdp;
	int dir;
	struct Hdlc_frame *frame;
{
	register struct hdlctrace *htp;
	register struct mbuf *m;

	if (freezetrace)
		return;
	htp = &hdtrace[lasttracelogged];
	lasttracelogged = (lasttracelogged + 1) % NTRACE;
	if (m = htp->ht_frame)
		m_freem(m);
	m = dtom(frame);
	htp->ht_frame = m_copy(m, 0, m->m_len);
	htp->ht_hdp = hdp;
	htp->ht_dir = dir;
	htp->ht_time = time;
}

void
hd_dumptrace(hdp)
	struct hdcb    *hdp;
{
	register int    i, ltrace;
	register struct hdlctrace *htp;

	freezetrace = 1;
	hd_status(hdp);
	kprintf("retransmit queue:");
	for (i = 0; i < 8; i++)
		kprintf(" %x", hdp->hd_retxq[i]);
	kprintf("\n");
	ltrace = hdp->hd_xcp->xc_ltrace;
	hdp->hd_xcp->xc_ltrace = 1;
	for (i = 0; i < NTRACE; i++) {
		htp = &hdtrace[(lasttracelogged + i) % NTRACE];
		if (htp->ht_hdp != hdp || htp->ht_frame == 0)
			continue;
		kprintf("%d/%d	", htp->ht_time.tv_sec & 0xff,
		       htp->ht_time.tv_usec / 10000);
		hd_trace(htp->ht_hdp, htp->ht_dir,
			 mtod(htp->ht_frame, struct Hdlc_frame *));
		m_freem(htp->ht_frame);
		htp->ht_frame = 0;
	}
	hdp->hd_xcp->xc_ltrace = ltrace;
	freezetrace = 0;
}
#endif
