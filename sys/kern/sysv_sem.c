/*	$NetBSD: sysv_sem.c,v 1.32.6.1 2000/06/01 17:58:03 he Exp $	*/

/*
 * Implementation of SVID semaphores
 *
 * Author:  Daniel Boulet
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#define SYSVSEM

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sem.h>
#include <sys/malloc.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

int	semtot = 0;

#ifdef SEM_DEBUG
#define SEM_PRINTF(a) printf a
#else
#define SEM_PRINTF(a)
#endif

struct sem_undo *semu_alloc __P((struct proc *));
int semundo_adjust __P((struct proc *, struct sem_undo **, int, int, int));
void semundo_clear __P((int, int));

/*
 * XXXSMP Once we go MP, there needs to be a lock for the semaphore system.
 * Until then, we're saved by being a non-preemptive kernel.
 */

void
seminit()
{
	register int i;

	if (sema == NULL)
		panic("sema is NULL");
	if (semu == NULL)
		panic("semu is NULL");

	for (i = 0; i < seminfo.semmni; i++) {
		sema[i].sem_base = 0;
		sema[i].sem_perm.mode = 0;
	}
	for (i = 0; i < seminfo.semmnu; i++) {
		register struct sem_undo *suptr = SEMU(i);
		suptr->un_proc = NULL;
	}
	semu_list = NULL;
}

/*
 * Placebo.
 */

int
sys_semconfig(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	*retval = 0;
	return 0;
}

/*
 * Allocate a new sem_undo structure for a process
 * (returns ptr to structure or NULL if no more room)
 */

struct sem_undo *
semu_alloc(p)
	struct proc *p;
{
	register int i;
	register struct sem_undo *suptr;
	register struct sem_undo **supptr;
	int attempt;

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
				return(suptr);
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
					supptr = &(suptr->un_next);
			}

			/* If we didn't free anything then just give-up */
			if (!did_something)
				return(NULL);
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
semundo_adjust(p, supptr, semid, semnum, adjval)
	register struct proc *p;
	struct sem_undo **supptr;
	int semid, semnum;
	int adjval;
{
	register struct sem_undo *suptr;
	register struct undo *sunptr;
	int i;

	/* Look for and remember the sem_undo if the caller doesn't provide
	   it */

	suptr = *supptr;
	if (suptr == NULL) {
		for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next) {
			if (suptr->un_proc == p) {
				*supptr = suptr;
				break;
			}
		}
		if (suptr == NULL) {
			if (adjval == 0)
				return(0);
			suptr = semu_alloc(p);
			if (suptr == NULL)
				return(ENOSPC);
			*supptr = suptr;
		}
	}

	/*
	 * Look for the requested entry and adjust it (delete if adjval becomes
	 * 0).
	 */
	sunptr = &suptr->un_ent[0];
	for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
		if (sunptr->un_id != semid || sunptr->un_num != semnum)
			continue;
		if (adjval == 0)
			sunptr->un_adjval = 0;
		else
			sunptr->un_adjval += adjval;
		if (sunptr->un_adjval == 0) {
			suptr->un_cnt--;
			if (i < suptr->un_cnt)
				suptr->un_ent[i] =
				    suptr->un_ent[suptr->un_cnt];
		}
		return(0);
	}

	/* Didn't find the right entry - create it */
	if (adjval == 0)
		return(0);
	if (suptr->un_cnt == SEMUME)
		return(EINVAL);

	sunptr = &suptr->un_ent[suptr->un_cnt];
	suptr->un_cnt++;
	sunptr->un_adjval = adjval;
	sunptr->un_id = semid;
	sunptr->un_num = semnum;
	return(0);
}

void
semundo_clear(semid, semnum)
	int semid, semnum;
{
	register struct sem_undo *suptr;

	for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next) {
		register struct undo *sunptr;
		register int i;

		sunptr = &suptr->un_ent[0];
		for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
			if (sunptr->un_id == semid) {
				if (semnum == -1 || sunptr->un_num == semnum) {
					suptr->un_cnt--;
					if (i < suptr->un_cnt) {
						suptr->un_ent[i] =
						  suptr->un_ent[suptr->un_cnt];
						i--, sunptr--;
					}
				}
				if (semnum != -1)
					break;
			}
		}
	}
}

