/*	$NetBSD: linux32_misc.c,v 1.20 2010/11/02 18:14:06 chs Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux32_misc.c,v 1.20 2010/11/02 18:14:06 chs Exp $");

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
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_statfs.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/linux_syscallargs.h>

extern const struct linux_mnttypes linux_fstypes[];
extern const int linux_fstypes_cnt;

void linux32_to_native_timespec(struct timespec *, struct linux32_timespec *);

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

int
linux32_sys_statfs64(struct lwp *l, const struct linux32_sys_statfs64_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp char) path;
		syscallarg(linux32_statfs64p) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs64 ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG_P32(uap, path), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs64(sb, &ltmp);
		error = copyout(&ltmp, SCARG_P32(uap, sp), sizeof ltmp);
	}

	STATVFSBUF_PUT(sb);
	return error;
}

int
linux32_sys_fstatfs64(struct lwp *l, const struct linux32_sys_fstatfs64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(linux32_statfs64p) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs64 ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs64(sb, &ltmp);
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
		syscallarg(netbsd32_u_long) per;
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

int
linux32_sys_futex(struct lwp *l,
    const struct linux32_sys_futex_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_intp_t) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(linux32_timespecp_t) timeout;
		syscallarg(linux32_intp_t) uaddr2;
		syscallarg(int) val3;
	} */
	struct linux_sys_futex_args ua;
	struct linux32_timespec lts;
	struct timespec ts = { 0, 0 };
	int error;

	NETBSD32TOP_UAP(uaddr, int);
	NETBSD32TO64_UAP(op);
	NETBSD32TO64_UAP(val);
	NETBSD32TOP_UAP(timeout, struct linux_timespec);
	NETBSD32TOP_UAP(uaddr2, int);
	NETBSD32TO64_UAP(val3);
	if ((SCARG(uap, op) & ~LINUX_FUTEX_PRIVATE_FLAG) == LINUX_FUTEX_WAIT &&
	    SCARG_P32(uap, timeout) != NULL) {
		if ((error = copyin((void *)SCARG_P32(uap, timeout), 
		    &lts, sizeof(lts))) != 0) {
			return error;
		}
		linux32_to_native_timespec(&ts, &lts);
	}
	return linux_do_futex(l, &ua, retval, &ts);
}

int
linux32_sys_set_robust_list(struct lwp *l,
    const struct linux32_sys_set_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_robust_list_headp_t) head;
		syscallarg(linux32_size_t) len;
	} */
	struct linux_sys_set_robust_list_args ua;
	struct linux_emuldata *led;

	if (SCARG(uap, len) != 12)
		return EINVAL;

	NETBSD32TOP_UAP(head, struct robust_list_head);
	NETBSD32TOX64_UAP(len, size_t);

	led = l->l_emuldata;
	led->led_robust_head = SCARG(&ua, head);
	*retval = 0;
	return 0;
}

int
linux32_sys_get_robust_list(struct lwp *l,
    const struct linux32_sys_get_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_robust_list_headpp_t) head;
		syscallarg(linux32_sizep_t) len;
	} */
	struct linux_sys_get_robust_list_args ua;

	NETBSD32TOP_UAP(head, struct robust_list_head *);
	NETBSD32TOP_UAP(len, size_t *);
	return linux_sys_get_robust_list(l, &ua, retval);
}

int
linux32_sys_truncate64(struct lwp *l, const struct linux32_sys_truncate64_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) path;
		syscallarg(off_t) length;
	} */
	struct sys_truncate_args ua;

	/* Linux doesn't have the 'pad' pseudo-parameter */
	NETBSD32TOP_UAP(path, const char *);
	SCARG(&ua, PAD) = 0;
	SCARG(&ua, length) = ((off_t)SCARG(uap, lenhi) << 32) + SCARG(uap, lenlo);
	return sys_truncate(l, &ua, retval);
}

int
linux32_sys_ftruncate64(struct lwp *l, const struct linux32_sys_ftruncate64_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned int) fd;
		syscallarg(off_t) length;
	} */
	struct sys_ftruncate_args ua;

	/* Linux doesn't have the 'pad' pseudo-parameter */
	NETBSD32TO64_UAP(fd);
	SCARG(&ua, PAD) = 0;
	SCARG(&ua, length) = ((off_t)SCARG(uap, lenhi) << 32) + SCARG(uap, lenlo);
	return sys_ftruncate(l, &ua, retval);
}

int
linux32_sys_setdomainname(struct lwp *l, const struct linux32_sys_setdomainname_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) domainname;
		syscallarg(int) len;
	} */
	struct linux_sys_setdomainname_args ua;

	NETBSD32TOP_UAP(domainname, char);
	NETBSD32TO64_UAP(len);
	return linux_sys_setdomainname(l, &ua, retval);
}
