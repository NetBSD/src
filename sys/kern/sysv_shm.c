/*	$NetBSD: sysv_shm.c,v 1.87 2006/05/14 21:15:11 elad Exp $	*/

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

/*
 * Copyright (c) 1994 Adam Glass and Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Adam Glass and Charles M.
 *	Hannum.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysv_shm.c,v 1.87 2006/05/14 21:15:11 elad Exp $");

#define SYSVSHM

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/shm.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/mount.h>		/* XXX for <sys/syscallargs.h> */
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

static MALLOC_DEFINE(M_SHM, "shm", "SVID compatible shared memory segments");

/*
 * Provides the following externally accessible functions:
 *
 * shminit(void);		                 initialization
 * shmexit(struct vmspace *)                     cleanup
 * shmfork(struct vmspace *, struct vmspace *)   fork handling
 *
 * Structures:
 * shmsegs (an array of 'struct shmid_ds')
 * per proc array of 'struct shmmap_state'
 */

int shm_nused;
struct	shmid_ds *shmsegs;

struct shmmap_entry {
	SLIST_ENTRY(shmmap_entry) next;
	vaddr_t va;
	int shmid;
};

static int	shm_last_free, shm_committed;

static POOL_INIT(shmmap_entry_pool, sizeof(struct shmmap_entry), 0, 0, 0,
    "shmmp", &pool_allocator_nointr);

struct shmmap_state {
	unsigned int nitems;
	unsigned int nrefs;
	SLIST_HEAD(, shmmap_entry) entries;
};

static int shm_find_segment_by_key(key_t);
static void shm_deallocate_segment(struct shmid_ds *);
static void shm_delete_mapping(struct vmspace *, struct shmmap_state *,
			       struct shmmap_entry *);
static int shmget_existing(struct proc *, struct sys_shmget_args *,
			   int, int, register_t *);
static int shmget_allocate_segment(struct proc *, struct sys_shmget_args *,
				   int, register_t *);
static struct shmmap_state *shmmap_getprivate(struct proc *);
static struct shmmap_entry *shm_find_mapping(struct shmmap_state *, vaddr_t);

static int
shm_find_segment_by_key(key_t key)
{
	int i;

	for (i = 0; i < shminfo.shmmni; i++)
		if ((shmsegs[i].shm_perm.mode & SHMSEG_ALLOCATED) &&
		    shmsegs[i].shm_perm._key == key)
			return i;
	return -1;
}

static struct shmid_ds *
shm_find_segment_by_shmid(int shmid)
{
	int segnum;
	struct shmid_ds *shmseg;

	segnum = IPCID_TO_IX(shmid);
	if (segnum < 0 || segnum >= shminfo.shmmni)
		return NULL;
	shmseg = &shmsegs[segnum];
	if ((shmseg->shm_perm.mode & SHMSEG_ALLOCATED) == 0)
		return NULL;
	if ((shmseg->shm_perm.mode & (SHMSEG_REMOVED|SHMSEG_RMLINGER)) == SHMSEG_REMOVED)
		return NULL;
	if (shmseg->shm_perm._seq != IPCID_TO_SEQ(shmid))
		return NULL;
	return shmseg;
}

static void
shm_deallocate_segment(struct shmid_ds *shmseg)
{
	struct uvm_object *uobj = shmseg->_shm_internal;
	size_t size = (shmseg->shm_segsz + PGOFSET) & ~PGOFSET;

#ifdef SHMDEBUG
	printf("shm freeing key 0x%lx seq 0x%x\n",
	       shmseg->shm_perm._key, shmseg->shm_perm._seq);
#endif

	(*uobj->pgops->pgo_detach)(uobj);
	shmseg->_shm_internal = NULL;
	shm_committed -= btoc(size);
	shmseg->shm_perm.mode = SHMSEG_FREE;
	shm_nused--;
}

static void
shm_delete_mapping(struct vmspace *vm, struct shmmap_state *shmmap_s,
    struct shmmap_entry *shmmap_se)
{
	struct shmid_ds *shmseg;
	int segnum;
	size_t size;

