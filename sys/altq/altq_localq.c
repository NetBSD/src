/*	$NetBSD: altq_localq.c,v 1.7 2006/10/12 01:30:42 christos Exp $	*/
/*	$KAME: altq_localq.c,v 1.4 2001/08/16 11:28:25 kjc Exp $	*/
/*
 * a skeleton file for implementing a new queueing discipline.
 * this file is in the public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altq_localq.c,v 1.7 2006/10/12 01:30:42 christos Exp $");

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include "opt_altq.h"
#endif /* __FreeBSD__ || __NetBSD__ */
#ifdef ALTQ_LOCALQ  /* localq is enabled by ALTQ_LOCALQ option in opt_altq.h */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <netinet/in.h>

#include <altq/altq.h>
#include <altq/altq_conf.h>

/*
 * localq device interface
 */
altqdev_decl(localq);

int
localqopen(dev_t dev __unused, int flag __unused, int fmt __unused,
    struct lwp *l __unused)
{
	/* everything will be done when the queueing scheme is attached. */
	return 0;
}

int
localqclose(dev_t dev __unused, int flag __unused, int fmt __unused,
    struct lwp *l __unused)
{
	int error = 0;

	return error;
}

int
localqioctl(dev_t dev __unused, ioctlcmd_t cmd __unused, caddr_t addr __unused,
    int flag __unused, struct lwp *l __unused)
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
