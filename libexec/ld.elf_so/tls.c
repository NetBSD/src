/*	$NetBSD: tls.c,v 1.14.8.2 2024/08/07 11:00:12 martin Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: tls.c,v 1.14.8.2 2024/08/07 11:00:12 martin Exp $");

/*
 * Thread-local storage
 *
 * Reference:
 *
 *	[ELFTLS] Ulrich Drepper, `ELF Handling For Thread-Local
 *	Storage', Version 0.21, 2023-08-22.
 *	https://akkadia.org/drepper/tls.pdf
 *	https://web.archive.org/web/20240718081934/https://akkadia.org/drepper/tls.pdf
 */

#include <sys/param.h>
#include <sys/ucontext.h>
#include <lwp.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "rtld.h"

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)

static struct tls_tcb *_rtld_tls_allocate_locked(void);
static void *_rtld_tls_module_allocate(struct tls_tcb *, size_t);

/*
 * DTV offset
 *
 *	On some architectures (m68k, mips, or1k, powerpc, and riscv),
 *	the DTV offsets passed to __tls_get_addr have a bias relative
 *	to the start of the DTV, in order to maximize the range of TLS
 *	offsets that can be used by instruction encodings with signed
 *	displacements.
 */
#ifndef TLS_DTV_OFFSET
#define	TLS_DTV_OFFSET	0
#endif

static size_t _rtld_tls_static_space;	/* Static TLS space allocated */
static size_t _rtld_tls_static_offset;	/* Next offset for static TLS to use */
size_t _rtld_tls_dtv_generation = 1;	/* Bumped on each load of obj w/ TLS */
size_t _rtld_tls_max_index = 1;		/* Max index into up-to-date DTV */

/*
 * DTV -- Dynamic Thread Vector
 *
 *	The DTV is a per-thread array that maps each module with
 *	thread-local storage to a pointer into part of the thread's TCB
 *	(thread control block), or dynamically loaded TLS blocks,
 *	reserved for that module's storage.
 *
 *	The TCB itself, struct tls_tcb, has a pointer to the DTV at
 *	tcb->tcb_dtv.
 *
 *	The layout is:
 *
 *		+---------------+
 *		| max index     | -1    max index i for which dtv[i] is alloced
 *		+---------------+
 *		| generation    |  0    void **dtv points here
 *		+---------------+
 *		| obj 1 tls ptr |  1    TLS pointer for obj w/ obj->tlsindex 1
 *		+---------------+
 *		| obj 2 tls ptr |  2    TLS pointer for obj w/ obj->tlsindex 2
 *		+---------------+
 *		  .
 *		  .
 *		  .
 *
 *	The values of obj->tlsindex start at 1; this way,
 *	dtv[obj->tlsindex] works, when dtv[0] is the generation.  The
 *	TLS pointers go either into the static thread-local storage,
 *	for the initial objects (i.e., those loaded at startup), or
 *	into TLS blocks dynamically allocated for objects that
 *	dynamically loaded by dlopen.
 *
 *	The generation field is a cache of the global generation number
 *	_rtld_tls_dtv_generation, which is bumped every time an object
 *	with TLS is loaded in _rtld_map_object, and cached by
 *	__tls_get_addr (via _rtld_tls_get_addr) when a newly loaded
 *	module lies outside the bounds of the current DTV.
 *
 *	XXX Why do we keep max index and generation separately?  They
 *	appear to be initialized the same, always incremented together,
 *	and always stored together.
 *
 *	XXX Why is this not a struct?
 *
 *		struct dtv {
 *			size_t	dtv_gen;
 *			void	*dtv_module[];
 *		};
 */
#define	DTV_GENERATION(dtv)		((size_t)((dtv)[0]))
#define	DTV_MAX_INDEX(dtv)		((size_t)((dtv)[-1]))
#define	SET_DTV_GENERATION(dtv, val)	(dtv)[0] = (void *)(size_t)(val)
#define	SET_DTV_MAX_INDEX(dtv, val)	(dtv)[-1] = (void *)(size_t)(val)

/*
 * _rtld_tls_get_addr(tcb, idx, offset)
 *
 *	Slow path for __tls_get_addr (see below), called to allocate
 *	TLS space if needed for the object obj with obj->tlsindex idx,
 *	at offset, which must be below obj->tlssize.
 *
 *	This may allocate a DTV if the current one is too old, and it
 *	may allocate a dynamically loaded TLS block if there isn't one
 *	already allocated for it.
 *
 *	XXX Why is the first argument passed as `void *tls' instead of
 *	just `struct tls_tcb *tcb'?
 */
