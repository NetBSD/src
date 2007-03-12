/*	$NetBSD: svr4_32_ipc.h,v 1.3.26.1 2007/03/12 05:52:48 rmind Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _SVR4_32_IPC_H_
#define _SVR4_32_IPC_H_

#include <compat/svr4/svr4_ipc.h>
/*
 * General IPC
 */
struct svr4_32_ipc_perm {
	svr4_32_uid_t	uid;
	svr4_32_gid_t	gid;
	svr4_32_uid_t	cuid;
	svr4_32_gid_t	cgid;
	svr4_32_mode_t	mode;
	netbsd32_u_long	seq;
	svr4_32_key_t	key;
	netbsd32_long	pad[4];
};
typedef netbsd32_caddr_t svr4_32_ipc_permp;
/*
 * Message queues
 */
typedef netbsd32_caddr_t svr4_32_msgp;
struct svr4_32_msg {
	svr4_32_msgp	msg_next;
	netbsd32_long	msg_type;
	u_short		msg_ts;
	short		msg_spot;
};

struct svr4_32_msqid_ds {
	struct svr4_32_ipc_perm msg_perm;
	svr4_32_msgp		msg_first;
	svr4_32_msgp		msg_last;
	netbsd32_u_long		msg_cbytes;
	netbsd32_u_long		msg_qnum;
	netbsd32_u_long		msg_qbytes;
	svr4_32_pid_t		msg_lspid;
	svr4_32_pid_t		msg_lrpid;
	svr4_32_time_t		msg_stime;
	netbsd32_long		msg_pad1;
	svr4_32_time_t		msg_rtime;
	netbsd32_long		msg_pad2;
	svr4_32_time_t		msg_ctime;
	netbsd32_long		msg_pad3;
	short			msg_cv;
	short			msg_qnum_cv;
	netbsd32_long		msg_pad4[3];
};
typedef netbsd32_caddr_t svr4_32_msqid_dsp;

struct svr4_32_msgbuf {
	netbsd32_long	mtype;		/* message type */
	char		mtext[1];	/* message text */
};
typedef netbsd32_caddr_t svr4_32_msgbufp;

typedef netbsd32_caddr_t svr4_32_msginfop;

/*
 * Shared memory
 */

struct svr4_32_shmid_ds {
	struct svr4_32_ipc_perm	shm_perm;
	int			shm_segsz;
	netbsd32_caddr_t 	shm_amp;
	u_short			shm_lkcnt;
	svr4_32_pid_t		shm_lpid;
	svr4_32_pid_t		shm_cpid;
	netbsd32_u_long		shm_nattch;
	netbsd32_u_long		shm_cnattch;
	svr4_32_time_t		shm_atime;
	netbsd32_long		shm_pad1;
	svr4_32_time_t		shm_dtime;
	netbsd32_long		shm_pad2;
	svr4_32_time_t		shm_ctime;
	netbsd32_long		shm_pad3;
	netbsd32_long		shm_pad4[4];
};
typedef netbsd32_caddr_t svr4_32_shmid_dsp;

/*
 * Semaphores
 */

typedef netbsd32_caddr_t svr4_32_semp;

struct svr4_32_semid_ds {
	struct svr4_32_ipc_perm sem_perm;
	svr4_32_semp		sem_base;
	u_short			sem_nsems;
	svr4_32_time_t		sem_otime;
	netbsd32_long		sem_pad1;
	svr4_32_time_t		sem_ctime;
	netbsd32_long		sem_pad2;
	netbsd32_long		sem_pad3[4];
};
typedef netbsd32_caddr_t svr4_32_semid_dsp;
typedef netbsd32_caddr_t svr4_32_sembufp;

#endif	/* _SVR4_32_IPC_H */
