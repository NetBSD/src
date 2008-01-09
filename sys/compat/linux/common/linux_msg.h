/*	$NetBSD: linux_msg.h,v 1.7.40.1 2008/01/09 01:51:13 matt Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#ifndef _LINUX_MSG_H
#define _LINUX_MSG_H

#include <sys/msg.h>

/*
 * msq_id_ds structure. Mostly the same fields, except for some internal
 * ones.
 */
struct linux_msqid_ds {
	struct linux_ipc_perm	l_msg_perm;
	void			*l_msg_first;
	void			*l_msg_last;
	linux_time_t		l_msg_stime;
	linux_time_t		l_msg_rtime;
	linux_time_t		l_msg_ctime;
	void			*l_wwait;	/* Linux internal */
	void			*l_rwait;	/* Linux internal */
	ushort			l_msg_cbytes;
	ushort			l_msg_qnum;
	ushort			l_msg_qbytes;
	ushort			l_msg_lspid;
	ushort			l_msg_lrpid;
};

#define LINUX_MSG_NOERROR	0x1000
#define LINUX_MSG_EXCEPT	0x2000

/*
 * The notorious anonymous message structure.
 */
struct linux_mymsg {
	long	l_mtype;
	char	l_mtext[1];
};

/*
 * This kludge is used for the 6th argument to the msgrcv system
 * call, to get around the maximum of 5 arguments to a syscall in Linux.
 */
struct linux_msgrcv_msgarg {
	struct linux_mymsg *msg;
	int type;
};
/*
 * For msgctl calls.
 */
struct linux_msginfo {
	int	l_msgpool;
	int	l_msgmap;
	int	l_msgmax;
	int	l_msgmnb;
	int	l_msgmni;
	int	l_msgssz;
	int	l_msgtql;
	ushort	l_msgseg;
};

#define LINUX_MSG_STAT	11
#define LINUX_MSG_INFO	12

/* Pretend the sys_msgctl syscall is defined */
struct linux_sys_msgctl_args {
	syscallarg(int) msqid;
	syscallarg(int) cmd;
	syscallarg(struct linux_msqid_ds *) buf;
};


#ifdef SYSVMSG
#ifdef _KERNEL
__BEGIN_DECLS
int linux_sys_msgctl(struct lwp *, const struct linux_sys_msgctl_args *, register_t *);
void linux_to_bsd_msqid_ds(struct linux_msqid_ds *,
				       struct msqid_ds *);
void bsd_to_linux_msqid_ds(struct msqid_ds *,
				       struct linux_msqid_ds *);
__END_DECLS
#endif	/* !_KERNEL */
#endif	/* !SYSVMSG */

#endif /* !_LINUX_MSG_H */
