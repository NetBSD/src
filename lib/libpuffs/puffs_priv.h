/*	$NetBSD: puffs_priv.h,v 1.37 2007/12/25 20:38:01 pooka Exp $	*/

/*
 * Copyright (c) 2006 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PUFFS_PRIVATE_H_
#define _PUFFS_PRIVATE_H_

#include <sys/types.h>
#include <fs/puffs/puffs_msgif.h>

#include <puffs.h>
#include <ucontext.h>

#ifdef PUFFS_WITH_THREADS
#include <pthread.h>

extern pthread_mutex_t pu_lock;
#define PU_LOCK() pthread_mutex_lock(&pu_lock)
#define PU_UNLOCK() pthread_mutex_unlock(&pu_lock)
#else
#define PU_LOCK()
#define PU_UNLOCK()
#endif

#define PU_CMAP(pu, c) (pu->pu_cmap ? pu->pu_cmap(pu,c) : (struct puffs_node*)c)

struct puffs_framectrl {
	puffs_framev_readframe_fn rfb;
	puffs_framev_writeframe_fn wfb;
	puffs_framev_cmpframe_fn cmpfb;
	puffs_framev_gotframe_fn gotfb;
	puffs_framev_fdnotify_fn fdnotfn;
};

struct puffs_fctrl_io {
	struct puffs_framectrl *fctrl;

	int io_fd;
	int stat;

	int rwait;
	int wwait;

	struct puffs_framebuf *cur_in;

	TAILQ_HEAD(, puffs_framebuf) snd_qing;	/* queueing to be sent */
	TAILQ_HEAD(, puffs_framebuf) res_qing;	/* q'ing for rescue */
	LIST_HEAD(, puffs_fbevent) ev_qing;	/* q'ing for events */

	LIST_ENTRY(puffs_fctrl_io) fio_entries;
};
#define FIO_WR		0x01
#define FIO_WRGONE	0x02
#define FIO_RDGONE	0x04
#define FIO_DEAD	0x08
#define FIO_ENABLE_R	0x10
#define FIO_ENABLE_W	0x20

#define FIO_EN_WRITE(fio)				\
    (!(fio->stat & FIO_WR)				\
      && ((!TAILQ_EMPTY(&fio->snd_qing)			\
            && (fio->stat & FIO_ENABLE_W))		\
         || fio->wwait))

#define FIO_RM_WRITE(fio)			\
    ((fio->stat & FIO_WR)			\
      && (((TAILQ_EMPTY(&fio->snd_qing)		\
        || (fio->stat & FIO_ENABLE_W) == 0))	\
	&& (fio->wwait == 0)))

struct puffs_executor {
	struct puffs_framebuf		*pex_pufbuf;
	TAILQ_ENTRY(puffs_executor)	pex_entries;
};

/*
 * usermount: describes one file system instance
 */
struct puffs_usermount {
	struct puffs_ops	pu_ops;

	int			pu_fd;
	size_t			pu_maxreqlen;

	uint32_t		pu_flags;
	int			pu_cc_stackshift;

	int			pu_kq;
	int			pu_state;
#define PU_STATEMASK	0xff
#define PU_INLOOP	0x100
#define PU_ASYNCFD	0x200
#define PU_HASKQ	0x400
#define PU_PUFFSDAEMON	0x800
#define PU_SETSTATE(pu, s) (pu->pu_state = (s) | (pu->pu_state & ~PU_STATEMASK))
	int			pu_dpipe[2];

	struct puffs_node	*pu_pn_root;

	LIST_HEAD(, puffs_node)	pu_pnodelst;
	LIST_HEAD(, puffs_cc)	pu_ccnukelst;
	TAILQ_HEAD(, puffs_cc)	pu_sched;

	TAILQ_HEAD(, puffs_executor) pu_exq;

	pu_cmap_fn		pu_cmap;

	pu_pathbuild_fn		pu_pathbuild;
	pu_pathtransform_fn	pu_pathtransform;
	pu_pathcmp_fn		pu_pathcmp;
	pu_pathfree_fn		pu_pathfree;
	pu_namemod_fn		pu_namemod;

	pu_errnotify_fn		pu_errnotify;

	pu_prepost_fn		pu_oppre;
	pu_prepost_fn		pu_oppost;

