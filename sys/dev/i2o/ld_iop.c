/*	$NetBSD: ld_iop.c,v 1.4 2001/01/03 21:01:29 ad Exp $	*/

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
 * I2O front-end for ld(4) driver, supporting random block storage class
 * devices.  Currently, this doesn't handle anything more complex than
 * fixed direct-access devices.
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

#include <dev/ldvar.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopvar.h>

#define	LD_IOP_MAXQUEUECNT	64		/* XXX */
#define	LD_IOP_TIMEOUT		10*1000*1000

struct ld_iop_softc {
	struct	ld_softc sc_ld;
	struct	iop_initiator sc_ii;
	struct	iop_initiator sc_eventii;
	int	sc_claimed;
	u_int	sc_tid;
};

static void	ld_iop_attach(struct device *, struct device *, void *);
static int	ld_iop_detach(struct device *, int);
static int	ld_iop_dump(struct ld_softc *, void *, int, int);
static int	ld_iop_flush(struct ld_softc *);
static void	ld_iop_intr(struct device *, struct iop_msg *, void *);
static void	ld_iop_intr_event(struct device *, struct iop_msg *, void *);
static int	ld_iop_start(struct ld_softc *, struct buf *);
static int	ld_iop_match(struct device *, struct cfdata *, void *);

struct cfattach ld_iop_ca = {
	sizeof(struct ld_iop_softc),
	ld_iop_match,
	ld_iop_attach,
	ld_iop_detach
};

#ifdef I2OVERBOSE
static const char *ld_iop_errors[] = { 
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
ld_iop_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct iop_attach_args *ia;

	ia = aux;

	return (ia->ia_class == I2O_CLASS_RANDOM_BLOCK_STORAGE);
}

