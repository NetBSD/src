/*	$NetBSD: kern_proc.c,v 1.41.4.2 2002/04/26 17:56:40 he Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_proc.c	8.7 (Berkeley) 2/14/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/signalvar.h>

/*
 * Structure associated with user cacheing.
 */
struct uidinfo {
	LIST_ENTRY(uidinfo) ui_hash;
	uid_t	ui_uid;
	long	ui_proccnt;
};
#define	UIHASH(uid)	(&uihashtbl[(uid) & uihash])
LIST_HEAD(uihashhead, uidinfo) *uihashtbl;
u_long uihash;		/* size of hash table - 1 */

/*
 * Other process lists
 */
struct pidhashhead *pidhashtbl;
u_long pidhash;
struct pgrphashhead *pgrphashtbl;
u_long pgrphash;

struct proclist allproc;
struct proclist zombproc;	/* resources have been freed */

/*
 * Process list locking:
 *
 * We have two types of locks on the proclists: read locks and write
 * locks.  Read locks can be used in interrupt context, so while we
 * hold the write lock, we must also block clock interrupts to
 * lock out any scheduling changes that may happen in interrupt
 * context.
 *
 * The proclist lock locks the following structures:
 *
 *	allproc
 *	zombproc
 *	pidhashtbl
 */
struct lock proclist_lock;

/*
 * Locking of this proclist is special; it's accessed in a
 * critical section of process exit, and thus locking it can't
 * modify interrupt state.  We use a simple spin lock for this
 * proclist.  Processes on this proclist are also on zombproc;
 * we use the p_hash member to linkup to deadproc.
 */
struct simplelock deadproc_slock;
struct proclist deadproc;	/* dead, but not yet undead */

struct pool proc_pool;
struct pool pcred_pool;
struct pool plimit_pool;
struct pool pgrp_pool;
struct pool rusage_pool;

/*
 * The process list descriptors, used during pid allocation and
 * by sysctl.  No locking on this data structure is needed since
 * it is completely static.
 */
const struct proclist_desc proclists[] = {
	{ &allproc	},
	{ &zombproc	},
	{ NULL		},
};

static void orphanpg __P((struct pgrp *));
#ifdef DEBUG
void pgrpdump __P((void));
#endif

/*
 * Initialize global process hashing structures.
 */
