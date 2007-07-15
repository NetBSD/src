/*	$NetBSD: sysv_sem.c,v 1.68.2.2 2007/07/15 15:52:57 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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

/*
 * Implementation of SVID semaphores
 *
 * Author: Daniel Boulet
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysv_sem.c,v 1.68.2.2 2007/07/15 15:52:57 ad Exp $");

#define SYSVSEM

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sem.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>
#include <sys/mount.h>		/* XXX for <sys/syscallargs.h> */
#include <sys/syscallargs.h>
#include <sys/kauth.h>

static int	semtot = 0;
struct	semid_ds *sema;			/* semaphore id pool */
static struct	__sem *sem;		/* semaphore pool */
static struct	sem_undo *semu_list;	/* list of active undo structures */
static int	*semu;			/* undo structure pool */
static kcondvar_t *semcv;
static kmutex_t semlock;

#ifdef SEM_DEBUG
#define SEM_PRINTF(a) printf a
#else
#define SEM_PRINTF(a)
#endif

struct sem_undo *semu_alloc(struct proc *);
int semundo_adjust(struct proc *, struct sem_undo **, int, int, int);
void semundo_clear(int, int);

/*
 * XXXSMP Once we go MP, there needs to be a lock for the semaphore system.
 * Until then, we're saved by being a non-preemptive kernel.
 */

void
seminit(void)
{
	int i, sz;
	vaddr_t v;

	mutex_init(&semlock, MUTEX_DEFAULT, IPL_NONE);

	/* Allocate pageable memory for our structures */
	sz = seminfo.semmni * sizeof(struct semid_ds) +
	    seminfo.semmns * sizeof(struct __sem) +
	    seminfo.semmnu * seminfo.semusz +
	    seminfo.semmni * sizeof(kcondvar_t);
	v = uvm_km_alloc(kernel_map, round_page(sz), 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	if (v == 0)
		panic("sysv_sem: cannot allocate memory");
	sema = (void *)v;
	sem = (void *)(sema + seminfo.semmni);
	semu = (void *)(sem + seminfo.semmns);
	semcv = (void *)(semu + seminfo.semmnu);

	for (i = 0; i < seminfo.semmni; i++) {
		sema[i]._sem_base = 0;
		sema[i].sem_perm.mode = 0;
		cv_init(&semcv[i], "semwait");
	}
	for (i = 0; i < seminfo.semmnu; i++) {
		struct sem_undo *suptr = SEMU(i);
		suptr->un_proc = NULL;
	}
	semu_list = NULL;
	exithook_establish(semexit, NULL);
}

/*
 * Placebo.
 */

int
sys_semconfig(struct lwp *l, void *v, register_t *retval)
{

	*retval = 0;
	return 0;
}

/*
 * Allocate a new sem_undo structure for a process
 * (returns ptr to structure or NULL if no more room)
 */

struct sem_undo *
semu_alloc(struct proc *p)
{
	int i;
	struct sem_undo *suptr;
	struct sem_undo **supptr;
	int attempt;

	KASSERT(mutex_owned(&semlock));

	/*
	 * Try twice to allocate something.
	 * (we'll purge any empty structures after the first pass so
	 * two passes are always enough)
	 */

	for (attempt = 0; attempt < 2; attempt++) {
		/*
		 * Look for a free structure.
		 * Fill it in and return it if we find one.
		 */

		for (i = 0; i < seminfo.semmnu; i++) {
			suptr = SEMU(i);
			if (suptr->un_proc == NULL) {
				suptr->un_next = semu_list;
				semu_list = suptr;
				suptr->un_cnt = 0;
				suptr->un_proc = p;
				return (suptr);
			}
		}

		/*
		 * We didn't find a free one, if this is the first attempt
		 * then try to free some structures.
		 */

		if (attempt == 0) {
			/* All the structures are in use - try to free some */
			int did_something = 0;

			supptr = &semu_list;
			while ((suptr = *supptr) != NULL) {
				if (suptr->un_cnt == 0)  {
					suptr->un_proc = NULL;
					*supptr = suptr->un_next;
					did_something = 1;
				} else
					supptr = &suptr->un_next;
			}

			/* If we didn't free anything then just give-up */
			if (!did_something)
				return (NULL);
		} else {
			/*
			 * The second pass failed even though we freed
			 * something after the first pass!
			 * This is IMPOSSIBLE!
			 */
			panic("semu_alloc - second attempt failed");
		}
	}
	return NULL;
}

/*
 * Adjust a particular entry for a particular proc
 */

int
semundo_adjust(struct proc *p, struct sem_undo **supptr, int semid, int semnum,
    int adjval)
{
	struct sem_undo *suptr;
	struct undo *sunptr;
	int i;

	KASSERT(mutex_owned(&semlock));

	/*
	 * Look for and remember the sem_undo if the caller doesn't
	 * provide it
	 */

	suptr = *supptr;
	if (suptr == NULL) {
		for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next)
			if (suptr->un_proc == p)
				break;

		if (suptr == NULL) {
			suptr = semu_alloc(p);
			if (suptr == NULL)
				return (ENOSPC);
		}
		*supptr = suptr;
	}

	/*
	 * Look for the requested entry and adjust it (delete if
	 * adjval becomes 0).
	 */
	sunptr = &suptr->un_ent[0];
	for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
		if (sunptr->un_id != semid || sunptr->un_num != semnum)
			continue;
		sunptr->un_adjval += adjval;
		if (sunptr->un_adjval == 0) {
			suptr->un_cnt--;
			if (i < suptr->un_cnt)
				suptr->un_ent[i] =
				    suptr->un_ent[suptr->un_cnt];
		}
		return (0);
	}

	/* Didn't find the right entry - create it */
	if (suptr->un_cnt == SEMUME)
		return (EINVAL);

	sunptr = &suptr->un_ent[suptr->un_cnt];
	suptr->un_cnt++;
	sunptr->un_adjval = adjval;
	sunptr->un_id = semid;
	sunptr->un_num = semnum;
	return (0);
}