static void
ld_iop_attach(struct device *parent, struct device *self, void *aux)
{
	struct iop_attach_args *ia;
	struct ld_softc *ld;
	struct ld_iop_softc *sc;
	struct iop_softc *iop;
	int rv, evreg, enable;
	char ident[64 + 1], *typestr, *fixedstr;
	u_int cachesz;
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		union {
			struct	i2o_param_rbs_cache_control cc;
			struct	i2o_param_rbs_device_info bdi;
			struct	i2o_param_device_identity di;
			struct	i2o_param_rbs_operation op;
		} p;
	} param;

	sc = (struct ld_iop_softc *)self;
	ld = &sc->sc_ld;
	iop = (struct iop_softc *)parent;
	ia = (struct iop_attach_args *)aux;
	sc->sc_tid = ia->ia_tid;
	evreg = 0;

	/* Register us as an initiator. */
	sc->sc_ii.ii_dv = self;
	sc->sc_ii.ii_intr = ld_iop_intr;
	sc->sc_ii.ii_flags = 0;
	sc->sc_ii.ii_tid = ia->ia_tid;
	if (iop_initiator_register(iop, &sc->sc_ii) != 0) {
		printf("%s: unable to register initiator\n", self->dv_xname);
		return;
	}

	/* Register another initiator to handle events from the device. */
	sc->sc_eventii.ii_dv = self;
	sc->sc_eventii.ii_intr = ld_iop_intr_event;
	sc->sc_eventii.ii_flags = II_DISCARD | II_UTILITY;
	sc->sc_eventii.ii_tid = ia->ia_tid;
	if (iop_initiator_register(iop, &sc->sc_eventii) != 0) {
		printf("%s: unable to register initiator", self->dv_xname);
		goto bad;
	}
	if (iop_util_eventreg(iop, &sc->sc_eventii, 0xffffffff)) {
		printf("%s: unable to register for events", self->dv_xname);
		goto bad;
	}
	evreg = 1;

	ld->sc_maxxfer = IOP_MAX_XFER;
	ld->sc_maxqueuecnt = LD_IOP_MAXQUEUECNT;
	ld->sc_dump = ld_iop_dump;
	ld->sc_flush = ld_iop_flush;
	ld->sc_start = ld_iop_start;

	/* Say what the device is. */
	printf(": ");
	if (iop_param_op(iop, ia->ia_tid, 0, I2O_PARAM_DEVICE_IDENTITY, &param,
	    sizeof(param)) == 0) {
		iop_strvis(iop, param.p.di.vendorinfo, 
		    sizeof(param.p.di.vendorinfo), ident, sizeof(ident));
		printf("<%s, ", ident);
		iop_strvis(iop, param.p.di.productinfo, 
		    sizeof(param.p.di.productinfo), ident, sizeof(ident));
		printf("%s, ", ident);
		iop_strvis(iop, param.p.di.revlevel, 
		    sizeof(param.p.di.revlevel), ident, sizeof(ident));
		printf("%s> ", ident);
	}

	/*
	 * Claim the device so that we don't get any nasty surprises.  Allow
	 * failure.
	 */
	sc->sc_claimed = !iop_util_claim(iop, &sc->sc_ii, 0,
	    I2O_UTIL_CLAIM_CAPACITY_SENSITIVE |
	    I2O_UTIL_CLAIM_NO_PEER_SERVICE |
	    I2O_UTIL_CLAIM_NO_MANAGEMENT_SERVICE |
	    I2O_UTIL_CLAIM_PRIMARY_USER);

	rv = iop_param_op(iop, ia->ia_tid, 0, I2O_PARAM_RBS_DEVICE_INFO,
	    &param, sizeof(param));
	if (rv != 0) {
		printf("%s: unable to get parameters (0x%04x; %d)\n",
		   ld->sc_dv.dv_xname, I2O_PARAM_RBS_DEVICE_INFO, rv);
		goto bad;
	}

	ld->sc_secsize = le32toh(param.p.bdi.blocksize);
	ld->sc_secperunit = (int)
	    (le64toh(param.p.bdi.capacity) / ld->sc_secsize);

	/* Build synthetic geometry. */
	if (ld->sc_secperunit <= 528 * 2048)		/* 528MB */
		ld->sc_nheads = 16;
	else if (ld->sc_secperunit <= 1024 * 2048)	/* 1GB */
		ld->sc_nheads = 32;
	else if (ld->sc_secperunit <= 21504 * 2048)	/* 21GB */
		ld->sc_nheads = 64;
	else if (ld->sc_secperunit <= 43008 * 2048)	/* 42GB */
		ld->sc_nheads = 128;
	else
		ld->sc_nheads = 255;

	ld->sc_nsectors = 63;
	ld->sc_ncylinders = ld->sc_secperunit / 
	    (ld->sc_nheads * ld->sc_nsectors);

	switch (param.p.bdi.type) {
	case I2O_RBS_TYPE_DIRECT:
		typestr = "direct access";
		enable = 1;
		break;
	case I2O_RBS_TYPE_WORM:
		typestr = "WORM";
		enable = 0;
		break;
	case I2O_RBS_TYPE_CDROM:
		typestr = "cdrom";
		enable = 0;
		break;
	case I2O_RBS_TYPE_OPTICAL:
		typestr = "optical";
		enable = 0;
		break;
	default:
		typestr = "unknown";
		enable = 0;
		break;
	}

	if ((le32toh(param.p.bdi.capabilities) & I2O_RBS_CAP_REMOVEABLE_MEDIA)
	    != 0) {
		/* ld->sc_flags = LDF_REMOVEABLE; */
		fixedstr = "removeable";
		enable = 0;
	} else
		fixedstr = "fixed";

	printf("%s, %s", typestr, fixedstr);

	/*
	 * Determine if the device has an private cache.  If so, print the
	 * cache size.  Even if the device doesn't appear to have a cache,
	 * we perform a flush at shutdown, as it is still valid to do so.
	 */
	rv = iop_param_op(iop, ia->ia_tid, 0, I2O_PARAM_RBS_CACHE_CONTROL,
	    &param, sizeof(param));
	if (rv != 0) {
		printf("%s: unable to get parameters (0x%04x; %d)\n",
		   ld->sc_dv.dv_xname, I2O_PARAM_RBS_CACHE_CONTROL, rv);
		goto bad;
	}

	if ((cachesz = le32toh(param.p.cc.totalcachesize)) != 0)
		printf(", %dkB cache", cachesz >> 10);

	printf("\n");

	/*
	 * Configure the DDM's timeout functions to time out all commands
	 * after 10 seconds.
	 */
	rv = iop_param_op(iop, ia->ia_tid, 0, I2O_PARAM_RBS_OPERATION,
	    &param, sizeof(param));
	if (rv != 0) {
		printf("%s: unable to get parameters (0x%04x; %d)\n",
		   ld->sc_dv.dv_xname, I2O_PARAM_RBS_OPERATION, rv);
		goto bad;
	}

	param.p.op.timeoutbase = htole32(LD_IOP_TIMEOUT); 
	param.p.op.rwvtimeoutbase = htole32(LD_IOP_TIMEOUT); 
	param.p.op.rwvtimeout = 0; 

	rv = iop_param_op(iop, ia->ia_tid, 1, I2O_PARAM_RBS_OPERATION,
	    &param, sizeof(param));
	if (rv != 0) {
		printf("%s: unable to set parameters (0x%04x; %d)\n",
		   ld->sc_dv.dv_xname, I2O_PARAM_RBS_OPERATION, rv);
		goto bad;
	}

	if (enable)
		ld->sc_flags |= LDF_ENABLED;
	else
		printf("%s: device not yet supported\n", self->dv_xname);

	ldattach(ld);
	return;

