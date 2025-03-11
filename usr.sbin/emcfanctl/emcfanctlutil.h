/*	$NetBSD: emcfanctlutil.h,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

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

#ifndef _EMCFANCTLUTIL_H_
#define _EMCFANCTLUTIL_H_

EXTERN int emcfan_read_register(int, uint8_t, uint8_t *, bool);
EXTERN int emcfan_write_register(int, uint8_t, uint8_t, bool);
EXTERN int emcfan_rmw_register(int, uint8_t, uint8_t, const struct emcfan_bits_translate *, long unsigned int, int, bool);
EXTERN int emcfan_product_to_family(uint8_t);
EXTERN char *emcfan_product_to_name(uint8_t);
EXTERN void emcfan_family_to_name(int, char *, int);
EXTERN int emcfan_find_info(uint8_t);
EXTERN bool emcfan_reg_is_real(int, uint8_t);
EXTERN int emcfan_reg_by_name(uint8_t, int, char*);
EXTERN const char *emcfan_regname_by_reg(uint8_t, int, uint8_t);
EXTERN int find_translated_blob_by_bits_instance(const struct emcfan_bits_translate *, long unsigned int, uint8_t, int);
EXTERN int find_translated_bits_by_hint_instance(const struct emcfan_bits_translate *, long unsigned int, int, int);
EXTERN int find_translated_bits_by_hint(const struct emcfan_bits_translate *, long unsigned int, int);
EXTERN int find_translated_bits_by_str(const struct emcfan_bits_translate *,long unsigned int,char *);
EXTERN int find_translated_bits_by_str_instance(const struct emcfan_bits_translate *,long unsigned int,char *, int);
EXTERN int find_human_int(const struct emcfan_bits_translate *, long unsigned int, uint8_t);
EXTERN char *find_human_str(const struct emcfan_bits_translate *, long unsigned int, uint8_t);

#endif
