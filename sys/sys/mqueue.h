/*	$NetBSD: mqueue.h,v 1.2.14.3 2008/01/09 01:58:12 matt Exp $	*/

/*
 * Copyright (c) 2007, Mindaugas Rasiukevicius <rmind at NetBSD org>
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

#ifndef _SYS_MQUEUE_H_
#define _SYS_MQUEUE_H_

/* Maximal number of mqueue descriptors, that process could open */
#define	MQ_OPEN_MAX		512

/* Maximal priority of the message */
#define	MQ_PRIO_MAX		64

struct mq_attr {
	long	mq_flags;	/* Flags of message queue */
	long	mq_maxmsg;	/* Maximum number of messages */
	long	mq_msgsize;	/* Maximum size of the message */
	long	mq_curmsgs;	/* Count of the queued messages */
};

/* Internal kernel data */
#ifdef _KERNEL

#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/types.h>

/*
 * Flags below are used in mq_flags for internal
 * purposes, this is appropriate according to POSIX.
 */

/* Message queue is unlinking */
#define	MQ_UNLINK		0x10000000
/* There are receive-waiters */
#define	MQ_RECEIVE		0x40000000

/* Maximal length of mqueue name */
#define	MQ_NAMELEN		(NAME_MAX + 1)

/* Default size of the message */
#define	MQ_DEF_MSGSIZE		1024

/* Structure of the message queue */
struct mqueue {
	kmutex_t		mq_mtx;
	kcondvar_t		mq_send_cv;
	kcondvar_t		mq_recv_cv;
	char			mq_name[MQ_NAMELEN];
	struct mq_attr		mq_attrib;
	/* Notification */
	struct sigevent		mq_sig_notify;
	struct proc *		mq_notify_proc;
	/* Permissions */
	mode_t			mq_mode;
	uid_t			mq_euid;
	gid_t			mq_egid;
	/* Reference counter, head of the message queue */
	u_int			mq_refcnt;
	TAILQ_HEAD(, mq_msg)	mq_head;
	/* Entry of the global list */
	LIST_ENTRY(mqueue)	mq_list;
};

/* Structure of the message */
struct mq_msg {
	TAILQ_ENTRY(mq_msg)	msg_queue;
	size_t			msg_len;
	u_int			msg_prio;
	int8_t			msg_ptr[1];
};

/* Prototypes */
void	mqueue_sysinit(void);
void	mqueue_print_list(void (*pr)(const char *, ...));

#endif	/* _KERNEL */

#endif	/* _SYS_MQUEUE_H_ */