void *
_rtld_tls_get_addr(void *tls, size_t idx, size_t offset)
{
	struct tls_tcb *tcb = tls;
	void **dtv, **new_dtv;
	sigset_t mask;

	_rtld_exclusive_enter(&mask);

	dtv = tcb->tcb_dtv;

	/*
	 * If the generation number has changed, we have to allocate a
	 * new DTV.
	 *
	 * XXX Do we really?  Isn't it enough to check whether idx <=
	 * DTV_MAX_INDEX(dtv)?
	 */
	if (__predict_false(DTV_GENERATION(dtv) != _rtld_tls_dtv_generation)) {
		size_t to_copy = DTV_MAX_INDEX(dtv);

		/*
		 * "2 +" because the first element is the generation and
		 * the second one is the maximum index.
		 */
		new_dtv = xcalloc((2 + _rtld_tls_max_index) * sizeof(*dtv));
		++new_dtv;		/* advance past DTV_MAX_INDEX */
		if (to_copy > _rtld_tls_max_index)	/* XXX How? */
			to_copy = _rtld_tls_max_index;
		memcpy(new_dtv + 1, dtv + 1, to_copy * sizeof(*dtv));
		xfree(dtv - 1);		/* retreat back to DTV_MAX_INDEX */
		dtv = tcb->tcb_dtv = new_dtv;
		SET_DTV_MAX_INDEX(dtv, _rtld_tls_max_index);
		SET_DTV_GENERATION(dtv, _rtld_tls_dtv_generation);
	}

	if (__predict_false(dtv[idx] == NULL))
		dtv[idx] = _rtld_tls_module_allocate(tcb, idx);

	_rtld_exclusive_exit(&mask);

	return (uint8_t *)dtv[idx] + offset;
}

/*
 * _rtld_tls_initial_allocation()
 *
 *	Allocate the TCB (thread control block) for the initial thread,
 *	once the static TLS space usage has been determined (plus some
 *	slop to allow certain special cases like Mesa to be dlopened).
 *
 *	This must be done _after_ all initial objects (i.e., those
 *	loaded at startup, as opposed to objects dynamically loaded by
 *	dlopen) have had TLS offsets allocated if need be by
 *	_rtld_tls_offset_allocate, and have had relocations processed.
 */
void
_rtld_tls_initial_allocation(void)
{
	struct tls_tcb *tcb;

	_rtld_tls_static_space = _rtld_tls_static_offset +
	    RTLD_STATIC_TLS_RESERVATION;

#ifndef __HAVE_TLS_VARIANT_I
	_rtld_tls_static_space = roundup2(_rtld_tls_static_space,
	    alignof(max_align_t));
#endif
	dbg(("_rtld_tls_static_space %zu", _rtld_tls_static_space));

	tcb = _rtld_tls_allocate_locked();
#ifdef __HAVE___LWP_SETTCB
	__lwp_settcb(tcb);
#else
	_lwp_setprivate(tcb);
#endif
}

/*
 * _rtld_tls_allocate_locked()
 *
 *	Internal subroutine to allocate a TCB (thread control block)
 *	for the current thread.
 *
 *	This allocates a DTV and a TCB that points to it, including
 *	static space in the TCB for the TLS of the initial objects.
 *	TLS blocks for dynamically loaded objects are allocated lazily.
 *
 *	Caller must either be single-threaded (at startup via
 *	_rtld_tls_initial_allocation) or hold the rtld exclusive lock
 *	(via _rtld_tls_allocate).
 */
static struct tls_tcb *
_rtld_tls_allocate_locked(void)
{
	Obj_Entry *obj;
	struct tls_tcb *tcb;
	uint8_t *p, *q;

	p = xcalloc(_rtld_tls_static_space + sizeof(struct tls_tcb));
#ifdef __HAVE_TLS_VARIANT_I
	tcb = (struct tls_tcb *)p;
	p += sizeof(struct tls_tcb);
#else
	p += _rtld_tls_static_space;
	tcb = (struct tls_tcb *)p;
	tcb->tcb_self = tcb;
#endif
	dbg(("lwp %d tls tcb %p", _lwp_self(), tcb));
	/*
	 * "2 +" because the first element is the generation and the second
	 * one is the maximum index.
	 */
	tcb->tcb_dtv = xcalloc(sizeof(*tcb->tcb_dtv) * (2 + _rtld_tls_max_index));
	++tcb->tcb_dtv;		/* advance past DTV_MAX_INDEX */
	SET_DTV_MAX_INDEX(tcb->tcb_dtv, _rtld_tls_max_index);
	SET_DTV_GENERATION(tcb->tcb_dtv, _rtld_tls_dtv_generation);

	for (obj = _rtld_objlist; obj != NULL; obj = obj->next) {
		if (obj->tls_static) {
#ifdef __HAVE_TLS_VARIANT_I
			q = p + obj->tlsoffset;
#else
			q = p - obj->tlsoffset;
#endif
			dbg(("%s: [lwp %d] tls dtv %p index %zu offset %zu",
			    obj->path, _lwp_self(),
			    q, obj->tlsindex, obj->tlsoffset));
			if (obj->tlsinitsize)
				memcpy(q, obj->tlsinit, obj->tlsinitsize);
			tcb->tcb_dtv[obj->tlsindex] = q;
		}
	}

	return tcb;
}

