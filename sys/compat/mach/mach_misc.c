/*	$NetBSD: mach_misc.c,v 1.2 2001/07/29 19:30:56 christos Exp $	 */

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
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_syscallargs.h>

#ifdef DEBUG_MACH
static void mach_print_msg_header_t(mach_msg_header_t *);

static void
mach_print_msg_header_t(mach_msg_header_t *mh) {
	uprintf("msgh { bits=0x%x, size=%d, remote_port=%d, local_port=%d,"
	    "reserved=%d,id=%d }\n",
	    mh->msgh_bits, mh->msgh_size, mh->msgh_remote_port,
	    mh->msgh_local_port, mh->msgh_reserved, mh->msgh_id);
}
#endif /* DEBUG_MACH */

int
mach_sys_reply_port(struct proc *p, void *vv, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_reply_port();\n"));
	return 0;
}

int
mach_sys_thread_self_trap(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_thread_self();\n"));
	return 0;
}


int
mach_sys_task_self_trap(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_task_self();\n"));
	return 0;
}


int
mach_sys_host_self_trap(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_host_self();\n"));
	return 0;
}


int
mach_sys_msg_overwrite_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_msg_overwrite_trap_args *ap = v;
	int error;
	*r = 0;

	DPRINTF(("mach_sys_msg_overwrite_trap(%p, 0x%x,"
	    " %d, %d, 0x%x, %d, 0x%x, %p, %d);\n",
	    SCARG(ap, msg), SCARG(ap, option), SCARG(ap, send_size),
	    SCARG(ap, rcv_size), SCARG(ap, rcv_name), SCARG(ap, timeout),
	    SCARG(ap, notify), SCARG(ap, rcv_msg),
	    SCARG(ap, scatter_list_size)));

	switch (SCARG(ap, option)) {
	case MACH_SEND_MSG|MACH_RCV_MSG:
		if (SCARG(ap, msg)) {
			mach_msg_header_t mh;
			if ((error = copyin(SCARG(ap, msg), &mh,
			    sizeof(mh))) != 0)
				return error;
#ifdef DEBUG_MACH
			mach_print_msg_header_t(&mh);
#endif /* DEBUG_MACH */
		}
		break;
	default:
		uprintf("unhandled sys_msg_override_trap option %x\n",
		    SCARG(ap, option));
		break;
	}
	return 0;
}


int
mach_sys_semaphore_signal_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_signal_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_signal_trap(0x%x);\n",
	    SCARG(ap, signal_name)));
	return 0;
}


int
mach_sys_semaphore_signal_all_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_signal_all_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_signal_all_trap(0x%x);\n",
	    SCARG(ap, signal_name)));
	return 0;
}


int
mach_sys_semaphore_signal_thread_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_signal_thread_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_signal_thread_trap(0x%x);\n",
	    SCARG(ap, signal_name)));
	return 0;
}


int
mach_sys_semaphore_wait_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_wait_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_wait_trap(0x%x);\n",
	    SCARG(ap, wait_name)));
	return 0;
}


int
mach_sys_semaphore_wait_signal_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_wait_signal_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_wait_signal_trap(0x%x, 0x%x);\n",
	    SCARG(ap, wait_name), SCARG(ap, signal_name)));
	return 0;
}


int
mach_sys_semaphore_timedwait_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_semaphore_timedwait_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_semaphore_timedwait_trap(0x%x, %d, %d);\n",
	    SCARG(ap, wait_name), SCARG(ap, sec), SCARG(ap, nsec)));
	return 0;
}


int
mach_sys_semaphore_timedwait_signal_trap(struct proc *p, void *v, register_t *r)
{
	struct mach_sys_semaphore_timedwait_signal_trap_args *ap = v;
	*r = 0;
	DPRINTF((
	    "mach_sys_semaphore_timedwait_signal_trap(0x%x, 0x%x, %d, %d);\n",
	    SCARG(ap, wait_name), SCARG(ap, signal_name), SCARG(ap, sec),
	    SCARG(ap, nsec)));
	return 0;
}


int
mach_sys_init_process(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_init_process();\n"));
	return 0;
}


int
mach_sys_map_fd(struct proc *p, void *v, register_t *r) {
	struct mach_sys_map_fd_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_map_fd(0x%lx, %p, %d, %d);\n",
	    SCARG(ap, offset), SCARG(ap, va), SCARG(ap, findspace),
	    SCARG(ap, size)));
	return 0;
}


