/*	$NetBSD: mlcd.c,v 1.1 2002/11/15 14:10:51 itohy Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/conf.h>

#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>

#define MLCD_MAXACCSIZE	1012	/* (255*4) - 8  =  253*32 / 8 */

struct mlcd_funcdef {	/* XXX assuming little-endian structure packing */
	unsigned unused	: 6,
		 bw	: 1,	/* 0: normally white, 1: normally black */
		 hv	: 1,	/* 0: horizontal, 1: vertical */
		 ra	: 4,	/* 0 */
		 wa	: 4,	/* number of access / write */
		 bb	: 8,	/* block size / 32 - 1 */
		 pt	: 8;	/* number of partition - 1 */
};

struct mlcd_request_write_data {
	u_int32_t	func_code;
	u_int8_t	pt;
	u_int8_t	phase;		/* 0, 1, 2, 3: for each 128 byte */
	u_int16_t	block;
	u_int8_t	data[MLCD_MAXACCSIZE];
};
#define MLCD_SIZE_REQW(sc)	((sc)->sc_waccsz + 8)

struct mlcd_request_get_media_info {
	u_int32_t	func_code;
	u_int32_t	pt;		/* pt (1 byte) and unused 3 bytes */
};

struct mlcd_media_info {
	u_int8_t	width;		/* width - 1 */
	u_int8_t	height;		/* height - 1 */
	u_int8_t	rsvd[2];	/* ? 0x10 0x02 */
};

struct mlcd_response_media_info {
	u_int32_t	func_code;	/* function code (big endian) */
	struct mlcd_media_info info;
};

struct mlcd_softc {
	struct device	sc_dev;

	struct device	*sc_parent;
	struct maple_unit *sc_unit;
	enum mlcd_stat {
		MLCD_INIT,	/* during initialization */
		MLCD_INIT2,	/* during initialization */
		MLCD_IDLE,	/* init done, not in I/O */
		MLCD_WRITE,	/* in write operation */
		MLCD_DETACH	/* detaching */
	} sc_stat;

	int		sc_npt;		/* number of partitions */
	int		sc_bsize;	/* block size */
	int		sc_wacc;	/* number of write access per block */
	int		sc_waccsz;	/* size of a write access */

	struct mlcd_pt {
		int		pt_flags;
#define MLCD_PT_OK	1	/* partition is alive */
#define MLCD_PT_OPEN	2
		struct mlcd_media_info pt_info;	/* geometry per part */
		int		pt_size;	/* partition size in byte */
		int		pt_nblk;	/* partition size on block */

		char		pt_name[16 /* see device.h */ + 4 /* ".256" */];
	} *sc_pt;

	/* write request buffer (only one is used at a time) */
	union {
		struct mlcd_request_write_data req_write;
		struct mlcd_request_get_media_info req_minfo;
	} sc_req;
#define sc_reqw	sc_req.req_write
#define sc_reqm	sc_req.req_minfo

	/* pending buffers */
	struct bufq_state sc_q;

	/* current I/O access */
	struct buf	*sc_bp;
	int		sc_retry;
#define MLCD_MAXRETRY	10
};

/*
 * minor number layout (mlcddetach() depends on this layout):
 *
 * 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * |---------------------------------| |---------------------|
 *                unit                          part
 */
#define MLCD_PART(dev)		(minor(dev) & 0xff)
#define MLCD_UNIT(dev)		(minor(dev) >> 8)
#define MLCD_MINOR(unit, part)	(((unit) << 8) | (part))

static int	mlcdmatch __P((struct device *, struct cfdata *, void *));
static void	mlcdattach __P((struct device *, struct device *, void *));
static int	mlcddetach __P((struct device *, int));
static void	mlcd_intr __P((void *, struct maple_response *, int, int));
static void	mlcd_printerror __P((const char *, u_int32_t));
static void	mlcdstart __P((struct mlcd_softc *));
static void	mlcdstart_bp __P((struct mlcd_softc *, struct buf *bp));
static void	mlcddone __P((struct mlcd_softc *));