void
semundo_clear(int semid, int semnum)
{
	struct sem_undo *suptr;
	struct undo *sunptr, *sunend;

	KASSERT(mutex_owned(&semlock));

	for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next)
		for (sunptr = &suptr->un_ent[0],
		    sunend = sunptr + suptr->un_cnt; sunptr < sunend;) {
			if (sunptr->un_id == semid) {
				if (semnum == -1 || sunptr->un_num == semnum) {
					suptr->un_cnt--;
					sunend--;
					if (sunptr != sunend)
						*sunptr = *sunend;
					if (semnum != -1)
						break;
					else
						continue;
				}
			}
			sunptr++;
		}
}

int
sys_____semctl13(struct lwp *l, void *v, register_t *retval)
{
	struct sys_____semctl13_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun *) arg;
	} */ *uap = v;
	struct semid_ds sembuf;
	int cmd, error;
	void *pass_arg;
	union __semun karg;

	cmd = SCARG(uap, cmd);

	pass_arg = get_semctl_arg(cmd, &sembuf, &karg);

	if (pass_arg) {
		error = copyin(SCARG(uap, arg), &karg, sizeof(karg));
		if (error)
			return error;
		if (cmd == IPC_SET) {
			error = copyin(karg.buf, &sembuf, sizeof(sembuf));
			if (error)
				return (error);
		}
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT)
		error = copyout(&sembuf, karg.buf, sizeof(sembuf));

	return (error);
}

