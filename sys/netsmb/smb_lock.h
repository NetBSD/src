/*	$NetBSD: smb_lock.h,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

#ifndef _NETSMB_SMB_LOCK_H_
#define _NETSMB_SMB_LOCK_H_

/*
 * Temporary, until the real SMP stuff appears...
 */
struct simplespinlock {
	struct simplelock sl_interlock;
	char *		sl_name;
	int		sl_prio;
	int		sl_count;
};

int  simple_spinlock_init(struct simplespinlock *slp, char *name, int prio);
int  simple_spinlock(struct simplespinlock *slp, int intr);
void simple_spinunlock(struct simplespinlock *slp);

#endif /* !_NETSMB_SMB_LOCK_H_ */