	struct puffs_framectrl	pu_framectrl[2];
#define PU_FRAMECTRL_FS   0
#define PU_FRAMECTRL_USER 1
	LIST_HEAD(, puffs_fctrl_io) pu_ios;
	LIST_HEAD(, puffs_fctrl_io) pu_ios_rmlist;
	struct kevent		*pu_evs;
	size_t			pu_nfds;

	puffs_ml_loop_fn	pu_ml_lfn;
	struct timespec		pu_ml_timeout;
	struct timespec		*pu_ml_timep;

	struct puffs_kargs	*pu_kargp;

	uint64_t		pu_nextreq;
	void			*pu_privdata;
};

/* call context */

struct puffs_cc {
	struct puffs_usermount	*pcc_pu;
	struct puffs_framebuf	*pcc_pb;

	ucontext_t		pcc_uc;		/* "continue" 		*/
	ucontext_t		pcc_uc_ret;	/* "yield" 		*/
	void			*pcc_stack;

	pid_t			pcc_pid;
	lwpid_t			pcc_lid;

	int			pcc_flags;

	TAILQ_ENTRY(puffs_cc)	entries;
	LIST_ENTRY(puffs_cc)	nlst_entries;
};
#define PCC_FAKECC	0x01
#define PCC_REALCC	0x02
#define PCC_DONE	0x04
#define PCC_BORROWED	0x08
#define PCC_HASCALLER	0x10
#define PCC_THREADED	0x20

#define pcc_callstat(a)	   (a->pcc_flags & PCC_CALL_MASK)
#define pcc_callset(a, b)  (a->pcc_flags = (a->pcc_flags & ~PCC_CALL_MASK) | b)

#define pcc_init_unreal(ap, type) 					\
do {									\
	memset(ap, 0, sizeof(*ap));					\
	(ap)->pcc_flags = type;						\
} while (/*CONSTCOND*/0)

struct puffs_newinfo {
	void		**pni_cookie;
	enum vtype	*pni_vtype;
	voff_t		*pni_size;
	dev_t		*pni_rdev;
};

#define PUFFS_MAKEKCRED(to, from)					\
	/*LINTED: tnilxnaht, the cast is ok */				\
	const struct puffs_kcred *to = (const void *)from
#define PUFFS_MAKECRED(to, from)					\
	/*LINTED: tnilxnaht, the cast is ok */				\
	const struct puffs_cred *to = (const void *)from
#define PUFFS_KCREDTOCRED(to, from)					\
	/*LINTED: tnilxnaht, the cast is ok */				\
	to = (void *)from

__BEGIN_DECLS

void	puffs_calldispatcher(struct puffs_cc *);

void	puffs_framev_input(struct puffs_usermount *, struct puffs_framectrl *,
			   struct puffs_fctrl_io *);
int	puffs_framev_output(struct puffs_usermount *, struct puffs_framectrl*,
			    struct puffs_fctrl_io *);
void	puffs_framev_exit(struct puffs_usermount *);
void	puffs_framev_readclose(struct puffs_usermount *,
			       struct puffs_fctrl_io *, int);
void	puffs_framev_writeclose(struct puffs_usermount *,
				struct puffs_fctrl_io *, int);
void	puffs_framev_notify(struct puffs_fctrl_io *, int);
void	*puffs__framebuf_getdataptr(struct puffs_framebuf *);
int	puffs__framev_addfd_ctrl(struct puffs_usermount *, int, int,
				 struct puffs_framectrl *);
void	puffs__framebuf_moveinfo(struct puffs_framebuf *,
				 struct puffs_framebuf *);

int	puffs_cc_create(struct puffs_usermount *, struct puffs_framebuf *,
			int, struct puffs_cc **);
void	puffs_cc_destroy(struct puffs_cc *);
void	puffs_cc_setcaller(struct puffs_cc *, pid_t, lwpid_t);
void	puffs_goto(struct puffs_cc *);

int	puffs_fsframe_read(struct puffs_usermount *, struct puffs_framebuf *,
			   int, int *);
int	puffs_fsframe_write(struct puffs_usermount *, struct puffs_framebuf *,
			   int, int *);
int	puffs_fsframe_cmp(struct puffs_usermount *, struct puffs_framebuf *,
			  struct puffs_framebuf *, int *);
void	puffs_fsframe_gotframe(struct puffs_usermount *,
			       struct puffs_framebuf *);

uint64_t	puffs__nextreq(struct puffs_usermount *pu);

__END_DECLS

#endif /* _PUFFS_PRIVATE_H_ */
