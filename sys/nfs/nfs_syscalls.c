/*	$NetBSD: nfs_syscalls.c,v 1.107.2.5 2007/04/10 12:07:14 ad Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)nfs_syscalls.c	8.5 (Berkeley) 3/30/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfs_syscalls.c,v 1.107.2.5 2007/04/10 12:07:14 ad Exp $");

#include "fs_nfs.h"
#include "opt_nfs.h"
#include "opt_nfsserver.h"
#include "opt_iso.h"
#include "opt_inet.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/signalvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/namei.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/kthread.h>
#include <sys/kauth.h>
#include <sys/syscallargs.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef ISO
#include <netiso/iso.h>
#endif
#include <nfs/xdr_subs.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsrvcache.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsnode.h>
#include <nfs/nfsrtt.h>
#include <nfs/nfs_var.h>

/* Global defs. */
extern int32_t (*nfsrv3_procs[NFS_NPROCS]) __P((struct nfsrv_descript *,
						struct nfssvc_sock *,
						struct lwp *, struct mbuf **));
extern int nfsrvw_procrastinate;

struct nfssvc_sock *nfs_udpsock;
#ifdef ISO
struct nfssvc_sock *nfs_cltpsock;
#endif
#ifdef INET6
struct nfssvc_sock *nfs_udp6sock;
#endif
int nuidhash_max = NFS_MAXUIDHASH;
int nfsd_waiting = 0;
#ifdef NFSSERVER
static int nfs_numnfsd = 0;
static struct nfsdrt nfsdrt;
#endif

#ifdef NFSSERVER
struct simplelock nfsd_slock = SIMPLELOCK_INITIALIZER;
struct nfssvc_sockhead nfssvc_sockhead;
struct nfssvc_sockhead nfssvc_sockpending;
struct nfsdhead nfsd_head;
struct nfsdidlehead nfsd_idle_head;

int nfssvc_sockhead_flag;
int nfsd_head_flag;
#endif

MALLOC_DEFINE(M_NFSUID, "NFS uid", "Nfs uid mapping structure");

#ifdef NFS
struct nfs_iod nfs_asyncdaemon[NFS_MAXASYNCDAEMON];
int nfs_niothreads = -1; /* == "0, and has never been set" */
#endif

#ifdef NFSSERVER
static struct nfssvc_sock *nfsrv_sockalloc __P((void));
static void nfsrv_sockfree __P((struct nfssvc_sock *));
static void nfsd_rt __P((int, struct nfsrv_descript *, int));
#endif

/*
 * NFS server system calls
 */


/*
 * Nfs server pseudo system call for the nfsd's
 * Based on the flag value it either:
 * - adds a socket to the selection list
 * - remains in the kernel as an nfsd
 * - remains in the kernel as an nfsiod
 */
int
sys_nfssvc(struct lwp *l, void *v, register_t *retval)
{
	struct sys_nfssvc_args /* {
		syscallarg(int) flag;
		syscallarg(void *) argp;
	} */ *uap = v;
	int error;
#ifdef NFSSERVER
	int s;
	struct file *fp;
	struct mbuf *nam;
	struct nfsd_args nfsdarg;
	struct nfsd_srvargs nfsd_srvargs, *nsd = &nfsd_srvargs;
	struct nfsd *nfsd;
	struct nfssvc_sock *slp;
	struct nfsuid *nuidp;
#endif

	/*
	 * Must be super user
	 */
	error = kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    NULL);
	if (error)
		return (error);

	/* Initialize NFS server / client shared data. */
	nfs_init();

#ifdef NFSSERVER
	s = splsoftnet();
	simple_lock(&nfsd_slock);
	while (nfssvc_sockhead_flag & SLP_INIT) {
		nfssvc_sockhead_flag |= SLP_WANTINIT;
		(void) ltsleep(&nfssvc_sockhead, PSOCK, "nfsd init", 0,
		    &nfsd_slock);
	}
	simple_unlock(&nfsd_slock);
	splx(s);
