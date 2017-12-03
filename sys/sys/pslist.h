/*	$NetBSD: pslist.h,v 1.4.14.2 2017/12/03 11:39:20 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_PSLIST_H
#define	_SYS_PSLIST_H

#include <sys/param.h>
#include <sys/atomic.h>

struct pslist_head;
struct pslist_entry;

struct pslist_head {
	struct pslist_entry *plh_first;
};

struct pslist_entry {
	struct pslist_entry **ple_prevp;
	struct pslist_entry *ple_next;
};

#ifdef _KERNEL
#define	_PSLIST_ASSERT	KASSERT
#else
#include <assert.h>
#define	_PSLIST_ASSERT	assert
#endif

#define	_PSLIST_POISON	((void *)1ul)

/*
 * Initialization.  Allowed only when the caller has exclusive access,
 * excluding writers and readers.
 */

static inline void
pslist_init(struct pslist_head *head)
{

	head->plh_first = NULL;
}

static inline void
pslist_destroy(struct pslist_head *head __diagused)
{

	_PSLIST_ASSERT(head->plh_first == NULL);
}

static inline void
pslist_entry_init(struct pslist_entry *entry)
{

	entry->ple_next = NULL;
	entry->ple_prevp = NULL;
}

static inline void
pslist_entry_destroy(struct pslist_entry *entry)
{

	_PSLIST_ASSERT(entry->ple_prevp == NULL);

	/*
	 * Poison the next entry.  If we used NULL here, then readers
	 * would think they were simply at the end of the list.
	 * Instead, cause readers to crash.
	 */
	entry->ple_next = _PSLIST_POISON;
}

/*
 * Writer operations.  Caller must exclude other writers, but not
 * necessarily readers.
 *
 * Writes to initialize a new entry must precede its publication by
 * writing to plh_first / ple_next / *ple_prevp.
 *
 * The ple_prevp field is serialized by the caller's exclusive lock and
 * not read by readers, and hence its ordering relative to the internal
 * memory barriers is inconsequential.
 */

static inline void
pslist_writer_insert_head(struct pslist_head *head, struct pslist_entry *new)
{

	_PSLIST_ASSERT(head->plh_first == NULL ||
	    head->plh_first->ple_prevp == &head->plh_first);
	_PSLIST_ASSERT(new->ple_next == NULL);
	_PSLIST_ASSERT(new->ple_prevp == NULL);

	new->ple_prevp = &head->plh_first;
	new->ple_next = head->plh_first;
	if (head->plh_first != NULL)
		head->plh_first->ple_prevp = &new->ple_next;
	membar_producer();
	head->plh_first = new;
}

static inline void
pslist_writer_insert_before(struct pslist_entry *entry,
    struct pslist_entry *new)
{

	_PSLIST_ASSERT(entry->ple_next != _PSLIST_POISON);
	_PSLIST_ASSERT(entry->ple_prevp != NULL);
	_PSLIST_ASSERT(*entry->ple_prevp == entry);
	_PSLIST_ASSERT(new->ple_next == NULL);
	_PSLIST_ASSERT(new->ple_prevp == NULL);

	new->ple_prevp = entry->ple_prevp;
	new->ple_next = entry;
	membar_producer();
	*entry->ple_prevp = new;
	entry->ple_prevp = &new->ple_next;
}

static inline void
pslist_writer_insert_after(struct pslist_entry *entry,
    struct pslist_entry *new)
{

	_PSLIST_ASSERT(entry->ple_next != _PSLIST_POISON);
	_PSLIST_ASSERT(entry->ple_prevp != NULL);
	_PSLIST_ASSERT(*entry->ple_prevp == entry);
	_PSLIST_ASSERT(new->ple_next == NULL);
	_PSLIST_ASSERT(new->ple_prevp == NULL);

	new->ple_prevp = &entry->ple_next;
	new->ple_next = entry->ple_next;
	if (new->ple_next != NULL)
		new->ple_next->ple_prevp = &new->ple_next;
	membar_producer();
	entry->ple_next = new;
}

static inline void
pslist_writer_remove(struct pslist_entry *entry)
{

	_PSLIST_ASSERT(entry->ple_next != _PSLIST_POISON);
	_PSLIST_ASSERT(entry->ple_prevp != NULL);
	_PSLIST_ASSERT(*entry->ple_prevp == entry);

	if (entry->ple_next != NULL)
		entry->ple_next->ple_prevp = entry->ple_prevp;
	*entry->ple_prevp = entry->ple_next;
	entry->ple_prevp = NULL;

	/*
	 * Leave entry->ple_next intact so that any extant readers can
	 * continue iterating through the list.  The caller must then
	 * wait for readers to drain, e.g. with pserialize_perform,
	 * before destroying and reusing the entry.
	 */
}

static inline struct pslist_entry *
pslist_writer_first(const struct pslist_head *head)
{

	return head->plh_first;
}

static inline struct pslist_entry *
pslist_writer_next(const struct pslist_entry *entry)
{

	_PSLIST_ASSERT(entry->ple_next != _PSLIST_POISON);
	return entry->ple_next;
}

static inline void *
_pslist_writer_first_container(const struct pslist_head *head,
    const ptrdiff_t offset)
{
	struct pslist_entry *first = head->plh_first;

	return (first == NULL ? NULL : (char *)first - offset);
}

