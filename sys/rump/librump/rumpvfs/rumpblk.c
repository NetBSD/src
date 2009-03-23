/*	$NetBSD: rumpblk.c,v 1.14 2009/03/23 11:52:42 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rumpblk.c,v 1.14 2009/03/23 11:52:42 pooka Exp $");

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
	uint8_t *rblk_mem;
	off_t rblk_size;

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
dev_type_strategy(rumpblk_strategy_fail);
dev_type_dump(rumpblk_dump);
dev_type_size(rumpblk_size);

static const struct bdevsw rumpblk_bdevsw = {
	rumpblk_open, rumpblk_close, rumpblk_strategy, rumpblk_ioctl,
	nodump, nosize, D_DISK
};

static const struct bdevsw rumpblk_bdevsw_fail = {
	rumpblk_open, rumpblk_close, rumpblk_strategy_fail, rumpblk_ioctl,
	nodump, nosize, D_DISK
};

static const struct cdevsw rumpblk_cdevsw = {
	rumpblk_open, rumpblk_close, rumpblk_read, rumpblk_write,
	rumpblk_ioctl, nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

/* fail every n out of BLKFAIL_MAX */
#define BLKFAIL_MAX 10000
static int blkfail;
static unsigned randstate;

int
rumpblk_init(void)
{
	char buf[64];
	int rumpblk = RUMPBLK;
	int error;

	if (rumpuser_getenv("RUMP_BLKFAIL", buf, sizeof(buf), &error) == 0) {
		blkfail = strtoul(buf, NULL, 10);
		/* fail everything */
		if (blkfail > BLKFAIL_MAX)
			blkfail = BLKFAIL_MAX;
		if (rumpuser_getenv("RUMP_BLKFAIL_SEED", buf, sizeof(buf),
		    &error) == 0) {
			randstate = strtoul(buf, NULL, 10);
		} else {
			randstate = arc4random(); /* XXX: not enough entropy */
		}
		printf("rumpblk: FAULT INJECTION ACTIVE!  every %d out of"
		    " %d I/O will fail.  key %u\n", blkfail, BLKFAIL_MAX,
		    randstate);
	} else {
		blkfail = 0;
	}

	if (blkfail) {
		return devsw_attach("rumpblk", &rumpblk_bdevsw_fail, &rumpblk,
		    &rumpblk_cdevsw, &rumpblk);
	} else {
		return devsw_attach("rumpblk", &rumpblk_bdevsw, &rumpblk,
		    &rumpblk_cdevsw, &rumpblk);
	}
}

int
rumpblk_register(const char *path)
{
	size_t len;
	int i;

	for (i = 0; i < RUMPBLK_SIZE; i++)
		if (minors[i].rblk_path && strcmp(minors[i].rblk_path, path) == 0)
			return i;

	for (i = 0; i < RUMPBLK_SIZE; i++)
		if (minors[i].rblk_path == NULL)
			break;
	if (i == RUMPBLK_SIZE)
		return -1;

	len = strlen(path);
	minors[i].rblk_path = malloc(len + 1, M_TEMP, M_WAITOK);
	strcpy(minors[i].rblk_path, path);
	minors[i].rblk_fd = -1;
	return i;
}

int
rumpblk_open(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct rblkdev *rblk = &minors[minor(dev)];
	uint8_t *mem = NULL;
	uint64_t fsize;
	int ft, dummy;
	int error, fd;

	KASSERT(rblk->rblk_fd == -1);
	fd = rumpuser_open(rblk->rblk_path, OFLAGS(flag), &error);
	if (error)
		return error;

	if (rumpuser_getfileinfo(rblk->rblk_path, &fsize, &ft, &error) == -1) {
		rumpuser_close(fd, &dummy);
		return error;
	}

	if (ft == RUMPUSER_FT_REG) {
		/*
		 * Try to mmap the file if it's size is max. half of
		 * the address space.  If mmap fails due to e.g. limits,
		 * we fall back to the read/write path.  This test is only
		 * to prevent size_t vs. off_t wraparounds.
		 */
		if (fsize < UINT64_C(1) << (sizeof(void *) * 8 - 1)) {
			int mmflags;

			mmflags = 0;
			if (flag & FREAD)
				mmflags |= RUMPUSER_FILEMMAP_READ;
			if (flag & FWRITE) {
				mmflags |= RUMPUSER_FILEMMAP_WRITE;
				mmflags |= RUMPUSER_FILEMMAP_SHARED;
			}
			mem = rumpuser_filemmap(fd, 0, fsize, mmflags, &error);
		}

		memset(&rblk->rblk_dl, 0, sizeof(rblk->rblk_dl));

		rblk->rblk_size = fsize;
		rblk->rblk_pi.p_size = fsize >> DEV_BSHIFT;
		rblk->rblk_dl.d_secsize = DEV_BSIZE;
		rblk->rblk_curpi = &rblk->rblk_pi;
	} else {
		if (rumpuser_ioctl(fd, DIOCGDINFO, &rblk->rblk_dl,
		    &error) != -1) {
			rumpuser_close(fd, &dummy);
			return error;
		}

		rblk->rblk_curpi = &rblk->rblk_dl.d_partitions[0];
	}
	rblk->rblk_fd = fd;
	rblk->rblk_mem = mem;
	if (rblk->rblk_mem != NULL)
		printf("rumpblk%d: using mmio for %s\n",
		    minor(dev), rblk->rblk_path);

	return 0;
}

