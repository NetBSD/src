/*	$NetBSD: aarch64.c,v 1.2 2022/06/07 08:08:31 ryo Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ryo Shimizu.
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
#ifndef lint
__RCSID("$NetBSD: aarch64.c,v 1.2 2022/06/07 08:08:31 ryo Exp $");
#endif /* not lint */

#include <kvm.h>
#include <nlist.h>
#include <err.h>
#include <stdlib.h>

#include "extern.h"

vaddr_t el0_trap;
vaddr_t el1_trap;
vaddr_t cpu_switchto_softint;

static struct nlist nl[] = {
	{ .n_name = "el0_trap" },		/* 0 */
	{ .n_name = "el1_trap" },		/* 1 */
	{ .n_name = "cpu_switchto_softint" },	/* 2 */
	{ .n_name = NULL },
};

void
db_mach_init(kvm_t *kd)
{
	if (kvm_nlist(kd, nl) == -1)
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd));

	el0_trap = nl[0].n_value;
	el1_trap = nl[1].n_value;
	cpu_switchto_softint = nl[2].n_value;
}