static inline void *
_pslist_writer_next_container(const struct pslist_entry *entry,
    const ptrdiff_t offset)
{
	struct pslist_entry *next = entry->ple_next;

	_PSLIST_ASSERT(next != _PSLIST_POISON);
	return (next == NULL ? NULL : (char *)next - offset);
}

/*
 * Reader operations.  Caller must block pserialize_perform or
 * equivalent and be bound to a CPU.  Only plh_first/ple_next may be
 * read, and after that, a membar_datadep_consumer must precede
 * dereferencing the resulting pointer.
 */

static inline struct pslist_entry *
pslist_reader_first(const struct pslist_head *head)
{
	struct pslist_entry *first = head->plh_first;

	if (first != NULL)
		membar_datadep_consumer();

	return first;
}

static inline struct pslist_entry *
pslist_reader_next(const struct pslist_entry *entry)
{
	struct pslist_entry *next = entry->ple_next;

	_PSLIST_ASSERT(next != _PSLIST_POISON);
	if (next != NULL)
		membar_datadep_consumer();

	return next;
}

static inline void *
_pslist_reader_first_container(const struct pslist_head *head,
    const ptrdiff_t offset)
{
	struct pslist_entry *first = head->plh_first;

	if (first == NULL)
		return NULL;
	membar_datadep_consumer();

	return (char *)first - offset;
}

static inline void *
_pslist_reader_next_container(const struct pslist_entry *entry,
    const ptrdiff_t offset)
{
	struct pslist_entry *next = entry->ple_next;

	_PSLIST_ASSERT(next != _PSLIST_POISON);
	if (next == NULL)
		return NULL;
	membar_datadep_consumer();

	return (char *)next - offset;
}

/*
 * Type-safe macros for convenience.
 */

#ifdef __COVERITY__
#define	_PSLIST_VALIDATE_PTRS(P, Q)		0
#define	_PSLIST_VALIDATE_CONTAINER(P, T, F)	0
#else
#define	_PSLIST_VALIDATE_PTRS(P, Q)					      \
	(0 * sizeof((P) - (Q)) * sizeof(*(P)) * sizeof(*(Q)))
#define	_PSLIST_VALIDATE_CONTAINER(P, T, F)				      \
	(0 * sizeof((P) - &((T *)(((char *)(P)) - offsetof(T, F)))->F))
#endif

#define	PSLIST_INITIALIZER		{ .plh_first = NULL }
#define	PSLIST_ENTRY_INITIALIZER	{ .ple_next = NULL, .ple_prevp = NULL }

#define	PSLIST_INIT(H)			pslist_init((H))
#define	PSLIST_DESTROY(H)		pslist_destroy((H))
#define	PSLIST_ENTRY_INIT(E, F)		pslist_entry_init(&(E)->F)
#define	PSLIST_ENTRY_DESTROY(E, F)	pslist_entry_destroy(&(E)->F)

#define	PSLIST_WRITER_INSERT_HEAD(H, V, F)				      \
	pslist_writer_insert_head((H), &(V)->F)
#define	PSLIST_WRITER_INSERT_BEFORE(E, N, F)				      \
	pslist_writer_insert_before(&(E)->F + _PSLIST_VALIDATE_PTRS(E, N),    \
	    &(N)->F)
#define	PSLIST_WRITER_INSERT_AFTER(E, N, F)				      \
	pslist_writer_insert_after(&(E)->F + _PSLIST_VALIDATE_PTRS(E, N),     \
	    &(N)->F)
#define	PSLIST_WRITER_REMOVE(E, F)					      \
	pslist_writer_remove(&(E)->F)
#define	PSLIST_WRITER_FIRST(H, T, F)					      \
	((T *)(_pslist_writer_first_container((H), offsetof(T, F))) +	      \
	    _PSLIST_VALIDATE_CONTAINER(pslist_writer_first(H), T, F))
#define	PSLIST_WRITER_NEXT(V, T, F)					      \
	((T *)(_pslist_writer_next_container(&(V)->F, offsetof(T, F))) +      \
	    _PSLIST_VALIDATE_CONTAINER(pslist_writer_next(&(V)->F), T, F))
#define	PSLIST_WRITER_FOREACH(V, H, T, F)				      \
	for ((V) = PSLIST_WRITER_FIRST((H), T, F);			      \
		(V) != NULL;						      \
		(V) = PSLIST_WRITER_NEXT((V), T, F))

#define	PSLIST_READER_FIRST(H, T, F)					      \
	((T *)(_pslist_reader_first_container((H), offsetof(T, F))) +	      \
	    _PSLIST_VALIDATE_CONTAINER(pslist_reader_first(H), T, F))
#define	PSLIST_READER_NEXT(V, T, F)					      \
	((T *)(_pslist_reader_next_container(&(V)->F, offsetof(T, F))) +      \
	    _PSLIST_VALIDATE_CONTAINER(pslist_reader_next(&(V)->F), T, F))
#define	PSLIST_READER_FOREACH(V, H, T, F)				      \
	for ((V) = PSLIST_READER_FIRST((H), T, F);			      \
		(V) != NULL;						      \
		(V) = PSLIST_READER_NEXT((V), T, F))

#endif	/* _SYS_PSLIST_H */
