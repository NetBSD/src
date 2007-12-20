/*	$NetBSD: irix_sysmp.c,v 1.20 2007/12/20 23:02:51 dsl Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: irix_sysmp.c,v 1.20 2007/12/20 23:02:51 dsl Exp $");

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>

#include <compat/common/compat_util.h>

#include <compat/svr4/svr4_types.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_sysmp.h>
#include <compat/irix/irix_syscallargs.h>

/* IRIX /dev/kmem diggers emulation */
static int irix_sysmp_kernaddr(int, register_t *);
static int irix_sysmp_sasz(int, register_t *);
static int irix_sysmp_saget(int, char *, size_t);
extern struct loadavg averunnable;
extern long irix_kernel_var[32];

int
irix_sys_sysmp(struct lwp *l, const struct irix_sys_sysmp_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(void *) arg1;
		syscallarg(void *) arg2;
		syscallarg(void *) arg3;
		syscallarg(void *) arg4;
	} */
	int cmd = SCARG(uap, cmd);

#ifdef DEBUG_IRIX
	printf("irix_sys_sysmp(): cmd = %d\n", cmd);
#endif

	switch(cmd) {
	case IRIX_MP_NPROCS:	/* Number of processors in complex */
	case IRIX_MP_NAPROCS: {	/* Number of active processors in complex */
		*retval = ncpu;
		return 0;
		break;
	}
	case IRIX_MP_PGSIZE:	/* Page size */
		*retval = (register_t)PAGE_SIZE;
		break;

	case IRIX_MP_KERNADDR: 	/* Kernel structure addresses */
		return irix_sysmp_kernaddr((int)SCARG(uap, arg1), retval);
		break;

	case IRIX_MP_SASZ: 	/* System accounting structure size */
		return irix_sysmp_sasz((int)SCARG(uap, arg1), retval);
		break;

	case IRIX_MP_SAGET1: /* Get system accounting structure for one CPU */
	case IRIX_MP_SAGET:  /* Get system accounting structure for all CPU */
		return irix_sysmp_saget((int)SCARG(uap, arg1),
		    (char *)SCARG(uap, arg2), (size_t)SCARG(uap, arg3));
		break;

	default:
		printf("Warning: call to unimplemented sysmp() command %d\n",
		    cmd);
		return EINVAL;
		break;
	}
	return 0;
}

static int
irix_sysmp_kernaddr(int kernaddr, register_t *retval)
{
	switch (kernaddr) {
	case IRIX_MPKA_AVENRUN:
		*retval = (register_t)&averunnable;
		break;

	case IRIX_MPKA_VAR:
		*retval = (register_t)&irix_kernel_var;
		break;

	default:
		printf("Warning: sysmp(KERNADDR) unimplemented address %d\n",
		    kernaddr);
		return EINVAL;
		break;
	}

	return 0;
}

static int
irix_sysmp_sasz(int cmd, register_t *retval)
{
	switch (cmd) {
	case IRIX_MPSA_RMINFO:
		*retval = sizeof(struct irix_sysmp_rminfo);
		break;
	default:
		printf("Warning: sysmp(SASZ) unimplemented struct %d\n",
		    cmd);
		return EINVAL;
		break;
	}
	return 0;
}

static int
irix_sysmp_saget(int cmd, char *buf, size_t len)
{
	extern u_int bufpages;
	void *kbuf;
	int error = 0;

	kbuf = malloc(len, M_TEMP, M_WAITOK);

	switch (cmd) {
	case IRIX_MPSA_RMINFO: {
		struct irix_sysmp_rminfo *irm =
		    (struct irix_sysmp_rminfo *)kbuf;
		int active, inactive;

		uvm_estimatepageable(&active, &inactive);
		irm->freemem = uvmexp.free + uvmexp.filepages;
		irm->availsmem = uvmexp.free + active + inactive
		    + uvmexp.wired + (uvmexp.swpages - uvmexp.swpgonly);
		irm->availrmem = uvmexp.free + active + inactive + uvmexp.wired;
		irm->bufmem = bufpages;
		irm->physmem = uvmexp.npages;
		irm->dchunkpages = 0; /* unsupported */
		irm->pmapmem = 0;     /* unsupported */
		irm->strmem = 0;      /* unsupported */
		irm->chunkpages = 0;  /* unsupported */
		irm->dpages = 0;      /* unsupported */
		irm->emptymem = uvmexp.free;
		irm->ravailrmem = active + inactive + uvmexp.free;
		break;
	}
	default:
		printf("Warning: sysmp(SAGET) unimplemented struct %d\n",
		    cmd);
		error = EINVAL;
		break;
	}

	if (error == 0)
		error = copyout(kbuf, buf, len);

	free(kbuf, M_TEMP);
	return error;
}
