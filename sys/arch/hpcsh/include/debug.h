/*	$NetBSD: debug.h,v 1.3.118.1 2008/05/16 02:22:32 yamt Exp $	*/

/*-
 * Copyright (c) 1999-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 */

#define USE_HPC_DPRINTF
#define __DPRINTF_EXT
#include <hpc/include/debug.h>

#include "debug_hpc.h"
/*
 * interrupt monitor
 */
#ifdef HPC_DEBUG_INTERRUPT_MONITOR
enum heart_beat {
	HEART_BEAT_BLACK = 0,
	HEART_BEAT_RED,
	HEART_BEAT_GREEN,
	HEART_BEAT_YELLOW,
	HEART_BEAT_BLUE,
	HEART_BEAT_MAGENTA,
	HEART_BEAT_CYAN,
	HEART_BEAT_WHITE,
};
void __dbg_heart_beat(enum heart_beat);
#else
#define __dbg_heart_beat(x)	((void)0)
#endif /* HPC_DEBUG_INTERRUPT_MONITOR */

