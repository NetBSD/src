/* $NetBSD: cgd.c,v 1.15 2004/03/18 10:42:08 dan Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgd.c,v 1.15 2004/03/18 10:42:08 dan Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/conf.h>

#include <dev/dkvar.h>
#include <dev/cgdvar.h>

/* Entry Point Functions */

void	cgdattach(int);

dev_type_open(cgdopen);
dev_type_close(cgdclose);
dev_type_read(cgdread);
dev_type_write(cgdwrite);
dev_type_ioctl(cgdioctl);
dev_type_strategy(cgdstrategy);
dev_type_dump(cgddump);
dev_type_size(cgdsize);

const struct bdevsw cgd_bdevsw = {
	cgdopen, cgdclose, cgdstrategy, cgdioctl,
	cgddump, cgdsize, D_DISK
};

const struct cdevsw cgd_cdevsw = {
	cgdopen, cgdclose, cgdread, cgdwrite, cgdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

/* Internal Functions */

static void	cgdstart(struct dk_softc *, struct buf *);
static void	cgdiodone(struct buf *);

static int	cgd_ioctl_set(struct cgd_softc *, void *, struct proc *);
static int	cgd_ioctl_clr(struct cgd_softc *, void *, struct proc *);
static int	cgdinit(struct cgd_softc *, char *, struct vnode *,
			struct proc *);
static void	cgd_cipher(struct cgd_softc *, caddr_t, caddr_t,
			   size_t, daddr_t, size_t, int);

/* Pseudo-disk Interface */

static struct dk_intf the_dkintf = {
	DTYPE_CGD,
	"cgd",
	cgdopen,
	cgdclose,
	cgdstrategy,
	cgdstart,
};
static struct dk_intf *di = &the_dkintf;

/* DIAGNOSTIC and DEBUG definitions */

#if defined(CGDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
int cgddebug = 0;

#define CGDB_FOLLOW	0x1
#define CGDB_IO	0x2
#define CGDB_CRYPTO	0x4

#define IFDEBUG(x,y)		if (cgddebug & (x)) y
#define DPRINTF(x,y)		IFDEBUG(x, printf y)
#define DPRINTF_FOLLOW(y)	DPRINTF(CGDB_FOLLOW, y)

static void	hexprint(char *, void *, int);

#else
#define IFDEBUG(x,y)
#define DPRINTF(x,y)
#define DPRINTF_FOLLOW(y)
#endif

#ifdef DIAGNOSTIC
#define DIAGPANIC(x)		panic x 
#define DIAGCONDPANIC(x,y)	if (x) panic y
#else
#define DIAGPANIC(x)
#define DIAGCONDPANIC(x,y)
#endif

/* Component Buffer Pool structures and macros */

struct cgdbuf {
	struct buf		 cb_buf;	/* new I/O buf */
	struct buf		*cb_obp;	/* ptr. to original I/O buf */
	struct cgd_softc	*cb_sc;		/* pointer to cgd softc */
};

struct pool cgd_cbufpool;

#define	CGD_GETBUF()		pool_get(&cgd_cbufpool, PR_NOWAIT)
#define	CGD_PUTBUF(cbp)		pool_put(&cgd_cbufpool, cbp)

/* Global variables */

struct	cgd_softc *cgd_softc;
int	numcgd = 0;

/* Utility Functions */

#define CGDUNIT(x)		DISKUNIT(x)
#define GETCGD_SOFTC(_cs, x)	if (!((_cs) = getcgd_softc(x))) return ENXIO

static struct cgd_softc *
getcgd_softc(dev_t dev)
{
	int	unit = CGDUNIT(dev);

	DPRINTF_FOLLOW(("getcgd_softc(0x%x): unit = %d\n", dev, unit));
	if (unit >= numcgd)
		return NULL;
	return &cgd_softc[unit];
}

/* The code */

static void
cgdsoftc_init(struct cgd_softc *cs, int num)
{
	char	buf[DK_XNAME_SIZE];

	memset(cs, 0x0, sizeof(*cs));
	snprintf(buf, DK_XNAME_SIZE, "cgd%d", num);
	dk_sc_init(&cs->sc_dksc, cs, buf);
}

void
cgdattach(int num)
{
	int	i;

	DPRINTF_FOLLOW(("cgdattach(%d)\n", num));
	if (num <= 0) {
		DIAGPANIC(("cgdattach: count <= 0"));
		return;
	}

	cgd_softc = (void *)malloc(num * sizeof(*cgd_softc), M_DEVBUF, M_NOWAIT);
	if (!cgd_softc) {
		printf("WARNING: unable to malloc(9) memory for crypt disks\n");
		DIAGPANIC(("cgdattach: cannot malloc(9) enough memory"));
		return;
	}

	numcgd = num;
	for (i=0; i<num; i++)
		cgdsoftc_init(&cgd_softc[i], i);

	/* Init component buffer pool. XXX, can we put this in dksubr.c? */
	pool_init(&cgd_cbufpool, sizeof(struct cgdbuf), 0, 0, 0,
	    "cgdpl", NULL);
}

int
cgdopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgdopen(%d, %d)\n", dev, flags));
	GETCGD_SOFTC(cs, dev);
	return dk_open(di, &cs->sc_dksc, dev, flags, fmt, p);
}