int
semctl1(struct lwp *l, int semid, int semnum, int cmd, void *v,
    register_t *retval)
{
	kauth_cred_t cred = l->l_cred;
	union __semun *arg = v;
	struct semid_ds *sembuf = v, *semaptr;
	int i, error, ix;

	SEM_PRINTF(("call to semctl(%d, %d, %d, %p)\n",
	    semid, semnum, cmd, v));

	mutex_enter(&semlock);

	ix = IPCID_TO_IX(semid);
	if (ix < 0 || ix >= seminfo.semmni) {
		mutex_exit(&semlock);
		return (EINVAL);
	}

	semaptr = &sema[ix];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm._seq != IPCID_TO_SEQ(semid)) {
		mutex_exit(&semlock);
		return (EINVAL);
	}

	switch (cmd) {
	case IPC_RMID:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_M)) != 0)
			break;
		semaptr->sem_perm.cuid = kauth_cred_geteuid(cred);
		semaptr->sem_perm.uid = kauth_cred_geteuid(cred);
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->_sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i]._sem_base > semaptr->_sem_base)
				sema[i]._sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		semundo_clear(ix, -1);
		cv_broadcast(&semcv[ix]);
		break;

	case IPC_SET:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_M)))
			break;
		KASSERT(sembuf != NULL);
		semaptr->sem_perm.uid = sembuf->sem_perm.uid;
		semaptr->sem_perm.gid = sembuf->sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (sembuf->sem_perm.mode & 0777);
		semaptr->sem_ctime = time_second;
		break;

	case IPC_STAT:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		KASSERT(sembuf != NULL);
		memcpy(sembuf, semaptr, sizeof(struct semid_ds));
		break;

	case GETNCNT:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			break;
		}
		*retval = semaptr->_sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			break;
		}
		*retval = semaptr->_sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			break;
		}
		*retval = semaptr->_sem_base[semnum].semval;
		break;

	case GETALL:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		KASSERT(arg != NULL);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			error = copyout(&semaptr->_sem_base[i].semval,
			    &arg->array[i], sizeof(arg->array[i]));
			if (error != 0)
				break;
		}
		break;

	case GETZCNT:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			break;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			break;
		}
		*retval = semaptr->_sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			break;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			break;
		}
		KASSERT(arg != NULL);
		semaptr->_sem_base[semnum].semval = arg->val;
		semundo_clear(ix, semnum);
		cv_broadcast(&semcv[ix]);
		break;

	case SETALL:
		if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			break;
		KASSERT(arg != NULL);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			error = copyin(&arg->array[i],
			    &semaptr->_sem_base[i].semval,
			    sizeof(arg->array[i]));
			if (error != 0)
				break;
		}
		semundo_clear(ix, -1);
		cv_broadcast(&semcv[ix]);
		break;

	default:
		error = EINVAL;
		break;
	}

	mutex_exit(&semlock);
	return (error);
}

int
sys_semget(struct lwp *l, void *v, register_t *retval)
{
	struct sys_semget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ *uap = v;
	int semid, error = 0;
	int key = SCARG(uap, key);
	int nsems = SCARG(uap, nsems);
	int semflg = SCARG(uap, semflg);
	kauth_cred_t cred = l->l_cred;

	SEM_PRINTF(("semget(0x%x, %d, 0%o)\n", key, nsems, semflg));

	mutex_enter(&semlock);

	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].sem_perm._key == key)
				break;
		}
		if (semid < seminfo.semmni) {
			SEM_PRINTF(("found public key\n"));
			if ((error = ipcperm(cred, &sema[semid].sem_perm,
			    semflg & 0700)))
			    	goto out;
			if (nsems > 0 && sema[semid].sem_nsems < nsems) {
				SEM_PRINTF(("too small\n"));
				error = EINVAL;
				goto out;
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
				SEM_PRINTF(("not exclusive\n"));
				error = EEXIST;
				goto out;
			}
			goto found;
		}
	}

	SEM_PRINTF(("need to allocate the semid_ds\n"));
	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			SEM_PRINTF(("nsems out of range (0<%d<=%d)\n", nsems,
			    seminfo.semmsl));
			error = EINVAL;
			goto out;
		}
		if (nsems > seminfo.semmns - semtot) {
			SEM_PRINTF(("not enough semaphores left "
			    "(need %d, got %d)\n",
			    nsems, seminfo.semmns - semtot));
			error = ENOSPC;
			goto out;
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) == 0)
				break;
		}
		if (semid == seminfo.semmni) {
			SEM_PRINTF(("no more semid_ds's available\n"));
			error = ENOSPC;
			goto out;
		}
		SEM_PRINTF(("semid %d is available\n", semid));
		sema[semid].sem_perm._key = key;
		sema[semid].sem_perm.cuid = kauth_cred_geteuid(cred);
		sema[semid].sem_perm.uid = kauth_cred_geteuid(cred);
		sema[semid].sem_perm.cgid = kauth_cred_getegid(cred);
		sema[semid].sem_perm.gid = kauth_cred_getegid(cred);
		sema[semid].sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].sem_perm._seq =
		    (sema[semid].sem_perm._seq + 1) & 0x7fff;
		sema[semid].sem_nsems = nsems;
		sema[semid].sem_otime = 0;
		sema[semid].sem_ctime = time_second;
		sema[semid]._sem_base = &sem[semtot];
		semtot += nsems;
		memset(sema[semid]._sem_base, 0,
		    sizeof(sema[semid]._sem_base[0]) * nsems);
		SEM_PRINTF(("sembase = %p, next = %p\n", sema[semid]._sem_base,
		    &sem[semtot]));
	} else {
		SEM_PRINTF(("didn't find it and wasn't asked to create it\n"));
		error = ENOENT;
		goto out;
	}

 found:
	*retval = IXSEQ_TO_IPCID(semid, sema[semid].sem_perm);
 out:
	mutex_exit(&semlock);
	return (error);
}

