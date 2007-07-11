/*	$NetBSD: kern_physio.c,v 1.80.4.1 2007/07/11 20:09:52 mjf Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)kern_physio.c	8.1 (Berkeley) 6/10/93
 */

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *	@(#)kern_physio.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_physio.c,v 1.80.4.1 2007/07/11 20:09:52 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/once.h>
#include <sys/workqueue.h>

#include <uvm/uvm_extern.h>

ONCE_DECL(physio_initialized);
struct workqueue *physio_workqueue;

/*
 * The routines implemented in this file are described in:
 *	Leffler, et al.: The Design and Implementation of the 4.3BSD
 *	    UNIX Operating System (Addison Welley, 1989)
 * on pages 231-233.
 *
 * The routines "getphysbuf" and "putphysbuf" steal and return a swap
 * buffer.  Leffler, et al., says that swap buffers are used to do the
 * I/O, so raw I/O requests don't have to be single-threaded.  Of course,
 * NetBSD doesn't use "swap buffers" -- we have our own memory pool for
 * buffer descriptors.
 */

/* #define	PHYSIO_DEBUG */
#if defined(PHYSIO_DEBUG)
#define	DPRINTF(a)	printf a
#else /* defined(PHYSIO_DEBUG) */
#define	DPRINTF(a)	/* nothing */
#endif /* defined(PHYSIO_DEBUG) */

/* abuse these members/flags of struct buf */
#define	b_running	b_freelistindex
#define	b_endoffset	b_lblkno
#define	B_DONTFREE	B_AGE

/*
 * allocate a buffer structure for use in physical I/O.
 */
static struct buf *
getphysbuf(void)
{
	struct buf *bp;

	bp = getiobuf();
	bp->b_error = 0;
	bp->b_flags = B_BUSY;
	return(bp);
}

/*
 * get rid of a swap buffer structure which has been used in physical I/O.
 */
static void
putphysbuf(struct buf *bp)
{

	if ((bp->b_flags & B_DONTFREE) != 0) {
		return;
	}

	if (__predict_false(bp->b_flags & B_WANTED))
		panic("putphysbuf: private buf B_WANTED");
	putiobuf(bp);
}

static void
physio_done(struct work *wk, void *dummy)
{
	struct buf *bp = (void *)wk;
	size_t todo = bp->b_bufsize;
	size_t done = bp->b_bcount - bp->b_resid;
	struct buf *mbp = bp->b_private;

	KASSERT(&bp->b_work == wk);
	KASSERT(bp->b_bcount <= todo);
	KASSERT(bp->b_resid <= bp->b_bcount);
	KASSERT((bp->b_flags & B_PHYS) != 0);
	KASSERT(dummy == NULL);

	vunmapbuf(bp, todo);
	uvm_vsunlock(bp->b_proc->p_vmspace, bp->b_data, todo);

	simple_lock(&mbp->b_interlock);
	if (__predict_false(done != todo)) {
		off_t endoffset = dbtob(bp->b_blkno) + done;

		/*
		 * we got an error or hit EOM.
		 *
		 * we only care about the first one.
		 * ie. the one at the lowest offset.
		 */

		KASSERT(mbp->b_endoffset != endoffset);
		DPRINTF(("%s: error=%d at %" PRIu64 " - %" PRIu64
		    ", blkno=%" PRIu64 ", bcount=%d, flags=0x%x\n",
		    __func__, bp->b_error, dbtob(bp->b_blkno), endoffset,
		    bp->b_blkno, bp->b_bcount, bp->b_flags));

		if (mbp->b_endoffset == -1 || endoffset < mbp->b_endoffset) {
			int error;

			if ((bp->b_flags & B_ERROR) != 0) {
				if (bp->b_error == 0) {
					error = EIO; /* XXX */
				} else {
					error = bp->b_error;
				}
			} else {
				error = 0; /* EOM */
			}

			DPRINTF(("%s: mbp=%p, error %d -> %d, endoff %" PRIu64
			    " -> %" PRIu64 "\n",
			    __func__, mbp,
			    mbp->b_error, error,
			    mbp->b_endoffset, endoffset));

			mbp->b_endoffset = endoffset;
			mbp->b_error = error;
		}
		mbp->b_flags |= B_ERROR;
	} else {
		KASSERT((bp->b_flags & B_ERROR) == 0);
	}

	mbp->b_running--;
	if ((mbp->b_flags & B_WANTED) != 0) {
		mbp->b_flags &= ~B_WANTED;
		wakeup(mbp);
	}
	simple_unlock(&mbp->b_interlock);

	putphysbuf(bp);
}

static void
physio_biodone(struct buf *bp)
{
#if defined(DIAGNOSTIC)
	struct buf *mbp = bp->b_private;
	size_t todo = bp->b_bufsize;

	KASSERT(mbp->b_running > 0);
	KASSERT(bp->b_bcount <= todo);
	KASSERT(bp->b_resid <= bp->b_bcount);
#endif /* defined(DIAGNOSTIC) */

	workqueue_enqueue(physio_workqueue, &bp->b_work);
}

static int
physio_wait(struct buf *bp, int n, const char *wchan)
{
	int error = 0;

	LOCK_ASSERT(simple_lock_held(&bp->b_interlock));

	while (bp->b_running > n) {
		bp->b_flags |= B_WANTED;
		error = ltsleep(bp, PRIBIO + 1, wchan, 0, &bp->b_interlock);
		if (error) {
			break;
		}
	}

	return error;
}

static int
physio_init(void)
{
	int error;

	KASSERT(physio_workqueue == NULL);

	error = workqueue_create(&physio_workqueue, "physiod",
	    physio_done, NULL, PRIBIO, IPL_BIO, 0);

	return error;
}

#define	PHYSIO_CONCURRENCY	16	/* XXX tune */

/*
 * Do "physical I/O" on behalf of a user.  "Physical I/O" is I/O directly
 * from the raw device to user buffers, and bypasses the buffer cache.
 *
 * Comments in brackets are from Leffler, et al.'s pseudo-code implementation.
 */
int
physio(void (*strategy)(struct buf *), struct buf *obp, dev_t dev, int flags,
    void (*min_phys)(struct buf *), struct uio *uio)
{
	struct iovec *iovp;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int i, s;
	int error;
	int error2;
	struct buf *bp = NULL;
	struct buf *mbp;
	int concurrency = PHYSIO_CONCURRENCY - 1;

	error = RUN_ONCE(&physio_initialized, physio_init);
	if (__predict_false(error != 0)) {
		return error;
	}

	DPRINTF(("%s: called: off=%" PRIu64 ", resid=%zu\n",
	    __func__, uio->uio_offset, uio->uio_resid));

	flags &= B_READ | B_WRITE;

	/* Make sure we have a buffer, creating one if necessary. */
	if (obp != NULL) {
		/* [raise the processor priority level to splbio;] */
		s = splbio();
		simple_lock(&obp->b_interlock);

		/* [while the buffer is marked busy] */
		while (obp->b_flags & B_BUSY) {
			/* [mark the buffer wanted] */
			obp->b_flags |= B_WANTED;
			/* [wait until the buffer is available] */
			ltsleep(obp, PRIBIO+1, "physbuf", 0, &obp->b_interlock);
		}

		/* Mark it busy, so nobody else will use it. */
		obp->b_flags = B_BUSY | B_DONTFREE;

		/* [lower the priority level] */
		simple_unlock(&obp->b_interlock);
		splx(s);

		concurrency = 0; /* see "XXXkludge" comment below */
	}

	mbp = getphysbuf();
	mbp->b_running = 0;
	mbp->b_endoffset = -1;

	uvm_lwp_hold(l);

	for (i = 0; i < uio->uio_iovcnt; i++) {
		bool sync = true;

		iovp = &uio->uio_iov[i];
		while (iovp->iov_len > 0) {
			size_t todo;
			vaddr_t endp;

			simple_lock(&mbp->b_interlock);
			if ((mbp->b_flags & B_ERROR) != 0) {
				goto done_locked;
			}
			error = physio_wait(mbp, sync ? 0 : concurrency,
			    "physio1");
			if (error) {
				goto done_locked;
			}
			simple_unlock(&mbp->b_interlock);
			if (obp != NULL) {
				/*
				 * XXXkludge
				 * some drivers use "obp" as an identifier.
				 */
				bp = obp;
			} else {
				bp = getphysbuf();
			}
			bp->b_dev = dev;
			bp->b_proc = p;
			bp->b_private = mbp;
			bp->b_vp = NULL;

			/*
			 * [mark the buffer busy for physical I/O]
			 * (i.e. set B_PHYS (because it's an I/O to user
			 * memory, and B_RAW, because B_RAW is to be
			 * "Set by physio for raw transfers.", in addition
			 * to the "busy" and read/write flag.)
			 */
			bp->b_flags = (bp->b_flags & B_DONTFREE) |
			    B_BUSY | B_PHYS | B_RAW | B_CALL | flags;
			bp->b_iodone = physio_biodone;

			/* [set up the buffer for a maximum-sized transfer] */
			bp->b_blkno = btodb(uio->uio_offset);
			if (dbtob(bp->b_blkno) != uio->uio_offset) {
				error = EINVAL;
				goto done;
			}
			bp->b_bcount = MIN(MAXPHYS, iovp->iov_len);
			bp->b_data = iovp->iov_base;

			/*
			 * [call minphys to bound the transfer size]
			 * and remember the amount of data to transfer,
			 * for later comparison.
			 */
			(*min_phys)(bp);
			todo = bp->b_bufsize = bp->b_bcount;
#if defined(DIAGNOSTIC)
			if (todo > MAXPHYS)
				panic("todo(%zu) > MAXPHYS; minphys broken",
				    todo);
#endif /* defined(DIAGNOSTIC) */

			sync = false;
			endp = (vaddr_t)bp->b_data + todo;
			if (trunc_page(endp) != endp) {
				/*
				 * following requests can overlap.
				 * note that uvm_vslock does round_page.
				 */
				sync = true;
			}

			/*
			 * [lock the part of the user address space involved
			 *    in the transfer]
			 * Beware vmapbuf(); it clobbers b_data and
			 * saves it in b_saveaddr.  However, vunmapbuf()
			 * restores it.
			 */
			error = uvm_vslock(p->p_vmspace, bp->b_data, todo,
			    (flags & B_READ) ?  VM_PROT_WRITE : VM_PROT_READ);
			if (error) {
				goto done;
			}
			vmapbuf(bp, todo);

			BIO_SETPRIO(bp, BPRIO_TIMECRITICAL);

			simple_lock(&mbp->b_interlock);
			mbp->b_running++;
			simple_unlock(&mbp->b_interlock);

			/* [call strategy to start the transfer] */
			(*strategy)(bp);
			bp = NULL;

			iovp->iov_len -= todo;
			iovp->iov_base = (char *)iovp->iov_base + todo;
			uio->uio_offset += todo;
			uio->uio_resid -= todo;
		}
	}

done:
	simple_lock(&mbp->b_interlock);
done_locked:
	error2 = physio_wait(mbp, 0, "physio2");
	if (error == 0) {
		error = error2;
	}
	simple_unlock(&mbp->b_interlock);

	if ((mbp->b_flags & B_ERROR) != 0) {
		off_t delta;

		delta = uio->uio_offset - mbp->b_endoffset;
		KASSERT(delta > 0);
		uio->uio_resid += delta;
		/* uio->uio_offset = mbp->b_endoffset; */
	} else {
		KASSERT(mbp->b_endoffset == -1);
	}
	if (bp != NULL) {
		putphysbuf(bp);
	}
	if (error == 0) {
		error = mbp->b_error;
	}
	putphysbuf(mbp);

	/*
	 * [clean up the state of the buffer]
	 * Remember if somebody wants it, so we can wake them up below.
	 * Also, if we had to steal it, give it back.
	 */
	if (obp != NULL) {
		KASSERT((obp->b_flags & B_BUSY) != 0);
		KASSERT((obp->b_flags & B_DONTFREE) != 0);

		/*
		 * [if another process is waiting for the raw I/O buffer,
		 *    wake up processes waiting to do physical I/O;
		 */
		s = splbio();
		simple_lock(&obp->b_interlock);
		obp->b_flags &=
		    ~(B_BUSY | B_PHYS | B_RAW | B_CALL | B_DONTFREE);
		if ((obp->b_flags & B_WANTED) != 0) {
			obp->b_flags &= ~B_WANTED;
			wakeup(obp);
		}
		simple_unlock(&obp->b_interlock);
		splx(s);
	}
	uvm_lwp_rele(l);

	DPRINTF(("%s: done: off=%" PRIu64 ", resid=%zu\n",
	    __func__, uio->uio_offset, uio->uio_resid));

	return error;
}

/*
 * Leffler, et al., says on p. 231:
 * "The minphys() routine is called by physio() to adjust the
 * size of each I/O transfer before the latter is passed to
 * the strategy routine..."
 *
 * so, just adjust the buffer's count accounting to MAXPHYS here,
 * and return the new count;
 */
void
minphys(struct buf *bp)
{

	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}
