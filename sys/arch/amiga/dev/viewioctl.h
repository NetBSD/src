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

/* The view major device is a placeholder device.  It serves
 * simply to map the semantics of a graphics dipslay to
 * the semantics of a character block device.  In other
 * words the graphics system as currently built does not like to be
 * refered to by open/close/ioctl.  This device serves as
 * a interface to graphics. */

#include "grf/grf_bitmap.h"
#include "grf/grf_colormap.h"

struct view_size {
    int x;
    int y;
    int width;
    int height;
    int depth;
};

#define VIEW_REMOVE     _IO ('V', 0x0)	/* if displaying remove. */
#define VIEW_DISPLAY    _IO ('V', 0x1)	/* if not displaying, display */
#define VIEW_SETSIZE	_IOW ('V', 0x2, struct view_size)	/* set size */
#define VIEW_GETSIZE	_IOR ('V', 0x3, struct view_size)	/* get size */
#define VIEW_GETBITMAP	_IOR ('V', 0x4, bmap_t)
#define VIEW_USECOLORMAP _IOW ('V', 0x5, colormap_t)
#define VIEW_GETCOLORMAP _IOWR ('V', 0x6, colormap_t)