#endif
	if (SCARG(uap, flag) & NFSSVC_BIOD) {
#if defined(NFS) && defined(COMPAT_14)
		error = nfssvc_iod(l);
#else
		error = ENOSYS;
#endif
	} else if (SCARG(uap, flag) & NFSSVC_MNTD) {
		error = ENOSYS;
	} else if (SCARG(uap, flag) & NFSSVC_ADDSOCK) {
#ifndef NFSSERVER
		error = ENOSYS;
#else
		error = copyin(SCARG(uap, argp), (void *)&nfsdarg,
		    sizeof(nfsdarg));
		if (error)
			return (error);
		/* getsock() will use the descriptor for us */
		error = getsock(l->l_proc->p_fd, nfsdarg.sock, &fp);
		if (error)
			return (error);
		/*
		 * Get the client address for connected sockets.
		 */
		if (nfsdarg.name == NULL || nfsdarg.namelen == 0)
			nam = (struct mbuf *)0;
		else {
			error = sockargs(&nam, nfsdarg.name, nfsdarg.namelen,
				MT_SONAME);
			if (error) {
				FILE_UNUSE(fp, NULL);
				return (error);
			}
		}
		error = nfssvc_addsock(fp, nam);
		FILE_UNUSE(fp, NULL);
#endif /* !NFSSERVER */
	} else if (SCARG(uap, flag) & NFSSVC_SETEXPORTSLIST) {
#ifndef NFSSERVER
		error = ENOSYS;
#else
		struct export_args *args;
		struct mountd_exports_list mel;

		error = copyin(SCARG(uap, argp), &mel, sizeof(mel));
		if (error != 0)
			return error;

		args = (struct export_args *)malloc(mel.mel_nexports *
		    sizeof(struct export_args), M_TEMP, M_WAITOK);
		error = copyin(mel.mel_exports, args, mel.mel_nexports *
		    sizeof(struct export_args));
		if (error != 0) {
			free(args, M_TEMP);
			return error;
		}
		mel.mel_exports = args;

		error = mountd_set_exports_list(&mel, l);

		free(args, M_TEMP);
#endif /* !NFSSERVER */
	} else {
#ifndef NFSSERVER
		error = ENOSYS;
#else
		error = copyin(SCARG(uap, argp), (void *)nsd, sizeof (*nsd));
		if (error)
			return (error);
		if ((SCARG(uap, flag) & NFSSVC_AUTHIN) &&
		    ((nfsd = nsd->nsd_nfsd)) != NULL &&
		    (nfsd->nfsd_slp->ns_flag & SLP_VALID)) {
			slp = nfsd->nfsd_slp;

			/*
			 * First check to see if another nfsd has already
			 * added this credential.
			 */
			LIST_FOREACH(nuidp, NUIDHASH(slp, nsd->nsd_cr.cr_uid),
			    nu_hash) {
				if (kauth_cred_geteuid(nuidp->nu_cr) ==
				    nsd->nsd_cr.cr_uid &&
				    (!nfsd->nfsd_nd->nd_nam2 ||
				     netaddr_match(NU_NETFAM(nuidp),
				     &nuidp->nu_haddr, nfsd->nfsd_nd->nd_nam2)))
					break;
			}
			if (nuidp) {
			    kauth_cred_hold(nuidp->nu_cr);
			    nfsd->nfsd_nd->nd_cr = nuidp->nu_cr;
			    nfsd->nfsd_nd->nd_flag |= ND_KERBFULL;
			} else {
			    /*
			     * Nope, so we will.
			     */
			    if (slp->ns_numuids < nuidhash_max) {
				slp->ns_numuids++;
				nuidp = (struct nfsuid *)
				   malloc(sizeof (struct nfsuid), M_NFSUID,
					M_WAITOK);
			    } else
				nuidp = (struct nfsuid *)0;
			    if ((slp->ns_flag & SLP_VALID) == 0) {
				if (nuidp)
				    free((void *)nuidp, M_NFSUID);
			    } else {
				if (nuidp == (struct nfsuid *)0) {
				    nuidp = TAILQ_FIRST(&slp->ns_uidlruhead);
				    LIST_REMOVE(nuidp, nu_hash);
				    TAILQ_REMOVE(&slp->ns_uidlruhead, nuidp,
					nu_lru);
				    if (nuidp->nu_flag & NU_NAM)
					m_freem(nuidp->nu_nam);
			        }
				nuidp->nu_flag = 0;
				kauth_uucred_to_cred(nuidp->nu_cr,
				    &nsd->nsd_cr);
				nuidp->nu_timestamp = nsd->nsd_timestamp;
				nuidp->nu_expire = time_second + nsd->nsd_ttl;
				/*
				 * and save the session key in nu_key.
				 */
				memcpy(nuidp->nu_key, nsd->nsd_key,
				    sizeof(nsd->nsd_key));
				if (nfsd->nfsd_nd->nd_nam2) {
				    struct sockaddr_in *saddr;

				    saddr = mtod(nfsd->nfsd_nd->nd_nam2,
					 struct sockaddr_in *);
				    switch (saddr->sin_family) {
				    case AF_INET:
					nuidp->nu_flag |= NU_INETADDR;
					nuidp->nu_inetaddr =
					     saddr->sin_addr.s_addr;
					break;
				    case AF_ISO:
				    default:
					nuidp->nu_flag |= NU_NAM;
					nuidp->nu_nam = m_copym(
					    nfsd->nfsd_nd->nd_nam2, 0,
					     M_COPYALL, M_WAIT);
					break;
				    };
				}
				TAILQ_INSERT_TAIL(&slp->ns_uidlruhead, nuidp,
					nu_lru);
				LIST_INSERT_HEAD(NUIDHASH(slp, nsd->nsd_uid),
					nuidp, nu_hash);
				kauth_cred_hold(nuidp->nu_cr);
				nfsd->nfsd_nd->nd_cr = nuidp->nu_cr;
				nfsd->nfsd_nd->nd_flag |= ND_KERBFULL;
			    }
			}
		}
		if ((SCARG(uap, flag) & NFSSVC_AUTHINFAIL) &&
		    (nfsd = nsd->nsd_nfsd))
			nfsd->nfsd_flag |= NFSD_AUTHFAIL;
		error = nfssvc_nfsd(nsd, SCARG(uap, argp), l);
#endif /* !NFSSERVER */
	}
	if (error == EINTR || error == ERESTART)
		error = 0;
	return (error);
}

#ifdef NFSSERVER
MALLOC_DEFINE(M_NFSD, "NFS daemon", "Nfs server daemon structure");
MALLOC_DEFINE(M_NFSSVC, "NFS srvsock", "Nfs server structure");

static struct nfssvc_sock *
nfsrv_sockalloc()
{
	struct nfssvc_sock *slp;
	int s;

	slp = (struct nfssvc_sock *)
	    malloc(sizeof (struct nfssvc_sock), M_NFSSVC, M_WAITOK);
	memset(slp, 0, sizeof (struct nfssvc_sock));
	simple_lock_init(&slp->ns_lock);
	TAILQ_INIT(&slp->ns_uidlruhead);
	LIST_INIT(&slp->ns_tq);
	SIMPLEQ_INIT(&slp->ns_sendq);
	s = splsoftnet();
	simple_lock(&nfsd_slock);
	TAILQ_INSERT_TAIL(&nfssvc_sockhead, slp, ns_chain);
	simple_unlock(&nfsd_slock);
	splx(s);

	return slp;
}

