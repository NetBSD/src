/*	$NetBSD: darwin_ptrace.h,v 1.2.72.1 2008/05/18 12:33:10 yamt Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#ifndef	_DARWIN_PTRACE_H_
#define	_DARWIN_PTRACE_H_

#define DARWIN_PT_TRACE_ME	0
#define DARWIN_PT_READ_I	1
#define DARWIN_PT_READ_D	2
#define DARWIN_PT_READ_U	3
#define DARWIN_PT_WRITE_I	4
#define DARWIN_PT_WRITE_D	5
#define DARWIN_PT_WRITE_U	6
#define DARWIN_PT_CONTINUE	7
#define DARWIN_PT_KILL		8
#define DARWIN_PT_STEP		9
#define DARWIN_PT_ATTACH	10
#define DARWIN_PT_DETACH	11
#define DARWIN_PT_SIGEXC	12
#define DARWIN_PT_THUPDATE	13
#define DARWIN_PT_ATTACHEXC	14

#define DARWIN_PT_FORCEQUOTA	30
#define DARWIN_PT_DENY_ATTACH	31

#endif /* _DARWIN_PTRACE_H_ */