	segnum = IPCID_TO_IX(shmmap_se->shmid);
#ifdef DEBUG
	if (segnum < 0 || segnum >= shminfo.shmmni)
		panic("shm_delete_mapping: vmspace %p state %p entry %p - "
		    "entry segment ID bad (%d)",
		    vm, shmmap_s, shmmap_se, segnum);
#endif
	shmseg = &shmsegs[segnum];
	size = (shmseg->shm_segsz + PGOFSET) & ~PGOFSET;
	uvm_deallocate(&vm->vm_map, shmmap_se->va, size);
	SLIST_REMOVE(&shmmap_s->entries, shmmap_se, shmmap_entry, next);
	shmmap_s->nitems--;
	pool_put(&shmmap_entry_pool, shmmap_se);
	shmseg->shm_dtime = time.tv_sec;
	if ((--shmseg->shm_nattch <= 0) &&
	    (shmseg->shm_perm.mode & SHMSEG_REMOVED)) {
		shm_deallocate_segment(shmseg);
		shm_last_free = segnum;
	}
}

/*
 * Get a non-shared shm map for that vmspace.
 * 3 cases:
 *   - no shm map present: create a fresh one
 *   - a shm map with refcount=1, just used by ourselves: fine
 *   - a shared shm map: copy to a fresh one and adjust refcounts
 */
static struct shmmap_state *
shmmap_getprivate(struct proc *p)
{
	struct shmmap_state *oshmmap_s, *shmmap_s;
	struct shmmap_entry *oshmmap_se, *shmmap_se;

	oshmmap_s = (struct shmmap_state *)p->p_vmspace->vm_shm;
	if (oshmmap_s && oshmmap_s->nrefs == 1)
		return (oshmmap_s);

	shmmap_s = malloc(sizeof(struct shmmap_state), M_SHM, M_WAITOK);
	memset(shmmap_s, 0, sizeof(struct shmmap_state));
	shmmap_s->nrefs = 1;
	SLIST_INIT(&shmmap_s->entries);
	p->p_vmspace->vm_shm = (caddr_t)shmmap_s;

	if (!oshmmap_s)
		return (shmmap_s);

#ifdef SHMDEBUG
	printf("shmmap_getprivate: vm %p split (%d entries), was used by %d\n",
	       p->p_vmspace, oshmmap_s->nitems, oshmmap_s->nrefs);
#endif
	SLIST_FOREACH(oshmmap_se, &oshmmap_s->entries, next) {
		shmmap_se = pool_get(&shmmap_entry_pool, PR_WAITOK);
		shmmap_se->va = oshmmap_se->va;
		shmmap_se->shmid = oshmmap_se->shmid;
		SLIST_INSERT_HEAD(&shmmap_s->entries, shmmap_se, next);
	}
	shmmap_s->nitems = oshmmap_s->nitems;
	oshmmap_s->nrefs--;
	return (shmmap_s);
}

static struct shmmap_entry *
shm_find_mapping(struct shmmap_state *map, vaddr_t va)
{
	struct shmmap_entry *shmmap_se;

	SLIST_FOREACH(shmmap_se, &map->entries, next) {
		if (shmmap_se->va == va)
			return shmmap_se;
	}
	return 0;
}

int
sys_shmdt(struct lwp *l, void *v, register_t *retval)
{
	struct sys_shmdt_args /* {
		syscallarg(const void *) shmaddr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct shmmap_state *shmmap_s, *shmmap_s1;
	struct shmmap_entry *shmmap_se;

	shmmap_s = (struct shmmap_state *)p->p_vmspace->vm_shm;
	if (shmmap_s == NULL)
		return EINVAL;

	shmmap_se = shm_find_mapping(shmmap_s, (vaddr_t)SCARG(uap, shmaddr));
	if (!shmmap_se)
		return EINVAL;

	shmmap_s1 = shmmap_getprivate(p);
	if (shmmap_s1 != shmmap_s) {
		/* map has been copied, lookup entry in new map */
		shmmap_se = shm_find_mapping(shmmap_s1,
					     (vaddr_t)SCARG(uap, shmaddr));
		KASSERT(shmmap_se != NULL);
	}
#ifdef SHMDEBUG
	printf("shmdt: vm %p: remove %d @%lx\n",
	       p->p_vmspace, shmmap_se->shmid, shmmap_se->va);
#endif
	shm_delete_mapping(p->p_vmspace, shmmap_s1, shmmap_se);
	return 0;
}