static void
nfsrv_sockfree(struct nfssvc_sock *slp)
{

	KASSERT(slp->ns_so == NULL);
	KASSERT(slp->ns_fp == NULL);
	KASSERT((slp->ns_flag & SLP_VALID) == 0);
	free(slp, M_NFSSVC);
}

/*
 * Adds a socket to the list for servicing by nfsds.
 */
int
nfssvc_addsock(fp, mynam)
	struct file *fp;
	struct mbuf *mynam;
{
	struct mbuf *m;
	int siz;
	struct nfssvc_sock *slp;
	struct socket *so;
	struct nfssvc_sock *tslp;
	int error, s;

	so = (struct socket *)fp->f_data;
	tslp = (struct nfssvc_sock *)0;
	/*
	 * Add it to the list, as required.
	 */
	if (so->so_proto->pr_protocol == IPPROTO_UDP) {
#ifdef INET6
		if (so->so_proto->pr_domain->dom_family == AF_INET6)
			tslp = nfs_udp6sock;
		else
#endif
		tslp = nfs_udpsock;
		if (tslp->ns_flag & SLP_VALID) {
			m_freem(mynam);
			return (EPERM);
		}
#ifdef ISO
	} else if (so->so_proto->pr_protocol == ISOPROTO_CLTP) {
		tslp = nfs_cltpsock;
		if (tslp->ns_flag & SLP_VALID) {
			m_freem(mynam);
			return (EPERM);
		}
#endif /* ISO */
	}
	if (so->so_type == SOCK_STREAM)
		siz = NFS_MAXPACKET + sizeof (u_long);
	else
		siz = NFS_MAXPACKET;
	error = soreserve(so, siz, siz);
	if (error) {
		m_freem(mynam);
		return (error);
	}

	/*
	 * Set protocol specific options { for now TCP only } and
	 * reserve some space. For datagram sockets, this can get called
	 * repeatedly for the same socket, but that isn't harmful.
	 */
	if (so->so_type == SOCK_STREAM) {
		m = m_get(M_WAIT, MT_SOOPTS);
		MCLAIM(m, &nfs_mowner);
		*mtod(m, int32_t *) = 1;
		m->m_len = sizeof(int32_t);
		sosetopt(so, SOL_SOCKET, SO_KEEPALIVE, m);
	}
	if ((so->so_proto->pr_domain->dom_family == AF_INET
#ifdef INET6
	    || so->so_proto->pr_domain->dom_family == AF_INET6
#endif
	    ) &&
	    so->so_proto->pr_protocol == IPPROTO_TCP) {
		m = m_get(M_WAIT, MT_SOOPTS);
		MCLAIM(m, &nfs_mowner);
		*mtod(m, int32_t *) = 1;
		m->m_len = sizeof(int32_t);
		sosetopt(so, IPPROTO_TCP, TCP_NODELAY, m);
	}
	so->so_rcv.sb_flags &= ~SB_NOINTR;
	so->so_rcv.sb_timeo = 0;
	so->so_snd.sb_flags &= ~SB_NOINTR;
	so->so_snd.sb_timeo = 0;
	if (tslp)
		slp = tslp;
	else {
		slp = nfsrv_sockalloc();
	}
	slp->ns_so = so;
	slp->ns_nam = mynam;
	fp->f_count++;
	slp->ns_fp = fp;
	s = splsoftnet();
	so->so_upcallarg = (void *)slp;
	so->so_upcall = nfsrv_rcv;
	so->so_rcv.sb_flags |= SB_UPCALL;
	slp->ns_flag = (SLP_VALID | SLP_NEEDQ);
	nfsrv_wakenfsd(slp);
	splx(s);
	return (0);
}

/*
 * Called by nfssvc() for nfsds. Just loops around servicing rpc requests
 * until it is killed by a signal.
 */