int
cgdclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgdclose(%d, %d)\n", dev, flags));
	GETCGD_SOFTC(cs, dev);
	return dk_close(di, &cs->sc_dksc, dev, flags, fmt, p);
}

void
cgdstrategy(struct buf *bp)
{
	struct	cgd_softc *cs = getcgd_softc(bp->b_dev);

	DPRINTF_FOLLOW(("cgdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));
	/* XXXrcd: Should we test for (cs != NULL)? */
	dk_strategy(di, &cs->sc_dksc, bp);
	return;
}

int
cgdsize(dev_t dev)
{
	struct cgd_softc *cs = getcgd_softc(dev);

	DPRINTF_FOLLOW(("cgdsize(%d)\n", dev));
	if (!cs)
		return -1;
	return dk_size(di, &cs->sc_dksc, dev);
}

static void
cgdstart(struct dk_softc *dksc, struct buf *bp)
{
	struct	cgd_softc *cs = dksc->sc_osc;
	struct	cgdbuf *cbp;
	struct	partition *pp;
	caddr_t	addr;
	caddr_t	newaddr;
	daddr_t	bn;

	DPRINTF_FOLLOW(("cgdstart(%p, %p)\n", dksc, bp));
	disk_busy(&dksc->sc_dkdev); /* XXX: put in dksubr.c */

	/* XXXrcd:
	 * Translate partition relative blocks to absolute blocks,
	 * this probably belongs (somehow) in dksubr.c, since it
	 * is independant of the underlying code...  This will require
	 * that the interface be expanded slightly, though.
	 */
	bn = bp->b_blkno;
	if (DISKPART(bp->b_dev) != RAW_PART) {
		pp = &cs->sc_dksc.sc_dkdev.dk_label->d_partitions[DISKPART(bp->b_dev)];
		bn += pp->p_offset;
	}

	/*
	 * If we are writing, then we need to encrypt the outgoing
	 * block.  In the best case scenario, we are able to allocate
	 * enough memory to encrypt the data in a new block, otherwise
	 * we encrypt it in place (noting we'll have to decrypt it after
	 * the write.)
	 */
	newaddr = addr = bp->b_data;
	if ((bp->b_flags & B_READ) == 0) {
		newaddr = malloc(bp->b_bcount, M_DEVBUF, 0);
		if (!newaddr)
			newaddr = addr;
		cgd_cipher(cs, newaddr, addr, bp->b_bcount, bn,
		    DEV_BSIZE, CGD_CIPHER_ENCRYPT);
	}

	cbp = CGD_GETBUF();
	if (cbp == NULL) {
		bp->b_error = ENOMEM;
		bp->b_flags |= B_ERROR;
		if (newaddr != addr)
			free(newaddr, M_DEVBUF);
		biodone(bp);
		disk_unbusy(&dksc->sc_dkdev, 0, (bp->b_flags & B_READ));
		return;
	}
	BUF_INIT(&cbp->cb_buf);
	cbp->cb_buf.b_data = newaddr;
	cbp->cb_buf.b_flags = bp->b_flags | B_CALL;
	cbp->cb_buf.b_iodone = cgdiodone;
	cbp->cb_buf.b_proc = bp->b_proc;
	cbp->cb_buf.b_blkno = bn;
	cbp->cb_buf.b_vp = cs->sc_tvn;
	cbp->cb_buf.b_bcount = bp->b_bcount;

	/* context for cgdiodone */
	cbp->cb_obp = bp;
	cbp->cb_sc = cs;

	BIO_COPYPRIO(&cbp->cb_buf, bp);

	if ((cbp->cb_buf.b_flags & B_READ) == 0)
		cbp->cb_buf.b_vp->v_numoutput++;
	VOP_STRATEGY(cs->sc_tvn, &cbp->cb_buf);
}

void
cgdiodone(struct buf *vbp)
{
	struct	cgdbuf *cbp = (struct cgdbuf *)vbp;
	struct	buf *obp = cbp->cb_obp;
	struct	buf *nbp = &cbp->cb_buf;
	struct	cgd_softc *cs = cbp->cb_sc;
	struct	dk_softc *dksc = &cs->sc_dksc;
	int	s;

	DPRINTF_FOLLOW(("cgdiodone(%p)\n", vbp));
	DPRINTF(CGDB_IO, ("cgdiodone: bp %p bcount %ld resid %ld\n",
	    obp, obp->b_bcount, obp->b_resid));
	DPRINTF(CGDB_IO, (" dev 0x%x, cbp %p bn %" PRId64 " addr %p bcnt %ld\n",
	    cbp->cb_buf.b_dev, cbp, cbp->cb_buf.b_blkno, cbp->cb_buf.b_data,
	    cbp->cb_buf.b_bcount));
	s = splbio();
	if (nbp->b_flags & B_ERROR) {
		obp->b_flags |= B_ERROR;
		obp->b_error  = nbp->b_error ? nbp->b_error : EIO;

		printf("%s: error %d\n", dksc->sc_xname, obp->b_error);
	}

	/* Perform the decryption if we need to:
	 *	o  if we are reading, or
	 *	o  we wrote and couldn't allocate memory.
	 *
	 * Note: use the blocknumber from nbp, since it is what
	 *       we used to encrypt the blocks.
	 */

	if (nbp->b_flags & B_READ || nbp->b_data == obp->b_data)
		cgd_cipher(cs, obp->b_data, obp->b_data, obp->b_bcount,
		    nbp->b_blkno, DEV_BSIZE, CGD_CIPHER_DECRYPT);

	/* If we managed to allocate memory, free it now... */
	if (nbp->b_data != obp->b_data)
		free(nbp->b_data, M_DEVBUF);

	CGD_PUTBUF(cbp);

	/* Request is complete for whatever reason */
	obp->b_resid = 0;
	if (obp->b_flags & B_ERROR)
		obp->b_resid = obp->b_bcount;
	disk_unbusy(&dksc->sc_dkdev, obp->b_bcount - obp->b_resid,
	    (obp->b_flags & B_READ));
	biodone(obp);
	splx(s);
}

/* XXX: we should probably put these into dksubr.c, mostly */
int
cgdread(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdread(%d, %p, %d)\n", dev, uio, flags));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	/* XXX see the comments about minphys in ccd.c */
	return physio(cgdstrategy, NULL, dev, B_READ, minphys, uio);
}

/* XXX: we should probably put these into dksubr.c, mostly */
int
cgdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdwrite(%d, %p, %d)\n", dev, uio, flags));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	/* XXX see the comments about minphys in ccd.c */
	return physio(cgdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

int
cgdioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;
	int	ret;
	int	part = DISKPART(dev);
	int	pmask = 1 << part;

	DPRINTF_FOLLOW(("cgdioctl(%d, %ld, %p, %d, %p)\n",
	    dev, cmd, data, flag, p));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	switch (cmd) {
	case CGDIOCSET:
	case CGDIOCCLR:
		if ((flag & FWRITE) == 0)
			return EBADF;
	}

	if ((ret = lockmgr(&dksc->sc_lock, LK_EXCLUSIVE, NULL)) != 0)
		return ret;

	switch (cmd) {
	case CGDIOCSET:
		if (dksc->sc_flags & DKF_INITED)
			ret = EBUSY;
		else
			ret = cgd_ioctl_set(cs, data, p);
		break;
	case CGDIOCCLR:
		if (!(dksc->sc_flags & DKF_INITED)) {
			ret = ENXIO;
			break;
		}
		if (DK_BUSY(&cs->sc_dksc, pmask)) {
			ret = EBUSY;
			break;
		}
		ret = cgd_ioctl_clr(cs, data, p);
		break;
	default:
		ret = dk_ioctl(di, dksc, dev, cmd, data, flag, p);
		break;
	}

	lockmgr(&dksc->sc_lock, LK_RELEASE, NULL);
	return ret;
}

int
cgddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgddump(%d, %" PRId64 ", %p, %lu)\n", dev, blkno, va,
	    (unsigned long)size));
	GETCGD_SOFTC(cs, dev);
	return dk_dump(di, &cs->sc_dksc, dev, blkno, va, size);
}

