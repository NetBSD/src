/*	$NetBSD: fbvar.h,v 1.5 1999/07/25 22:50:50 ad Exp $ */

/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.   Modifications have been made by Ted Lemon
 * to provide more generic frame buffer support.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)fbvar.h	8.1 (Berkeley) 6/11/93
 */

/* XXX */
#include <sys/select.h>

/* Hardware cursor information... */
struct hw_cursor {
	int	width, height;		/* Size of cursor... */
	int	x, y;			/* Position of cursor... */
	int	depth;			/* Depth in bits of cursor... */
	caddr_t	bitmap;			/* Cursor bitmap... */
	caddr_t cmap;			/* Cursor colormap... */
	int	cmap_size;		/* Size of cursor colormap... */
};

struct fbsoftc {
	struct	device sc_dv;
	struct	fbinfo *sc_fi;
};

struct fbinfo {
	int 	fi_unit;		/* Physical frame buffer unit. */
	struct	fbtype fi_type;		/* Geometry of frame buffer. */
	caddr_t	fi_pixels;		/* display RAM */
	int	fi_pixelsize;		/* size of display RAM in bytes*/
	caddr_t fi_base;		/* base address of fb I/O space. */
	caddr_t fi_vdac;		/* Colormap base address... */
	caddr_t fi_cmap_bits;		/* Colormap backing store... */
	int	fi_size;		/* Size of entire fb address space. */
	int	fi_linebytes;		/* bytes per display line */
	caddr_t	fi_savedcmap;		/* Colormap before open() */

	struct	fbdriver *fi_driver;	/* pointer to driver */
	struct	hw_cursor fi_cursor;	/* Hardware cursor info */

	u_int	fi_blanked;		/* Blanked? */
	u_int	fi_open;		/* Event queue mapped? */

	struct pmax_fbtty *fi_glasstty;	/* old-pmax fb driver compat  */

	struct	fbuaccess *fi_fbu;	/* qvss-style X event stuff */
	struct	selinfo fi_selp;	/* Select structure */
};

/*
 * Frame buffer variables.  All frame buffer drivers must provide the
 * following in order to participate.
 */
struct fbdriver {
	int	(*fbd_unblank) __P((struct fbinfo *));
	int	(*fbd_blank) __P((struct fbinfo *));
	void	(*fbd_initcmap) __P ((struct fbinfo *));
	int	(*fbd_getcmap) __P ((struct fbinfo *, caddr_t, int, int));
	int	(*fbd_putcmap) __P ((struct fbinfo *, caddr_t, int, int));
	void	(*fbd_poscursor) __P ((struct fbinfo *fi, int x, int y));
	void	(*fbd_loadcursor) __P ((struct fbinfo *fi, u_short *cursor));
	void	(*fbd_cursorcolor) __P ((struct fbinfo *fi, u_int *color));
};

#ifdef _KERNEL

#define kbd_docmd(cmd, val)	0	/* For now, do nothing. */
#define romgetcursoraddr(xp, yp)	0

void	fbconnect __P ((char *name, struct fbinfo *info, int silent));
int	fballoc __P ((caddr_t base, struct fbinfo **fip));

#endif /* _KERNEL */