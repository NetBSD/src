/*	$NetBSD: smb_lock.c,v 1.1.2.2 2001/01/08 14:58:09 bouyer Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>

#include <netsmb/smb_lock.h>

int
simple_spinlock_init(struct simplespinlock *slp, char *name, int prio)
{
	bzero(slp, sizeof(*slp));
	simple_lock_init(&slp->sl_interlock);
	slp->sl_name = name;
	prio = prio;
	return 0;
}

int
simple_spinlock(struct simplespinlock *slp, int intr)
{
	int flags = slp->sl_prio;
	int error;

	if (intr)
		flags |= PCATCH;
  try_again:
	simple_lock(&slp->sl_interlock);
	if (++slp->sl_count == 1) {
		simple_unlock(&slp->sl_interlock);
		return 0;
	}
#ifndef NetBSD
	asleep(&slp->sl_count, flags, slp->sl_name, 0);
	simple_unlock(&slp->sl_interlock);
	error = await(flags, 0);
#else
	simple_unlock(&slp->sl_interlock);
	error = tsleep(&slp->sl_count, flags, slp->sl_name, 0);
#endif
	if (error) {
		simple_lock(&slp->sl_interlock);
		slp->sl_count--;
		simple_unlock(&slp->sl_interlock);
		if (error == ERESTART)
			error = EINTR;
	}
#ifdef NetBSD
	else
		goto try_again;
#endif
	return error;
}

void
simple_spinunlock(struct simplespinlock *slp)
{
	simple_lock(&slp->sl_interlock);
	if (slp->sl_count == 0)
		panic("spinunlock(): negative lock count\n");
#ifndef NetBSD
	if (--slp->sl_count != 0)
		wakeup_one(&slp->sl_count);
	simple_unlock(&slp->sl_interlock);
#else
	--slp->sl_count;
	simple_unlock(&slp->sl_interlock);
	if (slp->sl_count != 0)
		wakeup(&slp->sl_count);
#endif
	return;
}