/*
 * XXXrcd:
 *  for now we hardcode the maximum key length.
 */
#define MAX_KEYSIZE	1024

/* ARGSUSED */
static int
cgd_ioctl_set(struct cgd_softc *cs, void *data, struct proc *p)
{
	struct	 cgd_ioctl *ci = data;
	struct	 vnode *vp;
	int	 ret;
	int	 keybytes;			/* key length in bytes */
	char	*cp;
	char	 inbuf[MAX_KEYSIZE];

	cp = ci->ci_disk;
	if ((ret = dk_lookup(cp, p, &vp)) != 0)
		return ret;

	if ((ret = cgdinit(cs, cp, vp, p)) != 0)
		goto bail;

	memset(inbuf, 0x0, sizeof(inbuf));
	ret = copyinstr(ci->ci_alg, inbuf, 256, NULL);
	if (ret)
		goto bail;
	cs->sc_cfuncs = cryptfuncs_find(inbuf);
	if (!cs->sc_cfuncs) {
		ret = EINVAL;
		goto bail;
	}

	/* right now we only support encblkno, so hard-code it */
	memset(inbuf, 0x0, sizeof(inbuf));
	ret = copyinstr(ci->ci_ivmethod, inbuf, sizeof(inbuf), NULL);
	if (ret)
		goto bail;
	if (strcmp("encblkno", inbuf)) {
		ret = EINVAL;
		goto bail;
	}

	keybytes = ci->ci_keylen / 8 + 1;
	if (keybytes > MAX_KEYSIZE) {
		ret = EINVAL;
		goto bail;
	}
	memset(inbuf, 0x0, sizeof(inbuf));
	ret = copyin(ci->ci_key, inbuf, keybytes);
	if (ret)
		goto bail;

	cs->sc_cdata.cf_blocksize = ci->ci_blocksize;
	cs->sc_cdata.cf_mode = CGD_CIPHER_CBC_ENCBLKNO;
	cs->sc_cdata.cf_priv = cs->sc_cfuncs->cf_init(ci->ci_keylen, inbuf,
	    &cs->sc_cdata.cf_blocksize);
	memset(inbuf, 0x0, sizeof(inbuf));
	if (!cs->sc_cdata.cf_priv) {
		printf("cgd: unable to initialize cipher\n");
		ret = EINVAL;		/* XXX is this the right error? */
		goto bail;
	}

	cs->sc_dksc.sc_flags |= DKF_INITED;

	/* Attach the disk. */
	disk_attach(&cs->sc_dksc.sc_dkdev);

	/* Try and read the disklabel. */
	dk_getdisklabel(di, &cs->sc_dksc, 0 /* XXX ? */);

	return 0;

bail:
	(void)vn_close(vp, FREAD|FWRITE, p->p_ucred, p);
	return ret;
}

