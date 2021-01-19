/*	$NetBSD: netbsd32_vm.c,v 1.3 2021/01/19 01:47:58 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_vm.c,v 1.3 2021/01/19 01:47:58 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/filedesc.h>
#include <sys/vfs_syscalls.h>

#include <compat/sys/mman.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>


int
netbsd32_mmap(struct lwp *l, const struct netbsd32_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) PAD;
		syscallarg(netbsd32_off_t) pos;
	} */
	struct sys_mmap_args ua;
	int error;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
#ifdef __x86_64__
	/*
	 * Ancient kernel on x86 did not obey PROT_EXEC on i386 at least
	 * and ld.so did not turn it on!
	 */
	if (SCARG(&ua, flags) & COMPAT_MAP_COPY) {
		SCARG(&ua, flags) = MAP_PRIVATE
		    | (SCARG(&ua, flags) & ~COMPAT_MAP_COPY);
		SCARG(&ua, prot) |= PROT_EXEC;
	}
#endif
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(PAD, long);
	NETBSD32TOX_UAP(pos, off_t);
#ifdef DEBUG_MMAP
	printf("mmap(addr=0x%lx, len=0x%lx, prot=0x%lx, flags=0x%lx, "
	    "fd=%ld, pos=0x%lx);\n",
	    (long)SCARG(&ua, addr), (long)SCARG(&ua, len),
	    (long)SCARG(&ua, prot), (long)SCARG(&ua, flags),
	    (long)SCARG(&ua, fd), (long)SCARG(&ua, pos));
#endif

	error = sys_mmap(l, &ua, retval);
#ifdef DEBUG_MMAP
	printf("mmap error = %d  *retval = %#"PRIxREGISTER"\n", error, *retval);
#endif
	if (error == 0 && (u_long)*retval > (u_long)UINT_MAX) {
		const char *name = curlwp->l_name;

		if (name == NULL)
			name = curproc->p_comm;
		printf("netbsd32_mmap(%s:%u:%u): retval out of range: 0x%lx\n",
		    name, curproc->p_pid, curlwp->l_lid, (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return error;
}
