/*	$NetBSD: specfs.c,v 1.1 2007/08/05 22:28:09 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>

#include <uvm/uvm_extern.h>

#include "rump.h"
#include "rumpuser.h"

/* We have special special ops */
static int rump_specopen(void *);
static int rump_specioctl(void *);
static int rump_specclose(void *);
static int rump_specfsync(void *);
static int rump_specstrategy(void *);

int (**spec_vnodeop_p)(void *);
const struct vnodeopv_entry_desc rumpspec_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_open_desc, rump_specopen },		/* open */
	{ &vop_close_desc, rump_specclose },		/* close */
	{ &vop_ioctl_desc, rump_specioctl },		/* ioctl */
	{ &vop_fsync_desc, rump_specfsync },		/* fsync */
	{ &vop_strategy_desc, rump_specstrategy },	/* strategy */
	{ NULL, NULL }
};
const struct vnodeopv_desc spec_vnodeop_opv_desc =
	{ &spec_vnodeop_p, rumpspec_vnodeop_entries };

static int
rump_specopen(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rump_specpriv *sp = vp->v_data;
	int fd, error;

	fd = rumpuser_open(sp->rsp_path, OFLAGS(ap->a_mode), &error);
	if (fd == -1)
		return error;

	sp->rsp_fd = fd;
	return 0;
}

int
rump_specioctl(void *v)
{
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rump_specpriv *sp = vp->v_data;
	int rv, error;

	if (ap->a_command == DIOCGPART)
		return EOPNOTSUPP;

	rv = rumpuser_ioctl(sp->rsp_fd, ap->a_command, ap->a_data, &error);
	if (rv == -1)
		return error;

	return 0;
}

int
rump_specclose(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rump_specpriv *sp = vp->v_data;

	rumpuser_close(sp->rsp_fd);
	return 0;
}

int
rump_specfsync(void *v)
{

	return 0;
}

int
rump_specstrategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp = ap->a_bp;
	struct rump_specpriv *sp;
	ssize_t rv;
	int error;

	assert(vp->v_type == VBLK);
	sp = vp->v_data;

	if (bp->b_flags & B_READ)
		rv = rumpuser_pread(sp->rsp_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno << DEV_BSHIFT, &error);
	else
		rv = rumpuser_pwrite(sp->rsp_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno << DEV_BSHIFT, &error);

	bp->b_error = 0;
	if (rv == -1)
		bp->b_error = error;
	else
		bp->b_resid = bp->b_bcount - rv;

	return error;
}
