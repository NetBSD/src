/*
 * Copyright (c) 1994 Adam Glass
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Adam Glass BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * based on ..., ..., and ...
 * $Header: /cvsroot/src/sys/kern/sysv_shm.c,v 1.11 1994/05/25 02:14:27 hpeyerl Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_map.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>

/*
 * Provides the following externally accessible functions:
 *
 * shminit(void);		           initialization
 * shmexit(struct proc *)                  cleanup
 * shmfork(struct proc *, struct proc *, int) fork handling
 * shmsys(arg1, arg2, arg3, arg4);         shm{at,ctl,dt,get}(arg2, arg3, arg4)
 *
 * Structures:
 * shmsegs (an array of 'struct shmid_ds')
 * per proc array of 'struct shmmap_state'
 */
 
#define SHMSEG_FREE      0x200
#define SHMSEG_REMOVED   0x400
#define SHMSEG_ALLOCATED 0x800
#define SHMSEG_INVALID (SHMSEG_FREE | SHMSEG_REMOVED)
#define SHMSEG_PERM_MASK 0x1FF

vm_map_t sysvshm_map;
int shm_last_free, shm_nused, shm_committed;

struct shmat_args {
	u_int shm_syscall;
	int shmid;
	void *shmaddr;
	int shmflg;
};

struct shmctl_args {
	u_int shm_syscall;
	int shmid;
	int cmd;
	struct shmat_ds *ubuf;
};

struct shmdt_args {
	u_int shm_syscall;
	int shmid;
	void *shmaddr;
	int shmflg;
};

struct shmget_args {
	u_int shm_syscall;
	key_t key;
	size_t size;
	int shmflg;
};

struct shmsys_args {
	u_int shm_syscall;
};

struct shm_handle {
	vm_offset_t kva;
};

struct shmmap_state {
	vm_offset_t va;
	int shmid;
};

/* external intefaces */
int shmsys __P((struct proc *, struct shmsys_args *, int *));
void shminit __P((void));

/* internal functions */
static int shmseg_alloc __P((struct proc *, key_t, size_t, int, int *));
static int shmseg_find_key __P((key_t));
static struct shmid_ds *shmseg_find_shmid __P((int, int *));
static int shmget_existing __P((struct proc *, struct shmget_args *, int,
				int, int *));
static int shmget __P((struct proc *, struct shmget_args *, int *));
static int shmctl __P((struct proc *, struct shmctl_args *, int *));

static int shmseg_alloc(p, key, size, mode, retval)
	struct proc *p;
	key_t key;
	size_t size;
	int mode;
	int *retval;
{
	int i, segnum, result, shmid;
	struct ucred *ucred = p->p_ucred;
	struct shmid_ds *shmseg;
	struct shm_handle *shm_handle;
	
	if (shm_nused >= shminfo.shmmni) /* any shmids left? */
		return ENOSPC;
	if (shm_last_free < 0) {
		for (i=0; i < shminfo.shmmni; i++)
			if (shmsegs[i].shm_perm.mode & SHMSEG_FREE) break;
		if (i == shminfo.shmmni)
			panic("shmseg free count inconsistent");
		segnum = i;
	}
	else 
		segnum = shm_last_free;
	shmseg = &shmsegs[segnum];
	shmseg->shm_perm.mode = SHMSEG_ALLOCATED;
	shm_handle = (struct shm_handle *)
		malloc(sizeof(struct shm_handle), M_SHM, M_WAITOK);
	shmid = IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);
	result = vm_mmap(sysvshm_map, &shm_handle->kva,
			 ctob(clrnd(btoc(size))), VM_PROT_ALL, VM_PROT_DEFAULT,
			 MAP_ANON, shmid, 0);
	if (result != KERN_SUCCESS) {
		shmseg->shm_perm.mode = SHMSEG_FREE;
		free((caddr_t) shm_handle, M_SHM);
		return ENOMEM;
	}
	shmseg->shm_internal = shm_handle;
	shmseg->shm_perm.cuid = shmseg->shm_perm.uid =
		ucred->cr_uid;
	shmseg->shm_perm.cgid = shmseg->shm_perm.gid =
		ucred->cr_gid;
	shmseg->shm_perm.mode = mode;
	shmseg->shm_perm.seq = shmseg->shm_perm.seq + 1;
	shmseg->shm_perm.key = key;
	shmseg->shm_segsz = size;
	shmseg->shm_cpid = p->p_pid;
	shmseg->shm_lpid = shmseg->shm_nattch =
		shmseg->shm_atime = shmseg->shm_dtime = 0;
	shmseg->shm_ctime = time.tv_sec;
	shmseg->shm_internal = NULL; /* we aren't using this field */
	shm_committed += clrnd(btoc(size));
	shm_nused++;
	shm_last_free = -1;
	*retval = shmid;
	return 0;
}

