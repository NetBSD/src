/*	$NetBSD: mach_message.c,v 1.4 2002/12/17 18:42:57 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_message.c,v 1.4 2002/12/17 18:42:57 manu Exp $");

#include "opt_ktrace.h"
#include "opt_compat_mach.h" /* For COMPAT_MACH in <sys/ktrace.h> */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif 

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_syscallargs.h>

/* Mach message pool */
static struct pool mach_message_pool;

int
mach_sys_msg_overwrite_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct mach_sys_msg_overwrite_trap_args /* {
		syscallarg(mach_msg_header_t *) msg;
		syscallarg(mach_msg_option_t) option;
		syscallarg(mach_msg_size_t) send_size;
		syscallarg(mach_msg_size_t) rcv_size;
		syscallarg(mach_port_name_t) rcv_name;
		syscallarg(mach_msg_timeout_t) timeout;
		syscallarg(mach_port_name_t) notify;
		syscallarg(mach_msg_header_t *) rcv_msg;
		syscallarg(mach_msg_size_t) scatter_list_size;
	} */ *uap = v;
	int error = 0;
	struct mach_subsystem_namemap *map;
	struct mach_trap_args args;
	struct mach_emuldata *med;
	mach_msg_header_t *sm;
	mach_msg_header_t *rm;
	mach_msg_header_t *urm;
	size_t send_size, rcv_size;
	struct mach_port *mp;
	struct mach_message *mm;
	int timeout;

	/*
	 * Catch unhandled options 
	 */
	if (SCARG(uap, option) & ~(MACH_SEND_MSG | MACH_RCV_MSG))
		uprintf("mach_msg: unhandled option 0x%x\n", 
		    SCARG(uap, option));

	/* 
	 * XXX Sanity check on the message size. This is not an accurate
	 * emulation, since Mach messages can be as large as 4GB. 
	 * Additionnaly, this does not address DoS attack by queueing
	 * lots of big messages in the kernel.
	 */
	send_size = SCARG(uap, send_size);
	rcv_size = SCARG(uap, rcv_size);
	if ((send_size > MACH_MAX_MSG_LEN) || (rcv_size > MACH_MAX_MSG_LEN))
		return E2BIG;

	/* 
	 * Two options: receive or send. If both are 
	 * set, we must send, and then receive.
	 */
	if (SCARG(uap, option) & MACH_SEND_MSG) {
		if (SCARG(uap, msg) == NULL)
			return EINVAL;

		/* 
		 * Allocate memory for the message and its reply,
		 * and copy the whole message in the kernel.
		 */
		sm = malloc(send_size, M_EMULDATA, M_WAITOK);
		if ((error = copyin(SCARG(uap, msg), sm, send_size)) != 0) 
			goto out1;

#ifdef KTRACE
		/* Dump the Mach message */
		if (KTRPOINT(p, KTR_MMSG))
			ktrmmsg(p, (char *)sm, send_size); 
#endif

		/* 
		 * Check that the process has send a send right on 
		 * the remote port. 
		 */
		if (mach_right_check((struct mach_right *)sm->msgh_remote_port,
		    p, MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_SEND_ONCE) == 0) {
			error = EPERM;
			goto out1;
		}

		/*
		 * If the remote port is a special port (host, kernel or
		 * bootstrap), the message will be handled by the kernel.
		 */
		med = (struct mach_emuldata *)p->p_emuldata;
		mp = ((struct mach_right *)sm->msgh_remote_port)->mr_port;
		if ((mp == med->med_host) ||
		    (mp == med->med_kernel) ||
		    (mp == med->med_bootstrap)) {
			/* 
			 * Look for the function that will handle it,
			 * using the message id.
			 */
			for (map = mach_namemap; map->map_id; map++)
				if (map->map_id == sm->msgh_id)
					break;
			
			/* 
			 * If no match, give up, and display a warning.
			 */
			if (map->map_handler == NULL) {
				uprintf("No mach trap handler for id = %d\n",
				    sm->msgh_id);
				error = EINVAL;
				goto out3;
			}
#ifdef KTRACE
			/*
			 * It is convenient to record in kernel trace 
			 * the name of the handler that has been used,
			 * it makes traces easier to read. The user
			 * facility does not produce a perfect result,
			 * but at least we have the information.
			 */
			ktruser(p, map->map_name, NULL, 0, 0);
#endif
			/* 
			 * Invoke the handler. We give it the opportunity
			 * to shorten rcv_size if there is less data in
			 * the reply than what the sender expected.
			 */
			rm = malloc(rcv_size, M_EMULDATA, M_WAITOK | M_ZERO);

			args.p = p;
			args.smsg = sm;
			args.rmsg = rm;
			args.rsize = &rcv_size;
			if ((error = (*map->map_handler)(&args)) != 0)
				goto out2;
			
			/* 
			 * Catch potential bug in the handler
			 */
			if (rcv_size > SCARG(uap, rcv_size)) {
				uprintf("mach_msg: reply too big\n");
				rcv_size = SCARG(uap, rcv_size);
			}

			/* 
			 * copyout the reply message, and return
			 * without handling a potential receive operation
			 * since we already done everything.
			 */
			if (SCARG(uap, rcv_msg) != NULL)
				urm = SCARG(uap, rcv_msg);
			else
				urm = SCARG(uap, msg);
			if ((error = copyout(rm, urm, rcv_size)) != 0)
				goto out2;
#ifdef KTRACE
			/* Dump the Mach message */
			if (KTRPOINT(p, KTR_MMSG))
				ktrmmsg(p, (char *)rm, rcv_size); 
#endif

out2:			free(rm, M_EMULDATA);
out3:			free(sm, M_EMULDATA);

			return error;

		} else {

			/* 
			 * The message is not to be handled by the kernel. 
			 * Queue the message in the remote port, and wakeup 
			 * any process that would be sleeping for it.
		 	 */
			 mp = ((struct mach_right *)
			     sm->msgh_remote_port)->mr_port;
			 (void)mach_message_get(sm, send_size, mp);
			 wakeup(mp);
		}

out1:
	if (error != 0) 
		free(sm, M_EMULDATA);
		return error;
	}

	if (SCARG(uap, option) & MACH_RCV_MSG) {
		/* 
		 * Find a buffer for the reply
		 */
		if (SCARG(uap, rcv_msg) != NULL)
			urm = SCARG(uap, rcv_msg);
		else if (SCARG(uap, msg) != NULL)
			urm = SCARG(uap, msg);
		else
			return EINVAL;

		/* 
		 * Check for receive right on the port 
		 */
		if ((mach_right_check((struct mach_right *)
		    SCARG(uap, rcv_name), p, MACH_PORT_TYPE_RECEIVE)) == 0)
			return EPERM;

		/*
		 * If there is no message queued on the port,
		 * block until we get some.
		 */
		mp = ((struct mach_right *)SCARG(uap, rcv_name))->mr_port;
		timeout = SCARG(uap, timeout) * hz / 1000;
		if (mp->mp_count == 0) {
			error = tsleep(mp, PZERO, "mach_msg", timeout);
			if ((error == ERESTART) || (error == EINTR))
				return EINTR;
			if (mp->mp_count == 0)
				return ETIMEDOUT;
		}

		/* 
		 * Dequeue the message.
		 * XXX Do we really need to lock here? There could be
		 * only one reader process, so mm will not disapear
		 * except if there is a port refcount error in our code.
		 */
		lockmgr(&mp->mp_msglock, LK_SHARED, NULL);
		mm = TAILQ_FIRST(&mp->mp_msglist);

		if (mm->mm_size > rcv_size) {
			lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);
			return ENOBUFS;
		}

		if ((error = copyout(mm->mm_msg, urm, mm->mm_size)) != 0) {
			lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);
			return error;
		}
