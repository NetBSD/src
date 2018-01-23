/* $NetBSD: cgd.c,v 1.116 2018/01/23 22:42:29 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cgd.c,v 1.116 2018/01/23 22:42:29 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/pool.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/namei.h> /* for pathbuf */
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <dev/dkvar.h>
#include <dev/cgdvar.h>

#include <miscfs/specfs/specdev.h> /* for v_rdev */

#include "ioconf.h"

struct selftest_params {
	const char *alg;
	int blocksize;	/* number of bytes */
	int secsize;
	daddr_t blkno;
	int keylen;	/* number of bits */
	int txtlen;	/* number of bytes */
	const uint8_t *key;
	const uint8_t *ptxt;
	const uint8_t *ctxt;
};

/* Entry Point Functions */

static dev_type_open(cgdopen);
static dev_type_close(cgdclose);
static dev_type_read(cgdread);
static dev_type_write(cgdwrite);
static dev_type_ioctl(cgdioctl);
static dev_type_strategy(cgdstrategy);
static dev_type_dump(cgddump);
static dev_type_size(cgdsize);

const struct bdevsw cgd_bdevsw = {
	.d_open = cgdopen,
	.d_close = cgdclose,
	.d_strategy = cgdstrategy,
	.d_ioctl = cgdioctl,
	.d_dump = cgddump,
	.d_psize = cgdsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw cgd_cdevsw = {
	.d_open = cgdopen,
	.d_close = cgdclose,
	.d_read = cgdread,
	.d_write = cgdwrite,
	.d_ioctl = cgdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

/*
 * Vector 5 from IEEE 1619/D16 truncated to 64 bytes, blkno 1.
 */
static const uint8_t selftest_aes_xts_256_ptxt[64] = {
	0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
	0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
	0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
	0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
	0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
	0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
	0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
	0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12,
};

static const uint8_t selftest_aes_xts_256_ctxt[512] = {
	0x26, 0x4d, 0x3c, 0xa8, 0x51, 0x21, 0x94, 0xfe,
	0xc3, 0x12, 0xc8, 0xc9, 0x89, 0x1f, 0x27, 0x9f,
	0xef, 0xdd, 0x60, 0x8d, 0x0c, 0x02, 0x7b, 0x60,
	0x48, 0x3a, 0x3f, 0xa8, 0x11, 0xd6, 0x5e, 0xe5,
	0x9d, 0x52, 0xd9, 0xe4, 0x0e, 0xc5, 0x67, 0x2d,
	0x81, 0x53, 0x2b, 0x38, 0xb6, 0xb0, 0x89, 0xce,
	0x95, 0x1f, 0x0f, 0x9c, 0x35, 0x59, 0x0b, 0x8b,
	0x97, 0x8d, 0x17, 0x52, 0x13, 0xf3, 0x29, 0xbb,
};

static const uint8_t selftest_aes_xts_256_key[33] = {
	0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
	0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26,
	0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
	0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95,
	0
};

/*
 * Vector 11 from IEEE 1619/D16 truncated to 64 bytes, blkno 0xffff.
 */
static const uint8_t selftest_aes_xts_512_ptxt[64] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

static const uint8_t selftest_aes_xts_512_ctxt[64] = {
	0x77, 0xa3, 0x12, 0x51, 0x61, 0x8a, 0x15, 0xe6,
	0xb9, 0x2d, 0x1d, 0x66, 0xdf, 0xfe, 0x7b, 0x50,
	0xb5, 0x0b, 0xad, 0x55, 0x23, 0x05, 0xba, 0x02,
	0x17, 0xa6, 0x10, 0x68, 0x8e, 0xff, 0x7e, 0x11,
	0xe1, 0xd0, 0x22, 0x54, 0x38, 0xe0, 0x93, 0x24,
	0x2d, 0x6d, 0xb2, 0x74, 0xfd, 0xe8, 0x01, 0xd4,
	0xca, 0xe0, 0x6f, 0x20, 0x92, 0xc7, 0x28, 0xb2,
	0x47, 0x85, 0x59, 0xdf, 0x58, 0xe8, 0x37, 0xc2,
};

static const uint8_t selftest_aes_xts_512_key[65] = {
	0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
	0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26,
	0x62, 0x49, 0x77, 0x57, 0x24, 0x70, 0x93, 0x69,
	0x99, 0x59, 0x57, 0x49, 0x66, 0x96, 0x76, 0x27,
	0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
	0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95,
	0x02, 0x88, 0x41, 0x97, 0x16, 0x93, 0x99, 0x37,
	0x51, 0x05, 0x82, 0x09, 0x74, 0x94, 0x45, 0x92,
	0
};

const struct selftest_params selftests[] = {
	{
		.alg = "aes-xts",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 1,
		.keylen = 256,
		.txtlen = sizeof(selftest_aes_xts_256_ptxt),
		.key  = selftest_aes_xts_256_key,
		.ptxt = selftest_aes_xts_256_ptxt,
		.ctxt = selftest_aes_xts_256_ctxt
	},
	{
		.alg = "aes-xts",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 0xffff,
		.keylen = 512,
		.txtlen = sizeof(selftest_aes_xts_512_ptxt),
		.key  = selftest_aes_xts_512_key,
		.ptxt = selftest_aes_xts_512_ptxt,
		.ctxt = selftest_aes_xts_512_ctxt
	}
};

static int cgd_match(device_t, cfdata_t, void *);
static void cgd_attach(device_t, device_t, void *);
static int cgd_detach(device_t, int);
static struct cgd_softc	*cgd_spawn(int);
static int cgd_destroy(device_t);

/* Internal Functions */

static int	cgd_diskstart(device_t, struct buf *);
static void	cgdiodone(struct buf *);
static int	cgd_dumpblocks(device_t, void *, daddr_t, int);

static int	cgd_ioctl_set(struct cgd_softc *, void *, struct lwp *);
static int	cgd_ioctl_clr(struct cgd_softc *, struct lwp *);
static int	cgd_ioctl_get(dev_t, void *, struct lwp *);
static int	cgdinit(struct cgd_softc *, const char *, struct vnode *,
			struct lwp *);
static void	cgd_cipher(struct cgd_softc *, void *, void *,
			   size_t, daddr_t, size_t, int);

static struct dkdriver cgddkdriver = {
        .d_minphys  = minphys,
        .d_open = cgdopen,
        .d_close = cgdclose,
        .d_strategy = cgdstrategy,
        .d_iosize = NULL,
        .d_diskstart = cgd_diskstart,
        .d_dumpblocks = cgd_dumpblocks,
        .d_lastclose = NULL
};

CFATTACH_DECL3_NEW(cgd, sizeof(struct cgd_softc),
    cgd_match, cgd_attach, cgd_detach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

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

static void	hexprint(const char *, void *, int);

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

/* Global variables */

/* Utility Functions */

#define CGDUNIT(x)		DISKUNIT(x)
#define GETCGD_SOFTC(_cs, x)	if (!((_cs) = getcgd_softc(x))) return ENXIO

/* The code */

static struct cgd_softc *
getcgd_softc(dev_t dev)
{
	int	unit = CGDUNIT(dev);
	struct cgd_softc *sc;

	DPRINTF_FOLLOW(("getcgd_softc(0x%"PRIx64"): unit = %d\n", dev, unit));

	sc = device_lookup_private(&cgd_cd, unit);
	if (sc == NULL)
		sc = cgd_spawn(unit);
	return sc;
}

static int
cgd_match(device_t self, cfdata_t cfdata, void *aux)
{

	return 1;
}

static void
cgd_attach(device_t parent, device_t self, void *aux)
{
	struct cgd_softc *sc = device_private(self);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_BIO);
	dk_init(&sc->sc_dksc, self, DKTYPE_CGD);
	disk_init(&sc->sc_dksc.sc_dkdev, sc->sc_dksc.sc_xname, &cgddkdriver);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self,
		    "unable to register power management hooks\n");
}


static int
cgd_detach(device_t self, int flags)
{
	int ret;
	const int pmask = 1 << RAW_PART;
	struct cgd_softc *sc = device_private(self);
	struct dk_softc *dksc = &sc->sc_dksc;

	if (DK_BUSY(dksc, pmask))
		return EBUSY;

	if (DK_ATTACHED(dksc) &&
	    (ret = cgd_ioctl_clr(sc, curlwp)) != 0)
		return ret;

	disk_destroy(&dksc->sc_dkdev);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

void
cgdattach(int num)
{
	int error;

	error = config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
	if (error != 0)
		aprint_error("%s: unable to register cfattach\n",
		    cgd_cd.cd_name);
}

static struct cgd_softc *
cgd_spawn(int unit)
{
	cfdata_t cf;

	cf = malloc(sizeof(*cf), M_DEVBUF, M_WAITOK);
	cf->cf_name = cgd_cd.cd_name;
	cf->cf_atname = cgd_cd.cd_name;
	cf->cf_unit = unit;
	cf->cf_fstate = FSTATE_STAR;

	return device_private(config_attach_pseudo(cf));
}

static int
cgd_destroy(device_t dev)
{
	int error;
	cfdata_t cf;

	cf = device_cfdata(dev);
	error = config_detach(dev, DETACH_QUIET);
	if (error)
		return error;
	free(cf, M_DEVBUF);
	return 0;
}

static int
cgdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgdopen(0x%"PRIx64", %d)\n", dev, flags));
	GETCGD_SOFTC(cs, dev);
	return dk_open(&cs->sc_dksc, dev, flags, fmt, l);
}

static int
cgdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	int error;
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdclose(0x%"PRIx64", %d)\n", dev, flags));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	if ((error =  dk_close(dksc, dev, flags, fmt, l)) != 0)
		return error;

	if (!DK_ATTACHED(dksc)) {
		if ((error = cgd_destroy(cs->sc_dksc.sc_dev)) != 0) {
			aprint_error_dev(dksc->sc_dev,
			    "unable to detach instance\n");
			return error;
		}
	}
	return 0;
}