static int shmseg_find_key(key)
	key_t key;
{
	int i;

	for (i=0; i < shminfo.shmmni; i++) {
		if (shmsegs[i].shm_perm.mode & SHMSEG_INVALID) continue;
		if (shmsegs[i].shm_perm.key != key) continue;
		return i;
	}	
	return -1;
}

static struct shmid_ds *shmseg_find_shmid(shmid, where)
	int shmid;
	int *where;
{
	int segnum;
	struct shmid_ds *shmseg;

	segnum = IPCID_TO_IX(shmid);
	if (segnum >= shminfo.shmmni)
		return NULL;
	shmseg = &shmsegs[segnum];
	if ((shmseg->shm_perm.mode & (SHMSEG_FREE|SHMSEG_ALLOCATED)) ||
	    (shmseg->shm_perm.seq != IPCID_TO_SEQ(shmid)))
		return NULL;
	if (where)
		*where = segnum;
	return shmseg;
}

static vm_offset_t shm_find_space(p, size)
	struct proc *p;
	size_t size;
{
	vm_offset_t low_end, range, current;
	int result;

	low_end = (vm_offset_t) p->p_vmspace->vm_daddr +
		NBPG * p->p_vmspace->vm_dsize;
	range = (USRSTACK - low_end);

	/* current = range *3/4 + low_end  */
	current = ((range&1)<<1 + range)>>2 + range>>1 + low_end;
#if 0
	result = vm_map_find(&p->p_vmspace->vm_map, NULL, 0, &current, size,
			     TRUE);
	if (result)
		return NULL;
#endif
	return current;
}

static int shmdt_seg(p, shmseg, shmmap_s)
	struct proc *p;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s;
{
	int result;
	size_t size;
	
	size = ctob(clrnd(btoc(shmseg->shm_segsz)));
	result = vm_deallocate(&p->p_vmspace->vm_map, shmmap_s->va, size);
	if (result != KERN_SUCCESS)
		return EINVAL;
	shmseg->shm_nattch--;
	shmseg->shm_dtime = time.tv_sec;
	if ((shmseg->shm_nattch == 0) &&
	    (shmseg->shm_perm.mode & SHMSEG_REMOVED)) {
		shmseg->shm_perm.mode = SHMSEG_FREE;
		shm_last_free = IPCID_TO_IX(shmmap_s->shmid);
	}
	shmmap_s->va = 0;
	return 0;
}

static int shmdt(p, uap, retval)
	struct proc *p;
	struct shmdt_args *uap;
	int *retval;
{
	int i;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s;

	if (p->p_vmspace->vm_shm == NULL)
		return EINVAL;
	shmseg = shmseg_find_shmid(uap->shmid, NULL);
	if (shmseg == NULL)
		return EINVAL;
	shmmap_s = (struct shmmap_state *) p->p_vmspace->vm_shm;
	for (i = 0; i < shminfo.shmseg; i++, shmmap_s++) {
		if ((shmmap_s->va != (vm_offset_t) uap->shmaddr) ||
		    (shmmap_s->shmid != uap->shmid)) continue;
		break;
	}
	if (i == shminfo.shmseg)
		return EINVAL;
	return shmdt_seg(p, shmseg, shmmap_s);
}

