/*	$NetBSD: verified_exec.c,v 1.17 2005/06/20 15:06:18 elad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.17 2005/06/20 15:06:18 elad Exp $");
#else
__RCSID("$Id: verified_exec.c,v 1.17 2005/06/20 15:06:18 elad Exp $\n$NetBSD: verified_exec.c,v 1.17 2005/06/20 15:06:18 elad Exp $");
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
int     veriexecopen(dev_t dev, int flags, int fmt, struct proc *p);
int     veriexecclose(dev_t dev, int flags, int fmt, struct proc *p);
int     veriexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
		       struct proc *p);

void
veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self,
		   void *aux)
{
	veriexec_dev_usage = 0;
	veriexec_dprintf(("Veriexec: veriexecattach: Veriexec pseudo-device "
	    "attached.\n"));
}

int
veriexecopen(dev_t dev __unused, int flags __unused,
		 int fmt __unused, struct proc *p __unused)
{
	if (veriexec_verbose >= 2) {
		printf("Veriexec: veriexecopen: Veriexec load device "
		       "open attempt by uid=%u, pid=%u. (dev=%d)\n",
		       p->p_ucred->cr_uid, p->p_pid, dev);
	}

	if (suser(p->p_ucred, &p->p_acflag) != 0)
		return (EPERM);

	if (veriexec_dev_usage > 0) {
		veriexec_dprintf(("Veriexec: load device already in use\n"));
		return(EBUSY);
	}

	veriexec_dev_usage++;
	return (0);
}

int
veriexecclose(dev_t dev __unused, int flags __unused,
		  int fmt __unused, struct proc *p __unused)
{
	if (veriexec_dev_usage > 0)
		veriexec_dev_usage--;
	return (0);
}

int
veriexecioctl(dev_t dev __unused, u_long cmd, caddr_t data,
		  int flags __unused, struct proc *p)
{
	struct veriexec_hashtbl *tbl;
	struct nameidata nid;
	struct vattr va;
	int error = 0;
	u_long hashmask;

	if (veriexec_strict > 0) {
		printf("Veriexec: veriexecioctl: Strict mode, modifying "
		       "veriexec tables is not permitted.\n"); 

		return (EPERM);
	}
	
	switch (cmd) {
	case VERIEXEC_TABLESIZE: {
		struct veriexec_sizing_params *params =
			(struct veriexec_sizing_params *) data;
		u_char node_name[16];

		/* Check for existing table for device. */
		if (veriexec_tblfind(params->dev) != NULL)
			return (EEXIST);

		/* Allocate and initialize a Veriexec hash table. */
		tbl = malloc(sizeof(struct veriexec_hashtbl), M_TEMP,
			     M_WAITOK);
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

		break;
		}

	case VERIEXEC_LOAD: {
		struct veriexec_params *params =
			(struct veriexec_params *) data;
		struct veriexec_hash_entry *hh;
		struct veriexec_hash_entry *e;

		NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, params->file, p);
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
		error = VOP_GETATTR(nid.ni_vp, &va, p->p_ucred, p);
		if (error)
			return (error);

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
			 * Duplicate entry; handle access type conflict
			 * and enforce 'FILE' over 'INDIRECT' over
			 * 'DIRECT'.
			 */
			if (hh->type < params->type) {
				hh->type = params->type;

				veriexec_report("Duplicate entry with "
						"access type mismatch. "
						"Updating to stricter "
						"type.", params->file,
						&va, NULL,
						REPORT_NOVERBOSE,
						REPORT_NOALARM,
						REPORT_NOPANIC);
			} else {
				veriexec_report("Duplicate entry.",
						params->file, &va, NULL,
						REPORT_VERBOSE_HIGH,
						REPORT_NOALARM,
						REPORT_NOPANIC);
			}

			return (0);
		}

		e = malloc(sizeof(*e), M_TEMP, M_WAITOK);
		e->inode = va.va_fileid;
		e->type = params->type;
		e->status = FINGERPRINT_NOTEVAL;
		if ((e->ops = veriexec_find_ops(params->fp_type)) == NULL) {
			free(e, M_TEMP);
			printf("Veriexec: veriexecioctl: Invalid or unknown "
			       "fingerprint type \"%s\" for file \"%s\" "
			       "(dev=%lu, inode=%lu)\n", params->fp_type,
			       params->file, va.va_fsid, va.va_fileid);
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
			       "\"%s\" (dev=%lu, inode=%lu), size was %u "
			       "was expecting %lu\n", params->fp_type,
			       params->file, va.va_fsid, va.va_fileid,
			       params->size, (unsigned long)e->ops->hash_len);
			free(e, M_TEMP);
			return(EINVAL);
		}
			
		e->fp = malloc(e->ops->hash_len, M_TEMP, M_WAITOK);
		memcpy(e->fp, params->fingerprint, e->ops->hash_len);

		veriexec_dprintf(("Veriexec: veriexecioctl: New entry. (file=%s,"
		    " dev=%d, inode=%u)\n", params->vxp_file, va.va_fsid,
		    va.va_fileid));

		error = veriexec_hashadd(tbl, e);

		break;
		}

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
