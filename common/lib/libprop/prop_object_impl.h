/*	$NetBSD: prop_object_impl.h,v 1.8 2006/10/15 19:08:48 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _PROPLIB_PROP_OBJECT_IMPL_H_
#define	_PROPLIB_PROP_OBJECT_IMPL_H_

#if defined(_KERNEL) || defined(_STANDALONE)
#include <lib/libkern/libkern.h>
#else
#include <inttypes.h>
#endif

struct _prop_object_externalize_context {
	char *		poec_buf;		/* string buffer */
	size_t		poec_capacity;		/* capacity of buffer */
	size_t		poec_len;		/* current length of string */
	unsigned int	poec_depth;		/* nesting depth */
};

boolean_t	_prop_object_externalize_start_tag(
				struct _prop_object_externalize_context *,
				const char *);
boolean_t	_prop_object_externalize_end_tag(
				struct _prop_object_externalize_context *,
				const char *);
boolean_t	_prop_object_externalize_empty_tag(
				struct _prop_object_externalize_context *,
				const char *);
boolean_t	_prop_object_externalize_append_cstring(
				struct _prop_object_externalize_context *,
				const char *);
boolean_t	_prop_object_externalize_append_encoded_cstring(
				struct _prop_object_externalize_context *,
				const char *);
boolean_t	_prop_object_externalize_append_char(
				struct _prop_object_externalize_context *,
				unsigned char);
boolean_t	_prop_object_externalize_header(
				struct _prop_object_externalize_context *);
boolean_t	_prop_object_externalize_footer(
				struct _prop_object_externalize_context *);

struct _prop_object_externalize_context *
		_prop_object_externalize_context_alloc(void);
void		_prop_object_externalize_context_free(
				struct _prop_object_externalize_context *);

typedef enum {
	_PROP_TAG_TYPE_START,			/* e.g. <dict> */
	_PROP_TAG_TYPE_END,			/* e.g. </dict> */
	_PROP_TAG_TYPE_EITHER
} _prop_tag_type_t;

struct _prop_object_internalize_context {
	const char *poic_xml;
	const char *poic_cp;

	const char *poic_tag_start;

	const char *poic_tagname;
	size_t      poic_tagname_len;
	const char *poic_tagattr;
	size_t      poic_tagattr_len;
	const char *poic_tagattrval;
	size_t      poic_tagattrval_len;

	boolean_t   poic_is_empty_element;
	_prop_tag_type_t poic_tag_type;
};

#define	_PROP_EOF(c)		((c) == '\0')
#define	_PROP_ISSPACE(c)	\
	((c) == ' ' || (c) == '\t' || (c) == '\n' || _PROP_EOF(c))

#define	_PROP_TAG_MATCH(ctx, t)					\
	_prop_object_internalize_match((ctx)->poic_tagname,	\
				       (ctx)->poic_tagname_len,	\
				       (t), strlen(t))

#define	_PROP_TAGATTR_MATCH(ctx, a)				\
	_prop_object_internalize_match((ctx)->poic_tagattr,	\
				       (ctx)->poic_tagattr_len,	\
				       (a), strlen(a))

#define	_PROP_TAGATTRVAL_MATCH(ctx, a)				  \
	_prop_object_internalize_match((ctx)->poic_tagattrval,	  \
				       (ctx)->poic_tagattrval_len,\
				       (a), strlen(a))

boolean_t	_prop_object_internalize_find_tag(
				struct _prop_object_internalize_context *,
				const char *, _prop_tag_type_t);
boolean_t	_prop_object_internalize_match(const char *, size_t,
					       const char *, size_t);
prop_object_t	_prop_object_internalize_by_tag(
				struct _prop_object_internalize_context *);
boolean_t	_prop_object_internalize_decode_string(
				struct _prop_object_internalize_context *,
				char *, size_t, size_t *, const char **);

struct _prop_object_internalize_context *
		_prop_object_internalize_context_alloc(const char *);
void		_prop_object_internalize_context_free(
				struct _prop_object_internalize_context *);

#if !defined(_KERNEL) && !defined(_STANDALONE)
boolean_t	_prop_object_externalize_write_file(const char *,
						    const char *, size_t);

struct _prop_object_internalize_mapped_file {
	char *	poimf_xml;
	size_t	poimf_mapsize;
};

struct _prop_object_internalize_mapped_file *
		_prop_object_internalize_map_file(const char *);
