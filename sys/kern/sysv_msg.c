/*	$NetBSD: sysv_msg.c,v 1.50 2007/10/09 03:27:37 rmind Exp $	*/

/*-
 * Copyright (c) 1999, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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

/*
 * Implementation of SVID messages
 *
 * Author: Daniel Boulet
 *
 * Copyright 1993 Daniel Boulet and RTMX Inc.
 *
 * This system call was implemented by Daniel Boulet under contract from RTMX.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysv_msg.c,v 1.50 2007/10/09 03:27:37 rmind Exp $");

#define SYSVMSG

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/mount.h>		/* XXX for <sys/syscallargs.h> */
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#define MSG_DEBUG
#undef MSG_DEBUG_OK

#ifdef MSG_DEBUG_OK
#define MSG_PRINTF(a)	printf a
#else
#define MSG_PRINTF(a)
#endif

static int	nfree_msgmaps;		/* # of free map entries */
static short	free_msgmaps;	/* head of linked list of free map entries */
static struct	__msg *free_msghdrs;	/* list of free msg headers */
static char	*msgpool;		/* MSGMAX byte long msg buffer pool */
static struct	msgmap *msgmaps;	/* MSGSEG msgmap structures */
static struct __msg *msghdrs;		/* MSGTQL msg headers */

kmsq_t	*msqs;				/* MSGMNI msqid_ds struct's */
kmutex_t msgmutex;			/* subsystem lock */

static void msg_freehdr(struct __msg *);

void
msginit(void)
{
	int i, sz;
	vaddr_t v;

	/*
	 * msginfo.msgssz should be a power of two for efficiency reasons.
	 * It is also pretty silly if msginfo.msgssz is less than 8
	 * or greater than about 256 so ...
	 */

	i = 8;
	while (i < 1024 && i != msginfo.msgssz)
		i <<= 1;
	if (i != msginfo.msgssz) {
		panic("msginfo.msgssz = %d, not a small power of 2",
		    msginfo.msgssz);
	}

	if (msginfo.msgseg > 32767) {
		panic("msginfo.msgseg = %d > 32767", msginfo.msgseg);
	}

	/* Allocate pageable memory for our structures */
	sz = msginfo.msgmax +
	    msginfo.msgseg * sizeof(struct msgmap) +
	    msginfo.msgtql * sizeof(struct __msg) +
	    msginfo.msgmni * sizeof(kmsq_t);
	v = uvm_km_alloc(kernel_map, round_page(sz), 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	if (v == 0)
		panic("sysv_msg: cannot allocate memory");
	msgpool = (void *)v;
	msgmaps = (void *)(msgpool + msginfo.msgmax);
	msghdrs = (void *)(msgmaps + msginfo.msgseg);
	msqs = (void *)(msghdrs + msginfo.msgtql);

	for (i = 0; i < (msginfo.msgseg - 1); i++)
		msgmaps[i].next = i + 1;
	msgmaps[msginfo.msgseg - 1].next = -1;

	free_msgmaps = 0;
	nfree_msgmaps = msginfo.msgseg;

	for (i = 0; i < (msginfo.msgtql - 1); i++) {
		msghdrs[i].msg_type = 0;
		msghdrs[i].msg_next = &msghdrs[i + 1];
	}
	i = msginfo.msgtql - 1;
	msghdrs[i].msg_type = 0;
	msghdrs[i].msg_next = NULL;
	free_msghdrs = &msghdrs[0];

	for (i = 0; i < msginfo.msgmni; i++) {
		cv_init(&msqs[i].msq_cv, "msgwait");
		/* Implies entry is available */
		msqs[i].msq_u.msg_qbytes = 0;
		/* Reset to a known value */
		msqs[i].msq_u.msg_perm._seq = 0;
	}

	mutex_init(&msgmutex, MUTEX_DEFAULT, IPL_NONE);
}

static void
msg_freehdr(struct __msg *msghdr)
{

	KASSERT(mutex_owned(&msgmutex));

	while (msghdr->msg_ts > 0) {
		short next;
		KASSERT(msghdr->msg_spot >= 0);
		KASSERT(msghdr->msg_spot < msginfo.msgseg);

		next = msgmaps[msghdr->msg_spot].next;
		msgmaps[msghdr->msg_spot].next = free_msgmaps;
		free_msgmaps = msghdr->msg_spot;
		nfree_msgmaps++;
		msghdr->msg_spot = next;
		if (msghdr->msg_ts >= msginfo.msgssz)
			msghdr->msg_ts -= msginfo.msgssz;
		else
			msghdr->msg_ts = 0;
	}
	KASSERT(msghdr->msg_spot == -1);
	msghdr->msg_next = free_msghdrs;
	free_msghdrs = msghdr;
}

int
sys___msgctl13(struct lwp *l, void *v, register_t *retval)
{
	struct sys___msgctl13_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct msqid_ds *) buf;
	} */ *uap = v;
	struct msqid_ds msqbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &msqbuf, sizeof(msqbuf));
		if (error)
			return (error);
	}

	error = msgctl1(l, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &msqbuf : NULL);

	if (error == 0 && cmd == IPC_STAT)
		error = copyout(&msqbuf, SCARG(uap, buf), sizeof(msqbuf));

	return (error);
}