int
sys_shmat(struct lwp *l, void *v, register_t *retval)
{
	struct sys_shmat_args /* {
		syscallarg(int) shmid;
		syscallarg(const void *) shmaddr;
		syscallarg(int) shmflg;
	} */ *uap = v;
	int error, flags;
	struct proc *p = l->l_proc;
	kauth_cred_t cred = p->p_cred;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s;
	struct uvm_object *uobj;
	vaddr_t attach_va;
	vm_prot_t prot;
	vsize_t size;
	struct shmmap_entry *shmmap_se;

	shmseg = shm_find_segment_by_shmid(SCARG(uap, shmid));
	if (shmseg == NULL)
		return EINVAL;
	error = ipcperm(cred, &shmseg->shm_perm,
		    (SCARG(uap, shmflg) & SHM_RDONLY) ? IPC_R : IPC_R|IPC_W);
	if (error)
		return error;

	shmmap_s = (struct shmmap_state *)p->p_vmspace->vm_shm;
	if (shmmap_s && shmmap_s->nitems >= shminfo.shmseg)
		return EMFILE;

	size = (shmseg->shm_segsz + PGOFSET) & ~PGOFSET;
	prot = VM_PROT_READ;
	if ((SCARG(uap, shmflg) & SHM_RDONLY) == 0)
		prot |= VM_PROT_WRITE;
	flags = MAP_ANON | MAP_SHARED;
	if (SCARG(uap, shmaddr)) {
		flags |= MAP_FIXED;
		if (SCARG(uap, shmflg) & SHM_RND)
			attach_va =
			    (vaddr_t)SCARG(uap, shmaddr) & ~(SHMLBA-1);
		else if (((vaddr_t)SCARG(uap, shmaddr) & (SHMLBA-1)) == 0)
			attach_va = (vaddr_t)SCARG(uap, shmaddr);
		else
			return EINVAL;
	} else {
		/* This is just a hint to uvm_mmap() about where to put it. */
		attach_va = p->p_emul->e_vm_default_addr(p,
		    (vaddr_t)p->p_vmspace->vm_daddr, size);
	}
	uobj = shmseg->_shm_internal;
	(*uobj->pgops->pgo_reference)(uobj);
	error = uvm_map(&p->p_vmspace->vm_map, &attach_va, size,
	    uobj, 0, 0,
	    UVM_MAPFLAG(prot, prot, UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
	if (error) {
		(*uobj->pgops->pgo_detach)(uobj);
		return error;
	}
	shmmap_se = pool_get(&shmmap_entry_pool, PR_WAITOK);
	shmmap_se->va = attach_va;
	shmmap_se->shmid = SCARG(uap, shmid);
	shmmap_s = shmmap_getprivate(p);
#ifdef SHMDEBUG
	printf("shmat: vm %p: add %d @%lx\n", p->p_vmspace, shmmap_se->shmid, attach_va);
#endif
	SLIST_INSERT_HEAD(&shmmap_s->entries, shmmap_se, next);
	shmmap_s->nitems++;
	shmseg->shm_lpid = p->p_pid;
	shmseg->shm_atime = time.tv_sec;
	shmseg->shm_nattch++;

	retval[0] = attach_va;
	return 0;
}

int
sys___shmctl13(struct lwp *l, void *v, register_t *retval)
{
	struct sys___shmctl13_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct shmid_ds *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct shmid_ds shmbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &shmbuf, sizeof(shmbuf));
		if (error)
			return (error);
	}

	error = shmctl1(p, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

	if (error == 0 && cmd == IPC_STAT)
		error = copyout(&shmbuf, SCARG(uap, buf), sizeof(shmbuf));

	return (error);
}

int
shmctl1(struct proc *p, int shmid, int cmd, struct shmid_ds *shmbuf)
{
	kauth_cred_t cred = p->p_cred;
	struct shmid_ds *shmseg;
	int error = 0;

	shmseg = shm_find_segment_by_shmid(shmid);
	if (shmseg == NULL)
		return EINVAL;
	switch (cmd) {
	case IPC_STAT:
		if ((error = ipcperm(cred, &shmseg->shm_perm, IPC_R)) != 0)
			return error;
		memcpy(shmbuf, shmseg, sizeof(struct shmid_ds));
		break;
	case IPC_SET:
		if ((error = ipcperm(cred, &shmseg->shm_perm, IPC_M)) != 0)
			return error;
		shmseg->shm_perm.uid = shmbuf->shm_perm.uid;
		shmseg->shm_perm.gid = shmbuf->shm_perm.gid;
		shmseg->shm_perm.mode =
		    (shmseg->shm_perm.mode & ~ACCESSPERMS) |
		    (shmbuf->shm_perm.mode & ACCESSPERMS);
		shmseg->shm_ctime = time.tv_sec;
		break;
	case IPC_RMID:
		if ((error = ipcperm(cred, &shmseg->shm_perm, IPC_M)) != 0)
			return error;
		shmseg->shm_perm._key = IPC_PRIVATE;
		shmseg->shm_perm.mode |= SHMSEG_REMOVED;
		if (shmseg->shm_nattch <= 0) {
			shm_deallocate_segment(shmseg);
			shm_last_free = IPCID_TO_IX(shmid);
		}
		break;
	case SHM_LOCK:
	case SHM_UNLOCK:
	default:
		return EINVAL;
	}
	return 0;
}

static int
shmget_existing(struct proc *p, struct sys_shmget_args *uap, int mode,
    int segnum, register_t *retval)
{
	struct shmid_ds *shmseg;
	kauth_cred_t cred = p->p_cred;
	int error;

	shmseg = &shmsegs[segnum];
	if (shmseg->shm_perm.mode & SHMSEG_REMOVED) {
		/*
		 * This segment is in the process of being allocated.  Wait
		 * until it's done, and look the key up again (in case the
		 * allocation failed or it was freed).
		 */
		shmseg->shm_perm.mode |= SHMSEG_WANTED;
		error = tsleep((caddr_t)shmseg, PLOCK | PCATCH, "shmget", 0);
		if (error)
			return error;
		return EAGAIN;
	}
	if ((error = ipcperm(cred, &shmseg->shm_perm, mode)) != 0)
		return error;
	if (SCARG(uap, size) && SCARG(uap, size) > shmseg->shm_segsz)
		return EINVAL;
	if ((SCARG(uap, shmflg) & (IPC_CREAT | IPC_EXCL)) ==
	    (IPC_CREAT | IPC_EXCL))
		return EEXIST;
	*retval = IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);
	return 0;
}

