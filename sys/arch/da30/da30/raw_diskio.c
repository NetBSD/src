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
 *	$Id: raw_diskio.c,v 1.1 1994/02/22 23:49:54 paulus Exp $
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
    int err;

    if( uio->uio_offset % blocksize != 0 )
	return EINVAL;

    MALLOC(bp, struct buf *, sizeof(struct buf), M_TEMP, M_WAITOK);
    bzero(bp, sizeof(*bp));
    bp->b_flags = B_BUSY | B_PHYS;
    if( uio->uio_rw == UIO_READ )
	bp->b_flags |= B_READ;
    bp->b_dev = dev;
    bp->b_proc = uio->uio_procp;

    err = uioapply(rawio_piece, bp, blocksize, uio);

    FREE(bp, M_TEMP);
    return err;
}

int
rawio_piece(struct buf *bp, int blocksize, long offset,
	    int rw, caddr_t addr, u_long *lenp, struct proc *p)
{
    u_long len, done;
    int s;
    vm_offset_t va;

    len = *lenp;
    if( len % blocksize != 0 )
	return EINVAL;
    if( !useracc(addr, len, (rw == UIO_READ? B_WRITE: B_READ)) )
	return EFAULT;

    while( len != 0 ){
	bp->b_un.b_addr = addr;
	bp->b_flags &= ~B_DONE;
	bp->b_bcount = min(len, MAXPHYS);
	bp->b_blkno = offset / blocksize;

	for( va = trunc_page(addr); va < (vm_offset_t) addr + bp->b_bcount;
		va += PAGE_SIZE )
	    vm_fault(&p->p_vmspace->vm_map, va,
		     VM_PROT_READ | (rw == UIO_READ? VM_PROT_WRITE: 0), FALSE);

	vslock(addr, bp->b_bcount);
	vmapbuf(bp);

	cdevsw[major(bp->b_dev)].d_strategy(bp);
	s = splbio();
	while( (bp->b_flags & B_DONE) == 0 )
	    sleep((caddr_t)bp, PRIBIO);
	splx(s);

	vunmapbuf(bp);
	vsunlock(addr, bp->b_bcount, rw == UIO_READ);

	done = bp->b_bcount - bp->b_resid;
	len -= done;
	addr += done;
	offset += done;

	if( (bp->b_flags & B_ERROR) != 0 || bp->b_error != 0 || bp->b_resid != 0 )
	    break;
    }

    if( (bp->b_flags & B_ERROR) != 0 && bp->b_error == 0 )
	bp->b_error = EIO;
    *lenp = len;
    return bp->b_error;
}