/*
 * _rtld_tls_allocate()
 *
 *	Allocate a TCB (thread control block) for the current thread.
 *
 *	Called by pthread_create for non-initial threads.  (The initial
 *	thread's TCB is allocated by _rtld_tls_initial_allocation.)
 */
struct tls_tcb *
_rtld_tls_allocate(void)
{
	struct tls_tcb *tcb;
	sigset_t mask;

	_rtld_exclusive_enter(&mask);
	tcb = _rtld_tls_allocate_locked();
	_rtld_exclusive_exit(&mask);

	return tcb;
}

/*
 * _rtld_tls_free(tcb)
 *
 *	Free a TCB allocated with _rtld_tls_allocate.
 *
 *	Frees any TLS blocks for dynamically loaded objects that tcb's
 *	DTV points to, and frees tcb's DTV, and frees tcb.
 */
void
_rtld_tls_free(struct tls_tcb *tcb)
{
	size_t i, max_index;
	uint8_t *p, *p_end;
	sigset_t mask;

	_rtld_exclusive_enter(&mask);

#ifdef __HAVE_TLS_VARIANT_I
	p = (uint8_t *)tcb;
#else
	p = (uint8_t *)tcb - _rtld_tls_static_space;
#endif
	p_end = p + _rtld_tls_static_space;

	max_index = DTV_MAX_INDEX(tcb->tcb_dtv);
	for (i = 1; i <= max_index; ++i) {
		if ((uint8_t *)tcb->tcb_dtv[i] < p ||
		    (uint8_t *)tcb->tcb_dtv[i] >= p_end)
			xfree(tcb->tcb_dtv[i]);
	}
	xfree(tcb->tcb_dtv - 1);	/* retreat back to DTV_MAX_INDEX */
	xfree(p);

	_rtld_exclusive_exit(&mask);
}

/*
 * _rtld_tls_module_allocate(tcb, idx)
 *
 *	Allocate thread-local storage in the thread with the given TCB
 *	(thread control block) for the object obj whose obj->tlsindex
 *	is idx.
 *
 *	If obj has had space in static TLS reserved (obj->tls_static),
 *	return a pointer into that.  Otherwise, allocate a TLS block,
 *	mark obj as having a TLS block allocated (obj->tls_dynamic),
 *	and return it.
 *
 *	Called by _rtld_tls_get_addr to get the thread-local storage
 *	for an object the first time around.
 */
static void *
_rtld_tls_module_allocate(struct tls_tcb *tcb, size_t idx)
{
	Obj_Entry *obj;
	uint8_t *p;

	for (obj = _rtld_objlist; obj != NULL; obj = obj->next) {
		if (obj->tlsindex == idx)
			break;
	}
	if (obj == NULL) {
		_rtld_error("Module for TLS index %zu missing", idx);
		_rtld_die();
	}
	if (obj->tls_static) {
#ifdef __HAVE_TLS_VARIANT_I
		p = (uint8_t *)tcb + obj->tlsoffset + sizeof(struct tls_tcb);
#else
		p = (uint8_t *)tcb - obj->tlsoffset;
#endif
		return p;
	}

	p = xmalloc(obj->tlssize);
	memcpy(p, obj->tlsinit, obj->tlsinitsize);
	memset(p + obj->tlsinitsize, 0, obj->tlssize - obj->tlsinitsize);

	obj->tls_dynamic = 1;

	return p;
}

/*
 * _rtld_tls_offset_allocate(obj)
 *
 *	Allocate a static thread-local storage offset for obj.
 *
 *	Called by _rtld at startup for all initial objects.  Called
 *	also by MD relocation logic, which is allowed (for Mesa) to
 *	allocate an additional 64 bytes (RTLD_STATIC_TLS_RESERVATION)
 *	of static thread-local storage in dlopened objects.
 */
