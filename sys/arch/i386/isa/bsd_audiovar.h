/*
 * Copyright (c) 1991-1993 Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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
 *	From: Header: audiovar.h,v 1.3 93/07/18 14:07:25 mccanne Exp  (LBL)
 *	$Id: bsd_audiovar.h,v 1.1 1994/01/09 19:35:05 cgd Exp $
 */

#define AUCB_SIZE 4096
#define AUCB_MOD(k)	((k) & (AUCB_SIZE - 1))

#define AUCB_INIT(cb)	((cb)->cb_head = (cb)->cb_tail = (cb)->cb_drops = \
			 (cb)->cb_pdrops = (cb)->cb_tdrops = (cb)->cb_cc = 0)

#define AUCB_EMPTY(cb)	((cb)->cb_head == (cb)->cb_tail)
#define AUCB_FULL(cb)	(AUCB_MOD((cb)->cb_tail + 1) == (cb)->cb_head)
#define AUCB_LEN(cb)	(AUCB_MOD((cb)->cb_tail - (cb)->cb_head))

#define MAXBLKSIZE (AUCB_SIZE / 2)
#define DEFBLKSIZE 128

#ifndef LOCORE
/*
 * aucb's are used for communication between the trap handler and
 * the software interrupt.
 */
struct aucb {
	int	cb_thresh;		/* threshold for wakeup */
	u_char	*cb_cp;
	u_char	*cb_bp;
	u_char	*cb_ep;
};

struct auio {
	u_long	au_stamp;		/* time stamp */
	int	au_lowat;		/* xmit low water mark (for wakeup) */
	int	au_hiwat;		/* xmit high water mark (for wakeup) */
	int	au_blksize;		/* recv block (chunk) size */
	int	au_backlog;		/* # samples of xmit backlog to gen. */
#ifdef notdef
	struct	aucb au_rb;		/* read (recv) buffer */
	struct	aucb au_wb;		/* write (xmit) buffer */
#endif
	struct	aucb au_cb;
};
#endif
