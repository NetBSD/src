/* $NetBSD: omrasops.c,v 1.26 2023/01/15 05:08:33 tsutsui Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: omrasops.c,v 1.26 2023/01/15 05:08:33 tsutsui Exp $");

/*
 * Designed speficically for 'm68k bitorder';
 *	- most significant byte is stored at lower address,
 *	- most significant bit is displayed at left most on screen.
 * Implementation relies on;
 *	- first column is at 32bit aligned address,
 *	- font glyphs are stored in 32bit padded.
 */
/*
 * BMSEL affects both of
 * 1) which plane a write to the common bitmap plane is reflected on and
 * 2) which plane's ROP a write to the common ROP is reflected on.
 *
 * The common ROP is not a ROP applied to write to the common bitmap plane.
 * It's equivalent to set ROPs of the plane selected in the plane mask one
 * by one.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <arch/luna68k/dev/omrasopsvar.h>

#ifdef luna68k
#define USE_M68K_ASM	1
#endif

/* To provide optimization conditions to compilers */
#if defined(__GNUC__)
#define ASSUME(cond)	if (!(cond)) __unreachable()
#elif defined(__clang__) && __has_builtin(__builtin_assume)
#define ASSUME(cond)	__builtin_assume(cond)
#else
#define ASSUME(cond)	(void)(cond)
#endif

/* XXX it should be redesigned, including making the attributes support 8bpp */
typedef struct {
	union {
		int32_t all;
		struct {
			int8_t ismulti; /* is multi color used */
			uint8_t fg;
			uint8_t bg;
			uint8_t reserved;
		};
	};
} rowattr_t;

/* wscons emulator operations */
static void	om_cursor(void *, int, int, int);
static int	om_mapchar(void *, int, u_int *);
static void	om_putchar(void *, int, int, u_int, long);
static void	om1_copycols(void *, int, int, int, int);
static void	om4_copycols(void *, int, int, int, int);
static void	om1_copyrows(void *, int, int, int num);
static void	om4_copyrows(void *, int, int, int num);
static void	om_erasecols(void *, int, int, int, long);
static void	om_eraserows(void *, int, int, long);
static int	om_allocattr(void *, int, int, int, long *);

static void	om_fill(int, int, uint8_t *, int, int, uint32_t, int, int);
static void	om_fill_color(int, int, uint8_t *, int, int, int, int);
static void	om_rascopy_single(int, uint8_t *, uint8_t *, int16_t, int16_t,
    uint8_t[]);
static void	om4_rascopy_multi(uint8_t *, uint8_t *, int16_t, int16_t);
static void	om_unpack_attr(long, uint8_t *, uint8_t *, int *);

static int	omrasops_init(struct rasops_info *, int, int);

/*
 * XXX should be fixed...
 * This number of elements is derived from howmany(1024, fontheight = 24).
 * But it is currently initialized with row = 34, so it is used only up to 34.
 */
#define OMRASOPS_MAX_ROWS	43
static rowattr_t rowattr[OMRASOPS_MAX_ROWS];

#define	ALL1BITS	(~0U)
#define	ALL0BITS	(0U)
#define	BLITWIDTH	(32)
#define	ALIGNMASK	(0x1f)
#define	BYTESDONE	(4)

#if 0 /* XXX not used yet */
/*
 * internal attributes. see om_allocattr().
 */
#define OMFB_ATTR_MULTICOLOR		(1U << 31)
#define OMFB_ATTR_UNDERLINE		(1U << 17)
#define OMFB_ATTR_BOLD			(1U << 16)
#endif

/*
 * XXX deprecated.
 * This way cannot be extended to 8bpp, so don't use it in new code.
 */
#define P0(addr) ((uint32_t *)((uint8_t *)(addr) + OMFB_PLANEOFFS * 1))
#define P1(addr) ((uint32_t *)((uint8_t *)(addr) + OMFB_PLANEOFFS * 2))
#define P2(addr) ((uint32_t *)((uint8_t *)(addr) + OMFB_PLANEOFFS * 3))
#define P3(addr) ((uint32_t *)((uint8_t *)(addr) + OMFB_PLANEOFFS * 4))

/*
 * macros to handle unaligned bit copy ops.
 * See src/sys/dev/rasops/rasops_masks.h for MI version.
 * Also refer src/sys/arch/hp300/dev/maskbits.h.
 * (which was implemented for ancient src/sys/arch/hp300/dev/grf_hy.c)
 */

/* luna68k version GETBITS() that gets w bits from bit x at psrc memory */
#define	FASTGETBITS(psrc, x, w, dst)					\
	asm("bfextu %3{%1:%2},%0"					\
	    : "=d" (dst)						\
	    : "di" (x), "di" (w), "o" (*(uint32_t *)(psrc)))

/* luna68k version PUTBITS() that puts w bits from bit x at pdst memory */
/* XXX this macro assumes (x + w) <= 32 to handle unaligned residual bits */
#define	FASTPUTBITS(src, x, w, pdst)					\
	asm("bfins %3,%0{%1:%2}"					\
	    : "+o" (*(uint32_t *)(pdst))				\
	    : "di" (x), "di" (w), "d" (src)				\
	    : "memory" )

#define	GETBITS(psrc, x, w, dst)	FASTGETBITS(psrc, x, w, dst)
#define	PUTBITS(src, x, w, pdst)	FASTPUTBITS(src, x, w, pdst)

/*
 * Clear lower w bits from x.
 * x must be filled with 1 at least lower w bits.
 */
#if USE_M68K_ASM
#define CLEAR_LOWER_BITS(x, w)						\
	asm volatile(							\
	"	bclr	%[width],%[data]	;\n"			\
	"	addq.l	#1,%[data]		;\n"			\
	    : [data] "+&d" (x)						\
	    : [width] "d" (w)						\
	    :								\
	)
#else
#define CLEAR_LOWER_BITS(x, w)	x = ((x) & ~(1U << (w))) + 1
#endif

/* Set planemask for the common plane and the common ROP */
static inline void
om_set_planemask(int planemask)
{

	*(volatile uint32_t *)OMFB_PLANEMASK = planemask;
}

/* Get a ROP address */
static inline volatile uint32_t *
om_rop_addr(int plane, int rop)
{

	return (volatile uint32_t *)
	    (OMFB_ROP_P0 + OMFB_PLANEOFFS * plane + rop * 4);
}

/* Set ROP and ROP's mask for individual plane */
static inline void
om_set_rop(int plane, int rop, uint32_t mask)
{

	*om_rop_addr(plane, rop) = mask;
}

/* Set ROP and ROP's mask for current setplanemask-ed plane(s) */
static inline void
om_set_rop_curplane(int rop, uint32_t mask)
{

	((volatile uint32_t *)(OMFB_ROP_COMMON))[rop] = mask;
}

