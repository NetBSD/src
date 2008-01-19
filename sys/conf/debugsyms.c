/*	$NetBSD: debugsyms.c,v 1.1.2.2 2008/01/19 12:15:01 bouyer Exp $	*/
/*
 * This file is in the public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: debugsyms.c,v 1.1.2.2 2008/01/19 12:15:01 bouyer Exp $");

#define	_CALLOUT_PRIVATE
#define	__MUTEX_PRIVATE

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
#include <sys/user.h>
#include <sys/vnode.h>