dev_type_open(mlcdopen);
dev_type_close(mlcdclose);
dev_type_write(mlcdwrite);
dev_type_ioctl(mlcdioctl);
dev_type_strategy(mlcdstrategy);

const struct cdevsw mlcd_cdevsw = {
	mlcdopen, mlcdclose, noread, mlcdwrite, mlcdioctl,
	nostop, notty, nopoll, nommap, nokqfilter
};

CFATTACH_DECL(mlcd, sizeof(struct mlcd_softc),
    mlcdmatch, mlcdattach, mlcddetach, NULL);

extern struct cfdriver mlcd_cd;

/* initial image "NetBSD dreamcast" */
static const char initimg48x32[192] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0xac, 0xe5, 0x56, 0x70, 0xb8,
	0x14, 0x12, 0x15, 0x49, 0x08, 0xa4, 0x13, 0x1c, 0x15, 0x4e, 0x78, 0xa4,
	0x10, 0x90, 0x15, 0x48, 0x49, 0xa4, 0x7b, 0x0c, 0xe3, 0xc6, 0x36, 0xb8,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0xcf, 0x9f, 0xb8, 0x79, 0x8e, 0x19, 0x99, 0xb3, 0x6c, 0x8d, 0x8c,
	0x31, 0xb1, 0x63, 0x4c, 0x0d, 0x8c, 0x31, 0xb0, 0x66, 0x18, 0x3b, 0x58,
	0x63, 0x18, 0x66, 0x19, 0xdb, 0x58, 0x63, 0x0c, 0x3e, 0x7d, 0x93, 0x58,
	0x63, 0x06, 0x46, 0x30, 0xe3, 0x78, 0x66, 0x66, 0xcc, 0x30, 0x06, 0x30,
	0x36, 0x64, 0xcc, 0x00, 0x06, 0x30, 0x0f, 0x38, 0x7e, 0x00, 0x0e, 0x38,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int
mlcdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct maple_attach_args *ma = aux;

	return (ma->ma_function == MAPLE_FN_LCD ? MAPLE_MATCH_FUNC : 0);
}

static void
mlcdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mlcd_softc *sc = (void *) self;
	struct maple_attach_args *ma = aux;
	int i;
	union {
		u_int32_t v;
		struct mlcd_funcdef s;
	} funcdef;

	sc->sc_parent = parent;
	sc->sc_unit = ma->ma_unit;

	funcdef.v = maple_get_function_data(ma->ma_devinfo, MAPLE_FN_LCD);
	printf(": LCD display\n");
	printf("%s: %d LCD, %d bytes/block, ",
	    sc->sc_dev.dv_xname,
	    sc->sc_npt = funcdef.s.pt + 1,
	    sc->sc_bsize = (funcdef.s.bb + 1) << 5);
	if ((sc->sc_wacc = funcdef.s.wa) == 0)
		printf("no ");
	else
		printf("%d acc/", sc->sc_wacc);
	printf("write, %s, normally %s\n",
	    funcdef.s.hv ? "vertical" : "horizontal",
	    funcdef.s.bw ? "black" : "white");

	/*
	 * start init sequence
	 */
	sc->sc_stat = MLCD_INIT;
	bufq_alloc(&sc->sc_q, BUFQ_DISKSORT|BUFQ_SORT_RAWBLOCK);

	/* check consistency */
	if (sc->sc_wacc != 0) {
		sc->sc_waccsz = sc->sc_bsize / sc->sc_wacc;
		if (sc->sc_bsize != sc->sc_waccsz * sc->sc_wacc) {
			printf("%s: write access isn't equally divided\n",
			    sc->sc_dev.dv_xname);
			sc->sc_wacc = 0;	/* no write */
		} else if (sc->sc_waccsz > MLCD_MAXACCSIZE) {
			printf("%s: write access size is too large\n",
			    sc->sc_dev.dv_xname);
			sc->sc_wacc = 0;	/* no write */
		}
	}
	if (sc->sc_wacc == 0) {
		printf("%s: device doesn't support write\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* per-part structure */
	sc->sc_pt = malloc(sizeof(struct mlcd_pt) * sc->sc_npt, M_DEVBUF,
	    M_WAITOK|M_ZERO);

	for (i = 0; i < sc->sc_npt; i++) {
		sprintf(sc->sc_pt[i].pt_name, "%s.%d", sc->sc_dev.dv_xname, i);
	}

	maple_set_callback(parent, sc->sc_unit, MAPLE_FN_LCD,
	    mlcd_intr, sc);

	/*
	 * get size (start from partition 0)
	 */
	sc->sc_reqm.func_code = htonl(MAPLE_FUNC(MAPLE_FN_LCD));
	sc->sc_reqm.pt = 0;
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD,
	    MAPLE_COMMAND_GETMINFO, sizeof sc->sc_reqm / 4, &sc->sc_reqm, 0);
}

