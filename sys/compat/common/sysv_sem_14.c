/*	$NetBSD: sysv_sem_14.c,v 1.1.10.1 2000/06/22 17:05:44 minoura Exp $	*/

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
#include <sys/sem.h>

#define	SYSVSEM

#include <sys/syscallargs.h>

static void semid_ds14_to_native __P((struct semid_ds14 *, struct semid_ds *));
static void native_to_semid_ds14 __P((struct semid_ds *, struct semid_ds14 *));

static void
semid_ds14_to_native(osembuf, sembuf)
	struct semid_ds14 *osembuf;
	struct semid_ds *sembuf;
{

	ipc_perm14_to_native(&osembuf->sem_perm, &sembuf->sem_perm);

#define	CVT(x)	sembuf->x = osembuf->x
	CVT(sem_nsems);
	CVT(sem_otime);
	CVT(sem_ctime);
#undef CVT
}

static void
native_to_semid_ds14(sembuf, osembuf)
	struct semid_ds *sembuf;
	struct semid_ds14 *osembuf;
{

	native_to_ipc_perm14(&sembuf->sem_perm, &osembuf->sem_perm);

#define	CVT(x)	osembuf->x = sembuf->x
	CVT(sem_nsems);
	CVT(sem_otime);
	CVT(sem_ctime);
#undef CVT
}

int
compat_14_sys___semctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_14_sys___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun *) arg;
	} */ *uap = v;
	union __semun arg;
	struct semid_ds sembuf;
	struct semid_ds14 osembuf;
	int cmd, error;
	void *pass_arg = NULL;

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case IPC_SET:    
	case IPC_STAT:
		pass_arg = &sembuf;
		break;

	case GETALL:
	case SETVAL:
	case SETALL:
		pass_arg = &arg;
		break;
	}

	if (pass_arg != NULL) {
		error = copyin(SCARG(uap, arg), &arg, sizeof(arg));
		if (error)
			return (error);  
		if (cmd == IPC_SET) { 
			error = copyin(arg.buf, &osembuf, sizeof(osembuf));
			if (error)  
				return (error);
			semid_ds14_to_native(&osembuf, &sembuf);
		}
	}

	error = semctl1(p, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_semid_ds14(&sembuf, &osembuf);
		error = copyout(&osembuf, arg.buf, sizeof(osembuf));
	}

	return (error);
}
