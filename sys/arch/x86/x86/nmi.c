/*	$Id: nmi.c,v 1.3.12.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c)2009,2011 YAMAMOTO Takashi,
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
__KERNEL_RCSID(0, "$NetBSD: nmi.c,v 1.3.12.2 2017/12/03 11:36:50 jdolecek Exp $");

/*
 * nmi dispatcher.
 *
 * XXX no need to be nmi-specific.
 * actual assumptions are:
 *	- dispatch() is called with preemption disabled.
 *	- handlers never block.
 *	- establish() and disestablish() are called within a thread context.
 *	  (thus can block)
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/pserialize.h>

#include <x86/nmi.h>

struct nmi_handler {
	int (*n_func)(const struct trapframe *, void *);
	void *n_arg;
	struct nmi_handler *n_next;
};

static kmutex_t nmi_list_lock; /* serialize establish and disestablish */
static pserialize_t nmi_psz;
static nmi_handler_t *nmi_handlers; /* list of handlers */

/*
 * nmi_establish: establish an nmi handler
 *
 * => can block.
 * => returns an opaque handle.
 */

nmi_handler_t *
nmi_establish(int (*func)(const struct trapframe *, void *), void *arg)
{
	struct nmi_handler *n;

	n = kmem_alloc(sizeof(*n), KM_SLEEP);
	n->n_func = func;
	n->n_arg = arg;

	/*
	 * put it into the list.
	 */

	mutex_enter(&nmi_list_lock);
	n->n_next = nmi_handlers;
	membar_producer(); /* n->n_next should be visible before nmi_handlers */
	nmi_handlers = n; /* atomic store */
	mutex_exit(&nmi_list_lock);

	return n;
}

/*
 * nmi_disestablish: disestablish an nmi handler.
 *
 * => can block.
 * => take an opaque handle.  it must be one returned by nmi_establish.
 */

void
nmi_disestablish(nmi_handler_t *handle)
{
	nmi_handler_t *n;
	nmi_handler_t **pp;

	KASSERT(handle != NULL);

	/*
	 * remove the handler from the list.
	 */

	mutex_enter(&nmi_list_lock);
	for (pp = &nmi_handlers, n = *pp; n != NULL; n = *pp) {
		if (n == handle) {
			break;
		}
		pp = &n->n_next;
	}
#if defined(DIAGNOSTIC)
	if (n == NULL) {
		mutex_exit(&nmi_list_lock);
		panic("%s: invalid handle %p", __func__, handle);
	}
#endif /* defined(DIAGNOSTIC) */
	*pp = n->n_next; /* atomic store */
	mutex_exit(&nmi_list_lock); /* mutex_exit implies a store fence */

	/*
	 * before freeing 'n', ensure that no other cpus are
	 * in the middle of nmi_dispatch.
	 */

	pserialize_perform(nmi_psz);
	kmem_free(n, sizeof(*n));
}

/*
 * nmi_dispatch: dispatch an nmi.
 *
 * => called by interrupts, thus preempt disabled.
 */

int
nmi_dispatch(const struct trapframe *tf)
{
	const struct nmi_handler *n;
	int handled = 0;

	/*
	 * XXX abstraction violation
	 *
	 * we don't bother to call pserialize_read_enter/pserialize_read_exit
	 * because they are not necessary here as we are sure our IPL is
	 * higher than IPL_SOFTSERIAL.  better to avoid unnecessary calls as
	 * we are in a dangerous context. (NMI)
	 */

	for (n = nmi_handlers; /* atomic load */
	    n != NULL;
	    membar_consumer(), n = n->n_next) { /* atomic load */
		handled |= (*n->n_func)(tf, n->n_arg);
	}
	return handled;
}

void
nmi_init(void)
{

	mutex_init(&nmi_list_lock, MUTEX_DEFAULT, IPL_NONE);
	nmi_psz = pserialize_create();
}
