/*	$NetBSD: internal.h,v 1.1 2021/06/08 18:43:54 nia Exp $	*/

/*-
 * Copyright (c) 1997-2021 The NetBSD Foundation, Inc.
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

#ifndef OSSAUDIO_INTERNAL_H
#define OSSAUDIO_INTERNAL_H

#include <sys/ioctl.h>
#include "soundcard.h"
#undef ioctl

#define TO_OSSVOL(x)	(((x) * 100 + 127) / 255)
#define FROM_OSSVOL(x)	((((x) > 100 ? 100 : (x)) * 255 + 50) / 100)

#define INTARG		(*(int*)argp)

#define GET_DEV(com)	((com) & 0xff)

#define oss_private	__attribute__((__visibility__("hidden")))

int _oss_ioctl(int, unsigned long, ...);

oss_private int _oss_dsp_ioctl(int, unsigned long, void *);
oss_private int _oss_get_caps(int, int *);
oss_private int _oss3_mixer_ioctl(int, unsigned long, void *);
oss_private int _oss4_mixer_ioctl(int, unsigned long, void *);
oss_private int _oss4_global_ioctl(int, unsigned long, void *);

#endif
