/*	$NetBSD: video.h,v 1.2.32.1 2008/05/18 12:32:22 yamt Exp $	*/
/*
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CPU_VIDEO_H_
#define _CPU_VIDEO_H_

#include <sys/types.h>

struct mac68k_video {
	/* From Booter via locore */
	vaddr_t mv_kvaddr;	/* Address used in kernel for video */
	size_t mv_stride;	/* Length of row in video RAM */
	size_t mv_depth;	/* Number of bits per pixel */
	size_t mv_width;	/* Framebuffer width */
	size_t mv_height;	/* Framebuffer height */

	/*
	 * Values for IIvx-like internal video
	 * -- should be zero if it is not used (usual case).
	 *
	 * XXX This doesn't seem to hold true nowadays.  It seems that
	 * these fields are accessed from many places and they are
	 * non-zero of machines with non-internal video.  They should also
	 * have better names to clarify, e.g, why we need mv_log and
	 * mv_kvaddr (do we need to have these two?) -- jmmv 20070829
	 */
	vaddr_t mv_log;		/* logical addr */
	paddr_t mv_phys;	/* physical addr */
	size_t mv_len;		/* mem length */
};

extern struct mac68k_video mac68k_video;

#endif /* _CPU_VIDEO_H_ */