static int shmat(p, uap, retval)
	struct proc *p;
	struct shmat_args *uap;
	int *retval;
{
	int error, segnum, i, ipc_prot;
	struct ucred *ucred = p->p_ucred;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s = NULL;
	vm_offset_t attach_va;
	vm_prot_t prot;
	vm_size_t size;

 start:
	shmseg = shmseg_find_shmid(uap->shmid, NULL);
	if (shmseg == NULL)
		return EINVAL;
	if (error = ipcperm(ucred, &shmseg->shm_perm,
			      ((uap->shmflg & SHM_RDONLY) ? IPC_R :
			       (IPC_R|IPC_W))))	return error;
	prot = VM_PROT_READ | ((uap->shmflg & SHM_RDONLY) ? 0 : VM_PROT_WRITE);
	if (p->p_vmspace->vm_shm == NULL) {
		shmmap_s = malloc(sizeof(struct shmmap_state)*shminfo.shmseg,
				  M_SHM, M_WAITOK);
		bzero((caddr_t) shmmap_s, sizeof(struct shmmap_state) *
		      shminfo.shmseg);
		p->p_vmspace->vm_shm = (caddr_t) shmmap_s;
		goto start;	/* things may have changed if we slept */
	}
	if (shmmap_s == NULL) {
		shmmap_s = (struct shmmap_state *) p->p_vmspace->vm_shm;
		for (i = 0; i < shminfo.shmseg; i++) {
			if (shmmap_s->va == 0) break;
			shmmap_s++;
		}
		if (i == shminfo.shmseg)
			return EMFILE;
	}
	if (uap->shmaddr) {
		if (uap->shmflg & SHM_RND) 
			attach_va = (vm_offset_t) uap->shmaddr -
				((vm_offset_t)uap->shmaddr % SHMLBA);
		else
			attach_va = (vm_offset_t) uap->shmaddr;
	}
	else attach_va = shm_find_space(p, shmseg->shm_segsz);
	if (attach_va == NULL)
		return ENOMEM;
	size = ctob(clrnd(btoc(shmseg->shm_segsz)));
	error = vm_mmap(&p->p_vmspace->vm_map, &attach_va, size,
			prot, VM_PROT_DEFAULT,
			MAP_ANON|MAP_SHARED|(uap->shmaddr ? MAP_FIXED : 0),
			uap->shmid, 0);
	if (error)
		return error;
	shmmap_s->shmid = uap->shmid;
	shmseg->shm_lpid = p->p_pid;
	shmseg->shm_atime = time.tv_sec;
	shmseg->shm_nattch++;
	*retval = shmmap_s->va = attach_va;
	return 0;
}

static int shmctl(p, uap, retval)
	struct proc *p;
	struct shmctl_args *uap;
	int *retval;
{
	int error, segnum;
	struct ucred *ucred = p->p_ucred;
	struct shmid_ds inbuf;
	struct shmid_ds *shmseg;

	shmseg = shmseg_find_shmid(uap->shmid, &segnum);
	if (shmseg == NULL)
		return EINVAL;
	switch (uap->cmd) {
	case IPC_STAT:
		if (error = ipcperm(ucred, &shmseg->shm_perm, IPC_R))
			return error;
		if (error = copyout((caddr_t) shmseg,
				    uap->ubuf, sizeof(inbuf)))
			return error;
		break;
	case IPC_SET:
		if (ucred->cr_uid && (ucred->cr_uid != shmseg->shm_perm.uid) &&
		    (ucred->cr_uid != shmseg->shm_perm.cuid))
			return EPERM;
		if (error = copyin((caddr_t) uap->ubuf, (caddr_t)&inbuf,
				   sizeof(inbuf)))
			return error;
		shmseg->shm_perm.uid = inbuf.shm_perm.uid;
		shmseg->shm_perm.gid = inbuf.shm_perm.gid;
		shmseg->shm_perm.mode = inbuf.shm_perm.mode & SHMSEG_PERM_MASK;
		break;
	case IPC_RMID:
		if (ucred->cr_uid && (ucred->cr_uid != shmseg->shm_perm.uid) &&
		    (ucred->cr_uid != shmseg->shm_perm.cuid))
			return EPERM;
		if (shmseg->shm_nattch) 
			shmseg->shm_perm.mode |= SHMSEG_REMOVED;
		else {
			struct shm_handle *shm_handle = shmseg->shm_internal;

			(void) vm_deallocate(sysvshm_map,
				     shm_handle->kva,
				     ctob(clrnd(btoc(shmseg->shm_segsz))));
			free((caddr_t) shm_handle, M_SHM);
			shmseg->shm_internal = NULL;
			shm_committed -= clrnd(btoc(shmseg->shm_segsz));
			free((caddr_t) shmseg->shm_internal, M_SHM);
			shmseg->shm_perm.mode = SHMSEG_FREE;
			shm_last_free = segnum;
		}
		break;
#if 0
	case SHM_LOCK:
	case SHM_UNLOCK:
#endif
	default:
		return EINVAL;
	}
	shmseg->shm_ctime = time.tv_sec;
	return 0;
}

