/*-
 * Copyright (c) 2018 Mindaugas Rasiukevicius <rmind at noxt eu>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_THMAP_H_
#define _SYS_THMAP_H_

#if !defined(_KERNEL)
#error "not supposed to be exposed to userland"
#endif

__BEGIN_DECLS

struct thmap;
typedef struct thmap thmap_t;

#define	THMAP_NOCOPY	0x01
#define	THMAP_SETROOT	0x02

typedef struct {
	uintptr_t	(*alloc)(size_t);
	void		(*free)(uintptr_t, size_t);
} thmap_ops_t;

thmap_t *	thmap_create(uintptr_t, const thmap_ops_t *, unsigned);
void		thmap_destroy(thmap_t *);

void *		thmap_get(thmap_t *, const void *, size_t);
void *		thmap_put(thmap_t *, const void *, size_t, void *);
void *		thmap_del(thmap_t *, const void *, size_t);

void *		thmap_stage_gc(thmap_t *);
void		thmap_gc(thmap_t *, void *);

int		thmap_setroot(thmap_t *, uintptr_t);
uintptr_t	thmap_getroot(const thmap_t *);

__END_DECLS

#endif
