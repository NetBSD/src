/*	$NetBSD: rump_private.h,v 1.24.2.1 2009/05/13 17:22:58 jym Exp $	*/

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

#ifndef _SYS_RUMP_PRIVATE_H_
#define _SYS_RUMP_PRIVATE_H_

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

extern kauth_cred_t rump_cred;
extern struct vmspace rump_vmspace;

extern struct rumpuser_mtx *rump_giantlock;

#define UIO_VMSPACE_SYS (&rump_vmspace)

#define RUMP_LMUTEX_MAGIC ((kmutex_t *)0x101)

extern int rump_threads;

extern struct sysent rump_sysent[];

void		rumpvm_init(void);
void		rump_sleepers_init(void);
struct vm_page	*rumpvm_makepage(struct uvm_object *, voff_t);

void		rumpvm_enterva(vaddr_t addr, struct vm_page *);
void		rumpvm_flushva(struct uvm_object *);

void		rump_gettime(struct timespec *);
void		rump_getuptime(struct timespec *);

lwpid_t		rump_nextlid(void);
void		rump_set_vmspace(struct vmspace *);

typedef void	(*rump_proc_vfs_init_fn)(struct proc *);
typedef void	(*rump_proc_vfs_release_fn)(struct proc *);
rump_proc_vfs_init_fn rump_proc_vfs_init;
rump_proc_vfs_release_fn rump_proc_vfs_release;

extern struct cpu_info rump_cpu;

extern rump_sysproxy_t rump_sysproxy;
extern void *rump_sysproxy_arg;

int		rump_sysproxy_copyout(const void *, void *, size_t);
int		rump_sysproxy_copyin(const void *, void *, size_t);

#endif /* _SYS_RUMP_PRIVATE_H_ */