int
nfssvc_nfsd(nsd, argp, l)
	struct nfsd_srvargs *nsd;
	void *argp;
	struct lwp *l;
{
	struct timeval tv;
	struct mbuf *m;
	struct nfssvc_sock *slp;
	struct nfsd *nfsd = nsd->nsd_nfsd;
	struct nfsrv_descript *nd = NULL;
	struct mbuf *mreq;
	u_quad_t cur_usec;
	int error = 0, cacherep, s, siz, sotype, writes_todo;
	struct proc *p = l->l_proc;

#ifndef nolint
	cacherep = RC_DOIT;
	writes_todo = 0;
#endif
	uvm_lwp_hold(l);
	s = splsoftnet();
	if (nfsd == (struct nfsd *)0) {
		nsd->nsd_nfsd = nfsd = (struct nfsd *)
			malloc(sizeof (struct nfsd), M_NFSD, M_WAITOK);
		memset((void *)nfsd, 0, sizeof (struct nfsd));
		nfsd->nfsd_procp = p;
		simple_lock(&nfsd_slock);
		TAILQ_INSERT_TAIL(&nfsd_head, nfsd, nfsd_chain);
		nfs_numnfsd++;
		simple_unlock(&nfsd_slock);
	}
	/*
	 * Loop getting rpc requests until SIGKILL.
	 */
	for (;;) {
		if (nfsd->nfsd_slp == NULL) {
			simple_lock(&nfsd_slock);
			while (nfsd->nfsd_slp == NULL &&
			    (nfsd_head_flag & NFSD_CHECKSLP) == 0) {
				SLIST_INSERT_HEAD(&nfsd_idle_head, nfsd,
				    nfsd_idle);
				nfsd_waiting++;
				error = ltsleep(nfsd, PSOCK | PCATCH, "nfsd",
				    0, &nfsd_slock);
				nfsd_waiting--;
				if (error) {
					slp = nfsd->nfsd_slp;
					nfsd->nfsd_slp = NULL;
					if (!slp)
						SLIST_REMOVE(&nfsd_idle_head,
						    nfsd, nfsd, nfsd_idle);
					simple_unlock(&nfsd_slock);
					if (slp) {
						nfsrv_wakenfsd(slp);
						nfsrv_slpderef(slp);
					}
					goto done;
				}
			}
			if (nfsd->nfsd_slp == NULL &&
			    (nfsd_head_flag & NFSD_CHECKSLP) != 0) {
				slp = TAILQ_FIRST(&nfssvc_sockpending);
				if (slp) {
					KASSERT((slp->ns_flag &
					    (SLP_VALID | SLP_DOREC))
					    == (SLP_VALID | SLP_DOREC));
					TAILQ_REMOVE(&nfssvc_sockpending, slp,
					    ns_pending);
					slp->ns_flag &= ~SLP_DOREC;
					slp->ns_sref++;
					nfsd->nfsd_slp = slp;
				} else
					nfsd_head_flag &= ~NFSD_CHECKSLP;
			}
			simple_unlock(&nfsd_slock);
			if ((slp = nfsd->nfsd_slp) == NULL)
				continue;
			if (slp->ns_flag & SLP_VALID) {
				if (slp->ns_flag & SLP_DISCONN)
					nfsrv_zapsock(slp);
				else if ((slp->ns_flag & SLP_NEEDQ) != 0) {
					nfsrv_rcv(slp->ns_so, (void *)slp,
					    M_WAIT);
				}
				error = nfsrv_dorec(slp, nfsd, &nd);
				getmicrotime(&tv);
				cur_usec = (u_quad_t)tv.tv_sec * 1000000 +
					(u_quad_t)tv.tv_usec;
				if (error && LIST_FIRST(&slp->ns_tq) &&
				    LIST_FIRST(&slp->ns_tq)->nd_time <=
				    cur_usec) {
					error = 0;
					cacherep = RC_DOIT;
					writes_todo = 1;
				} else
					writes_todo = 0;
				if (error == 0 && slp->ns_rec != NULL) {
					nfsrv_wakenfsd(slp);
				}
			}
		} else {
			error = 0;
			slp = nfsd->nfsd_slp;
		}
		if (error || (slp->ns_flag & SLP_VALID) == 0) {
			if (nd) {
				nfsdreq_free(nd);
				nd = NULL;
			}
			nfsd->nfsd_slp = NULL;
			nfsrv_slpderef(slp);
			continue;
		}
		splx(s);
		sotype = slp->ns_so->so_type;
		if (nd) {
			getmicrotime(&nd->nd_starttime);
			if (nd->nd_nam2)
				nd->nd_nam = nd->nd_nam2;
			else
				nd->nd_nam = slp->ns_nam;

			/*
			 * Check to see if authorization is needed.
			 */
			if (nfsd->nfsd_flag & NFSD_NEEDAUTH) {
				nfsd->nfsd_flag &= ~NFSD_NEEDAUTH;
				nsd->nsd_haddr = mtod(nd->nd_nam,
				    struct sockaddr_in *)->sin_addr.s_addr;
				nsd->nsd_authlen = nfsd->nfsd_authlen;
				nsd->nsd_verflen = nfsd->nfsd_verflen;
				if (!copyout(nfsd->nfsd_authstr,
				    nsd->nsd_authstr, nfsd->nfsd_authlen) &&
				    !copyout(nfsd->nfsd_verfstr,
				    nsd->nsd_verfstr, nfsd->nfsd_verflen) &&
				    !copyout(nsd, argp, sizeof (*nsd))) {
					uvm_lwp_rele(l);
					return (ENEEDAUTH);
				}
				cacherep = RC_DROPIT;
			} else
				cacherep = nfsrv_getcache(nd, slp, &mreq);

			if (nfsd->nfsd_flag & NFSD_AUTHFAIL) {
				nfsd->nfsd_flag &= ~NFSD_AUTHFAIL;
				nd->nd_procnum = NFSPROC_NOOP;
				nd->nd_repstat =
				    (NFSERR_AUTHERR | AUTH_TOOWEAK);
				cacherep = RC_DOIT;
			}
		}

		/*
		 * Loop to get all the write rpc relies that have been
		 * gathered together.
		 */
		do {
#ifdef DIAGNOSTIC
			int lockcount;
#endif
			switch (cacherep) {
			case RC_DOIT:
#ifdef DIAGNOSTIC
				/*
				 * NFS server procs should neither release
				 * locks already held, nor leave things
				 * locked.  Catch this sooner, rather than
				 * later (when we try to relock something we
				 * already have locked).  Careful inspection
				 * of the failing routine usually turns up the
				 * lock leak.. once we know what it is..
				 */
				lockcount = l->l_locks;
#endif
				mreq = NULL;
				netexport_rdlock();
				if (writes_todo || nd == NULL ||
				     (!(nd->nd_flag & ND_NFSV3) &&
				     nd->nd_procnum == NFSPROC_WRITE &&
				     nfsrvw_procrastinate > 0))
					error = nfsrv_writegather(&nd, slp,
					    l, &mreq);
				else
					error =
					    (*(nfsrv3_procs[nd->nd_procnum]))
					    (nd, slp, l, &mreq);
				netexport_rdunlock();
#ifdef DIAGNOSTIC
				if (l->l_locks != lockcount) {
					/*
					 * If you see this panic, audit
					 * nfsrv3_procs[nd->nd_procnum] for
					 * vnode locking errors (usually, it's
					 * due to forgetting to vput()
					 * something).
					 */
#ifdef DEBUG
					extern void printlockedvnodes(void);
					printlockedvnodes();
#endif
					printf("nfsd: locking botch in op %d"
					    " (before %d, after %d)\n",
					    nd ? nd->nd_procnum : -1,
					    lockcount, l->l_locks);
				}
#endif
				if (mreq == NULL) {
					if (nd != NULL) {
						if (nd->nd_nam2)
							m_free(nd->nd_nam2);
						if (nd->nd_mrep)
							m_freem(nd->nd_mrep);
					}
					break;
				}
				if (error) {
					nfsstats.srv_errs++;
					nfsrv_updatecache(nd, false, mreq);
					if (nd->nd_nam2)
						m_freem(nd->nd_nam2);
					break;
				}
				nfsstats.srvrpccnt[nd->nd_procnum]++;
				nfsrv_updatecache(nd, true, mreq);
				nd->nd_mrep = (struct mbuf *)0;
			case RC_REPLY:
				m = mreq;
				siz = 0;
				while (m) {
					siz += m->m_len;
					m = m->m_next;
				}
				if (siz <= 0 || siz > NFS_MAXPACKET) {
					printf("mbuf siz=%d\n",siz);
					panic("Bad nfs svc reply");
				}
				m = mreq;
				m->m_pkthdr.len = siz;
				m->m_pkthdr.rcvif = (struct ifnet *)0;
				/*
				 * For stream protocols, prepend a Sun RPC
				 * Record Mark.
				 */
				if (sotype == SOCK_STREAM) {
					M_PREPEND(m, NFSX_UNSIGNED, M_WAIT);
					*mtod(m, u_int32_t *) =
					    htonl(0x80000000 | siz);
				}
				nd->nd_mreq = m;
				if (nfsrtton) {
					nfsd_rt(slp->ns_so->so_type, nd,
					    cacherep);
				}
				s = splsoftnet();
				error = nfsdsock_sendreply(slp, nd);
				nd = NULL;
				if (error == EPIPE)
					nfsrv_zapsock(slp);
				if (error == EINTR || error == ERESTART) {
					nfsrv_slpderef(slp);
					goto done;
				}
				splx(s);
				break;
			case RC_DROPIT:
				if (nfsrtton)
					nfsd_rt(sotype, nd, cacherep);
				m_freem(nd->nd_mrep);
				m_freem(nd->nd_nam2);
				break;
			}
			if (nd) {
				nfsdreq_free(nd);
				nd = NULL;
			}

			/*
			 * Check to see if there are outstanding writes that
			 * need to be serviced.
			 */
			getmicrotime(&tv);
			cur_usec = (u_quad_t)tv.tv_sec * 1000000 +
			    (u_quad_t)tv.tv_usec;
			s = splsoftclock();
			if (LIST_FIRST(&slp->ns_tq) &&
			    LIST_FIRST(&slp->ns_tq)->nd_time <= cur_usec) {
				cacherep = RC_DOIT;
				writes_todo = 1;
			} else
				writes_todo = 0;
			splx(s);
		} while (writes_todo);
		s = splsoftnet();
		if (nfsrv_dorec(slp, nfsd, &nd)) {
			nfsd->nfsd_slp = NULL;
			nfsrv_slpderef(slp);
		}
	}
done:
	simple_lock(&nfsd_slock);
	TAILQ_REMOVE(&nfsd_head, nfsd, nfsd_chain);
	simple_unlock(&nfsd_slock);
	splx(s);
	free((void *)nfsd, M_NFSD);
	nsd->nsd_nfsd = (struct nfsd *)0;
	if (--nfs_numnfsd == 0)
		nfsrv_init(true);	/* Reinitialize everything */
	uvm_lwp_rele(l);
	return (error);
}

