/* $NetBSD: linux32_ipc.h,v 1.3 2009/11/16 13:32:40 joerg Exp $ */

/*
 * Copyright (c) 2008 Nicolas Joly
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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

#ifndef _LINUX32_IPC_H
#define _LINUX32_IPC_H

typedef	int32_t	linux32_key_t;

struct linux32_ipc_perm {
	linux32_key_t	l_key;
	ushort		l_uid;
	ushort		l_gid;
	ushort		l_cuid;
	ushort		l_cgid;
	ushort		l_mode;
	ushort		l_seq;
};

struct linux32_ipc64_perm {
	linux32_key_t	l_key;
	uint		l_uid;
	uint		l_gid;
	uint		l_cuid;
	uint		l_cgid;
	ushort		l_mode;
	ushort		l___pad1;
	ushort		l_seq;
	ushort		l___pad2;
	netbsd32_u_long	l___unused1;
	netbsd32_u_long	l___unused2;
};

struct linux32_msqid_ds {
	struct linux32_ipc_perm	l_msg_perm;
	netbsd32_pointer_t	l_msg_first;
	netbsd32_pointer_t	l_msg_last;
	linux_time_t		l_msg_stime;
	linux_time_t		l_msg_rtime;
	linux_time_t		l_msg_ctime;
	netbsd32_pointer_t	l_wwait;	/* Linux internal */
	netbsd32_pointer_t	l_rwait;	/* Linux internal */
	ushort	l_msg_cbytes;
	ushort	l_msg_qnum;
	ushort	l_msg_qbytes;
	ushort	l_msg_lspid;
	ushort	l_msg_lrpid;
};

struct linux32_msqid64_ds {
	struct linux32_ipc64_perm	l_msg_perm;
	linux32_time_t		l_msg_stime;
	netbsd32_u_long		l___unused1;
	linux32_time_t		l_msg_rtime;
	netbsd32_u_long		l___unused2;
	linux32_time_t		l_msg_ctime;
	netbsd32_u_long		l___unused3;
	netbsd32_u_long		l_msg_cbytes;
	netbsd32_u_long		l_msg_qnum;
	netbsd32_u_long		l_msg_qbytes;
	int			l_msg_lspid;
	int			l_msg_lrpid;
	netbsd32_u_long		l___unused4;
	netbsd32_u_long		l___unused5;
	
};

#define LINUX32_IPC_RMID	0
#define LINUX32_IPC_SET		1
#define LINUX32_IPC_STAT	2
#define LINUX32_IPC_INFO	3

#define LINUX32_IPC_64		0x0100

#endif /* _LINUX32_IPC_H */