/* ARGSUSED */
static int
cgd_ioctl_clr(struct cgd_softc *cs, void *data, struct proc *p)
{

	(void)vn_close(cs->sc_tvn, FREAD|FWRITE, p->p_ucred, p);
	cs->sc_cfuncs->cf_destroy(cs->sc_cdata.cf_priv);
	free(cs->sc_tpath, M_DEVBUF);
	cs->sc_dksc.sc_flags &= ~DKF_INITED;
	disk_detach(&cs->sc_dksc.sc_dkdev);

	return 0;
}

static int
cgdinit(struct cgd_softc *cs, char *cpath, struct vnode *vp,
	struct proc *p)
{
	struct	dk_geom *pdg;
	struct	partinfo dpart;
	struct	vattr va;
	size_t	size;
	int	maxsecsize = 0;
	int	ret;
	char	tmppath[MAXPATHLEN];

	cs->sc_dksc.sc_size = 0;
	cs->sc_tvn = vp;

	memset(tmppath, 0x0, sizeof(tmppath));
	ret = copyinstr(cpath, tmppath, MAXPATHLEN, &cs->sc_tpathlen);
	if (ret)
		goto bail;
	cs->sc_tpath = malloc(cs->sc_tpathlen, M_DEVBUF, M_WAITOK);
	memcpy(cs->sc_tpath, tmppath, cs->sc_tpathlen);