/*
 * Shut down a socket associated with an nfssvc_sock structure.
 * Should be called with the send lock set, if required.
 * The trick here is to increment the sref at the start, so that the nfsds
 * will stop using it and clear ns_flag at the end so that it will not be
 * reassigned during cleanup.
 *
 * called at splsoftnet.
 */
void
nfsrv_zapsock(slp)
	struct nfssvc_sock *slp;
{
	struct nfsuid *nuidp, *nnuidp;
	struct nfsrv_descript *nwp, *nnwp;
	struct socket *so;
	struct mbuf *m;
	int s;

	if (nfsdsock_drain(slp)) {
		return;
	}
	simple_lock(&nfsd_slock);
	if (slp->ns_flag & SLP_DOREC) {
		TAILQ_REMOVE(&nfssvc_sockpending, slp, ns_pending);
	}
	simple_unlock(&nfsd_slock);

	so = slp->ns_so;
	KASSERT(so != NULL);
	so->so_upcall = NULL;
	so->so_upcallarg = NULL;
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	soshutdown(so, SHUT_RDWR);

	if (slp->ns_nam)
		m_free(slp->ns_nam);
	m_freem(slp->ns_raw);
	m = slp->ns_rec;
	while (m != NULL) {
		struct mbuf *n;

		n = m->m_nextpkt;
		m_freem(m);
		m = n;
	}
	for (nuidp = TAILQ_FIRST(&slp->ns_uidlruhead); nuidp != 0;
	    nuidp = nnuidp) {
		nnuidp = TAILQ_NEXT(nuidp, nu_lru);
		LIST_REMOVE(nuidp, nu_hash);
		TAILQ_REMOVE(&slp->ns_uidlruhead, nuidp, nu_lru);
		if (nuidp->nu_flag & NU_NAM)
			m_freem(nuidp->nu_nam);
		free((void *)nuidp, M_NFSUID);
	}
	s = splsoftclock();
	for (nwp = LIST_FIRST(&slp->ns_tq); nwp; nwp = nnwp) {
		nnwp = LIST_NEXT(nwp, nd_tq);
		LIST_REMOVE(nwp, nd_tq);
		nfsdreq_free(nwp);
	}
	splx(s);
}

