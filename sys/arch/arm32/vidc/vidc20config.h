/* $NetBSD: vidc20config.h,v 1.1.2.2 2001/03/27 15:30:33 bouyer Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * Copyright (c) 2001 Reinoud Zandijk
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
 *
 * Video department of VIDC chip for wscons
 *
 * Created:	16/03/2001
 *
 */

#ifndef _VIDC20CONFIG_H_
#define _VIDC20CONFIG_H_

extern int  vidcvideo_write        __P((u_int reg, int value));
extern int  vidcvideo_cursor_init  __P((int width, int heigth));
extern void vidcvideo_setpalette   __P((struct vidc_state *vidc));
extern int  vidcvideo_textpalette  __P((void));
extern void vidcvideo_stdpalette   __P((void));
extern void vidcvideo_setstate     __P((struct vidc_state *vidc));
extern void vidcvideo_getstate     __P((struct vidc_state *vidc));
extern void vidcvideo_getmode      __P((struct vidc_mode *mode));
extern void vidcvideo_setmode      __P((struct vidc_mode *mode));
extern int  vidcvideo_init         __P((void));
extern void vidcvideo_reinit       __P((void));
extern void vidcvideo_printdetails __P((void));
extern int  vidcvideo_blank	   __P((int));

#endif

/* end of vidcvideo.h */