static void
cgdstrategy(struct buf *bp)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));

	cs = getcgd_softc(bp->b_dev);
	if (!cs) {
		bp->b_error = ENXIO;
		goto bail;
	}

	/*
	 * Reject unaligned writes.
	 */
	if (((uintptr_t)bp->b_data & 3) != 0) {
		bp->b_error = EINVAL;
		goto bail;
	}

	dk_strategy(&cs->sc_dksc, bp);
	return;

bail:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
}

static int
cgdsize(dev_t dev)
{
	struct cgd_softc *cs = getcgd_softc(dev);

	DPRINTF_FOLLOW(("cgdsize(0x%"PRIx64")\n", dev));
	if (!cs)
		return -1;
	return dk_size(&cs->sc_dksc, dev);
}

/*
 * cgd_{get,put}data are functions that deal with getting a buffer
 * for the new encrypted data.  We have a buffer per device so that
 * we can ensure that we can always have a transaction in flight.
 * We use this buffer first so that we have one less piece of
 * malloc'ed data at any given point.
 */

static void *
cgd_getdata(struct dk_softc *dksc, unsigned long size)
{
	struct	cgd_softc *cs = (struct cgd_softc *)dksc;
	void *	data = NULL;

	mutex_enter(&cs->sc_lock);
	if (cs->sc_data_used == 0) {
		cs->sc_data_used = 1;
		data = cs->sc_data;
	}
	mutex_exit(&cs->sc_lock);

	if (data)
		return data;

	return malloc(size, M_DEVBUF, M_NOWAIT);
}

