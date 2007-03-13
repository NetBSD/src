/*	$NetBSD: tty_ptm.c,v 1.17.2.1 2007/03/13 16:51:58 ad Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tty_ptm.c,v 1.17.2.1 2007/03/13 16:51:58 ad Exp $");

#include "opt_ptm.h"

/* pty multiplexor driver /dev/ptm{,x} */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/signalvar.h>
#include <sys/uio.h>
#include <sys/filedesc.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/pty.h>
#include <sys/kauth.h>

#ifdef DEBUG_PTM
#define DPRINTF(a)	printf a
#else
#define DPRINTF(a)
#endif

#ifdef NO_DEV_PTM
const struct cdevsw ptm_cdevsw = {
	noopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_TTY
};
#else

static struct ptm_pty *ptm;
int pts_major, ptc_major;

static dev_t pty_getfree(void);
static int pty_alloc_master(struct lwp *, int *, dev_t *);
static int pty_alloc_slave(struct lwp *, int *, dev_t);

void ptmattach(int);

dev_t
pty_makedev(char ms, int minor)
{
	return makedev(ms == 't' ? pts_major : ptc_major, minor);
}


static dev_t
pty_getfree(void)
{
	extern kmutex_t pt_softc_mutex;
	int i;

	mutex_enter(&pt_softc_mutex);
	for (i = 0; i < npty; i++) {
		if (pty_isfree(i, 0))
			break;
	}
	mutex_exit(&pt_softc_mutex);
	return pty_makedev('t', i);
}

/*
 * Hacked up version of vn_open. We _only_ handle ptys and only open
 * them with FREAD|FWRITE and never deal with creat or stuff like that.
 *
 * We need it because we have to fake up root credentials to open the pty.
 */
int
pty_vn_open(struct vnode *vp, struct lwp *l)
{
	int error;

	if (vp->v_type != VCHR) {
		vput(vp);
		return EINVAL;
	}

	error = VOP_OPEN(vp, FREAD|FWRITE, lwp0.l_cred, l);

	if (error) {
		vput(vp);
		return error;
	}

	vp->v_writecount++;

	return 0;
}

static int
pty_alloc_master(struct lwp *l, int *fd, dev_t *dev)
{
	int error;
	struct file *fp;
	struct vnode *vp;
	int md;

	if ((error = falloc(l, &fp, fd)) != 0) {
		DPRINTF(("falloc %d\n", error));
		return error;
	}
retry:
	/* Find and open a free master pty. */
	*dev = pty_getfree();
	md = minor(*dev);
	if ((error = pty_check(md)) != 0) {
		DPRINTF(("pty_check %d\n", error));
		goto bad;
	}
	if (ptm == NULL) {
		DPRINTF(("no ptm\n"));
		error = EOPNOTSUPP;
		goto bad;
	}
	if ((error = (*ptm->allocvp)(ptm, l, &vp, *dev, 'p')) != 0) {
		DPRINTF(("pty_allocvp %d\n", error));
		goto bad;
	}

	if ((error = pty_vn_open(vp, l)) != 0) {
		DPRINTF(("pty_vn_open %d\n", error));
		/*
		 * Check if the master open failed because we lost
		 * the race to grab it.
		 */
		if (error != EIO)
			goto bad;
		error = !pty_isfree(md, 1);
		DPRINTF(("pty_isfree %d\n", error));
		if (error)
			goto retry;
		else
			goto bad;
	}
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	VOP_UNLOCK(vp, 0);
	FILE_SET_MATURE(fp);
	FILE_UNUSE(fp, l);
	return 0;
bad:
	FILE_UNUSE(fp, l);
	fdremove(l->l_proc->p_fd, *fd);
	ffree(fp);
	return error;
}

int
pty_grant_slave(struct lwp *l, dev_t dev)
{
	int error;
	struct vnode *vp;

	/*
	 * Open the slave.
	 * namei -> setattr -> unlock -> revoke -> vrele ->
	 * namei -> open -> unlock
	 * Three stage rocket:
	 * 1. Change the owner and permissions on the slave.
	 * 2. Revoke all the users of the slave.
	 * 3. open the slave.
	 */
	if (ptm == NULL)
		return EOPNOTSUPP;
	if ((error = (*ptm->allocvp)(ptm, l, &vp, dev, 't')) != 0)
		return error;

	if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
		struct vattr vattr;
		(*ptm->getvattr)(ptm, l, &vattr);
		/* Do the VOP_SETATTR() as root. */
		error = VOP_SETATTR(vp, &vattr, lwp0.l_cred, l);
		if (error) {
			DPRINTF(("setattr %d\n", error));
			VOP_UNLOCK(vp, 0);
			vrele(vp);
			return error;
		}
	}
	VOP_UNLOCK(vp, 0);
	if (vp->v_usecount > 1 ||
	    (vp->v_flag & (VALIASED | VLAYER)))
		VOP_REVOKE(vp, REVOKEALL);

	/*
	 * The vnode is useless after the revoke, we need to get it again.
	 */
	vrele(vp);
	return 0;
}

