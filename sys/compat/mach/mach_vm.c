/*	$NetBSD: mach_vm.c,v 1.4 2002/11/11 09:28:00 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_vm.c,v 1.4 2002/11/11 09:28:00 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
 
#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_vm.h> 
#include <compat/mach/mach_errno.h> 
#include <compat/mach/mach_syscallargs.h>

int
mach_vm_map(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_vm_map_request_t req;
	mach_vm_map_reply_t rep;
	struct sys_mmap_args cup;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));

	SCARG(&cup, addr) = (void *)req.req_address;
	SCARG(&cup, len) = req.req_size;
	SCARG(&cup, prot) = req.req_cur_protection; /* XXX */
	SCARG(&cup, flags) = req.req_flags; /* XXX */
	SCARG(&cup, fd) = 0; /* XXX */
	SCARG(&cup, pos) = (off_t)req.req_offset;
	
	if ((error = sys_mmap(p, &cup, &rep.rep_retval)) != 0)
		rep.rep_retval = native_to_mach_errno[error];
	
	rep.rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}

int
mach_vm_deallocate(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_vm_deallocate_request_t req;
	mach_vm_deallocate_reply_t rep;
	struct sys_munmap_args cup;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));

	SCARG(&cup, addr) = (caddr_t)req.req_address;
	SCARG(&cup, len) = req.req_size;

	if ((error = sys_munmap(p, &cup, &rep.rep_retval)) != 0)
		rep.rep_retval = native_to_mach_errno[error];
	
	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}
