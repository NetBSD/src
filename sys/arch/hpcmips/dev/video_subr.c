/*	$NetBSD: video_subr.c,v 1.1 2000/05/08 21:57:56 uch Exp $	*/

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <arch/hpcmips/dev/video_subr.h>

int
cmap_work_alloc(r, g, b, rgb, cnt)
	u_int8_t **r, **g, **b;
	u_int32_t **rgb;
	int cnt;
{
	KASSERT(r && g && b && rgb && LEGAL_CLUT_INDEX(cnt - 1));

	*r = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*g = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*b = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*rgb = malloc(cnt * sizeof(u_int32_t), M_DEVBUF, M_WAITOK);	

	return (!(*r && *g && *b && *rgb));
}

void
cmap_work_free(r, g, b, rgb)
	u_int8_t *r, *g, *b;
	u_int32_t *rgb;
{
	if (r)
		free(r, M_DEVBUF);
	if (g)
		free(g, M_DEVBUF);
	if (b)
		free(b, M_DEVBUF);
	if (rgb)
		free(rgb, M_DEVBUF);
}

void
rgb24_compose(rgb24, r, g, b, cnt)
	u_int32_t *rgb24;
	u_int8_t *r, *g, *b;
	int cnt;
{
	int i;
	KASSERT(rgb24 && r && g && b && LEGAL_CLUT_INDEX(cnt - 1));

	for (i = 0; i < cnt; i++) {
		*rgb24++ = RGB24(r[i], g[i], b[i]);
	}
}

void
rgb24_decompose(rgb24, r, g, b, cnt)
	u_int32_t *rgb24;
	u_int8_t *r, *g, *b;
	int cnt;
{
	int i;
	KASSERT(rgb24 && r && g && b && LEGAL_CLUT_INDEX(cnt - 1));

	for (i = 0; i < cnt; i++) {
		u_int32_t rgb = *rgb24++;
		*r++ = (rgb >> 16) & 0xff;
		*g++ = (rgb >> 8) & 0xff;
		*b++ = rgb & 0xff;
	}
}
