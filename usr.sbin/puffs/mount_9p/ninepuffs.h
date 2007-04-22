/*	$NetBSD: ninepuffs.h,v 1.2 2007/04/22 18:10:48 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
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

#ifndef PUFFS9P_H_
#define PUFFS9P_H_

#include <sys/queue.h>

#include <puffs.h>

PUFFSOP_PROTOS(puffs9p);

/* Qid structure.  optimized for in-mem.  different order on-wire */
struct qid9p {
	uint64_t	qidpath;
	uint32_t	qidvers;
	uint8_t		qidtype;
};

typedef uint16_t p9ptag_t;
typedef uint32_t p9pfid_t;

/*
 * refuse (no, not *that* refuse) to play if the server doesn't
 * support requests of at least the following size.  It would only
 * make life difficult
 */
#define P9P_MINREQLEN	512

#define P9P_DEFREQLEN	(16*1024)
#define P9P_INVALFID	0
#define P9P_ROOTFID	1

#define NEXTTAG(p9p)	\
    ((++(p9p->nexttag)) == P9PROTO_NOTAG ? p9p->nexttag = 0 : p9p->nexttag)

#define NEXTFID(p9p)	\
    ((++(p9p->nextfid)) == P9P_INVALFID ? p9p->nextfid = 2 : p9p->nextfid)

#define AUTOVAR(pcc)							\
	struct puffs9p *p9p = puffs_cc_getspecific(pcc);		\
	uint16_t tag = NEXTTAG(p9p);					\
	struct p9pbuf *pb = p9pbuf_make(p9p->maxreq, P9PB_OUT);		\
	int rv = 0

#define RETURN(rv)							\
	p9pbuf_destroy(pb);						\
	return (rv)

struct puffs9p;
/*
 * XXX: urgh
 *
 * This is slightly messy / abusatory structure.  It is used for multiple
 * things.  Typical life cycle: create output buffer, append to output
 * queue (pcc included) .  Once the buffer has been sent, the buffer is
 * freed and the structure is appended to reqqueue as psreq.  It is kept
 * there until matching network data is read.  Once this happens, the
 * data from the received buffer is copied to buffer stored on the queue.
 * This should be rewritten, clearly.
 *
 * Also, this should not be copypasted from psshfs.  But that's
 * another matter ;)
 */
#define P9PB_OUT 0
#define P9PB_IN 1
struct p9pbuf {
	struct p9preq {
		p9ptag_t tagid;

		/* either ... */
		struct puffs_cc *pcc;

		/* ... or */
		void (*func)(struct puffs9p *, struct p9pbuf *, void *);
		void *arg;
		/* no union, we'd need a "which" flag */

		TAILQ_ENTRY(p9pbuf) entries;
	} p9pr;

	/* in / out */
	uint32_t len;
	uint32_t offset;
	uint8_t *buf;

	/* helpers for in */
	uint32_t remain;
	uint8_t type;
	p9ptag_t tagid;

	int state;
};
#define P9PBUF_PUT 0
#define P9PBUF_PUTDONE 1
#define P9PBUF_GETLEN 2
#define P9PBUF_GETDATA 3
#define P9PBUF_GETREADY 4

#define P9PB_CHECK(pb, space) if (pb->remain < (space)) return ENOMEM

struct puffs9p {
	int servsock;

	p9ptag_t nexttag;
	p9pfid_t nextfid;

	size_t maxreq;		/* negotiated with server */
	struct p9pbuf *curpb;

	TAILQ_HEAD(, p9pbuf) outbufq;
	TAILQ_HEAD(, p9pbuf) req_queue;
};

struct dirfid {
	p9pfid_t	fid;
	off_t		seekoff;
	LIST_ENTRY(dirfid) entries;
};

struct p9pnode {
	p9pfid_t	fid_base;
	p9pfid_t	fid_read;
	p9pfid_t	fid_write;
	int		opencount;

	LIST_HEAD(,dirfid) dir_openlist;
};

struct p9pbuf 	*p9pbuf_make(size_t, int);
void		p9pbuf_destroy(struct p9pbuf *);
void		p9pbuf_recycle(struct p9pbuf *, int);

int		p9pbuf_read(struct puffs9p *, struct p9pbuf *);
int		p9pbuf_write(struct puffs9p *, struct p9pbuf *);

void		outbuf_enqueue(struct puffs9p *, struct p9pbuf *,
			       struct puffs_cc *, uint16_t);
void		outbuf_enqueue_nocc(struct puffs9p *, struct p9pbuf *,
				    void (*f)(struct puffs9p *,
					struct p9pbuf *, void *),
					void *, uint16_t);
struct p9pbuf	*req_get(struct puffs9p *, uint16_t);


int	p9pbuf_put_1(struct p9pbuf *, uint8_t);
int	p9pbuf_put_2(struct p9pbuf *, uint16_t);
int	p9pbuf_put_4(struct p9pbuf *, uint32_t);
int	p9pbuf_put_8(struct p9pbuf *, uint64_t);
int	p9pbuf_put_str(struct p9pbuf *, const char *);
int	p9pbuf_put_data(struct p9pbuf *, const void *, uint16_t);
int	p9pbuf_write_data(struct p9pbuf *, uint8_t *, uint32_t);

int	p9pbuf_get_1(struct p9pbuf *, uint8_t *);
int	p9pbuf_get_2(struct p9pbuf *, uint16_t *);
int	p9pbuf_get_4(struct p9pbuf *, uint32_t *);
int	p9pbuf_get_8(struct p9pbuf *, uint64_t *);
int	p9pbuf_get_str(struct p9pbuf *, char **, uint16_t *);
int	p9pbuf_get_data(struct p9pbuf *, uint8_t **, uint16_t *);
int	p9pbuf_read_data(struct p9pbuf *, uint8_t *, uint32_t);

int	p9pbuf_remaining(struct p9pbuf *);
int	p9pbuf_tell(struct p9pbuf *);
void	p9pbuf_seekset(struct p9pbuf *, int);

int	proto_getqid(struct p9pbuf *, struct qid9p *);
int	proto_getstat(struct p9pbuf *, struct vattr *, char **, uint16_t *);
int	proto_expect_walk_nqids(struct p9pbuf *, uint16_t *);
int	proto_expect_stat(struct p9pbuf *, struct vattr *);
int	proto_expect_qid(struct p9pbuf *, uint8_t, struct qid9p *);

int	proto_cc_dupfid(struct puffs_cc *, p9pfid_t, p9pfid_t);
int	proto_cc_clunkfid(struct puffs_cc *, p9pfid_t, int);
int	proto_cc_open(struct puffs_cc *, p9pfid_t, p9pfid_t, int);

void	proto_make_stat(struct p9pbuf *, const struct vattr *, const char *);

struct puffs_node	*p9p_handshake(struct puffs_usermount *, const char *);

void			qid2vattr(struct vattr *, const struct qid9p *);
struct puffs_node	*newp9pnode_va(struct puffs_usermount *,
				       const struct vattr *, p9pfid_t);
struct puffs_node	*newp9pnode_qid(struct puffs_usermount *,
					const struct qid9p *, p9pfid_t);

int	getdfwithoffset(struct puffs_cc *, struct p9pnode *, off_t,
			 struct dirfid **);
void	storedf(struct p9pnode *, struct dirfid *);
void	releasedf(struct puffs_cc *, struct dirfid *);
void	nukealldf(struct puffs_cc *, struct p9pnode *);

#endif /* PUFFS9P_H_ */