void
procinit()
{
	const struct proclist_desc *pd;

	for (pd = proclists; pd->pd_list != NULL; pd++)
		LIST_INIT(pd->pd_list);

	spinlockinit(&proclist_lock, "proclk", 0);

	LIST_INIT(&deadproc);
	simple_lock_init(&deadproc_slock);

	pidhashtbl = hashinit(maxproc / 4, M_PROC, M_WAITOK, &pidhash);
	pgrphashtbl = hashinit(maxproc / 4, M_PROC, M_WAITOK, &pgrphash);
	uihashtbl = hashinit(maxproc / 16, M_PROC, M_WAITOK, &uihash);

	pool_init(&proc_pool, sizeof(struct proc), 0, 0, 0, "procpl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_PROC);
	pool_init(&pgrp_pool, sizeof(struct pgrp), 0, 0, 0, "pgrppl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_PGRP);
	pool_init(&pcred_pool, sizeof(struct pcred), 0, 0, 0, "pcredpl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_SUBPROC);
	pool_init(&plimit_pool, sizeof(struct plimit), 0, 0, 0, "plimitpl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_SUBPROC);
	pool_init(&rusage_pool, sizeof(struct rusage), 0, 0, 0, "rusgepl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_ZOMBIE);
}

/*
 * Acquire a read lock on the proclist.
 */
void
proclist_lock_read()
{
	int error, s;

	s = splclock();
	error = spinlockmgr(&proclist_lock, LK_SHARED, NULL);
#ifdef DIAGNOSTIC
	if (__predict_false(error != 0))
		panic("proclist_lock_read: failed to acquire lock");
#endif
	splx(s);
}

/*
 * Release a read lock on the proclist.
 */
void
proclist_unlock_read()
{
	int s;

	s = splclock();
	(void) spinlockmgr(&proclist_lock, LK_RELEASE, NULL);
	splx(s);
}

/*
 * Acquire a write lock on the proclist.
 */
int
proclist_lock_write()
{
	int error, s;

	s = splclock();
	error = spinlockmgr(&proclist_lock, LK_EXCLUSIVE, NULL);
#ifdef DIAGNOSTIC
	if (__predict_false(error != 0))
		panic("proclist_lock: failed to acquire lock");
#endif
	return (s);
}

/*
 * Release a write lock on the proclist.
 */
void
proclist_unlock_write(s)
	int s;
{

	(void) spinlockmgr(&proclist_lock, LK_RELEASE, NULL);
	splx(s);
}

/*
 * Change the count associated with number of processes
 * a given user is using.
 */
int
chgproccnt(uid, diff)
	uid_t	uid;
	int	diff;
{
	struct uidinfo *uip;
	struct uihashhead *uipp;

	uipp = UIHASH(uid);
	for (uip = uipp->lh_first; uip != 0; uip = uip->ui_hash.le_next)
		if (uip->ui_uid == uid)
			break;
	if (uip) {
		uip->ui_proccnt += diff;
		if (uip->ui_proccnt > 0)
			return (uip->ui_proccnt);
		if (uip->ui_proccnt < 0)
			panic("chgproccnt: procs < 0");
		LIST_REMOVE(uip, ui_hash);
		FREE(uip, M_PROC);
		return (0);
	}
	if (diff <= 0) {
		if (diff == 0)
			return(0);
		panic("chgproccnt: lost user");
	}
	MALLOC(uip, struct uidinfo *, sizeof(*uip), M_PROC, M_WAITOK);
	LIST_INSERT_HEAD(uipp, uip, ui_hash);
	uip->ui_uid = uid;
	uip->ui_proccnt = diff;
	return (diff);
}

/*
 * Is p an inferior of q?
 */
int
inferior(p, q)
	struct proc *p;
	struct proc *q;
{

	for (; p != q; p = p->p_pptr)
		if (p->p_pid == 0)
			return (0);
	return (1);
}

/*
 * Locate a process by number
 */
struct proc *
pfind(pid)
	pid_t pid;
{
	struct proc *p;

	proclist_lock_read();
	for (p = PIDHASH(pid)->lh_first; p != 0; p = p->p_hash.le_next)
		if (p->p_pid == pid)
			goto out;
 out:
	proclist_unlock_read();
	return (p);
}

/*
 * Locate a process group by number
 */
struct pgrp *
pgfind(pgid)
	pid_t pgid;
{
	struct pgrp *pgrp;

	for (pgrp = PGRPHASH(pgid)->lh_first; pgrp != 0; pgrp = pgrp->pg_hash.le_next)
		if (pgrp->pg_id == pgid)
			return (pgrp);
	return (NULL);
}

/*
 * Move p to a new or existing process group (and session)
 */
int
enterpgrp(p, pgid, mksess)
	struct proc *p;
	pid_t pgid;
	int mksess;
{
	struct pgrp *pgrp = pgfind(pgid);

#ifdef DIAGNOSTIC
	if (__predict_false(pgrp != NULL && mksess))	/* firewalls */
		panic("enterpgrp: setsid into non-empty pgrp");
	if (__predict_false(SESS_LEADER(p)))
		panic("enterpgrp: session leader attempted setpgrp");
#endif
	if (pgrp == NULL) {
		pid_t savepid = p->p_pid;
		struct proc *np;
		/*
		 * new process group
		 */
#ifdef DIAGNOSTIC
		if (__predict_false(p->p_pid != pgid))
			panic("enterpgrp: new pgrp and pid != pgid");
#endif
		pgrp = pool_get(&pgrp_pool, PR_WAITOK);
		if ((np = pfind(savepid)) == NULL || np != p)
			return (ESRCH);
		if (mksess) {
			struct session *sess;

			/*
			 * new session
			 */
			MALLOC(sess, struct session *, sizeof(struct session),
			    M_SESSION, M_WAITOK);
			sess->s_sid = p->p_pid;
			sess->s_leader = p;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			memcpy(sess->s_login, p->p_session->s_login,
			    sizeof(sess->s_login));
			p->p_flag &= ~P_CONTROLT;
			pgrp->pg_session = sess;
#ifdef DIAGNOSTIC
			if (__predict_false(p != curproc))
				panic("enterpgrp: mksession and p != curproc");
#endif
		} else {
			SESSHOLD(p->p_session);
			pgrp->pg_session = p->p_session;
		}
		pgrp->pg_id = pgid;
		LIST_INIT(&pgrp->pg_members);
		LIST_INSERT_HEAD(PGRPHASH(pgid), pgrp, pg_hash);
		pgrp->pg_jobc = 0;
	} else if (pgrp == p->p_pgrp)
		return (0);

	/*
	 * Adjust eligibility of affected pgrps to participate in job control.
	 * Increment eligibility counts before decrementing, otherwise we
	 * could reach 0 spuriously during the first call.
	 */
	fixjobc(p, pgrp, 1);
	fixjobc(p, p->p_pgrp, 0);

	LIST_REMOVE(p, p_pglist);
	if (p->p_pgrp->pg_members.lh_first == 0)
		pgdelete(p->p_pgrp);
	p->p_pgrp = pgrp;
	LIST_INSERT_HEAD(&pgrp->pg_members, p, p_pglist);
	return (0);
}

/*
 * remove process from process group
 */
int
leavepgrp(p)
	struct proc *p;
{

	LIST_REMOVE(p, p_pglist);
	if (p->p_pgrp->pg_members.lh_first == 0)
		pgdelete(p->p_pgrp);
	p->p_pgrp = 0;
	return (0);
}

/*
 * delete a process group
 */
void
pgdelete(pgrp)
	struct pgrp *pgrp;
{

	/* Remove reference (if any) from tty to this process group */
	if (pgrp->pg_session->s_ttyp != NULL && 
	    pgrp->pg_session->s_ttyp->t_pgrp == pgrp)
		pgrp->pg_session->s_ttyp->t_pgrp = NULL;
	LIST_REMOVE(pgrp, pg_hash);
	SESSRELE(pgrp->pg_session);
	pool_put(&pgrp_pool, pgrp);
}

/*
 * Adjust pgrp jobc counters when specified process changes process group.
 * We count the number of processes in each process group that "qualify"
 * the group for terminal job control (those with a parent in a different
 * process group of the same session).  If that count reaches zero, the
 * process group becomes orphaned.  Check both the specified process'
 * process group and that of its children.
 * entering == 0 => p is leaving specified group.
 * entering == 1 => p is entering specified group.
 */
void
fixjobc(p, pgrp, entering)
	struct proc *p;
	struct pgrp *pgrp;
	int entering;
{
	struct pgrp *hispgrp;
	struct session *mysession = pgrp->pg_session;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	if ((hispgrp = p->p_pptr->p_pgrp) != pgrp &&
	    hispgrp->pg_session == mysession) {
		if (entering)
			pgrp->pg_jobc++;
		else if (--pgrp->pg_jobc == 0)
			orphanpg(pgrp);
	}

	/*
	 * Check this process' children to see whether they qualify
	 * their process groups; if so, adjust counts for children's
	 * process groups.
	 */
	for (p = p->p_children.lh_first; p != 0; p = p->p_sibling.le_next) {
		if ((hispgrp = p->p_pgrp) != pgrp &&
		    hispgrp->pg_session == mysession &&
		    P_ZOMBIE(p) == 0) {
			if (entering)
				hispgrp->pg_jobc++;
			else if (--hispgrp->pg_jobc == 0)
				orphanpg(hispgrp);
		}
	}
}

/* 
 * A process group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 */
static void
orphanpg(pg)
	struct pgrp *pg;
{
	struct proc *p;

	for (p = pg->pg_members.lh_first; p != 0; p = p->p_pglist.le_next) {
		if (p->p_stat == SSTOP) {
			for (p = pg->pg_members.lh_first; p != 0;
			    p = p->p_pglist.le_next) {
				psignal(p, SIGHUP);
				psignal(p, SIGCONT);
			}
			return;
		}
	}
}

/* mark process as suid/sgid, reset some values do defaults */
void
p_sugid(p)
	struct proc *p;
{
	struct plimit *newlim;

	p->p_flag |= P_SUGID;
	/* reset what needs to be reset in plimit */
	if (p->p_limit->pl_corename != defcorename) {
		if (p->p_limit->p_refcnt > 1 &&
		    (p->p_limit->p_lflags & PL_SHAREMOD) == 0) {
			newlim = limcopy(p->p_limit);
			limfree(p->p_limit);
			p->p_limit = newlim;
		} else {
			free(p->p_limit->pl_corename, M_TEMP);
		}
		p->p_limit->pl_corename = defcorename;
	}
}


#ifdef DEBUG
void
pgrpdump()
{
	struct pgrp *pgrp;
	struct proc *p;
	int i;

	for (i = 0; i <= pgrphash; i++) {
		if ((pgrp = pgrphashtbl[i].lh_first) != NULL) {
			printf("\tindx %d\n", i);
			for (; pgrp != 0; pgrp = pgrp->pg_hash.le_next) {
				printf("\tpgrp %p, pgid %d, sess %p, sesscnt %d, mem %p\n",
				    pgrp, pgrp->pg_id, pgrp->pg_session,
				    pgrp->pg_session->s_count,
				    pgrp->pg_members.lh_first);
				for (p = pgrp->pg_members.lh_first; p != 0;
				    p = p->p_pglist.le_next) {
					printf("\t\tpid %d addr %p pgrp %p\n", 
					    p->p_pid, p, p->p_pgrp);
				}
			}
		}
	}
}
#endif /* DEBUG */