/* Reset planemask and ROP */
static inline void
om_reset_planemask_and_rop(void)
{

	om_set_planemask(hwplanemask);
	om_set_rop_curplane(ROP_THROUGH, ~0U);
}

static inline void
om_set_rowattr(int row, uint8_t fg, uint8_t bg)
{

	if (rowattr[row].fg == fg && rowattr[row].bg == bg)
		return;
	if (rowattr[row].ismulti)
		return;

	if (rowattr[row].fg == rowattr[row].bg) {
		/* From the initial (erased) state, */
		if (rowattr[row].fg != fg && rowattr[row].bg != bg) {
			/* if both are changed at once, it's multi color */
			rowattr[row].ismulti = true;
		} else {
			/* otherwise, it's single color */
			rowattr[row].fg = fg;
			rowattr[row].bg = bg;
		}
	} else {
		rowattr[row].ismulti = true;
	}
}

static inline void
om_reset_rowattr(int row, uint8_t bg)
{

	/* Setting fg equal to bg means 'reset' or 'erased'. */
	rowattr[row].ismulti = false;
	rowattr[row].bg = bg;
	rowattr[row].fg = bg;
}

/*
 * Fill rectangle.
 * val is assumed only ALL0BITS or ALL1BITS, because all bits are used as is
 * regardless of bit offset of the destination.
 */
static void
om_fill(int planemask, int rop, uint8_t *dstptr, int dstbitoffs, int dstspan,
    uint32_t val, int width, int height)
{
	uint32_t mask;
	uint32_t prev_mask;
	int32_t height_m1;
	int dw;		/* 1 pass width bits */

	ASSUME(width > 0);
	ASSUME(height > 0);
	ASSUME(0 <= dstbitoffs && dstbitoffs < 32);

	om_set_planemask(planemask);

	height_m1 = height - 1;
	mask = ALL1BITS >> dstbitoffs;
	prev_mask = ~mask;
	dw = 32 - dstbitoffs;

	/* do-while loop seems slightly faster than a for loop */
	do {
		uint8_t *d;
		int32_t h;

		width -= dw;
		if (width < 0) {
			CLEAR_LOWER_BITS(mask, -width);
			/* To exit this loop. */
			width = 0;
		}

		if (prev_mask != mask) {
			om_set_rop_curplane(rop, mask);
			prev_mask = mask;
		}

		d = dstptr;
		dstptr += 4;
		h = height_m1;

#if USE_M68K_ASM
		asm volatile("\n"
		"om_fill_loop_h:\n"
		"	move.l	%[val],(%[d])			;\n"
		"	add.l	%[dstspan],%[d]			;\n"
		"	dbra	%[h],om_fill_loop_h		;\n"
		    : /* output */
		      [d] "+&a" (d),
		      [h] "+&d" (h)
		    : /* input */
		      [val] "d" (val),
		      [dstspan] "r" (dstspan)
		    : /* clobbers */
		      "memory"
		);
#else
		do {
			*(uint32_t *)d = val;
			d += dstspan;
		} while (--h >= 0);
#endif
		mask = ALL1BITS;
		dw = 32;
	} while (width > 0);
}

static void
om_fill_color(int planecount, int color, uint8_t *dstptr, int dstbitoffs,
    int dstspan, int width, int height)
{
	uint32_t mask;
	uint32_t prev_mask;
	int32_t height_m1;
	int dw;		/* 1 pass width bits */

	ASSUME(width > 0);
	ASSUME(height > 0);
	ASSUME(planecount > 0);

	/* select all planes */
	om_set_planemask(hwplanemask);

	mask = ALL1BITS >> dstbitoffs;
	prev_mask = ~mask;
	dw = 32 - dstbitoffs;
	height_m1 = height - 1;

	do {
		uint8_t *d;
		int32_t plane;
		int32_t h;
		int16_t rop;

		width -= dw;
		if (width < 0) {
			CLEAR_LOWER_BITS(mask, -width);
			/* To exit this loop. */
			width = 0;
		}

		if (prev_mask != mask) {
			for (plane = 0; plane < planecount; plane++) {
				if ((color & (1U << plane)) != 0)
					rop = ROP_ONE;
				else
					rop = ROP_ZERO;
				om_set_rop(plane, rop, mask);
			}
			prev_mask = mask;
		}

		d = dstptr;
		dstptr += 4;
		h = height_m1;

#if USE_M68K_ASM
		asm volatile("\n"
		"om_fill_color_loop_h:\n"
		"	clr.l	(%[d])				;\n"
		"	add.l	%[dstspan],%[d]			;\n"
		"	dbra	%[h],om_fill_color_loop_h	;\n"
		    : /* output */
		      [d] "+&a" (d),
		      [h] "+&d" (h)
		    : /* input */
		      [dstspan] "r" (dstspan)
		    : /* clobbers */
		      "memory"
		);
#else
		do {
			/*
			 * ROP is either ONE or ZERO,
			 * so don't care what you write to *d.
			 */
			*(uint32_t *)d = 0;
			d += dstspan;
		} while (--h >= 0);
#endif
		mask = ALL1BITS;
		dw = 32;
	} while (width > 0);
}

/*
 * Calculate ROP depending on fg/bg color combination as follows.
 * This is called per individual plane while shifting fg and bg.
 * So the LSB of fg and bg points to this plane.
 *
 * All ROP values we want to use here happens to be a multiple of 5.
 *
 *  bg fg  rop               result
 *  -- --  ----------------  ------
 *   0  0  ROP_ZERO    =  0   0
 *   0  1  ROP_THROUGH =  5   D
 *   1  0  ROP_INV1    = 10  ~D
 *   1  1  ROP_ONE     = 15   1
 *
 * This allows characters to be drawn in the specified fg/bg colors with
 * a single write to the common plane.
 */
static inline int
om_fgbg2rop(uint8_t fg, uint8_t bg)
{
	int t;

	t = (bg & 1) * 2 + (fg & 1);
	return t * 5;
}

/*
 * Blit a character at the specified co-ordinates.
 * This function modifies(breaks) the planemask and ROPs.
 */
