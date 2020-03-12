/*	$NetBSD: netbsd32_quota.c,v 1.3 2020/03/12 15:02:29 pgoyette Exp $	*/

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
 * from: NetBSD: netbsd32_netbsd.c,v 1.218 2018/08/10 21:44:58 pgoyette Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_quota.c,v 1.3 2020/03/12 15:02:29 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/quotactl.h>
#include <sys/module.h>
#include <sys/vfs_syscalls.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#ifdef QUOTA
int
netbsd32___quotactl(struct lwp *l, const struct netbsd32___quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_voidp) args;
	} */
	struct netbsd32_quotactlargs args32;
	struct quotactl_args args;
	int error;

	error = copyin(SCARG_P32(uap, args), &args32, sizeof(args32));
	if (error) {
		return error;
	}

	args.qc_op = args32.qc_op;
	switch (args.qc_op) {
	    case QUOTACTL_STAT:
		args.u.stat.qc_info = NETBSD32PTR64(args32.u.stat.qc_info);
		break;
	    case QUOTACTL_IDTYPESTAT:
		args.u.idtypestat.qc_idtype = args32.u.idtypestat.qc_idtype;
		args.u.idtypestat.qc_info =
			NETBSD32PTR64(args32.u.idtypestat.qc_info);
		break;
	    case QUOTACTL_OBJTYPESTAT:
		args.u.objtypestat.qc_objtype =
			args32.u.objtypestat.qc_objtype;
		args.u.objtypestat.qc_info =
			NETBSD32PTR64(args32.u.objtypestat.qc_info);
		break;
	    case QUOTACTL_GET:
		args.u.get.qc_key = NETBSD32PTR64(args32.u.get.qc_key);
		args.u.get.qc_val = NETBSD32PTR64(args32.u.get.qc_val);
		break;
	    case QUOTACTL_PUT:
		args.u.put.qc_key = NETBSD32PTR64(args32.u.put.qc_key);
		args.u.put.qc_val = NETBSD32PTR64(args32.u.put.qc_val);
		break;
	    case QUOTACTL_DEL:
		args.u.del.qc_key = NETBSD32PTR64(args32.u.del.qc_key);
		break;
	    case QUOTACTL_CURSOROPEN:
		args.u.cursoropen.qc_cursor =
			NETBSD32PTR64(args32.u.cursoropen.qc_cursor);
		break;
	    case QUOTACTL_CURSORCLOSE:
		args.u.cursorclose.qc_cursor =
			NETBSD32PTR64(args32.u.cursorclose.qc_cursor);
		break;
	    case QUOTACTL_CURSORSKIPIDTYPE:
		args.u.cursorskipidtype.qc_cursor =
			NETBSD32PTR64(args32.u.cursorskipidtype.qc_cursor);
		args.u.cursorskipidtype.qc_idtype =
			args32.u.cursorskipidtype.qc_idtype;
		break;
	    case QUOTACTL_CURSORGET:
		args.u.cursorget.qc_cursor =
			NETBSD32PTR64(args32.u.cursorget.qc_cursor);
		args.u.cursorget.qc_keys =
			NETBSD32PTR64(args32.u.cursorget.qc_keys);
		args.u.cursorget.qc_vals =
			NETBSD32PTR64(args32.u.cursorget.qc_vals);
		args.u.cursorget.qc_maxnum =
			args32.u.cursorget.qc_maxnum;
		args.u.cursorget.qc_ret =
			NETBSD32PTR64(args32.u.cursorget.qc_ret);
		break;
	    case QUOTACTL_CURSORATEND:
		args.u.cursoratend.qc_cursor =
			NETBSD32PTR64(args32.u.cursoratend.qc_cursor);
		args.u.cursoratend.qc_ret =
			NETBSD32PTR64(args32.u.cursoratend.qc_ret);
		break;
	    case QUOTACTL_CURSORREWIND:
		args.u.cursorrewind.qc_cursor =
			NETBSD32PTR64(args32.u.cursorrewind.qc_cursor);
		break;
	    case QUOTACTL_QUOTAON:
		args.u.quotaon.qc_idtype = args32.u.quotaon.qc_idtype;
		args.u.quotaon.qc_quotafile =
			NETBSD32PTR64(args32.u.quotaon.qc_quotafile);
		break;
	    case QUOTACTL_QUOTAOFF:
		args.u.quotaoff.qc_idtype = args32.u.quotaoff.qc_idtype;
		break;
	    default:
		return EINVAL;
	}

	return do_sys_quotactl(SCARG_P32(uap, path), &args);
}
static struct syscall_package compat_netbsd32_quota_syscalls[] = {
	{ NETBSD32_SYS_netbsd32___quotactl, 0,
	    (sy_call_t *)netbsd32___quotactl },
	{ 0, 0, NULL }
};

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_quota, "compat_netbsd32");

static int
compat_netbsd32_quota_modcmd(modcmd_t cmd, void *arg)
{
	int ret;

	switch (cmd) {
	case MODULE_CMD_INIT:
		ret = syscall_establish(&emul_netbsd32,
		    compat_netbsd32_quota_syscalls);
		return ret;
	case MODULE_CMD_FINI:
		ret = syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_quota_syscalls);
		return ret;
	default:
		return ENOTTY;
	}
}
#endif
