/*	$NetBSD: powerpc.c,v 1.1 2023/04/12 17:53:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: powerpc.c,v 1.1 2023/04/12 17:53:32 riastradh Exp $");

#include <err.h>
#include <kvm.h>
#include <stdlib.h>

#include "extern.h"

enum {
	N_TRAPEXIT,
	N_SCTRAPEXIT,
	N_INTRCALL,
	N
};

void *trapexit;
void *sctrapexit;
void *intrcall;

static struct nlist nl[] = {
	[N_TRAPEXIT] = { .n_name = "trapexit" },
	[N_SCTRAPEXIT] = { .n_name = "sctrapexit" },
	[N_INTRCALL] = { .n_name = "intrcall" },
	[N] = { .n_name = NULL },
};

void
db_mach_init(kvm_t *kd)
{

	if (kvm_nlist(kd, nl) == -1)
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd));

	trapexit = (void *)nl[N_TRAPEXIT].n_value;
	sctrapexit = (void *)nl[N_SCTRAPEXIT].n_value;
	intrcall = (void *)nl[N_INTRCALL].n_value;
}
