/*	$NetBSD: linux32_missing.h,v 1.8.18.1 2009/07/23 23:31:42 jym Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _AMD64_LINUX32_MISSING_H
#define _AMD64_LINUX32_MISSING_H

#include <compat/netbsd32/netbsd32.h>
#include <compat/linux/common/linux_types.h>

/* 
 * system calls not defined for COMPAT_LINUX/amd64 
 * but needed for COMPAT_LINUX32 
 */
struct linux_sys_old_mmap_args {
	syscallarg(struct linux_oldmmap *) lmp;
};  

struct linux_sys_fcntl64_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};
struct linux_sys_llseek_args {
        syscallarg(int) fd; 
        syscallarg(u_int32_t) ohigh;  
        syscallarg(u_int32_t) olow;
        syscallarg(void *) res;
        syscallarg(int) whence;
};

struct linux_sys_getgroups16_args {
        syscallarg(int) gidsetsize;
        syscallarg(linux_gid16_t *) gidset;
};

struct linux_sys_setgroups16_args {
        syscallarg(int) gidsetsize;
        syscallarg(linux_gid16_t *) gidset;
};

#ifdef _KERNEL
__BEGIN_DECLS
#define SYS_DEF(foo) struct foo##_args; \
	int foo(lwp_t *, const struct foo##_args *, register_t *)
SYS_DEF(linux_sys_old_mmap);
SYS_DEF(linux_sys_fcntl64);
SYS_DEF(linux_sys_llseek);
SYS_DEF(linux_sys_getgroups16);
SYS_DEF(linux_sys_setgroups16);
#undef SYS_DEF
__END_DECLS
#endif /* !_KERNEL */

#endif /* _AMD64_LINUX32_MISSING_H */
