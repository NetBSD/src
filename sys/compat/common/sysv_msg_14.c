/*	$NetBSD: sysv_msg_14.c,v 1.3.2.3 2002/05/29 21:32:15 nathanw Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: sysv_msg_14.c,v 1.3.2.3 2002/05/29 21:32:15 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/msg.h>

#ifndef SYSVMSG
#define	SYSVMSG
#endif

#include <sys/sa.h>
#include <sys/syscallargs.h>

static void msqid_ds14_to_native __P((struct msqid_ds14 *, struct msqid_ds *));
static void native_to_msqid_ds14 __P((struct msqid_ds *, struct msqid_ds14 *));

static void
msqid_ds14_to_native(omsqbuf, msqbuf)
	struct msqid_ds14 *omsqbuf;
	struct msqid_ds *msqbuf;
{

	ipc_perm14_to_native(&omsqbuf->msg_perm, &msqbuf->msg_perm);

#define	CVT(x)	msqbuf->x = omsqbuf->x
	CVT(msg_qnum);
	CVT(msg_qbytes);
	CVT(msg_lspid);
	CVT(msg_lrpid);
	CVT(msg_stime);
	CVT(msg_rtime);
	CVT(msg_ctime);
#undef CVT
}

static void
native_to_msqid_ds14(msqbuf, omsqbuf)
	struct msqid_ds *msqbuf;
	struct msqid_ds14 *omsqbuf;
{

	native_to_ipc_perm14(&msqbuf->msg_perm, &omsqbuf->msg_perm);

#define	CVT(x)	omsqbuf->x = msqbuf->x
	CVT(msg_qnum);
	CVT(msg_qbytes);
	CVT(msg_lspid);
	CVT(msg_lrpid);
	CVT(msg_stime);
	CVT(msg_rtime);
	CVT(msg_ctime);
#undef CVT

	/*
	 * Not part of the API, but some programs might look at it.
	 */
	omsqbuf->msg_cbytes = msqbuf->_msg_cbytes;
}

int
compat_14_sys_msgctl(struct lwp *l, void *v, register_t *retval)
{
	struct compat_14_sys_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct msqid_ds14 *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct msqid_ds msqbuf;
	struct msqid_ds14 omsqbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &omsqbuf, sizeof(omsqbuf));
		if (error) 
			return (error);
		msqid_ds14_to_native(&omsqbuf, &msqbuf);
	}

	error = msgctl1(p, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &msqbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_msqid_ds14(&msqbuf, &omsqbuf);     
		error = copyout(&omsqbuf, SCARG(uap, buf), sizeof(omsqbuf));
	}

	return (error);
}
