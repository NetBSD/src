/*	$NetBSD: socketvar.h,v 1.46 2001/05/06 19:22:33 manu Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)socketvar.h	8.3 (Berkeley) 2/19/95
 */

#ifndef _SYS_SOCKETVAR_H_
#define	_SYS_SOCKETVAR_H_

#include <sys/select.h>			/* for struct selinfo */
#include <sys/queue.h>

#if !defined(_KERNEL) || defined(LKM)
struct uio;
#endif

TAILQ_HEAD(soqhead, socket);

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 */
struct socket {
	short		so_type;	/* generic type, see socket.h */
	short		so_options;	/* from socket call, see socket.h */
	short		so_linger;	/* time to linger while closing */
	short		so_state;	/* internal state flags SS_*, below */
	void		*so_pcb;	/* protocol control block */
	struct protosw	*so_proto;	/* protocol handle */
/*
 * Variables for connection queueing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct socket	*so_head;	/* back pointer to accept socket */
	struct soqhead	*so_onq;	/* queue (q or q0) that we're on */
	struct soqhead	so_q0;		/* queue of partial connections */
	struct soqhead	so_q;		/* queue of incoming connections */
	TAILQ_ENTRY(socket) so_qe;	/* our queue entry (q or q0) */
	short		so_q0len;	/* partials on so_q0 */
	short		so_qlen;	/* number of connections on so_q */
	short		so_qlimit;	/* max number queued connections */
	short		so_timeo;	/* connection timeout */
	u_short		so_error;	/* error affecting connection */
	pid_t		so_pgid;	/* pgid for signals */
	u_long		so_oobmark;	/* chars to oob mark */
/*
 * Variables for socket buffering.
 */
	struct sockbuf {
		u_long	sb_cc;		/* actual chars in buffer */
		u_long	sb_hiwat;	/* max actual char count */
		u_long	sb_mbcnt;	/* chars of mbufs used */
		u_long	sb_mbmax;	/* max chars of mbufs to use */
		long	sb_lowat;	/* low water mark */
		struct mbuf *sb_mb;	/* the mbuf chain */
		struct selinfo sb_sel;	/* process selecting read/write */
		short	sb_flags;	/* flags, see below */
		short	sb_timeo;	/* timeout for read/write */
	} so_rcv, so_snd;

#ifndef SB_MAX
#define	SB_MAX		(256*1024)	/* default for max chars in sockbuf */
#endif

#define	SB_LOCK		0x01		/* lock on data queue */
#define	SB_WANT		0x02		/* someone is waiting to lock */
#define	SB_WAIT		0x04		/* someone is waiting for data/space */
#define	SB_SEL		0x08		/* someone is selecting */
#define	SB_ASYNC	0x10		/* ASYNC I/O, need signals */
#define	SB_UPCALL	0x20		/* someone wants an upcall */
#define	SB_NOINTR	0x40		/* operations not interruptible */

	void		*so_internal;	/* Space for svr4 stream data */
	void		(*so_upcall) __P((struct socket *so, caddr_t arg,
					int waitf));
	caddr_t		so_upcallarg;	/* Arg for above */
	int		(*so_send) __P((struct socket *so, struct mbuf *addr,
					struct uio *uio, struct mbuf *top,
					struct mbuf *control, int flags));
	int		(*so_receive) __P((struct socket *so,
					struct mbuf **paddr,
					struct uio *uio, struct mbuf **mp0,
					struct mbuf **controlp, int *flagsp));
	uid_t		so_uid;		/* who opened the socket */
};

/*
 * Socket state bits.
 */
#define	SS_NOFDREF		0x001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x008	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x010	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x020	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x040	/* at mark on input */
#define	SS_ISDISCONNECTED	0x800	/* socket disconnected from peer */

#define	SS_NBIO			0x080	/* non-blocking ops */
#define	SS_ASYNC		0x100	/* async i/o notify */
#define	SS_ISCONFIRMING		0x200	/* deciding to accept connection req */
#define	SS_MORETOCOME		0x400	/*
					 * hint from sosend to lower layer;
					 * more data coming
					 */
