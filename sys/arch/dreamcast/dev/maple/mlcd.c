/*	$NetBSD: mlcd.c,v 1.13 2010/10/17 14:13:44 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mlcd.c,v 1.13 2010/10/17 14:13:44 tsutsui Exp $");

#include <sys/param.h>
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
	uint32_t	func_code;
	uint8_t		pt;
	uint8_t		phase;		/* 0, 1, 2, 3: for each 128 byte */
	uint16_t	block;
	uint8_t		data[MLCD_MAXACCSIZE];
};
#define MLCD_SIZE_REQW(sc)	((sc)->sc_waccsz + 8)

struct mlcd_request_get_media_info {
	uint32_t	func_code;
	uint32_t	pt;		/* pt (1 byte) and unused 3 bytes */
};

struct mlcd_media_info {
	uint8_t		width;		/* width - 1 */
	uint8_t		height;		/* height - 1 */
	uint8_t		rsvd[2];	/* ? 0x10 0x02 */
};

struct mlcd_response_media_info {
	uint32_t	func_code;	/* function code (big endian) */
	struct mlcd_media_info info;
};

struct mlcd_buf {
	SIMPLEQ_ENTRY(mlcd_buf)	lb_q;
	int		lb_error;
	int		lb_partno;
	int		lb_blkno;
	uint32_t	lb_data[1];	/* variable length */
};
#define MLCD_BUF_SZ(sc) (offsetof(struct mlcd_buf, lb_data) + (sc)->sc_bsize)

struct mlcd_softc {
	device_t sc_dev;

	device_t sc_parent;
	struct maple_unit *sc_unit;
	int		sc_direction;
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
		int		pt_nblk;	/* partition size in block */

		char		pt_name[16 /* see device.h */ + 4 /* ".255" */];
	} *sc_pt;

	/* write request buffer (only one is used at a time) */
	union {
		struct mlcd_request_write_data req_write;
		struct mlcd_request_get_media_info req_minfo;
	} sc_req;
#define sc_reqw	sc_req.req_write
#define sc_reqm	sc_req.req_minfo

	/* pending buffers */
	SIMPLEQ_HEAD(mlcd_bufq, mlcd_buf) sc_q;

	/* current I/O access */
	struct mlcd_buf	*sc_bp;
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

static int	mlcdmatch(device_t, cfdata_t, void *);
static void	mlcdattach(device_t, device_t, void *);
static int	mlcddetach(device_t, int);
static void	mlcd_intr(void *, struct maple_response *, int, int);
static void	mlcd_printerror(const char *, uint32_t);
static struct mlcd_buf *mlcd_buf_alloc(int /*dev*/, int /*flags*/);
static void	mlcd_buf_free(struct mlcd_buf *);
static inline uint32_t reverse_32(uint32_t);
static void	mlcd_rotate_bitmap(void *, size_t);
static void	mlcdstart(struct mlcd_softc *);
static void	mlcdstart_bp(struct mlcd_softc *);
static void	mlcddone(struct mlcd_softc *);

dev_type_open(mlcdopen);
dev_type_close(mlcdclose);
dev_type_write(mlcdwrite);
dev_type_ioctl(mlcdioctl);

const struct cdevsw mlcd_cdevsw = {
	mlcdopen, mlcdclose, noread, mlcdwrite, mlcdioctl,
	nostop, notty, nopoll, nommap, nokqfilter
};

CFATTACH_DECL_NEW(mlcd, sizeof(struct mlcd_softc),
    mlcdmatch, mlcdattach, mlcddetach, NULL);

extern struct cfdriver mlcd_cd;

/* initial image "NetBSD dreamcast" */
static const char initimg48x32[192] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1c, 0x70, 0x00, 0x7e, 0x1c, 0xf0, 0x0c, 0x60, 0x00, 0x33, 0x26, 0x6c,
	0x0c, 0x60, 0x0c, 0x33, 0x66, 0x66, 0x1e, 0xc7, 0x0c, 0x62, 0x60, 0xc6,
	0x1a, 0xc9, 0xbe, 0x7c, 0x30, 0xc6, 0x1a, 0xdb, 0x98, 0x66, 0x18, 0xc6,
	0x1a, 0xdc, 0x18, 0x66, 0x0d, 0x8c, 0x31, 0xb0, 0x32, 0xc6, 0x8d, 0x8c,
	0x31, 0xb1, 0x36, 0xcd, 0x99, 0x98, 0x71, 0x9e, 0x1d, 0xf9, 0xf3, 0xe0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x08,
	0x1d, 0x6c, 0x63, 0xc7, 0x30, 0xde, 0x25, 0x92, 0x12, 0xa8, 0x09, 0x08,
	0x25, 0x1e, 0x72, 0xa8, 0x38, 0xc8, 0x25, 0x10, 0x92, 0xa8, 0x48, 0x28,
	0x1d, 0x0e, 0x6a, 0xa7, 0x35, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* ARGSUSED */
