/*	$NetBSD: getstack.c,v 1.1 2026/08/28 06:35:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: getstack.c,v 1.1 2026/08/28 06:35:26 riastradh Exp $");

#include "getstack.h"

#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <dlfcn.h>
#include <err.h>
#include <pthread.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

extern struct ps_strings *__ps_strings; /* XXX */

void *
getstack_elf_auxv(void)
{
	const AuxInfo *aux;

	for (aux = _dlauxinfo(); aux->a_type != AT_NULL; aux++) {
		if (aux->a_type == AT_STACKBASE)
			return (void *)aux->a_v;
	}
	warnx("missing AT_STACKBASE in ELF auxv");
	return NULL;
}

void *
getstack_getcontext(size_t *usedsizep)
{
	ucontext_t uc;

	if (getcontext(&uc) == -1) {
		warn("getcontext");
		*usedsizep = (size_t)-1;
		return NULL;
	}
	*usedsizep = uc.uc_stack.ss_size;
	return uc.uc_stack.ss_sp;
}

void *
getstack_psstrings(void)
{

	if (__ps_strings == NULL) {
		warnx("missing __ps_strings");
		return NULL;
	}

#ifdef __MACHINE_STACK_GROWS_UP
	return __ps_strings;
#else
	return (char *)__ps_strings + sizeof(*__ps_strings);
#endif
}

void *
getstack_pthreadattr(size_t *minsizep)
{
	pthread_attr_t attr_storage, *attr = NULL;
	void *addr = NULL;
	int error;

	error = pthread_getattr_np(pthread_self(), &attr_storage);
	if (error) {
		warnc(error, "pthread_getattr_np");
		addr = NULL;
		*minsizep = (size_t)-1;
		goto out;
	}
	attr = &attr_storage;

	error = pthread_attr_getstack(attr, &addr, minsizep);
	if (error) {
		warnc(error, "pthread_attr_getstack");
		addr = NULL;
		*minsizep = (size_t)-1;
		goto out;
	}

#ifndef __MACHINE_STACK_GROWS_UP
	addr = (char *)addr + *minsizep;
#endif

out:	error = pthread_attr_destroy(attr);
	if (error)
		warnc(error, "pthread_attr_destroy");
	return addr;
}

void
getstack_rlimit(size_t *minsizep, size_t *maxsizep)
{
	struct rlimit rlim;

	if (getrlimit(RLIMIT_STACK, &rlim) == -1) {
		warn("getrlimit(RLIMIT_STACK)");
		*minsizep = (size_t)-1;
		*maxsizep = (size_t)-1;
		return;
	}
	*minsizep = rlim.rlim_cur;
	*maxsizep = rlim.rlim_max;
}

void *
getstack_sigaltstack(size_t *minsizep)
{
	stack_t ss;

	if (sigaltstack(NULL, &ss) == -1) {
		warn("sigaltstack");
		*minsizep = (size_t)-1;
		return NULL;
	}
	*minsizep = ss.ss_size;
	return ss.ss_sp;
}

int
checkgetstack(void)
{
	void *addr_elf_auxv;
	void *addr_getcontext;
	void *addr_psstrings;
	void *addr_pthreadattr;
	void *addr_sigaltstack;
	size_t usedsize_getcontext;
	size_t minsize_pthreadattr;
	size_t minsize_rlimit;
	size_t maxsize_rlimit;
	size_t minsize_sigaltstack;

	addr_elf_auxv = getstack_elf_auxv();
	addr_getcontext = getstack_getcontext(&usedsize_getcontext);
	addr_psstrings = getstack_psstrings();
	addr_pthreadattr = getstack_pthreadattr(&minsize_pthreadattr);
	getstack_rlimit(&minsize_rlimit, &maxsize_rlimit);
	addr_sigaltstack = getstack_sigaltstack(&minsize_sigaltstack);

	warnx("%16s: addr=%p", "ELF AT_STACKBASE", addr_elf_auxv);
	warnx("%16s: addr=%-16p used=%zx", "getcontext",
	    addr_getcontext, usedsize_getcontext);
	warnx("%16s: addr=%p", "__ps_strings", addr_psstrings);
	warnx("%16s: addr=%-16p size=%zx", "pthread attr",
	    addr_pthreadattr, minsize_pthreadattr);
	warnx("%16s:      %-16s size=%zx min, %zx max", "RLIMIT_STACK",
	    "", minsize_rlimit, maxsize_rlimit);
	warnx("%16s: addr=%-16p size=%zx", "sigaltstack",
	    addr_sigaltstack, minsize_sigaltstack);

	if (addr_elf_auxv == addr_getcontext &&
	    addr_elf_auxv == addr_psstrings &&
	    addr_elf_auxv == addr_pthreadattr &&
	    (addr_sigaltstack == NULL || addr_elf_auxv == addr_sigaltstack) &&
	    usedsize_getcontext <= minsize_rlimit &&
	    minsize_pthreadattr == minsize_rlimit &&
	    (addr_sigaltstack == NULL ||
		minsize_pthreadattr == minsize_sigaltstack) &&
	    minsize_rlimit <= maxsize_rlimit) {
		warnx("Stack OK");
		return 0;
	}

	warnx("Stack disagreement!");
	return -1;
}

int
checkgetstack_singlethreaded(void)
{
	void *addr_elf_auxv;
	void *addr_getcontext;
	void *addr_psstrings;
	void *addr_sigaltstack;
	size_t usedsize_getcontext;
	size_t minsize_rlimit;
	size_t maxsize_rlimit;
	size_t minsize_sigaltstack;

	addr_elf_auxv = getstack_elf_auxv();
	addr_getcontext = getstack_getcontext(&usedsize_getcontext);
	addr_psstrings = getstack_psstrings();
	getstack_rlimit(&minsize_rlimit, &maxsize_rlimit);
	addr_sigaltstack = getstack_sigaltstack(&minsize_sigaltstack);

	warnx("%16s: addr=%p", "ELF AT_STACKBASE", addr_elf_auxv);
	warnx("%16s: addr=%-16p used=%zx", "getcontext",
	    addr_getcontext, usedsize_getcontext);
	warnx("%16s: addr=%p", "__ps_strings", addr_psstrings);
	warnx("%16s:      %-16s size=%zx min, %zx max", "RLIMIT_STACK",
	    "", minsize_rlimit, maxsize_rlimit);
	warnx("%16s: addr=%-16p size=%zx", "sigaltstack",
	    addr_sigaltstack, minsize_sigaltstack);

	if (addr_elf_auxv == addr_getcontext &&
	    addr_elf_auxv == addr_psstrings &&
	    (addr_sigaltstack == NULL || addr_elf_auxv == addr_sigaltstack) &&
	    usedsize_getcontext <= minsize_rlimit &&
	    (addr_sigaltstack == NULL ||
		minsize_rlimit == minsize_sigaltstack) &&
	    minsize_rlimit <= maxsize_rlimit) {
		warnx("Stack OK");
		return 0;
	}

	warnx("Stack disagreement!");
	return -1;
}