static int
mlcddetach(self, flags)
	struct device *self;
	int flags;
{
	struct mlcd_softc *sc = (struct mlcd_softc *) self;
	struct buf *bp;
	int minor_l, minor_h;

	sc->sc_stat = MLCD_DETACH;	/* just in case */

	/*
	 * kill pending I/O
	 */
	if ((bp = sc->sc_bp) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	while ((bp = BUFQ_GET(&sc->sc_q)) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	bufq_free(&sc->sc_q);

	/*
	 * revoke vnodes
	 */
	minor_l = MLCD_MINOR(self->dv_unit, 0);
	minor_h = MLCD_MINOR(self->dv_unit, sc->sc_npt - 1);
	vdevgone(cdevsw_lookup_major(&mlcd_cdevsw), minor_l, minor_h, VCHR);

	/*
	 * free per-partition structure
	 */
	if (sc->sc_pt)
		free(sc->sc_pt, M_DEVBUF);

	return 0;
}

/*
 * called back from maple bus driver
 */
static void
mlcd_intr(dev, response, sz, flags)
	void *dev;
	struct maple_response *response;
	int sz, flags;
{
	struct mlcd_softc *sc = dev;
	struct mlcd_response_media_info *rm = (void *) response->data;
	struct buf *bp;
	int part;
	struct mlcd_pt *pt;

	switch (sc->sc_stat) {
	case MLCD_INIT:
		/* checking part geometry */
		part = sc->sc_reqm.pt;
		pt = &sc->sc_pt[part];
		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_DATATRF:
			pt->pt_info = rm->info;
			pt->pt_size = ((pt->pt_info.width + 1) *
			    (pt->pt_info.height + 1) + 7) / 8;
			pt->pt_nblk = pt->pt_size / sc->sc_bsize;
			printf("%s: %dx%d display, %d bytes\n",
			    pt->pt_name,
			    pt->pt_info.width + 1, pt->pt_info.height + 1,
			    pt->pt_size);

			/* this partition is active */
			pt->pt_flags = MLCD_PT_OK;

			break;
		default:
			printf("%s: init: unexpected response %#x, sz %d\n",
			    pt->pt_name, ntohl(response->response_code), sz);
			break;
		}
		if (++part == sc->sc_npt) {
			/* init done */

			/* XXX initial image for Visual Memory */
			if (sc->sc_pt[0].pt_size == sizeof initimg48x32 &&
			    sc->sc_waccsz == sizeof initimg48x32 &&
			    sc->sc_wacc == 1) {
				sc->sc_stat = MLCD_INIT2;
				sc->sc_reqw.func_code =
				    htonl(MAPLE_FUNC(MAPLE_FN_LCD));
				sc->sc_reqw.pt = 0;	/* part 0 */
				sc->sc_reqw.block = 0;
				sc->sc_reqw.phase = 0;
				bcopy(initimg48x32, sc->sc_reqw.data,
				    sizeof initimg48x32);
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_LCD, MAPLE_COMMAND_BWRITE,
				    MLCD_SIZE_REQW(sc) / 4, &sc->sc_reqw, 0);
			} else
				sc->sc_stat = MLCD_IDLE;	/* init done */
		} else {
			sc->sc_reqm.pt = part;
			maple_command(sc->sc_parent, sc->sc_unit,
			    MAPLE_FN_LCD, MAPLE_COMMAND_GETMINFO,
			    sizeof sc->sc_reqm / 4, &sc->sc_reqm, 0);
		}
		break;

	case MLCD_INIT2:
		sc->sc_stat = MLCD_IDLE;	/* init done */
		break;

	case MLCD_WRITE:
		bp = sc->sc_bp;

		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_OK:			/* write done */
			if (++sc->sc_reqw.phase == sc->sc_wacc) {
				/* all phase done */
				mlcddone(sc);
			} else {
				/* go next phase */
				bcopy(bp->b_data
					+ sc->sc_waccsz * sc->sc_reqw.phase,
				    sc->sc_reqw.data, sc->sc_waccsz);
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_LCD, MAPLE_COMMAND_BWRITE,
				    MLCD_SIZE_REQW(sc) / 4, &sc->sc_reqw, 0);
			}
			break;
		case MAPLE_RESPONSE_LCDERR:
			mlcd_printerror(sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    rm->func_code /* XXX */);
			mlcdstart_bp(sc, bp);		/* retry */
			break;
		default:
			printf("%s: write: unexpected response %#x, %#x, sz %d\n",
			    sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    ntohl(response->response_code),
			    ntohl(rm->func_code), sz);
			mlcdstart_bp(sc, bp);		/* retry */
			break;
		}
		break;

	default:
		break;
	}
}