static void
cgd_putdata(struct dk_softc *dksc, void *data)
{
	struct	cgd_softc *cs = (struct cgd_softc *)dksc;

	if (data == cs->sc_data) {
		mutex_enter(&cs->sc_lock);
		cs->sc_data_used = 0;
		mutex_exit(&cs->sc_lock);
	} else {
		free(data, M_DEVBUF);
	}
}

static int
cgd_diskstart(device_t dev, struct buf *bp)
{
	struct	cgd_softc *cs = device_private(dev);
	struct	dk_softc *dksc = &cs->sc_dksc;
	struct	disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	struct	buf *nbp;
	void *	addr;
	void *	newaddr;
	daddr_t	bn;
	struct	vnode *vp;

	DPRINTF_FOLLOW(("cgd_diskstart(%p, %p)\n", dksc, bp));

	bn = bp->b_rawblkno;

	/*
	 * We attempt to allocate all of our resources up front, so that
	 * we can fail quickly if they are unavailable.
	 */
	nbp = getiobuf(cs->sc_tvn, false);
	if (nbp == NULL)
		return EAGAIN;

	/*
	 * If we are writing, then we need to encrypt the outgoing
	 * block into a new block of memory.
	 */
	newaddr = addr = bp->b_data;
	if ((bp->b_flags & B_READ) == 0) {
		newaddr = cgd_getdata(dksc, bp->b_bcount);
		if (!newaddr) {
			putiobuf(nbp);
			return EAGAIN;
		}
		cgd_cipher(cs, newaddr, addr, bp->b_bcount, bn,
		    dg->dg_secsize, CGD_CIPHER_ENCRYPT);
	}

	nbp->b_data = newaddr;
	nbp->b_flags = bp->b_flags;
	nbp->b_oflags = bp->b_oflags;
	nbp->b_cflags = bp->b_cflags;
	nbp->b_iodone = cgdiodone;
	nbp->b_proc = bp->b_proc;
	nbp->b_blkno = btodb(bn * dg->dg_secsize);
	nbp->b_bcount = bp->b_bcount;
	nbp->b_private = bp;

	BIO_COPYPRIO(nbp, bp);

	if ((nbp->b_flags & B_READ) == 0) {
		vp = nbp->b_vp;
		mutex_enter(vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(vp->v_interlock);
	}
	VOP_STRATEGY(cs->sc_tvn, nbp);

	return 0;
}

static void
cgdiodone(struct buf *nbp)
{
	struct	buf *obp = nbp->b_private;
	struct	cgd_softc *cs = getcgd_softc(obp->b_dev);
	struct	dk_softc *dksc = &cs->sc_dksc;
	struct	disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	daddr_t	bn;

	KDASSERT(cs);

	DPRINTF_FOLLOW(("cgdiodone(%p)\n", nbp));
	DPRINTF(CGDB_IO, ("cgdiodone: bp %p bcount %d resid %d\n",
	    obp, obp->b_bcount, obp->b_resid));
	DPRINTF(CGDB_IO, (" dev 0x%"PRIx64", nbp %p bn %" PRId64
	    " addr %p bcnt %d\n", nbp->b_dev, nbp, nbp->b_blkno, nbp->b_data,
		nbp->b_bcount));
	if (nbp->b_error != 0) {
		obp->b_error = nbp->b_error;
		DPRINTF(CGDB_IO, ("%s: error %d\n", dksc->sc_xname,
		    obp->b_error));
	}

	/* Perform the decryption if we are reading.
	 *
	 * Note: use the blocknumber from nbp, since it is what
	 *       we used to encrypt the blocks.
	 */

	if (nbp->b_flags & B_READ) {
		bn = dbtob(nbp->b_blkno) / dg->dg_secsize;
		cgd_cipher(cs, obp->b_data, obp->b_data, obp->b_bcount,
		    bn, dg->dg_secsize, CGD_CIPHER_DECRYPT);
	}

	/* If we allocated memory, free it now... */
	if (nbp->b_data != obp->b_data)
		cgd_putdata(dksc, nbp->b_data);

	putiobuf(nbp);

	/* Request is complete for whatever reason */
	obp->b_resid = 0;
	if (obp->b_error != 0)
		obp->b_resid = obp->b_bcount;

	dk_done(dksc, obp);
	dk_start(dksc, NULL);
}

static int
cgd_dumpblocks(device_t dev, void *va, daddr_t blkno, int nblk)
{
	struct cgd_softc *sc = device_private(dev);
	struct dk_softc *dksc = &sc->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	size_t nbytes, blksize;
	void *buf;
	int error;

	/*
	 * dk_dump gives us units of disklabel sectors.  Everything
	 * else in cgd uses units of diskgeom sectors.  These had
	 * better agree; otherwise we need to figure out how to convert
	 * between them.
	 */
	KASSERTMSG((dg->dg_secsize == dksc->sc_dkdev.dk_label->d_secsize),
	    "diskgeom secsize %"PRIu32" != disklabel secsize %"PRIu32,
	    dg->dg_secsize, dksc->sc_dkdev.dk_label->d_secsize);
	blksize = dg->dg_secsize;

	/*
	 * Compute the number of bytes in this request, which dk_dump
	 * has `helpfully' converted to a number of blocks for us.
	 */
	nbytes = nblk*blksize;

	/* Try to acquire a buffer to store the ciphertext.  */
	buf = cgd_getdata(dksc, nbytes);
	if (buf == NULL)
		/* Out of memory: give up.  */
		return ENOMEM;

	/* Encrypt the caller's data into the temporary buffer.  */
	cgd_cipher(sc, buf, va, nbytes, blkno, blksize, CGD_CIPHER_ENCRYPT);

	/* Pass it on to the underlying disk device.  */
	error = bdev_dump(sc->sc_tdev, blkno, buf, nbytes);

	/* Release the buffer.  */
	cgd_putdata(dksc, buf);

	/* Return any error from the underlying disk device.  */
	return error;
}

/* XXX: we should probably put these into dksubr.c, mostly */
static int
cgdread(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdread(0x%llx, %p, %d)\n",
	    (unsigned long long)dev, uio, flags));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	if (!DK_ATTACHED(dksc))
		return ENXIO;
	return physio(cgdstrategy, NULL, dev, B_READ, minphys, uio);
}