static int shmget_existing(p, uap, mode, segnum, retval)
	struct proc *p;
	struct shmget_args *uap;
	int mode;
	int segnum;
	int *retval;
{
	int error;
	struct ucred *ucred = p->p_ucred;
	struct shmid_ds *shmseg;

	shmseg = &shmsegs[segnum];
	if ((uap->shmflg & IPC_CREAT) || (uap->shmflg & IPC_EXCL)) /* XXX */
		return EEXIST;
	if (error = ipcperm(ucred, shmseg->shm_perm, mode))
		return error;
	if (uap->size && (uap->size > shmseg->shm_segsz))
		return EINVAL;
	*retval = IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);
	return 0;
}

static int shmget(p, uap, retval)
	struct proc *p;
	struct shmget_args *uap;
	int *retval;
{
	int segnum, mode, error;
	size_t size;
	struct shmid_ds *shmseg;

	if ((uap->size < shminfo.shmmin) || (uap->size > shminfo.shmmax))
		return EINVAL;
	mode = uap->shmflg & SHMSEG_PERM_MASK;
	if (uap->key != IPC_PRIVATE) {
		segnum = shmseg_find_key(uap->key);
		if (segnum >=0)
			return shmget_existing(p, uap, mode, segnum, retval);
	}
	if ((uap->shmflg & IPC_CREAT) == 0) 
		return ENOENT;
	size = clrnd(btoc(uap->size));
	if (size + shm_committed > shminfo.shmall) 
		return ENOMEM;
	return shmseg_alloc(p, uap->key, uap->size, mode, retval);
}

int shmsys(p, uap, retval)
	struct proc *p;
	struct shmsys_args *uap;
	int *retval;
{
	int result;

	/*
	 * pass whole uap to avoid structure alignment problems
	 */

	/* sub-syscall # not exported in any header file */
	switch(uap->shm_syscall) {
	case 0:
		result = shmat(p, (struct shmat_args *) uap, retval);
		break;
	case 1:
		result = shmctl(p, (struct shmctl_args *) uap, retval);
		break;
	case 2:
		result = shmdt(p, (struct shmdt_args *) uap, retval);
		break;
	case 3:
		result = shmget(p, (struct shmget_args *) uap, retval);
		break;
	default:
		result = EINVAL;
		break;
	}
	return result;
}

void shmfork(p1, p2, isvfork)
    struct proc *p1, *p2;
    int isvfork;
{
	struct shmmap_state *shmmap_s;
	int i;

	shmmap_s = malloc(sizeof(struct shmmap_state) * shminfo.shmseg,
			  M_SHM, M_WAITOK);
	p2->p_vmspace->vm_shm = (caddr_t) shmmap_s;
	bcopy((caddr_t) p1->p_vmspace->vm_shm, (caddr_t) shmmap_s,
	      sizeof(struct shmmap_state) * shminfo.shmseg);
	for (i=0; i < shminfo.shmseg; i++, shmmap_s++)
		if (shmmap_s->va != NULL)
			shmsegs[IPCID_TO_IX(shmmap_s->shmid)].shm_nattch++;
}

void shmexit(p)
	struct proc *p;
{
	int i;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s;

	shmmap_s = (struct shmmap_state *)p->p_vmspace->vm_shm;
	for (i=0; i < shminfo.shmseg; i++, shmmap_s++) {
		if (shmmap_s->va != 0) continue;
		shmseg = shmseg_find_shmid(shmmap_s->shmid, NULL);
		(void) shmdt_seg(p, shmseg, shmmap_s);
	}
	free(p->p_vmspace->vm_shm, M_SHM);
	p->p_vmspace->vm_shm = NULL;
}

void shminit()
{
	int i;
	vm_offset_t garbage1, garbage2;

	/* actually this *should* be pageable.  SHM_{LOCK,UNLOCK} */
	sysvshm_map = kmem_suballoc(kernel_map, &garbage1, &garbage2,
				    shminfo.shmall * NBPG, FALSE);
	for (i = 0; i < shminfo.shmmni; i++) {
		shmsegs[i].shm_perm.mode = SHMSEG_FREE;
		shmsegs[i].shm_perm.seq = 0;
	}
	shm_last_free = 0;
	shm_nused = 0;
	shm_committed = 0;
}
