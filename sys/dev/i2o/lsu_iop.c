/*	$NetBSD: lsu_iop.c,v 1.1.2.2 2000/11/22 17:34:20 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * I2O front-end for lsu(4) driver, supporting random block storage class
 * devices.  Currently, this doesn't support anything more complex than
 * fixed, direct access devices; hopefully, scsipi can take care of the
 * rest.
 */

#include "opt_i2o.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/proc.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>

#include <dev/lsuvar.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopvar.h>

#define	LSU_IOP_MAXQUEUECNT	64		/* XXX */

struct lsu_iop_softc {
	struct	lsu_softc sc_lsu;
	struct	iop_initiator sc_ii;
	u_int	sc_tid;
};

static void	lsu_iop_attach(struct device *, struct device *, void *);
static int	lsu_iop_dump(struct lsu_softc *, void *, int, int);
static int	lsu_iop_flush(struct lsu_softc *);
static void	lsu_iop_intr(struct device *, struct iop_msg *, void *);
static int	lsu_iop_start(struct lsu_softc *, struct buf *);
static int	lsu_iop_match(struct device *, struct cfdata *, void *);

struct cfattach lsu_iop_ca = {
	sizeof(struct lsu_iop_softc), lsu_iop_match, lsu_iop_attach
};

#ifdef I2OVERBOSE
static const char *lsu_iop_errors[] = { 
	"success", 
	"media error", 
	"failure communicating with device",
	"device failure",
	"device is not ready",
	"media not present",
	"media locked by another user",
	"media failure",
	"failure communicating to device",
	"device bus failure",
	"device locked by another user",
	"device write protected",
	"device reset",
	"volume has changed, waiting for acknowledgement",
};
#endif

static int
lsu_iop_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct iop_attach_args *ia;
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		struct	i2o_param_rbs_device_info bdi;
	} param;
	u_int32_t caps;

	ia = aux;

	if (ia->ia_class != I2O_CLASS_RANDOM_BLOCK_STORAGE)
		return (0);

	if (iop_params_get((struct iop_softc *)parent, ia->ia_tid,
	    I2O_PARAM_RBS_DEVICE_INFO, &param, sizeof(param)) != 0)
		return (0);
	
	caps = le32toh(param.bdi.capabilities);
	
	if (param.bdi.type != I2O_RBS_TYPE_DIRECT ||
	    (caps & I2O_RBS_CAP_REMOVEABLE_MEDIA) != 0 ||
	    (caps & I2O_RBS_CAP_REMOVEABLE_DEVICE) != 0)
		return (0);

	/*
	 * Mark the physical device(s) that comprise(s) the block storage
	 * unit as being `in use'.
	 */
	iop_tid_markallused((struct iop_softc *)parent, ia->ia_tid);
	return (1);
}