#define SMALL_SOPS 8

int
sys_semop(struct lwp *l, void *v, register_t *retval)
{
	struct sys_semop_args /* {
		syscallarg(int) semid;
		syscallarg(struct sembuf *) sops;
		syscallarg(size_t) nsops;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int semid = SCARG(uap, semid), seq;
	size_t nsops = SCARG(uap, nsops);
	struct sembuf small_sops[SMALL_SOPS];
	struct sembuf *sops;
	struct semid_ds *semaptr;
	struct sembuf *sopptr = NULL;
	struct __sem *semptr = NULL;
	struct sem_undo *suptr = NULL;
	kauth_cred_t cred = l->l_cred;
	int i, error;
	int do_wakeup, do_undos;

	SEM_PRINTF(("call to semop(%d, %p, %zd)\n", semid, SCARG(uap,sops), nsops));

	if (nsops <= SMALL_SOPS) {
		sops = small_sops;
	} else if (nsops <= seminfo.semopm)
		sops = kmem_alloc(nsops * sizeof(*sops), KM_SLEEP);
	else {
		SEM_PRINTF(("too many sops (max=%d, nsops=%zd)\n",
		    seminfo.semopm, nsops));
		return (E2BIG);
	}

	mutex_enter(&semlock);

	semid = IPCID_TO_IX(semid);	/* Convert back to zero origin */
	if (semid < 0 || semid >= seminfo.semmni) {
		error = EINVAL;
		goto out;
	}

	semaptr = &sema[semid];
	seq = IPCID_TO_SEQ(SCARG(uap, semid));
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm._seq != seq) {
		error = EINVAL;
		goto out;
	}

	if ((error = ipcperm(cred, &semaptr->sem_perm, IPC_W))) {
		SEM_PRINTF(("error = %d from ipaccess\n", error));
		goto out;
	}

	if ((error = copyin(SCARG(uap, sops),
	    sops, nsops * sizeof(sops[0]))) != 0) {
		SEM_PRINTF(("error = %d from copyin(%p, %p, %zd)\n", error,
		    SCARG(uap, sops), &sops, nsops * sizeof(sops[0])));
		goto out;
	}

	for (i = 0; i < nsops; i++)
		if (sops[i].sem_num >= semaptr->sem_nsems) {
			error = EFBIG;
			goto out;
		}

	/*
	 * Loop trying to satisfy the vector of requests.
	 * If we reach a point where we must wait, any requests already
	 * performed are rolled back and we go to sleep until some other
	 * process wakes us up.  At this point, we start all over again.
	 *
	 * This ensures that from the perspective of other tasks, a set
	 * of requests is atomic (never partially satisfied).
	 */
	do_undos = 0;

	for (;;) {
		do_wakeup = 0;

		for (i = 0; i < nsops; i++) {
			sopptr = &sops[i];
			semptr = &semaptr->_sem_base[sopptr->sem_num];

			SEM_PRINTF(("semop:  semaptr=%p, sem_base=%p, "
			    "semptr=%p, sem[%d]=%d : op=%d, flag=%s\n",
			    semaptr, semaptr->_sem_base, semptr,
			    sopptr->sem_num, semptr->semval, sopptr->sem_op,
			    (sopptr->sem_flg & IPC_NOWAIT) ?
			    "nowait" : "wait"));

			if (sopptr->sem_op < 0) {
				if ((int)(semptr->semval +
				    sopptr->sem_op) < 0) {
					SEM_PRINTF(("semop:  "
					    "can't do it now\n"));
					break;
				} else {
					semptr->semval += sopptr->sem_op;
					if (semptr->semval == 0 &&
					    semptr->semzcnt > 0)
						do_wakeup = 1;
				}
				if (sopptr->sem_flg & SEM_UNDO)
					do_undos = 1;
			} else if (sopptr->sem_op == 0) {
				if (semptr->semval > 0) {
					SEM_PRINTF(("semop:  not zero now\n"));
					break;
				}
			} else {
				if (semptr->semncnt > 0)
					do_wakeup = 1;
				semptr->semval += sopptr->sem_op;
				if (sopptr->sem_flg & SEM_UNDO)
					do_undos = 1;
			}
		}

		/*
		 * Did we get through the entire vector?
		 */
		if (i >= nsops)
			goto done;

		/*
		 * No ... rollback anything that we've already done
		 */
		SEM_PRINTF(("semop:  rollback 0 through %d\n", i - 1));
		while (i-- > 0)
			semaptr->_sem_base[sops[i].sem_num].semval -=
			    sops[i].sem_op;

		/*
		 * If the request that we couldn't satisfy has the
		 * NOWAIT flag set then return with EAGAIN.
		 */
		if (sopptr->sem_flg & IPC_NOWAIT) {
			error = EAGAIN;
			goto out;
		}

		if (sopptr->sem_op == 0)
			semptr->semzcnt++;
		else
			semptr->semncnt++;

		SEM_PRINTF(("semop:  good night!\n"));
		error = cv_wait_sig(&semcv[semid], &semlock);
		SEM_PRINTF(("semop:  good morning (error=%d)!\n", error));

		/*
		 * Make sure that the semaphore still exists
		 */
		if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
		    semaptr->sem_perm._seq != seq) {
			error = EIDRM;
			goto out;
		}

		/*
		 * The semaphore is still alive.  Readjust the count of
		 * waiting processes.
		 */
		semptr = &semaptr->_sem_base[sopptr->sem_num];
		if (sopptr->sem_op == 0)
			semptr->semzcnt--;
		else
			semptr->semncnt--;
		/*
		 * Is it really morning, or was our sleep interrupted?
		 * (Delayed check of tsleep() return code because we
		 * need to decrement sem[nz]cnt either way.)
		 */
		if (error != 0) {
			error = EINTR;
			goto out;
		}
		SEM_PRINTF(("semop:  good morning!\n"));
	}

