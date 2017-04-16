/*	$NetBSD: offtab.h,v 1.3 2017/04/16 23:50:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	VNDCOMPRESS_OFFTAB_H
#define	VNDCOMPRESS_OFFTAB_H

#include <sys/cdefs.h>

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

struct offtab {
	uint32_t	ot_n_offsets;
	uint32_t	ot_window_size;
	uint32_t	ot_window_start;
	uint64_t	*ot_window;
	uint32_t	ot_blkno;
	int		ot_fd;
	off_t		ot_fdpos;
	void		(*ot_report)(const char *, ...) __printflike(1,2);
	void		(*ot_reportx)(const char *, ...) __printflike(1,2);
	enum offtab_mode {
		OFFTAB_MODE_NONE,
		OFFTAB_MODE_READ,
		OFFTAB_MODE_WRITE,
	}		ot_mode;
};

#define	OFFTAB_MAX_FDPOS						      \
	((off_t)(MIN(OFF_MAX, UINT64_MAX) -				      \
	    (off_t)MAX_N_OFFSETS*sizeof(uint64_t)))

void		offtab_init(struct offtab *, uint32_t, uint32_t, int, off_t);
void		offtab_destroy(struct offtab *);

bool		offtab_transmogrify_read_to_write(struct offtab *, uint32_t);

bool		offtab_reset_read(struct offtab *,
		    void (*)(const char *, ...) __printflike(1,2),
		    void (*)(const char *, ...) __printflike(1,2));
bool		offtab_prepare_get(struct offtab *, uint32_t);
uint64_t	offtab_get(struct offtab *, uint32_t);

void		offtab_reset_write(struct offtab *);
void		offtab_checkpoint(struct offtab *, uint32_t, int);
#define	OFFTAB_CHECKPOINT_SYNC	1
void		offtab_prepare_put(struct offtab *, uint32_t);
void		offtab_put(struct offtab *, uint32_t, uint64_t);

#endif	/* VNDCOMPRESS_OFFTAB_H */
