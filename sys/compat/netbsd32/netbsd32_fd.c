/*	$NetBSD: netbsd32_fd.c,v 1.1 2018/12/24 21:27:05 mrg Exp $	*/

/*
 * Copyright (c) 1998, 2001, 2008, 2018 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: NetBSD: netbsd32_netbsd.c,v 1.221 2018/12/24 20:44:39 mrg Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_fd.c,v 1.1 2018/12/24 21:27:05 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/vfs_syscalls.h>
#include <sys/kauth.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>


int
netbsd32___getfh30(struct lwp *l, const struct netbsd32___getfh30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_size_tp) fh_size;
	} */
	struct vnode *vp;
	fhandle_t *fh;
	int error;
	struct pathbuf *pb;
	struct nameidata nd;
	netbsd32_size_t usz32, sz32;
	size_t sz;

	/*
	 * Must be super user
	 */
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL);
	if (error)
		return error;

	error = pathbuf_copyin(SCARG_P32(uap, fname), &pb);
	if (error) {
		return error;
	}

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error) {
		pathbuf_destroy(pb);
		return error;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);

	error = vfs_composefh_alloc(vp, &fh);
	vput(vp);
	if (error != 0) {
		return error;
	}
	error = copyin(SCARG_P32(uap, fh_size), &usz32, sizeof(usz32));
	if (error != 0) {
		goto out;
	}
	sz = FHANDLE_SIZE(fh);
	sz32 = sz;

	error = copyout(&sz32, SCARG_P32(uap, fh_size), sizeof(sz32));
	if (error != 0) {
		goto out;
	}
	if (usz32 >= sz32) {
		error = copyout(fh, SCARG_P32(uap, fhp), sz);
	} else {
		error = E2BIG;
	}
out:
	vfs_composefh_free(fh);
	return error;
}

int
netbsd32_pipe2(struct lwp *l, const struct netbsd32_pipe2_args *uap,
	       register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) fildes;
		syscallarg(int) flags;
	} */
	int fd[2], error;

	error = pipe1(l, fd, SCARG(uap, flags));
	if (error != 0)
		return error;

	error = copyout(fd, SCARG_P32(uap, fildes), sizeof(fd));
	if (error != 0)
		return error;

	retval[0] = 0;
	return 0;
}
