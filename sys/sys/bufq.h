/*	$NetBSD: bufq.h,v 1.1.2.2 2004/11/02 07:53:37 skrll Exp $	*/
/*	NetBSD: buf.h,v 1.75 2004/09/18 16:40:11 yamt Exp 	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)buf.h	8.9 (Berkeley) 3/30/95
 */

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

struct buf;

/*
 * Device driver buffer queue.
 */
struct bufq_state {
	void (*bq_put)(struct bufq_state *, struct buf *);
	struct buf *(*bq_get)(struct bufq_state *, int);
	void *bq_private;
	int bq_flags;			/* Flags from bufq_alloc() */
};

/*
 * Flags for bufq_alloc.
 */
#define BUFQ_SORT_RAWBLOCK	0x0001	/* Sort by b_rawblkno */
#define BUFQ_SORT_CYLINDER	0x0002	/* Sort by b_cylinder, b_rawblkno */

#define BUFQ_FCFS		0x0010	/* First-come first-serve */
#define BUFQ_DISKSORT		0x0020	/* Min seek sort */
#define BUFQ_READ_PRIO		0x0030	/* Min seek and read priority */
#define BUFQ_PRIOCSCAN		0x0040	/* Per-priority CSCAN */

#define BUFQ_SORT_MASK		0x000f
#define BUFQ_METHOD_MASK	0x00f0

extern int bufq_disk_default_strat;
#define	BUFQ_DISK_DEFAULT_STRAT()	bufq_disk_default_strat
void	bufq_alloc(struct bufq_state *, int);
void	bufq_free(struct bufq_state *);

#define BUFQ_PUT(bufq, bp) \
	(*(bufq)->bq_put)((bufq), (bp))	/* Put buffer in queue */
#define BUFQ_GET(bufq) \
	(*(bufq)->bq_get)((bufq), 1)	/* Get and remove buffer from queue */
#define BUFQ_PEEK(bufq) \
	(*(bufq)->bq_get)((bufq), 0)	/* Get buffer from queue */

void	bufq_fcfs_init(struct bufq_state *);
void	bufq_disksort_init(struct bufq_state *);
void	bufq_readprio_init(struct bufq_state *);
void	bufq_priocscan_init(struct bufq_state *);

static __inline int buf_inorder(const struct buf *, const struct buf *, int)
    __unused;

#include <sys/null.h> /* for NULL */

/*
 * Check if two buf's are in ascending order.
 */
static __inline int
buf_inorder(const struct buf *bp, const struct buf *bq, int sortby)
{

	if (bp == NULL || bq == NULL)
		return (bq == NULL);

	if (sortby == BUFQ_SORT_CYLINDER) {
		if (bp->b_cylinder != bq->b_cylinder)
			return bp->b_cylinder < bq->b_cylinder;
		else
			return bp->b_rawblkno < bq->b_rawblkno;
	} else
		return bp->b_rawblkno < bq->b_rawblkno;
}
