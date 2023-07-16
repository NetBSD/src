/*	$NetBSD: rndsource.h,v 1.9 2023/07/16 10:36:11 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_RNDSOURCE_H
#define	_SYS_RNDSOURCE_H

#ifndef _KERNEL			/* XXX */
#error <sys/rndsource.h> is meant for kernel consumers only.
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/rndio.h>
#include <sys/rngtest.h>

struct percpu;

/*
 * struct rnd_delta_estimator
 *
 *	State for the time-delta entropy estimation model.
 */
typedef struct rnd_delta_estimator {
	uint64_t	x;
	uint64_t	dx;
	uint64_t	d2x;
	uint64_t	insamples;
	uint64_t	outbits;
} rnd_delta_t;

/*
 * struct krndsource
 *
 *	State for an entropy source.  To be allocated by a driver for a
 *	hardware entropy source, and treated as opaque.
 *
 *	There are a number of unused fields in order to preserve ABI
 *	compatibility with the old implementation.
 */
struct krndsource {
	LIST_ENTRY(krndsource) list;	/* the linked list */
	char		name[16];	/* device name */
	rnd_delta_t	time_delta;	/* time samples added while cold */
	rnd_delta_t	value_delta;	/* value samples added whiel cold */
	uint32_t	total;		/* number of bits added while cold */
	uint32_t	type;		/* type, RND_TYPE_* */
	uint32_t	flags;		/* flags, RND_FLAG_* */
	void		*state;		/* percpu (struct rndsource_cpu *) */
	size_t		test_cnt;	/* unused */
	void		(*get)(size_t, void *);	/* pool wants N bytes (badly) */
	void		*getarg;	/* argument to get-function */
	void		(*enable)(struct krndsource *, bool); /* unused */
	rngtest_t	*test;		/* unused */
	unsigned	refcnt;		/* unused */
};

typedef struct krndsource krndsource_t;

void	rndsource_setcb(struct krndsource *, void (*)(size_t, void *), void *);
void	rnd_attach_source(struct krndsource *, const char *, uint32_t,
	    uint32_t);
void	rnd_detach_source(struct krndsource *);

void	_rnd_add_uint32(struct krndsource *, uint32_t); /* legacy */
void	_rnd_add_uint64(struct krndsource *, uint64_t); /* legacy */

void	rnd_add_uint32(struct krndsource *, uint32_t);
void	rnd_add_data(struct krndsource *, const void *, uint32_t, uint32_t);
void	rnd_add_data_sync(struct krndsource *, const void *, uint32_t,
	    uint32_t);

#endif	/* _SYS_RNDSOURCE_H */
