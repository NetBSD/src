/*
 * Copyright (c) 1993 Paul Mackerras.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied.  The author makes no representations
 * about the suitability of this software for any purpose.
 *
 *	$Id: raw_diskio.c,v 1.3 1994/07/08 12:02:23 paulus Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <vm/vm.h>

int rawio_piece();

int
raw_disk_io(int dev, struct uio *uio, int blocksize)
{
    struct buf *bp;
    int err, ioc;
    struct iovec *iov;

    if( uio->uio_offset % blocksize != 0 )
	return EINVAL;

    MALLOC(bp, struct buf *, sizeof(struct buf), M_TEMP, M_WAITOK);
    bzero(bp, sizeof(*bp));
    bp->b_flags = B_BUSY | B_PHYS | B_RAW;
    if( uio->uio_rw == UIO_READ )
	bp->b_flags |= B_READ;
    bp->b_dev = dev;
    bp->b_proc = curproc;

    while (uio->uio_resid != 0) {
	iov = uio->uio_iov;
	if (iov->iov_len != 0) {
	    err = rawio_piece(bp, blocksize, uio, iov);
	    if (err != 0 || iov->iov_len != 0)
		break;
	}
	++uio->uio_iov;
	--uio->uio_iovcnt;
    }
			      
    FREE(bp, M_TEMP);
    return err;
}

int
rawio_piece(struct buf *bp, int blocksize,
	    struct uio *uio, struct iovec *iov)
{
    int s, todo, done;
    vm_offset_t va;

    if (iov->iov_len % blocksize != 0)
	return EINVAL;
    if (!useracc(iov->iov_base, iov->iov_len,
		 (uio->uio_rw == UIO_READ? B_WRITE: B_READ)))
	return EFAULT;

    while( iov->iov_len != 0 ){
	bp->b_data = iov->iov_base;
	bp->b_flags &= ~B_DONE;
	bp->b_bufsize = iov->iov_len;
	bp->b_bcount = todo = min(iov->iov_len, MAXPHYS);
	bp->b_blkno = uio->uio_offset / blocksize;

	++bp->b_proc->p_holdcnt;
	vslock(iov->iov_base, todo);
	vmapbuf(bp);

	cdevsw[major(bp->b_dev)].d_strategy(bp);
	s = splbio();
	while( (bp->b_flags & B_DONE) == 0 )
	    tsleep((caddr_t) bp, PRIBIO + 1, "physio", 0);
	splx(s);

	vunmapbuf(bp);
	vsunlock(iov->iov_base, todo, uio->uio_rw == UIO_READ);
	--bp->b_proc->p_holdcnt;

	done = todo - bp->b_resid;
	iov->iov_len -= done;
	iov->iov_base += done;
	uio->uio_offset += done;
	uio->uio_resid -= done;

	if ((bp->b_flags & B_ERROR) != 0 || bp->b_error != 0
	    || bp->b_resid != 0)
	    break;
    }

    if ((bp->b_flags & B_ERROR) != 0 && bp->b_error == 0)
	bp->b_error = EIO;
    return bp->b_error;
}