/* XXX: we should probably put these into dksubr.c, mostly */
static int
cgdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdwrite(0x%"PRIx64", %p, %d)\n", dev, uio, flags));
	GETCGD_SOFTC(cs, dev);
	dksc = &cs->sc_dksc;
	if (!DK_ATTACHED(dksc))
		return ENXIO;
	return physio(cgdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
cgdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct	cgd_softc *cs;
	struct	dk_softc *dksc;
	int	part = DISKPART(dev);
	int	pmask = 1 << part;

	DPRINTF_FOLLOW(("cgdioctl(0x%"PRIx64", %ld, %p, %d, %p)\n",
	    dev, cmd, data, flag, l));

	switch (cmd) {
	case CGDIOCGET:
		return cgd_ioctl_get(dev, data, l);
	case CGDIOCSET:
	case CGDIOCCLR:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* FALLTHROUGH */
	default:
		GETCGD_SOFTC(cs, dev);
		dksc = &cs->sc_dksc;
		break;
	}

	switch (cmd) {
	case CGDIOCSET:
		if (DK_ATTACHED(dksc))
			return EBUSY;
		return cgd_ioctl_set(cs, data, l);
	case CGDIOCCLR:
		if (DK_BUSY(&cs->sc_dksc, pmask))
			return EBUSY;
		return cgd_ioctl_clr(cs, l);
	case DIOCGCACHE:
	case DIOCCACHESYNC:
		if (!DK_ATTACHED(dksc))
			return ENOENT;
		/*
		 * We pass this call down to the underlying disk.
		 */
		return VOP_IOCTL(cs->sc_tvn, cmd, data, flag, l->l_cred);
	case DIOCGSTRATEGY:
	case DIOCSSTRATEGY:
		if (!DK_ATTACHED(dksc))
			return ENOENT;
		/*FALLTHROUGH*/
	default:
		return dk_ioctl(dksc, dev, cmd, data, flag, l);
	case CGDIOCGET:
		KASSERT(0);
		return EINVAL;
	}
}