int
sys___semctl(p, v, retval)
	struct proc *p;
	register void *v;
	register_t *retval;
{
	register struct sys___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union semun *) arg;
	} */ *uap = v;
	int semid = SCARG(uap, semid);
	int semnum = SCARG(uap, semnum);
	int cmd = SCARG(uap, cmd);
	union semun *arg = SCARG(uap, arg);
	union semun real_arg;
	struct ucred *cred = p->p_ucred;
	int i, rval, eval;
	struct semid_ds sbuf;
	register struct semid_ds *semaptr;

	SEM_PRINTF(("call to semctl(%d, %d, %d, %p)\n",
	    semid, semnum, cmd, arg));

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmsl)
		return(EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(SCARG(uap, semid)))
		return(EINVAL);

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)) != 0)
			return(eval);
		semaptr->sem_perm.cuid = cred->cr_uid;
		semaptr->sem_perm.uid = cred->cr_uid;
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i].sem_base > semaptr->sem_base)
				sema[i].sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	case IPC_SET:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		if ((eval = copyin(real_arg.buf, (caddr_t)&sbuf,
		    sizeof(sbuf))) != 0)
			return(eval);
		semaptr->sem_perm.uid = sbuf.sem_perm.uid;
		semaptr->sem_perm.gid = sbuf.sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (sbuf.sem_perm.mode & 0777);
		semaptr->sem_ctime = time.tv_sec;
		break;

	case IPC_STAT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		eval = copyout((caddr_t)semaptr, real_arg.buf,
		    sizeof(struct semid_ds));
		break;

	case GETNCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->sem_base[semnum].semval;
		break;

	case GETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyout((caddr_t)&semaptr->sem_base[i].semval,
			    &real_arg.array[i], sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		break;

	case GETZCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		semaptr->sem_base[semnum].semval = real_arg.val;
		semundo_clear(semid, semnum);
		wakeup((caddr_t)semaptr);
		break;

	case SETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyin(&real_arg.array[i],
			    (caddr_t)&semaptr->sem_base[i].semval,
			    sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	default:
		return(EINVAL);
	}

	if (eval == 0)
		*retval = rval;
	return(eval);
}

int
sys_semget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_semget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ *uap = v;
	int semid, eval;
	int key = SCARG(uap, key);
	int nsems = SCARG(uap, nsems);
	int semflg = SCARG(uap, semflg);
	struct ucred *cred = p->p_ucred;

	SEM_PRINTF(("semget(0x%x, %d, 0%o)\n", key, nsems, semflg));

	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].sem_perm.key == key)
				break;
		}
		if (semid < seminfo.semmni) {
			SEM_PRINTF(("found public key\n"));
			if ((eval = ipcperm(cred, &sema[semid].sem_perm,
			    semflg & 0700)))
				return(eval);
			if (nsems > 0 && sema[semid].sem_nsems < nsems) {
				SEM_PRINTF(("too small\n"));
				return(EINVAL);
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
				SEM_PRINTF(("not exclusive\n"));
				return(EEXIST);
			}
			goto found;
		}
	}

	SEM_PRINTF(("need to allocate the semid_ds\n"));
	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			SEM_PRINTF(("nsems out of range (0<%d<=%d)\n", nsems,
			    seminfo.semmsl));
			return(EINVAL);
		}
		if (nsems > seminfo.semmns - semtot) {
			SEM_PRINTF(("not enough semaphores left (need %d, got %d)\n",
			    nsems, seminfo.semmns - semtot));
			return(ENOSPC);
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) == 0)
				break;
		}
		if (semid == seminfo.semmni) {
			SEM_PRINTF(("no more semid_ds's available\n"));
			return(ENOSPC);
		}
		SEM_PRINTF(("semid %d is available\n", semid));
		sema[semid].sem_perm.key = key;
		sema[semid].sem_perm.cuid = cred->cr_uid;
		sema[semid].sem_perm.uid = cred->cr_uid;
		sema[semid].sem_perm.cgid = cred->cr_gid;
		sema[semid].sem_perm.gid = cred->cr_gid;
		sema[semid].sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].sem_perm.seq =
		    (sema[semid].sem_perm.seq + 1) & 0x7fff;
		sema[semid].sem_nsems = nsems;
		sema[semid].sem_otime = 0;
		sema[semid].sem_ctime = time.tv_sec;
		sema[semid].sem_base = &sem[semtot];
		semtot += nsems;
		memset(sema[semid].sem_base, 0,
		    sizeof(sema[semid].sem_base[0])*nsems);
		SEM_PRINTF(("sembase = %p, next = %p\n", sema[semid].sem_base,
		    &sem[semtot]));
	} else {
		SEM_PRINTF(("didn't find it and wasn't asked to create it\n"));
		return(ENOENT);
	}

