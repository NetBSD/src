/*	$NetBSD: psgpam_table.h,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

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

#ifndef _PSGPAM_TABLE_H_
#define _PSGPAM_TABLE_H_

#define PCM1_TABLE_BITS 	8
#define PCM2_TABLE_BITS 	8
#define PCM3_TABLE_BITS 	9
#define PAM2A_TABLE_BITS	8
#define PAM2B_TABLE_BITS	8
#define PAM3A_TABLE_BITS	11
#define PAM3B_TABLE_BITS	9

extern const uint8_t  PCM1_TABLE[];
extern const uint8_t  PCM2_TABLE[];
extern const uint16_t PCM3_TABLE[];
extern const uint8_t  PAM2A_TABLE[];
extern const uint8_t  PAM2B_TABLE[];
extern const uint16_t PAM3A_TABLE[];
extern const uint16_t PAM3B_TABLE[];

#endif /* !_PSGPAM_TABLE_H_ */
