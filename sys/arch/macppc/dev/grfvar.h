/*	$NetBSD: grfvar.h,v 1.1 1998/05/15 10:15:48 tsubai Exp $	*/

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
 * from: Utah $Hdr: grfvar.h 1.9 91/01/21$
 *
 *	@(#)grfvar.h	7.3 (Berkeley) 5/7/91
 */

/*
 * State info, per grf instance.
 */
struct grf_softc {
	struct device	sc_dev;		/* device glue */
	struct grfinfo	*sc_display;	/* hardware description (for ioctl) */
	int		sc_flags;	/* software flags */
	int		(*sc_mode) __P((struct grf_softc *, int, void *));
					/* mode-change on/off/mode function */
};

/*
 * Attach grf and ite semantics to Mac video hardware.
 */
struct grf_attach_args {
	char	*ga_name;		/* name of semantics to attach */
	struct	grfinfo *ga_display;
	int	(*ga_mode) __P((struct grf_softc *, int, void *));
					/* mode-change on/off/mode function */
};

/* flags */
#define	GF_ALIVE	0x01
#define GF_OPEN		0x02
#define GF_EXCLUDE	0x04
#define GF_WANTED	0x08
#define GF_BSDOPEN	0x10
#define GF_HPUXOPEN	0x20

/* requests to mode routine */
#define GM_GRFON	1
#define GM_GRFOFF	2
#define GM_CURRMODE	3
#define GM_LISTMODES	4
#define GM_NEWMODE	5

/* minor device interpretation */
#define GRFUNIT(d)	((d) & 0x7)