bad:
	if (sc->sc_claimed)
		iop_util_claim(iop, &sc->sc_ii, 1,
		    I2O_UTIL_CLAIM_PRIMARY_USER);
	if (evreg)
		iop_util_eventreg(iop, &sc->sc_eventii, 0);
	if (sc->sc_eventii.ii_intr != NULL)
		iop_initiator_unregister(iop, &sc->sc_eventii);
	iop_initiator_unregister(iop, &sc->sc_ii);
}

static int
ld_iop_detach(struct device *self, int flags)
{
	struct ld_iop_softc *sc;
	struct iop_softc *iop;
	int rv;

	sc = (struct ld_iop_softc *)self;

	if ((rv = lddrain(&sc->sc_ld, flags)) != 0)
		return (rv);

	iop = (struct iop_softc *)self->dv_parent;

	/*
	 * Abort any requests queued with the IOP, but allow requests that
	 * are already in progress to complete.
	 */
	if ((sc->sc_ld.sc_flags & LDF_ENABLED) != 0)
		iop_util_abort(iop, &sc->sc_ii, 0, 0,
		    I2O_UTIL_ABORT_WILD | I2O_UTIL_ABORT_CLEAN);

	lddetach(&sc->sc_ld);

	/* Un-claim the target, and un-register us as an initiator. */
	if ((sc->sc_ld.sc_flags & LDF_ENABLED) != 0) {
		if (sc->sc_claimed) {
			rv = iop_util_claim(iop, &sc->sc_ii, 1,
			    I2O_UTIL_CLAIM_PRIMARY_USER);
			if (rv != 0)
				return (rv);
		}
		iop_util_eventreg(iop, &sc->sc_eventii, 0);
		iop_initiator_unregister(iop, &sc->sc_eventii);
		iop_initiator_unregister(iop, &sc->sc_ii);
	}

	return (0);
}

static int
ld_iop_start(struct ld_softc *ld, struct buf *bp)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct ld_iop_softc *sc;
	struct i2o_rbs_block_read *mb;
	int rv, flags, write;
	u_int64_t ba;

	sc = (struct ld_iop_softc *)ld;
	iop = (struct iop_softc *)ld->sc_dv.dv_parent;

	im = NULL;
	if ((rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT)) != 0)
		goto bad;
	im->im_dvcontext = bp;

	write = ((bp->b_flags & B_READ) == 0);
	ba = (u_int64_t)bp->b_rawblkno * ld->sc_secsize;

	/*
	 * Write through the cache when performing synchronous writes.  When
	 * performing a read, we don't request that the DDM cache the data,
	 * as there's little advantage to it.
	 */
	if (write) {
		if ((bp->b_flags & B_ASYNC) == 0)
			flags = I2O_RBS_BLOCK_WRITE_CACHE_WT;
		else
			flags = I2O_RBS_BLOCK_WRITE_CACHE_WB;
	} else
		flags = 0;

	/*
	 * Fill the message frame.  We can use the block_read structure for
	 * both reads and writes, as it's almost identical to the
	 * block_write structure.
	 */
	mb = (struct i2o_rbs_block_read *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_block_read);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid,
	    write ? I2O_RBS_BLOCK_WRITE : I2O_RBS_BLOCK_READ);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = flags | (1 << 16);		/* flags & time multiplier */
	mb->datasize = bp->b_bcount;
	mb->lowoffset = (u_int32_t)ba;
	mb->highoffset = (u_int32_t)(ba >> 32);

	/* Map the data transfer. */
	if ((rv = iop_msg_map(iop, im, bp->b_data, bp->b_bcount, write)) != 0)
		goto bad;

	/* Enqueue the command. */
	iop_msg_enqueue(iop, im, 0);
	return (0);

