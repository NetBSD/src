/*	$NetBSD: verified_exec.c,v 1.61 2007/07/09 21:00:29 ad Exp $	*/

/*-
 * Copyright (c) 2005, 2006 Elad Efrat <elad@NetBSD.org>
 * Copyright (c) 2005, 2006 Brett Lymn <blymn@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the authors may not be used to endorse or promote products
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
#if defined(__NetBSD__)
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.61 2007/07/09 21:00:29 ad Exp $");
#else
__RCSID("$Id: verified_exec.c,v 1.61 2007/07/09 21:00:29 ad Exp $\n$NetBSD: verified_exec.c,v 1.61 2007/07/09 21:00:29 ad Exp $");
#endif

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/verified_exec.h>
#include <sys/kauth.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#ifdef __FreeBSD__
#include <sys/kernel.h>
#include <sys/device_port.h>
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#include <sys/device.h>
#define DEVPORT_DEVICE struct device
#endif

#include <prop/proplib.h>

void veriexecattach(struct device *, struct device *, void *);
static dev_type_open(veriexecopen);
static dev_type_close(veriexecclose);
static dev_type_ioctl(veriexecioctl);

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

/* count of number of times device is open (we really only allow one open) */
static unsigned int veriexec_dev_usage = 0;

void
veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self, void *aux)
{
	veriexec_dev_usage = 0;
}

static int
veriexecopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL))
		return (EPERM);

	if (veriexec_dev_usage > 0)
		return(EBUSY);

	veriexec_dev_usage++;
	return (0);
}

static int
veriexecclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	if (veriexec_dev_usage > 0)
		veriexec_dev_usage--;
	return (0);
}

static int
veriexec_delete(prop_dictionary_t dict, struct lwp *l)
{
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	/* XXX this should be done differently... */
	if (nid.ni_vp->v_type == VREG)
		error = veriexec_file_delete(l, nid.ni_vp);
	else if (nid.ni_vp->v_type == VDIR)
		error = veriexec_table_delete(l, nid.ni_vp->v_mount);

	vrele(nid.ni_vp);

	return (error);
}

static int
veriexec_query(prop_dictionary_t dict, prop_dictionary_t rdict, struct lwp *l)
{
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	error = veriexec_convert(nid.ni_vp, rdict);

	vrele(nid.ni_vp);

	return (error);
}

int
veriexecioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	extern int veriexec_strict;
	struct plistref *plistref;
	prop_dictionary_t dict;
	int error = 0;

	/* XXX This should be replaced with a kauth(9) request. */
	switch (cmd) {
	case VERIEXEC_TABLESIZE:
	case VERIEXEC_LOAD:
	case VERIEXEC_DELETE:
	case VERIEXEC_FLUSH:
		if (veriexec_strict > VERIEXEC_LEARNING) {
			log(LOG_WARNING, "Veriexec: Strict mode, modifying "
			    "tables not permitted.\n");

			return (EPERM);
		}

		break;

	case VERIEXEC_QUERY:
	case VERIEXEC_DUMP:
		break;

	default:
		/* Invalid operation. */
		return (ENODEV);
	}

	plistref = (struct plistref *)data;

	switch (cmd) {
	case VERIEXEC_TABLESIZE:
		/* Do nothing. Kept for binary compatibility. */
		break;

	case VERIEXEC_LOAD:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_file_add(l, dict);
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
			prop_object_release(dict);
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

	case VERIEXEC_DUMP: {
		prop_array_t rarray;

		rarray = prop_array_create();
		if (rarray == NULL) {
			error = ENOMEM;
			break;
		}

		error = veriexec_dump(l, rarray);
		if (error == 0) {
			error = prop_array_copyout_ioctl(plistref, cmd,
			    rarray);
		}

		prop_object_release(rarray);

		break;
		}

	case VERIEXEC_FLUSH:
		error = veriexec_flush(l);
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
veriexec_drvinit(void *unused)
{
	make_dev(&verifiedexec_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
	    "veriexec");
	verifiedexecattach(0, 0, 0);
}

SYSINIT(veriexec, SI_SUB_PSEUDO, SI_ORDER_ANY, veriexec_drvinit, NULL);
#endif
