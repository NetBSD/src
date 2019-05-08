/*	$NetBSD: linear.h,v 1.2 2019/05/08 13:40:17 isaki Exp $	*/

/*
 * Copyright (C) 2017 Tetsuya Isaki. All rights reserved.
 * Copyright (C) 2017 Y.Sugahara (moveccr). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_DEV_AUDIO_LINEAR_H_
#define _SYS_DEV_AUDIO_LINEAR_H_

#include <dev/audio/audio_if.h>

extern void audio_internal_to_linear8(audio_filter_arg_t *);
extern void audio_internal_to_linear16(audio_filter_arg_t *);
extern void audio_internal_to_linear24(audio_filter_arg_t *);
extern void audio_internal_to_linear32(audio_filter_arg_t *);
extern void audio_linear8_to_internal(audio_filter_arg_t *);
extern void audio_linear16_to_internal(audio_filter_arg_t *);
extern void audio_linear24_to_internal(audio_filter_arg_t *);
extern void audio_linear32_to_internal(audio_filter_arg_t *);

#endif /* !_SYS_DEV_AUDIO_LINEAR_H_ */
