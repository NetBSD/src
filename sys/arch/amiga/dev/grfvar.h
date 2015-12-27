/*	$NetBSD: grfvar.h,v 1.24.14.1 2015/12/27 12:09:28 skrll Exp $	*/

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
 * from: Utah $Hdr: grfvar.h 1.9 91/01/21$
 *
 *	@(#)grfvar.h	7.3 (Berkeley) 5/7/91
 */

struct ite_softc;

#ifdef _KERNEL
/* maximum console size */
#define MAXROWS 200
#define MAXCOLS 200

/*
 * this struct is owned by the driver (grfcc, grfrt)
 * and is passed to grf when grf is configed. The ite also
 * uses it...
 */
struct	grf_softc {
	device_t	g_device;	/* config sets this up. */
	struct grfinfo	g_display;	/* hardware description (for ioctl) */
	volatile void	*g_regkva;	/* KVA of registers */
	volatile void	*g_fbkva;	/* KVA of framebuffer */
	int		g_flags;	/* software flags */
	int		g_unit;		/* grf unit we want/have */
	dev_t		g_itedev;	/* ite device number */
	dev_t		g_grfdev;	/* grf device number */
	void		*g_data;	/* device dependent data */
	int		g_blank;	/* shadow copy of blank value */
	int		(*g_mode)(struct grf_softc *, u_long, void *,
				  u_long, int);
#if NWSDISPLAY > 0
	struct wsdisplay_accessops	*g_accessops;
	struct wsdisplay_emulops	*g_emulops;
	struct wsscreen_descr		*g_defaultscr;
	struct wsscreen_list		*g_scrlist;
	struct vcons_data		g_vd;
	uint16_t g_rowoffset[MAXROWS];	/* speed up putchar-multiplication */
	int	g_wsmode;		/* current wsdisplay mode */ 
	
#else
	int	g_conpri;		/* priority of ite as console */
	void 	(*g_iteinit)(struct ite_softc *);
	void 	(*g_itedeinit)(struct ite_softc *);
	void 	(*g_iteclear)(struct ite_softc *, int, int, int, int);
	void 	(*g_iteputc)(struct ite_softc *, int, int, int, int);
	void 	(*g_itecursor)(struct ite_softc *, int);
	void 	(*g_itescroll)(struct ite_softc *, int, int, int, int);
#endif /* NWSDISPLAY */
};

#if NWSDISPLAY > 0
/*
 * Generic wsdisplay access ops that can be used from all grf drivers.
 */
paddr_t	grf_wsmmap(void *, void *, off_t, int);
int	grf_wsioctl(void *, void *, u_long, void *, int, struct lwp *);
#endif /* NWSDISPLAY */

#endif /* _KERNEL */

/* flags */
#define	GF_ALIVE	0x01
#define GF_OPEN		0x02
#define GF_EXCLUDE	0x04
#define GF_WANTED	0x08
#define GF_GRFON	0x10
#define GF_CONSOLE	0x20

/* software ids defined in grfioctl.h */

/* requests to mode routine (g_mode())*/
#define GM_GRFON	1
#define GM_GRFOFF	2
#define GM_GRFOVON	3
#define GM_GRFOVOFF	4
#define GM_GRFCONFIG	5
#define GM_GRFGETVMODE	6
#define GM_GRFSETVMODE	7
#define GM_GRFGETNUMVM	8
#define GM_GRFGETBANK	9
#define GM_GRFSETBANK	10
#define GM_GRFGETCURBANK 11
#define GM_GRFIOCTL	12
#define GM_GRFTOGGLE	13

/* minor device interpretation */
#define GRFOVDEV	0x10	/* used by grf_ul, overlay planes */
#define GRFIMDEV	0x20	/* used by grf_ul, images planes */
#define GRFUNIT(d)	((d) & 0x7)

/*
 * unit numbers for devices
 */
enum grfunits {
	GRF_CC_UNIT,
	GRF_RETINAII_UNIT,
	GRF_RETINAIII_UNIT,
	GRF_CL5426_UNIT,
	GRF_ULOWELL_UNIT,
	GRF_CV64_UNIT,
	GRF_ET4000_UNIT,
	GRF_CV3D_UNIT
};