static int
cgddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct	cgd_softc *cs;

	DPRINTF_FOLLOW(("cgddump(0x%"PRIx64", %" PRId64 ", %p, %lu)\n",
	    dev, blkno, va, (unsigned long)size));
	GETCGD_SOFTC(cs, dev);
	return dk_dump(&cs->sc_dksc, dev, blkno, va, size);
}

/*
 * XXXrcd:
 *  for now we hardcode the maximum key length.
 */
#define MAX_KEYSIZE	1024

static const struct {
	const char *n;
	int v;
	int d;
} encblkno[] = {
	{ "encblkno",  CGD_CIPHER_CBC_ENCBLKNO8, 1 },
	{ "encblkno8", CGD_CIPHER_CBC_ENCBLKNO8, 1 },
	{ "encblkno1", CGD_CIPHER_CBC_ENCBLKNO1, 8 },
};

/* ARGSUSED */
static int
cgd_ioctl_set(struct cgd_softc *cs, void *data, struct lwp *l)
{
	struct	 cgd_ioctl *ci = data;
	struct	 vnode *vp;
	int	 ret;
	size_t	 i;
	size_t	 keybytes;			/* key length in bytes */
	const char *cp;
	struct pathbuf *pb;
	char	 *inbuf;
	struct dk_softc *dksc = &cs->sc_dksc;

	cp = ci->ci_disk;

	ret = pathbuf_copyin(ci->ci_disk, &pb);
	if (ret != 0) {
		return ret;
	}
	ret = dk_lookup(pb, l, &vp);
	pathbuf_destroy(pb);
	if (ret != 0) {
		return ret;
	}

	inbuf = malloc(MAX_KEYSIZE, M_TEMP, M_WAITOK);

	if ((ret = cgdinit(cs, cp, vp, l)) != 0)
		goto bail;

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyinstr(ci->ci_alg, inbuf, 256, NULL);
	if (ret)
		goto bail;
	cs->sc_cfuncs = cryptfuncs_find(inbuf);
	if (!cs->sc_cfuncs) {
		ret = EINVAL;
		goto bail;
	}

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyinstr(ci->ci_ivmethod, inbuf, MAX_KEYSIZE, NULL);
	if (ret)
		goto bail;

	for (i = 0; i < __arraycount(encblkno); i++)
		if (strcmp(encblkno[i].n, inbuf) == 0)
			break;

	if (i == __arraycount(encblkno)) {
		ret = EINVAL;
		goto bail;
	}

	keybytes = ci->ci_keylen / 8 + 1;
	if (keybytes > MAX_KEYSIZE) {
		ret = EINVAL;
		goto bail;
	}

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyin(ci->ci_key, inbuf, keybytes);
	if (ret)
		goto bail;

	cs->sc_cdata.cf_blocksize = ci->ci_blocksize;
	cs->sc_cdata.cf_mode = encblkno[i].v;
	cs->sc_cdata.cf_keylen = ci->ci_keylen;
	cs->sc_cdata.cf_priv = cs->sc_cfuncs->cf_init(ci->ci_keylen, inbuf,
	    &cs->sc_cdata.cf_blocksize);
	if (cs->sc_cdata.cf_blocksize > CGD_MAXBLOCKSIZE) {
	    log(LOG_WARNING, "cgd: Disallowed cipher with blocksize %zu > %u\n",
		cs->sc_cdata.cf_blocksize, CGD_MAXBLOCKSIZE);
	    cs->sc_cdata.cf_priv = NULL;
	}

	/*
	 * The blocksize is supposed to be in bytes. Unfortunately originally
	 * it was expressed in bits. For compatibility we maintain encblkno
	 * and encblkno8.
	 */
	cs->sc_cdata.cf_blocksize /= encblkno[i].d;
	(void)explicit_memset(inbuf, 0, MAX_KEYSIZE);
	if (!cs->sc_cdata.cf_priv) {
		ret = EINVAL;		/* XXX is this the right error? */
		goto bail;
	}
	free(inbuf, M_TEMP);

	bufq_alloc(&dksc->sc_bufq, "fcfs", 0);

	cs->sc_data = malloc(MAXPHYS, M_DEVBUF, M_WAITOK);
	cs->sc_data_used = 0;

	/* Attach the disk. */
	dk_attach(dksc);
	disk_attach(&dksc->sc_dkdev);

	disk_set_info(dksc->sc_dev, &dksc->sc_dkdev, NULL);

	/* Discover wedges on this disk. */
	dkwedge_discover(&dksc->sc_dkdev);

	return 0;

bail:
	free(inbuf, M_TEMP);
	(void)vn_close(vp, FREAD|FWRITE, l->l_cred);
	return ret;
}