int
msgctl1(struct lwp *l, int msqid, int cmd, struct msqid_ds *msqbuf)
{
	kauth_cred_t cred = l->l_cred;
	struct msqid_ds *msqptr;
	kmsq_t *msq;
	int error = 0, ix;

	MSG_PRINTF(("call to msgctl1(%d, %d)\n", msqid, cmd));

	ix = IPCID_TO_IX(msqid);

	mutex_enter(&msgmutex);

	if (ix < 0 || ix >= msginfo.msgmni) {
		MSG_PRINTF(("msqid (%d) out of range (0<=msqid<%d)\n", ix,
		    msginfo.msgmni));
		error = EINVAL;
		goto unlock;
	}

	msq = &msqs[ix];
	msqptr = &msq->msq_u;

	if (msqptr->msg_qbytes == 0) {
		MSG_PRINTF(("no such msqid\n"));
		error = EINVAL;
		goto unlock;
	}
	if (msqptr->msg_perm._seq != IPCID_TO_SEQ(msqid)) {
		MSG_PRINTF(("wrong sequence number\n"));
		error = EINVAL;
		goto unlock;
	}

	switch (cmd) {
	case IPC_RMID:
	{
		struct __msg *msghdr;
		if ((error = ipcperm(cred, &msqptr->msg_perm, IPC_M)) != 0)
			break;
		/* Free the message headers */
		msghdr = msqptr->_msg_first;
		while (msghdr != NULL) {
			struct __msg *msghdr_tmp;

			/* Free the segments of each message */
			msqptr->_msg_cbytes -= msghdr->msg_ts;
			msqptr->msg_qnum--;
			msghdr_tmp = msghdr;
			msghdr = msghdr->msg_next;
			msg_freehdr(msghdr_tmp);
		}
		KASSERT(msqptr->_msg_cbytes == 0);
		KASSERT(msqptr->msg_qnum == 0);

		/* Mark it as free */
		msqptr->msg_qbytes = 0;
		cv_broadcast(&msq->msq_cv);
	}
		break;

	case IPC_SET:
		if ((error = ipcperm(cred, &msqptr->msg_perm, IPC_M)))
			break;
		if (msqbuf->msg_qbytes > msqptr->msg_qbytes &&
		    kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
		    NULL) != 0) {
			error = EPERM;
			break;
		}
		if (msqbuf->msg_qbytes > msginfo.msgmnb) {
			MSG_PRINTF(("can't increase msg_qbytes beyond %d "
			    "(truncating)\n", msginfo.msgmnb));
			/* silently restrict qbytes to system limit */
			msqbuf->msg_qbytes = msginfo.msgmnb;
		}
		if (msqbuf->msg_qbytes == 0) {
			MSG_PRINTF(("can't reduce msg_qbytes to 0\n"));
			error = EINVAL;		/* XXX non-standard errno! */
			break;
		}
		msqptr->msg_perm.uid = msqbuf->msg_perm.uid;
		msqptr->msg_perm.gid = msqbuf->msg_perm.gid;
		msqptr->msg_perm.mode = (msqptr->msg_perm.mode & ~0777) |
		    (msqbuf->msg_perm.mode & 0777);
		msqptr->msg_qbytes = msqbuf->msg_qbytes;
		msqptr->msg_ctime = time_second;
		break;

	case IPC_STAT:
		if ((error = ipcperm(cred, &msqptr->msg_perm, IPC_R))) {
			MSG_PRINTF(("requester doesn't have read access\n"));
			break;
		}
		memcpy(msqbuf, msqptr, sizeof(struct msqid_ds));
		break;

	default:
		MSG_PRINTF(("invalid command %d\n", cmd));
		error = EINVAL;
		break;
	}

