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
 *	$Id: grf_draw.h,v 1.2 1994/01/29 06:59:52 chopps Exp $
 */

#if ! defined (_GRF_DRAW_H)
#define _GRF_DRAW_H

#define BOX_2_RECT(b,r) do { \
			     (r)->left = (b)->x; (r)->top = (b)->y; \
			     (r)->right = (b)->x + (b)->width -1; \
			     (r)->bottom = (b)->y + (b)->height -1; \
		       } while (0)

#define RECT_2_BOX(r,b) do { \
			     (b)->x = (r)->left; \
			     (b)->y = (r)->top; \
		             (b)->width = (r)->right - (r)->left +1; \
			     (b)->height = (r)->bottom - (r)->top +1; \
		       } while (0)

#define INIT_BOX(b,xx,yy,ww,hh) do{(b)->x = xx; (b)->y = yy; (b)->width = ww; (b)->height = hh;}while (0)
#define INIT_RECT(rc,l,t,r,b) do{(rc)->left = l; (rc)->right = r; (rc)->top = t; (rc)->bottom = b;}while (0)
#define INIT_POINT(p,xx,yy) do {(p)->x = xx; (p)->y = yy;} while (0)
#define INIT_DIM(d,w,h) do {(d)->width = w; (d)->height = h;} while (0)

#endif /* _GRF_DRAW_H */

