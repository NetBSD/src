/*	$NetBSD: procfs_map.c,v 1.28.2.1 2007/02/27 16:54:37 yamt Exp $	*/

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)procfs_status.c	8.3 (Berkeley) 2/17/94
 *
 *	$FreeBSD: procfs_map.c,v 1.18 1998/12/04 22:54:51 archie Exp $
 */

/*
 * Copyright (c) 1993 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)procfs_status.c	8.3 (Berkeley) 2/17/94
 *
 *	$FreeBSD: procfs_map.c,v 1.18 1998/12/04 22:54:51 archie Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_map.c,v 1.28.2.1 2007/02/27 16:54:37 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <miscfs/procfs/procfs.h>

#include <sys/lock.h>

#include <uvm/uvm.h>

#define MEBUFFERSIZE 256

extern int getcwd_common(struct vnode *, struct vnode *,
			      char **, char *, int, int, struct lwp *);

static int procfs_vnode_to_path(struct vnode *vp, char *path, int len,
				struct lwp *curl, struct proc *p);

/*
 * The map entries can *almost* be read with programs like cat.  However,
 * large maps need special programs to read.  It is not easy to implement
 * a program that can sense the required size of the buffer, and then
 * subsequently do a read with the appropriate size.  This operation cannot
 * be atomic.  The best that we can do is to allow the program to do a read
 * with an arbitrarily large buffer, and return as much as we can.  We can
 * return an error code if the buffer is too small (EFBIG), then the program
 * can try a bigger buffer.
 */
int
procfs_domap(struct lwp *curl, struct proc *p, struct pfsnode *pfs,
	     struct uio *uio, int linuxmode)
{
	size_t len;
	int error, retries;
	struct vmspace *vm;
	struct vm_map *map;
	struct vm_map_entry *entry;
	char mebuffer[MEBUFFERSIZE];
	char *path;
	struct vnode *vp;
	struct vattr va;
	dev_t dev;
	long fileid;
	unsigned timestamp;
	struct uio savuio;

	if (uio->uio_rw != UIO_READ)
		return (EOPNOTSUPP);

	if (uio->uio_offset != 0)
		return (0);

	error = 0;
	path = NULL;

	if (linuxmode != 0) {
		path = (char *)malloc(MAXPATHLEN * 4, M_TEMP, M_WAITOK);
		if (path == NULL)
			return ENOMEM;
	}

	if ((error = proc_vmspace_getref(p, &vm)) != 0) {
		if (path != NULL)
			free(path, M_TEMP);
		return (error);
	}

	map = &vm->vm_map;
	memcpy(&savuio, uio, sizeof(savuio));
	retries = 0;
	vm_map_lock_read(map);

