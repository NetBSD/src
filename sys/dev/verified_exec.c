/*	$NetBSD: verified_exec.c,v 1.31.10.3 2006/05/11 23:28:05 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@bsd.org.il>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org>
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brett Lymn and Elad Efrat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.31.10.3 2006/05/11 23:28:05 elad Exp $");
#else
__RCSID("$Id: verified_exec.c,v 1.31.10.3 2006/05/11 23:28:05 elad Exp $\n$NetBSD: verified_exec.c,v 1.31.10.3 2006/05/11 23:28:05 elad Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#ifdef __FreeBSD__
#include <sys/kernel.h>
#include <sys/device_port.h>
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#include <sys/device.h>
#define DEVPORT_DEVICE struct device
#endif

#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#define VERIEXEC_NEED_NODE
#include <sys/verified_exec.h>

/* count of number of times device is open (we really only allow one open) */
static unsigned int veriexec_dev_usage;

struct veriexec_softc {
        DEVPORT_DEVICE veriexec_dev;
};

#if defined(__FreeBSD__)
# define CDEV_MAJOR 216
# define BDEV_MAJOR -1
#endif

const struct cdevsw veriexec_cdevsw = {
        veriexecopen,
	veriexecclose,
	noread,
	nowrite,
        veriexecioctl,
#ifdef __NetBSD__
	nostop,
	notty,
#endif
	nopoll,
	nommap,
#if defined(__NetBSD__)
       nokqfilter,
#elif defined(__FreeBSD__)
       nostrategy,
       "veriexec",
       CDEV_MAJOR,
       nodump,
       nopsize,
       0,                              /* flags */
       BDEV_MAJOR
#endif
};

/* Autoconfiguration glue */
void    veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self,
			void *aux);
int     veriexecopen(dev_t dev, int flags, int fmt, struct lwp *l);
int     veriexecclose(dev_t dev, int flags, int fmt, struct lwp *l);
int     veriexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
		       struct lwp *l);

void
veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self,
		   void *aux)
{
	veriexec_dev_usage = 0;

	if (veriexec_verbose >= 2)
		printf("Veriexec: veriexecattach: Veriexec pseudo-device"
		       "attached.\n");
}

int
veriexecopen(dev_t dev __unused, int flags __unused,
		 int fmt __unused, struct lwp *l __unused)
{
	if (veriexec_verbose >= 2) {
		printf("Veriexec: veriexecopen: Veriexec load device "
		       "open attempt by uid=%u, pid=%u. (dev=%u)\n",
		       kauth_cred_geteuid(l->l_proc->p_cred),
		       l->l_proc->p_pid, dev);
	}

	if (kauth_authorize_generic(l->l_proc->p_cred, KAUTH_GENERIC_ISSUSER,
			      &l->l_proc->p_acflag) != 0)
		return (EPERM);

	if (veriexec_dev_usage > 0) {
		if (veriexec_verbose >= 2)
			printf("Veriexec: load device already in use.\n");

		return(EBUSY);
	}

	veriexec_dev_usage++;
	return (0);
}

int
veriexecclose(dev_t dev __unused, int flags __unused,
		  int fmt __unused, struct lwp *l __unused)
{
	if (veriexec_dev_usage > 0)
		veriexec_dev_usage--;
	return (0);
}

int
veriexecioctl(dev_t dev __unused, u_long cmd, caddr_t data,
		  int flags __unused, struct lwp *l)
{
	int error = 0;

	if (veriexec_strict > 0) {
		printf("Veriexec: veriexecioctl: Strict mode, modifying "
		       "veriexec tables is not permitted.\n"); 

		return (EPERM);
	}
	
	switch (cmd) {
	case VERIEXEC_TABLESIZE:
		error = veriexec_newtable((struct veriexec_sizing_params *)
					  data);
		break;

	case VERIEXEC_LOAD:
		error = veriexec_load((struct veriexec_params *)data, l);
		break;

	case VERIEXEC_DELETE:
		error = veriexec_delete((struct veriexec_delete_params *)data);
		break;

	case VERIEXEC_QUERY:
		error = veriexec_query((struct veriexec_query_params *)data);
		break;

	default:
		/* Invalid operation. */
		error = ENODEV;
		break;
	}

	return (error);
}

