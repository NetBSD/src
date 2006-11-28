/*	$NetBSD: verified_exec.c,v 1.50 2006/11/28 22:22:02 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.50 2006/11/28 22:22:02 elad Exp $");
#else
__RCSID("$Id: verified_exec.c,v 1.50 2006/11/28 22:22:02 elad Exp $\n$NetBSD: verified_exec.c,v 1.50 2006/11/28 22:22:02 elad Exp $");
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
#include <sys/kauth.h>

#include <sys/fileassoc.h>
#include <sys/syslog.h>

#include <prop/proplib.h>

/* count of number of times device is open (we really only allow one open) */
static unsigned int veriexec_dev_usage;
static unsigned int veriexec_tablecount = 0;

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
       D_OTHER,
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

static int veriexec_newtable(prop_dictionary_t, struct lwp *);
static int veriexec_load(prop_dictionary_t, struct lwp *);
static int veriexec_delete(prop_dictionary_t, struct lwp *);
static int veriexec_query(prop_dictionary_t, prop_dictionary_t, struct lwp *);

void
veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self,
    void *aux)
{
	veriexec_dev_usage = 0;

	if (veriexec_verbose >= 2)
		log(LOG_DEBUG, "Veriexec: Pseudo-device attached.\n");
}

int
veriexecopen(dev_t dev, int flags,
		 int fmt, struct lwp *l)
{
	if (veriexec_verbose >= 2) {
		log(LOG_DEBUG, "Veriexec: Pseudo-device open attempt by "
		    "uid=%u, pid=%u. (dev=%u)\n",
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid,
		    dev);
	}

	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag) != 0)
		return (EPERM);

	if (veriexec_dev_usage > 0) {
		if (veriexec_verbose >= 2)
			log(LOG_ERR, "Veriexec: pseudo-device already in "
			    "use.\n");

		return(EBUSY);
	}

	veriexec_dev_usage++;
	return (0);
}

int
veriexecclose(dev_t dev, int flags, int fmt,
    struct lwp *l)
{
	if (veriexec_dev_usage > 0)
		veriexec_dev_usage--;
	return (0);
}

int
veriexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
    struct lwp *l)
{
	struct plistref *plistref;
	prop_dictionary_t dict;
	int error = 0;

	if (veriexec_strict > VERIEXEC_LEARNING) {
		log(LOG_WARNING, "Veriexec: Strict mode, modifying tables not "
		    "permitted.\n");

		return (EPERM);
	}

	plistref = (struct plistref *)data;

	switch (cmd) {
	case VERIEXEC_TABLESIZE:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_newtable(dict, l);
		prop_object_release(dict);
		break;

	case VERIEXEC_LOAD:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_load(dict, l);
		prop_object_release(dict);
		break;

	case VERIEXEC_DELETE:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_delete(dict, l);
		prop_object_release(dict);
		break;

	case VERIEXEC_QUERY: {
		prop_dictionary_t rdict;

		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			return (error);

		rdict = prop_dictionary_create();
		if (rdict == NULL) {
			error = ENOMEM;
			break;
		}

		error = veriexec_query(dict, rdict, l);
		if (error == 0) {
			error = prop_dictionary_copyout_ioctl(plistref, cmd,
			    rdict);
		}

		prop_object_release(rdict);
		prop_object_release(dict);

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
veriexec_drvinit(void *unused)
{
	make_dev(&verifiedexec_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
	    "veriexec");
	verifiedexecattach(0, 0, 0);
}

SYSINIT(veriexec, SI_SUB_PSEUDO, SI_ORDER_ANY, veriexec_drvinit, NULL);
#endif

/*
 * Create a new Veriexec table.
 */
static int
veriexec_newtable(prop_dictionary_t dict, struct lwp *l)
{
	struct veriexec_table_entry *vte;
	struct nameidata nid;
	u_char buf[16];
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "mount")), l);
	error = namei(&nid);
	if (error)
		return (error);

	error = fileassoc_table_add(nid.ni_vp->v_mount,
	    prop_number_integer_value(prop_dictionary_get(dict, "count")));
	if (error && (error != EEXIST))
		goto out;

	vte = malloc(sizeof(*vte), M_TEMP, M_WAITOK | M_ZERO);
	error = fileassoc_tabledata_add(nid.ni_vp->v_mount, veriexec_hook, vte);
#ifdef DIAGNOSTIC
	if (error)
		panic("Fileassoc: Inconsistency after adding table");
#endif /* DIAGNOSTIC */