/* ARGSUSED */
static int
cgd_ioctl_clr(struct cgd_softc *cs, struct lwp *l)
{
	struct	dk_softc *dksc = &cs->sc_dksc;

	if (!DK_ATTACHED(dksc))
		return ENXIO;

	/* Delete all of our wedges. */
	dkwedge_delall(&dksc->sc_dkdev);

	/* Kill off any queued buffers. */
	dk_drain(dksc);
	bufq_free(dksc->sc_bufq);

	(void)vn_close(cs->sc_tvn, FREAD|FWRITE, l->l_cred);
	cs->sc_cfuncs->cf_destroy(cs->sc_cdata.cf_priv);
	free(cs->sc_tpath, M_DEVBUF);
	free(cs->sc_data, M_DEVBUF);
	cs->sc_data_used = 0;
	dk_detach(dksc);
	disk_detach(&dksc->sc_dkdev);

	return 0;
}

static int
cgd_ioctl_get(dev_t dev, void *data, struct lwp *l)
{
	struct cgd_softc *cs = getcgd_softc(dev);
	struct cgd_user *cgu;
	int unit;
	struct	dk_softc *dksc = &cs->sc_dksc;

	unit = CGDUNIT(dev);
	cgu = (struct cgd_user *)data;

	DPRINTF_FOLLOW(("cgd_ioctl_get(0x%"PRIx64", %d, %p, %p)\n",
			   dev, unit, data, l));

	if (cgu->cgu_unit == -1)
		cgu->cgu_unit = unit;

	if (cgu->cgu_unit < 0)
		return EINVAL;	/* XXX: should this be ENXIO? */

	cs = device_lookup_private(&cgd_cd, unit);
	if (cs == NULL || !DK_ATTACHED(dksc)) {
		cgu->cgu_dev = 0;
		cgu->cgu_alg[0] = '\0';
		cgu->cgu_blocksize = 0;
		cgu->cgu_mode = 0;
		cgu->cgu_keylen = 0;
	}
	else {
		cgu->cgu_dev = cs->sc_tdev;
		strlcpy(cgu->cgu_alg, cs->sc_cfuncs->cf_name,
		    sizeof(cgu->cgu_alg));
		cgu->cgu_blocksize = cs->sc_cdata.cf_blocksize;
		cgu->cgu_mode = cs->sc_cdata.cf_mode;
		cgu->cgu_keylen = cs->sc_cdata.cf_keylen;
	}
	return 0;
}

static int
cgdinit(struct cgd_softc *cs, const char *cpath, struct vnode *vp,
	struct lwp *l)
{
	struct	disk_geom *dg;
	int	ret;
	char	*tmppath;
	uint64_t psize;
	unsigned secsize;
	struct dk_softc *dksc = &cs->sc_dksc;