done:
	/*
	 * Process any SEM_UNDO requests.
	 */
	if (do_undos) {
		for (i = 0; i < nsops; i++) {
			/*
			 * We only need to deal with SEM_UNDO's for non-zero
			 * op's.
			 */
			int adjval;

			if ((sops[i].sem_flg & SEM_UNDO) == 0)
				continue;
			adjval = sops[i].sem_op;
			if (adjval == 0)
				continue;
			error = semundo_adjust(p, &suptr, semid,
			    sops[i].sem_num, -adjval);
			if (error == 0)
				continue;

			/*
			 * Oh-Oh!  We ran out of either sem_undo's or undo's.
			 * Rollback the adjustments to this point and then
			 * rollback the semaphore ups and down so we can return
			 * with an error with all structures restored.  We
			 * rollback the undo's in the exact reverse order that
			 * we applied them.  This guarantees that we won't run
			 * out of space as we roll things back out.
			 */
			while (i-- > 0) {
				if ((sops[i].sem_flg & SEM_UNDO) == 0)
					continue;
				adjval = sops[i].sem_op;
				if (adjval == 0)
					continue;
				if (semundo_adjust(p, &suptr, semid,
				    sops[i].sem_num, adjval) != 0)
					panic("semop - can't undo undos");
			}

			for (i = 0; i < nsops; i++)
				semaptr->_sem_base[sops[i].sem_num].semval -=
				    sops[i].sem_op;

			SEM_PRINTF(("error = %d from semundo_adjust\n", error));
			goto out;
		} /* loop through the sops */
	} /* if (do_undos) */

	/* We're definitely done - set the sempid's */
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semaptr->_sem_base[sopptr->sem_num];
		semptr->sempid = p->p_pid;
	}

	/* Update sem_otime */
	semaptr->sem_otime = time_second;

	/* Do a wakeup if any semaphore was up'd. */
	if (do_wakeup) {
		SEM_PRINTF(("semop:  doing wakeup\n"));
		cv_broadcast(&semcv[semid]);
		SEM_PRINTF(("semop:  back from wakeup\n"));
	}
	SEM_PRINTF(("semop:  done\n"));
	*retval = 0;

 out:
	mutex_exit(&semlock);
	if (sops != small_sops) {
		kmem_free(sops, nsops * sizeof(*sops));
	}
	return error;
}

