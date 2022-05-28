/* $NetBSD: vidc20config.h,v 1.8 2022/05/28 22:33:42 andvar Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *
 * Video department of VIDC chip for wscons
 *
 * Created:	16/03/2001
 *
 */

#ifndef _VIDC20CONFIG_H_
#define _VIDC20CONFIG_H_

extern int   vidcvideo_write          (u_int reg, int value);
extern int   vidcvideo_cursor_init    (int width, int height);
extern void  vidcvideo_setpalette     (struct vidc_state *vidc);
extern void  vidcvideo_stdpalette     (void);
extern void  vidcvideo_setstate       (struct vidc_state *vidc);
extern void  vidcvideo_getstate       (struct vidc_state *vidc);
extern void  vidcvideo_getmode        (struct vidc_mode *mode);
extern void  vidcvideo_setmode        (struct vidc_mode *mode);
extern int   vidcvideo_init           (void);
extern void  vidcvideo_reinit         (void);
extern void  vidcvideo_printdetails   (void);
extern int   vidcvideo_blank          (int);
extern void *vidcvideo_hwscroll       (int);
extern void *vidcvideo_hwscroll_reset (void);
extern void *vidcvideo_hwscroll_back  (void *cookie);
extern void  vidcvideo_progr_scroll   (void);
extern void  vidcvideo_updatecursor   (int, int);
extern void  vidcvideo_enablecursor   (int);


#define CURSOR_MAX_WIDTH	32
#define CURSOR_MAX_HEIGHT	32	/* for now */
#define CURSOR_MAX_COLOURS	4	/* AFAIK   */

#endif

/* end of vidcvideo.h */