#define 	SS_ISAPIPE 		0x800 /* socket is implementing a pipe */


/*
 * Macros for sockets and socket buffering.
 */

/*
 * Do we need to notify the other side when I/O is possible?
 */
#define	sb_notify(sb)	(((sb)->sb_flags & \
	(SB_WAIT | SB_SEL | SB_ASYNC | SB_UPCALL)) != 0)

/*
 * How much space is there in a socket buffer (so->so_snd or so->so_rcv)?
 * This is problematical if the fields are unsigned, as the space might
 * still be negative (cc > hiwat or mbcnt > mbmax).  Should detect
 * overflow and return 0.  Should use "lmin" but it doesn't exist now.
 */
#define	sbspace(sb) \
	((long) imin((int)((sb)->sb_hiwat - (sb)->sb_cc), \
	 (int)((sb)->sb_mbmax - (sb)->sb_mbcnt)))

/* do we have to send all at once on a socket? */
#define	sosendallatonce(so) \
	((so)->so_proto->pr_flags & PR_ATOMIC)

/* can we read something from so? */
#define	soreadable(so) \
	((so)->so_rcv.sb_cc >= (so)->so_rcv.sb_lowat || \
	    ((so)->so_state & SS_CANTRCVMORE) || \
	    (so)->so_qlen || (so)->so_error)

/* can we write something to so? */
#define	sowriteable(so) \
	((sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat && \
	    (((so)->so_state&SS_ISCONNECTED) || \
	      ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0)) || \
	 ((so)->so_state & SS_CANTSENDMORE) || \
	 (so)->so_error)

/* adjust counters in sb reflecting allocation of m */
#define	sballoc(sb, m)							\
do {									\
	(sb)->sb_cc += (m)->m_len;					\
	(sb)->sb_mbcnt += MSIZE;					\
	if ((m)->m_flags & M_EXT)					\
		(sb)->sb_mbcnt += (m)->m_ext.ext_size;			\
} while (/* CONSTCOND */ 0)

/* adjust counters in sb reflecting freeing of m */
#define	sbfree(sb, m)							\
do {									\
	(sb)->sb_cc -= (m)->m_len;					\
	(sb)->sb_mbcnt -= MSIZE;					\
	if ((m)->m_flags & M_EXT)					\
		(sb)->sb_mbcnt -= (m)->m_ext.ext_size;			\
} while (/* CONSTCOND */ 0)

/*
 * Set lock on sockbuf sb; sleep if lock is already held.
 * Unless SB_NOINTR is set on sockbuf, sleep is interruptible.
 * Returns error without lock if sleep is interrupted.
 */
#define	sblock(sb, wf)							\
	((sb)->sb_flags & SB_LOCK ?					\
	    (((wf) == M_WAITOK) ? sb_lock(sb) : EWOULDBLOCK) :		\
	    ((sb)->sb_flags |= SB_LOCK), 0)

/* release lock on sockbuf sb */
#define	sbunlock(sb)							\
do {									\
	(sb)->sb_flags &= ~SB_LOCK;					\
	if ((sb)->sb_flags & SB_WANT) {					\
		(sb)->sb_flags &= ~SB_WANT;				\
		wakeup((caddr_t)&(sb)->sb_flags);			\
	}								\
} while (/* CONSTCOND */ 0)

#define	sorwakeup(so)							\
do {									\
	if (sb_notify(&(so)->so_rcv))					\
		sowakeup((so), &(so)->so_rcv);				\
} while (/* CONSTCOND */ 0)

#define	sowwakeup(so)							\
do {									\
	if (sb_notify(&(so)->so_snd))					\
		sowakeup((so), &(so)->so_snd);				\
} while (/* CONSTCOND */ 0)

#ifdef _KERNEL
extern u_long		sb_max;
/* to catch callers missing new second argument to sonewconn: */
#define	sonewconn(head, connstatus)	sonewconn1((head), (connstatus))

/* strings for sleep message: */
extern const char	netio[], netcon[], netcls[];

extern struct pool	socket_pool;

struct mbuf;
struct sockaddr;
struct proc;
struct msghdr;
struct stat;