	cs->sc_tvn = vp;
	cs->sc_tpath = NULL;

	tmppath = malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
	ret = copyinstr(cpath, tmppath, MAXPATHLEN, &cs->sc_tpathlen);
	if (ret)
		goto bail;
	cs->sc_tpath = malloc(cs->sc_tpathlen, M_DEVBUF, M_WAITOK);
	memcpy(cs->sc_tpath, tmppath, cs->sc_tpathlen);

	cs->sc_tdev = vp->v_rdev;

	if ((ret = getdisksize(vp, &psize, &secsize)) != 0)
		goto bail;

	if (psize == 0) {
		ret = ENODEV;
		goto bail;
	}

	/*
	 * XXX here we should probe the underlying device.  If we
	 *     are accessing a partition of type RAW_PART, then
	 *     we should populate our initial geometry with the
	 *     geometry that we discover from the device.
	 */
	dg = &dksc->sc_dkdev.dk_geom;
	memset(dg, 0, sizeof(*dg));
	dg->dg_secperunit = psize;
	dg->dg_secsize = secsize;
	dg->dg_ntracks = 1;
	dg->dg_nsectors = 1024 * 1024 / dg->dg_secsize;
	dg->dg_ncylinders = dg->dg_secperunit / dg->dg_nsectors;

bail:
	free(tmppath, M_TEMP);
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
 * XXXrcd: for now we rely on our own crypto framework defined
 *         in dev/cgd_crypto.c.  This will change when we
 *         get a generic kernel crypto framework.
 */

static void
blkno2blkno_buf(char *sbuf, daddr_t blkno)
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
		*sbuf++ = blkno & 0xff;
		blkno >>= 8;
	}
}

static void
cgd_cipher(struct cgd_softc *cs, void *dstv, void *srcv,
    size_t len, daddr_t blkno, size_t secsize, int dir)
{
	char		*dst = dstv;
	char		*src = srcv;
	cfunc_cipher_prep	*ciprep = cs->sc_cfuncs->cf_cipher_prep;
	cfunc_cipher	*cipher = cs->sc_cfuncs->cf_cipher;
	struct uio	dstuio;
	struct uio	srcuio;
	struct iovec	dstiov[2];
	struct iovec	srciov[2];
	size_t		blocksize = cs->sc_cdata.cf_blocksize;
	size_t		todo;
	char		blkno_buf[CGD_MAXBLOCKSIZE], *iv;

	DPRINTF_FOLLOW(("cgd_cipher() dir=%d\n", dir));

	DIAGCONDPANIC(len % blocksize != 0,
	    ("cgd_cipher: len %% blocksize != 0"));

	/* ensure that sizeof(daddr_t) <= blocksize (for encblkno IVing) */
	DIAGCONDPANIC(sizeof(daddr_t) > blocksize,
	    ("cgd_cipher: sizeof(daddr_t) > blocksize"));

	DIAGCONDPANIC(blocksize > CGD_MAXBLOCKSIZE,
	    ("cgd_cipher: blocksize > CGD_MAXBLOCKSIZE"));

	dstuio.uio_iov = dstiov;
	dstuio.uio_iovcnt = 1;

	srcuio.uio_iov = srciov;
	srcuio.uio_iovcnt = 1;

	for (; len > 0; len -= todo) {
		todo = MIN(len, secsize);

		dstiov[0].iov_base = dst;
		srciov[0].iov_base = src;
		dstiov[0].iov_len  = todo;
		srciov[0].iov_len  = todo;

		memset(blkno_buf, 0x0, blocksize);
		blkno2blkno_buf(blkno_buf, blkno);
		IFDEBUG(CGDB_CRYPTO, hexprint("step 1: blkno_buf",
		    blkno_buf, blocksize));

		/*
		 * Compute an initial IV. All ciphers
		 * can convert blkno_buf in-place.
		 */
		iv = blkno_buf;
		ciprep(cs->sc_cdata.cf_priv, iv, blkno_buf, blocksize, dir);
		IFDEBUG(CGDB_CRYPTO, hexprint("step 2: iv", iv, blocksize));

		cipher(cs->sc_cdata.cf_priv, &dstuio, &srcuio, iv, dir);

		dst += todo;
		src += todo;
		blkno++;
	}
}

#ifdef DEBUG
static void
hexprint(const char *start, void *buf, int len)
{
	char	*c = buf;

	DIAGCONDPANIC(len < 0, ("hexprint: called with len < 0"));
	printf("%s: len=%06d 0x", start, len);
	while (len--)
		printf("%02x", (unsigned char) *c++);
}
#endif

