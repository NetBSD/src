/*	$NetBSD: kern_extern.h,v 1.1 1996/02/04 02:19:29 christos Exp $	*/

/*
 * Copyright (c) 1995 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct proc;
struct vnode;
struct ucred;
struct socket;
struct stat;
struct mount;
struct core;
struct ipc_perm;
struct clist;
struct socket;
struct mbuf;

/* kern_clock.c */
int sysctl_clockrate __P((char *, size_t *));

/* kern_exit.c */
void exit1 __P((struct proc *, int));

/* kern_fork.c */
int fork1 __P((struct proc *, int, register_t *));

/* kern_malloc.c */
void kmeminit __P((void));

/* kern_synch.c */
void rqinit __P((void));

/* kern_sysctl.c */
int sysctl_rdstring __P((void *, size_t *, void *, char *));
int sysctl_rdstruct __P((void *, size_t *, void *, void *, int));

/* sys_process.c */
int trace_req __P((struct proc *));

/* sys_socket.c */
int soo_stat __P((struct socket *, struct stat *));

/* subr_log.c */
void logwakeup __P((void));

/* uipc_domain.c */
void domaininit __P((void));

/* uipc_mbuf.c */
void mbinit __P((void));

/* uipc_usrreq.c */
int	uipc_usrreq __P((struct socket *, int , struct mbuf *,
			 struct mbuf *, struct mbuf *));

/* vfs_init.c */
void vfsinit __P((void));

/* vfs_subr.c */
int sysctl_vnode __P((char *, size_t *));
void vclean __P((struct vnode *, int));
void vntblinit __P((void));
void vwakeup __P((struct buf *));

#ifdef SYSVSHM
/* sysv_shm.c */
void shminit __P((void));
void shmexit __P((struct proc *));
#endif

#ifdef SYSVSEM
/* sysv_sem.c */
void seminit __P((void));
void semexit __P((struct proc *));
#endif

#ifdef SYSVMSG
/* sysv_msg.c */
void msginit __P((void));
#endif

#if 0
/* XXX: Fixme; these are really arch stuff */
void consinit __P((void));
void boot __P((int));
int fuswintr __P((caddr_t));
int suswintr __P((caddr_t, u_int));
void pagemove __P((caddr_t, caddr_t, size_t));

void cpu_exit __P((struct proc *));
void cpu_startup __P((void));
void cpu_initclocks __P((void));
void cpu_switch __P((struct proc *));
int cpu_coredump __P((struct proc *, struct vnode *, struct ucred *,
		      struct core *));
#endif
