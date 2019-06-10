/*	$NetBSD: kleak.h,v 1.1.6.2 2019/06/10 22:05:47 christos Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include <sys/ksyms.h>

#include <amd64/pmap.h>
#include <amd64/vmparam.h>

static void
kleak_md_init(uintptr_t *sva, uintptr_t *eva)
{
	extern char __rodata_start;
	*sva = (uintptr_t)KERNTEXTOFF;
	*eva = (uintptr_t)&__rodata_start;
}

static inline bool
__md_unwind_end(const char *name)
{
	if (!strcmp(name, "syscall") ||
	    !strcmp(name, "handle_syscall") ||
	    !strncmp(name, "Xintr", 5) ||
	    !strncmp(name, "Xhandle", 7) ||
	    !strncmp(name, "Xresume", 7) ||
	    !strncmp(name, "Xstray", 6) ||
	    !strncmp(name, "Xhold", 5) ||
	    !strncmp(name, "Xrecurse", 8) ||
	    !strcmp(name, "Xdoreti") ||
	    !strncmp(name, "Xsoft", 5)) {
		return true;
	}

	return false;
}

static void
kleak_md_unwind(struct kleak_hit *hit)
{
	uint64_t *rbp, rip;
	const char *mod;
	const char *sym;
	int error;

	rbp = (uint64_t *)__builtin_frame_address(0);

	hit->npc = 0;

	while (1) {
		/* 8(%rbp) contains the saved %rip. */
		rip = *(rbp + 1);

		if (rip < KERNBASE) {
			break;
		}
		error = ksyms_getname(&mod, &sym, (vaddr_t)rip, KSYMS_PROC);
		if (error) {
			break;
		}
		hit->pc[hit->npc++] = rip;
		if (__md_unwind_end(sym)) {
			break;
		}

		rbp = (uint64_t *)*(rbp);
		if (rbp == 0) {
			break;
		}

		if (hit->npc >= KLEAK_HIT_MAXPC) {
			break;
		}
	}
}
