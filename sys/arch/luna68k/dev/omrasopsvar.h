/* $NetBSD: omrasopsvar.h,v 1.3 2014/10/04 16:58:17 tsutsui Exp $ */
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

/*
 * Base addresses of LUNA's frame buffer
 * XXX: We consider only 1bpp and 4bpp for now
 */

#define OMFB_PLANEMASK	0xB1040000	/* BMSEL register */
#define OMFB_FB_WADDR	0xB1080008	/* common plane */
#define OMFB_FB_RADDR	0xB10C0008	/* plane #0 */
#define OMFB_ROPFUNC	0xB12C0000	/* common ROP function */

/*
 * Helper macros
 */
#define W(addr)  ((uint32_t *)(addr))
#define R(addr)  ((uint32_t *)((uint8_t *)(addr) +  0x40000))
#define P0(addr) ((uint32_t *)((uint8_t *)(addr) +  0x40000))
#define P1(addr) ((uint32_t *)((uint8_t *)(addr) +  0x80000))
#define P2(addr) ((uint32_t *)((uint8_t *)(addr) +  0xC0000))
#define P3(addr) ((uint32_t *)((uint8_t *)(addr) + 0x100000))

/*
 * ROP function
 *
 * LUNA's frame buffer uses Hitach HM53462 video RAM, which has raster
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
