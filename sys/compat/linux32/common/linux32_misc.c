/*	$NetBSD: linux32_misc.c,v 1.9.10.3 2009/06/20 07:20:17 yamt Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz; by Jason R. Thorpe
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research Center;
 * by Edgar Fu\ss, Mathematisches Institut der Uni Bonn.
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
__KERNEL_RCSID(0, "$NetBSD: linux32_misc.c,v 1.9.10.3 2009/06/20 07:20:17 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/fstypes.h>
#include <sys/vfs_syscalls.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/linux32_syscallargs.h>

#include <compat/linux/common/linux_ptrace.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_statfs.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

extern const struct linux_mnttypes linux_fstypes[];
extern const int linux_fstypes_cnt;


/*
 * Implement the fs stat functions. Straightforward.
 */
int
linux32_sys_statfs(struct lwp *l, const struct linux32_sys_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp char) path;
		syscallarg(linux32_statfsp) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG_P32(uap, path), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs(sb, &ltmp);
		error = copyout(&ltmp, SCARG_P32(uap, sp), sizeof ltmp);
	}

	STATVFSBUF_PUT(sb);
	return error;
}

int
linux32_sys_fstatfs(struct lwp *l, const struct linux32_sys_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(linux32_statfsp) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs(sb, &ltmp);
		error = copyout(&ltmp, SCARG_P32(uap, sp), sizeof ltmp);
	}
	STATVFSBUF_PUT(sb);

	return error;
}

extern const int linux_ptrace_request_map[];

int
linux32_sys_ptrace(struct lwp *l, const struct linux32_sys_ptrace_args *uap, register_t *retval)
{
	/* {
		i386, m68k, powerpc: T=int
		alpha, amd64: T=long
		syscallarg(T) request;
		syscallarg(T) pid;
		syscallarg(T) addr;
		syscallarg(T) data;
	} */
	const int *ptr;
	int request;
	int error;

	ptr = linux_ptrace_request_map;
	request = SCARG(uap, request);
	while (*ptr != -1)
		if (*ptr++ == request) {
			struct sys_ptrace_args pta;

			SCARG(&pta, req) = *ptr;
			SCARG(&pta, pid) = SCARG(uap, pid);
			SCARG(&pta, addr) = NETBSD32IPTR64(SCARG(uap, addr));
			SCARG(&pta, data) = SCARG(uap, data);

			/*
			 * Linux ptrace(PTRACE_CONT, pid, 0, 0) means actually
			 * to continue where the process left off previously.
			 * The same thing is achieved by addr == (void *) 1
			 * on NetBSD, so rewrite 'addr' appropriately.
			 */
			if (request == LINUX_PTRACE_CONT && SCARG(uap, addr)==0)
				SCARG(&pta, addr) = (void *) 1;

			error = sysent[SYS_ptrace].sy_call(l, &pta, retval);
			if (error)
				return error;
			switch (request) {
			case LINUX_PTRACE_PEEKTEXT:
			case LINUX_PTRACE_PEEKDATA:
				error = copyout (retval,
				    NETBSD32IPTR64(SCARG(uap, data)), 
				    sizeof *retval);
				*retval = SCARG(uap, data);
				break;
			default:
				break;
			}
			return error;
		}
		else
			ptr++;

	return EIO;
}

int
linux32_sys_personality(struct lwp *l, const struct linux32_sys_personality_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) per;
	} */

	switch (SCARG(uap, per)) {
	case LINUX_PER_LINUX:
	case LINUX_PER_LINUX32:
	case LINUX_PER_QUERY:
		break;
	default:
		return EINVAL;
	}

	retval[0] = LINUX_PER_LINUX;
	return 0;
}