static void
om_putchar(void *cookie, int row, int startcol, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *fontptr;
	uint8_t *dstcmn;
	uint32_t mask;
	int width;
	int height;
	int planecount;
	int x, y;
	int fontstride;
	int fontx;
	int plane;
	int dw;		/* 1 pass width bits */
	int xh, xl;
	uint8_t fg, bg;
	/* ROP address cache */
	static volatile uint32_t *ropaddr[OMFB_MAX_PLANECOUNT];
	static uint8_t last_fg, last_bg;

	if (uc >= 0x80)
		return;

	width = ri->ri_font->fontwidth;
	height = ri->ri_font->fontheight;
	planecount = ri->ri_depth;
	fontstride = ri->ri_font->stride;
	y = height * row;
	x = width * startcol;
	fontptr = (uint8_t *)ri->ri_font->data +
	    (uc - ri->ri_font->firstchar) * ri->ri_fontscale;

	om_unpack_attr(attr, &fg, &bg, NULL);
	om_set_rowattr(row, fg, bg);

	if (last_fg != fg || last_bg != bg) {
		last_fg = fg;
		last_bg = bg;
		/* calculate ROP */
		for (plane = 0; plane < planecount; plane++) {
			int t = om_fgbg2rop(fg, bg);
			ropaddr[plane] = om_rop_addr(plane, t);
			fg >>= 1;
			bg >>= 1;
		}
	}

	/* divide x into the lower 5 bits and the rest. */
	xh = x >> 5;
	xl = x & 0x1f;

	/* write to common plane */
	dstcmn = (uint8_t *)ri->ri_bits + xh * 4 + y * OMFB_STRIDE;

	/* select all plane */
	om_set_planemask(hwplanemask);

	fontx = 0;
	mask = ALL1BITS >> xl;
	dw = 32 - xl;

	ASSUME(planecount == 1 ||
	       planecount == 4 ||
	       planecount == 8);

	do {
		uint8_t *d;
		uint8_t *f;
		int32_t h;

		width -= dw;
		if (width < 0) {
			CLEAR_LOWER_BITS(mask, -width);
			/* To exit this loop. */
			width = 0;
		}

		switch (planecount) {
		 case 8:
			*(ropaddr[7]) = mask;
			*(ropaddr[6]) = mask;
			*(ropaddr[5]) = mask;
			*(ropaddr[4]) = mask;
			/* FALLTHROUGH */
		 case 4:
			*(ropaddr[3]) = mask;
			*(ropaddr[2]) = mask;
			*(ropaddr[1]) = mask;
			/* FALLTHROUGH */
		 case 1:
			*(ropaddr[0]) = mask;
			break;
		}

		d = dstcmn;
		f = fontptr;
		h = height - 1;
		do {
			uint32_t v;
			GETBITS(f, fontx, dw, v);
			/* no need to shift v because it's masked by ROP */
			*(uint32_t *)d = v;
			d += OMFB_STRIDE;
			f += fontstride;
		} while (--h >= 0);

		dstcmn += 4;
		fontx += dw;
		mask = ALL1BITS;
		dw = 32;
	} while (width > 0);

	om_reset_planemask_and_rop();
}

static void
om_erasecols(void *cookie, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = cookie;
	int startx;
	int width;
	int height;
	int planecount;
	int sh, sl;
	int y;
	int scanspan;
	uint8_t *p;
	uint8_t fg, bg;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	planecount = ri->ri_depth;
	om_unpack_attr(attr, &fg, &bg, NULL);
	sh = startx >> 5;
	sl = startx & 0x1f;
	p = (uint8_t *)ri->ri_bits + y * scanspan + sh * 4;

	/* I'm not sure */
	om_set_rowattr(row, fg, bg);

	if (bg == 0) {
		/* om_fill seems slightly efficient */
		om_fill(hwplanemask, ROP_ZERO,
		    p, sl, scanspan, 0, width, height);
	} else {
		om_fill_color(planecount, bg, p, sl, scanspan, width, height);
	}

	/* reset mask value */
	om_reset_planemask_and_rop();
}

static void
om_eraserows(void *cookie, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	int startx;
	int width;
	int height;
	int planecount;
	int sh, sl;
	int y;
	int scanspan;
	int row;
	uint8_t *p;
	uint8_t fg, bg;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	startx = 0;
	width = ri->ri_emuwidth;
	height = ri->ri_font->fontheight * nrows;
	planecount = ri->ri_depth;
	om_unpack_attr(attr, &fg, &bg, NULL);
	sh = startx >> 5;
	sl = startx & 0x1f;
	p = (uint8_t *)ri->ri_bits + y * scanspan + sh * 4;

	for (row = startrow; row < startrow + nrows; row++) {
		om_reset_rowattr(row, bg);
	}

	if (bg == 0) {
		/* om_fill seems slightly efficient */
		om_fill(hwplanemask, ROP_ZERO,
		    p, sl, scanspan, 0, width, height);
	} else {
		om_fill_color(planecount, bg, p, sl, scanspan, width, height);
	}
	/* reset mask value */
	om_reset_planemask_and_rop();
}

/*
 * Single plane raster copy.
 *  dst: destination plane pointer.
 *  src: source plane pointer.
 *       if y-forward, src > dst, point to left-top.
 *       if y-backward, src < dst, point to left-bottom.
 *  width: pixel width (must > 0)
 *  height: pixel height (> 0 if forward, < 0 if backward)
 *  rop: ROP array with planecount elements.
 *
 * This function modifies(breaks) the planemask and ROPs.
 */
