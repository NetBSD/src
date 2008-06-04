/* $NetBSD: linux32_sem.h,v 1.1.4.2 2008/06/04 02:05:05 yamt Exp $ */

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

#ifndef _LINUX32_SEM_H
#define _LINUX32_SEM_H

struct linux32_semid_ds {
	struct linux32_ipc_perm	l_sem_perm;
	linux32_time_t		l_sem_otime;
	linux32_time_t		l_sem_ctime;
	netbsd32_voidp		l_sem_base;
	netbsd32_voidp		l_eventn;
	netbsd32_voidp		l_eventz;
	netbsd32_voidp		l_undo;
	ushort			l_sem_nsems;
};

struct linux32_semid64_ds {
	struct linux32_ipc64_perm l_sem_perm;
	linux32_time_t		l_sem_otime;
	netbsd32_u_long		l___unused1;
	linux32_time_t		l_sem_ctime;
	netbsd32_u_long		l___unused2;
	netbsd32_u_long		l_sem_nsems;
	netbsd32_u_long		l___unused3;
	netbsd32_u_long		l___unused4;
};

typedef netbsd32_pointer_t linux32_semid_dsp;

union linux32_semun {
	int			l_val;
	linux32_semid_dsp	l_buf;
	netbsd32_u_shortp	l_array;
	netbsd32_voidp		l___buf;
	netbsd32_voidp		l___pad;
};

#define LINUX32_GETPID	11
#define LINUX32_GETVAL	12
#define LINUX32_GETALL	13
#define LINUX32_GETNCNT	14
#define LINUX32_GETZCNT	15
#define LINUX32_SETVAL	16
#define LINUX32_SETALL	17

#endif /* _LINUX32_SEM_H */