unlock:
	mutex_exit(&msgmutex);
	return (error);
}

int
sys_msgget(struct lwp *l, void *v, register_t *retval)
{
	struct sys_msgget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) msgflg;
	} */ *uap = v;
	int msqid, error = 0;
	int key = SCARG(uap, key);
	int msgflg = SCARG(uap, msgflg);
	kauth_cred_t cred = l->l_cred;
	struct msqid_ds *msqptr = NULL;
	kmsq_t *msq;

	mutex_enter(&msgmutex);

	MSG_PRINTF(("msgget(0x%x, 0%o)\n", key, msgflg));

	if (key != IPC_PRIVATE) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			msq = &msqs[msqid];
			msqptr = &msq->msq_u;
			if (msqptr->msg_qbytes != 0 &&
			    msqptr->msg_perm._key == key)
				break;
		}
		if (msqid < msginfo.msgmni) {
			MSG_PRINTF(("found public key\n"));
			if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
				MSG_PRINTF(("not exclusive\n"));
				error = EEXIST;
				goto unlock;
			}
			if ((error = ipcperm(cred, &msqptr->msg_perm,
			    msgflg & 0700 ))) {
				MSG_PRINTF(("requester doesn't have 0%o access\n",
				    msgflg & 0700));
				goto unlock;
			}
			goto found;
		}
	}

	MSG_PRINTF(("need to allocate the msqid_ds\n"));
	if (key == IPC_PRIVATE || (msgflg & IPC_CREAT)) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			/*
			 * Look for an unallocated and unlocked msqid_ds.
			 * msqid_ds's can be locked by msgsnd or msgrcv while
			 * they are copying the message in/out.  We can't
			 * re-use the entry until they release it.
			 */
			msq = &msqs[msqid];
			msqptr = &msq->msq_u;
			if (msqptr->msg_qbytes == 0 &&
			    (msqptr->msg_perm.mode & MSG_LOCKED) == 0)
				break;
		}
		if (msqid == msginfo.msgmni) {
			MSG_PRINTF(("no more msqid_ds's available\n"));
			error = ENOSPC;
			goto unlock;
		}
		MSG_PRINTF(("msqid %d is available\n", msqid));
		msqptr->msg_perm._key = key;
		msqptr->msg_perm.cuid = kauth_cred_geteuid(cred);
		msqptr->msg_perm.uid = kauth_cred_geteuid(cred);
		msqptr->msg_perm.cgid = kauth_cred_getegid(cred);
		msqptr->msg_perm.gid = kauth_cred_getegid(cred);
		msqptr->msg_perm.mode = (msgflg & 0777);
		/* Make sure that the returned msqid is unique */
		msqptr->msg_perm._seq++;
		msqptr->_msg_first = NULL;
		msqptr->_msg_last = NULL;
		msqptr->_msg_cbytes = 0;
		msqptr->msg_qnum = 0;
		msqptr->msg_qbytes = msginfo.msgmnb;
		msqptr->msg_lspid = 0;
		msqptr->msg_lrpid = 0;
		msqptr->msg_stime = 0;
		msqptr->msg_rtime = 0;
		msqptr->msg_ctime = time_second;
	} else {
		MSG_PRINTF(("didn't find it and wasn't asked to create it\n"));
		error = ENOENT;
		goto unlock;
	}

found:
	/* Construct the unique msqid */
	*retval = IXSEQ_TO_IPCID(msqid, msqptr->msg_perm);

unlock:
	mutex_exit(&msgmutex);
	return (error);
}

