/*	$NetBSD: rump_private.h,v 1.70.14.2 2013/02/25 00:30:08 tls Exp $	*/

/*
 * Copyright (c) 2007-2011 Antti Kantee.  All Rights Reserved.
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
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "rumpkern_if_priv.h"

extern struct rumpuser_mtx *rump_giantlock;

extern int rump_threads;
extern struct device rump_rootdev;

extern struct sysent rump_sysent[];

enum rump_component_type {
	RUMP_COMPONENT_DEV,
	RUMP_COMPONENT_NET,
		RUMP_COMPONENT_NET_ROUTE,
		RUMP_COMPONENT_NET_IF,
		RUMP_COMPONENT_NET_IFCFG,
	RUMP_COMPONENT_VFS,
	RUMP_COMPONENT_KERN,
		RUMP_COMPONENT_KERN_VFS,
	RUMP_COMPONENT_POSTINIT,

	RUMP__FACTION_DEV,
	RUMP__FACTION_VFS,
	RUMP__FACTION_NET,

	RUMP_COMPONENT_MAX,
};
struct rump_component {
	enum rump_component_type rc_type;
	void (*rc_init)(void);
};
#define RUMP_COMPONENT(type)				\
static void rumpcompinit##type(void);			\
static const struct rump_component rumpcomp##type = {	\
	.rc_type = type,				\
	.rc_init = rumpcompinit##type,			\
};							\
__link_set_add_rodata(rump_components, rumpcomp##type);	\
static void rumpcompinit##type(void)

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

#define RUMPMEM_UNLIMITED ((unsigned long)-1)
extern unsigned long rump_physmemlimit;

#define RUMP_LOCALPROC_P(p) (p->p_vmspace == vmspace_kernel())

void		rump_component_init(enum rump_component_type);
int		rump_component_count(enum rump_component_type);

typedef void	(*rump_proc_vfs_init_fn)(struct proc *);
typedef void	(*rump_proc_vfs_release_fn)(struct proc *);
extern rump_proc_vfs_init_fn rump_proc_vfs_init;
extern rump_proc_vfs_release_fn rump_proc_vfs_release;

extern struct cpu_info *rump_cpu;

extern bool rump_ttycomponent;

struct lwp *	rump__lwproc_alloclwp(struct proc *);

void	rump_cpus_bootstrap(int *);
void	rump_biglock_init(void);
void	rump_scheduler_init(int);
void	rump_schedule(void);
void	rump_unschedule(void);
void 	rump_schedule_cpu(struct lwp *);
void 	rump_schedule_cpu_interlock(struct lwp *, void *);
void	rump_unschedule_cpu(struct lwp *);
void	rump_unschedule_cpu_interlock(struct lwp *, void *);
void	rump_unschedule_cpu1(struct lwp *, void *);

void	rump_schedlock_cv_wait(struct rumpuser_cv *);
int	rump_schedlock_cv_timedwait(struct rumpuser_cv *,
				    const struct timespec *);

void	rump_user_schedule(int, void *);
void	rump_user_unschedule(int, int *, void *);

void	rump_cpu_attach(struct cpu_info *);

void	rump_kernel_bigwrap(int *);
void	rump_kernel_bigunwrap(int);

void	rump_tsleep_init(void);

void	rump_intr_init(int);
void	rump_softint_run(struct cpu_info *);

void	*rump_hypermalloc(size_t, int, bool, const char *);
void	rump_hyperfree(void *, size_t);

void	rump_xc_highpri(struct cpu_info *);

#endif /* _SYS_RUMP_PRIVATE_H_ */