int
mach_sys_task_for_pid(struct proc *p, void *v, register_t *r) {
	struct mach_sys_task_for_pid_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_task_for_pid(0x%x, %d, %p);\n",
	    SCARG(ap, target_tport), SCARG(ap, pid), SCARG(ap, t)));
	return 0;
}


int
mach_sys_pid_for_task(struct proc *p, void *v, register_t *r) {
	struct mach_sys_pid_for_task_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_pid_for_task(0x%x, %p);\n",
	    SCARG(ap, t), SCARG(ap, x)));
	return 0;
}


int
mach_sys_macx_swapon(struct proc *p, void *v, register_t *r) {
	struct mach_sys_macx_swapon_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_macx_swapon(%p, %d, %d, %d);\n",
	    SCARG(ap, name), SCARG(ap, flags), SCARG(ap, size),
	    SCARG(ap, priority)));
	return 0;
}

int
mach_sys_macx_swapoff(struct proc *p, void *v, register_t *r) {
	struct mach_sys_macx_swapoff_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_macx_swapoff(%p, %d);\n",
	    SCARG(ap, name), SCARG(ap, flags)));
	return 0;
}

int
mach_sys_macx_triggers(struct proc *p, void *v, register_t *r) {
	struct mach_sys_macx_triggers_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_macx_triggers(%d, %d, %d, 0x%x);\n",
	    SCARG(ap, hi_water), SCARG(ap, low_water), SCARG(ap, flags),
	    SCARG(ap, alert_port)));
	return 0;
}


int
mach_sys_swtch_pri(struct proc *p, void *v, register_t *r) {
	struct mach_sys_swtch_pri_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_swtch_pri(%d);\n",
	    SCARG(ap, pri)));
	return 0;
}


int
mach_sys_swtch(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_swtch();\n"));
	return 0;
}


int
mach_sys_syscall_thread_switch(struct proc *p, void *v, register_t *r) {
	struct mach_sys_syscall_thread_switch_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_syscall_thread_switch(0x%x, %d, %d);\n",
	    SCARG(ap, thread_name), SCARG(ap, option), SCARG(ap, option_time)));
	return 0;
}


int
mach_sys_clock_sleep_trap(struct proc *p, void *v, register_t *r) {
	struct mach_sys_clock_sleep_trap_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_sleep_trap(0x%x, %d, %d, %d, %p);\n",
	    SCARG(ap, clock_name), SCARG(ap, sleep_type),
	    SCARG(ap, sleep_sec), SCARG(ap, sleep_nsec),
	    SCARG(ap, wakeup_time)));
	return 0;
}


int
mach_sys_timebase_info(struct proc *p, void *v, register_t *r) {
	struct mach_sys_timebase_info_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_timebase_info(%p);\n",
	    &SCARG(ap, info)));
	return 0;
}


int
mach_sys_wait_until(struct proc *p, void *v, register_t *r) {
	struct mach_sys_wait_until_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_wait_until(%lld);\n",
	    SCARG(ap, deadline)));
	return 0;
}


int
mach_sys_timer_create(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_timer_create();\n"));
	return 0;
}


int
mach_sys_timer_destroy(struct proc *p, void *v, register_t *r) {
	struct mach_sys_timer_destroy_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_timer_destroy(0x%x);\n", SCARG(ap, name)));
	return 0;
}


int
mach_sys_timer_arm(struct proc *p, void *v, register_t *r) {
	struct mach_sys_timer_arm_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_timer_arm(0x%x, %d);\n",
	    SCARG(ap, name), SCARG(ap, expire_time)));
	return 0;
}


int
mach_sys_timer_cancel(struct proc *p, void *v, register_t *r) {
	struct mach_sys_timer_cancel_args *ap = v;
	*r = 0;
	DPRINTF(("mach_sys_timer_cancel(0x%x, %p);\n",
	    SCARG(ap, name), SCARG(ap, result_time)));
	return 0;
}


int
mach_sys_get_time_base_info(struct proc *p, void *v, register_t *r) {
	*r = 0;
	DPRINTF(("mach_sys_get_time_base_info();\n"));
	return 0;
}