/*
 * Derefence a server socket structure. If it has no more references and
 * is no longer valid, you can throw it away.
 */
void
nfsrv_slpderef(slp)
	struct nfssvc_sock *slp;
{
	KASSERT(!simple_lock_held(&nfsd_slock));

	if (--(slp->ns_sref) == 0 && (slp->ns_flag & SLP_VALID) == 0) {
		struct file *fp;
		int s = splsoftnet();
		simple_lock(&nfsd_slock);
		TAILQ_REMOVE(&nfssvc_sockhead, slp, ns_chain);
		simple_unlock(&nfsd_slock);
		splx(s);

		fp = slp->ns_fp;
		if (fp != NULL) {
			slp->ns_fp = NULL;
			KASSERT(fp != NULL);
			KASSERT(fp->f_data == slp->ns_so);
			mutex_enter(&fp->f_lock);
			FILE_USE(fp);
			closef(fp, (struct lwp *)0);
			slp->ns_so = NULL;
		}

		nfsrv_sockfree(slp);
	}
}

/*
 * Initialize the data structures for the server.
 * Handshake with any new nfsds starting up to avoid any chance of
 * corruption.
 */
void
nfsrv_init(terminating)
	int terminating;
{
	struct nfssvc_sock *slp;
	int s;

	s = splsoftnet();
	simple_lock(&nfsd_slock);
	if (nfssvc_sockhead_flag & SLP_INIT)
		panic("nfsd init");
	nfssvc_sockhead_flag |= SLP_INIT;

	if (terminating) {
		while ((slp = TAILQ_FIRST(&nfssvc_sockhead)) != NULL) {
			simple_unlock(&nfsd_slock);
			KASSERT(slp->ns_sref == 0);
			slp->ns_sref++;
			nfsrv_zapsock(slp);
			nfsrv_slpderef(slp);
			simple_lock(&nfsd_slock);
		}
		simple_unlock(&nfsd_slock);
		splx(s);
		nfsrv_cleancache();	/* And clear out server cache */
	} else {
		simple_unlock(&nfsd_slock);
		splx(s);
		nfs_pub.np_valid = 0;
	}

	TAILQ_INIT(&nfssvc_sockhead);
	TAILQ_INIT(&nfssvc_sockpending);
	nfssvc_sockhead_flag &= ~SLP_INIT;

	TAILQ_INIT(&nfsd_head);
	SLIST_INIT(&nfsd_idle_head);
	nfsd_head_flag &= ~NFSD_CHECKSLP;

	nfs_udpsock = nfsrv_sockalloc();

#ifdef INET6
	nfs_udp6sock = nfsrv_sockalloc();
#endif

#ifdef ISO
	nfs_cltpsock = nfsrv_sockalloc();
#endif

	simple_lock(&nfsd_slock);
	if (nfssvc_sockhead_flag & SLP_WANTINIT) {
		nfssvc_sockhead_flag &= ~SLP_WANTINIT;
		wakeup(&nfssvc_sockhead);
	}
	simple_unlock(&nfsd_slock);
	splx(s);
}

/*
 * Add entries to the server monitor log.
 */
static void
nfsd_rt(sotype, nd, cacherep)
	int sotype;
	struct nfsrv_descript *nd;
	int cacherep;
{
	struct timeval tv;
	struct drt *rt;

	rt = &nfsdrt.drt[nfsdrt.pos];
	if (cacherep == RC_DOIT)
		rt->flag = 0;
	else if (cacherep == RC_REPLY)
		rt->flag = DRT_CACHEREPLY;
	else
		rt->flag = DRT_CACHEDROP;
	if (sotype == SOCK_STREAM)
		rt->flag |= DRT_TCP;
	if (nd->nd_flag & ND_NFSV3)
		rt->flag |= DRT_NFSV3;
	rt->proc = nd->nd_procnum;
	if (mtod(nd->nd_nam, struct sockaddr *)->sa_family == AF_INET)
	    rt->ipadr = mtod(nd->nd_nam, struct sockaddr_in *)->sin_addr.s_addr;
	else
	    rt->ipadr = INADDR_ANY;
	getmicrotime(&tv);
	rt->resptime = ((tv.tv_sec - nd->nd_starttime.tv_sec) * 1000000) +
		(tv.tv_usec - nd->nd_starttime.tv_usec);
	rt->tstamp = tv;
	nfsdrt.pos = (nfsdrt.pos + 1) % NFSRTTLOGSIZ;
}
#endif /* NFSSERVER */

#ifdef NFS

int nfs_defect = 0;
/*
 * Asynchronous I/O threads for client nfs.
 * They do read-ahead and write-behind operations on the block I/O cache.
 * Never returns unless it fails or gets killed.
 */

int
nfssvc_iod(l)
	struct lwp *l;
{
	struct buf *bp;
	int i;
	struct nfs_iod *myiod;
	struct nfsmount *nmp;
	int error = 0;
	struct proc *p = l->l_proc;