/*
 * Go through the undo structures for this process and apply the
 * adjustments to semaphores.
 */
/*ARGSUSED*/
void
semexit(struct proc *p, void *v)
{
	struct sem_undo *suptr;
	struct sem_undo **supptr;

	mutex_enter(&semlock);

	/*
	 * Go through the chain of undo vectors looking for one
	 * associated with this process.
	 */

	for (supptr = &semu_list; (suptr = *supptr) != NULL;
	    supptr = &suptr->un_next) {
		if (suptr->un_proc == p)
			break;
	}

	/*
	 * If there is no undo vector, skip to the end.
	 */

	if (suptr == NULL) {
		mutex_exit(&semlock);
		return;
	}

	/*
	 * We now have an undo vector for this process.
	 */

	SEM_PRINTF(("proc @%p has undo structure with %d entries\n", p,
	    suptr->un_cnt));

	/*
	 * If there are any active undo elements then process them.
	 */
	if (suptr->un_cnt > 0) {
		int ix;

		for (ix = 0; ix < suptr->un_cnt; ix++) {
			int semid = suptr->un_ent[ix].un_id;
			int semnum = suptr->un_ent[ix].un_num;
			int adjval = suptr->un_ent[ix].un_adjval;
			struct semid_ds *semaptr;

			semaptr = &sema[semid];
			if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0)
				panic("semexit - semid not allocated");
			if (semnum >= semaptr->sem_nsems)
				panic("semexit - semnum out of range");

			SEM_PRINTF(("semexit:  %p id=%d num=%d(adj=%d) ; "
			    "sem=%d\n",
			    suptr->un_proc, suptr->un_ent[ix].un_id,
			    suptr->un_ent[ix].un_num,
			    suptr->un_ent[ix].un_adjval,
			    semaptr->_sem_base[semnum].semval));

			if (adjval < 0 &&
			    semaptr->_sem_base[semnum].semval < -adjval)
				semaptr->_sem_base[semnum].semval = 0;
			else
				semaptr->_sem_base[semnum].semval += adjval;

			cv_broadcast(&semcv[semid]);
			SEM_PRINTF(("semexit:  back from wakeup\n"));
		}
	}

	/*
	 * Deallocate the undo vector.
	 */
	SEM_PRINTF(("removing vector\n"));
	suptr->un_proc = NULL;
	*supptr = suptr->un_next;
	mutex_exit(&semlock);
}