static int
shmget_allocate_segment(struct proc *p, struct sys_shmget_args *uap, int mode,
    register_t *retval)
{
	int i, segnum, shmid, size;
	kauth_cred_t cred = p->p_cred;
	struct shmid_ds *shmseg;
	int error = 0;

	if (SCARG(uap, size) < shminfo.shmmin ||
	    SCARG(uap, size) > shminfo.shmmax)
		return EINVAL;
	if (shm_nused >= shminfo.shmmni) /* any shmids left? */
		return ENOSPC;
	size = (SCARG(uap, size) + PGOFSET) & ~PGOFSET;
	if (shm_committed + btoc(size) > shminfo.shmall)
		return ENOMEM;
	if (shm_last_free < 0) {
		for (i = 0; i < shminfo.shmmni; i++)
			if (shmsegs[i].shm_perm.mode & SHMSEG_FREE)
				break;
		if (i == shminfo.shmmni)
			panic("shmseg free count inconsistent");
		segnum = i;
	} else  {
		segnum = shm_last_free;
		shm_last_free = -1;
	}
	shmseg = &shmsegs[segnum];
	/*
	 * In case we sleep in malloc(), mark the segment present but deleted
	 * so that noone else tries to create the same key.
	 */
	shmseg->shm_perm.mode = SHMSEG_ALLOCATED | SHMSEG_REMOVED;
	shmseg->shm_perm._key = SCARG(uap, key);
	shmseg->shm_perm._seq = (shmseg->shm_perm._seq + 1) & 0x7fff;
	shmid = IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);

	shmseg->_shm_internal = uao_create(size, 0);

	shmseg->shm_perm.cuid = shmseg->shm_perm.uid = kauth_cred_geteuid(cred);
	shmseg->shm_perm.cgid = shmseg->shm_perm.gid = kauth_cred_getegid(cred);
	shmseg->shm_perm.mode = (shmseg->shm_perm.mode & SHMSEG_WANTED) |
	    (mode & (ACCESSPERMS|SHMSEG_RMLINGER)) | SHMSEG_ALLOCATED;
	shmseg->shm_segsz = SCARG(uap, size);
	shmseg->shm_cpid = p->p_pid;
	shmseg->shm_lpid = shmseg->shm_nattch = 0;
	shmseg->shm_atime = shmseg->shm_dtime = 0;
	shmseg->shm_ctime = time.tv_sec;
	shm_committed += btoc(size);
	shm_nused++;

	*retval = shmid;
	if (shmseg->shm_perm.mode & SHMSEG_WANTED) {
		/*
		 * Somebody else wanted this key while we were asleep.  Wake
		 * them up now.
		 */
		shmseg->shm_perm.mode &= ~SHMSEG_WANTED;
		wakeup((caddr_t)shmseg);
	}
	return error;
}