int
sys_msgsnd(struct lwp *l, void *v, register_t *retval)
{
	struct sys_msgsnd_args /* {
		syscallarg(int) msqid;
		syscallarg(const void *) msgp;
		syscallarg(size_t) msgsz;
		syscallarg(int) msgflg;
	} */ *uap = v;

	return msgsnd1(l, SCARG(uap, msqid), SCARG(uap, msgp),
	    SCARG(uap, msgsz), SCARG(uap, msgflg), sizeof(long), copyin);
}

int
msgsnd1(struct lwp *l, int msqidr, const char *user_msgp, size_t msgsz,
    int msgflg, size_t typesz, copyin_t fetch_type)
{
	int segs_needed, error = 0, msqid;
	kauth_cred_t cred = l->l_cred;
	struct msqid_ds *msqptr;
	struct __msg *msghdr;
	kmsq_t *msq;
	short next;

	MSG_PRINTF(("call to msgsnd(%d, %p, %lld, %d)\n", msqid, user_msgp,
	    (long long)msgsz, msgflg));

	msqid = IPCID_TO_IX(msqidr);

	mutex_enter(&msgmutex);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
		MSG_PRINTF(("msqid (%d) out of range (0<=msqid<%d)\n", msqid,
		    msginfo.msgmni));
		error = EINVAL;
		goto unlock;
	}

	msq = &msqs[msqid];
	msqptr = &msq->msq_u;

	if (msqptr->msg_qbytes == 0) {
		MSG_PRINTF(("no such message queue id\n"));
		error = EINVAL;
		goto unlock;
	}
	if (msqptr->msg_perm._seq != IPCID_TO_SEQ(msqidr)) {
		MSG_PRINTF(("wrong sequence number\n"));
		error = EINVAL;
		goto unlock;
	}

	if ((error = ipcperm(cred, &msqptr->msg_perm, IPC_W))) {
		MSG_PRINTF(("requester doesn't have write access\n"));
		goto unlock;
	}

	segs_needed = (msgsz + msginfo.msgssz - 1) / msginfo.msgssz;
	MSG_PRINTF(("msgsz=%lld, msgssz=%d, segs_needed=%d\n",
	    (long long)msgsz, msginfo.msgssz, segs_needed));
	for (;;) {
		int need_more_resources = 0;

		/*
		 * check msgsz [cannot be negative since it is unsigned]
		 * (inside this loop in case msg_qbytes changes while we sleep)
		 */

		if (msgsz > msqptr->msg_qbytes) {
			MSG_PRINTF(("msgsz > msqptr->msg_qbytes\n"));
			error = EINVAL;
			goto unlock;
		}

		if (msqptr->msg_perm.mode & MSG_LOCKED) {
			MSG_PRINTF(("msqid is locked\n"));
			need_more_resources = 1;
		}
		if (msgsz + msqptr->_msg_cbytes > msqptr->msg_qbytes) {
			MSG_PRINTF(("msgsz + msg_cbytes > msg_qbytes\n"));
			need_more_resources = 1;
		}
		if (segs_needed > nfree_msgmaps) {
			MSG_PRINTF(("segs_needed > nfree_msgmaps\n"));
			need_more_resources = 1;
		}
		if (free_msghdrs == NULL) {
			MSG_PRINTF(("no more msghdrs\n"));
			need_more_resources = 1;
		}

		if (need_more_resources) {
			int we_own_it;

			if ((msgflg & IPC_NOWAIT) != 0) {
				MSG_PRINTF(("need more resources but caller "
				    "doesn't want to wait\n"));
				error = EAGAIN;
				goto unlock;
			}

			if ((msqptr->msg_perm.mode & MSG_LOCKED) != 0) {
				MSG_PRINTF(("we don't own the msqid_ds\n"));
				we_own_it = 0;
			} else {
				/* Force later arrivals to wait for our
				   request */
				MSG_PRINTF(("we own the msqid_ds\n"));
				msqptr->msg_perm.mode |= MSG_LOCKED;
				we_own_it = 1;
			}
			MSG_PRINTF(("goodnight\n"));
			error = cv_wait_sig(&msq->msq_cv, &msgmutex);
			MSG_PRINTF(("good morning, error=%d\n", error));
			if (we_own_it)
				msqptr->msg_perm.mode &= ~MSG_LOCKED;
			if (error != 0) {
				MSG_PRINTF(("msgsnd: interrupted system "
				    "call\n"));
				error = EINTR;
				goto unlock;
			}

			/*
			 * Make sure that the msq queue still exists
			 */

			if (msqptr->msg_qbytes == 0) {
				MSG_PRINTF(("msqid deleted\n"));
				error = EIDRM;
				goto unlock;
			}
		} else {
			MSG_PRINTF(("got all the resources that we need\n"));
			break;
		}
	}

	/*
	 * We have the resources that we need.
	 * Make sure!
	 */

	KASSERT((msqptr->msg_perm.mode & MSG_LOCKED) == 0);
	KASSERT(segs_needed <= nfree_msgmaps);
	KASSERT(msgsz + msqptr->_msg_cbytes <= msqptr->msg_qbytes);
	KASSERT(free_msghdrs != NULL);

	/*
	 * Re-lock the msqid_ds in case we page-fault when copying in the
	 * message
	 */

	KASSERT((msqptr->msg_perm.mode & MSG_LOCKED) == 0);
	msqptr->msg_perm.mode |= MSG_LOCKED;

	/*
	 * Allocate a message header
	 */

	msghdr = free_msghdrs;
	free_msghdrs = msghdr->msg_next;
	msghdr->msg_spot = -1;
	msghdr->msg_ts = msgsz;

	/*
	 * Allocate space for the message
	 */

	while (segs_needed > 0) {
		KASSERT(nfree_msgmaps > 0);
		KASSERT(free_msgmaps != -1);
		KASSERT(free_msgmaps < msginfo.msgseg);

		next = free_msgmaps;
		MSG_PRINTF(("allocating segment %d to message\n", next));
		free_msgmaps = msgmaps[next].next;
		nfree_msgmaps--;
		msgmaps[next].next = msghdr->msg_spot;
		msghdr->msg_spot = next;
		segs_needed--;
	}

	/*
	 * Copy in the message type
	 */
	mutex_exit(&msgmutex);
	error = (*fetch_type)(user_msgp, &msghdr->msg_type, typesz);
	mutex_enter(&msgmutex);
	if (error != 0) {
		MSG_PRINTF(("error %d copying the message type\n", error));
		msg_freehdr(msghdr);
		msqptr->msg_perm.mode &= ~MSG_LOCKED;
		cv_broadcast(&msq->msq_cv);
		goto unlock;
	}
	user_msgp += typesz;

	/*
	 * Validate the message type
	 */

	if (msghdr->msg_type < 1) {
		msg_freehdr(msghdr);
		msqptr->msg_perm.mode &= ~MSG_LOCKED;
		cv_broadcast(&msq->msq_cv);
		MSG_PRINTF(("mtype (%ld) < 1\n", msghdr->msg_type));
		goto unlock;
	}

	/*
	 * Copy in the message body
	 */

	next = msghdr->msg_spot;
	while (msgsz > 0) {
		size_t tlen;
		KASSERT(next > -1);
		KASSERT(next < msginfo.msgseg);

		if (msgsz > msginfo.msgssz)
			tlen = msginfo.msgssz;
		else
			tlen = msgsz;
		mutex_exit(&msgmutex);
		error = copyin(user_msgp, &msgpool[next * msginfo.msgssz], tlen);
		mutex_enter(&msgmutex);
		if (error != 0) {
			MSG_PRINTF(("error %d copying in message segment\n",
			    error));
			msg_freehdr(msghdr);
			msqptr->msg_perm.mode &= ~MSG_LOCKED;
			cv_broadcast(&msq->msq_cv);
			goto unlock;
		}
		msgsz -= tlen;
		user_msgp += tlen;
		next = msgmaps[next].next;
	}
	KASSERT(next == -1);

	/*
	 * We've got the message.  Unlock the msqid_ds.
	 */

	msqptr->msg_perm.mode &= ~MSG_LOCKED;

	/*
	 * Make sure that the msqid_ds is still allocated.
	 */

	if (msqptr->msg_qbytes == 0) {
		msg_freehdr(msghdr);
		cv_broadcast(&msq->msq_cv);
		error = EIDRM;
		goto unlock;
	}

	/*
	 * Put the message into the queue
	 */

	if (msqptr->_msg_first == NULL) {
		msqptr->_msg_first = msghdr;
		msqptr->_msg_last = msghdr;
	} else {
		msqptr->_msg_last->msg_next = msghdr;
		msqptr->_msg_last = msghdr;
	}
	msqptr->_msg_last->msg_next = NULL;

	msqptr->_msg_cbytes += msghdr->msg_ts;
	msqptr->msg_qnum++;
	msqptr->msg_lspid = l->l_proc->p_pid;
	msqptr->msg_stime = time_second;

	cv_broadcast(&msq->msq_cv);