found:
	*retval = IXSEQ_TO_IPCID(semid, sema[semid].sem_perm);
	return(0);
}

int
sys_semop(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_semop_args /* {
		syscallarg(int) semid;
		syscallarg(struct sembuf *) sops;
		syscallarg(size_t) nsops;
	} */ *uap = v;
	int semid = SCARG(uap, semid);
	int nsops = SCARG(uap, nsops);
	struct sembuf sops[MAX_SOPS];
	register struct semid_ds *semaptr;
	register struct sembuf *sopptr = NULL;
	register struct sem *semptr = NULL;
	struct sem_undo *suptr = NULL;
	struct ucred *cred = p->p_ucred;
	int i, j, eval;
	int do_wakeup, do_undos;

	SEM_PRINTF(("call to semop(%d, %p, %d)\n", semid, sops, nsops));

	semid = IPCID_TO_IX(semid);	/* Convert back to zero origin */

	if (semid < 0 || semid >= seminfo.semmsl)
		return(EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(SCARG(uap, semid)))
		return(EINVAL);

	if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W))) {
		SEM_PRINTF(("eval = %d from ipaccess\n", eval));
		return(eval);
	}

	if (nsops > MAX_SOPS) {
		SEM_PRINTF(("too many sops (max=%d, nsops=%d)\n", MAX_SOPS, nsops));
		return(E2BIG);
	}

	if ((eval = copyin(SCARG(uap, sops), sops, nsops * sizeof(sops[0])))
	    != 0) {
		SEM_PRINTF(("eval = %d from copyin(%p, %p, %d)\n", eval,
		    SCARG(uap, sops), &sops, nsops * sizeof(sops[0])));
		return(eval);
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

			if (sopptr->sem_num >= semaptr->sem_nsems)
				return(EFBIG);

			semptr = &semaptr->sem_base[sopptr->sem_num];

			SEM_PRINTF(("semop:  semaptr=%x, sem_base=%x, semptr=%x, sem[%d]=%d : op=%d, flag=%s\n",
			    semaptr, semaptr->sem_base, semptr,
			    sopptr->sem_num, semptr->semval, sopptr->sem_op,
			    (sopptr->sem_flg & IPC_NOWAIT) ? "nowait" : "wait"));

			if (sopptr->sem_op < 0) {
				if ((int)(semptr->semval +
					  sopptr->sem_op) < 0) {
					SEM_PRINTF(("semop:  can't do it now\n"));
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
		SEM_PRINTF(("semop:  rollback 0 through %d\n", i-1));
		for (j = 0; j < i; j++)
			semaptr->sem_base[sops[j].sem_num].semval -=
			    sops[j].sem_op;

		/*
		 * If the request that we couldn't satisfy has the
		 * NOWAIT flag set then return with EAGAIN.
		 */
		if (sopptr->sem_flg & IPC_NOWAIT)
			return(EAGAIN);

		if (sopptr->sem_op == 0)
			semptr->semzcnt++;
		else
			semptr->semncnt++;

		SEM_PRINTF(("semop:  good night!\n"));
		eval = tsleep((caddr_t)semaptr, (PZERO - 4) | PCATCH,
		    "semwait", 0);
		SEM_PRINTF(("semop:  good morning (eval=%d)!\n", eval));

		suptr = NULL;	/* sem_undo may have been reallocated */

		if (eval != 0)
			return(EINTR);
		SEM_PRINTF(("semop:  good morning!\n"));

		/*
		 * Make sure that the semaphore still exists
		 */
		if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
		    semaptr->sem_perm.seq != IPCID_TO_SEQ(SCARG(uap, semid))) {
			/* The man page says to return EIDRM. */
			/* Unfortunately, BSD doesn't define that code! */
#ifdef EIDRM
			return(EIDRM);
#else
			return(EINVAL);
#endif
		}

		/*
		 * The semaphore is still alive.  Readjust the count of
		 * waiting processes.
		 */
		if (sopptr->sem_op == 0)
			semptr->semzcnt--;
		else
			semptr->semncnt--;
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
			eval = semundo_adjust(p, &suptr, semid,
			    sops[i].sem_num, -adjval);
			if (eval == 0)
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
			for (j = i - 1; j >= 0; j--) {
				if ((sops[j].sem_flg & SEM_UNDO) == 0)
					continue;
				adjval = sops[j].sem_op;
				if (adjval == 0)
					continue;
				if (semundo_adjust(p, &suptr, semid,
				    sops[j].sem_num, adjval) != 0)
					panic("semop - can't undo undos");
			}

			for (j = 0; j < nsops; j++)
				semaptr->sem_base[sops[j].sem_num].semval -=
				    sops[j].sem_op;

			SEM_PRINTF(("eval = %d from semundo_adjust\n", eval));
			return(eval);
		} /* loop through the sops */
	} /* if (do_undos) */

	/* We're definitely done - set the sempid's */
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semaptr->sem_base[sopptr->sem_num];
		semptr->sempid = p->p_pid;
	}

	/* Do a wakeup if any semaphore was up'd. */
	if (do_wakeup) {
		SEM_PRINTF(("semop:  doing wakeup\n"));
#ifdef SEM_WAKEUP
		sem_wakeup((caddr_t)semaptr);
#else
		wakeup((caddr_t)semaptr);
#endif
		SEM_PRINTF(("semop:  back from wakeup\n"));
	}
	SEM_PRINTF(("semop:  done\n"));
	*retval = 0;
	return(0);
}