static int
mlcdmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return (ma->ma_function == MAPLE_FN_LCD ? MAPLE_MATCH_FUNC : 0);
}

static void
mlcdattach(device_t parent, device_t self, void *aux)
{
	struct mlcd_softc *sc = device_private(self);
	struct maple_attach_args *ma = aux;
	int i;
	union {
		uint32_t v;
		struct mlcd_funcdef s;
	} funcdef;

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_unit = ma->ma_unit;
	sc->sc_direction = ma->ma_basedevinfo->di_connector_direction;

	funcdef.v = maple_get_function_data(ma->ma_devinfo, MAPLE_FN_LCD);
	printf(": LCD display\n");
	printf("%s: %d LCD, %d bytes/block, ",
	    device_xname(self),
	    sc->sc_npt = funcdef.s.pt + 1,
	    sc->sc_bsize = (funcdef.s.bb + 1) << 5);
	if ((sc->sc_wacc = funcdef.s.wa) == 0)
		printf("no ");
	else
		printf("%d acc/", sc->sc_wacc);
	printf("write, %s, norm %s%s\n",
	    funcdef.s.hv ? "vert" : "horiz",
	    funcdef.s.bw ? "black" : "white",
	    sc->sc_direction == MAPLE_CONN_TOP ? ", upside-down" : "");

	/*
	 * start init sequence
	 */
	sc->sc_stat = MLCD_INIT;
	SIMPLEQ_INIT(&sc->sc_q);

	/* check consistency */
	if (sc->sc_wacc != 0) {
		sc->sc_waccsz = sc->sc_bsize / sc->sc_wacc;
		if (sc->sc_bsize != sc->sc_waccsz * sc->sc_wacc) {
			printf("%s: write access isn't equally divided\n",
			    device_xname(self));
			sc->sc_wacc = 0;	/* no write */
		} else if (sc->sc_waccsz > MLCD_MAXACCSIZE) {
			printf("%s: write access size is too large\n",
			    device_xname(self));
			sc->sc_wacc = 0;	/* no write */
		}
	}
	if (sc->sc_wacc == 0) {
		printf("%s: device doesn't support write\n",
		    device_xname(self));
		return;
	}

	/* per-part structure */
	sc->sc_pt = malloc(sizeof(struct mlcd_pt) * sc->sc_npt, M_DEVBUF,
	    M_WAITOK|M_ZERO);

	for (i = 0; i < sc->sc_npt; i++) {
		sprintf(sc->sc_pt[i].pt_name, "%s.%d", device_xname(self), i);
	}

	maple_set_callback(parent, sc->sc_unit, MAPLE_FN_LCD,
	    mlcd_intr, sc);

	/*
	 * get size (start from partition 0)
	 */
	sc->sc_reqm.func_code = htobe32(MAPLE_FUNC(MAPLE_FN_LCD));
	sc->sc_reqm.pt = 0;
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD,
	    MAPLE_COMMAND_GETMINFO, sizeof sc->sc_reqm / 4, &sc->sc_reqm, 0);
}

/* ARGSUSED1 */
static int
mlcddetach(device_t self, int flags)
{
	struct mlcd_softc *sc = device_private(self);
	struct mlcd_buf *bp;
	int minor_l, minor_h;

	sc->sc_stat = MLCD_DETACH;	/* just in case */

	/*
	 * kill pending I/O
	 */
	if ((bp = sc->sc_bp) != NULL) {
		bp->lb_error = EIO;
		wakeup(bp);
	}
	while ((bp = SIMPLEQ_FIRST(&sc->sc_q)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_q, lb_q);
		bp->lb_error = EIO;
		wakeup(bp);
	}

	/*
	 * revoke vnodes
	 */
	minor_l = MLCD_MINOR(device_unit(self), 0);
	minor_h = MLCD_MINOR(device_unit(self), sc->sc_npt - 1);
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
/* ARGSUSED3 */
static void
mlcd_intr(void *arg, struct maple_response *response, int sz, int flags)
{
	struct mlcd_softc *sc = arg;
	struct mlcd_response_media_info *rm = (void *) response->data;
	struct mlcd_buf *bp;
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
			    pt->pt_name, be32toh(response->response_code), sz);
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
				    htobe32(MAPLE_FUNC(MAPLE_FN_LCD));
				sc->sc_reqw.pt = 0;	/* part 0 */
				sc->sc_reqw.block = 0;
				sc->sc_reqw.phase = 0;
				memcpy(sc->sc_reqw.data, initimg48x32, 
				    sizeof initimg48x32);
				if (sc->sc_direction == MAPLE_CONN_TOP) {
					/* the LCD is upside-down */
					mlcd_rotate_bitmap(sc->sc_reqw.data,
					    sizeof initimg48x32);
				}
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
				memcpy(sc->sc_reqw.data,
				    (char *)bp->lb_data +
				    sc->sc_waccsz * sc->sc_reqw.phase,
				    sc->sc_waccsz);
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_LCD, MAPLE_COMMAND_BWRITE,
				    MLCD_SIZE_REQW(sc) / 4, &sc->sc_reqw, 0);
			}
			break;
		case MAPLE_RESPONSE_LCDERR:
			mlcd_printerror(sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    rm->func_code /* XXX */);
			mlcdstart_bp(sc);		/* retry */
			break;
		default:
			printf("%s: write: unexpected response %#x, %#x, sz %d\n",
			    sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    be32toh(response->response_code),
			    be32toh(rm->func_code), sz);
			mlcdstart_bp(sc);		/* retry */
			break;
		}
		break;

	default:
		break;
	}
}

