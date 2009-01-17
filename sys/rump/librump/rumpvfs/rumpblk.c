/*	$NetBSD: rumpblk.c,v 1.1.2.2 2009/01/17 13:29:38 mjf Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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

/*
 * Block device emulation.  Presents a block device interface and
 * uses rumpuser system calls to satisfy I/O requests.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rumpblk.c,v 1.1.2.2 2009/01/17 13:29:38 mjf Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/stat.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

#define RUMPBLK_SIZE 16
static struct rblkdev {
	char *rblk_path;
	int rblk_fd;

	struct partition *rblk_curpi;
	struct partition rblk_pi;
	struct disklabel rblk_dl;
} minors[RUMPBLK_SIZE];

dev_type_open(rumpblk_open);
dev_type_close(rumpblk_close);
dev_type_read(rumpblk_read);
dev_type_write(rumpblk_write);
dev_type_ioctl(rumpblk_ioctl);
dev_type_strategy(rumpblk_strategy);
dev_type_dump(rumpblk_dump);
dev_type_size(rumpblk_size);

static const struct bdevsw rumpblk_bdevsw = {
	rumpblk_open, rumpblk_close, rumpblk_strategy, rumpblk_ioctl,
	nodump, nosize, D_DISK
};

static const struct cdevsw rumpblk_cdevsw = {
	rumpblk_open, rumpblk_close, rumpblk_read, rumpblk_write,
	rumpblk_ioctl, nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

/* XXX: not mpsafe */

int
rumpblk_init()
{
	int rumpblk = RUMPBLK;

	return devsw_attach("rumpblk", &rumpblk_bdevsw, &rumpblk,
	    &rumpblk_cdevsw, &rumpblk);
}

int
rumpblk_register(const char *path)
{
	size_t len;
	int i;

	for (i = 0; i < RUMPBLK_SIZE; i++)
		if (minors[i].rblk_path && strcmp(minors[i].rblk_path, path)==0)
			return i;

	for (i = 0; i < RUMPBLK_SIZE; i++)
		if (minors[i].rblk_path == NULL)
			break;
	if (i == RUMPBLK_SIZE)
		return -1;

	len = strlen(path);
	minors[i].rblk_path = malloc(len+1, M_TEMP, M_WAITOK);
	strcpy(minors[i].rblk_path, path);
	minors[i].rblk_fd = -1;
	return i;
}

int
rumpblk_open(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct rblkdev *rblk = &minors[minor(dev)];
	struct stat sb;
	int error, fd;

	KASSERT(rblk->rblk_fd == -1);
	fd = rumpuser_open(rblk->rblk_path, OFLAGS(flag), &error);
	if (error)
		return error;

	/*
	 * Setup partition info.  First try the usual. */
	if (rumpuser_ioctl(fd, DIOCGDINFO, &rblk->rblk_dl, &error) != -1) {
		/*
		 * If that works, use it.  We still need to guess
		 * which partition we are on.
		 */
		rblk->rblk_curpi = &rblk->rblk_dl.d_partitions[0];
	} else {
		/*
		 * If that didn't work, assume were a regular file
		 * and just try to fake the info the best we can.
		 */
		memset(&rblk->rblk_dl, 0, sizeof(rblk->rblk_dl));

		if (rumpuser_stat(rblk->rblk_path, &sb, &error) == -1) {
			int dummy;

			rumpuser_close(fd, &dummy);
			return error;
		}
		rblk->rblk_pi.p_size = sb.st_size >> DEV_BSHIFT;
		rblk->rblk_dl.d_secsize = DEV_BSIZE;
		rblk->rblk_curpi = &rblk->rblk_pi;
	}
	rblk->rblk_fd = fd;

	return 0;
}

int
rumpblk_close(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct rblkdev *rblk = &minors[minor(dev)];
	int dummy;

	rumpuser_close(rblk->rblk_fd, &dummy);
	rblk->rblk_fd = -1;

	return 0;
}

