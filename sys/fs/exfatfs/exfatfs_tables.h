/* $NetBSD: exfatfs_tables.h,v 1.1.2.3 2024/07/24 00:38:26 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
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

#ifndef EXFATFS_TABLES_H_
#define EXFATFS_TABLES_H_

#include <sys/queue.h>

struct exfatfs_upcase_range_offset {
	STAILQ_ENTRY(exfatfs_upcase_range_offset) euro_list;
	uint16_t euro_begin; /* First valid character */
	uint16_t euro_end;   /* First invalid character */
	int16_t  euro_ucoff; /* Offset of uppercase version of characters */
};

int exfatfs_check_filename_ucs2(struct exfatfs *, uint16_t *, int);
void exfatfs_load_uctable(struct exfatfs *, const uint16_t *, int);
void exfatfs_destroy_uctable(struct exfatfs *);
void exfatfs_upcase_str(struct exfatfs *, uint16_t *, int);
int exfatfs_upcase_cmp(struct exfatfs *, uint16_t *, int, uint16_t *, int);

#endif /* EXFATFS_TABLE_H_ */