	snprintf(buf, sizeof(buf), "table%u", veriexec_tablecount++);
	sysctl_createv(NULL, 0, &veriexec_count_node, &vte->vte_node,
		       0, CTLTYPE_NODE, buf, NULL, NULL, 0, NULL,
		       0, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_STRING, "mntpt",
		       NULL, NULL, 0, nid.ni_vp->v_mount->mnt_stat.f_mntonname,
		       0, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_STRING, "fstype",
		       NULL, NULL, 0, nid.ni_vp->v_mount->mnt_stat.f_fstypename,
		       0, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_QUAD, "nentries",
		       NULL, NULL, 0, &vte->vte_count, 0, CTL_CREATE, CTL_EOL);

 out:
	vrele(nid.ni_vp);
	return (error);
}

static int
veriexec_load(prop_dictionary_t dict, struct lwp *l)
{
	struct veriexec_file_entry *hh, *e;
	struct nameidata nid;
	const char *file, *fp_type;
	int error;

	file = prop_string_cstring_nocopy(prop_dictionary_get(dict, "file"));
	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, file, l);
	error = namei(&nid);
	if (error)
		return (error);

	/* Add only regular files. */
	if (nid.ni_vp->v_type != VREG) {
		log(LOG_ERR, "Veriexec: Not adding `%s': Not a regular file.\n",
		    file);
		error = EINVAL;
		goto out;
	}

	e = malloc(sizeof(*e), M_TEMP, M_WAITOK);

	fp_type = prop_string_cstring_nocopy(prop_dictionary_get(dict,
	    "fp-type"));
	if ((e->ops = veriexec_find_ops(__UNCONST(fp_type))) == NULL) {
		free(e, M_TEMP);
		log(LOG_ERR, "Veriexec: Invalid or unknown fingerprint type "
		    "`%s' for file `%s'.\n", fp_type, file);
		error = EINVAL;
		goto out;
	}

	e->fp = prop_data_data(prop_dictionary_get(dict, "fp"));
	if (e->fp == NULL) {
		free(e, M_TEMP);
		goto out;
	}

	hh = veriexec_lookup(nid.ni_vp);
	if (hh != NULL) {
		boolean_t fp_mismatch;

		if (strcmp(e->ops->type, fp_type) ||
		    memcmp(hh->fp, e->fp, hh->ops->hash_len))
			fp_mismatch = TRUE;
		else
			fp_mismatch = FALSE;

		if ((veriexec_verbose >= 1) || fp_mismatch)
			log(LOG_NOTICE, "Veriexec: Duplicate entry for `%s' "
			    "ignored. (%s fingerprint)\n", file, 
			    fp_mismatch ? "different" : "same");

		free(e->fp, M_TEMP);
		free(e, M_TEMP);

		error = 0;
		goto out;
	}

	e->type = prop_number_integer_value(prop_dictionary_get(dict,
	    "entry-type"));

	e->status = FINGERPRINT_NOTEVAL;

	e->page_fp = NULL;
	e->page_fp_status = PAGE_FP_NONE;
	e->npages = 0;
	e->last_page_size = 0;

	error = veriexec_hashadd(nid.ni_vp, e);
	if (error) {
		free(e->fp, M_TEMP);
		free(e, M_TEMP);
		goto out;
	}

	veriexec_report("New entry.", file, NULL, REPORT_DEBUG);

 out:
	vrele(nid.ni_vp);
	return (error);
}

static int
veriexec_delete(prop_dictionary_t dict, struct lwp *l)
{
	struct veriexec_table_entry *vte;
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	vte = veriexec_tblfind(nid.ni_vp);
	if (vte == NULL) {
		error = ENOENT;
		goto out;
	}

	/* XXX this should either receive the filename to remove OR a mount point! */
	/* Delete an entire table */
	if (nid.ni_vp->v_type == VDIR) {
		sysctl_free(__UNCONST(vte->vte_node));

		veriexec_tablecount--;

		error = fileassoc_table_clear(nid.ni_vp->v_mount, veriexec_hook);
		if (error)
			goto out;
	} else if (nid.ni_vp->v_type == VREG) {
		error = fileassoc_clear(nid.ni_vp, veriexec_hook);
		if (error)
			goto out;

		vte->vte_count--;
	}

 out:
	vrele(nid.ni_vp);

	return (error);
}

static int
veriexec_query(prop_dictionary_t dict, prop_dictionary_t rdict, struct lwp *l)
{
	struct veriexec_file_entry *vfe;
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	vfe = veriexec_lookup(nid.ni_vp);
	if (vfe == NULL) {
		error = ENOENT;
		goto out;
	}

	prop_dictionary_set_uint8(rdict, "entry-type", vfe->type);
	prop_dictionary_set_uint8(rdict, "status", vfe->status);
	prop_dictionary_set(rdict, "fp-type",
	    prop_string_create_cstring(vfe->ops->type));
	prop_dictionary_set(rdict, "fp",
	    prop_data_create_data(vfe->fp, vfe->ops->hash_len));

 out:
	vrele(nid.ni_vp);

	return (error);
}