static void
om_rascopy_single(int planecount, uint8_t *dst, uint8_t *src,
    int16_t width, int16_t height, uint8_t rop[])
{
	uint32_t mask;
	int wh;
	int wl;
	int step;
	int plane;
	int16_t height_m1;
	int16_t w, h;

	step = OMFB_STRIDE;

	/*
	 * X direction is always forward (or ascend order) to use (An)+
	 * addressing mode in asm.
	 */

	/* Reverse order Y if backward copy */
	if (height < 0) {
		/* The sign is managed by step, height is always positive */
		step = -step;
		height = -height;
	}
	height_m1 = height - 1;

	/*
	 * On single, it's not necessary to process two longwords at a time,
	 * but we do so for symmetry and speedup.
	 */

	/* First, transfer a rectangle consist of two longwords */
	wh = (width >> 6);
	if (wh > 0) {
		int step8 = step - wh * 8;

#if USE_M68K_ASM
		wh--;	/* for dbra */
		h = height_m1;
		asm volatile("\n"
		"om_rascopy_single_LL:\n"
		"	move.w	%[wh],%[w]			;\n"
		"1:\n"
		"	move.l	(%[src])+,(%[dst])+		;\n"
		"	move.l	(%[src])+,(%[dst])+		;\n"
		"	dbra	%[w],1b				;\n"

		"	adda.l	%[step8],%[src]			;\n"
		"	adda.l	%[step8],%[dst]			;\n"
		"	dbra	%[h],om_rascopy_single_LL	;\n"
		    : /* output */
		      [src] "+&a" (src),
		      [dst] "+&a" (dst),
		      [h] "+&d" (h),
		      [w] "=&d" (w)
		    : /* input */
		      [wh] "r" (wh),
		      [step8] "r" (step8)
		    : /* clobbers */
		      "memory"
		);
#else
		wh--;	/* to match to asm side */
		for (h = height_m1; h >= 0; h--) {
			uint32_t *s32 = (uint32_t *)src;
			uint32_t *d32 = (uint32_t *)dst;
			for (w = wh; w >= 0; w--) {
				*d32++ = *s32++;
				*d32++ = *s32++;
			}
			src = (uint8_t *)s32 + step8;
			dst = (uint8_t *)d32 + step8;
		}
#endif

		if ((width & 0x3f) == 0) {
			/* transfer completed */
			return;
		}

		/* rewind y for the next transfer */
		src -= height * step;
		dst -= height * step;
	}

	if ((width & 32) != 0) {
		/* Transfer one longword since an odd longword */
#if USE_M68K_ASM
		h = height_m1;
		asm volatile("\n"
		"om_rascopy_single_L:\n"
		"	move.l	(%[src]),(%[dst])		;\n"
		"	adda.l	%[step],%[src]			;\n"
		"	adda.l	%[step],%[dst]			;\n"
		"	dbra	%[h],om_rascopy_single_L	;\n"
		    : /* output */
		      [src] "+&a" (src),
		      [dst] "+&a" (dst),
		      [h] "+&d" (h)
		    : /* input */
		      [step] "r" (step)
		    : /* clobbers */
		      "memory"
		);
#else
		for (h = height_m1; h >= 0; h--) {
			*(uint32_t *)dst = *(uint32_t *)src;
			dst += step;
			src += step;
		}
#endif

		if ((width & 0x1f) == 0) {
			/* transfer completed */
			return;
		}

		/* rewind y for the next transfer */
		src += 4 - height * step;
		dst += 4 - height * step;
	}

	wl = width & 0x1f;
	/* wl > 0 at this point */

	/* Then, transfer residual bits */

	mask = ALL1BITS << (32 - wl);
	/*
	 * The common ROP cannot be used here.  Because the hardware doesn't
	 * allow you to set the mask while keeping the ROP states.
	 */
	for (plane = 0; plane < planecount; plane++) {
		om_set_rop(plane, rop[plane], mask);
	}

#if USE_M68K_ASM
	h = height_m1;
	asm volatile("\n"
	"om_rascopy_single_bit:\n"
	"	move.l	(%[src]),(%[dst])			;\n"
	"	adda.l	%[step],%[src]				;\n"
	"	adda.l	%[step],%[dst]				;\n"
	"	dbra	%[h],om_rascopy_single_bit		;\n"
	    : /* output */
	      [src] "+&a" (src),
	      [dst] "+&a" (dst),
	      [h] "+&d" (h)
	    : /* input */
	      [step] "r" (step)
	    : /* clobbers */
	      "memory"
	);
#else
	for (h = height_m1; h >= 0; h--) {
		*(uint32_t *)dst = *(uint32_t *)src;
		dst += step;
		src += step;
	}
#endif

	for (plane = 0; plane < planecount; plane++) {
		om_set_rop(plane, rop[plane], ALL1BITS);
	}
}

/*
 * Multiple plane raster copy.
 *  dst0: destination pointer in Plane0.
 *  src0: source pointer in Plane0.
 *       if y-forward, src0 > dst0, point to left-top.
 *       if y-backward, src0 < dst0, point to left-bottom.
 *  width: pixel width (must > 0)
 *  height: pixel height (> 0 if forward, < 0 if backward)
 *
 * This function modifies(breaks) the planemask and ROPs.
 */
