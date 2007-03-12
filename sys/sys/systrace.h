/*	$NetBSD: systrace.h,v 1.22.2.1 2007/03/12 06:00:55 rmind Exp $	*/

/*
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_SYSTRACE_H_
#define _SYS_SYSTRACE_H_

#include <sys/select.h>
#include <sys/ioccom.h>
#include <sys/mutex.h>
#include <sys/queue.h>

#define SYSTR_EMULEN	8	/* sync with sys proc */

struct str_msg_emul {
	char emul[SYSTR_EMULEN];
};

struct str_msg_ugid {
	uid_t uid;
	gid_t gid;
};

struct str_msg_execve {
	char path[MAXPATHLEN];
};

#define SYSTR_MAX_POLICIES	64
#define SYSTR_MAXARGS		64
#define SYSTR_MAXFNAME		8
#define SYSTR_MAXREPLEN		2048

struct str_msg_ask {
	int32_t code;
	int32_t argsize;
	register_t args[SYSTR_MAXARGS];
	register_t rval[2];
	int32_t result;
};

/* Queued on fork or exit of a process */

struct str_msg_child {
	pid_t new_pid;
};

#define SYSTR_MSG_ASK		1
#define SYSTR_MSG_RES		2
#define SYSTR_MSG_EMUL		3
#define SYSTR_MSG_CHILD		4
#define SYSTR_MSG_UGID		5
#define SYSTR_MSG_POLICYFREE	6
#define	SYSTR_MSG_EXECVE	7
#define	SYSTR_MSG_SCRIPTNAME	8

#define SYSTR_MSG_NOPROCESS(x) \
	((x)->msg.msg_type == SYSTR_MSG_CHILD || \
	 (x)->msg.msg_type == SYSTR_MSG_POLICYFREE)

struct str_message {
	int32_t msg_type;
	pid_t msg_pid;
	uint16_t msg_seqnr;	/* answer has to match seqnr */
	int16_t msg_policy;
	union {
		struct str_msg_emul msg_emul;
		struct str_msg_ugid msg_ugid;
		struct str_msg_ask msg_ask;
		struct str_msg_child msg_child;
		struct str_msg_execve msg_execve;
	} msg_data;
};

struct str_process;
struct str_msgcontainer {
	TAILQ_ENTRY(str_msgcontainer) next;
	struct str_process *strp;

	struct str_message msg;
};


struct systrace_answer {
	pid_t stra_pid;
	uint16_t stra_seqnr;
	int16_t reserved;
 	uid_t stra_seteuid;	/* elevated privileges for system call */
 	gid_t stra_setegid;
	int32_t stra_policy;
	int32_t stra_error;
	int32_t stra_flags;
};

struct systrace_scriptname {
	pid_t sn_pid;
	char sn_scriptname[MAXPATHLEN];
};

#define SYSTR_READ		1
#define SYSTR_WRITE		2

struct systrace_io {
	pid_t strio_pid;
	int32_t strio_op;
	void *strio_offs;
	void *strio_addr;
	size_t strio_len;
};

#define SYSTR_POLICY_NEW	1
#define SYSTR_POLICY_ASSIGN	2
#define SYSTR_POLICY_MODIFY	3

struct systrace_policy {
	int32_t strp_op;
	int32_t strp_num;
	union {
		struct {
			int16_t code;
			int16_t policy;
		} assign;
		pid_t pid;
		int32_t maxents;
	} strp_data;
};

#define strp_pid	strp_data.pid
#define strp_maxents	strp_data.maxents
#define strp_code	strp_data.assign.code
#define strp_policy	strp_data.assign.policy

#define SYSTR_NOLINKS	1

struct systrace_replace {
	pid_t strr_pid;
	uint16_t strr_seqnr;
	int16_t reserved;
	int32_t strr_nrepl;
	void *	strr_base;	/* Base memory */
	size_t strr_len;	/* Length of memory */
	int32_t strr_argind[SYSTR_MAXARGS];
	size_t strr_off[SYSTR_MAXARGS];
	size_t strr_offlen[SYSTR_MAXARGS];
	int32_t strr_flags[SYSTR_MAXARGS];
};

#define STRIOCATTACH	_IOW('s', 101, pid_t)
#define STRIOCDETACH	_IOW('s', 102, pid_t)
#define STRIOCANSWER	_IOW('s', 103, struct systrace_answer)
#define STRIOCIO	_IOWR('s', 104, struct systrace_io)
#define STRIOCPOLICY	_IOWR('s', 105, struct systrace_policy)
#define STRIOCGETCWD	_IOW('s', 106, pid_t)
#define STRIOCRESCWD	_IO('s', 107)
#define STRIOCREPORT	_IOW('s', 108, pid_t)
#define STRIOCREPLACE	_IOW('s', 109, struct systrace_replace)
#define	STRIOCSCRIPTNAME	_IOW('s', 110, struct systrace_scriptname)

#define SYSTR_POLICY_ASK	0
#define SYSTR_POLICY_PERMIT	1
#define SYSTR_POLICY_NEVER	2

#define SYSTR_FLAGS_RESULT	0x001
#define SYSTR_FLAGS_SETEUID	0x002
#define SYSTR_FLAGS_SETEGID	0x004

#ifdef _KERNEL
#include <sys/namei.h>

struct fsystrace {
	kmutex_t mutex;
	struct selinfo si;

	TAILQ_HEAD(strprocessq, str_process) processes;
	size_t nprocesses;

	TAILQ_HEAD(strpolicyq, str_policy) policies;

	TAILQ_HEAD(strmessageq, str_msgcontainer) messages;

	size_t npolicynr;
	size_t npolicies;

	int issuser;
	uid_t p_ruid;
	gid_t p_rgid;

	/* cwd magic */
	pid_t fd_pid;
	struct vnode *fd_cdir;
	struct vnode *fd_rdir;
};

/* Internal prototypes */

void systrace_init(void);
int systrace_enter(struct lwp *, register_t, void *);
void systrace_namei(struct nameidata *);
void systrace_exit(struct lwp *, register_t, void *, register_t [], int);
void systrace_sys_exit(struct proc *);
void systrace_sys_fork(struct proc *, struct proc *);
#ifndef __NetBSD__
void systrace_init(void);
#endif /* ! __NetBSD__ */
void systrace_execve0(struct proc *);
void systrace_execve1(char *, struct proc *);
int systrace_scriptname(struct proc *, char *);

#endif /* _KERNEL */
#endif /* !_SYS_SYSTRACE_H_ */
