/*	$NetBSD: mach_message.c,v 1.1 2002/11/28 21:21:32 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_message.c,v 1.1 2002/11/28 21:21:32 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_syscallargs.h>

#ifdef DEBUG_MACH
static void mach_print_msg_header_t(mach_msg_header_t *);
static char *mach_print_msg_bits_t(mach_msg_bits_t, char *, size_t);

static char *
mach_print_msg_bits_t(mach_msg_bits_t bits, char *buf, size_t bufsiz) {
	switch (bits) {
	case MACH_MSG_TYPE_MOVE_RECEIVE:
		return "move_receive";
	case MACH_MSG_TYPE_MOVE_SEND:
		return "move_send";
	case MACH_MSG_TYPE_MOVE_SEND_ONCE:
		return "move_send_once";
	case MACH_MSG_TYPE_COPY_SEND:
		return "copy_send";
	case MACH_MSG_TYPE_MAKE_SEND:
		return "make_send";
	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
		return "make_send_once";
	case MACH_MSG_TYPE_COPY_RECEIVE:
		return "copy_receive";
	default:
		(void)snprintf(buf, bufsiz, "%d", bits);
		return buf;
	}
}


static void
mach_print_msg_header_t(mach_msg_header_t *mh) {
	char lbuf[128];
	char rbuf[128];
	uprintf("msgh { bits=[l=%s|r=%s], size=%d, remote_port=%d, "
	    "local_port=%d, reserved=%d, id=%d }\n",
	    mach_print_msg_bits_t(MACH_MSGH_LOCAL_BITS(mh->msgh_bits),
		lbuf, sizeof(lbuf)),
	    mach_print_msg_bits_t(MACH_MSGH_REMOTE_BITS(mh->msgh_bits),
		rbuf, sizeof(rbuf)),
	    mh->msgh_size, mh->msgh_remote_port,
	    mh->msgh_local_port, mh->msgh_reserved, mh->msgh_id);
}
#endif /* DEBUG_MACH */


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
	int error;
	struct mach_subsystem_namemap *namemap;

	switch (SCARG(uap, option)) {
	case MACH_SEND_MSG|MACH_RCV_MSG:
		if (SCARG(uap, msg)) {
			mach_msg_header_t mh;
			if ((error = copyin(SCARG(uap, msg), &mh,
			    sizeof(mh))) != 0)
				return error;
			for (namemap = mach_namemap; 
			    namemap->map_id; namemap++)
				if (namemap->map_id == mh.msgh_id)
					break;
			if (namemap->map_handler == NULL) {
#ifdef DEBUG_MACH
			mach_print_msg_header_t(&mh);
#endif /* DEBUG_MACH */
				break;
			}
			return (*namemap->map_handler)(p, SCARG(uap, msg), 
			    SCARG(uap, rcv_size), SCARG(uap, rcv_msg));
		}
		break;
	default:
		uprintf("unhandled sys_msg_override_trap option %x\n",
		    SCARG(uap, option));
		break;
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
	int error;
	struct mach_subsystem_namemap *namemap;

	switch (SCARG(uap, option)) {
	case MACH_SEND_MSG|MACH_RCV_MSG:
		if (SCARG(uap, msg)) {
			mach_msg_header_t mh;
			if ((error = copyin(SCARG(uap, msg), &mh,
			    sizeof(mh))) != 0)
				return error;
			for (namemap = mach_namemap; 
			    namemap->map_id; namemap++)
				if (namemap->map_id == mh.msgh_id)
					break;
			if (namemap->map_handler == NULL) {
#ifdef DEBUG_MACH
				mach_print_msg_header_t(&mh);
#endif /* DEBUG_MACH */
				break;
			}
			return (*namemap->map_handler)(p, SCARG(uap, msg),
			    SCARG(uap, rcv_size), NULL);
		}
		break;
	default:
		uprintf("unhandled sys_msg_trap option %x\n",
		    SCARG(uap, option));
		break;
	}
	return 0;
}

