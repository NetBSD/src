/*	$NetBSD: altq_localq.c,v 1.3.2.1 2001/10/10 11:55:47 fvdl Exp $	*/
/*	$KAME: altq_localq.c,v 1.4 2001/08/16 11:28:25 kjc Exp $	*/
/*
 * a skeleton file for implementing a new queueing discipline.
 * this file is in the public domain.
 */

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include "opt_altq.h"
#endif /* __FreeBSD__ || __NetBSD__ */
#ifdef ALTQ_LOCALQ  /* localq is enabled by ALTQ_LOCALQ option in opt_altq.h */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/vnode.h>

#include <net/if.h>
#include <netinet/in.h>

#include <altq/altq.h>
#include <altq/altq_conf.h>

/*
 * localq device interface
 */
altqdev_decl(localq);

int
localqopen(devvp, flag, fmt, p)
	struct vnode *devvp;
	int flag, fmt;
	struct proc *p;
{
	/* everything will be done when the queueing scheme is attached. */
	return 0;
}

int
localqclose(devvp, flag, fmt, p)
	struct vnode *devvp;
	int flag, fmt;
	struct proc *p;
{
	int error = 0;

	return error;
}

int
localqioctl(devvp, cmd, addr, flag, p)
	struct vnode *devvp;
	ioctlcmd_t cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	int error = 0;
	
	return error;
}

#ifdef KLD_MODULE

static struct altqsw localq_sw =
	{"localq", localqopen, localqclose, localqioctl};

ALTQ_MODULE(altq_localq, ALTQT_LOCALQ, &localq_sw);

#endif /* KLD_MODULE */

#endif /* ALTQ_LOCALQ */
