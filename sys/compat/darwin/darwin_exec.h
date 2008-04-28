/*	$NetBSD: darwin_exec.h,v 1.15 2008/04/28 20:23:41 martin Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_EXEC_H_
#define	_DARWIN_EXEC_H_

#include <compat/mach/mach_exec.h>

/*
 * Because it can be used by Mach emulation code as well as Darwin emulation
 * code, this structure must begin with a struct mach_emuldata.
 */
struct darwin_emuldata {
	struct mach_emuldata ded_mach_emuldata;
	pid_t ded_fakepid;
	dev_t ded_wsdev;		/* display to restore on exit */
	int *ded_hidsystem_finished;	/* iohidsystem thread finished flag */
	int ded_flags;			/* flags, see below */
	void *ded_vramoffset;		/* Where VRAM was mapped? */
};

#define DARWIN_DED_SIGEXC	1	/* Mach exceptions instead of signals */

int exec_darwin_copyargs(struct lwp *, struct exec_package *,
    struct ps_strings *, char **, void *);
int exec_darwin_probe(const char **);
int darwin_exec_setup_stack(struct lwp *, struct exec_package *);

extern const struct emul emul_darwin;

#endif /* !_DARWIN_EXEC_H_ */
