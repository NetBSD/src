/*	$NetBSD: psshfs.h,v 1.10 2007/04/19 20:31:09 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

#ifndef PSSHFS_H_
#define PSSHFS_H_

#include <sys/queue.h>

#include <puffs.h>

/*
 * Later protocol versions would have some advantages (such as link count
 * supported directly as a part of stat), but since proto version 3 seems
 * to be the only widely available version, let's not try to jump through
 * too many hoops to be compatible with all versions.
 */
#define SFTP_PROTOVERSION 3

/*
 * Refresh directories every n seconds as indicated by the following macro.
 * Note that local changes will still be visible immediately.
 */
#define PSSHFS_REFRESHIVAL 30

/* warm getattr cache in readdir */
#define SUPERREADDIR

PUFFSOP_PROTOS(psshfs);

#define NEXTREQ(pctx) ((pctx->nextreq)++)
#define PSSHFSAUTOVAR(pcc)						\
	struct psshfs_ctx *pctx = puffs_cc_getspecific(pcc);		\
	uint32_t reqid = NEXTREQ(pctx);					\
	struct psbuf *pb = psbuf_make(PSB_OUT);				\
	int rv = 0

#define PSSHFSRETURN(rv)						\
	psbuf_destroy(pb);						\
	return (rv)

struct psshfs_dir {
	int valid;
	struct puffs_node *entry;

	char *entryname;
	struct vattr va;
	time_t attrread;
};

struct psshfs_fid {
	time_t mounttime;
	struct puffs_node *node;
};

struct psshfs_node {
	struct puffs_node *parent;

	struct psshfs_dir *dir;	/* only valid if we're of type VDIR */
	size_t denttot;
	size_t dentnext;
	time_t dentread;
	int childcount;
	int reclaimed;
	int hasfh;

	time_t attrread;
};

struct psshfs_ctx;
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
 */
#define PSDEFALLOC 0x2000
#define PSBUFMAX 0x40000
#define PSB_OUT 0
#define PSB_IN 1
struct psbuf {
	struct psreq {
		uint32_t reqid;

		/* either ... */
		struct puffs_cc *pcc;

		/* ... or */
		void (*func)(struct psshfs_ctx *, struct psbuf *, void *);
		void *arg;
		/* no union, we'd need a "which" flag */

		TAILQ_ENTRY(psbuf) entries;
	} psr;

	/* in / out */
	uint32_t len;
	uint32_t remain;
	uint32_t offset;
	uint8_t *buf;

	int state;

	/* helpers for in */
	uint8_t type;		/* buf[0] */
	uint32_t reqid;		/* buf[1-4] */
};
#define PSBUF_PUT 0
#define PSBUF_PUTDONE 1
#define PSBUF_GETLEN 2
#define PSBUF_GETDATA 3
#define PSBUF_GETREADY 4

struct psshfs_ctx {
	int sshfd;
	pid_t sshpid;
	const char *mountpath;

	int protover;
	uint32_t nextreq;

	struct psbuf *curpb;

	struct psshfs_node psn_root;
	ino_t nextino;

	int canexport;
	time_t mounttime;

	TAILQ_HEAD(, psbuf) outbufq;
	TAILQ_HEAD(, psbuf) req_queue;
};

int	psshfs_domount(struct puffs_usermount *);

struct psbuf 	*psbuf_make(int);
void		psbuf_destroy(struct psbuf *);
void		psbuf_recycle(struct psbuf *, int);

int		psbuf_read(struct psshfs_ctx *, struct psbuf *);
int		psbuf_write(struct psshfs_ctx *, struct psbuf *);

void		pssh_outbuf_enqueue(struct psshfs_ctx *, struct psbuf *,
				    struct puffs_cc *, uint32_t);
void		pssh_outbuf_enqueue_nocc(struct psshfs_ctx *, struct psbuf *,
				    void (*f)(struct psshfs_ctx *,
					      struct psbuf *, void *),
				    void *, uint32_t);
struct psbuf	*psshreq_get(struct psshfs_ctx *, uint32_t);


int	psbuf_put_1(struct psbuf *, uint8_t);
int	psbuf_put_2(struct psbuf *, uint16_t);
int	psbuf_put_4(struct psbuf *, uint32_t);
int	psbuf_put_8(struct psbuf *, uint64_t);
int	psbuf_put_str(struct psbuf *, const char *);
int	psbuf_put_data(struct psbuf *, const void *, uint32_t);
int	psbuf_put_vattr(struct psbuf *, const struct vattr *);

int	psbuf_get_1(struct psbuf *, uint8_t *);
int	psbuf_get_2(struct psbuf *, uint16_t *);
int	psbuf_get_4(struct psbuf *, uint32_t *);
int	psbuf_get_8(struct psbuf *, uint64_t *);
int	psbuf_get_str(struct psbuf *, char **, uint32_t *);
int	psbuf_get_vattr(struct psbuf *, struct vattr *);

int	psbuf_expect_status(struct psbuf *);
int	psbuf_expect_handle(struct psbuf *, char **, uint32_t *);
int	psbuf_expect_name(struct psbuf *, uint32_t *);
int	psbuf_expect_attrs(struct psbuf *, struct vattr *);

int	psbuf_do_data(struct psbuf *, uint8_t *, uint32_t *);

int	psbuf_req_data(struct psbuf *, int, uint32_t, const void *, uint32_t);
int	psbuf_req_str(struct psbuf *, int, uint32_t, const char *);


int	sftp_readdir(struct puffs_cc *, struct psshfs_ctx *,
		     struct puffs_node *);

struct psshfs_dir *lookup(struct psshfs_dir *, size_t, const char *);
struct puffs_node *makenode(struct puffs_usermount *, struct puffs_node *,
			    struct psshfs_dir *, const struct vattr *);
struct puffs_node *allocnode(struct puffs_usermount *, struct puffs_node *,
			    const char *, const struct vattr *);
struct psshfs_dir *direnter(struct puffs_node *, const char *);
void nukenode(struct puffs_node *, const char *, int);
void doreclaim(struct puffs_node *);
int getpathattr(struct puffs_cc *, const char *, struct vattr *);
int getnodeattr(struct puffs_cc *, struct puffs_node *);

#endif /* PSSHFS_H_ */