static void
mlcd_printerror(head, code)
	const char *head;
	u_int32_t code;
{

	printf("%s:", head);
	NTOHL(code);
	if (code & 1)
		printf(" PT error");
	if (code & 2)
		printf(" Phase error");
	if (code & 4)
		printf(" Block error");
	if (code & 010)
		printf(" Write error");
	if (code & 020)
		printf(" Length error");
	if (code & ~037)
		printf(" Unknown error %#x", code & ~037);
	printf("\n");
}

int
mlcdopen(dev, flags, devtype, p)
	dev_t dev;
	int flags, devtype;
	struct proc *p;
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	if ((sc = device_lookup(&mlcd_cd, unit)) == NULL
	    || sc->sc_stat == MLCD_INIT
	    || sc->sc_stat == MLCD_INIT2
	    || part >= sc->sc_npt || (pt = &sc->sc_pt[part])->pt_flags == 0)
		return ENXIO;

	if (pt->pt_flags & MLCD_PT_OPEN)
		return EBUSY;

	pt->pt_flags |= MLCD_PT_OPEN;

	return 0;
}

int
mlcdclose(dev, flags, devtype, p)
	dev_t dev;
	int flags, devtype;
	struct proc *p;
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = mlcd_cd.cd_devs[unit];
	pt = &sc->sc_pt[part];

	pt->pt_flags &= ~MLCD_PT_OPEN;

	return 0;
}

void
mlcdstrategy(bp)
	struct buf *bp;
{
	int dev, unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;
	daddr_t off, nblk, cnt;

	dev = bp->b_dev;
	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = mlcd_cd.cd_devs[unit];
	pt = &sc->sc_pt[part];

#if 0
	printf("%s: mlcdstrategy: blkno %d, count %ld\n",
	    pt->pt_name, bp->b_blkno, bp->b_bcount);
#endif

	if (bp->b_flags & B_READ)
		goto inval;			/* no read */

	cnt = howmany(bp->b_bcount, sc->sc_bsize);
	if (cnt == 0)
		goto done;	/* no work */

	/* XXX We have set the transfer is only one block in mlcd_minphys(). */
	KASSERT(cnt == 1);

	if (bp->b_blkno & ~(~(daddr_t)0 >> (DEV_BSHIFT + 1 /* sign bit */))
	    /*|| (bp->b_bcount % sc->sc_bsize) != 0*/)
		goto inval;

	off = bp->b_blkno * DEV_BSIZE / sc->sc_bsize;
	nblk = pt->pt_nblk;