	/*
	 * Assign my position or return error if too many already running
	 */
	myiod = NULL;
	for (i = 0; i < NFS_MAXASYNCDAEMON; i++)
		if (nfs_asyncdaemon[i].nid_proc == NULL) {
			myiod = &nfs_asyncdaemon[i];
			break;
		}
	if (myiod == NULL)
		return (EBUSY);
	myiod->nid_proc = p;
	nfs_numasync++;
	uvm_lwp_hold(l);
	/*
	 * Just loop around doing our stuff until SIGKILL
	 */
	for (;;) {
		while (/*CONSTCOND*/ true) {
			mutex_enter(&myiod->nid_lock);
			nmp = myiod->nid_mount;
			if (nmp) {
				mutex_enter(&nmp->nm_lock);
				if (!TAILQ_EMPTY(&nmp->nm_bufq)) {
					mutex_exit(&myiod->nid_lock);
					break;
				}
				nmp->nm_bufqiods--;
				mutex_exit(&nmp->nm_lock);
			}
			myiod->nid_want = p;
			myiod->nid_mount = NULL;
			error = mtsleep(&myiod->nid_want,
			    PWAIT | PCATCH | PNORELOCK, "nfsidl", 0,
			    &myiod->nid_lock);
			if (error)
				goto quit;
		}

		while ((bp = TAILQ_FIRST(&nmp->nm_bufq)) != NULL) {
			/* Take one off the front of the list */
			TAILQ_REMOVE(&nmp->nm_bufq, bp, b_freelist);
			nmp->nm_bufqlen--;
			if (nmp->nm_bufqwant &&
			    nmp->nm_bufqlen < 2 * nfs_numasync) {
				nmp->nm_bufqwant = false;
				wakeup(&nmp->nm_bufq);
			}
			mutex_exit(&nmp->nm_lock);
			(void)nfs_doio(bp);
			mutex_enter(&nmp->nm_lock);
			/*
			 * If there are more than one iod on this mount, 
			 * then defect so that the iods can be shared out
			 * fairly between the mounts
			 */
			if (nfs_defect && nmp->nm_bufqiods > 1) {
				myiod->nid_mount = NULL;
				nmp->nm_bufqiods--;
				break;
			}
		}
		mutex_exit(&nmp->nm_lock);
	}
quit:
	uvm_lwp_rele(l);
	if (nmp)
		nmp->nm_bufqiods--;
	myiod->nid_want = NULL;
	myiod->nid_mount = NULL;
	myiod->nid_proc = NULL;
	nfs_numasync--;

	return error;
}

void
nfs_iodinit()
{
	int i;

	for (i = 0; i < NFS_MAXASYNCDAEMON; i++)
		mutex_init(&nfs_asyncdaemon[i].nid_lock, MUTEX_DEFAULT,
		    IPL_NONE);
}

void
start_nfsio(void *arg)
{
	nfssvc_iod(curlwp);

	kthread_exit(0);
}

void
nfs_getset_niothreads(set)
	int set;
{
	int i, have, start;

	for (have = 0, i = 0; i < NFS_MAXASYNCDAEMON; i++)
		if (nfs_asyncdaemon[i].nid_proc != NULL)
			have++;

	if (set) {
		/* clamp to sane range */
		nfs_niothreads = max(0, min(nfs_niothreads, NFS_MAXASYNCDAEMON));

		start = nfs_niothreads - have;

		while (start > 0) {
			kthread_create(PRI_NONE, false, start_nfsio, NULL,
			    NULL, "nfsio");
			start--;
		}

		for (i = 0; (start < 0) && (i < NFS_MAXASYNCDAEMON); i++)
			if (nfs_asyncdaemon[i].nid_proc != NULL) {
				psignal(nfs_asyncdaemon[i].nid_proc, SIGKILL);
				start++;
			}
	} else {
		if (nfs_niothreads >= 0)
			nfs_niothreads = have;
	}
}

/*
 * Get an authorization string for the uid by having the mount_nfs sitting
 * on this mount point porpous out of the kernel and do it.
 */
int
nfs_getauth(nmp, rep, cred, auth_str, auth_len, verf_str, verf_len, key)
	struct nfsmount *nmp;
	struct nfsreq *rep;
	kauth_cred_t cred;
	char **auth_str;
	int *auth_len;
	char *verf_str;
	int *verf_len;
	NFSKERBKEY_T key;		/* return session key */
{
	int error = 0;

	while ((nmp->nm_iflag & NFSMNT_WAITAUTH) == 0) {
		nmp->nm_iflag |= NFSMNT_WANTAUTH;
		(void) tsleep((void *)&nmp->nm_authtype, PSOCK,
			"nfsauth1", 2 * hz);
		error = nfs_sigintr(nmp, rep, rep->r_lwp);
		if (error) {
			nmp->nm_iflag &= ~NFSMNT_WANTAUTH;
			return (error);
		}
	}
	nmp->nm_iflag &= ~(NFSMNT_WAITAUTH | NFSMNT_WANTAUTH);
	nmp->nm_authstr = *auth_str = (char *)malloc(RPCAUTH_MAXSIZ, M_TEMP, M_WAITOK);
	nmp->nm_authlen = RPCAUTH_MAXSIZ;
	nmp->nm_verfstr = verf_str;
	nmp->nm_verflen = *verf_len;
	nmp->nm_authuid = kauth_cred_geteuid(cred);
	wakeup((void *)&nmp->nm_authstr);

	/*
	 * And wait for mount_nfs to do its stuff.
	 */
	while ((nmp->nm_iflag & NFSMNT_HASAUTH) == 0 && error == 0) {
		(void) tsleep((void *)&nmp->nm_authlen, PSOCK,
			"nfsauth2", 2 * hz);
		error = nfs_sigintr(nmp, rep, rep->r_lwp);
	}
	if (nmp->nm_iflag & NFSMNT_AUTHERR) {
		nmp->nm_iflag &= ~NFSMNT_AUTHERR;
		error = EAUTH;
	}
	if (error)
		free((void *)*auth_str, M_TEMP);
	else {
		*auth_len = nmp->nm_authlen;
		*verf_len = nmp->nm_verflen;
		memcpy(key, nmp->nm_key, sizeof (NFSKERBKEY_T));
	}
	nmp->nm_iflag &= ~NFSMNT_HASAUTH;
	nmp->nm_iflag |= NFSMNT_WAITAUTH;
	if (nmp->nm_iflag & NFSMNT_WANTAUTH) {
		nmp->nm_iflag &= ~NFSMNT_WANTAUTH;
		wakeup((void *)&nmp->nm_authtype);
	}
	return (error);
}

