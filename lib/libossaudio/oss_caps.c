/*	$NetBSD: oss_caps.c,v 1.2 2021/06/09 14:49:13 nia Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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
#include <sys/audioio.h>
#include "internal.h"

oss_private int
_oss_get_caps(int fd, int *out)
{
	struct audio_info info;
	int props, caps;
	int nchannels;

	if (ioctl(fd, AUDIO_GETPROPS, &props) < 0)
		return -1;

	caps = 0;
	caps |= PCM_CAP_TRIGGER;
	caps |= PCM_CAP_MULTI;
	caps |= PCM_CAP_FREERATE;

	if (ioctl(fd, AUDIO_GETFORMAT, &info) != -1) {
		nchannels = (props & AUDIO_PROP_PLAYBACK) ?
		    info.play.channels : info.record.channels;

		switch (nchannels) {
		case 2:
			caps |= DSP_CH_STEREO;
			break;
		case 1:
			caps |= DSP_CH_MONO;
			break;
		default:
			caps |= DSP_CH_MULTI;
			break;
		}
	}

	if (props & AUDIO_PROP_FULLDUPLEX)
		caps |= PCM_CAP_DUPLEX;
	if (props & AUDIO_PROP_MMAP)
		caps |= PCM_CAP_MMAP;
	if (props & AUDIO_PROP_CAPTURE)
		caps |= PCM_CAP_INPUT;
	if (props & AUDIO_PROP_PLAYBACK)
		caps |= PCM_CAP_OUTPUT;

	*out = caps;
	return 0;
}
