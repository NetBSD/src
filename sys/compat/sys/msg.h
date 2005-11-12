/*	$NetBSD: msg.h,v 1.2 2005/11/12 00:39:22 simonb Exp $	*/

/*
 * SVID compatible msg.h file
 *
 * Author:  Daniel Boulet
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

#ifndef _COMPAT_SYS_MSG_H_
#define _COMPAT_SYS_MSG_H_

#ifdef _KERNEL
#include <compat/sys/ipc.h>
/*
 * Old message queue data structure used before NetBSD 1.5.
 */
struct msqid_ds14 {
	struct	ipc_perm14 msg_perm;	/* msg queue permission bits */
	struct	__msg *msg_first;	/* first message in the queue */
	struct	__msg *msg_last;	/* last message in the queue */
	u_long	msg_cbytes;	/* number of bytes in use on the queue */
	u_long	msg_qnum;	/* number of msgs in the queue */
	u_long	msg_qbytes;	/* max # of bytes on the queue */
	pid_t	msg_lspid;	/* pid of last msgsnd() */
	pid_t	msg_lrpid;	/* pid of last msgrcv() */
	time_t	msg_stime;	/* time of last msgsnd() */
	long	msg_pad1;
	time_t	msg_rtime;	/* time of last msgrcv() */
	long	msg_pad2;
	time_t	msg_ctime;	/* time of last msgctl() */
	long	msg_pad3;
	long	msg_pad4[4];
};
#endif

#endif /* !_COMPAT_SYS_MSG_H_ */
