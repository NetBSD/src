/* 	$NetBSD: rasops.h,v 1.6 1999/05/18 21:51:59 ad Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef _RASOPS_H_
#define _RASOPS_H_ 1

/* Avoid dragging in dev/wscons/wsconsio.h */
struct wsdisplay_font;

struct rasops_info {
	/* These must be filled in by the caller */
	int	ri_depth;	/* depth in bits */
	u_char	*ri_bits;	/* ptr to bits */
	int	ri_width;	/* width (pels) */
	int	ri_height;	/* height (pels) */
	int	ri_stride;	/* stride in bytes */

	/* 
	 * These can optionally be left empty. If you fill ri_font,
	 * but aren't using wsfont, set ri_wsfcookie to -1.
	 */
	struct	wsdisplay_font *ri_font;
	int	ri_wsfcookie;
	void	*ri_priv;	/* driver private data */
	u_char	ri_forcemono;	/* force monochrome operation */
	u_char	ri_swab;	/* swap bytes for 15/16/32 bit depths? */
	
	/* 
	 * These are optional and will default if zero. Meaningless 
	 * on depths other than 15, 16, 24 and 32 bits per pel. On
	 * 24 bit displays, ri_{r,g,b}num must be 8.
	 */
	u_char	ri_rnum;	/* number of bits for red */
	u_char	ri_gnum;	/* number of bits for green */
	u_char	ri_bnum;	/* number of bits for blue */
	u_char	ri_rpos;	/* which bit red starts at */
	u_char	ri_gpos;	/* which bit green starts at */
	u_char	ri_bpos;	/* which bit blue starts at */

	/* These are filled in by rasops_init() */
	int	ri_emuwidth;	/* width we actually care about */
	int	ri_emuheight;	/* height we actually care about */
	int	ri_emustride;	/* bytes per row we actually care about */
	int	ri_rows;	/* number of rows (characters, not pels) */
	int	ri_cols;	/* number of columns (characters, not pels) */
	int	ri_delta;	/* row delta in bytes */
	int	ri_flg;		/* flags */
	int	ri_crow;	/* cursor row */
	int	ri_ccol;	/* cursor column */
	int	ri_pelbytes;	/* bytes per pel (may be zero) */
	int	ri_fontscale;	/* fontheight * fontstride */
	int	ri_xscale;	/* fontwidth * pelbytes */
	int	ri_yscale;	/* fontheight * stride */
	u_char  *ri_origbits;	/* where screen bits actually start */
	int	ri_xorigin;	/* where ri_bits begins (x) */
	int	ri_yorigin;	/* where ri_bits begins (y) */
	
	/* For 15, 16, 24, 32 bits */
	int32_t	ri_devcmap[16]; /* device colormap (WSCOL_*) */

	/* The emulops you need to use, and the screen caps for wscons */
	struct	wsdisplay_emulops ri_ops;
	int	ri_caps;
	
	/* Callbacks so we can share some code */
	void	(*ri_do_cursor) __P((struct rasops_info *));
};

#define RASOPS_CURSOR		(0x01)	/* cursor is on */
#define RASOPS_INITTED		(0x02)	/* struct is initialized */
#define RASOPS_CURSOR_CLIPPED	(0x04)	/* cursor is clipped */

#define DELTA(p, d, cast) ((p) = (cast)((caddr_t)(p) + (d)))

/* 
 * rasops_init().
 *
 * Integer parameters are: number of rows we'd like, number of columns we'd 
 * like, whether we should clear the display and whether we should center
 * the output. Remember that what you'd like is always not what you get.
 *
 * In terms of optimization, fonts that are a multiple of 8 pixels wide
 * work the best: this is important, and will cost you if you don't use 'em.
 *
 * rasops_init() takes care of rasops_setfont(). You only need to use this
 * when you want to switch fonts later on. The integer parameters to both
 * are the same. Before calling rasops_setfont(), set ri.ri_font. This
 * should happen at _splhigh_. If (ri_wsfcookie >= 0), you must call
 * wsfont_unlock() on it first.
 */

/* rasops.c */
int	rasops_init __P((struct rasops_info *, int, int, int, int));
int	rasops_setfont __P((struct rasops_info *, int, int, int, int));
void	rasops_unpack_attr __P((long, int *, int *, int *));

/* These should _not_ be called outside rasops */
void	rasops_eraserows __P((void *, int, int, long));
void	rasops_erasecols __P((void *, int, int, int, long));
void	rasops_copycols __P((void *, int, int, int, int));

extern u_char	rasops_isgray[16];
extern u_char	rasops_cmap[256*3];

#endif /* _RASOPS_H_ */
