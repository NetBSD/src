/*	$NetBSD: linux32_missing.h,v 1.2.6.1 2007/03/12 05:52:29 rmind Exp $ */

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
#ifndef _I386_LINUX32_MISSING_H
#define _I386_LINUX32_MISSING_H

#include <compat/netbsd32/netbsd32.h>
#include <compat/linux/common/linux_types.h>

/* 
 * system calls not defined for COMPAT_LINUX/amd64 
 * but needed for COMPAT_LINUX32 
 */
struct linux_sys_old_mmap_args {
	syscallarg(struct linux_oldmmap *) lmp;
};  
struct linux_sys_ugetrlimit_args {
	syscallarg(int) which;
	syscallarg(struct orlimit *) rlp;
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

struct linux_sys_setresgid16_args {
        syscallarg(gid_t) rgid;
        syscallarg(gid_t) egid;
        syscallarg(gid_t) sgid;
};

struct linux_sys_setresuid16_args {
        syscallarg(uid_t) ruid;
        syscallarg(uid_t) euid;
        syscallarg(uid_t) suid;
};

struct linux_sys_nice_args {
	syscallarg(int) incr;
};

struct linux_sys_getgroups16_args {
        syscallarg(int) gidsetsize;
        syscallarg(linux_gid_t *) gidset;
};

struct linux_sys_setgroups16_args {
        syscallarg(int) gidsetsize;
        syscallarg(linux_gid_t *) gidset;
};

struct linux_sys_mmap2_args {
	syscallarg(unsigned long) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(linux_off_t) offset;	
};

#ifdef _KERNEL
__BEGIN_DECLS
int linux_sys_old_mmap(struct lwp *, void *, register_t *);
int linux_sys_ugetrlimit(struct lwp *, void *, register_t *);
int linux_sys_fcntl64(struct lwp *, void *, register_t *);
int linux_sys_llseek(struct lwp *, void *, register_t *);
int linux_sys_setresuid16(struct lwp *, void *, register_t *);
int linux_sys_setresgid16(struct lwp *, void *, register_t *);
int linux_sys_nice(struct lwp *, void *, register_t *);
int linux_sys_getgroups16(struct lwp *, void *, register_t *);
int linux_sys_setgroups16(struct lwp *, void *, register_t *);
int linux_sys_mmap2(struct lwp *, void *, register_t *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _I386_LINUX32_MISSING_H */