#ifdef KTRACE
		/* Dump the Mach message */
		if (KTRPOINT(p, KTR_MMSG))
			ktrmmsg(p, (char *)mm->mm_msg, mm->mm_size); 
#endif

		free(mm->mm_msg, M_EMULDATA);
		mach_message_put_shlocked(mm); /* decrease mp_count */
		lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);
	}

	return 0;
}

int
mach_sys_msg_trap(p, v, retval)
	struct proc *p;
	void *v;
	 register_t *retval;
{
	struct mach_sys_msg_trap_args /* {
		syscallarg(mach_msg_header_t *) msg;
		syscallarg(mach_msg_option_t) option;
		syscallarg(mach_msg_size_t) send_size;
		syscallarg(mach_msg_size_t) rcv_size;
		syscallarg(mach_port_name_t) rcv_name;
		syscallarg(mach_msg_timeout_t) timeout;
		syscallarg(mach_port_name_t) notify;
	} */ *uap = v;
	struct mach_sys_msg_overwrite_trap_args cup;

	SCARG(&cup, msg) = SCARG(uap, msg);
	SCARG(&cup, option) = SCARG(uap, option);
	SCARG(&cup, send_size) = SCARG(uap, send_size);
	SCARG(&cup, rcv_size) = SCARG(uap, rcv_size);
	SCARG(&cup, rcv_name) = SCARG(uap, rcv_name);
	SCARG(&cup, timeout) = SCARG(uap, timeout);
	SCARG(&cup, notify) = SCARG(uap, notify);
	SCARG(&cup, rcv_msg) = NULL;
	SCARG(&cup, scatter_list_size) = 0;

	return mach_sys_msg_overwrite_trap(p, &cup, retval);
}

void
mach_message_init(void)
{
	pool_init(&mach_message_pool, sizeof (struct mach_message),
	    0, 0, 128, "mach_message_pool", NULL);
	return;
}

struct mach_message *
mach_message_get(msgh, size, mp)
	mach_msg_header_t *msgh;
	size_t size;
	struct mach_port *mp;
{
	struct mach_message *mm;

	mm = (struct mach_message *)pool_get(&mach_message_pool, M_WAITOK);
	bzero(mm, sizeof(*mm));
	mm->mm_msg = msgh;
	mm->mm_size = size;
	mm->mm_port = mp;
	
	lockmgr(&mp->mp_msglock, LK_EXCLUSIVE, NULL);
	TAILQ_INSERT_TAIL(&mp->mp_msglist, mm, mm_list);
	mp->mp_count++;
	lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);

	return mm;
}

void
mach_message_put(mm)
	struct mach_message *mm;
{
	struct mach_port *mp;

	mp = mm->mm_port;

	lockmgr(&mp->mp_msglock, LK_EXCLUSIVE, NULL);
	mach_message_put_exclocked(mm);
	lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);

	return;
}

void
mach_message_put_shlocked(mm)
	struct mach_message *mm;
{
	struct mach_port *mp;

	mp = mm->mm_port;

	lockmgr(&mp->mp_msglock, LK_UPGRADE, NULL);
	mach_message_put_exclocked(mm);
	lockmgr(&mp->mp_msglock, LK_DOWNGRADE, NULL);

	return;
}

void
mach_message_put_exclocked(mm)
	struct mach_message *mm;
{
	struct mach_port *mp;

	mp = mm->mm_port;

	TAILQ_REMOVE(&mp->mp_msglist, mm, mm_list);
	mp->mp_count--;

	pool_put(&mach_message_pool, mm);

	return;
}
