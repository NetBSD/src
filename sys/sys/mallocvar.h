/*	$NetBSD: mallocvar.h,v 1.1 2003/02/01 06:23:50 thorpej Exp $	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */

#ifndef _SYS_MALLOC_TYPE_H_
#define	_SYS_MALLOC_TYPE_H_

#include <sys/types.h>

#define	M_MAGIC		877983977

/*
 * This structure describes a type of malloc'd memory and carries
 * allocation statistics for that memory.
 */
struct malloc_type {
	struct malloc_type *ks_next;	/* next in list */
	u_long	ks_magic;	 /* indicates valid structure */
	const char *ks_shortdesc;/* short description */

	/* Statistics */
	u_long	ks_inuse;	/* # of packets of this type currently in use */
	u_long	ks_calls;	/* total packets of this type ever allocated */
	u_long 	ks_memuse;	/* total memory held in bytes */
	u_short	ks_limblocks;	/* number of times blocked for hitting limit */
	u_short	ks_mapblocks;	/* number of times blocked for kernel map */
	u_long	ks_maxused;	/* maximum number ever used */
	u_long	ks_limit;	/* most that are allowed to exist */
	u_long	ks_size;	/* sizes of this thing that are allocated */
	u_long	ks_spare;
};

#ifdef _KERNEL
#define	MALLOC_DEFINE_LIMIT(type, shortdesc, longdesc, limit)		\
struct malloc_type type[1] = {						\
	{ NULL, M_MAGIC, shortdesc,					\
	  0, /* ks_inuse */						\
	  0, /* ks_calls */						\
	  0, /* ks_memuse */						\
	  0, /* ks_limblocks */						\
	  0, /* ks_mapblocks */						\
	  0, /* ks_maxused */						\
	  limit }							\
};									\
__link_set_add_data(malloc_types, type)

#define	MALLOC_DEFINE(type, shortdesc, longdesc)			\
	MALLOC_DEFINE_LIMIT(type, shortdesc, longdesc, 0)

#define	MALLOC_DECLARE(type)						\
	extern struct malloc_type type[1]

void	malloc_type_attach(struct malloc_type *);
void	malloc_type_detach(struct malloc_type *);

void	malloc_type_setlimit(struct malloc_type *, u_long);
#endif /* _KERNEL */

#endif /* _SYS_MALLOC_TYPE_H_ */