	/* deal with the EOF condition */
	if (off + cnt > nblk) {
		if (off >= nblk) {
			if (off == nblk) {
				bp->b_resid = bp->b_bcount;
				goto done;
			}
			goto inval;
		}
		cnt = nblk - off;
		bp->b_resid = bp->b_bcount - (cnt * sc->sc_bsize);
	}

	bp->b_rawblkno = off;

	/* queue this transfer */
	BUFQ_PUT(&sc->sc_q, bp);

	if (sc->sc_stat == MLCD_IDLE)
		mlcdstart(sc);

	return;

inval:	bp->b_error = EINVAL;
	bp->b_flags |= B_ERROR;
done:	biodone(bp);
}

/*
 * start I/O operations
 */
static void
mlcdstart(sc)
	struct mlcd_softc *sc;
{
	struct buf *bp;

	if ((bp = BUFQ_GET(&sc->sc_q)) == NULL) {
		sc->sc_stat = MLCD_IDLE;
		maple_enable_unit_ping(sc->sc_parent, sc->sc_unit,
		    MAPLE_FN_LCD, 1);
		return;
	}

	sc->sc_retry = 0;
	mlcdstart_bp(sc, bp);
}

/*
 * start/retry a specified I/O operation
 */
static void
mlcdstart_bp(sc, bp)
	struct mlcd_softc *sc;
	struct buf *bp;
{
	int part;
	struct mlcd_pt *pt;

	part = MLCD_PART(bp->b_dev);
	pt = &sc->sc_pt[part];

	/* handle retry */
	if (sc->sc_retry++ > MLCD_MAXRETRY) {
		/* retry count exceeded */
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		mlcddone(sc);
		return;
	}

	sc->sc_bp = bp;
	/* sc->sc_cnt = cnt; */	/* cnt is always 1 */

	/*
	 * I/O access will fail if the removal detection (by maple driver)
	 * occurs before finishing the I/O, so disable it.
	 * We are sending commands, and the removal detection is still alive.
	 */
	maple_enable_unit_ping(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD, 0);

	/*
	 * Start the first phase (phase# = 0).
	 */
	KASSERT((bp->b_flags & B_READ) == 0);
	/* write */
	sc->sc_stat = MLCD_WRITE;
	sc->sc_reqw.func_code = htonl(MAPLE_FUNC(MAPLE_FN_LCD));
	sc->sc_reqw.pt = part;
	sc->sc_reqw.block = htons(bp->b_rawblkno);
	sc->sc_reqw.phase = 0;		/* first phase */
	bcopy(bp->b_data /* + sc->sc_waccsz * phase */,
	    sc->sc_reqw.data, sc->sc_waccsz);
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD,
	    MAPLE_COMMAND_BWRITE, MLCD_SIZE_REQW(sc) / 4, &sc->sc_reqw, 0);
}

static void
mlcddone(sc)
	struct mlcd_softc *sc;
{
	struct buf *bp;

	/* terminate current transfer */
	bp = sc->sc_bp;
	KASSERT(bp);
	sc->sc_bp = NULL;
	biodone(bp);

	/* go next transfer */
	mlcdstart(sc);
}

static void mlcd_minphys __P((struct buf *));

static void
mlcd_minphys(bp)
	struct buf *bp;
{
	int unit;
	struct mlcd_softc *sc;

	unit = MLCD_UNIT(bp->b_dev);
	sc = mlcd_cd.cd_devs[unit];

	/* XXX one block only */
	if (bp->b_bcount > sc->sc_bsize)
		bp->b_bcount = sc->sc_bsize;
}

int
mlcdwrite(dev, uio, flags)
	dev_t	dev;
	struct	uio *uio;
	int	flags;
{

	return (physio(mlcdstrategy, NULL, dev, B_WRITE, mlcd_minphys, uio));
}

int
mlcdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = mlcd_cd.cd_devs[unit];
	pt = &sc->sc_pt[part];

	switch (cmd) {

	default:
		/* generic maple ioctl */
		return maple_unit_ioctl(sc->sc_parent, sc->sc_unit, cmd, data,
		    flag, p);
	}

	return 0;
}