void		_prop_object_internalize_unmap_file(
				struct _prop_object_internalize_mapped_file *);
#endif /* !_KERNEL && !_STANDALONE */

	/* These are here because they're required by shared code. */
prop_object_t	_prop_array_internalize(
				struct _prop_object_internalize_context *);
prop_object_t	_prop_bool_internalize(
				struct _prop_object_internalize_context *);
prop_object_t	_prop_data_internalize(
				struct _prop_object_internalize_context *);
prop_object_t	_prop_dictionary_internalize(
				struct _prop_object_internalize_context *);
prop_object_t	_prop_number_internalize(
				struct _prop_object_internalize_context *);
prop_object_t	_prop_string_internalize(
				struct _prop_object_internalize_context *);

struct _prop_object_type {
	uint32_t	pot_type;		/* type indicator */
	void		(*pot_free)(void *);	/* func to free object */
	boolean_t	(*pot_extern)		/* func to externalize object */
			    (struct _prop_object_externalize_context *,
			     void *);
	boolean_t	(*pot_equals)		/* func to test quality */
			    (void *, void *);
};

struct _prop_object {
	const struct _prop_object_type *po_type;/* type descriptor */
	uint32_t	po_refcnt;		/* reference count */
};

void	_prop_object_init(struct _prop_object *,
			  const struct _prop_object_type *);
void	_prop_object_fini(struct _prop_object *);

struct _prop_object_iterator {
	prop_object_t	(*pi_next_object)(void *);
	void		(*pi_reset)(void *);
	prop_object_t	pi_obj;
	uint32_t	pi_version;
};

#if defined(_KERNEL)

/*
 * proplib in the kernel...
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/systm.h>
#include <sys/lock.h>

#define	_PROP_ASSERT(x)		KASSERT(x)

#define	_PROP_MALLOC(s, t)	malloc((s), (t), M_WAITOK)
#define	_PROP_CALLOC(s, t)	malloc((s), (t), M_WAITOK | M_ZERO)
#define	_PROP_REALLOC(v, s, t)	realloc((v), (s), (t), M_WAITOK)
#define	_PROP_FREE(v, t)	free((v), (t))

#define	_PROP_POOL_GET(p)	pool_get(&(p), PR_WAITOK)
#define	_PROP_POOL_PUT(p, v)	pool_put(&(p), (v))

#define	_PROP_POOL_INIT(p, s, d)					\
		POOL_INIT(p, s, 0, 0, 0, d, &pool_allocator_nointr);

#define	_PROP_MALLOC_DEFINE(t, s, l)					\
		MALLOC_DEFINE(t, s, l);

#define	_PROP_MUTEX_DECL_STATIC(x)					\
		static struct simplelock x = SIMPLELOCK_INITIALIZER;
#define	_PROP_MUTEX_LOCK(x)	simple_lock(&(x))
#define	_PROP_MUTEX_UNLOCK(x)	simple_unlock(&(x))

#define	_PROP_RWLOCK_DECL(x)	struct lock x ;
#define	_PROP_RWLOCK_INIT(x)	lockinit(&(x), PZERO, "proprwlk", 0, 0)
#define	_PROP_RWLOCK_RDLOCK(x)	lockmgr(&(x), LK_SHARED, NULL)
#define	_PROP_RWLOCK_WRLOCK(x)	lockmgr(&(x), LK_EXCLUSIVE, NULL)
#define	_PROP_RWLOCK_UNLOCK(x)	lockmgr(&(x), LK_RELEASE, NULL)
#define	_PROP_RWLOCK_DESTROY(x)	lockmgr(&(x), LK_DRAIN, NULL)

#elif defined(_STANDALONE)

/*
 * proplib in a standalone environment...
 */

#include <lib/libsa/stand.h>

void *		_prop_standalone_calloc(size_t);
void *		_prop_standalone_realloc(void *, size_t);

#define	_PROP_ASSERT(x)		/* nothing */

#define	_PROP_MALLOC(s, t)	alloc((s))
#define	_PROP_CALLOC(s, t)	_prop_standalone_calloc((s))
#define	_PROP_REALLOC(v, s, t)	_prop_standalone_realloc((v), (s))
#define	_PROP_FREE(v, t)	dealloc((v), 0)		/* XXX */

#define	_PROP_POOL_GET(p)	alloc((p))
#define	_PROP_POOL_PUT(p, v)	dealloc((v), (p))