static void
mlcd_printerror(const char *head, uint32_t code)
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

/* ARGSUSED */
int
mlcdopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	if ((sc = device_lookup_private(&mlcd_cd, unit)) == NULL
	    || sc->sc_stat == MLCD_INIT
	    || sc->sc_stat == MLCD_INIT2
	    || part >= sc->sc_npt || (pt = &sc->sc_pt[part])->pt_flags == 0)
		return ENXIO;

	if (pt->pt_flags & MLCD_PT_OPEN)
		return EBUSY;

	pt->pt_flags |= MLCD_PT_OPEN;

	return 0;
}

/* ARGSUSED */
int
mlcdclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = device_lookup_private(&mlcd_cd, unit);
	pt = &sc->sc_pt[part];

	pt->pt_flags &= ~MLCD_PT_OPEN;

	return 0;
}

/*
 * start I/O operations
 */
static void
mlcdstart(struct mlcd_softc *sc)
{
	struct mlcd_buf *bp;

	if ((bp = SIMPLEQ_FIRST(&sc->sc_q)) == NULL) {
		sc->sc_stat = MLCD_IDLE;
		maple_enable_unit_ping(sc->sc_parent, sc->sc_unit,
		    MAPLE_FN_LCD, 1);
		return;
	}

	SIMPLEQ_REMOVE_HEAD(&sc->sc_q, lb_q);

	sc->sc_bp = bp;
	sc->sc_retry = 0;
	mlcdstart_bp(sc);
}

/*
 * start/retry a specified I/O operation
 */
static void
mlcdstart_bp(struct mlcd_softc *sc)
{
	struct mlcd_buf *bp;
	struct mlcd_pt *pt;

	bp = sc->sc_bp;
	pt = &sc->sc_pt[bp->lb_partno];

	/* handle retry */
	if (sc->sc_retry++ > MLCD_MAXRETRY) {
		/* retry count exceeded */
		bp->lb_error = EIO;
		mlcddone(sc);
		return;
	}

	/*
	 * I/O access will fail if the removal detection (by maple driver)
	 * occurs before finishing the I/O, so disable it.
	 * We are sending commands, and the removal detection is still alive.
	 */
	maple_enable_unit_ping(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD, 0);

	/*
	 * Start the first phase (phase# = 0).
	 */
	/* write */
	sc->sc_stat = MLCD_WRITE;
	sc->sc_reqw.func_code = htobe32(MAPLE_FUNC(MAPLE_FN_LCD));
	sc->sc_reqw.pt = bp->lb_partno;
	sc->sc_reqw.block = htobe16(bp->lb_blkno);
	sc->sc_reqw.phase = 0;		/* first phase */
	memcpy(sc->sc_reqw.data,
	    (char *) bp->lb_data /* + sc->sc_waccsz * phase */, sc->sc_waccsz);
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_LCD,
	    MAPLE_COMMAND_BWRITE, MLCD_SIZE_REQW(sc) / 4, &sc->sc_reqw, 0);
}

static void
mlcddone(struct mlcd_softc *sc)
{
	struct mlcd_buf *bp;

	/* terminate current transfer */
	bp = sc->sc_bp;
	KASSERT(bp);
	sc->sc_bp = NULL;
	wakeup(bp);

	/* go next transfer */
	mlcdstart(sc);
}

/*
 * allocate a buffer for one block
 *
 * return NULL if
 *	[flags == M_NOWAIT] out of buffer space
 *	[flags == M_WAITOK] device detach detected
 */