int
rumpblk_ioctl(dev_t dev, u_long xfer, void *addr, int flag, struct lwp *l)
{
	struct rblkdev *rblk = &minors[minor(dev)];
	int rv, error;

	if (xfer == DIOCGPART) {
		struct partinfo *pi = (struct partinfo *)addr;

		pi->part = rblk->rblk_curpi;
		pi->disklab = &rblk->rblk_dl;

		return 0;
	}

	rv = rumpuser_ioctl(rblk->rblk_fd, xfer, addr, &error);
	if (rv == -1)
		return error;

	return 0;
}

int
rumpblk_read(dev_t dev, struct uio *uio, int flags)
{

	panic("%s: unimplemented", __func__);
}

int
rumpblk_write(dev_t dev, struct uio *uio, int flags)
{

	panic("%s: unimplemented", __func__);
}

void
rumpblk_strategy(struct buf *bp)
{
	struct rblkdev *rblk = &minors[minor(bp->b_dev)];
	off_t off;
	int async;

	off = bp->b_blkno << DEV_BSHIFT;
	DPRINTF(("rumpblk_strategy: 0x%x bytes %s off 0x%" PRIx64
	    " (0x%" PRIx64 " - 0x%" PRIx64")\n",
	    bp->b_bcount, BUF_ISREAD(bp) "READ" : "WRITE",
	    off, off, (off + bp->b_bcount)));

	/*
	 * Do I/O.  We have different paths for async and sync I/O.
	 * Async I/O is done by passing a request to rumpuser where
	 * it is executed.  The rumpuser routine then calls
	 * biodone() to signal any waiters in the kernel.  I/O's are
	 * executed in series.  Technically executing them in parallel
	 * would produce better results, but then we'd need either
	 * more threads or posix aio.  Maybe worth investigating
	 * this later.
	 *
	 * Synchronous I/O is done directly in the context mainly to
	 * avoid unnecessary scheduling with the I/O thread.
	 */
	async = bp->b_flags & B_ASYNC;
	if (async && rump_threads) {
		struct rumpuser_aio *rua;

		rua = kmem_alloc(sizeof(struct rumpuser_aio), KM_SLEEP);
		rua->rua_fd = rblk->rblk_fd;
		rua->rua_data = bp->b_data;
		rua->rua_dlen = bp->b_bcount;
		rua->rua_off = off;
		rua->rua_bp = bp;
		rua->rua_op = BUF_ISREAD(bp);

		rumpuser_mutex_enter(&rumpuser_aio_mtx);

		/*
		 * Check if our buffer is full.  Doing it this way
		 * throttles the I/O a bit if we have a massive
		 * async I/O burst.
		 *
		 * XXX: this actually leads to deadlocks with spl()
		 * (caller maybe be at splbio() legally for async I/O),
		 * so for now set N_AIOS high and FIXXXME some day.
		 */
		if ((rumpuser_aio_head+1) % N_AIOS == rumpuser_aio_tail) {
			kmem_free(rua, sizeof(*rua));
			rumpuser_mutex_exit(&rumpuser_aio_mtx);
			goto syncfallback;
		}

		/* insert into queue & signal */
		rumpuser_aios[rumpuser_aio_head] = rua;
		rumpuser_aio_head = (rumpuser_aio_head+1) % (N_AIOS-1);
		rumpuser_cv_signal(&rumpuser_aio_cv);
		rumpuser_mutex_exit(&rumpuser_aio_mtx);
	} else {
 syncfallback:
		if (BUF_ISREAD(bp)) {
			rumpuser_read_bio(rblk->rblk_fd, bp->b_data,
			    bp->b_bcount, off, rump_biodone, bp);
		} else {
			rumpuser_write_bio(rblk->rblk_fd, bp->b_data,
			    bp->b_bcount, off, rump_biodone, bp);
		}
		if (!async) {
			int error;

			if (BUF_ISWRITE(bp))
				rumpuser_fsync(rblk->rblk_fd, &error);
		}
	}
}