#if defined(__FreeBSD__)
static void
veriexec_drvinit(void *unused __unused)
{
	make_dev(&verifiedexec_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
		 "veriexec");
	verifiedexecattach(0, 0, 0);
}

SYSINIT(veriexec, SI_SUB_PSEUDO, SI_ORDER_ANY, veriexec_drvinit, NULL);
#endif

int
veriexec_newtable(struct veriexec_sizing_params *params)
{
	struct veriexec_hashtbl *tbl;
	u_char node_name[16];
	u_long hashmask;

	/* Check for existing table for device. */
	if (veriexec_tblfind(params->dev) != NULL)
		return (EEXIST);

	/* Allocate and initialize a Veriexec hash table. */
	tbl = malloc(sizeof(*tbl), M_TEMP, M_WAITOK);
	tbl->hash_size = params->hash_size;
	tbl->hash_dev = params->dev;
	tbl->hash_tbl = hashinit(params->hash_size, HASH_LIST, M_TEMP,
				 M_WAITOK, &hashmask);
	tbl->hash_count = 0;

	LIST_INSERT_HEAD(&veriexec_tables, tbl, hash_list);

	snprintf(node_name, sizeof(node_name), "dev_%u",
		 tbl->hash_dev);

	sysctl_createv(NULL, 0, &veriexec_count_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_QUAD, node_name,
		       NULL, NULL, 0, &tbl->hash_count, 0,
		       tbl->hash_dev, CTL_EOL);

	return (0);
}

int
veriexec_load(struct veriexec_params *params, struct lwp *l)
{
	struct veriexec_hashtbl *tbl;
	struct veriexec_hash_entry *hh;
	struct veriexec_hash_entry *e;
	struct nameidata nid;
	struct vattr va;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, params->file, l);
	error = namei(&nid);
	if (error)
		return (error);

	/* Add only regular files. */
	if (nid.ni_vp->v_type != VREG) {
		printf("Veriexec: veriexecioctl: Not adding \"%s\": "
		    "Not a regular file.\n", params->file);
		vrele(nid.ni_vp);
		return (EINVAL);
	}

	/* Get attributes for device and inode. */
	error = VOP_GETATTR(nid.ni_vp, &va, l->l_proc->p_cred, l);
	if (error) {
		vrele(nid.ni_vp);
		return (error);
	}

	/* Release our reference to the vnode. (namei) */
	vrele(nid.ni_vp);

	/* Get table for the device. */
	tbl = veriexec_tblfind(va.va_fsid);
	if (tbl == NULL) {
		return (EINVAL);
	}

	hh = veriexec_lookup(va.va_fsid, va.va_fileid);
	if (hh != NULL) {
		/*
		 * Duplicate entry means something is wrong in
		 * the signature file. Just give collision info
		 * and return.
		 */
		printf("veriexec: Duplicate entry. [%s, %ld:%llu] "
		       "old[type=0x%02x, algorithm=%s], "
		       "new[type=0x%02x, algorithm=%s] "
		       "(%s fingerprint)\n",
		       params->file, va.va_fsid,
		       (unsigned long long)va.va_fileid,
		       hh->type, hh->ops->type,
		       params->type, params->fp_type,
		       (((hh->ops->hash_len != params->size) ||
			(memcmp(hh->fp, params->fingerprint,
				min(hh->ops->hash_len, params->size))
				!= 0)) ? "different" : "same"));

			return (0);
	}

	e = malloc(sizeof(*e), M_TEMP, M_WAITOK);
	e->inode = va.va_fileid;
	e->type = params->type;
	e->status = FINGERPRINT_NOTEVAL;
	e->page_fp = NULL;
	e->page_fp_status = PAGE_FP_NONE;
	e->npages = 0;
	e->last_page_size = 0;
	if ((e->ops = veriexec_find_ops(params->fp_type)) == NULL) {
		free(e, M_TEMP);
		printf("Veriexec: veriexecioctl: Invalid or unknown "
		       "fingerprint type \"%s\" for file \"%s\" "
		       "(dev=%ld, inode=%llu)\n", params->fp_type,
		       params->file, va.va_fsid, 
		       (unsigned long long)va.va_fileid);
		return(EINVAL);
	}

	/*
	 * Just a bit of a sanity check - require the size of
	 * the fp to be passed in, check this against the expected
	 * size.  Of course userland could lie deliberately, this
	 * really only protects against the obvious fumble of
	 * changing the fp type but not updating the fingerprint
	 * string.
	 */
	if (e->ops->hash_len != params->size) {
		printf("Veriexec: veriexecioctl: Inconsistent "
		       "fingerprint size for type \"%s\" for file "
		       "\"%s\" (dev=%ld, inode=%llu), size was %u "
		       "was expecting %zu\n", params->fp_type,
		       params->file, va.va_fsid,
		       (unsigned long long)va.va_fileid,
		       params->size, e->ops->hash_len);

		free(e, M_TEMP);
		return(EINVAL);
	}
			
	e->fp = malloc(e->ops->hash_len, M_TEMP, M_WAITOK);
	memcpy(e->fp, params->fingerprint, e->ops->hash_len);

	veriexec_report("New entry.", params->file, &va, NULL,
			REPORT_VERBOSE_HIGH, REPORT_NOALARM,
			REPORT_NOPANIC);

	error = veriexec_hashadd(tbl, e);

	return (error);
}

