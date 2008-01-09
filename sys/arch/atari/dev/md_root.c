/*	NetBSD: md_root.c,v 1.17 2002/05/23 14:59:28 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
 * All rights reserved.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: md_root.c,v 1.23.6.2 2008/01/09 01:45:31 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkbad.h>

#include <dev/cons.h>
#include <dev/md.h>

#include <atari/atari/device.h>

/*
 * Misc. defines:
 */
#define	RAMD_CHUNK	(9 * 512)	/* Chunk-size for auto-load	*/
#define	RAMD_NDEV	3		/* Number of devices configured	*/

struct   ramd_info {
	u_long	ramd_size;  /* Size of disk in bytes			*/
	u_long	ramd_flag;  /* see defs below				*/
	dev_t	ramd_dev;   /* device to load from			*/
};

/*
 * ramd_flag:
 */
#define	RAMD_LOAD	0x01	/* Auto load when first opened	*/
#define	RAMD_LCOMP	0x02	/* Input is compressed		*/

struct ramd_info rd_info[RAMD_NDEV] = {
    {
	1105920,		/*	1Mb in 2160 sectors		*/
	RAMD_LOAD,		/* auto-load this device		*/
	MAKEDISKDEV(2, 0, 2),	/* XXX: This is crap! (720Kb flop)	*/
    },
    {
	1474560,		/* 1.44Mb in 2880 sectors		*/
	RAMD_LOAD,		/* auto-load this device		*/
	MAKEDISKDEV(2, 0, 2),	/* XXX: This is crap! (720Kb flop)	*/
    },
    {
	1474560,		/* 1.44Mb in 2880 sectors		*/
	RAMD_LOAD,		/* auto-load this device		*/
	MAKEDISKDEV(2, 0, 3),	/* XXX: This is crap! (1.44Mb flop)	*/
    }
};

struct read_info {
    struct buf	*bp;		/* buffer for strategy function		*/
    long	nbytes;		/* total number of bytes to read	*/
    long	offset;		/* offset in input medium		*/
    char	*bufp;		/* current output buffer		*/
    char	*ebufp;		/* absolute maximum for bufp		*/
    int		chunk;		/* chunk size on input medium		*/
    int		media_sz;	/* size of input medium			*/
    void	(*strat)(struct buf *);	/* strategy function for read	*/
};


static int  loaddisk __P((struct  md_conf *, dev_t ld_dev, struct lwp *));
static int  ramd_norm_read __P((struct read_info *));

#ifdef support_compression
static int  cpy_uncompressed __P((void *, int, struct read_info *));
static int  md_compressed __P((void *, int, struct read_info *));
#endif

/*
 * This is called during autoconfig.
 */
void
md_attach_hook(unit, md)
int		unit;
struct md_conf	*md;
{
	if (atari_realconfig && (unit < RAMD_NDEV) && rd_info[unit].ramd_flag) {
		printf ("md%d: %sauto-load on open. Size %ld bytes.\n", unit,
		    rd_info[unit].ramd_flag & RAMD_LCOMP ? "decompress/" : "",
		    rd_info[unit].ramd_size);
		md->md_type = MD_UNCONFIGURED; /* Paranoia... */
	}
}

void
md_open_hook(unit, md)
int		unit;
struct md_conf	*md;
{
	struct ramd_info *ri;

	if(unit >= RAMD_NDEV)
		return;

	ri = &rd_info[unit];
	if (md->md_type != MD_UNCONFIGURED)
		return;	/* Only configure once */
	md->md_addr = malloc(ri->ramd_size, M_DEVBUF, M_WAITOK);
	md->md_size = ri->ramd_size;
	if(md->md_addr == NULL)
		return;
	if(ri->ramd_flag & RAMD_LOAD) {
		if (loaddisk(md, ri->ramd_dev, curlwp)) {
			free(md->md_addr, M_DEVBUF);
			md->md_addr = NULL;
			return;
		}
	}
	md->md_type = MD_KMEM_ALLOCATED;
}

static int
loaddisk(md, ld_dev, lwp)
struct md_conf		*md;
dev_t			ld_dev;
struct lwp		*lwp;
{
	struct buf		*buf;
	int			error;
	const struct bdevsw	*bdp;
	struct disklabel	dl;
	struct read_info	rs;

	bdp = bdevsw_lookup(ld_dev);
	if (bdp == NULL)
		return (ENXIO);

	/*
	 * Initialize our buffer header:
	 */
	buf = getiobuf(NULL, false);
	buf->b_cflags = BC_BUSY;
	buf->b_dev   = ld_dev;
	buf->b_error = 0;
	buf->b_proc  = lwp->l_proc;

