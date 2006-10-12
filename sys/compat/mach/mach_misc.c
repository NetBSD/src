/*	$NetBSD: mach_misc.c,v 1.23 2006/10/12 01:30:49 christos Exp $	 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * MACH compatibility module.
 *
 * We actually don't implement anything here yet!
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_misc.c,v 1.23 2006/10/12 01:30:49 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/socket.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/times.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/ptrace.h>
#include <sys/signalvar.h>

#include <netinet/in.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_syscallargs.h>


int
mach_sys_semaphore_timedwait_trap(struct lwp *l __unused, void *v,
    register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_semaphore_timedwait_trap_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_semaphore_timedwait_trap(0x%x, %d, %d);\n",
	    SCARG(ap, wait_name), SCARG(ap, sec), SCARG(ap, nsec)));
	return 0;
}


int
mach_sys_semaphore_timedwait_signal_trap(struct lwp *l __unused, void *v,
    register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_semaphore_timedwait_signal_trap_args *ap = v;
#endif
	*r = 0;
	DPRINTF((
	    "mach_sys_semaphore_timedwait_signal_trap(0x%x, 0x%x, %d, %d);\n",
	    SCARG(ap, wait_name), SCARG(ap, signal_name), SCARG(ap, sec),
	    SCARG(ap, nsec)));
	return 0;
}


int
mach_sys_init_process(struct lwp *l __unused, void *v __unused, register_t *r)
{
	*r = 0;
	DPRINTF(("mach_sys_init_process();\n"));
	return 0;
}


int
mach_sys_pid_for_task(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_pid_for_task_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_pid_for_task(0x%x, %p);\n",
	    SCARG(ap, t), SCARG(ap, x)));
	return 0;
}


int
mach_sys_macx_swapon(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_macx_swapon_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_macx_swapon(%p, %d, %d, %d);\n",
	    SCARG(ap, name), SCARG(ap, flags), SCARG(ap, size),
	    SCARG(ap, priority)));
	return 0;
}

int
mach_sys_macx_swapoff(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_macx_swapoff_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_macx_swapoff(%p, %d);\n",
	    SCARG(ap, name), SCARG(ap, flags)));
	return 0;
}

int
mach_sys_macx_triggers(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_macx_triggers_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_macx_triggers(%d, %d, %d, 0x%x);\n",
	    SCARG(ap, hi_water), SCARG(ap, low_water), SCARG(ap, flags),
	    SCARG(ap, alert_port)));
	return 0;
}


int
mach_sys_wait_until(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_wait_until_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_wait_until(%lld);\n",
	    SCARG(ap, deadline)));
	return 0;
}


int
mach_sys_timer_create(struct lwp *l __unused, void *v __unused, register_t *r)
{
	*r = 0;
	DPRINTF(("mach_sys_timer_create();\n"));
	return 0;
}


int
mach_sys_timer_destroy(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_timer_destroy_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_timer_destroy(0x%x);\n", SCARG(ap, name)));
	return 0;
}


int
mach_sys_timer_arm(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_timer_arm_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_timer_arm(0x%x, %d);\n",
	    SCARG(ap, name), SCARG(ap, expire_time)));
	return 0;
}


int
mach_sys_timer_cancel(struct lwp *l __unused, void *v, register_t *r)
{
#ifdef DEBUG_MACH
	struct mach_sys_timer_cancel_args *ap = v;
#endif
	*r = 0;
	DPRINTF(("mach_sys_timer_cancel(0x%x, %p);\n",
	    SCARG(ap, name), SCARG(ap, result_time)));
	return 0;
}


int
mach_sys_get_time_base_info(struct lwp *l __unused, void *v __unused,
    register_t *r)
{
	*r = 0;
	DPRINTF(("mach_sys_get_time_base_info();\n"));
	return 0;
}