#define	_PROP_POOL_INIT(p, s, d)	static const size_t p = s;

#define	_PROP_MALLOC_DEFINE(t, s, l)	/* nothing */

#define	_PROP_MUTEX_DECL_STATIC(x)	/* nothing */
#define	_PROP_MUTEX_LOCK(x)		/* nothing */
#define	_PROP_MUTEX_UNLOCK(x)		/* nothing */

#define	_PROP_RWLOCK_DECL(x)	/* nothing */
#define	_PROP_RWLOCK_INIT(x)	/* nothing */
#define	_PROP_RWLOCK_RDLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_WRLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_UNLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_DESTROY(x)	/* nothing */

#else

/*
 * proplib in user space...
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define	_PROP_ASSERT(x)		/*LINTED*/assert(x)

#define	_PROP_MALLOC(s, t)	malloc((s))
#define	_PROP_CALLOC(s, t)	calloc(1, (s))
#define	_PROP_REALLOC(v, s, t)	realloc((v), (s))
#define	_PROP_FREE(v, t)	free((v))

#define	_PROP_POOL_GET(p)	malloc((p))
#define	_PROP_POOL_PUT(p, v)	free((v))

#define	_PROP_POOL_INIT(p, s, d)	static const size_t p = s;

#define	_PROP_MALLOC_DEFINE(t, s, l)	/* nothing */

#if defined(__NetBSD__) && defined(_LIBPROP)
/*
 * Use the same mechanism as libc; we get pthread mutexes for threaded
 * programs and do-nothing stubs for non-threaded programs.
 */
#include "reentrant.h"
#define	_PROP_MUTEX_DECL_STATIC(x)	static mutex_t x = MUTEX_INITIALIZER;
#define	_PROP_MUTEX_LOCK(x)		mutex_lock(&(x))
#define	_PROP_MUTEX_UNLOCK(x)		mutex_unlock(&(x))

#define	_PROP_RWLOCK_DECL(x)	rwlock_t x ;
#define	_PROP_RWLOCK_INIT(x)	rwlock_init(&(x), NULL)
#define	_PROP_RWLOCK_RDLOCK(x)	rwlock_rdlock(&(x))
#define	_PROP_RWLOCK_WRLOCK(x)	rwlock_wrlock(&(x))
#define	_PROP_RWLOCK_UNLOCK(x)	rwlock_unlock(&(x))
#define	_PROP_RWLOCK_DESTROY(x)	rwlock_destroy(&(x))
#elif defined(HAVE_NBTOOL_CONFIG_H)
/*
 * None of NetBSD's build tools are multi-threaded.
 */
#define	_PROP_MUTEX_DECL_STATIC(x)	/* nothing */
#define	_PROP_MUTEX_LOCK(x)		/* nothing */
#define	_PROP_MUTEX_UNLOCK(x)		/* nothing */

#define	_PROP_RWLOCK_DECL(x)	/* nothing */
#define	_PROP_RWLOCK_INIT(x)	/* nothing */
#define	_PROP_RWLOCK_RDLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_WRLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_UNLOCK(x)	/* nothing */
#define	_PROP_RWLOCK_DESTROY(x)	/* nothing */
#else
/*
 * Use pthread mutexes everywhere else.
 */
#include <pthread.h>
#define	_PROP_MUTEX_DECL_STATIC(x)					\
		static pthread_mutex_t x = PTHREAD_MUTEX_INITIALIZER;
#define	_PROP_MUTEX_LOCK(x)	pthread_mutex_lock(&(x))
#define	_PROP_MUTEX_UNLOCK(x)	pthread_mutex_unlock(&(x))

#define	_PROP_RWLOCK_DECL(x)	pthread_rwlock_t x ;
#define	_PROP_RWLOCK_INIT(x)	pthread_rwlock_init(&(x), NULL)
#define	_PROP_RWLOCK_RDLOCK(x)	pthread_rwlock_rdlock(&(x))
#define	_PROP_RWLOCK_WRLOCK(x)	pthread_rwlock_wrlock(&(x))
#define	_PROP_RWLOCK_UNLOCK(x)	pthread_rwlock_unlock(&(x))
#define	_PROP_RWLOCK_DESTROY(x)	pthread_rwlock_destroy(&(x))
#endif

#endif /* _KERNEL */

#endif /* _PROPLIB_PROP_OBJECT_IMPL_H_ */
