/*	$NetBSD: hpux_ipc.h,v 1.1 1999/08/25 04:50:08 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _COMPAT_HPUX_HPUX_IPC_H_
#define	_COMPAT_HPUX_HPUX_IPC_H_

/*
 * HP-UX 9.x hp9000s300 compatible IPC data structures.
 */

#include <compat/hpux/hpux_types.h>

struct hpux_ipc_perm {
	hpux_uid_t	uid;
	hpux_gid_t	gid;
	hpux_uid_t	cuid;
	hpux_gid_t	cgid;
	hpux_mode_t	mode;
	u_short		seq;
	hpux_key_t	key;
	char		pad[20];
};

#define	HPUX_IPC_CREAT		0001000		/* same as NetBSD */
#define	HPUX_IPC_EXCL		0002000		/* same as NetBSD */
#define	HPUX_IPC_NOWAIT		0004000		/* same as NetBSD */

#define	HPUX_IPC_PRIVATE	(hpux_key_t)0	/* same as NetBSD */

#define	HPUX_IPC_RMID		0		/* same as NetBSD */
#define	HPUX_IPC_SET		1		/* same as NetBSD */
#define	HPUX_IPC_STAT		2		/* same as NetBSD */

struct hpux_oipc_perm {
	u_short		uid;
	u_short		gid;
	u_short		cuid;
	u_short		cgid;
	u_short		mode;
	u_short		seq;
	hpux_key_t	key;
};

struct hpux_msqid_ds {
	struct hpux_ipc_perm msg_perm;
	void		*msg_first;
	void		*msg_last;
	u_short		msg_qnum;
	u_short		msg_qbytes;
	hpux_pid_t	msg_lspid;
	hpux_pid_t	msg_lrpid;
	hpux_time_t	msg_stime;
	hpux_time_t	msg_rtime;
	hpux_time_t	msg_ctime;
	u_short		msg_cbytes;
	char		pad[22];
};

struct hpux_omsqid_ds {
	struct hpux_oipc_perm msg_perm;
	void		*msg_first;
	void		*msg_last;
	u_short		msg_cbytes;
	u_short		msg_qnum;
	u_short		msg_qbytes;
	u_short		msg_lspid;
	u_short		msg_lrpid;
	hpux_time_t	msg_stime;
	hpux_time_t	msg_rtime;
	hpux_time_t	msg_ctime;
};

#define	HPUX_SEM_UNDO	010000			/* same as NetBSD */
#define	HPUX_GETNCNT	3			/* same as NetBSD */
#define	HPUX_GETPID	4			/* same as NetBSD */
#define	HPUX_GETVAL	5			/* same as NetBSD */
#define	HPUX_GETALL	6			/* same as NetBSD */
#define	HPUX_GETZCNT	7			/* same as NetBSD */
#define	HPUX_SETVAL	8			/* same as NetBSD */
#define	HPUX_SETALL	9			/* same as NetBSD */

struct hpux_semid_ds {
	struct hpux_ipc_perm sem_perm;
	void		*sem_base;
	hpux_time_t	sem_otime;
	hpux_time_t	sem_ctime;
	u_short		sem_nsems;
	char		pad[22];
};

struct hpux_sembuf {				/* same as NetBSD */
	u_short		sem_num;
	short		sem_op;
	short		sem_flg;
};

struct hpux_osemid_ds {
	struct hpux_oipc_perm sem_perm;
	void		*sem_base;
	u_short		sem_nsems;
	hpux_time_t	sem_otime;
	hpux_time_t	sem_ctime;
};

/*
 * This is not always the same on NetBSD's m68k platforms, but should
 * always be the same on any NetBSD platform that can run HP-UX binaries.
 */
#define	HPUX_SHMLBA	4096

#define	HPUX_SHM_RDONLY	010000			/* same as NetBSD */
#define	HPUX_SHM_RND	020000			/* same as NetBSD */

struct hpux_shmid_ds {
	struct hpux_ipc_perm shm_perm;
	int		shm_segsz;
	void		*shm_vas;
	hpux_pid_t	shm_lpid;
	hpux_pid_t	shm_cpid;
	u_short		shm_nattch;	/* current # attached (accurate) */
	u_short		shm_cnattch;	/* in memory # attached (inaccurate) */
	hpux_time_t	shm_atime;
	hpux_time_t	shm_dtime;
	hpux_time_t	shm_ctime;
	char		pad[24];
};

struct hpux_oshmid_ds {
	struct hpux_oipc_perm shm_perm;
	int		shm_segsz;
	void		*shm_vas;
	u_short		shm_lpid;
	u_short		shm_cpid;
	u_short		shm_nattch;
	u_short		shm_cnattch;
	hpux_time_t	shm_atime;
	hpux_time_t	shm_dtime;
	hpux_time_t	shm_ctime;
	char		pad[60];
};

#define	HPUX_SHM_LOCK	3			/* same as NetBSD */
#define	HPUX_SHM_UNLOCK	4			/* same as NetBSD */

#endif /* _COMPAT_HPUX_HPUX_IPC_H_ */