int
veriexec_delete(struct veriexec_delete_params *params)
{
	struct veriexec_hashtbl *tbl;
	struct veriexec_hash_entry *vhe;

	/* Delete an entire table */
	if (params->ino == 0) {
		struct veriexec_hashhead *tbl_list;
		u_long i;

		tbl = veriexec_tblfind(params->dev);
		if (tbl == NULL)
			return (ENOENT);

		/* Remove all entries from the table and lists */
		tbl_list = tbl->hash_tbl;
		for (i = 0; i < tbl->hash_size; i++) {
			while (LIST_FIRST(&tbl_list[i]) != NULL) {
				vhe = LIST_FIRST(&tbl_list[i]);
				if (vhe->fp != NULL)
					free(vhe->fp, M_TEMP);
				if (vhe->page_fp != NULL)
					free(vhe->page_fp, M_TEMP);
				LIST_REMOVE(vhe, entries);
				free(vhe, M_TEMP);
			}
		}

		/* Remove hash table and sysctl node */
		hashdone(tbl->hash_tbl, M_TEMP);
		LIST_REMOVE(tbl, hash_list);
		sysctl_destroyv(__UNCONST(veriexec_count_node), params->dev,
				CTL_EOL);
	} else {
		tbl = veriexec_tblfind(params->dev);
		if (tbl == NULL)
			return (ENOENT);

		vhe = veriexec_lookup(params->dev, params->ino);
		if (vhe != NULL) {
			if (vhe->fp != NULL)
				free(vhe->fp, M_TEMP);
			if (vhe->page_fp != NULL)
				free(vhe->page_fp, M_TEMP);
			LIST_REMOVE(vhe, entries);
			free(vhe, M_TEMP);

			tbl->hash_count--;
		}
	}

	return (0);
}

int
veriexec_query(struct veriexec_query_params *params)
{
	struct veriexec_hash_entry *vhe;
	int error;

	vhe = veriexec_lookup(params->dev, params->ino);
	if (vhe == NULL)
		return (ENOENT);

	params->type = vhe->type;
	params->status = vhe->status;
	params->hash_len = vhe->ops->hash_len;
	strlcpy(params->fp_type, vhe->ops->type, sizeof(params->fp_type));
	memcpy(params->fp_type, vhe->ops->type, sizeof(params->fp_type));
	error = copyout(params, params->uaddr, sizeof(*params));
	if (error)
		return (error);
	if (params->fp_bufsize >= vhe->ops->hash_len) {
		error = copyout(vhe->fp, params->fp, vhe->ops->hash_len);
		if (error)
			return (error);
	} else
		error = ENOMEM;

	return (error);
}