	if ((ret = VOP_GETATTR(vp, &va, p->p_ucred, p)) != 0) 
		goto bail;

	cs->sc_tdev = va.va_rdev;

	ret = VOP_IOCTL(vp, DIOCGPART, &dpart, FREAD, p->p_ucred, p);
	if (ret)
		goto bail;

	maxsecsize =
	    ((dpart.disklab->d_secsize > maxsecsize) ?
	    dpart.disklab->d_secsize : maxsecsize);
	size = dpart.part->p_size;

	if (!size) {
		ret = ENODEV;
		goto bail;
	}

	cs->sc_dksc.sc_size = size;

	/*
	 * XXX here we should probe the underlying device.  If we
	 *     are accessing a partition of type RAW_PART, then
	 *     we should populate our initial geometry with the
	 *     geometry that we discover from the device.
	 */
	pdg = &cs->sc_dksc.sc_geom;
	pdg->pdg_secsize = DEV_BSIZE;
	pdg->pdg_ntracks = 1;
	pdg->pdg_nsectors = 1024 * (1024 / pdg->pdg_secsize);
	pdg->pdg_ncylinders = cs->sc_dksc.sc_size / pdg->pdg_nsectors;

bail:
	if (ret && cs->sc_tpath)
		free(cs->sc_tpath, M_DEVBUF);
	return ret;
}

/*
 * Our generic cipher entry point.  This takes care of the
 * IV mode and passes off the work to the specific cipher.
 * We implement here the IV method ``encrypted block
 * number''.
 * 
 * For the encryption case, we accomplish this by setting
 * up a struct uio where the first iovec of the source is
 * the blocknumber and the first iovec of the dest is a
 * sink.  We then call the cipher with an IV of zero, and
 * the right thing happens.
 * 
 * For the decryption case, we use the same basic mechanism
 * for symmetry, but we encrypt the block number in the
 * first iovec.
 *
 * We mainly do this to avoid requiring the definition of
 * an ECB mode.
 *
 * XXXrcd: for now we rely on our own crypto framework defined
 *         in dev/cgd_crypto.c.  This will change when we
 *         get a generic kernel crypto framework.
 */