static void
selftest(void)
{
	struct cgd_softc cs;
	void *buf;

	printf("running cgd selftest ");

	for (size_t i = 0; i < __arraycount(selftests); i++) {
		const char *alg = selftests[i].alg;
		const uint8_t *key = selftests[i].key;
		int keylen = selftests[i].keylen;
		int txtlen = selftests[i].txtlen;

		printf("%s-%d ", alg, keylen);

		memset(&cs, 0, sizeof(cs));

		cs.sc_cfuncs = cryptfuncs_find(alg);
		if (cs.sc_cfuncs == NULL)
			panic("%s not implemented", alg);

		cs.sc_cdata.cf_blocksize = 8 * selftests[i].blocksize;
		cs.sc_cdata.cf_mode = CGD_CIPHER_CBC_ENCBLKNO1;
		cs.sc_cdata.cf_keylen = keylen;

		cs.sc_cdata.cf_priv = cs.sc_cfuncs->cf_init(keylen,
		    key, &cs.sc_cdata.cf_blocksize);
		if (cs.sc_cdata.cf_priv == NULL)
			panic("cf_priv is NULL");
		if (cs.sc_cdata.cf_blocksize > CGD_MAXBLOCKSIZE)
			panic("bad block size %zu", cs.sc_cdata.cf_blocksize);

		cs.sc_cdata.cf_blocksize /= 8;

		buf = malloc(txtlen, M_DEVBUF, M_WAITOK);
		memcpy(buf, selftests[i].ptxt, txtlen);

		cgd_cipher(&cs, buf, buf, txtlen, selftests[i].blkno,
				selftests[i].secsize, CGD_CIPHER_ENCRYPT);
		if (memcmp(buf, selftests[i].ctxt, txtlen) != 0)
			panic("encryption is broken");

		cgd_cipher(&cs, buf, buf, txtlen, selftests[i].blkno,
				selftests[i].secsize, CGD_CIPHER_DECRYPT);
		if (memcmp(buf, selftests[i].ptxt, txtlen) != 0)
			panic("decryption is broken");

		free(buf, M_DEVBUF);
		cs.sc_cfuncs->cf_destroy(cs.sc_cdata.cf_priv);
	}

	printf("done\n");
}

MODULE(MODULE_CLASS_DRIVER, cgd, "blowfish,des,dk_subr,bufq_fcfs");

#ifdef _MODULE
CFDRIVER_DECL(cgd, DV_DISK, NULL);

devmajor_t cgd_bmajor = -1, cgd_cmajor = -1;
#endif

static int
cgd_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		selftest();
#ifdef _MODULE
		error = config_cfdriver_attach(&cgd_cd);
		if (error)
			break;

		error = config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
	        if (error) {
			config_cfdriver_detach(&cgd_cd);
			aprint_error("%s: unable to register cfattach for"
			    "%s, error %d\n", __func__, cgd_cd.cd_name, error);
			break;
		}
		/*
		 * Attach the {b,c}devsw's
		 */
		error = devsw_attach("cgd", &cgd_bdevsw, &cgd_bmajor,
		    &cgd_cdevsw, &cgd_cmajor);

		/*
		 * If devsw_attach fails, remove from autoconf database
		 */
		if (error) {
			config_cfattach_detach(cgd_cd.cd_name, &cgd_ca);
			config_cfdriver_detach(&cgd_cd);
			aprint_error("%s: unable to attach %s devsw, "
			    "error %d", __func__, cgd_cd.cd_name, error);
			break;
		}
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		/*
		 * Remove {b,c}devsw's
		 */
		devsw_detach(&cgd_bdevsw, &cgd_cdevsw);

		/*
		 * Now remove device from autoconf database
		 */
		error = config_cfattach_detach(cgd_cd.cd_name, &cgd_ca);
		if (error) {
			(void)devsw_attach("cgd", &cgd_bdevsw, &cgd_bmajor,
			    &cgd_cdevsw, &cgd_cmajor);
			aprint_error("%s: failed to detach %s cfattach, "
			    "error %d\n", __func__, cgd_cd.cd_name, error);
 			break;
		}
		error = config_cfdriver_detach(&cgd_cd);
		if (error) {
			(void)config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
			(void)devsw_attach("cgd", &cgd_bdevsw, &cgd_bmajor,
			    &cgd_cdevsw, &cgd_cmajor);
			aprint_error("%s: failed to detach %s cfdriver, "
			    "error %d\n", __func__, cgd_cd.cd_name, error);
			break;
		}
#endif
		break;

	case MODULE_CMD_STAT:
		error = ENOTTY;
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
