/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 */

#if ! defined (_GRF_BITMAP_H)
#define _GRF_BITMAP_H

/* Note structure is 5 long words big.  This may come in handy for
 * contiguous allocations 
 * 
 * Please do fill in everything correctly this is the main input for
 * all other programs.  In other words all problems with RTG start here.
 * If you do not mimic everyone else exactly problems will appear.
 * If you need a template look at alloc_bitmap() in grf_cc.c.
 *
 * WARNING: the plane array is only for convience, all data for bitplanes 
 *          MUST be contiguous.  This is for mapping purposes.  The reason
 *          for the plane pointers and row_mod is to support interleaving
 *          on monitors that wish to support this. 
 *
 * 2nd Warning: Also don't get funky with these pointers you are expected
 *              to place the start of mappable plane data in ``hardware_address'',
 *              ``hardware_address'' is the only thing that /dev/view checks and it expects
 *              the planes to follow with no padding in between.  If you have
 *              special alignment requirements make use of the given fields
 *              so that the entire contiguous plane data is exactly:
 *              bytes_per_row*height*depth long starting at the physical address
 *              contained within hardware_address.
 *
 * Final Warning: Plane data must begin on a PAGE address and the allocation must
 *                be ``n'' PAGES big do to mapping requirements (otherwise the
 *                user could write over non-allocated memory.
 * -ch
 */

struct bitmap {
    u_word   bytes_per_row;			  /* number of bytes per display row. */
    u_word   row_mod;				  /* number of bytes to reach next row. */
    u_word   rows;				  /* number of display rows. */
    u_word   depth;				  /* depth of bitmap. */
    u_word   flags;				  /* flags. */
    u_word   pad;
    u_byte  *blit_temp;				  /* private monitor buffer. */
    u_byte **plane;				  /* plane data for bitmap. */
    u_byte  *hardware_address;			  /* mappable bitplane pointer. */
};

enum bitmap_flag_bits {
    BMB_CLEAR,					  /* init only. */
    BMB_INTERLEAVED,				  /* init/read. */
};

enum bitmap_flags {
    BMF_CLEAR = 1 << BMB_CLEAR,			  /* init only. */
    BMF_INTERLEAVED = 1 << BMB_INTERLEAVED,	  /* init/read. */
};

/* Use these macros to find misc. sizes of actual bitmap */
#define BM_WIDTH(b)        (b->bytes_per_row << 3)                                  
#define BM_HEIGHT(b)	   (b->rows)                                                
#define BM_ROW(b,p,l)	   (b->plane[p] + ((b->bytes_per_row+b->row_mod)*l))


#endif /* _GRF_BITMAP_H */
