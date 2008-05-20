/* $NetBSD: linux32_shm.h,v 1.1 2008/05/20 17:31:56 njoly Exp $ */

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

#ifndef _LINUX32_SHM_H
#define _LINUX32_SHM_H

struct linux32_shmid_ds {
	struct linux32_ipc_perm	l_shm_perm;
	int			l_shm_segsz;
	linux32_time_t		l_shm_atime;
	linux32_time_t		l_shm_dtime;
	linux32_time_t		l_shm_ctime;
	ushort			l_shm_cpid;
	ushort			l_shm_lpid;
	short			l_shm_nattch;
	ushort			l_private1;
	netbsd32_voidp		l_private2;
	netbsd32_voidp		l_private3;
};

struct linux32_shmid64_ds {
	struct linux32_ipc64_perm l_shm_perm;
	netbsd32_size_t		l_shm_segsz;
	linux32_time_t		l_shm_atime;
	netbsd32_u_long		l____unused1;
	linux32_time_t		l_shm_dtime;
	netbsd32_u_long		l____unused2;
	linux32_time_t		l_shm_ctime;
	netbsd32_u_long		l____unused3;
	int			l_shm_cpid;
	int			l_shm_lpid;
	netbsd32_u_long		l_shm_nattch;
	netbsd32_u_long		l___unused4;
	netbsd32_u_long		l___unused5;
};

struct linux32_shminfo64 {
	netbsd32_u_long		l_shmmax;
	netbsd32_u_long		l_shmmin;
	netbsd32_u_long		l_shmmni;
	netbsd32_u_long		l_shmseg;
	netbsd32_u_long		l_shmall;
	netbsd32_u_long		l___unused1;
	netbsd32_u_long		l___unused2;
	netbsd32_u_long		l___unused3;
	netbsd32_u_long		l___unused4;
};

struct linux32_shm_info {
	int			l_used_ids;
	netbsd32_u_long		l_shm_tot;
	netbsd32_u_long		l_shm_rss;
	netbsd32_u_long		l_shm_swp;
	netbsd32_u_long		l_swap_attempts;
	netbsd32_u_long		l_swap_successes;
};

#define LINUX32_SHM_RDONLY	0x1000
#define LINUX32_SHM_RND		0x2000
#define LINUX32_SHM_REMAP	0x4000

#define LINUX32_SHM_LOCK	11
#define LINUX32_SHM_UNLOCK	12
#define LINUX32_SHM_STAT	13
#define LINUX32_SHM_INFO	14

#endif /* _LINUX32_SHM_H */