static struct mlcd_buf *
mlcd_buf_alloc(int dev, int flags)
{
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;
	int unit, part;
	struct mlcd_buf *bp;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = device_lookup_private(&mlcd_cd, unit);
	KASSERT(sc);
	pt = &sc->sc_pt[part];
	KASSERT(pt);

	if ((bp = malloc(MLCD_BUF_SZ(sc), M_DEVBUF, flags)) == NULL)
		return bp;

	/*
	 * malloc() may sleep, and the device may be detached during sleep.
	 * XXX this check is not complete.
	 */
	if (sc != device_lookup_private(&mlcd_cd, unit)
	    || sc->sc_stat == MLCD_INIT
	    || sc->sc_stat == MLCD_INIT2
	    || part >= sc->sc_npt || pt != &sc->sc_pt[part]
	    || pt->pt_flags == 0) {
		free(bp, M_DEVBUF);
		return NULL;
	}

	bp->lb_error = 0;

	return bp;
}

static void
mlcd_buf_free(struct mlcd_buf *bp)
{

	free(bp, M_DEVBUF);
}

/* invert order of bits */
static inline uint32_t
reverse_32(uint32_t b)
{
	uint32_t b1;

	/* invert every 8bit */
	b1 = (b & 0x55555555) << 1;  b = (b >> 1) & 0x55555555;  b |= b1;
	b1 = (b & 0x33333333) << 2;  b = (b >> 2) & 0x33333333;  b |= b1;
	b1 = (b & 0x0f0f0f0f) << 4;  b = (b >> 4) & 0x0f0f0f0f;  b |= b1;

	/* invert byte order */
	return bswap32(b);
}

static void
mlcd_rotate_bitmap(void *ptr, size_t size)
{
	uint32_t *p, *q, tmp;

	KDASSERT(size % sizeof(uint32_t) == 0);
	for (p = ptr, q = (void *)((char *)ptr + size); p < q; ) {
		tmp = reverse_32(*p);
		*p++ = reverse_32(*--q);
		*q = tmp;
	}
}

/* ARGSUSED2 */
int
mlcdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;
	struct mlcd_buf *bp;
	int part;
	off_t devsize;
	int error = 0;

	part = MLCD_PART(dev);
	sc = device_lookup_private(&mlcd_cd, MLCD_UNIT(dev));
	pt = &sc->sc_pt[part];

#if 0
	printf("%s: mlcdwrite: offset %ld, size %d\n",
	    pt->pt_name, (long) uio->uio_offset, uio->uio_resid);
#endif

	devsize = pt->pt_nblk * sc->sc_bsize;
	if (uio->uio_offset % sc->sc_bsize || uio->uio_offset > devsize)
		return EINVAL;

	if ((bp = mlcd_buf_alloc(dev, M_WAITOK)) == NULL)
		return EIO;	/* device is detached during allocation */

	bp->lb_partno = part;

	while (uio->uio_offset < devsize
	    && uio->uio_resid >= (size_t) sc->sc_bsize) {
		/* invert block number if upside-down */
		bp->lb_blkno = (sc->sc_direction == MAPLE_CONN_TOP) ?
		    pt->pt_nblk - uio->uio_offset / sc->sc_bsize - 1 :
		    uio->uio_offset / sc->sc_bsize;

		if ((error = uiomove(bp->lb_data, sc->sc_bsize, uio)) != 0)
			break;

		if (sc->sc_direction == MAPLE_CONN_TOP) {
			/* the LCD is upside-down */
			mlcd_rotate_bitmap(bp->lb_data, sc->sc_bsize);
		}

		/* queue this transfer */
		SIMPLEQ_INSERT_TAIL(&sc->sc_q, bp, lb_q);

		if (sc->sc_stat == MLCD_IDLE)
			mlcdstart(sc);

		tsleep(bp, PRIBIO + 1, "mlcdbuf", 0);

		if ((error = bp->lb_error) != 0) {
			uio->uio_resid += sc->sc_bsize;
			break;
		}
	}

	mlcd_buf_free(bp);

	return error;
}

int
mlcdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int unit, part;
	struct mlcd_softc *sc;
	struct mlcd_pt *pt;

	unit = MLCD_UNIT(dev);
	part = MLCD_PART(dev);
	sc = device_lookup_private(&mlcd_cd, unit);
	pt = &sc->sc_pt[part];

	switch (cmd) {

	default:
		/* generic maple ioctl */
		return maple_unit_ioctl(sc->sc_parent, sc->sc_unit, cmd, data,
		    flag, l);
	}

	return 0;
}
