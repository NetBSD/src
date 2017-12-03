/* $NetBSD: xenbus_dev.c,v 1.9.12.1 2017/12/03 11:36:52 jdolecek Exp $ */
/*
 * xenbus_dev.c
 * 
 * Driver giving user-space access to the kernel's xenbus connection
 * to xenstore.
 * 
 * Copyright (c) 2005, Christian Limpach
 * Copyright (c) 2005, Rusty Russell, IBM Corporation
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenbus_dev.c,v 1.9.12.1 2017/12/03 11:36:52 jdolecek Exp $");

#include "opt_xen.h"

#include <sys/types.h>
#include <sys/null.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/tree.h> 
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <xen/kernfs_machdep.h>

#include <xen/hypervisor.h>
#include <xen/xenbus.h>
#include "xenbus_comms.h"

static int xenbus_dev_read(void *);
static int xenbus_dev_write(void *);
static int xenbus_dev_open(void *);
static int xenbus_dev_close(void *);
static int xsd_port_read(void *);

#define DIR_MODE	 (S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define PRIVCMD_MODE    (S_IRUSR | S_IWUSR)
static const struct kernfs_fileop xenbus_fileops[] = {
  { .kf_fileop = KERNFS_FILEOP_OPEN, .kf_vop = xenbus_dev_open },
  { .kf_fileop = KERNFS_FILEOP_CLOSE, .kf_vop = xenbus_dev_close },
  { .kf_fileop = KERNFS_FILEOP_READ, .kf_vop = xenbus_dev_read },
  { .kf_fileop = KERNFS_FILEOP_WRITE, .kf_vop = xenbus_dev_write },
};

#define XSD_MODE    (S_IRUSR)
static const struct kernfs_fileop xsd_port_fileops[] = {
    { .kf_fileop = KERNFS_FILEOP_READ, .kf_vop = xsd_port_read },
};

static kmutex_t xenbus_dev_open_mtx;

void
xenbus_kernfs_init(void)
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	kfst = KERNFS_ALLOCTYPE(xenbus_fileops);
	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "xenbus", NULL, kfst, VREG,
	    PRIVCMD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);

	if (xendomain_is_dom0()) {
		kfst = KERNFS_ALLOCTYPE(xsd_port_fileops);
		KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
		KERNFS_INITENTRY(dkt, DT_REG, "xsd_port", NULL,
		    kfst, VREG, XSD_MODE);
		kernfs_addentry(kernxen_pkt, dkt);
	}
	mutex_init(&xenbus_dev_open_mtx, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * several process may open /kern/xen/xenbus in parallel.
 * In a transaction one or more write is followed by one or more read.
 * Unfortunably we don't get a file descriptor identifier down there,
 * which we could use to link a read() to a transaction started in a write().
 * To work around this we keep a list of lwp that opended the xenbus file.
 * This assumes that a single lwp won't open /kern/xen/xenbus more
 * than once, and that a transaction started by one lwp won't be ended
 * by another.
 * because of this, we also assume that we always got the data before
 * the read() syscall.
 */

struct xenbus_dev_transaction {
	SLIST_ENTRY(xenbus_dev_transaction) trans_next;
	struct xenbus_transaction *handle;
};

struct xenbus_dev_lwp {
	SLIST_ENTRY(xenbus_dev_lwp) lwp_next;
	SLIST_HEAD(, xenbus_dev_transaction) transactions;
	lwp_t *lwp;
	/* Response queue. */
#define BUFFER_SIZE (PAGE_SIZE)
#define MASK_READ_IDX(idx) ((idx)&(BUFFER_SIZE-1))
	char read_buffer[BUFFER_SIZE];
	unsigned int read_cons, read_prod;
	/* Partial request. */
	unsigned int len;
	union {
		struct xsd_sockmsg msg;
		char buffer[BUFFER_SIZE];
	} u;
	kmutex_t mtx;
};

struct xenbus_dev_data {
	/* lwps which opended this device */
	SLIST_HEAD(, xenbus_dev_lwp) lwps;
	kmutex_t mtx;
};