unlock:
	mutex_exit(&msgmutex);
	return error;
}

int
sys_msgrcv(struct lwp *l, void *v, register_t *retval)
{
	struct sys_msgrcv_args /* {
		syscallarg(int) msqid;
		syscallarg(void *) msgp;
		syscallarg(size_t) msgsz;
		syscallarg(long) msgtyp;
		syscallarg(int) msgflg;
	} */ *uap = v;

	return msgrcv1(l, SCARG(uap, msqid), SCARG(uap, msgp),
	    SCARG(uap, msgsz), SCARG(uap, msgtyp), SCARG(uap, msgflg),
	    sizeof(long), copyout, retval);
}

int
msgrcv1(struct lwp *l, int msqidr, char *user_msgp, size_t msgsz, long msgtyp,
    int msgflg, size_t typesz, copyout_t put_type, register_t *retval)
{
	size_t len;
	kauth_cred_t cred = l->l_cred;
	struct msqid_ds *msqptr;
	struct __msg *msghdr;
	int error = 0, msqid;
	kmsq_t *msq;
	short next;

	MSG_PRINTF(("call to msgrcv(%d, %p, %lld, %ld, %d)\n", msqid,
	    user_msgp, (long long)msgsz, msgtyp, msgflg));

	msqid = IPCID_TO_IX(msqidr);

	mutex_enter(&msgmutex);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
		MSG_PRINTF(("msqid (%d) out of range (0<=msqid<%d)\n", msqid,
		    msginfo.msgmni));
		error = EINVAL;
		goto unlock;
	}

	msq = &msqs[msqid];
	msqptr = &msq->msq_u;

	if (msqptr->msg_qbytes == 0) {
		MSG_PRINTF(("no such message queue id\n"));
		error = EINVAL;
		goto unlock;
	}
	if (msqptr->msg_perm._seq != IPCID_TO_SEQ(msqidr)) {
		MSG_PRINTF(("wrong sequence number\n"));
		error = EINVAL;
		goto unlock;
	}

	if ((error = ipcperm(cred, &msqptr->msg_perm, IPC_R))) {
		MSG_PRINTF(("requester doesn't have read access\n"));
		goto unlock;
	}

	msghdr = NULL;
	while (msghdr == NULL) {
		if (msgtyp == 0) {
			msghdr = msqptr->_msg_first;
			if (msghdr != NULL) {
				if (msgsz < msghdr->msg_ts &&
				    (msgflg & MSG_NOERROR) == 0) {
					MSG_PRINTF(("first msg on the queue "
					    "is too big (want %lld, got %d)\n",
					    (long long)msgsz, msghdr->msg_ts));
					error = E2BIG;
					goto unlock;
				}
				if (msqptr->_msg_first == msqptr->_msg_last) {
					msqptr->_msg_first = NULL;
					msqptr->_msg_last = NULL;
				} else {
					msqptr->_msg_first = msghdr->msg_next;
					KASSERT(msqptr->_msg_first != NULL);
				}
			}
		} else {
			struct __msg *previous;
			struct __msg **prev;

			for (previous = NULL, prev = &msqptr->_msg_first;
			     (msghdr = *prev) != NULL;
			     previous = msghdr, prev = &msghdr->msg_next) {
				/*
				 * Is this message's type an exact match or is
				 * this message's type less than or equal to
				 * the absolute value of a negative msgtyp?
				 * Note that the second half of this test can
				 * NEVER be true if msgtyp is positive since
				 * msg_type is always positive!
				 */

				if (msgtyp != msghdr->msg_type &&
				    msghdr->msg_type > -msgtyp)
					continue;

				MSG_PRINTF(("found message type %ld, requested %ld\n",
				    msghdr->msg_type, msgtyp));
				if (msgsz < msghdr->msg_ts &&
				     (msgflg & MSG_NOERROR) == 0) {
					MSG_PRINTF(("requested message on the queue "
					    "is too big (want %lld, got %d)\n",
					    (long long)msgsz, msghdr->msg_ts));
					error = E2BIG;
					goto unlock;
				}
				*prev = msghdr->msg_next;
				if (msghdr != msqptr->_msg_last)
					break;
				if (previous == NULL) {
					KASSERT(prev == &msqptr->_msg_first);
					msqptr->_msg_first = NULL;
					msqptr->_msg_last = NULL;
				} else {
					KASSERT(prev != &msqptr->_msg_first);
					msqptr->_msg_last = previous;
				}
				break;
			}
		}

		/*
		 * We've either extracted the msghdr for the appropriate
		 * message or there isn't one.
		 * If there is one then bail out of this loop.
		 */
		if (msghdr != NULL)
			break;

		/*
		 * Hmph!  No message found.  Does the user want to wait?
		 */

		if ((msgflg & IPC_NOWAIT) != 0) {
			MSG_PRINTF(("no appropriate message found (msgtyp=%ld)\n",
			    msgtyp));
			error = ENOMSG;
			goto unlock;
		}

		/*
		 * Wait for something to happen
		 */

		MSG_PRINTF(("msgrcv:  goodnight\n"));
		error = cv_wait_sig(&msq->msq_cv, &msgmutex);
		MSG_PRINTF(("msgrcv: good morning (error=%d)\n", error));

		if (error != 0) {
			MSG_PRINTF(("msgsnd: interrupted system call\n"));
			error = EINTR;
			goto unlock;
		}

		/*
		 * Make sure that the msq queue still exists
		 */

		if (msqptr->msg_qbytes == 0 ||
		    msqptr->msg_perm._seq != IPCID_TO_SEQ(msqidr)) {
			MSG_PRINTF(("msqid deleted\n"));
			error = EIDRM;
			goto unlock;
		}
	}

	/*
	 * Return the message to the user.
	 *
	 * First, do the bookkeeping (before we risk being interrupted).
	 */

	msqptr->_msg_cbytes -= msghdr->msg_ts;
	msqptr->msg_qnum--;
	msqptr->msg_lrpid = l->l_proc->p_pid;
	msqptr->msg_rtime = time_second;

	/*
	 * Make msgsz the actual amount that we'll be returning.
	 * Note that this effectively truncates the message if it is too long
	 * (since msgsz is never increased).
	 */

	MSG_PRINTF(("found a message, msgsz=%lld, msg_ts=%d\n",
	    (long long)msgsz, msghdr->msg_ts));
	if (msgsz > msghdr->msg_ts)
		msgsz = msghdr->msg_ts;

	/*
	 * Return the type to the user.
	 */
	mutex_exit(&msgmutex);
	error = (*put_type)(&msghdr->msg_type, user_msgp, typesz);
	mutex_enter(&msgmutex);
	if (error != 0) {
		MSG_PRINTF(("error (%d) copying out message type\n", error));
		msg_freehdr(msghdr);
		cv_broadcast(&msq->msq_cv);
		goto unlock;
	}
	user_msgp += typesz;

	/*
	 * Return the segments to the user
	 */

	next = msghdr->msg_spot;
	for (len = 0; len < msgsz; len += msginfo.msgssz) {
		size_t tlen;
		KASSERT(next > -1);
		KASSERT(next < msginfo.msgseg);

		if (msgsz - len > msginfo.msgssz)
			tlen = msginfo.msgssz;
		else
			tlen = msgsz - len;
		mutex_exit(&msgmutex);
		error = copyout(&msgpool[next * msginfo.msgssz],
		    user_msgp, tlen);
		mutex_enter(&msgmutex);
		if (error != 0) {
			MSG_PRINTF(("error (%d) copying out message segment\n",
			    error));
			msg_freehdr(msghdr);
			cv_broadcast(&msq->msq_cv);
			goto unlock;
		}
		user_msgp += tlen;
		next = msgmaps[next].next;
	}

	/*
	 * Done, return the actual number of bytes copied out.
	 */

	msg_freehdr(msghdr);
	cv_broadcast(&msq->msq_cv);
	*retval = msgsz;

unlock:
	mutex_exit(&msgmutex);
	return error;
}