int
rumpblk_close(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct rblkdev *rblk = &minors[minor(dev)];
	int dummy;

	if (rblk->rblk_mem) {
		KASSERT(rblk->rblk_size);
		rumpuser_memsync(rblk->rblk_mem, rblk->rblk_size, &dummy);
		rumpuser_unmap(rblk->rblk_mem, rblk->rblk_size);
		rblk->rblk_mem = NULL;
	}
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

static void
dostrategy(struct buf *bp)
{
	struct rblkdev *rblk = &minors[minor(bp->b_dev)];
	off_t off;
	int async, error;

	off = bp->b_blkno << DEV_BSHIFT;
	/*
	 * Do bounds checking if we're working on a file.  Otherwise
	 * invalid file systems might attempt to read beyond EOF.  This
	 * is bad(tm) especially on mmapped images.  This is essentially
	 * the kernel bounds_check() routines.
	 */
	if (rblk->rblk_size && off + bp->b_bcount > rblk->rblk_size) {
		int64_t sz = rblk->rblk_size - off;

		/* EOF */
		if (sz == 0) {
			rump_biodone(bp, 0, 0);
			return;
		}
		/* beyond EOF ==> error */
		if (sz < 0) {
			rump_biodone(bp, 0, EINVAL);
			return;
		}

		/* truncate to device size */
		bp->b_bcount = sz;
	}

	async = bp->b_flags & B_ASYNC;
	DPRINTF(("rumpblk_strategy: 0x%x bytes %s off 0x%" PRIx64
	    " (0x%" PRIx64 " - 0x%" PRIx64")\n",
	    bp->b_bcount, BUF_ISREAD(bp) "READ" : "WRITE",
	    off, off, (off + bp->b_bcount)));

	/* mem optimization?  handle here and return */
	if (rblk->rblk_mem) {
		uint8_t *ioaddr = rblk->rblk_mem + off;

		if (BUF_ISREAD(bp)) {
			memcpy(bp->b_data, ioaddr, bp->b_bcount);
		} else {
			memcpy(ioaddr, bp->b_data, bp->b_bcount);
		}

		/* synchronous write, sync necessary bits back to disk */
		if (BUF_ISWRITE(bp) && !async) {
			rumpuser_memsync(ioaddr, bp->b_bcount, &error);
		}
		rump_biodone(bp, bp->b_bcount, 0);

		return;
	}

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
	 * Using bufq here might be a good idea.
	 */
	if (rump_threads) {
		struct rumpuser_aio *rua;

		rumpuser_mutex_enter(&rumpuser_aio_mtx);
		while ((rumpuser_aio_head+1) % N_AIOS == rumpuser_aio_tail)
			rumpuser_cv_wait(&rumpuser_aio_cv, &rumpuser_aio_mtx);

		rua = &rumpuser_aios[rumpuser_aio_head];
		KASSERT(rua->rua_bp == NULL);
		rua->rua_fd = rblk->rblk_fd;
		rua->rua_data = bp->b_data;
		rua->rua_dlen = bp->b_bcount;
		rua->rua_off = off;
		rua->rua_bp = bp;
		rua->rua_op = BUF_ISREAD(bp);

		/* insert into queue & signal */
		rumpuser_aio_head = (rumpuser_aio_head+1) % N_AIOS;
		rumpuser_cv_signal(&rumpuser_aio_cv);
		rumpuser_mutex_exit(&rumpuser_aio_mtx);

		/* make sure non-async writes end up on backing media */
		if (BUF_ISWRITE(bp) && !async) {
			biowait(bp);
			rumpuser_fsync(rblk->rblk_fd, &error);
		}
	} else {
		if (BUF_ISREAD(bp)) {
			rumpuser_read_bio(rblk->rblk_fd, bp->b_data,
			    bp->b_bcount, off, rump_biodone, bp);
		} else {
			rumpuser_write_bio(rblk->rblk_fd, bp->b_data,
			    bp->b_bcount, off, rump_biodone, bp);
		}
		if (!async) {
			if (BUF_ISWRITE(bp))
				rumpuser_fsync(rblk->rblk_fd, &error);
		}
	}
}

void
rumpblk_strategy(struct buf *bp)
{

	dostrategy(bp);
}

/*
 * Simple random number generator.  This is private so that we can
 * very repeatedly control which blocks will fail.
 *
 * <mlelstv> pooka, rand()
 * <mlelstv> [paste]
 */
static unsigned
gimmerand(void)
{

	return (randstate = randstate * 1103515245 + 12345) % (0x80000000L);
}

/*
 * Block device with very simple fault injection.  Fails every
 * n out of BLKFAIL_MAX I/O with EIO.  n is determined by the env
 * variable RUMP_BLKFAIL.
 */
void
rumpblk_strategy_fail(struct buf *bp)
{

	if (gimmerand() % BLKFAIL_MAX >= blkfail) {
		dostrategy(bp);
	} else { 
		printf("block fault injection: failing I/O on block %lld\n",
		    (long long)bp->b_blkno);
		bp->b_error = EIO;
		biodone(bp);
	}
}