static void
om4_rascopy_multi(uint8_t *dst0, uint8_t *src0, int16_t width, int16_t height)
{
	uint8_t *dst1, *dst2, *dst3;
	int wh;
	int wl;
	int rewind;
	int step;
	uint32_t mask;
	int16_t height_m1;
	int16_t w, h;

	step = OMFB_STRIDE;

	/*
	 * X direction is always forward (or ascend order) to use (An)+
	 * addressing mode in asm.
	 */

	/* Reverse order Y if backward copy */
	if (height < 0) {
		/* The sign is managed by step, height is always positive */
		step = -step;
		height = -height;
	}
	height_m1 = height - 1;

	dst1 = dst0 + OMFB_PLANEOFFS;
	dst2 = dst1 + OMFB_PLANEOFFS;
	dst3 = dst2 + OMFB_PLANEOFFS;

	/* First, transfer a rectangle consist of two longwords */
	wh = width >> 6;
	if (wh > 0) {
		int step8 = step - wh * 8;

#if USE_M68K_ASM
		wh--;	/* for dbra */
		h = height_m1;
		asm volatile("\n"
		"om4_rascopy_multi_LL:\n"
		"	move.w	%[wh],%[w]		;\n"
		"1:\n"
			/*
			 * Optimized for 68030.
			 *
			 * On LUNA, the following is faster than any of
			 * "MOVE.L (An)+,(An)+", "MOVE.L (An,Dn),(An,Dn)", or
			 * "MOVEM.L", due to the relationship of instruction
			 *  overlaps and access waits.
			 *
			 * The head time of (An)+ as source operand is 0 and
			 * the head time of ADDA instruction is 2.  If the
			 * previous instruction has some write wait cycles,
			 * i.e., tail cycles, (An)+ as source operand cannot
			 * overlap it but ADDA instruction can.
			 */
		"	move.l	(%[src0]),(%[dst0])+	;\n"	/* P0 */
		"	adda.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0]),(%[dst1])+	;\n"	/* P1 */
		"	adda.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0]),(%[dst2])+	;\n"	/* P2 */
		"	adda.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0]),(%[dst3])+	;\n"	/* P3 */
			/* Expect an overlap, so don't use (An)+ */
		"	addq.l	#4,%[src0]		;\n"

		"	move.l	(%[src0]),(%[dst3])+	;\n"	/* P3 */
		"	suba.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0]),(%[dst2])+	;\n"	/* P2 */
		"	suba.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0]),(%[dst1])+	;\n"	/* P1 */
		"	suba.l	%[PLANEOFFS],%[src0]	;\n"
		"	move.l	(%[src0])+,(%[dst0])+	;\n"	/* P0 */
		"	dbra	%[w],1b			;\n"

		"	adda.l	%[step8],%[src0]	;\n"
		"	adda.l	%[step8],%[dst0]	;\n"
		"	adda.l	%[step8],%[dst1]	;\n"
		"	adda.l	%[step8],%[dst2]	;\n"
		"	adda.l	%[step8],%[dst3]	;\n"
		"	dbra	%[h],om4_rascopy_multi_LL	;\n"
		    : /* output */
		      [src0] "+&a" (src0),
		      [dst0] "+&a" (dst0),
		      [dst1] "+&a" (dst1),
		      [dst2] "+&a" (dst2),
		      [dst3] "+&a" (dst3),
		      [h] "+&d" (h),
		      [w] "=&d" (w)
		    : /* input */
		      [wh] "r" (wh),
		      [PLANEOFFS] "r" (OMFB_PLANEOFFS),
		      [step8] "r" (step8)
		    : /* clobbers */
		      "memory"
		);
#else
		wh--;	/* to match to asm side */
		for (h = height_m1; h >= 0; h--) {
			for (w = wh; w >= 0; w--) {
				*(uint32_t *)dst0 = *(uint32_t *)src0;
				dst0 += 4;
				src0 += OMFB_PLANEOFFS;
				*(uint32_t *)dst1 = *(uint32_t *)src0;
				dst1 += 4;
				src0 += OMFB_PLANEOFFS;
				*(uint32_t *)dst2 = *(uint32_t *)src0;
				dst2 += 4;
				src0 += OMFB_PLANEOFFS;
				*(uint32_t *)dst3 = *(uint32_t *)src0;
				dst3 += 4;
				src0 += 4;

				*(uint32_t *)dst3 = *(uint32_t *)src0;
				dst3 += 4;
				src0 -= OMFB_PLANEOFFS;
				*(uint32_t *)dst2 = *(uint32_t *)src0;
				dst2 += 4;
				src0 -= OMFB_PLANEOFFS;
				*(uint32_t *)dst1 = *(uint32_t *)src0;
				dst1 += 4;
				src0 -= OMFB_PLANEOFFS;
				*(uint32_t *)dst0 = *(uint32_t *)src0;
				dst0 += 4;
				src0 += 4;
			}
			src0 += step8;
			dst0 += step8;
			dst1 += step8;
			dst2 += step8;
			dst3 += step8;
		}
#endif

		if ((width & 0x3f) == 0) {
			/* transfer completed */
			return;
		}

		/* rewind y for the next transfer */
		src0 -= height * step;
		dst0 -= height * step;
		dst1 -= height * step;
		dst2 -= height * step;
		dst3 -= height * step;
	}

	/* This rewind rewinds the plane, so Y order is irrelevant */
	rewind = OMFB_STRIDE - OMFB_PLANEOFFS * 3;

	if ((width & 32) != 0) {
		/* Transfer one longword since an odd longword */
#if USE_M68K_ASM
		h = height_m1;
		asm volatile("\n"
		"om4_rascopy_multi_L:\n"
		"	move.l	(%[src0]),(%[dst0])		;\n"
		"	adda.l	%[PLANEOFFS],%[src0]		;\n"
		"	move.l	(%[src0]),(%[dst1])		;\n"
		"	adda.l	%[PLANEOFFS],%[src0]		;\n"
		"	move.l	(%[src0]),(%[dst2])		;\n"
		"	adda.l	%[PLANEOFFS],%[src0]		;\n"
		"	move.l	(%[src0]),(%[dst3])		;\n"
		"	adda.l	%[rewind],%[src0]		;\n"

		"	adda.l	%[step],%[dst0]			;\n"
		"	adda.l	%[step],%[dst1]			;\n"
		"	adda.l	%[step],%[dst2]			;\n"
		"	adda.l	%[step],%[dst3]			;\n"
		"	dbra	%[h],om4_rascopy_multi_L	;\n"
		    : /* output */
		      [src0] "+&a" (src0),
		      [dst0] "+&a" (dst0),
		      [dst1] "+&a" (dst1),
		      [dst2] "+&a" (dst2),
		      [dst3] "+&a" (dst3),
		      [h] "+&d" (h)
		    : /* input */
		      [PLANEOFFS] "r" (OMFB_PLANEOFFS),
		      [rewind] "r" (rewind),
		      [step] "r" (step)
		    : /* clobbers */
		      "memory"
		);
#else
		for (h = height_m1; h >= 0; h--) {
			*(uint32_t *)dst0 = *(uint32_t *)src0;
			src0 += OMFB_PLANEOFFS;
			*(uint32_t *)dst1 = *(uint32_t *)src0;
			src0 += OMFB_PLANEOFFS;
			*(uint32_t *)dst2 = *(uint32_t *)src0;
			src0 += OMFB_PLANEOFFS;
			*(uint32_t *)dst3 = *(uint32_t *)src0;
			src0 += rewind;

			dst0 += step;
			dst1 += step;
			dst2 += step;
			dst3 += step;
		}
#endif

		if ((width & 0x1f) == 0) {
			/* transfer completed */
			return;
		}

		/* rewind y for the next transfer */
		src0 += 4 - height * step;
		dst0 += 4 - height * step;
		dst1 += 4 - height * step;
		dst2 += 4 - height * step;
		dst3 += 4 - height * step;
	}

	wl = width & 0x1f;
	/* wl > 0 at this point */

	/* Then, transfer residual bits */

	mask = ALL1BITS << (32 - wl);
	om_set_planemask(hwplanemask);
	om_set_rop_curplane(ROP_THROUGH, mask);

#if USE_M68K_ASM
	h = height_m1;
	asm volatile("\n"
	"om4_rascopy_multi_bit:\n"
	"	move.l	(%[src0]),(%[dst0])			;\n"
	"	adda.l	%[PLANEOFFS],%[src0]			;\n"
	"	move.l	(%[src0]),(%[dst1])			;\n"
	"	adda.l	%[PLANEOFFS],%[src0]			;\n"
	"	move.l	(%[src0]),(%[dst2])			;\n"
	"	adda.l	%[PLANEOFFS],%[src0]			;\n"
	"	move.l	(%[src0]),(%[dst3])			;\n"
	"	adda.l	%[rewind],%[src0]			;\n"

	"	adda.l	%[step],%[dst0]				;\n"
	"	adda.l	%[step],%[dst1]				;\n"
	"	adda.l	%[step],%[dst2]				;\n"
	"	adda.l	%[step],%[dst3]				;\n"
	"	dbra	%[h],om4_rascopy_multi_bit		;\n"
	    : /* output */
	      [src0] "+&a" (src0),
	      [dst0] "+&a" (dst0),
	      [dst1] "+&a" (dst1),
	      [dst2] "+&a" (dst2),
	      [dst3] "+&a" (dst3),
	      [h] "+&d" (h)
	    : /* input */
	      [PLANEOFFS] "r" (OMFB_PLANEOFFS),
	      [rewind] "r" (rewind),
	      [step] "r" (step)
	    : /* clobbers */
	      "memory"
	);
#else
	for (h = height_m1; h >= 0; h--) {
		*(uint32_t *)dst0 = *(uint32_t *)src0;
		src0 += OMFB_PLANEOFFS;
		*(uint32_t *)dst1 = *(uint32_t *)src0;
		src0 += OMFB_PLANEOFFS;
		*(uint32_t *)dst2 = *(uint32_t *)src0;
		src0 += OMFB_PLANEOFFS;
		*(uint32_t *)dst3 = *(uint32_t *)src0;
		src0 += rewind;

		dst0 += step;
		dst1 += step;
		dst2 += step;
		dst3 += step;
	}
#endif
	om_reset_planemask_and_rop();
}

