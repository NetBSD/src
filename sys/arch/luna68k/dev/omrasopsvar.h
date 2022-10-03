/* $NetBSD: omrasopsvar.h,v 1.8 2022/10/03 17:42:35 tsutsui Exp $ */
/*
 * Copyright (c) 2013 Kenji Aoyama
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <machine/board.h>

/*
 * Base addresses of LUNA's frame buffer
 * XXX: We consider only 1bpp and 4bpp for now
 */

#define OMFB_PLANEMASK		BMAP_BMSEL	/* BMSEL register */
#define OMFB_ROP_COMMON		BMAP_FN		/* common ROP */
#define OMFB_ROP_P0		BMAP_FN0

/* will be merged in near future */
#define OMFB_ROPFUNC		BMAP_FN		/* common ROP function */

#define OMFB_MAX_PLANECOUNT	(8)
#define OMFB_PLANEOFFS		(0x40000)	/* plane offset */
#define OMFB_STRIDE		(2048/8)	/* stride [byte] */

/* TODO: should be improved... */
extern int hwplanemask;

/*
 * ROP function
 *
 * LUNA's frame buffer uses Hitachi HM53462 video RAM, which has raster
 * (logic) operation, or ROP, function.  To use ROP function on LUNA, write
 * a 32bit `mask' value to the specified address corresponding to each ROP
 * logic.
 *
 * D: the data writing to the video RAM
 * M: the data already stored on the video RAM
 */

/* operation		index	the video RAM contents will be */
#define ROP_ZERO	 0	/* all 0	*/
#define ROP_AND1	 1	/* D & M	*/
#define ROP_AND2	 2	/* ~D & M	*/
/* Not used on LUNA	 3			*/
#define ROP_AND3	 4	/* D & ~M	*/
#define ROP_THROUGH	 5	/* D		*/
#define ROP_EOR		 6	/* (~D & M) | (D & ~M)	*/
#define ROP_OR1		 7	/* D | M	*/
#define ROP_NOR		 8	/* ~D | ~M	*/
#define ROP_ENOR	 9	/* (D & M) | (~D & ~M)	*/
#define ROP_INV1	10	/* ~D		*/
#define ROP_OR2		11	/* ~D | M	*/
#define ROP_INV2	12	/* ~M		*/
#define ROP_OR3		13	/* D | ~M	*/
#define ROP_NAND	14	/* ~D | ~M	*/
#define ROP_ONE		15	/* all 1	*/

int omrasops1_init(struct rasops_info *, int, int);
int omrasops4_init(struct rasops_info *, int, int);
