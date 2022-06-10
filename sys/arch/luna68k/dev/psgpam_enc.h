/*	$NetBSD: psgpam_enc.h,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Yosuke Sugahara. All rights reserved.
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

#ifndef _PSGPAM_ENC_H_
#define _PSGPAM_ENC_H_

#define XP_ATN_RELOAD	0x80808080
#define XP_ATN_STAT	0x40404040
#define XP_ATN_RESET	0x20202020

struct psgpam_codecvar {
	int expire;		/* min expire counter */
	int expire_initial;	/* expire initial (as 10Hz) */
	auint_t offset;		/* dynamic offset */

	u_int sample_rate;
};

void psgpam_init_context(struct psgpam_codecvar *, u_int);

void psgpam_aint_to_pam2a(audio_filter_arg_t *);
void psgpam_aint_to_pam2b(audio_filter_arg_t *);
void psgpam_aint_to_pam3a(audio_filter_arg_t *);
void psgpam_aint_to_pam3b(audio_filter_arg_t *);
void psgpam_aint_to_pcm1(audio_filter_arg_t *);
void psgpam_aint_to_pcm2(audio_filter_arg_t *);
void psgpam_aint_to_pcm3(audio_filter_arg_t *);

void psgpam_aint_to_pam2a_d(audio_filter_arg_t *);
void psgpam_aint_to_pam2b_d(audio_filter_arg_t *);
void psgpam_aint_to_pam3a_d(audio_filter_arg_t *);
void psgpam_aint_to_pam3b_d(audio_filter_arg_t *);
void psgpam_aint_to_pcm1_d(audio_filter_arg_t *);
void psgpam_aint_to_pcm2_d(audio_filter_arg_t *);
void psgpam_aint_to_pcm3_d(audio_filter_arg_t *);

#endif /* !_PSGPAM_ENC_H_ */
