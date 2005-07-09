/*	$NetBSD: netbsd32_select.c,v 1.6 2005/07/09 21:58:09 cube Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_select.c,v 1.6 2005/07/09 21:58:09 cube Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>

#include <sys/proc.h>

#include <net/if.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32_select(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_select_args /* {
		syscallarg(int) nd;
		syscallarg(netbsd32_fd_setp_t) in;
		syscallarg(netbsd32_fd_setp_t) ou;
		syscallarg(netbsd32_fd_setp_t) ex;
		syscallarg(netbsd32_timevalp_t) tv;
	} */ *uap = v;
	int error;
	struct netbsd32_timeval tv32;
	struct timeval atv, *tv = NULL;

	if (SCARG(uap, tv)) {
		if ((error = copyin(NETBSD32PTR64(SCARG(uap, tv)),
		    (caddr_t)&tv32, sizeof(tv32))) != 0)
			return error;
		netbsd32_to_timeval(&tv32, &atv);
		tv = &atv;
	}

	return selcommon(l, retval, SCARG(uap, nd), NETBSD32PTR64(SCARG(uap, in)),
	    NETBSD32PTR64(SCARG(uap, ou)), NETBSD32PTR64(SCARG(uap, ex)), tv,
	    NULL);
}
