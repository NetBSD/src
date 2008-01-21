/*	$NetBSD: linux_shm.h,v 1.6.12.3 2008/01/21 09:41:28 yamt Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#ifndef _LINUX_SHM_H
#define _LINUX_SHM_H

#include <sys/shm.h>

/*
 * shm segment control structure
 */
struct linux_shmid_ds {
	struct linux_ipc_perm	l_shm_perm;
	int			l_shm_segsz;
	linux_time_t		l_shm_atime;
	linux_time_t		l_shm_dtime;
	linux_time_t		l_shm_ctime;
	ushort			l_shm_cpid;
	ushort			l_shm_lpid;
	short			l_shm_nattch;
	ushort			l_private1;
	void			*l_private2;
	void			*l_private3;
};

struct linux_shmid64_ds {
	struct linux_ipc64_perm	l_shm_perm;
	uint			l_shm_segsz;
	linux_time_t		l_shm_atime;
	u_long			l____unused1;
	linux_time_t		l_shm_dtime;
	u_long			l____unused2;
	linux_time_t		l_shm_ctime;
	u_long			l____unused3;
	int			l_shm_cpid;
	int			l_shm_lpid;
	u_long			l_shm_nattch;
	u_long			l___unused4;
	u_long			l___unused5;
};

struct linux_shminfo64 {
	u_long			l_shmmax;
	u_long			l_shmmin;
	u_long			l_shmmni;
	u_long			l_shmseg;
	u_long			l_shmall;
	u_long			l___unused1;
	u_long			l___unused2;
	u_long			l___unused3;
	u_long			l___unused4;
};

struct linux_shm_info {
	int			l_used_ids;
	u_long			l_shm_tot;
	u_long			l_shm_rss;
	u_long			l_shm_swp;
	u_long			l_swap_attempts;
	u_long			l_swap_successes;
};

#define LINUX_SHM_RDONLY	0x1000
#define LINUX_SHM_RND		0x2000
#define LINUX_SHM_REMAP		0x4000

#define LINUX_SHM_LOCK		11
#define LINUX_SHM_UNLOCK	12
#define LINUX_SHM_STAT		13
#define LINUX_SHM_INFO		14


/* Pretend the sys_shmat and sys_shmctl syscalls are defined */
struct linux_sys_shmat_args {
	syscallarg(int) shmid;
	syscallarg(void *) shmaddr;
	syscallarg(int) shmflg;
	syscallarg(u_long *) raddr;
};

struct linux_sys_shmctl_args {
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(struct linux_shmid_ds *) buf;
};

struct linux_sys_shmget_args {
	syscallarg(key_t) key;
	syscallarg(size_t) size;
	syscallarg(int) shmflg;
};

#ifdef SYSVSHM
#ifdef _KERNEL
__BEGIN_DECLS
int linux_sys_shmget(struct lwp *, const struct linux_sys_shmget_args *, register_t *);
int linux_sys_shmat(struct lwp *, const struct linux_sys_shmat_args *, register_t *);
int linux_sys_shmctl(struct lwp *, const struct linux_sys_shmctl_args *, register_t *);
void linux_to_bsd_shmid_ds(struct linux_shmid_ds *,
    struct shmid_ds *);
void linux_to_bsd_shmid64_ds(struct linux_shmid64_ds *,
    struct shmid_ds *);
void bsd_to_linux_shmid_ds(struct shmid_ds *,
    struct linux_shmid_ds *);
void bsd_to_linux_shmid64_ds(struct shmid_ds *,
    struct linux_shmid64_ds *);
__END_DECLS
#endif	/* !_KERNEL */
#endif	/* !SYSVSHM */

#endif /* !_LINUX_SHM_H */
