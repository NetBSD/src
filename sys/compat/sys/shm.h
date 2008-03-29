/*	$NetBSD: shm.h,v 1.4.78.1 2008/03/29 20:46:59 christos Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Glass.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * As defined+described in "X/Open System Interfaces and Headers"
 *                         Issue 4, p. XXX
 */

#ifndef _COMPAT_SYS_SHM_H_
#define _COMPAT_SYS_SHM_H_

#ifdef _KERNEL
#include <compat/sys/ipc.h>

struct shmid_ds14 {
	struct ipc_perm14 shm_perm;	/* operation permission structure */
	int		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	short		shm_nattch;	/* number of current attaches */
	int32_t		shm_atime;	/* time of last shmat() */
	int32_t		shm_dtime;	/* time of last shmdt() */
	int32_t		shm_ctime;	/* time of last change by shmctl() */
	void		*shm_internal;	/* sysv stupidity */
};

struct shmid_ds13 {
	struct ipc_perm	shm_perm;	/* operation permission structure */
	size_t		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm operation */
	pid_t		shm_cpid;	/* process ID of creator */
	shmatt_t	shm_nattch;	/* number of current attaches */
	int32_t		shm_atime;	/* time of last shmat() */
	int32_t		shm_dtime;	/* time of last shmdt() */
	int32_t		shm_ctime;	/* time of last change by shmctl() */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	void		*_shm_internal;
};
#endif

#endif /* !_COMPAT_SYS_SHM_H_ */