static void
om1_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	uint8_t *p, *q;
	int scanspan, offset, srcy, height, width, w;
	uint32_t rmask;

	scanspan = ri->ri_stride;
	height = ri->ri_font->fontheight * nrows;
	offset = (dstrow - srcrow) * scanspan * ri->ri_font->fontheight;
	srcy = ri->ri_font->fontheight * srcrow;
	if (srcrow < dstrow && srcrow + nrows > dstrow) {
		scanspan = -scanspan;
		srcy = srcy + height - 1;
	}

	p = (uint8_t *)ri->ri_bits + srcy * ri->ri_stride;
	w = ri->ri_emuwidth;
	width = w;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	q = p;
	while (height > 0) {
		*P0(p + offset) = *P0(p);		/* always aligned */
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			*P0(p + offset) = *P0(p);
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		*P0(p + offset) = (*P0(p) & rmask) | (*P0(p + offset) & ~rmask);

		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om4_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	uint8_t *src, *dst;
	int width, rowheight;
	int planecount;
	int ptrstep, rowstep;
	int srcplane;
	int i;
	int r;
	uint8_t rop[OMFB_MAX_PLANECOUNT];

	width = ri->ri_emuwidth;
	rowheight = ri->ri_font->fontheight;
	planecount = ri->ri_depth;
	src = (uint8_t *)ri->ri_bits + srcrow * rowheight * ri->ri_stride;
	dst = (uint8_t *)ri->ri_bits + dstrow * rowheight * ri->ri_stride;

	if (nrows <= 0 || srcrow == dstrow) {
		return;
	} else if (srcrow < dstrow) {
		/* y-backward */

		/* select the bottom raster of the bottom row */
		srcrow += nrows - 1;
		dstrow += nrows - 1;
		src += nrows * rowheight * ri->ri_stride - ri->ri_stride;
		dst += nrows * rowheight * ri->ri_stride - ri->ri_stride;
		rowstep = -1;
		rowheight = -rowheight;
	} else {
		/* y-forward */
		rowstep = 1;
	}
	ptrstep = ri->ri_stride * rowheight;

	om_set_planemask(hwplanemask);

	srcplane = 0;
	while (nrows > 0) {
		r = 1;
		if (rowattr[srcrow].ismulti == false &&
		    rowattr[srcrow].fg == rowattr[srcrow].bg &&
		    rowattr[srcrow].all == rowattr[dstrow].all) {
			goto skip;
		}

		/* count the number of rows with the same attributes */
		for (; r < nrows; r++) {
			if (rowattr[srcrow + r * rowstep].all !=
			    rowattr[srcrow].all) {
				break;
			}
		}
		/* r is the number of rows including srcrow itself */

		if (rowattr[srcrow].ismulti) {
			/*
			 * src,dst point to the common plane.  src0,dst0 will
			 * point to the same offset in plane0 because plane0
			 * is placed just after the common plane.
			 */
			uint8_t *src0 = src + OMFB_PLANEOFFS;
			uint8_t *dst0 = dst + OMFB_PLANEOFFS;
			om_set_rop_curplane(ROP_THROUGH, ALL1BITS);
			om4_rascopy_multi(dst0, src0, width, rowheight * r);
		} else {
			uint8_t *srcp;
			uint8_t fg;
			uint8_t bg;
			uint8_t set;

			fg = rowattr[srcrow].fg;
			bg = rowattr[srcrow].bg;
			set = fg ^ bg;
			if (set == 0) {
				/* use fg since both can be acceptable */
				set = fg;
			} else if ((set & fg) != 0) {
				/*
				 * set is the set of bits that set in fg and
				 * cleared in bg.
				 */
				set &= fg;
			} else {
				/*
				 * otherwise, set is the set of bits that
				 * (probably) set in bg and cleared in fg.
				 */
				uint8_t tmp;

				set &= bg;
				/* and swap fg and bg */
				tmp = fg;
				fg = bg;
				bg = tmp;
			}

			for (i = 0; i < planecount; i++) {
				int t = om_fgbg2rop(fg, bg);
				rop[i] = t;
				om_set_rop(i, rop[i], ALL1BITS);
				fg >>= 1;
				bg >>= 1;
			}

			/*
			 * If any bit in 'set' is set, any of them can be used.
			 * If all bits in 'set' are cleared, use plane 0.
			 * srcplane is the plane that fg is set and bg is
			 * cleared.
			 */
			srcplane = (set != 0) ? (31 - __builtin_clz(set)) : 0;

			srcp = src + OMFB_PLANEOFFS + srcplane * OMFB_PLANEOFFS;
			om_rascopy_single(planecount, dst, srcp,
			    width, rowheight * r, rop);
		}

skip:
		for (i = 0; i < r; i++) {
			rowattr[dstrow] = rowattr[srcrow];

			srcrow += rowstep;
			dstrow += rowstep;
			src += ptrstep;
			dst += ptrstep;
			nrows--;
		}
	}
}

/*
 * XXX om{1,4}_copycols can be merged, but these are not frequently executed
 * and have low execution costs.  So I'm putting it off for now.
 */