static void
lsu_iop_attach(struct device *parent, struct device *self, void *aux)
{
	struct iop_attach_args *ia;
	struct lsu_softc *lsu;
	struct lsu_iop_softc *sc;
	struct iop_softc *iop;
	int rv;
	char ident[64 + 1];
	u_int cachesz;
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		union {
			struct	i2o_param_rbs_cache_control cc;
			struct	i2o_param_rbs_device_info bdi;
			struct	i2o_param_device_identity di;
		} p;
	} param;

	sc = (struct lsu_iop_softc *)self;
	lsu = &sc->sc_lsu;
	iop = (struct iop_softc *)parent;
	ia = (struct iop_attach_args *)aux;
	sc->sc_tid = ia->ia_tid;

	/* Register us as an initiator. */
	sc->sc_ii.ii_dv = self;
	sc->sc_ii.ii_intr = lsu_iop_intr;
	sc->sc_ii.ii_flags = 0;
	if (iop_initiator_register(iop, &sc->sc_ii) != 0) {
		printf("%s: unable to register as an initiator\n",
		    self->dv_xname);
		return;
	}

	lsu->sc_maxxfer = IOP_MAX_XFER;
	lsu->sc_maxqueuecnt = LSU_IOP_MAXQUEUECNT;
	lsu->sc_dump = lsu_iop_dump;
	lsu->sc_flush = lsu_iop_flush;
	lsu->sc_start = lsu_iop_start;

	/* Say what the device is. */
	printf(": ");
	if (iop_params_get(iop, ia->ia_tid, I2O_PARAM_DEVICE_IDENTITY, &param,
	    sizeof(param)) == 0) {
		iop_strvis(param.p.di.vendorinfo, 
		    sizeof(param.p.di.vendorinfo), ident, sizeof(ident));
		printf("<%s, ", ident);
		iop_strvis(param.p.di.productinfo, 
		    sizeof(param.p.di.productinfo), ident, sizeof(ident));
		printf("%s, ", ident);
		iop_strvis(param.p.di.revlevel, 
		    sizeof(param.p.di.revlevel), ident, sizeof(ident));
		printf("%s> ", ident);
	}

	/* Claim the device so that we don't get any nasty surprises. */
	rv = iop_tid_claim(iop, ia->ia_tid, sc->sc_ii.ii_ictx,
	    I2O_UTIL_CLAIM_RESET_SENSITIVE |
	    I2O_UTIL_CLAIM_STATE_SENSITIVE |
	    I2O_UTIL_CLAIM_CAPACITY_SENSITIVE |
	    I2O_UTIL_CLAIM_NO_PEER_SERVICE |
	    I2O_UTIL_CLAIM_NO_MANAGEMENT_SERVICE |
	    I2O_UTIL_CLAIM_PRIMARY_USER);
	if (rv != 0) {
		printf("%s: unable to claim device (%d)\n",
		   lsu->sc_dv.dv_xname, rv);
		goto bad;
	}

	rv = iop_params_get(iop, ia->ia_tid, I2O_PARAM_RBS_DEVICE_INFO, &param,
	    sizeof(param));
	if (rv != 0) {
		printf("%s: unable to retrieve device parameters (%d)\n",
		   lsu->sc_dv.dv_xname, rv);
		goto bad;
	}

	lsu->sc_secsize = le32toh(param.p.bdi.blocksize);
	lsu->sc_secperunit = (int)
	    (le64toh(param.p.bdi.capacity) / lsu->sc_secsize);

	/* Build synthetic geometry. */
	if (lsu->sc_secperunit <= 528 * 2048)		/* 528MB */
		lsu->sc_nheads = 16;
	else if (lsu->sc_secperunit <= 1024 * 2048)	/* 1GB */
		lsu->sc_nheads = 32;
	else if (lsu->sc_secperunit <= 21504 * 2048)	/* 21GB */
		lsu->sc_nheads = 64;
	else if (lsu->sc_secperunit <= 43008 * 2048)	/* 42GB */
		lsu->sc_nheads = 128;
	else
		lsu->sc_nheads = 255;
	
	lsu->sc_nsectors = 63;
	lsu->sc_ncylinders = lsu->sc_secperunit / 
	    (lsu->sc_nheads * lsu->sc_nsectors);

#ifdef notyet	    
	switch (param.p.bdi.type) {
	case I2O_RBS_TYPE_DIRECT:
		typestr = "direct access";
		break;
	case I2O_RBS_TYPE_WORM:
		typestr = "WORM";
		break;
	case I2O_RBS_TYPE_CDROM:
		typestr = "cdrom";
		break;
	case I2O_RBS_TYPE_OPTICAL:
		typestr = "optical";
		break;
	default:
		typestr = "unknown";
		break;
	}

	if ((le32toh(param.p.bdi.capabilities) & I2O_RBS_CAP_REMOVEABLE_MEDIA)
	    != 0) {
		lsu->sc_flags = LSUF_ENABLED | LSUF_REMOVEABLE;
		fixedstr = "removeable";
	} else {
		lsu->sc_flags = LSUF_ENABLED;
		fixedstr = "fixed";
	}
#endif
	printf("direct access, fixed");

	/*
	 * Determine if the device has an private cache.  If so, print the
	 * cache size.  Even if the device doesn't appear to have a cache,
	 * we perform a flush at shutdown, as it is still valid to do so.
	 */
	rv = iop_params_get(iop, ia->ia_tid, I2O_PARAM_RBS_CACHE_CONTROL,
	    &param, sizeof(param));
	if (rv != 0) {
		printf("%s: unable to retrieve cache parameters (%d)\n",
		   lsu->sc_dv.dv_xname, rv);
		goto bad;
	}

	if ((cachesz = le32toh(param.p.cc.totalcachesize)) != 0)
		printf(", %dkB cache", cachesz >> 10);

	printf("\n");		
	lsu->sc_flags = LSUF_ENABLED;
	lsuattach(lsu);
	return;

bad:
	iop_initiator_unregister(iop, &sc->sc_ii);
}