static void
blkno2blkno_buf(char *buf, daddr_t blkno)
{
	int	i;

	/* Set up the blkno in blkno_buf, here we do not care much
	 * about the final layout of the information as long as we
	 * can guarantee that each sector will have a different IV
	 * and that the endianness of the machine will not affect
	 * the representation that we have chosen.
	 *
	 * We choose this representation, because it does not rely
	 * on the size of buf (which is the blocksize of the cipher),
	 * but allows daddr_t to grow without breaking existing
	 * disks.
	 *
	 * Note that blkno2blkno_buf does not take a size as input,
	 * and hence must be called on a pre-zeroed buffer of length
	 * greater than or equal to sizeof(daddr_t).
	 */
	for (i=0; i < sizeof(daddr_t); i++) {
		*buf++ = blkno & 0xff;
		blkno >>= 8;
	}
}

static void
cgd_cipher(struct cgd_softc *cs, caddr_t dst, caddr_t src,
	   size_t len, daddr_t blkno, size_t secsize, int dir)
{
	cfunc_cipher	*cipher = cs->sc_cfuncs->cf_cipher;
	struct uio	dstuio;
	struct uio	srcuio;
	struct iovec	dstiov[2];
	struct iovec	srciov[2];
	int		blocksize = cs->sc_cdata.cf_blocksize;
	char		sink[blocksize];
	char		zero_iv[blocksize];
	char		blkno_buf[blocksize];

	DPRINTF_FOLLOW(("cgd_cipher() dir=%d\n", dir));

	DIAGCONDPANIC(len % blocksize != 0, 
	    ("cgd_cipher: len %% blocksize != 0"));

	/* ensure that sizeof(daddr_t) <= blocksize (for encblkno IVing) */
	DIAGCONDPANIC(sizeof(daddr_t) > blocksize,
	    ("cgd_cipher: sizeof(daddr_t) > blocksize"));

	memset(zero_iv, 0x0, sizeof(zero_iv));

	dstuio.uio_iov = dstiov;
	dstuio.uio_iovcnt = 2;

	srcuio.uio_iov = srciov;
	srcuio.uio_iovcnt = 2;

	dstiov[0].iov_base = sink;
	dstiov[0].iov_len  = blocksize;
	srciov[0].iov_base = blkno_buf;
	srciov[0].iov_len  = blocksize;
	dstiov[1].iov_len  = secsize;
	srciov[1].iov_len  = secsize;

	for (; len > 0; len -= secsize) {
		dstiov[1].iov_base = dst;
		srciov[1].iov_base = src;

		memset(blkno_buf, 0x0, sizeof(blkno_buf));
		blkno2blkno_buf(blkno_buf, blkno);
		if (dir == CGD_CIPHER_DECRYPT) {
			dstuio.uio_iovcnt = 1;
			srcuio.uio_iovcnt = 1;
			IFDEBUG(CGDB_CRYPTO, hexprint("step 0: blkno_buf",
			    blkno_buf, sizeof(blkno_buf)));
			cipher(cs->sc_cdata.cf_priv, &dstuio, &srcuio,
			    zero_iv, CGD_CIPHER_ENCRYPT);
			memcpy(blkno_buf, sink, blocksize);
			dstuio.uio_iovcnt = 2;
			srcuio.uio_iovcnt = 2;
		}

		IFDEBUG(CGDB_CRYPTO, hexprint("step 1: blkno_buf",
		    blkno_buf, sizeof(blkno_buf)));
		cipher(cs->sc_cdata.cf_priv, &dstuio, &srcuio, zero_iv, dir);
		IFDEBUG(CGDB_CRYPTO, hexprint("step 2: sink",
		    sink, sizeof(sink)));

		dst += secsize;
		src += secsize;
		blkno++;
	}
}

#ifdef DEBUG
static void
hexprint(char *start, void *buf, int len)
{
	char	*c = buf;

	DIAGCONDPANIC(len < 0, ("hexprint: called with len < 0"));
	printf("%s: len=%06d 0x", start, len);
	while (len--)
		printf("%02x", (unsigned) *c++);
}
#endif
