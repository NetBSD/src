/* $NetBSD: cxa_thread_atexit.c,v 1.1 2017/07/11 15:21:35 joerg Exp $ */

/*-
 * Copyright (c) 2017 Joerg Sonnenberger <joerg@NetBSD.org>
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: cxa_thread_atexit.c,v 1.1 2017/07/11 15:21:35 joerg Exp $");

#include <sys/queue.h>
#include <dlfcn.h>
#include <stdlib.h>

#include "atexit.h"

__dso_hidden bool __cxa_thread_atexit_used;

struct cxa_dtor {
	SLIST_ENTRY(cxa_dtor) link;
	void *dso_symbol;
	void *obj;
	void (*dtor)(void *);
};

/* Assumes NULL initialization. */
static __thread SLIST_HEAD(, cxa_dtor) cxa_dtors = SLIST_HEAD_INITIALIZER(cxa_dstors);

void
__cxa_thread_run_atexit(void)
{
	struct cxa_dtor *entry;

	while ((entry = SLIST_FIRST(&cxa_dtors)) != NULL) {
		SLIST_REMOVE_HEAD(&cxa_dtors, link);
		(*entry->dtor)(entry->obj);
		if (entry->dso_symbol)
			__dl_cxa_refcount(entry->dso_symbol, -1);
		free(entry);
	}
}

/*
 * This dance is necessary since libstdc++ includes
 * __cxa_thread_atexit unconditionally.
 */
__weak_alias(__cxa_thread_atexit, __cxa_thread_atexit_impl)
int	__cxa_thread_atexit_impl(void (*)(void *), void *, void *);

int
__cxa_thread_atexit_impl(void (*dtor)(void *), void *obj, void *dso_symbol)
{
	struct cxa_dtor *entry;

	__cxa_thread_atexit_used = true;

	entry = malloc(sizeof(*entry));
	if (entry == NULL)
		return -1;
	entry->dso_symbol = dso_symbol;
	if (dso_symbol)
		__dl_cxa_refcount(entry->dso_symbol, 1);
	entry->obj = obj;
	entry->dtor = dtor;
	SLIST_INSERT_HEAD(&cxa_dtors, entry, link);
	return 0;
}
