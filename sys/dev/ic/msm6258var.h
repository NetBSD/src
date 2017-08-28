/*	$NetBSD: msm6258var.h,v 1.8.30.1 2017/08/28 17:52:03 skrll Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
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

/*
 * OKI MSM6258 ADPCM voice synthesizer codec.
 */
extern stream_filter_factory_t msm6258_slinear16_to_adpcm;
extern stream_filter_factory_t msm6258_linear8_to_adpcm;
extern stream_filter_factory_t msm6258_adpcm_to_slinear16;
extern stream_filter_factory_t msm6258_adpcm_to_linear8;

/* XXX */
extern void *vs_alloc_msm6258codec(void);
extern void vs_free_msm6258codec(void *);
extern void vs_slinear16be_to_adpcm(void *, void *, const void *, int);
extern void vs_slinear8_to_adpcm(void *, void *, const void *, int);
extern void vs_adpcm_to_slinear16be(void *, void *, int, const void *);
extern void vs_adpcm_to_slinear8(void *, void *, int, const void *);