 restart:
	for (entry = map->header.next;
		((uio->uio_resid > 0) && (entry != &map->header));
		entry = entry->next) {

		if (retries > 250) {
			error = EWOULDBLOCK;
			break;
		}

		if (UVM_ET_ISSUBMAP(entry))
			continue;

		if (linuxmode != 0) {
			*path = 0;
			dev = (dev_t)0;
			fileid = 0;
			if (UVM_ET_ISOBJ(entry) &&
			    UVM_OBJ_IS_VNODE(entry->object.uvm_obj)) {
				vp = (struct vnode *)entry->object.uvm_obj;
				error = VOP_GETATTR(vp, &va, curl->l_cred,
				    curl);
				if (error == 0 && vp != pfs->pfs_vnode) {
					fileid = va.va_fileid;
					dev = va.va_fsid;
					error = procfs_vnode_to_path(vp, path,
					    MAXPATHLEN * 4, curl, p);
				}
			}
			snprintf(mebuffer, sizeof(mebuffer),
			    "%0*lx-%0*lx %c%c%c%c %0*lx %02x:%02x %ld     %s\n",
			    (int)sizeof(void *) * 2,(unsigned long)entry->start,
			    (int)sizeof(void *) * 2,(unsigned long)entry->end,
			    (entry->protection & VM_PROT_READ) ? 'r' : '-',
			    (entry->protection & VM_PROT_WRITE) ? 'w' : '-',
			    (entry->protection & VM_PROT_EXECUTE) ? 'x' : '-',
			    (entry->etype & UVM_ET_COPYONWRITE) ? 'p' : 's',
			    (int)sizeof(void *) * 2,
			    (unsigned long)entry->offset,
			    major(dev), minor(dev), fileid, path);
		} else {
			snprintf(mebuffer, sizeof(mebuffer),
			    "0x%lx 0x%lx %c%c%c %c%c%c %s %s %d %d %d\n",
			    entry->start, entry->end,
			    (entry->protection & VM_PROT_READ) ? 'r' : '-',
			    (entry->protection & VM_PROT_WRITE) ? 'w' : '-',
			    (entry->protection & VM_PROT_EXECUTE) ? 'x' : '-',
			    (entry->max_protection & VM_PROT_READ) ? 'r' : '-',
			    (entry->max_protection & VM_PROT_WRITE) ? 'w' : '-',
			    (entry->max_protection & VM_PROT_EXECUTE) ?
				'x' : '-',
			    (entry->etype & UVM_ET_COPYONWRITE) ?
				"COW" : "NCOW",
			    (entry->etype & UVM_ET_NEEDSCOPY) ? "NC" : "NNC",
			    entry->inheritance, entry->wired_count,
			    entry->advice);
		}

		len = strlen(mebuffer);
		if (len > uio->uio_resid) {
			error = EFBIG;
			break;
		}

		timestamp = map->timestamp;
		vm_map_unlock_read(map);
		error = uiomove(mebuffer, len, uio);
		vm_map_lock_read(map);
		if (error)
			break;

		if (timestamp != map->timestamp) {
			/*
			 * The map may have changed, so restart.  We
			 * make an ugly assumption about uiomove()
			 * and the vm_map timestamp: it will never
			 * fall back to copyout_vmspace() because
			 * we are copying out to curproc.
			 */
			KASSERT(uio->uio_vmspace == curproc->p_vmspace);
			retries++;
			memcpy(uio, &savuio, sizeof(*uio));
			goto restart;
		}
	}

	vm_map_unlock_read(map);
	uvmspace_free(vm);
	if (path != NULL)
		free(path, M_TEMP);

	return error;
}

int
procfs_validmap(struct lwp *l, struct mount *mp)
{
	return ((l->l_flag & LW_SYSTEM) == 0);
}

/*
 * Try to find a pathname for a vnode. Since there is no mapping
 * vnode -> parent directory, this needs the NAMECACHE_ENTER_REVERSE
 * option to work (to make cache_revlookup succeed).
 */
static int procfs_vnode_to_path(struct vnode *vp, char *path, int len,
				struct lwp *curl, struct proc *p)
{
	struct proc *curp = curl->l_proc;
	int error, lenused, elen;
	char *bp, *bend;
	struct vnode *dvp;

	bp = bend = &path[len];
	*(--bp) = '\0';

	error = vget(vp, LK_EXCLUSIVE | LK_RETRY);
	if (error != 0)
		return error;
	error = cache_revlookup(vp, &dvp, &bp, path);
	vput(vp);
	if (error != 0)
		return (error == -1 ? ENOENT : error);

	error = vget(dvp, 0);
	if (error != 0)
		return error;
	*(--bp) = '/';
	/* XXX GETCWD_CHECK_ACCESS == 0x0001 */
	error = getcwd_common(dvp, NULL, &bp, path, len / 2, 1, curl);

	/*
	 * Strip off emulation path for emulated processes looking at
	 * the maps file of a process of the same emulation. (Won't
	 * work if /emul/xxx is a symlink..)
	 */
	if (curp->p_emul == p->p_emul && curp->p_emul->e_path != NULL) {
		elen = strlen(curp->p_emul->e_path);
		if (!strncmp(bp, curp->p_emul->e_path, elen))
			bp = &bp[elen];
	}

	lenused = bend - bp;

	memcpy(path, bp, lenused);
	path[lenused] = 0;

	return 0;
}