static int
xenbus_dev_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct uio *uio = ap->a_uio;
	struct xenbus_dev_data *u;
	struct xenbus_dev_lwp *xlwp;
	int err;
	off_t offset;

	mutex_enter(&xenbus_dev_open_mtx);
	u = kfs->kfs_v;
	if (u == NULL) {
		mutex_exit(&xenbus_dev_open_mtx);
		return EBADF;
	}
	mutex_enter(&u->mtx);
	mutex_exit(&xenbus_dev_open_mtx);
	SLIST_FOREACH(xlwp, &u->lwps, lwp_next) {
		if (xlwp->lwp == curlwp) {
			break;
		}
	}
	if (xlwp == NULL) {
		mutex_exit(&u->mtx);
		return EBADF;
	}
	mutex_enter(&xlwp->mtx);
	mutex_exit(&u->mtx);

	if (xlwp->read_prod == xlwp->read_cons) {
		err = EWOULDBLOCK;
		goto end;
	}

	offset = uio->uio_offset;
	if (xlwp->read_cons > xlwp->read_prod) {
		err = uiomove(
		    &xlwp->read_buffer[MASK_READ_IDX(xlwp->read_cons)],
		    0U - xlwp->read_cons, uio);
		if (err)
			goto end;
		xlwp->read_cons += (uio->uio_offset - offset);
		offset = uio->uio_offset;
	}
	err = uiomove(&xlwp->read_buffer[MASK_READ_IDX(xlwp->read_cons)],
	    xlwp->read_prod - xlwp->read_cons, uio);
	if (err == 0)
		xlwp->read_cons += (uio->uio_offset - offset);

end:
	mutex_exit(&xlwp->mtx);
	return err;
}

static void
queue_reply(struct xenbus_dev_lwp *xlwp,
			char *data, unsigned int len)
{
	int i;
	KASSERT(mutex_owned(&xlwp->mtx));
	for (i = 0; i < len; i++, xlwp->read_prod++)
		xlwp->read_buffer[MASK_READ_IDX(xlwp->read_prod)] = data[i];

	KASSERT((xlwp->read_prod - xlwp->read_cons) <= sizeof(xlwp->read_buffer));
}

