/*	$NetBSD: irix_prctl.h,v 1.1.4.3 2002/06/23 17:43:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _IRIX_PRCTL_H_
#define _IRIX_PRCTL_H_

int irix_prda_init __P((struct proc *));
int irix_sync_saddr_syscall __P((struct proc *, void *, register_t *,
	int (*syscall) __P((struct proc *, void *, register_t *))));
int irix_sync_saddr_vmcmd __P((struct proc *, struct exec_vmcmd *));

/* From IRIX's <sys/prctl.h> */

#define IRIX_PR_MAXPROCS	1
#define IRIX_PR_ISBLOCKED	2
#define IRIX_PR_SETSTACKSIZE	3
#define IRIX_PR_GETSTACKSIZE	4
#define IRIX_PR_MAXPPROCS	5
#define IRIX_PR_UNBLKONEXEC	6
#define IRIX_PR_SETEXITSIG	8
#define IRIX_PR_RESIDENT	9
#define IRIX_PR_ATTACHADDR	10
#define IRIX_PR_DETACHADDR	11
#define IRIX_PR_TERMCHILD	12
#define IRIX_PR_GETSHMASK	13
#define IRIX_PR_GETNSHARE	14
#define IRIX_PR_COREPID		15
#define IRIX_PR_ATTACHADDRPERM	16
#define IRIX_PR_PTHREADEXIT	17
#define IRIX_PR_SETABORTSIG	18
#define IRIX_PR_INIT_THREADS	20
#define IRIX_PR_THREAD_CTL	21
#define IRIX_PR_LASTSHEXIT	22

/* sproc flags */
#define IRIX_PR_SPROC		0x00000001
#define IRIX_PR_SFDS		0x00000002
#define IRIX_PR_SDIR		0x00000004
#define IRIX_PR_SUMASK		0x00000008
#define IRIX_PR_SULIMIT		0x00000010
#define IRIX_PR_SID		0x00000020
#define IRIX_PR_SADDR		0x00000040
#define IRIX_PR_THREADS		0x00000080
#define IRIX_PR_BLOCK		0x01000000
#define IRIX_PR_NOLIBC		0x02000000
#define IRIX_PR_EVENT		0x04000000

/* blockproc constants */
#define IRIX_PR_MAXBLOCKCNT	10000
#define IRIX_PR_MINBLOCKCNT	-10000

/* This is undocumented */
#define IRIX_PROCBLK_BLOCK	0
#define IRIX_PROCBLK_UNBLOCK	1
#define IRIX_PROCBLK_COUNT	2
#define IRIX_PROCBLK_BLOCKALL	3
#define IRIX_PROCBLK_UNBLOCKALL	4
#define IRIX_PROCBLK_COUNTALL	5
#define IRIX_PROCBLK_ONLYONE	-3

/* From <sys/prctl.h> */
#define IRIX_PRDA 		((struct prda *)0x00200000L)
struct irix_prda_sys {
	irix_pid_t	t_pid;
	uint32_t	t_hint;
	uint32_t	t_dlactseq;
	uint32_t	t_fpflags;
	uint32_t	t_prid;
	uint32_t	t_dlendseq;
	uint64_t	t_unused1[5];
	irix_pid_t	t_rpid;
	int32_t		t_resched;
	int32_t		t_syserror;
	int32_t		t_nid;
	int32_t		t_affinity_nid;
	uint32_t	t_unused2[5];
	uint32_t	t_cpu;
	uint32_t	t_flags;
	irix_k_sigset_t	t_hold;
};
struct irix_prda {
	char	unused[2048];
	union {
		char	fill[512];
		uint32_t rsvd[8];
	} sys2_prda;
	union {
		char fill[512];
	} lib2_prda;
	union {
		char fill[512];
	} usr2rda;
	union {
		struct irix_prda_sys prda_sys;
		char fill[128];
	} sys_prda;
	union {
		char fill[256];
	} lib_prda;
	union {
		char fill[128];
	} usr_prda;
};

#endif /* _IRIX_IRIX_PRCTL_H_ */