/*
 * File operations on sockets.
 */
int	soo_read(struct file *fp, off_t *offset, struct uio *uio,
	    struct ucred *cred, int flags);
int	soo_write(struct file *fp, off_t *offset, struct uio *uio,
	    struct ucred *cred, int flags);
int	soo_fcntl(struct file *fp, u_int cmd, caddr_t data, struct proc *p);
int	soo_ioctl(struct file *fp, u_long cmd, caddr_t data, struct proc *p);
int	soo_poll(struct file *fp, int events, struct proc *p);
int 	soo_close(struct file *fp, struct proc *p);
int	soo_stat(struct file *fp, struct stat *ub, struct proc *p);
int	uipc_usrreq(struct socket *, int , struct mbuf *,
	    struct mbuf *, struct mbuf *, struct proc *);
int	uipc_ctloutput(int, struct socket *, int, int, struct mbuf **);
void	sbappend(struct sockbuf *sb, struct mbuf *m);
int	sbappendaddr(struct sockbuf *sb, struct sockaddr *asa,
	    struct mbuf *m0, struct mbuf *control);
int	sbappendcontrol(struct sockbuf *sb, struct mbuf *m0,
	    struct mbuf *control);
void	sbappendrecord(struct sockbuf *sb, struct mbuf *m0);
void	sbcheck(struct sockbuf *sb);
void	sbcompress(struct sockbuf *sb, struct mbuf *m, struct mbuf *n);
struct mbuf *
	sbcreatecontrol(caddr_t p, int size, int type, int level);
void	sbdrop(struct sockbuf *sb, int len);
void	sbdroprecord(struct sockbuf *sb);
void	sbflush(struct sockbuf *sb);
void	sbinsertoob(struct sockbuf *sb, struct mbuf *m0);
void	sbrelease(struct sockbuf *sb);
int	sbreserve(struct sockbuf *sb, u_long cc);
int	sbwait(struct sockbuf *sb);
int	sb_lock(struct sockbuf *sb);
void	soinit(void);
int	soabort(struct socket *so);
int	soaccept(struct socket *so, struct mbuf *nam);
int	sobind(struct socket *so, struct mbuf *nam, struct proc *);
void	socantrcvmore(struct socket *so);
void	socantsendmore(struct socket *so);
int	soclose(struct socket *so);
int	soconnect(struct socket *so, struct mbuf *nam);
int	soconnect2(struct socket *so1, struct socket *so2);
int	socreate(int dom, struct socket **aso, int type, int proto);
int	sodisconnect(struct socket *so);
void	sofree(struct socket *so);
int	sogetopt(struct socket *so, int level, int optname, struct mbuf **mp);
void	sohasoutofband(struct socket *so);
void	soisconnected(struct socket *so);
void	soisconnecting(struct socket *so);
void	soisdisconnected(struct socket *so);
void	soisdisconnecting(struct socket *so);
int	solisten(struct socket *so, int backlog);
struct socket *
	sonewconn1(struct socket *head, int connstatus);
void	soqinsque(struct socket *head, struct socket *so, int q);
int	soqremque(struct socket *so, int q);
int	soreceive(struct socket *so, struct mbuf **paddr, struct uio *uio,
	    struct mbuf **mp0, struct mbuf **controlp, int *flagsp);
int	soreserve(struct socket *so, u_long sndcc, u_long rcvcc);
void	sorflush(struct socket *so);
int	sosend(struct socket *so, struct mbuf *addr, struct uio *uio,
	    struct mbuf *top, struct mbuf *control, int flags);
int	sosetopt(struct socket *so, int level, int optname, struct mbuf *m0);
int	soshutdown(struct socket *so, int how);
void	sowakeup(struct socket *so, struct sockbuf *sb);
int	sockargs(struct mbuf **, const void *, int, int);

int	sendit(struct proc *, int, struct msghdr *, int, register_t *);
int	recvit(struct proc *, int, struct msghdr *, caddr_t, register_t *);

#endif /* _KERNEL */

#endif /* !_SYS_SOCKETVAR_H_ */
