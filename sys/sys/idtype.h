/*	$NetBSD: idtype.h,v 1.2 2016/04/02 21:21:57 christos Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#ifndef _SYS_IDTYPE_H_
#define _SYS_IDTYPE_H_

/*
 * Using the solaris constants, some of them are not applicable to us
 */
#define	P_MYID		-1	/* Me/my process group */
#define	P_PID		1	/* A process identifier. */
#define	P_PPID		2	/* A parent process identifier.	*/
#define	P_PGID		3	/* A process group identifier. */
#define	P_SID		4	/* A session identifier. */
#define	P_CID		5	/* A scheduling class identifier. */
#define	P_UID		6	/* A user identifier. */
#define	P_GID		7	/* A group identifier. */
#define	P_ALL		8	/* All processes. */
#define	P_LWPID		9	/* An LWP identifier. */
#define	P_TASKID	10	/* A task identifier. */
#define	P_PROJID	11	/* A project identifier. */
#define	P_POOLID	12	/* A pool identifier. */
#define	P_ZONEID	13	/* A zone identifier. */
#define	P_CTID		14	/* A (process) contract identifier. */
#define	P_CPUID		15	/* CPU identifier. */
#define	P_PSETID	16	/* Processor set identifier. */

#endif /* _SYS_IDTYPE_H_ */
