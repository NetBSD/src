/*	$NetBSD: cpu.h,v 1.47.2.1 2020/01/17 21:47:37 ad Exp $	*/

/*-
 * Copyright (c) 2007 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

#ifndef _LOCORE

#include <machine/cpu.h>

#include <sys/lwp.h>

struct cpu_info;

#ifdef _KERNEL
#ifndef cpu_idle
void cpu_idle(void);
#endif

#ifdef CPU_UCODE
#include <sys/cpuio.h>
#include <dev/firmload.h>
#ifdef COMPAT_60
#include <compat/sys/cpuio.h>
#endif
#endif

#ifndef cpu_need_resched
void cpu_need_resched(struct cpu_info *, struct lwp *, int);
#endif

/*
 * CPU_INFO_ITERATOR() may be supplied by machine dependent code as it
 * controls how the cpu_info structures are allocated.
 * 
 * This macro must always iterate just the boot-CPU when the system has
 * not attached any cpus via mi_cpu_attach() yet, and the "ncpu" variable
 * is zero.
 */
#ifndef CPU_INFO_ITERATOR
#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	\
    (void)cii, ci = curcpu(); ci != NULL; ci = NULL
#endif

#ifndef CPU_IS_PRIMARY
#define	CPU_IS_PRIMARY(ci)	((void)ci, 1)
#endif

#ifdef __HAVE_MD_CPU_OFFLINE
void	cpu_offline_md(void);
#endif

struct lwp *cpu_switchto(struct lwp *, struct lwp *, bool);
struct	cpu_info *cpu_lookup(u_int);
int	cpu_setmodel(const char *fmt, ...)	__printflike(1, 2);
const char *cpu_getmodel(void);
int	cpu_setstate(struct cpu_info *, bool);
int	cpu_setintr(struct cpu_info *, bool);
bool	cpu_intr_p(void);
bool	cpu_softintr_p(void);
bool	cpu_kpreempt_enter(uintptr_t, int);
void	cpu_kpreempt_exit(uintptr_t);
bool	cpu_kpreempt_disabled(void);
int	cpu_lwp_setprivate(struct lwp *, void *);
void	cpu_intr_redistribute(void);
u_int	cpu_intr_count(struct cpu_info *);
void	cpu_topology_set(struct cpu_info *, u_int, u_int, u_int, u_int, bool);
void	cpu_topology_init(void);
#endif

#ifdef _KERNEL
extern kmutex_t cpu_lock;
extern u_int maxcpus;
extern struct cpu_info **cpu_infos;
extern kcpuset_t *kcpuset_attached;
extern kcpuset_t *kcpuset_running;

static __inline u_int
cpu_index(const struct cpu_info *ci)
{
	return ci->ci_index;
}

static __inline char *
cpu_name(struct cpu_info *ci)
{
	return ci->ci_data.cpu_name;
}

#ifdef CPU_UCODE
struct cpu_ucode_softc {
	int loader_version;
	char *sc_blob;
	off_t sc_blobsize;
};

int cpu_ucode_get_version(struct cpu_ucode_version *);
int cpu_ucode_apply(const struct cpu_ucode *);
#ifdef COMPAT_60
int compat6_cpu_ucode_get_version(struct compat6_cpu_ucode *);
int compat6_cpu_ucode_apply(const struct compat6_cpu_ucode *);
#endif
int cpu_ucode_load(struct cpu_ucode_softc *, const char *);
int cpu_ucode_md_open(firmware_handle_t *, int, const char *);
#endif

#endif
#endif	/* !_LOCORE */

/*
 * Flags for cpu_need_resched.  RESCHED_KERNEL must be greater than
 * RESCHED_USER; see sched_resched_cpu().
 */
#define	RESCHED_REMOTE		0x01	/* request is for a remote CPU */
#define	RESCHED_IDLE		0x02	/* idle LWP observed */
#define	RESCHED_UPREEMPT	0x04	/* immediate user ctx switch */
#define	RESCHED_KPREEMPT	0x08	/* immediate kernel ctx switch */

#endif	/* !_SYS_CPU_H_ */
