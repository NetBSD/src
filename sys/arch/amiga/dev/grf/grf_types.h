/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: grf_types.h,v 1.2 1994/01/29 07:00:00 chopps Exp $
 */


#if ! defined (_GRF_TYPES_H)
#define _GRF_TYPES_H

#include "sys/types.h"
#include "../../amiga/cc_types.h" /* the user should not have to request these */
#include "../../amiga/dlists.h"   /* just to use the graphics stuff. */

typedef struct point {
    long x;
    long y;
} point_t;

typedef struct dimension {
    u_long width;
    u_long height;
} dimen_t;

typedef struct box {
    long x;
    long y;
    u_long width;
    u_long height;
} box_t;

typedef struct rectangle {
    long left;
    long top;
    long right;
    long bottom;
} rect_t;

/* these are defined on a need to know basis (in their respective files */
typedef struct bitmap bmap_t;
typedef struct colormap colormap_t;
typedef struct display_mode dmode_t;
typedef struct view view_t;
typedef struct monitor monitor_t;

#endif /* _GRF_TYPES_H */