/*
 * Get a nickname authenticator and verifier.
 */
int
nfs_getnickauth(struct nfsmount *nmp, kauth_cred_t cred, char **auth_str,
    int *auth_len, char *verf_str, int verf_len)
{
	struct timeval ktvin, ktvout, tv;
	struct nfsuid *nuidp;
	u_int32_t *nickp, *verfp;

	memset(&ktvout, 0, sizeof ktvout);	/* XXX gcc */

#ifdef DIAGNOSTIC
	if (verf_len < (4 * NFSX_UNSIGNED))
		panic("nfs_getnickauth verf too small");
#endif
	LIST_FOREACH(nuidp, NMUIDHASH(nmp, kauth_cred_geteuid(cred)), nu_hash) {
		if (kauth_cred_geteuid(nuidp->nu_cr) == kauth_cred_geteuid(cred))
			break;
	}
	if (!nuidp || nuidp->nu_expire < time_second)
		return (EACCES);

	/*
	 * Move to the end of the lru list (end of lru == most recently used).
	 */
	TAILQ_REMOVE(&nmp->nm_uidlruhead, nuidp, nu_lru);
	TAILQ_INSERT_TAIL(&nmp->nm_uidlruhead, nuidp, nu_lru);

	nickp = (u_int32_t *)malloc(2 * NFSX_UNSIGNED, M_TEMP, M_WAITOK);
	*nickp++ = txdr_unsigned(RPCAKN_NICKNAME);
	*nickp = txdr_unsigned(nuidp->nu_nickname);
	*auth_str = (char *)nickp;
	*auth_len = 2 * NFSX_UNSIGNED;

	/*
	 * Now we must encrypt the verifier and package it up.
	 */
	verfp = (u_int32_t *)verf_str;
	*verfp++ = txdr_unsigned(RPCAKN_NICKNAME);
	getmicrotime(&tv);
	if (tv.tv_sec > nuidp->nu_timestamp.tv_sec ||
	    (tv.tv_sec == nuidp->nu_timestamp.tv_sec &&
	     tv.tv_usec > nuidp->nu_timestamp.tv_usec))
		nuidp->nu_timestamp = tv;
	else
		nuidp->nu_timestamp.tv_usec++;
	ktvin.tv_sec = txdr_unsigned(nuidp->nu_timestamp.tv_sec);
	ktvin.tv_usec = txdr_unsigned(nuidp->nu_timestamp.tv_usec);

	/*
	 * Now encrypt the timestamp verifier in ecb mode using the session
	 * key.
	 */
#ifdef NFSKERB
	XXX
#endif

	*verfp++ = ktvout.tv_sec;
	*verfp++ = ktvout.tv_usec;
	*verfp = 0;
	return (0);
}

/*
 * Save the current nickname in a hash list entry on the mount point.
 */
int
nfs_savenickauth(nmp, cred, len, key, mdp, dposp, mrep)
	struct nfsmount *nmp;
	kauth_cred_t cred;
	int len;
	NFSKERBKEY_T key;
	struct mbuf **mdp;
	char **dposp;
	struct mbuf *mrep;
{
	struct nfsuid *nuidp;
	u_int32_t *tl;
	int32_t t1;
	struct mbuf *md = *mdp;
	struct timeval ktvin, ktvout;
	u_int32_t nick;
	char *dpos = *dposp, *cp2;
	int deltasec, error = 0;

	memset(&ktvout, 0, sizeof ktvout);	 /* XXX gcc */

	if (len == (3 * NFSX_UNSIGNED)) {
		nfsm_dissect(tl, u_int32_t *, 3 * NFSX_UNSIGNED);
		ktvin.tv_sec = *tl++;
		ktvin.tv_usec = *tl++;
		nick = fxdr_unsigned(u_int32_t, *tl);

		/*
		 * Decrypt the timestamp in ecb mode.
		 */
#ifdef NFSKERB
		XXX
#endif
		ktvout.tv_sec = fxdr_unsigned(long, ktvout.tv_sec);
		ktvout.tv_usec = fxdr_unsigned(long, ktvout.tv_usec);
		deltasec = time_second - ktvout.tv_sec;
		if (deltasec < 0)
			deltasec = -deltasec;
		/*
		 * If ok, add it to the hash list for the mount point.
		 */
		if (deltasec <= NFS_KERBCLOCKSKEW) {
			if (nmp->nm_numuids < nuidhash_max) {
				nmp->nm_numuids++;
				nuidp = (struct nfsuid *)
				   malloc(sizeof (struct nfsuid), M_NFSUID,
					M_WAITOK);
			} else {
				nuidp = TAILQ_FIRST(&nmp->nm_uidlruhead);
				LIST_REMOVE(nuidp, nu_hash);
				TAILQ_REMOVE(&nmp->nm_uidlruhead, nuidp,
					nu_lru);
			}
			nuidp->nu_flag = 0;
			kauth_cred_seteuid(nuidp->nu_cr, kauth_cred_geteuid(cred));
			nuidp->nu_expire = time_second + NFS_KERBTTL;
			nuidp->nu_timestamp = ktvout;
			nuidp->nu_nickname = nick;
			memcpy(nuidp->nu_key, key, sizeof (NFSKERBKEY_T));
			TAILQ_INSERT_TAIL(&nmp->nm_uidlruhead, nuidp,
				nu_lru);
			LIST_INSERT_HEAD(NMUIDHASH(nmp, kauth_cred_geteuid(cred)),
				nuidp, nu_hash);
		}
	} else
		nfsm_adv(nfsm_rndup(len));
nfsmout:
	*mdp = md;
	*dposp = dpos;
	return (error);
}
#endif /* NFS */
