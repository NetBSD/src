/*	$NetBSD: grfvar.h,v 1.9.26.1 2007/03/12 05:51:37 rmind Exp $	*/

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
 * from: Utah $Hdr: grfvar.h 1.11 93/08/13$
 *
 *	@(#)grfvar.h	8.2 (Berkeley) 9/9/93
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
 * from: Utah $Hdr: grfvar.h 1.11 93/08/13$
 *
 *	@(#)grfvar.h	8.2 (Berkeley) 9/9/93
 */

/* internal structure of lock page */
#define GRFMAXLCK	256
struct	grf_lockpage {
	u_char	gl_locks[GRFMAXLCK];
};
#define gl_lockslot gl_locks[0]

struct grf_softc;

/*
 * Static configuration info for display types
 */
struct	grfsw {
	int	gd_hwid;	/* id returned by hardware */
	int	gd_swid;	/* id to be returned by software */
	const char *gd_desc;	/* description printed at config time */
	int	(*gd_init)(struct grf_softc *, void *);
	int	(*gd_mode)(struct grf_softc *, u_long, void *);
};

/* per display info */
struct	grf_softc {
	struct device g_device;		/* for config */
	int	g_flags;		/* software flags */
	int	g_cfaddr;		/* "addr" locator */
	struct  grfsw *g_sw;		/* static configuration info */
	void *	g_regkva;		/* KVA of registers */
	void *	g_fbkva;		/* KVA of framebuffer */
	struct	grfinfo g_display;	/* hardware description (for ioctl) */
	void *	g_data;			/* device dependent data */
};

/* flags */
#define	GF_ALIVE	0x01
#define GF_OPEN		0x02
#define GF_EXCLUDE	0x04
#define GF_WANTED	0x08

/* requests to mode routine */
#define GM_GRFON	1
#define GM_GRFOFF	2
#define GM_GRFOVON	3
#define GM_GRFOVOFF	4
#define GM_DESCRIBE	5
#define GM_MAP		6
#define GM_UNMAP	7

#define GM_GRFCONFIG	8
#define GM_GRFGETVMODE	9
#define GM_GRFSETVMODE	10
#define GM_GRFGETNUMVM	11
#define GM_GRFGETBANK	12
#define GM_GRFSETBANK	13
#define GM_GRFGETCURBANK 14
#define GM_GRFIOCTL	15

/* minor device interpretation */
#define GRFUNIT(d)	minor(d)

#ifdef _KERNEL
extern	struct grfsw grfsw[];
extern	int ngrfsw;
#endif