int
sys_shmget(struct lwp *l, void *v, register_t *retval)
{
	struct sys_shmget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) size;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int segnum, mode, error;

	mode = SCARG(uap, shmflg) & ACCESSPERMS;
	if (SCARG(uap, shmflg) & _SHM_RMLINGER)
		mode |= SHMSEG_RMLINGER;

#ifdef SHMDEBUG
	printf("shmget: key 0x%lx size 0x%x shmflg 0x%x mode 0x%x\n",
        	SCARG(uap, key), SCARG(uap, size), SCARG(uap, shmflg), mode);
#endif

	if (SCARG(uap, key) != IPC_PRIVATE) {
again:
		segnum = shm_find_segment_by_key(SCARG(uap, key));
		if (segnum >= 0) {
			error = shmget_existing(p, uap, mode, segnum, retval);
			if (error == EAGAIN)
				goto again;
			return error;
		}
		if ((SCARG(uap, shmflg) & IPC_CREAT) == 0)
			return ENOENT;
	}
	return shmget_allocate_segment(p, uap, mode, retval);
}

void
shmfork(struct vmspace *vm1, struct vmspace *vm2)
{
	struct shmmap_state *shmmap_s;
	struct shmmap_entry *shmmap_se;

	vm2->vm_shm = vm1->vm_shm;

	if (vm1->vm_shm == NULL)
		return;

#ifdef SHMDEBUG
	printf("shmfork %p->%p\n", vm1, vm2);
#endif

	shmmap_s = (struct shmmap_state *)vm1->vm_shm;

	SLIST_FOREACH(shmmap_se, &shmmap_s->entries, next)
		shmsegs[IPCID_TO_IX(shmmap_se->shmid)].shm_nattch++;
	shmmap_s->nrefs++;
}

void
shmexit(struct vmspace *vm)
{
	struct shmmap_state *shmmap_s;
	struct shmmap_entry *shmmap_se;

	shmmap_s = (struct shmmap_state *)vm->vm_shm;
	if (shmmap_s == NULL)
		return;

	vm->vm_shm = NULL;

	if (--shmmap_s->nrefs > 0) {
#ifdef SHMDEBUG
		printf("shmexit: vm %p drop ref (%d entries), now used by %d\n",
		       vm, shmmap_s->nitems, shmmap_s->nrefs);
#endif
		SLIST_FOREACH(shmmap_se, &shmmap_s->entries, next)
			shmsegs[IPCID_TO_IX(shmmap_se->shmid)].shm_nattch--;
		return;
	}

#ifdef SHMDEBUG
	printf("shmexit: vm %p cleanup (%d entries)\n", vm, shmmap_s->nitems);
#endif
	while (!SLIST_EMPTY(&shmmap_s->entries)) {
		shmmap_se = SLIST_FIRST(&shmmap_s->entries);
		shm_delete_mapping(vm, shmmap_s, shmmap_se);
	}
	KASSERT(shmmap_s->nitems == 0);
	free(shmmap_s, M_SHM);
}

void
shminit(void)
{
	int i, sz;
	vaddr_t v;

	/* Allocate pageable memory for our structures */
	sz = shminfo.shmmni * sizeof(struct shmid_ds);
	v = uvm_km_alloc(kernel_map, round_page(sz), 0, UVM_KMF_WIRED);
	if (v == 0)
		panic("sysv_shm: cannot allocate memory");
	shmsegs = (void *)v;

	shminfo.shmmax *= PAGE_SIZE;

	for (i = 0; i < shminfo.shmmni; i++) {
		shmsegs[i].shm_perm.mode = SHMSEG_FREE;
		shmsegs[i].shm_perm._seq = 0;
	}
	shm_last_free = 0;
	shm_nused = 0;
	shm_committed = 0;
}
