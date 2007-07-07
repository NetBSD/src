/*	$NetBSD: puffs_priv.h,v 1.17 2007/07/07 21:13:42 pooka Exp $	*/

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

#define PU_CMAP(pu, c)	(pu->pu_cmap ? pu->pu_cmap(c) : (struct puffs_node *)c)

struct puffs_framectrl {
	puffs_framev_readframe_fn rfb;
	puffs_framev_writeframe_fn wfb;
	puffs_framev_cmpframe_fn cmpfb;
	puffs_framev_fdnotify_fn fdnotfn;

	struct kevent *evs;
	size_t nfds;

	struct timespec timeout;
	struct timespec *timp;

	LIST_HEAD(, puffs_fctrl_io) fb_ios;
	LIST_HEAD(, puffs_fctrl_io) fb_ios_rmlist;
};

struct puffs_fctrl_io {
	int io_fd;
	int stat;

	struct puffs_framebuf *cur_in;

	TAILQ_HEAD(, puffs_framebuf) snd_qing;	/* queueing to be sent */
	TAILQ_HEAD(, puffs_framebuf) res_qing;	/* q'ing for rescue */

	LIST_ENTRY(puffs_fctrl_io) fio_entries;
};
#define FIO_WR		0x01
#define FIO_WRGONE	0x02
#define FIO_RDGONE	0x04
#define FIO_DEAD	0x08
#define FIO_ENABLE_R	0x10
#define FIO_ENABLE_W	0x20

#define FIO_EN_WRITE(fio)			\
    (!(fio->stat & FIO_WR)			\
      && !TAILQ_EMPTY(&fio->snd_qing)		\
      && (fio->stat & FIO_ENABLE_W))

#define FIO_RM_WRITE(fio)			\
    ((fio->stat & FIO_WR)			\
      && (TAILQ_EMPTY(&fio->snd_qing)		\
        || ((fio->stat & FIO_ENABLE_W) == 0)))

/*
 * usermount: describes one file system instance
 */
struct puffs_usermount {
	struct puffs_ops	pu_ops;
	struct puffs_kargs	pu_kargs;

	uint32_t		pu_flags;
	size_t			pu_cc_stacksize;

	int			pu_kq;
	int			pu_haskq;
	int			pu_state;
#define PU_STATEMASK	0xff
#define PU_INLOOP	0x100
#define PU_ASYNCFD	0x200
#define PU_SETSTATE(pu, s) (pu->pu_state = (s) | (pu->pu_state & ~PU_STATEMASK))

	struct puffs_node	*pu_pn_root;

	LIST_HEAD(, puffs_node)	pu_pnodelst;
	LIST_HEAD(, puffs_cc)	pu_ccnukelst;

	struct puffs_node	*(*pu_cmap)(void *);

	pu_pathbuild_fn		pu_pathbuild;
	pu_pathtransform_fn	pu_pathtransform;
	pu_pathcmp_fn		pu_pathcmp;
	pu_pathfree_fn		pu_pathfree;
	pu_namemod_fn		pu_namemod;

	struct puffs_framectrl	pu_framectrl;

	puffs_ml_loop_fn	pu_ml_lfn;
	struct timespec		pu_ml_timeout;
	struct timespec		*pu_ml_timep;

	void			*pu_privdata;
};

/* call context */

struct puffs_cc {
	struct puffs_usermount	*pcc_pu;
	struct puffs_req	*pcc_preq;

	ucontext_t		pcc_uc;		/* "continue" 		*/
	ucontext_t		pcc_uc_ret;	/* "yield" 		*/
	void			*pcc_stack;

	int			pcc_flags;
	struct puffs_putreq	*pcc_ppr;

	TAILQ_ENTRY(puffs_cc)	entries;
	LIST_ENTRY(puffs_cc)	nlst_entries;
};
#define PCC_FAKECC	0x01
#define PCC_REALCC	0x02
#define PCC_DONE	0x04
#define PCC_BORROWED	0x08

#define pcc_callstat(a)	   (a->pcc_flags & PCC_CALL_MASK)
#define pcc_callset(a, b)  (a->pcc_flags = (a->pcc_flags & ~PCC_CALL_MASK) | b)

#define pcc_init_local(ap)   						\
do {									\
	memset(ap, 0, sizeof(*ap));					\
	(ap)->pcc_flags = PCC_FAKECC;					\
} while (/*CONSTCOND*/0)

/*
 * Reqs
 */

struct puffs_getreq {
	struct puffs_usermount	*pgr_pu;

	struct puffs_reqh_get	pgr_phg;
	struct puffs_reqh_get	pgr_phg_orig;

	struct puffs_req	*pgr_nextpreq;
	size_t			pgr_advance;

	/* diagnostics */
	int			pgr_nppr;
};

struct puffs_putreq {
	struct puffs_usermount *ppr_pu;

	struct puffs_reqh_put	ppr_php;

	/* to adjust next request info */
	void			**ppr_buf;
	size_t			*ppr_buflen;
	uint64_t 		*ppr_id;

	/* for delayed action freeing of preq's */
	TAILQ_HEAD(, puffs_cc)	ppr_pccq;

	/* diagnostics */
	struct puffs_getreq	*ppr_pgr;
};

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

#define PUFFS_MAKEKCID(to, from)					\
	/*LINTED: tnilxnaht, the cast is ok */				\
	const struct puffs_kcid *to = (const void *)from
#define PUFFS_MAKECID(to, from)						\
	/*LINTED: tnilxnaht, the cast is ok */				\
	const struct puffs_cid *to = (const void *)from
#define PUFFS_KCIDTOCID(to, from)					\
	/*LINTED: tnilxnaht, the cast is ok */				\
	to = (void *)from

__BEGIN_DECLS

void	puffs_calldispatcher(struct puffs_cc *);

void	puffs_framev_input(struct puffs_usermount *, struct puffs_framectrl *,
			   struct puffs_fctrl_io *, struct puffs_putreq *);
int	puffs_framev_output(struct puffs_usermount *, struct puffs_framectrl*,
			    struct puffs_fctrl_io *, struct puffs_putreq *);
void	puffs_framev_exit(struct puffs_usermount *);
void	puffs_framev_readclose(struct puffs_usermount *,
			       struct puffs_fctrl_io *, int);
void	puffs_framev_writeclose(struct puffs_usermount *,
				struct puffs_fctrl_io *, int);

void	puffs_goto(struct puffs_cc *);

__END_DECLS

#endif /* _PUFFS_PRIVATE_H_ */
