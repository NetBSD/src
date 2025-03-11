/*	$NetBSD: emcfanctloutputs.h,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _EMCFANCTLOUTPUTS_H_
#define _EMCFANCTLOUTPUTS_H_

EXTERN int output_emcfan_info(int, uint8_t, int, bool, bool);
EXTERN void output_emcfan_register_list(uint8_t, int, bool, bool);
EXTERN int output_emcfan_register_read(int, uint8_t, int, uint8_t, uint8_t, bool, bool);
EXTERN int output_emcfan_minexpected_rpm(int, uint8_t, int, uint8_t, bool, bool);
EXTERN int output_emcfan_edges(int, uint8_t, int, uint8_t, bool, bool);
EXTERN int output_emcfan_drive(int, uint8_t, int, uint8_t, bool, bool);
EXTERN int output_emcfan_divider(int, uint8_t, int, uint8_t, bool, bool);
EXTERN int output_emcfan_pwm_basefreq(int, uint8_t, int, uint8_t, int, bool, bool);
EXTERN int output_emcfan_polarity(int, uint8_t, int, uint8_t, int, bool, bool);
EXTERN int output_emcfan_pwm_output_type(int, uint8_t, int, uint8_t, int, bool, bool);
EXTERN int output_emcfan_fan_status(int, uint8_t, int, uint8_t, uint8_t, int, bool, bool);
EXTERN int output_emcfan_apd(int, uint8_t, int, uint8_t, bool, bool);
EXTERN int output_emcfan_smbusto(int, uint8_t, int, uint8_t, int, bool, bool);

#endif