static int
xenbus_dev_write(void *v)
{
	struct vop_write_args /* {    
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct uio *uio = ap->a_uio;

	struct xenbus_dev_data *u;
	struct xenbus_dev_lwp *xlwp;
	struct xenbus_dev_transaction *trans;
	void *reply;
	int err;
	size_t size;

	mutex_enter(&xenbus_dev_open_mtx);
	u = kfs->kfs_v;
	if (u == NULL) {
		mutex_exit(&xenbus_dev_open_mtx);
		return EBADF;
	}
	mutex_enter(&u->mtx);
	mutex_exit(&xenbus_dev_open_mtx);
	SLIST_FOREACH(xlwp, &u->lwps, lwp_next) {
		if (xlwp->lwp == curlwp) {
			break;
		}
	}
	if (xlwp == NULL) {
		mutex_exit(&u->mtx);
		return EBADF;
	}
	mutex_enter(&xlwp->mtx);
	mutex_exit(&u->mtx);

	if (uio->uio_offset < 0) {
		err = EINVAL;
		goto end;
	}
	size = uio->uio_resid;

	if ((size + xlwp->len) > sizeof(xlwp->u.buffer)) {
		err = EINVAL;
		goto end;
	}

	err = uiomove(xlwp->u.buffer + xlwp->len,
		      sizeof(xlwp->u.buffer) -  xlwp->len, uio);
	if (err) 
		goto end;

	xlwp->len += size;
	if (xlwp->len < (sizeof(xlwp->u.msg) + xlwp->u.msg.len))
		goto end;

	switch (xlwp->u.msg.type) {
	case XS_TRANSACTION_START:
	case XS_TRANSACTION_END:
	case XS_DIRECTORY:
	case XS_READ:
	case XS_GET_PERMS:
	case XS_RELEASE:
	case XS_GET_DOMAIN_PATH:
	case XS_WRITE:
	case XS_MKDIR:
	case XS_RM:
	case XS_SET_PERMS:
		err = xenbus_dev_request_and_reply(&xlwp->u.msg, &reply);
		if (err == 0) {
			if (xlwp->u.msg.type == XS_TRANSACTION_START) {
				trans = malloc(sizeof(*trans), M_DEVBUF,
				    M_WAITOK);
				trans->handle = (struct xenbus_transaction *)
					strtoul(reply, NULL, 0);
				SLIST_INSERT_HEAD(&xlwp->transactions,
				    trans, trans_next);
			} else if (xlwp->u.msg.type == XS_TRANSACTION_END) {
				SLIST_FOREACH(trans, &xlwp->transactions,
						    trans_next) {
					if ((unsigned long)trans->handle ==
					    (unsigned long)xlwp->u.msg.tx_id)
						break;
				}
				if (trans == NULL) {
					err = EINVAL;
					goto end;
				}
				SLIST_REMOVE(&xlwp->transactions, trans, 
				    xenbus_dev_transaction, trans_next);
				free(trans, M_DEVBUF);
			}
			queue_reply(xlwp, (char *)&xlwp->u.msg,
						sizeof(xlwp->u.msg));
			queue_reply(xlwp, (char *)reply, xlwp->u.msg.len);
			free(reply, M_DEVBUF);
		}
		break;

	default:
		err = EINVAL;
		break;
	}

	if (err == 0) {
		xlwp->len = 0;
	}
end:
	mutex_exit(&xlwp->mtx);
	return err;
}

static int
xenbus_dev_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct xenbus_dev_data *u;
	struct xenbus_dev_lwp *xlwp;

	if (xen_start_info.store_evtchn == 0)
		return ENOENT;

	mutex_enter(&xenbus_dev_open_mtx);
	u = kfs->kfs_v;
	if (u == NULL) {
		u = malloc(sizeof(*u), M_DEVBUF, M_WAITOK);
		if (u == NULL) {      
			mutex_exit(&xenbus_dev_open_mtx);
			return ENOMEM;
		}
		memset(u, 0, sizeof(*u));
		SLIST_INIT(&u->lwps); 
		mutex_init(&u->mtx, MUTEX_DEFAULT, IPL_NONE);
		kfs->kfs_v = u;       
	};
	mutex_enter(&u->mtx);
	mutex_exit(&xenbus_dev_open_mtx);
	SLIST_FOREACH(xlwp, &u->lwps, lwp_next) {
		if (xlwp->lwp == curlwp) {
			break;
		}
	}
	if (xlwp == NULL) {
		xlwp = malloc(sizeof(*xlwp ), M_DEVBUF, M_WAITOK);      
		if (xlwp  == NULL) {  
			mutex_exit(&u->mtx);
			return ENOMEM;
		}
		memset(xlwp, 0, sizeof(*xlwp));
		xlwp->lwp = curlwp;   
		SLIST_INIT(&xlwp->transactions);
		mutex_init(&xlwp->mtx, MUTEX_DEFAULT, IPL_NONE);
		SLIST_INSERT_HEAD(&u->lwps,
		    xlwp, lwp_next);  
	}
	mutex_exit(&u->mtx);
	return 0;
}

static int
xenbus_dev_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);

	struct xenbus_dev_data *u;
	struct xenbus_dev_lwp *xlwp;
	struct xenbus_dev_transaction *trans;

	mutex_enter(&xenbus_dev_open_mtx);
	u = kfs->kfs_v;
	KASSERT(u != NULL);
	mutex_enter(&u->mtx);
	SLIST_FOREACH(xlwp, &u->lwps, lwp_next) {
		if (xlwp->lwp == curlwp) {
			break;
		}
	}
	if (xlwp == NULL) {
		mutex_exit(&u->mtx);
		mutex_exit(&xenbus_dev_open_mtx);
		return EBADF;
	}
	mutex_enter(&xlwp->mtx);
	while (!SLIST_EMPTY(&xlwp->transactions)) {
		trans = SLIST_FIRST(&xlwp->transactions);
		xenbus_transaction_end(trans->handle, 1);
		SLIST_REMOVE_HEAD(&xlwp->transactions, trans_next);
		free(trans, M_DEVBUF);
	}
	mutex_exit(&xlwp->mtx);
	SLIST_REMOVE(&u->lwps, xlwp, xenbus_dev_lwp, lwp_next);
	mutex_destroy(&xlwp->mtx);

	if (!SLIST_EMPTY(&u->lwps)) {
		mutex_exit(&u->mtx);
		mutex_exit(&xenbus_dev_open_mtx);
		return 0;
	}
	mutex_exit(&u->mtx);
	mutex_destroy(&u->mtx);
	kfs->kfs_v = NULL;
	mutex_exit(&xenbus_dev_open_mtx);
	free(xlwp, M_DEVBUF);
	free(u, M_DEVBUF);
	return 0;
}

#define LD_STRLEN 21 /* a 64bit integer needs 20 digits in base10 */

static int
xsd_port_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	int off, error;
	size_t len;
	char strbuf[LD_STRLEN], *bf;

	off = (int)uio->uio_offset;
	if (off < 0)
		return EINVAL;

	len  = snprintf(strbuf, sizeof(strbuf), "%ld\n",
	    (long)xen_start_info.store_evtchn);
	if (off >= len) {
		bf = strbuf;
		len = 0;
	} else {
		bf = &strbuf[off];
		len -= off;
	}
	error = uiomove(bf, len, uio);
	return error;
}

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