	/*
	 * Setup read_info:
	 */
	rs.bp       = buf;
	rs.nbytes   = md->md_size;
	rs.offset   = 0;
	rs.bufp     = md->md_addr;
	rs.ebufp    = (char *)md->md_addr + md->md_size;
	rs.chunk    = RAMD_CHUNK;
	rs.media_sz = md->md_size;
	rs.strat    = bdp->d_strategy;

	/*
	 * Open device and try to get some statistics.
	 */
	if((error = bdp->d_open(ld_dev, FREAD | FNONBLOCK, 0, lwp)) != 0) {
		putiobuf(buf);
		return(error);
	}
	if(bdp->d_ioctl(ld_dev, DIOCGDINFO, (void *)&dl, FREAD, lwp) == 0) {
		/* Read on a cylinder basis */
		rs.chunk    = dl.d_secsize * dl.d_secpercyl;
		rs.media_sz = dl.d_secperunit * dl.d_secsize;
	}

#ifdef support_compression
	if(ri->ramd_flag & RAMD_LCOMP)
		error = decompress(cpy_uncompressed, md_compressed, &rs);
	else
#endif /* support_compression */
		error = ramd_norm_read(&rs);

	bdp->d_close(ld_dev,FREAD | FNONBLOCK, 0, lwp);
	putiobuf(buf);
	return(error);
}

static int
ramd_norm_read(rsp)
struct read_info	*rsp;
{
	long		bytes_left;
	int		done, error;
	struct buf	*bp;
	int		dotc = 0;

	bytes_left = rsp->nbytes;
	bp         = rsp->bp;
	error      = 0;

	while(bytes_left > 0) {
		bp->b_cflags = BC_BUSY;
		bp->b_flags  = B_PHYS | B_READ;
		bp->b_blkno  = btodb(rsp->offset);
		bp->b_bcount = rsp->chunk;
		bp->b_data   = rsp->bufp;
		bp->b_error  = 0;

		/* Initiate read */
		(*rsp->strat)(bp);

		/* Wait for results	*/
		biowait(bp);
		error = bp->b_error;

		/* Dot counter */
		printf(".");
		if(!(++dotc % 40))
			printf("\n");

		done = bp->b_bcount - bp->b_resid;
		bytes_left   -= done;
		rsp->offset  += done;
		rsp->bufp    += done;

		if(error || !done)
			break;

		if((rsp->offset == rsp->media_sz) && (bytes_left != 0)) {
			printf("\nInsert next media and hit any key...");
			cngetc();
			printf("\n");
			rsp->offset = 0;
		}
	}
	printf("\n");
	return(error);
}

#ifdef support_compression
/*
 * Functions supporting uncompression:
 */
/*
 * Copy from the uncompression buffer to the ramdisk
 */
static int
cpy_uncompressed(buf, nbyte, rsp)
void *			buf;
struct read_info	*rsp;
int			nbyte;
{
	if((rsp->bufp + nbyte) >= rsp->ebufp)
		return(0);
	bcopy(buf, rsp->bufp, nbyte);
	rsp->bufp += nbyte;
	return(0);
}

/*
 * Read a maximum of 'nbyte' bytes into 'buf'.
 */
static int
md_compressed(buf, nbyte, rsp)
void *			buf;
struct read_info	*rsp;
int			nbyte;
{
	static int	dotc = 0;
	struct buf	*bp;
	       int	nread = 0;
	       int	done, error;


	error  = 0;
	bp     = rsp->bp;
	nbyte &= ~(DEV_BSIZE - 1);

	while(nbyte > 0) {
		bp->b_cflags = BC_BUSY;
		bp->b_flags  = B_PHYS | B_READ;
		bp->b_blkno  = btodb(rsp->offset);
		bp->b_bcount = min(rsp->chunk, nbyte);
		bp->b_data   = buf;
		bp->b_error  = 0;

		/* Initiate read */
		(*rsp->strat)(bp);

		/* Wait for results	*/
		biowait(bp);

		/* Dot counter */
		printf(".");
		if(!(++dotc % 40))
			printf("\n");

		done = bp->b_bcount - bp->b_resid;
		nbyte        -= done;
		nread        += done;
		rsp->offset  += done;

		if(error || !done)
			break;

		if((rsp->offset == rsp->media_sz) && (nbyte != 0)) {
		if(rsp->offset == rsp->media_sz) {
			printf("\nInsert next media and hit any key...");
			if(cngetc() != '\n')
				printf("\n");
			rsp->offset = 0;
		}
	}
	return(nread);
}
#endif /* support_compression */