static void
om1_copycols(void *cookie, int startrow, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	uint8_t *sp, *dp, *sq, *dq, *basep;
	int scanspan, height, w, y, srcx, dstx;
	int sb, eb, db, sboff, full, cnt, lnum, rnum;
	uint32_t lmask, rmask, tmp;
	bool sbover;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	srcx = ri->ri_font->fontwidth * srccol;
	dstx = ri->ri_font->fontwidth * dstcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	basep = (uint8_t *)ri->ri_bits + y * scanspan;

	sb = srcx & ALIGNMASK;
	db = dstx & ALIGNMASK;

	om_reset_planemask_and_rop();

	if (db + w <= BLITWIDTH) {
		/* Destination is contained within a single word */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		while (height > 0) {
			GETBITS(P0(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P0(dp));
			dp += scanspan;
			sp += scanspan;
			height--;
		}
		return;
	}

	lmask = (db == 0) ? 0 : ALL1BITS >> db;
	eb = (db + w) & ALIGNMASK;
	rmask = (eb == 0) ? 0 : ALL1BITS << (32 - eb);
	lnum = (32 - db) & ALIGNMASK;
	rnum = (dstx + w) & ALIGNMASK;

	if (lmask != 0)
		full = (w - (32 - db)) / 32;
	else
		full = w / 32;

	sbover = (sb + lnum) >= 32;

	if (dstcol < srccol || srccol + ncols < dstcol) {
		/* copy forward (left-to-right) */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		if (lmask != 0) {
			sboff = sb + lnum;
			if (sboff >= 32)
				sboff -= 32;
		} else {
			sboff = sb;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (lmask != 0) {
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				dp += BYTESDONE;
				if (sbover)
					sp += BYTESDONE;
			}

			for (cnt = full; cnt; cnt--) {
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				sp += BYTESDONE;
				dp += BYTESDONE;
			}

			if (rmask != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	} else {
		/* copy backward (right-to-left) */
		sp = basep + ((srcx + w) / 32) * 4;
		dp = basep + ((dstx + w) / 32) * 4;

		sboff = (srcx + w) & ALIGNMASK;
		sboff -= rnum;
		if (sboff < 0) {
			sp -= BYTESDONE;
			sboff += 32;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (rnum != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
			}

			for (cnt = full; cnt; cnt--) {
				sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
			}

			if (lmask != 0) {
				if (sbover)
					sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	}
}

static void
om4_copycols(void *cookie, int startrow, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	uint8_t *sp, *dp, *sq, *dq, *basep;
	int scanspan, height, w, y, srcx, dstx;
	int sb, eb, db, sboff, full, cnt, lnum, rnum;
	uint32_t lmask, rmask, tmp;
	bool sbover;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	srcx = ri->ri_font->fontwidth * srccol;
	dstx = ri->ri_font->fontwidth * dstcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	basep = (uint8_t *)ri->ri_bits + y * scanspan;

	sb = srcx & ALIGNMASK;
	db = dstx & ALIGNMASK;

	om_reset_planemask_and_rop();

	if (db + w <= BLITWIDTH) {
		/* Destination is contained within a single word */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		while (height > 0) {
			GETBITS(P0(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P0(dp));
			GETBITS(P1(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P1(dp));
			GETBITS(P2(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P2(dp));
			GETBITS(P3(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P3(dp));
			dp += scanspan;
			sp += scanspan;
			height--;
		}
		return;
	}

	lmask = (db == 0) ? 0 : ALL1BITS >> db;
	eb = (db + w) & ALIGNMASK;
	rmask = (eb == 0) ? 0 : ALL1BITS << (32 - eb);
	lnum = (32 - db) & ALIGNMASK;
	rnum = (dstx + w) & ALIGNMASK;

	if (lmask != 0)
		full = (w - (32 - db)) / 32;
	else
		full = w / 32;

	sbover = (sb + lnum) >= 32;

	if (dstcol < srccol || srccol + ncols < dstcol) {
		/* copy forward (left-to-right) */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		if (lmask != 0) {
			sboff = sb + lnum;
			if (sboff >= 32)
				sboff -= 32;
		} else {
			sboff = sb;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (lmask != 0) {
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				GETBITS(P1(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P1(dp));
				GETBITS(P2(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P2(dp));
				GETBITS(P3(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P3(dp));
				dp += BYTESDONE;
				if (sbover)
					sp += BYTESDONE;
			}

			for (cnt = full; cnt; cnt--) {
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				GETBITS(P1(sp), sboff, 32, tmp);
				*P1(dp) = tmp;
				GETBITS(P2(sp), sboff, 32, tmp);
				*P2(dp) = tmp;
				GETBITS(P3(sp), sboff, 32, tmp);
				*P3(dp) = tmp;
				sp += BYTESDONE;
				dp += BYTESDONE;
			}

			if (rmask != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
				GETBITS(P1(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P1(dp));
				GETBITS(P2(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P2(dp));
				GETBITS(P3(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P3(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	} else {
		/* copy backward (right-to-left) */
		sp = basep + ((srcx + w) / 32) * 4;
		dp = basep + ((dstx + w) / 32) * 4;

		sboff = (srcx + w) & ALIGNMASK;
		sboff -= rnum;
		if (sboff < 0) {
			sp -= BYTESDONE;
			sboff += 32;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (rnum != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
				GETBITS(P1(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P1(dp));
				GETBITS(P2(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P2(dp));
				GETBITS(P3(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P3(dp));
			}

			for (cnt = full; cnt; cnt--) {
				sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				GETBITS(P1(sp), sboff, 32, tmp);
				*P1(dp) = tmp;
				GETBITS(P2(sp), sboff, 32, tmp);
				*P2(dp) = tmp;
				GETBITS(P3(sp), sboff, 32, tmp);
				*P3(dp) = tmp;
			}

			if (lmask != 0) {
				if (sbover)
					sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				GETBITS(P1(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P1(dp));
				GETBITS(P2(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P2(dp));
				GETBITS(P3(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P3(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	}
}

/*
 * Map a character.
 */
static int
om_mapchar(void *cookie, int c, u_int *cp)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *wf = ri->ri_font;

	if (wf->encoding != WSDISPLAY_FONTENC_ISO) {
		c = wsfont_map_unichar(wf, c);

		if (c < 0)
			goto fail;
	}
	if (c < wf->firstchar || c >= (wf->firstchar + wf->numchars))
		goto fail;

	*cp = c;
	return 5;

 fail:
	*cp = ' ';
	return 0;
}

/*
 * Position|{enable|disable} the cursor at the specified location.
 */
static void
om_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	int startx;
	int width;
	int height;
	int sh, sl;
	int y;
	int scanspan;
	uint8_t *p;

	if (!on) {
		/* make sure it's on */
		if ((ri->ri_flg & RI_CURSOR) == 0)
			return;

		row = ri->ri_crow;
		col = ri->ri_ccol;
	} else {
		/* unpaint the old copy. */
		ri->ri_crow = row;
		ri->ri_ccol = col;
	}

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * col;
	width = ri->ri_font->fontwidth;
	height = ri->ri_font->fontheight;
	sh = startx >> 5;
	sl = startx & 0x1f;
	p = (uint8_t *)ri->ri_bits + y * scanspan + sh * 4;

	/* ROP_INV2 ignores data from MPU and inverts the current VRAM data */
	om_fill(hwplanemask, ROP_INV2, p, sl, scanspan, 0, width, height);

	ri->ri_flg ^= RI_CURSOR;

	/* reset mask value */
	om_reset_planemask_and_rop();
}

/*
 * Allocate attribute. We just pack these into an integer.
 *
 * Attribute bitmap:
 *  b31:    Multi color (used by copyrows)
 *  b30-18: 0 (reserved)
 *  b17:    Underline (not supported yet)
 *  b16:    Bold (or HILIT if 1bpp; not supported yet)
 *  b15-8:  fg color code
 *  b7-0:   bg color code
 */
#if 0
/*
 * Future plan:
 * Place fg and bg side by side in advance to reduce the computation cost
 * at the time of ROP setting.
 *
 * bit: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *      f7 b7 f6 b6 f5 b5 f4 b4 f3 b3 f2 b2 f1 b1 f0 b0
 *
 * In this form, use bit1..0 if 1bpp, use bit7..0 if 4bpp.
 */
#endif
static int
om_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{
	struct rasops_info *ri = cookie;
	int planecount = ri->ri_depth;
	uint32_t a;
	uint16_t c;

	a = 0;
	c = 0;

	if ((flags & WSATTR_BLINK) != 0)
		return EINVAL;

	if ((flags & WSATTR_WSCOLORS) == 0) {
		fg = WSCOL_WHITE;	/* maybe 7 or 1 */
		bg = WSCOL_BLACK;	/* maybe 0 */
	}

	if ((flags & WSATTR_REVERSE) != 0) {
		int tmp;
		tmp = fg;
		fg = bg;
		bg = tmp;
	}

	if ((flags & WSATTR_HILIT) != 0) {
		if (planecount == 1) {
#if 0
			a |= OMFB_ATTR_BOLD;
#else
			return EINVAL;
#endif
		} else if (fg < 8) {
			fg += 8;
		}
	}

	if ((flags & WSATTR_UNDERLINE) != 0) {
#if 0
		a |= OMFB_ATTR_UNDERLINE;
#else
		return EINVAL;
#endif
	}

	fg &= hwplanemask;
	bg &= hwplanemask;

#if 0
	int i;
	for (i = 0; i < planecount; i++) {
		c += c;
		c += ((fg & 1) << 1) | (bg & 1);
		fg >>= 1;
		bg >>= 1;
	}
#else
	c = (fg  << 8) | bg;
#endif
	a |= c;

	*attrp = a;
	return 0;
}

static void
om_unpack_attr(long attr, uint8_t *fg, uint8_t *bg, int *underline)
{
	uint8_t f, b;

	f = (attr >> 8) & hwplanemask;
	b = attr & hwplanemask;

	if (fg)
		*fg = f;
	if (bg)
		*bg = b;
}

/*
 * Init subset of rasops(9) for omrasops.
 */
int
omrasops1_init(struct rasops_info *ri, int wantrows, int wantcols)
{

	omrasops_init(ri, wantrows, wantcols);

	/* fill our own emulops */
	ri->ri_ops.cursor    = om_cursor;
	ri->ri_ops.mapchar   = om_mapchar;
	ri->ri_ops.putchar   = om_putchar;
	ri->ri_ops.copycols  = om1_copycols;
	ri->ri_ops.erasecols = om_erasecols;
	ri->ri_ops.copyrows  = om1_copyrows;
	ri->ri_ops.eraserows = om_eraserows;
	ri->ri_ops.allocattr = om_allocattr;
	ri->ri_caps = WSSCREEN_REVERSE;

	ri->ri_flg |= RI_CFGDONE;

	return 0;
}

int
omrasops4_init(struct rasops_info *ri, int wantrows, int wantcols)
{

	omrasops_init(ri, wantrows, wantcols);

	/* fill our own emulops */
	ri->ri_ops.cursor    = om_cursor;
	ri->ri_ops.mapchar   = om_mapchar;
	ri->ri_ops.putchar   = om_putchar;
	ri->ri_ops.copycols  = om4_copycols;
	ri->ri_ops.erasecols = om_erasecols;
	ri->ri_ops.copyrows  = om4_copyrows;
	ri->ri_ops.eraserows = om_eraserows;
	ri->ri_ops.allocattr = om_allocattr;
	ri->ri_caps = WSSCREEN_HILIT | WSSCREEN_WSCOLORS | WSSCREEN_REVERSE;

	ri->ri_flg |= RI_CFGDONE;

	return 0;
}

static int
omrasops_init(struct rasops_info *ri, int wantrows, int wantcols)
{
	int wsfcookie, bpp;

	if (wantrows > OMRASOPS_MAX_ROWS)
		wantrows = OMRASOPS_MAX_ROWS;
	if (wantrows == 0)
		wantrows = 34;
	if (wantrows < 10)
		wantrows = 10;
	if (wantcols == 0)
		wantcols = 80;
	if (wantcols < 20)
		wantcols = 20;

	/* Use default font */
	wsfont_init();
	wsfcookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (wsfcookie < 0)
		panic("%s: no font available", __func__);
	if (wsfont_lock(wsfcookie, &ri->ri_font))
		panic("%s: unable to lock font", __func__);
	ri->ri_wsfcookie = wsfcookie;

	KASSERT(ri->ri_font->fontwidth > 4 && ri->ri_font->fontwidth <= 32);

	/* all planes are independently addressed */
	bpp = 1;

	/* Now constrain what they get */
	ri->ri_emuwidth = ri->ri_font->fontwidth * wantcols;
	ri->ri_emuheight = ri->ri_font->fontheight * wantrows;
	if (ri->ri_emuwidth > ri->ri_width)
		ri->ri_emuwidth = ri->ri_width;
	if (ri->ri_emuheight > ri->ri_height)
		ri->ri_emuheight = ri->ri_height;

	/* Reduce width until aligned on a 32-bit boundary */
	while ((ri->ri_emuwidth * bpp & 31) != 0)
		ri->ri_emuwidth--;

	ri->ri_cols = ri->ri_emuwidth / ri->ri_font->fontwidth;
	ri->ri_rows = ri->ri_emuheight / ri->ri_font->fontheight;
	ri->ri_emustride = ri->ri_emuwidth * bpp >> 3;
	ri->ri_ccol = 0;
	ri->ri_crow = 0;
	ri->ri_pelbytes = bpp >> 3;

	ri->ri_xscale = (ri->ri_font->fontwidth * bpp) >> 3;
	ri->ri_yscale = ri->ri_font->fontheight * ri->ri_stride;
	ri->ri_fontscale = ri->ri_font->fontheight * ri->ri_font->stride;

	/* Clear the entire display */
	if ((ri->ri_flg & RI_CLEAR) != 0)
		memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height);

	/* Now centre our window if needs be */
	ri->ri_origbits = ri->ri_bits;

	if ((ri->ri_flg & RI_CENTER) != 0) {
		ri->ri_bits += (((ri->ri_width * bpp >> 3) -
		    ri->ri_emustride) >> 1) & ~3;
		ri->ri_bits += ((ri->ri_height - ri->ri_emuheight) >> 1) *
		    ri->ri_stride;
		ri->ri_yorigin = (int)(ri->ri_bits - ri->ri_origbits)
		   / ri->ri_stride;
		ri->ri_xorigin = (((int)(ri->ri_bits - ri->ri_origbits)
		   % ri->ri_stride) * 8 / bpp);
	} else
		ri->ri_xorigin = ri->ri_yorigin = 0;

	return 0;
}
