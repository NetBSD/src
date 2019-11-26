/*	$NetBSD: tls.c,v 1.9.2.1 2019/11/26 08:12:26 martin Exp $	*/

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
__RCSID("$NetBSD: tls.c,v 1.9.2.1 2019/11/26 08:12:26 martin Exp $");

#include "namespace.h"

#define	_rtld_tls_allocate	__libc_rtld_tls_allocate
#define	_rtld_tls_free		__libc_rtld_tls_free

#include <sys/tls.h>

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)

#include <sys/param.h>
#include <sys/mman.h>
#include <link_elf.h>
#include <lwp.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

__dso_hidden void	__libc_static_tls_setup(void);

static bool is_dynamic;
static const void *tls_initaddr;
static size_t tls_initsize;
static size_t tls_size;
static size_t tls_allocation;
static void *initial_thread_tcb;

void * __libc_tls_get_addr(void);

__weak_alias(__tls_get_addr, __libc_tls_get_addr)
#ifdef __i386__
__weak_alias(___tls_get_addr, __libc_tls_get_addr)
#endif

void *
__libc_tls_get_addr(void)
{

	abort();
	/* NOTREACHED */
}

__weak_alias(_rtld_tls_allocate, __libc_rtld_tls_allocate)

struct tls_tcb *
_rtld_tls_allocate(void)
{
	struct tls_tcb *tcb;
	uint8_t *p;

	if (initial_thread_tcb == NULL) {
#ifdef __HAVE_TLS_VARIANT_I
		tls_allocation = tls_size;
#else
		tls_allocation = roundup2(tls_size, alignof(max_align_t));
#endif

		initial_thread_tcb = p = mmap(NULL,
		    tls_allocation + sizeof(*tcb), PROT_READ | PROT_WRITE,
		    MAP_ANON, -1, 0);
	} else {
		p = calloc(1, tls_allocation + sizeof(*tcb));
	}
	if (p == NULL) {
		static const char msg[] =  "TLS allocation failed, terminating\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(127);
	}
#ifdef __HAVE_TLS_VARIANT_I
	/* LINTED */
	tcb = (struct tls_tcb *)p;
	p += sizeof(struct tls_tcb);
#else
	/* LINTED tls_size is rounded above */
	tcb = (struct tls_tcb *)(p + tls_allocation);
	p = (uint8_t *)tcb - tls_size;
	tcb->tcb_self = tcb;
#endif
	memcpy(p, tls_initaddr, tls_initsize);

	return tcb;
}

__weak_alias(_rtld_tls_free, __libc_rtld_tls_free)

void
_rtld_tls_free(struct tls_tcb *tcb)
{
	uint8_t *p;

#ifdef __HAVE_TLS_VARIANT_I
	/* LINTED */
	p = (uint8_t *)tcb;
#else
	/* LINTED */
	p = (uint8_t *)tcb - tls_allocation;
#endif
	if (p == initial_thread_tcb)
		munmap(p, tls_allocation + sizeof(*tcb));
	else
		free(p);
}

static int __section(".text.startup")
__libc_static_tls_setup_cb(struct dl_phdr_info *data, size_t len, void *cookie)
{
	const Elf_Phdr *phdr = data->dlpi_phdr;
	const Elf_Phdr *phlimit = data->dlpi_phdr + data->dlpi_phnum;

	for (; phdr < phlimit; ++phdr) {
		if (phdr->p_type == PT_INTERP) {
			is_dynamic = true;
			return -1;
		}
		if (phdr->p_type != PT_TLS)
			continue;
		tls_initaddr = (void *)(phdr->p_vaddr + data->dlpi_addr);
		tls_initsize = phdr->p_filesz;
#ifdef __HAVE_TLS_VARIANT_I
		tls_size = phdr->p_memsz;
#else
		tls_size = roundup2(phdr->p_memsz, phdr->p_align);
#endif
	}
	return 0;
}

void
__libc_static_tls_setup(void)
{
	struct tls_tcb *tcb;

	dl_iterate_phdr(__libc_static_tls_setup_cb, NULL);
	if (is_dynamic)
		return;

	tcb = _rtld_tls_allocate();
#ifdef __HAVE___LWP_SETTCB
	__lwp_settcb(tcb);
#else
	_lwp_setprivate(tcb);
#endif
}

#endif /* __HAVE_TLS_VARIANT_I || __HAVE_TLS_VARIANT_II */
