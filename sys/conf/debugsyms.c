/*	$NetBSD: debugsyms.c,v 1.3.22.1 2017/12/03 11:36:57 jdolecek Exp $	*/
/*
 * This file is in the public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: debugsyms.c,v 1.3.22.1 2017/12/03 11:36:57 jdolecek Exp $");

#define	_CALLOUT_PRIVATE
#define	__MUTEX_PRIVATE
#define	__KAUTH_PRIVATE

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/time.h>
#include <sys/ucontext.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/unpcb.h>
#include <sys/vnode.h>
#include <sys/specificdata.h>
#include <sys/kauth.h>

/*
 * Without a dummy function referencing some of the types, gcc will
 * not emit any debug information.
 */
proc_t	*_debugsym_dummyfunc(vnode_t *vp);

proc_t *
_debugsym_dummyfunc(vnode_t *vp)
{
	struct kauth_cred *cr = (kauth_cred_t)vp;

	return cr->cr_uid ? ((lwp_t *)vp->v_mount)->l_proc : NULL;
}