static int
pty_alloc_slave(struct lwp *l, int *fd, dev_t dev)
{
	int error;
	struct file *fp;
	struct vnode *vp;

	/* Grab a filedescriptor for the slave */
	if ((error = falloc(l, &fp, fd)) != 0) {
		DPRINTF(("falloc %d\n", error));
		return error;
	}

	if (ptm == NULL) {
		error = EOPNOTSUPP;
		goto bad;
	}

	if ((error = (*ptm->allocvp)(ptm, l, &vp, dev, 't')) != 0)
		goto bad;
	if ((error = pty_vn_open(vp, l)) != 0)
		goto bad;

	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	VOP_UNLOCK(vp, 0);
	FILE_SET_MATURE(fp);
	FILE_UNUSE(fp, l);
	return 0;
bad:
	FILE_UNUSE(fp, l);
	fdremove(l->l_proc->p_fd, *fd);
	ffree(fp);
	return error;
}

struct ptm_pty *
pty_sethandler(struct ptm_pty *nptm)
{
	struct ptm_pty *optm = ptm;
	ptm = nptm;
	return optm;
}

int
pty_fill_ptmget(struct lwp *l, dev_t dev, int cfd, int sfd, void *data)
{
	struct ptmget *ptmg = data;
	int error;

	if (ptm == NULL)
		return EOPNOTSUPP;

	ptmg->cfd = cfd == -1 ? minor(dev) : cfd;
	ptmg->sfd = sfd == -1 ? minor(dev) : sfd;

	error = (*ptm->makename)(ptm, l, ptmg->cn, sizeof(ptmg->cn), dev, 'p');
	if (error)
		return error;

	return (*ptm->makename)(ptm, l, ptmg->sn, sizeof(ptmg->sn), dev, 't');
}

void
/*ARGSUSED*/
ptmattach(int n)
{
	extern const struct cdevsw pts_cdevsw, ptc_cdevsw;
	/* find the major and minor of the pty devices */
	if ((pts_major = cdevsw_lookup_major(&pts_cdevsw)) == -1)
		panic("ptmattach: Can't find pty slave in cdevsw");
	if ((ptc_major = cdevsw_lookup_major(&ptc_cdevsw)) == -1)
		panic("ptmattach: Can't find pty master in cdevsw");
#ifdef COMPAT_BSDPTY
	ptm = &ptm_bsdpty;
#endif
}

static int
/*ARGSUSED*/
ptmopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error;
	int fd;
	dev_t ttydev;

	switch(minor(dev)) {
	case 0:		/* /dev/ptmx */
	case 2:		/* /emul/linux/dev/ptmx */
		if ((error = pty_alloc_master(l, &fd, &ttydev)) != 0)
			return error;
		if (minor(dev) == 2) {
			/*
			 * Linux ptyfs grants the pty right here.
			 * Handle this case here, instead of writing
			 * a new linux module.
			 */
			if ((error = pty_grant_slave(l, ttydev)) != 0) {
				struct file *fp =
				    fd_getfile(l->l_proc->p_fd, fd);
				if (fp != NULL) {
					FILE_UNUSE(fp, l);
					fdremove(l->l_proc->p_fd, fd);
					ffree(fp);
				}
				return error;
			}
		}
		curlwp->l_dupfd = fd;
		return EMOVEFD;
	case 1:		/* /dev/ptm */
		return 0;
	default:
		return ENODEV;
	}
}

static int
/*ARGSUSED*/
ptmclose(dev_t dev, int flag, int mode, struct lwp *l)
{

	return (0);
}

static int
/*ARGSUSED*/
ptmioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;
	dev_t newdev;
	int cfd, sfd;
	struct file *fp;
	struct proc *p = l->l_proc;

	error = 0;
	switch (cmd) {
	case TIOCPTMGET:
		if ((error = pty_alloc_master(l, &cfd, &newdev)) != 0)
			return error;

		if ((error = pty_grant_slave(l, newdev)) != 0)
			goto bad;

		if ((error = pty_alloc_slave(l, &sfd, newdev)) != 0)
			goto bad;

		/* now, put the indices and names into struct ptmget */
		return pty_fill_ptmget(l, newdev, cfd, sfd, data);
	default:
		DPRINTF(("ptmioctl EINVAL\n"));
		return EINVAL;
	}
bad:
	fp = fd_getfile(p->p_fd, cfd);
	if (fp != NULL) {
		FILE_UNUSE(fp, l);
		fdremove(p->p_fd, cfd);
		ffree(fp);
	}
	return error;
}

const struct cdevsw ptm_cdevsw = {
	ptmopen, ptmclose, noread, nowrite, ptmioctl,
	nullstop, notty, nopoll, nommap, nokqfilter, D_TTY
};
#endif
