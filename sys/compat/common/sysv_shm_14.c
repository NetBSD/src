/*	$NetBSD: sysv_shm_14.c,v 1.2 2000/06/02 15:53:04 simonb Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/shm.h>  

#define	SYSVSHM

#include <sys/syscallargs.h>

static void shmid_ds14_to_native __P((struct shmid_ds14 *, struct shmid_ds *));
static void native_to_shmid_ds14 __P((struct shmid_ds *, struct shmid_ds14 *));

static void
shmid_ds14_to_native(oshmbuf, shmbuf)
	struct shmid_ds14 *oshmbuf;
	struct shmid_ds *shmbuf;
{

	ipc_perm14_to_native(&oshmbuf->shm_perm, &shmbuf->shm_perm);

#define	CVT(x)	shmbuf->x = oshmbuf->x
	CVT(shm_segsz);
	CVT(shm_lpid);
	CVT(shm_cpid);
	CVT(shm_nattch);
	CVT(shm_atime);
	CVT(shm_dtime);
	CVT(shm_ctime);
#undef CVT
}

static void
native_to_shmid_ds14(shmbuf, oshmbuf)
	struct shmid_ds *shmbuf;
	struct shmid_ds14 *oshmbuf;
{

	native_to_ipc_perm14(&shmbuf->shm_perm, &oshmbuf->shm_perm);

#define	CVT(x)	oshmbuf->x = shmbuf->x
	CVT(shm_segsz);
	CVT(shm_lpid);
	CVT(shm_cpid);
	CVT(shm_nattch);
	CVT(shm_atime);
	CVT(shm_dtime);
	CVT(shm_ctime);
#undef CVT
}

int
compat_14_sys_shmctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_14_sys_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct shmid_ds14 *) buf;
	} */ *uap = v;
	struct shmid_ds shmbuf;
	struct shmid_ds14 oshmbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &oshmbuf, sizeof(oshmbuf));
		if (error) 
			return (error);
		shmid_ds14_to_native(&oshmbuf, &shmbuf);
	}

	error = shmctl1(p, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_shmid_ds14(&shmbuf, &oshmbuf);     
		error = copyout(&oshmbuf, SCARG(uap, buf), sizeof(oshmbuf));
	}

	return (error);
}
