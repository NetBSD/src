/* 	$NetBSD: rasops_bitops.h,v 1.3 1999/05/18 21:51:59 ad Exp $ */

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

#ifndef _RASOPS_BITOPS_H_
#define _RASOPS_BITOPS_H_ 1

/*
 * Erase columns.
 */
static void
NAME(erasecols)(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	int32_t *dp, *rp, lmask, rmask, lclr, rclr, clr;
	struct rasops_info *ri;
	int height, cnt;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if (col < 0) {
		num += col;
		col = 0;
	}

	if ((col + num) > ri->ri_cols)
		num = ri->ri_cols - col;
	
	if (num <= 0)
		return;
#endif
	col *= ri->ri_font->fontwidth << PIXEL_SHIFT;
	num *= ri->ri_font->fontwidth << PIXEL_SHIFT;
	height = ri->ri_font->fontheight;
	clr = ri->ri_devcmap[(attr >> 16) & 15];	
	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + ((col >> 3) & ~3));

	if ((col & 31) + num <= 32) {
		lmask = ~rasops_pmask[col & 31][num];
		lclr = clr & ~lmask;

		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, int32_t *);

			*dp = (*dp & lmask) | lclr;
		}
	} else {
		lmask = rasops_rmask[col & 31];
		rmask = rasops_lmask[(col + num) & 31];
		
		if (lmask)
			num = (num - (32 - (col & 31))) >> 5;
		else
			num = num >> 5;
		
		lclr = clr & ~lmask;
		rclr = clr & ~rmask;

		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, int32_t *);

			if (lmask)
				*dp++ = (*dp & lmask) | lclr;

			for (cnt = num; cnt > 0; cnt--)
				*dp++ = clr;

			if (rmask)
				*dp = (*dp & rmask) | rclr;
		}
	}
}


/*
 * Actually paint the cursor.
 */
static void
NAME(do_cursor)(ri)
	struct rasops_info *ri;
{
	int32_t *dp, *rp, lmask, rmask;
	int height, row, col, num;
	
	row = ri->ri_crow;
	col = ri->ri_ccol * ri->ri_font->fontwidth << PIXEL_SHIFT;
	height = ri->ri_font->fontheight;
	num = ri->ri_font->fontwidth << PIXEL_SHIFT;
	rp = (int32_t *)(ri->ri_bits + row * ri->ri_yscale + ((col >> 3) & ~3));
	
	if ((col & 31) + num <= 32) {
		lmask = rasops_pmask[col & 31][num];

		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, int32_t *);
			*dp ^= lmask;
		}
	} else {
		lmask = ~rasops_rmask[col & 31];
		rmask = ~rasops_lmask[(col + num) & 31];

		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, int32_t *);

			if (lmask != -1)
				*dp++ ^= lmask;

			if (rmask != -1)
				*dp ^= rmask;
		}
	}
}


/*
 * Copy columns. This is slow. Ick!
 */
static void
NAME(copycols)(cookie, row, src, dst, num)
	void *cookie;
	int row, src, dst, num;
{
	int32_t *sp, *dp, *srp, *drp, tmp, lmask, rmask;
	int height, lnum, rnum, sb, db, cnt, full;
	struct rasops_info *ri;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING
	if (dst == src)
		return;
		
	/* Catches < 0 case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;
	
	if (src < 0) {
		num += src;
		src = 0;
	}

	if ((src + num) > ri->ri_cols)
		num = ri->ri_cols - src;

	if (dst < 0) {
		num += dst;
		dst = 0;
	}

	if ((dst + num) > ri->ri_cols)
		num = ri->ri_cols - dst;
	
	if (num <= 0)
		return;
#endif
	
	/* XXX pacify gcc until this is fixed XXX */
	db = 0;
	
	cnt = ri->ri_font->fontwidth << PIXEL_SHIFT;
	src *= cnt;
	dst *= cnt;
	num *= cnt;
	row *= ri->ri_yscale;
	height = ri->ri_font->fontheight;

	if (db + num <= 32) { 
		srp = (int32_t *)(ri->ri_bits + row + ((src >> 3) & ~3));
		drp = (int32_t *)(ri->ri_bits + row + ((dst >> 3) & ~3));
		sb = src & 31;
		db = dst & 31;

		while (height--) {
			GETBITS(srp, sb, num, tmp);
			PUTBITS(tmp, db, num, drp);
			DELTA(srp, ri->ri_stride, int32_t *); 
			DELTA(drp, ri->ri_stride, int32_t *); 
		}
		
		return;
	}

	lmask = rasops_rmask[dst & 31];
	rmask = rasops_lmask[(dst + num) & 31];

	lnum = (lmask ? 32 - db : 0);  
	rnum = (rmask ? (dst + num) & 31 : 0); 
	sb = src < dst && src + num > dst;
		
	if (lmask)
		full = (num - (32 - (dst & 31))) >> 5;
	else
		full = num >> 5;

	if (sb) {
		/* Go backwards */
		/* XXX this is broken! */
		src = src + num;
		dst = dst + num;
		srp = (int32_t *)(ri->ri_bits + row + ((src >> 3) & ~3));
		drp = (int32_t *)(ri->ri_bits + row + ((dst >> 3) & ~3));

		src = src & 31;
		db = dst & 31;

		while (height--) {
			sb = src;
			sp = srp;
			dp = drp;
			DELTA(srp, ri->ri_stride, int32_t *); 
			DELTA(drp, ri->ri_stride, int32_t *);
			
			if (db) {
				GETBITS(sp, sb, db, tmp);
				PUTBITS(tmp, db, db, dp);
				dp--;
				if ((sb -= rnum) < 0) {
					sp--;
					sb += 32;
				}
			}

			/* Now we're aligned to 32-bits with dp :) */
			for (cnt = full; cnt; cnt--, sp--) {
				GETBITS(sp, sb, 32, tmp);
				*dp-- = tmp;
			}

			if (lmask) {
				GETBITS(sp, sb, rnum, tmp);
				PUTBITS(tmp, 0, rnum, dp);
 			}
 		}
	} else {
		srp = (int32_t *)(ri->ri_bits + row + ((src >> 3) & ~3));
		drp = (int32_t *)(ri->ri_bits + row + ((dst >> 3) & ~3));
		db = dst & 31;

		while (height--) {
			sb = src & 31;
			sp = srp;
			dp = drp;
			DELTA(srp, ri->ri_stride, int32_t *); 
			DELTA(drp, ri->ri_stride, int32_t *); 
		
			if (lmask) {
				GETBITS(sp, sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, dp);
				dp++;
			
				if ((sb += lnum) > 31) {
					sp++;
					sb -= 32;
				}
			}
	
			/* Now we're aligned to 32-bits with dp :) */
			for (cnt = full; cnt; cnt--) {
				GETBITS(sp, sb, 32, tmp);
				*dp++ = tmp;
				sp++;
			}

			if (rmask) {
				GETBITS(sp, sb, rnum, tmp);
				PUTBITS(tmp, 0, rnum, dp);
 			}
 		}
 	}
}

#endif /* _RASOPS_BITOPS_H_ */