int
_rtld_tls_offset_allocate(Obj_Entry *obj)
{
	size_t offset, next_offset;

	if (obj->tls_dynamic)
		return -1;

	if (obj->tls_static)
		return 0;
	if (obj->tlssize == 0) {
		obj->tlsoffset = 0;
		obj->tls_static = 1;
		return 0;
	}

#ifdef __HAVE_TLS_VARIANT_I
	offset = roundup2(_rtld_tls_static_offset, obj->tlsalign);
	next_offset = offset + obj->tlssize;
#else
	offset = roundup2(_rtld_tls_static_offset + obj->tlssize,
	    obj->tlsalign);
	next_offset = offset;
#endif

	/*
	 * Check if the static allocation was already done.
	 * This happens if dynamically loaded modules want to use
	 * static TLS space.
	 *
	 * XXX Keep an actual free list and callbacks for initialisation.
	 */
	if (_rtld_tls_static_space) {
		if (obj->tlsinitsize) {
			_rtld_error("%s: Use of initialized "
			    "Thread Local Storage with model initial-exec "
			    "and dlopen is not supported",
			    obj->path);
			return -1;
		}
		if (next_offset > _rtld_tls_static_space) {
			_rtld_error("%s: No space available "
			    "for static Thread Local Storage",
			    obj->path);
			return -1;
		}
	}
	obj->tlsoffset = offset;
	dbg(("%s: static tls offset 0x%zx size %zu\n",
	    obj->path, obj->tlsoffset, obj->tlssize));
	_rtld_tls_static_offset = next_offset;
	obj->tls_static = 1;

	return 0;
}

/*
 * _rtld_tls_offset_free(obj)
 *
 *	Free a static thread-local storage offset for obj.
 *
 *	Called by dlclose (via _rtld_unload_object -> _rtld_obj_free).
 *
 *	Since static thread-local storage is normally not used by
 *	dlopened objects (with the exception of Mesa), this doesn't do
 *	anything to recycle the space right now.
 */
void
_rtld_tls_offset_free(Obj_Entry *obj)
{

	/*
	 * XXX See above.
	 */
	obj->tls_static = 0;
	return;
}

#if defined(__HAVE_COMMON___TLS_GET_ADDR) && defined(RTLD_LOADER)
/*
 * __tls_get_addr(tlsindex)
 *
 *	Symbol directly called by code generated by the compiler for
 *	references thread-local storage in the general-dynamic or
 *	local-dynamic TLS models (but not initial-exec or local-exec).
 *
 *	The argument is a pointer to
 *
 *		struct {
 *			unsigned long int ti_module;
 *			unsigned long int ti_offset;
 *		};
 *
 *	 as in, e.g., [ELFTLS] Sec. 3.4.3.  This coincides with the
 *	 type size_t[2] on all architectures that use this common
 *	 __tls_get_addr definition (XXX but why do we write it as
 *	 size_t[2]?).
 *
 *	 ti_module, i.e., arg[0], is the obj->tlsindex assigned at
 *	 load-time by _rtld_map_object, and ti_offset, i.e., arg[1], is
 *	 assigned at link-time by ld(1), possibly adjusted by
 *	 TLS_DTV_OFFSET.
 *
 *	 Some architectures -- specifically IA-64 -- use a different
 *	 calling convention.  Some architectures -- specifically i386
 *	 -- also use another entry point ___tls_get_addr (that's three
 *	 leading underscores) with a different calling convention.
 */
void *
__tls_get_addr(void *arg_)
{
	size_t *arg = (size_t *)arg_;
	void **dtv;
#ifdef __HAVE___LWP_GETTCB_FAST
	struct tls_tcb * const tcb = __lwp_gettcb_fast();
#else
	struct tls_tcb * const tcb = __lwp_getprivate_fast();
#endif
	size_t idx = arg[0], offset = arg[1] + TLS_DTV_OFFSET;

	dtv = tcb->tcb_dtv;

	/*
	 * Fast path: access to an already allocated DTV entry.  This
	 * checks the current limit and the entry without needing any
	 * locking.  Entries are only freed on dlclose() and it is an
	 * application bug if code of the module is still running at
	 * that point.
	 */
	if (__predict_true(idx < DTV_MAX_INDEX(dtv) && dtv[idx] != NULL))
		return (uint8_t *)dtv[idx] + offset;

	return _rtld_tls_get_addr(tcb, idx, offset);
}
#endif

#endif /* __HAVE_TLS_VARIANT_I || __HAVE_TLS_VARIANT_II */