static int
lsu_iop_start(struct lsu_softc *lsu, struct buf *bp)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct lsu_iop_softc *sc;
	struct i2o_rbs_block_read *mb;
	int rv, flags, write;
	u_int64_t ba;

	sc = (struct lsu_iop_softc *)lsu;
	iop = (struct iop_softc *)lsu->sc_dv.dv_parent;

	im = NULL;
	if ((rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT)) != 0)
		goto bad;
	im->im_dvcontext = bp;

	write = ((bp->b_flags & B_READ) == 0);
	ba = (u_int64_t)bp->b_rawblkno * lsu->sc_secsize;

	if (write) {
		if (bp == NULL || (bp->b_flags & B_ASYNC) == 0)
			flags = I2O_RBS_BLOCK_WRITE_CACHE_WT;
		else
			flags = I2O_RBS_BLOCK_WRITE_CACHE_WB;
	}

	/*
	 * Fill the message frame.  We can use the block_read structure for
	 * both reads and writes, as it's almost identical to the
	 * block_write structure.
	 *
	 * XXX We should be using the command time out facilities.
	 */
	mb = (struct i2o_rbs_block_read *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_block_read);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid,
	    write ? I2O_RBS_BLOCK_WRITE : I2O_RBS_BLOCK_READ);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = flags;
	mb->datasize = bp->b_bcount;
	mb->lowoffset = (u_int32_t)ba;
	mb->highoffset = (u_int32_t)(ba >> 32);

	/* Map the data transfer. */
	if ((rv = iop_msg_map(iop, im, bp->b_data, bp->b_bcount, write)) != 0)
		goto bad;

	/* Enqueue the command. */
	if ((rv = iop_msg_enqueue(iop, im)) != 0) {
		iop_msg_unmap(iop, im);
		goto bad;
	}

	return (0);

bad:
	if (im != NULL)
		iop_msg_free(iop, &sc->sc_ii, im);
	return (rv);
}

static int
lsu_iop_dump(struct lsu_softc *lsu, void *data, int blkno, int blkcnt)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct lsu_iop_softc *sc;
	struct i2o_rbs_block_write *mb;
	int rv, bcount;
	u_int64_t ba;

	sc = (struct lsu_iop_softc *)lsu;
	iop = (struct iop_softc *)lsu->sc_dv.dv_parent;
	bcount = blkcnt * lsu->sc_secsize;
	ba = (u_int64_t)blkno * lsu->sc_secsize;

	rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT | IM_NOINTR);
	if (rv != 0)
		return (rv);

	mb = (struct i2o_rbs_block_write *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_block_write);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid, I2O_RBS_BLOCK_WRITE);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = I2O_RBS_BLOCK_WRITE_CACHE_WT;
	mb->datasize = bcount;
	mb->lowoffset = (u_int32_t)ba;
	mb->highoffset = (u_int32_t)(ba >> 32);

	if ((rv = iop_msg_map(iop, im, data, bcount, 1)) != 0) {
		iop_msg_free(iop, &sc->sc_ii, im);
		return (rv);
	}

	rv = iop_msg_send(iop, im, 5000) != 0 ? EIO : 0;
	iop_msg_unmap(iop, im);
	iop_msg_free(iop, &sc->sc_ii, im);
 	return (rv);
}

static int
lsu_iop_flush(struct lsu_softc *lsu)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct lsu_iop_softc *sc;
	struct i2o_rbs_cache_flush *mb;
	int rv;

	sc = (struct lsu_iop_softc *)lsu;
	iop = (struct iop_softc *)lsu->sc_dv.dv_parent;

	rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT | IM_NOINTR);
	if (rv != 0)
		return (rv);

	mb = (struct i2o_rbs_cache_flush *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_cache_flush);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid, I2O_RBS_CACHE_FLUSH);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = 0;

 	rv = iop_msg_send(iop, im, 10000);
	iop_msg_free(iop, &sc->sc_ii, im);
	return (rv);
}

void
lsu_iop_intr(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_rbs_reply *rb;
	struct buf *bp;
	struct lsu_iop_softc *sc;
	struct iop_softc *iop;
#ifdef I2OVERBOSE
	int detail;
	const char *errstr;
#endif

	rb = reply;
	bp = im->im_dvcontext;
	sc = (struct lsu_iop_softc *)dv;
	iop = (struct iop_softc *)dv->dv_parent;

#ifdef I2OVERBOSE
	if (rb->reqstatus != I2O_STATUS_SUCCESS) {
		detail = le16toh(rb->detail);
		if (detail > sizeof(lsu_iop_errors) / sizeof(lsu_iop_errors[0]))
			errstr = "unknown error";
		else
			errstr = lsu_iop_errors[detail];
		printf("%s: %s\n", dv->dv_xname, errstr);
#else
	if (rb->reqstatus != I2O_STATUS_SUCCESS) {
#endif
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
#ifndef notyet
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = 0;
#else
	}
	bp->b_resid = bp->b_bcount - le32toh(rb->transfercount);
#endif

	iop_msg_unmap(iop, im);
	iop_msg_free(iop, &sc->sc_ii, im);
	lsudone(&sc->sc_lsu, bp);
}