bad:
	if (im != NULL)
		iop_msg_free(iop, &sc->sc_ii, im);
	return (rv);
}

static int
ld_iop_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct ld_iop_softc *sc;
	struct i2o_rbs_block_write *mb;
	int rv, bcount;
	u_int64_t ba;

	sc = (struct ld_iop_softc *)ld;
	iop = (struct iop_softc *)ld->sc_dv.dv_parent;
	bcount = blkcnt * ld->sc_secsize;
	ba = (u_int64_t)blkno * ld->sc_secsize;

	rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT | IM_NOINTR);
	if (rv != 0)
		return (rv);

	mb = (struct i2o_rbs_block_write *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_block_write);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid, I2O_RBS_BLOCK_WRITE);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = I2O_RBS_BLOCK_WRITE_CACHE_WT | (1 << 16);
	mb->datasize = bcount;
	mb->lowoffset = (u_int32_t)ba;
	mb->highoffset = (u_int32_t)(ba >> 32);

	if ((rv = iop_msg_map(iop, im, data, bcount, 1)) != 0) {
		iop_msg_free(iop, &sc->sc_ii, im);
		return (rv);
	}

	rv = (iop_msg_send(iop, im, 5000) != 0 ? EIO : 0);
	iop_msg_unmap(iop, im);
	iop_msg_free(iop, &sc->sc_ii, im);
 	return (rv);
}

static int
ld_iop_flush(struct ld_softc *ld)
{
	struct iop_msg *im;
	struct iop_softc *iop;
	struct ld_iop_softc *sc;
	struct i2o_rbs_cache_flush *mb;
	int rv;

	sc = (struct ld_iop_softc *)ld;
	iop = (struct iop_softc *)ld->sc_dv.dv_parent;

	rv = iop_msg_alloc(iop, &sc->sc_ii, &im, IM_NOWAIT | IM_NOINTR);
	if (rv != 0)
		return (rv);

	mb = (struct i2o_rbs_cache_flush *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_rbs_cache_flush);
	mb->msgfunc = I2O_MSGFUNC(sc->sc_tid, I2O_RBS_CACHE_FLUSH);
	mb->msgictx = sc->sc_ii.ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = 1 << 16;			/* time multiplier */

 	rv = iop_msg_send(iop, im, 10000);
	iop_msg_free(iop, &sc->sc_ii, im);
	return (rv);
}

void
ld_iop_intr(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_rbs_reply *rb;
	struct buf *bp;
	struct ld_iop_softc *sc;
	struct iop_softc *iop;
#ifdef I2OVERBOSE
	int detail;
	const char *errstr;
#endif

	rb = reply;
	bp = im->im_dvcontext;
	sc = (struct ld_iop_softc *)dv;
	iop = (struct iop_softc *)dv->dv_parent;

#ifdef I2OVERBOSE
	if (rb->reqstatus != I2O_STATUS_SUCCESS) {
		detail = le16toh(rb->detail);
		if (detail > sizeof(ld_iop_errors) / sizeof(ld_iop_errors[0]))
			errstr = "unknown error";
		else
			errstr = ld_iop_errors[detail];
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
	lddone(&sc->sc_ld, bp);
}

static void
ld_iop_intr_event(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_util_event_register_reply *rb;
	u_int event;

	rb = reply;
	event = le32toh(rb->event);

#ifndef I2ODEBUG
	if (event == I2O_EVENT_GEN_EVENT_MASK_MODIFIED)
		return;
#endif

	printf("%s: event 0x%08x received\n", dv->dv_xname, event);
}
