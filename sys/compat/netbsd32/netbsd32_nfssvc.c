/*	$NetBSD: netbsd32_nfssvc.c,v 1.5.14.2 2017/12/03 11:36:56 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_nfssvc.c,v 1.5.14.2 2017/12/03 11:36:56 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_nfs.h"
#include "opt_nfsserver.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/module.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfs_var.h>

extern struct emul emul_netbsd32;

static int nfssvc32_addsock_in(struct nfsd_args *, const void *);
static int nfssvc32_setexports_in(struct mountd_exports_list *, const void *);
static int nfssvc32_nsd_in(struct nfsd_srvargs *, const void *);
static int nfssvc32_nsd_out(void *, const struct nfsd_srvargs *);
static int nfssvc32_exp_in(struct export_args *, const void *, size_t);

static int
nfssvc32_addsock_in(struct nfsd_args *nfsdarg, const void *argp)
{
	struct netbsd32_nfsd_args args32;
	int error;

	error = copyin(argp, &args32, sizeof args32);
	if (!error) {
		nfsdarg->sock = args32.sock;
		nfsdarg->name = NETBSD32PTR64(args32.name);
		nfsdarg->namelen = args32.namelen;
	}

	return error;
}

static int
nfssvc32_setexports_in(struct mountd_exports_list *mel, const void *argp)
{
	struct netbsd32_mountd_exports_list args32;
	int error;

	error = copyin(argp, &args32, sizeof args32);
	if (!error) {
		mel->mel_path = NETBSD32PTR64(args32.mel_path);
		mel->mel_nexports = args32.mel_nexports;
		mel->mel_exports = NETBSD32PTR64(args32.mel_exports);
	}

	return error;
}

static int
nfssvc32_nsd_in(struct nfsd_srvargs *nsd, const void *argp)
{
	struct netbsd32_nfsd_srvargs args32;
	int error;

	error = copyin(argp, &args32, sizeof args32);
	if (!error) {
		nsd->nsd_nfsd = NETBSD32PTR64(args32.nsd_nfsd);
		nsd->nsd_uid = args32.nsd_uid;
		nsd->nsd_haddr = args32.nsd_haddr;
		nsd->nsd_cr = args32.nsd_cr;
		nsd->nsd_authlen = args32.nsd_authlen;
		nsd->nsd_authstr = NETBSD32PTR64(args32.nsd_authstr);
		nsd->nsd_verflen = args32.nsd_verflen;
		nsd->nsd_uid = args32.nsd_uid;
		netbsd32_to_timeval(&args32.nsd_timestamp, &nsd->nsd_timestamp);
		nsd->nsd_ttl = args32.nsd_ttl;
		nsd->nsd_key[0] = args32.nsd_key[0];
		nsd->nsd_key[1] = args32.nsd_key[1];
	}

	return error;
}

static int
nfssvc32_nsd_out(void *argp, const struct nfsd_srvargs *nsd)
{
	struct netbsd32_nfsd_srvargs args32;

	NETBSD32PTR32(args32.nsd_nfsd, nsd->nsd_nfsd);
	args32.nsd_uid = nsd->nsd_uid;
	args32.nsd_haddr = nsd->nsd_haddr;
	args32.nsd_cr = nsd->nsd_cr;
	args32.nsd_authlen = nsd->nsd_authlen;
	NETBSD32PTR32(args32.nsd_authstr, nsd->nsd_authstr);
	args32.nsd_verflen = nsd->nsd_verflen;
	args32.nsd_uid = nsd->nsd_uid;
	netbsd32_from_timeval(&nsd->nsd_timestamp, &args32.nsd_timestamp);
	args32.nsd_ttl = nsd->nsd_ttl;
	args32.nsd_key[0] = nsd->nsd_key[0];
	args32.nsd_key[1] = nsd->nsd_key[1];

	return copyout(nsd, argp, sizeof *nsd);
}

static int
nfssvc32_exp_in(struct export_args *exp, const void *argp, size_t nexports)
{
	struct netbsd32_export_args exp32;
	struct netbsd32_export_args const *argp32 = argp;
	int error = 0;

	for (size_t i = 0; i < nexports; i++) {
		error = copyin(argp32, &exp32, sizeof exp32);
		if (error)
			break;
		exp->ex_flags = exp32.ex_flags;
		exp->ex_root = exp32.ex_root;
		exp->ex_anon = exp32.ex_anon;
		exp->ex_addr = NETBSD32PTR64(exp32.ex_addr);
		exp->ex_addrlen = exp32.ex_addrlen;
		exp->ex_mask = NETBSD32PTR64(exp32.ex_mask);
		exp->ex_masklen = exp32.ex_masklen;
		exp->ex_indexfile = NETBSD32PTR64(exp32.ex_indexfile);
		exp++;
		argp32++;
	}

	return error;
}

/*
 * NFS server system calls
 */

static struct nfssvc_copy_ops netbsd32_ops = {
	.addsock_in = nfssvc32_addsock_in,
	.setexports_in = nfssvc32_setexports_in,
	.nsd_in = nfssvc32_nsd_in,
	.nsd_out = nfssvc32_nsd_out,
	.exp_in = nfssvc32_exp_in,
};

int
netbsd32_nfssvc(struct lwp *l, const struct netbsd32_nfssvc_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) flag;
		syscallarg(netbsd32_voidp *) argp;
	} */
	int	flag = SCARG(uap, flag);
	void	*argp = SCARG_P32(uap, argp);

	return do_nfssvc(&netbsd32_ops, l, flag, argp, retval);
}

static const struct syscall_package compat_nfssvc_syscalls[] = {
	{ NETBSD32_SYS_netbsd32_nfssvc, 0, (sy_call_t *)netbsd32_nfssvc },
	{ 0, 0, NULL },
};

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_nfssrv, "nfsserver,compat_netbsd32");

static int
compat_netbsd32_nfssrv_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(&emul_netbsd32,
		    compat_nfssvc_syscalls);
		break;
	case MODULE_CMD_FINI:
		error = syscall_disestablish(&emul_netbsd32,
		    compat_nfssvc_syscalls);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
