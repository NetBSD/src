/*	$NetBSD: bpf_jit.c,v 1.1 2012/08/01 23:24:29 rmind Exp $	*/

/*-
 * Copyright (C) 2002-2003 NetGroup, Politecnico di Torino (Italy)
 * Copyright (C) 2005-2009 Jung-uk Kim <jkim@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/net/bpf_jitter.c,v 1.11 2009/11/20 18:49:20 jkim Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bpf_jit.c,v 1.1 2012/08/01 23:24:29 rmind Exp $");

#ifdef _KERNEL
#if defined(_KERNEL_OPT)
#include "opt_bpf.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#else
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#endif

#include <net/bpf.h>
#include <net/bpf_jit.h>

bpf_filter_func	bpf_jit_compile(struct bpf_insn *, u_int, size_t *);

static u_int	bpf_jit_accept_all(u_char *, u_int, u_int);

#ifdef _KERNEL

bpf_jit_filter *
bpf_jit(struct bpf_insn *fp, int nins)
{
	bpf_jit_filter *filter;

	/* Allocate the filter structure */
	filter = malloc(sizeof(*filter), M_TEMP, M_NOWAIT);
	if (filter == NULL)
		return (NULL);

	/* No filter means accept all */
	if (fp == NULL || nins == 0) {
		filter->func = bpf_jit_accept_all;
		return (filter);
	}

	/* Create the binary */
	if ((filter->func = bpf_jit_compile(fp, nins, &filter->size)) == NULL) {
		free(filter, M_TEMP);
		return (NULL);
	}

	return (filter);
}

void
bpf_destroy_jit_filter(bpf_jit_filter *filter)
{

	if (filter->func != bpf_jit_accept_all)
		free(filter->func, M_TEMP);
	free(filter, M_TEMP);
}

#else

bpf_jit_filter *
bpf_jit(struct bpf_insn *fp, int nins)
{
	bpf_jit_filter *filter;

	/* Allocate the filter structure */
	filter = (struct bpf_jit_filter *)malloc(sizeof(*filter));
	if (filter == NULL)
		return (NULL);

	/* No filter means accept all */
	if (fp == NULL || nins == 0) {
		filter->func = bpf_jit_accept_all;
		return (filter);
	}

	/* Create the binary */
	if ((filter->func = bpf_jit_compile(fp, nins, &filter->size)) == NULL) {
		free(filter);
		return (NULL);
	}

	return (filter);
}

void
bpf_destroy_jit_filter(bpf_jit_filter *filter)
{

	if (filter->func != bpf_jit_accept_all)
		munmap(filter->func, filter->size);
	free(filter);
}
#endif

static u_int
bpf_jit_accept_all(__unused u_char *p, __unused u_int wirelen,
    __unused u_int buflen)
{

	return ((u_int)-1);
}