/*
 * Go through the undo structures for this process and apply the adjustments to
 * semaphores.
 */
void
semexit(p)
	struct proc *p;
{
	register struct sem_undo *suptr;
	register struct sem_undo **supptr;

	/*
	 * Go through the chain of undo vectors looking for one associated with
	 * this process.
	 */

	for (supptr = &semu_list; (suptr = *supptr) != NULL;
	    supptr = &suptr->un_next) {
		if (suptr->un_proc == p)
			break;
	}

	/*
	 * If there is no undo vector, skip to the end.
	 */

	if (suptr == NULL)
		return;
	
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

			SEM_PRINTF(("semexit:  %p id=%d num=%d(adj=%d) ; sem=%d\n",
			    suptr->un_proc, suptr->un_ent[ix].un_id,
			    suptr->un_ent[ix].un_num,
			    suptr->un_ent[ix].un_adjval,
			    semaptr->sem_base[semnum].semval));

			if (adjval < 0 &&
			    semaptr->sem_base[semnum].semval < -adjval)
				semaptr->sem_base[semnum].semval = 0;
			else
				semaptr->sem_base[semnum].semval += adjval;

#ifdef SEM_WAKEUP
			sem_wakeup((caddr_t)semaptr);
#else
			wakeup((caddr_t)semaptr);
#endif
			SEM_PRINTF(("semexit:  back from wakeup\n"));
		}
	}

	/*
	 * Deallocate the undo vector.
	 */
	SEM_PRINTF(("removing vector\n"));
	suptr->un_proc = NULL;
	*supptr = suptr->un_next;
}
