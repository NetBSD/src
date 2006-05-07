/* $NetBSD: xenbus_dev.c,v 1.4 2006/05/07 10:18:28 bouyer Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: xenbus_dev.c,v 1.4 2006/05/07 10:18:28 bouyer Exp $");

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
#include <machine/kernfs_machdep.h>

#include <machine/hypervisor.h>
#include <machine/xenbus.h>
#include "xenbus_comms.h"

static int xenbus_dev_read(void *);
static int xenbus_dev_write(void *);
static int xenbus_dev_open(void *);
static int xenbus_dev_close(void *);
static int xsd_port_read(void *);

struct xenbus_dev_transaction {
	SLIST_ENTRY(xenbus_dev_transaction) trans_next;
	struct xenbus_transaction *handle;
};

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

void
xenbus_kernfs_init()
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	kfst = KERNFS_ALLOCTYPE(xenbus_fileops);
	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "xenbus", NULL, kfst, VREG,
	    PRIVCMD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);

	kfst = KERNFS_ALLOCTYPE(xsd_port_fileops);
	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "xsd_port", NULL, kfst, VREG, XSD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}

struct xenbus_dev_data {
#define BUFFER_SIZE (PAGE_SIZE)
#define MASK_READ_IDX(idx) ((idx)&(BUFFER_SIZE-1))
	/* In-progress transaction. */
	SLIST_HEAD(, xenbus_dev_transaction) transactions;

	/* Partial request. */
	unsigned int len;
	union {
		struct xsd_sockmsg msg;
		char buffer[BUFFER_SIZE];
	} u;

	/* Response queue. */
	char read_buffer[BUFFER_SIZE];
	unsigned int read_cons, read_prod;
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
	struct xenbus_dev_data *u = kfs->kfs_v;
	int err;
	off_t offset;
	int s = spltty();

	while (u->read_prod == u->read_cons) {
		err = tsleep(u, PRIBIO | PCATCH, "xbrd", 0);
		if (err)
			goto end;
	}
	if (uio->uio_offset != 0) {
		err = EINVAL;
		goto end;
	}
	offset = 0;

	if (u->read_cons > u->read_prod) {
		err = uiomove(&u->read_buffer[MASK_READ_IDX(u->read_cons)],
		    0U - u->read_cons, uio);
		if (err)
			goto end;
		u->read_cons += uio->uio_offset;
		offset = uio->uio_offset;
	}
	err = uiomove(&u->read_buffer[MASK_READ_IDX(u->read_cons)],
	    u->read_prod - u->read_cons, uio);
	if (err == 0)
		u->read_cons += uio->uio_offset - offset;

end:
	splx(s);
	return err;
}

static void
queue_reply(struct xenbus_dev_data *u,
			char *data, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++, u->read_prod++)
		u->read_buffer[MASK_READ_IDX(u->read_prod)] = data[i];

	KASSERT((u->read_prod - u->read_cons) <= sizeof(u->read_buffer));

	wakeup(&u);
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

	struct xenbus_dev_data *u = kfs->kfs_v;
	struct xenbus_dev_transaction *trans;
	void *reply;
	int err;
	size_t size;

	if (uio->uio_offset < 0)
		return EINVAL;
	size = uio->uio_resid;

	if ((size + u->len) > sizeof(u->u.buffer))
		return EINVAL;

	err = uiomove(u->u.buffer + u->len, sizeof(u->u.buffer) -  u->len, uio);
	if (err)
		return err;

	u->len += size;
	if (u->len < (sizeof(u->u.msg) + u->u.msg.len))
		return 0;

	switch (u->u.msg.type) {
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
		err = xenbus_dev_request_and_reply(&u->u.msg, &reply);
		if (err == 0) {
			if (u->u.msg.type == XS_TRANSACTION_START) {
				trans = malloc(sizeof(*trans), M_DEVBUF,
				    M_WAITOK);
				trans->handle = (struct xenbus_transaction *)
					strtoul(reply, NULL, 0);
				SLIST_INSERT_HEAD(&u->transactions,
				    trans, trans_next);
			} else if (u->u.msg.type == XS_TRANSACTION_END) {
				SLIST_FOREACH(trans, &u->transactions,
						    trans_next) {
					if ((unsigned long)trans->handle ==
					    (unsigned long)u->u.msg.tx_id)
						break;
				}
				KASSERT(trans != NULL);
				SLIST_REMOVE(&u->transactions, trans, 
				    xenbus_dev_transaction, trans_next);
				free(trans, M_DEVBUF);
			}
			queue_reply(u, (char *)&u->u.msg, sizeof(u->u.msg));
			queue_reply(u, (char *)reply, u->u.msg.len);
			free(reply, M_DEVBUF);
		}
		break;

	default:
		err = EINVAL;
		break;
	}

	if (err == 0) {
		u->len = 0;
	}

	return err;
}

static int
xenbus_dev_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);    

	struct xenbus_dev_data *u;

	if (xen_start_info.store_evtchn == 0)
		return ENOENT;

	u = malloc(sizeof(*u), M_DEVBUF, M_WAITOK);
	if (u == NULL)
		return ENOMEM;

	memset(u, 0, sizeof(*u));
	SLIST_INIT(&u->transactions);

	kfs->kfs_v = u;

	return 0;
}

static int
xenbus_dev_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		struct ucred *a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);    

	struct xenbus_dev_data *u = kfs->kfs_v;
	struct xenbus_dev_transaction *trans;

	while (!SLIST_EMPTY(&u->transactions)) {
		trans = SLIST_FIRST(&u->transactions);
		xenbus_transaction_end(trans->handle, 1);
		SLIST_REMOVE_HEAD(&u->transactions, trans_next);
		free(trans, M_DEVBUF);
	}

	free(u, M_DEVBUF);
	kfs->kfs_v = NULL;

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
