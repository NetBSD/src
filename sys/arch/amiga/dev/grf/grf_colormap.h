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
#if ! defined (_GRF_COLORMAP_H)
#define _GRF_COLORMAP_H

enum colormap_type {
    CM_MONO,			/* only on or off allowed */
    CM_GREYSCALE,		/* 0-256 levels of grey */
    CM_COLOR			/* Red: 0-255 Green: 0-255 Blue: 0-255 */
};

/* valid masks are a bitfield of zeros followed by ones that indicate 
 * which mask are valid for each component.  The ones and zeros will 
 * be contiguous so adding one to this value yields the number of
 * levels for that component. 
 * -ch 
 */

struct colormap {
    u_byte type;	/* what type of entries these are. */
    union {
        /* CM_GREYSCALE */
        u_byte grey;
        /* CM_COLOR */
        struct {
            u_byte red;
            u_byte green;
            u_byte blue;
        } rgb_mask;
    } valid_mask;
    u_word first;	/* what color register does entry[0] refer to. */
    u_word size;	/* number of entries */
    u_long *entry;/* the table of actual color values. */
};
#define red_mask   valid_mask.rgb_mask.red
#define green_mask valid_mask.rgb_mask.green
#define blue_mask  valid_mask.rgb_mask.blue
#define grey_mask  valid_mask.grey


#define CM_FIXVAL(x) (0xff&x)

/* these macros are for creating entries */
#define MAKE_COLOR_ENTRY(r,g,b) (CM_FIXVAL(r) << 16 | CM_FIXVAL(g) << 8 | CM_FIXVAL(b))
#define MAKE_MONO_ENTRY(x)	(x ? 1 : 0)
#define MAKE_GREY_ENTRY(l)      CM_FIXVAL(l)

#define CM_LTOW(v) (((0x000F0000&v)>>8)|((0x00000F00&v)>>4)|(0xF&v))
#define CM_WTOL(v) (((0xF00&v)<<8)|((0x0F0&v)<<4)|(0xF&v))

#endif /* _GRF_COLORMAP_H */

