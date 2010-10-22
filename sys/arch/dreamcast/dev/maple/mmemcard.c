/*	$NetBSD: mmemcard.c,v 1.18.4.1 2010/10/22 07:21:12 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mmemcard.c,v 1.18.4.1 2010/10/22 07:21:12 uebayasi Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/conf.h>

#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>

#include "ioconf.h"

#define MMEM_MAXACCSIZE	1012	/* (255*4) - 8  =  253*32 / 8 */

struct mmem_funcdef {	/* XXX assuming little-endian structure packing */
	unsigned unused	: 8,
		 ra	: 4,	/* number of access / read */
		 wa	: 4,	/* number of access / write */
		 bb	: 8,	/* block size / 32 - 1 */
		 pt	: 8;	/* number of partition - 1 */
};

struct mmem_request_read_data {
	uint32_t	func_code;
	uint8_t		pt;
	uint8_t		phase;
	uint16_t	block;
};

struct mmem_response_read_data {
	uint32_t	func_code;	/* function code (big endian) */
	uint32_t	blkno;		/* 512byte block number (big endian) */
	uint8_t		data[MMEM_MAXACCSIZE];
};

struct mmem_request_write_data {
	uint32_t	func_code;
	uint8_t		pt;
	uint8_t		phase;		/* 0, 1, 2, 3: for each 128 byte */
	uint16_t	block;
	uint8_t		data[MMEM_MAXACCSIZE];
};
#define MMEM_SIZE_REQW(sc)	((sc)->sc_waccsz + 8)

struct mmem_request_get_media_info {
	uint32_t	func_code;
	uint32_t	pt;		/* pt (1 byte) and unused 3 bytes */
};

struct mmem_media_info {
	uint16_t	maxblk, minblk;
	uint16_t	infpos;
	uint16_t	fatpos, fatsz;
	uint16_t	dirpos, dirsz;
	uint16_t	icon;
	uint16_t	datasz;
	uint16_t	rsvd[3];
};

struct mmem_response_media_info {
	uint32_t	func_code;	/* function code (big endian) */
	struct mmem_media_info info;
};

struct mmem_softc {
	device_t sc_dev;

	device_t sc_parent;
	struct maple_unit *sc_unit;
	struct maple_devinfo *sc_devinfo;

	enum mmem_stat {
		MMEM_INIT,	/* during initialization */
		MMEM_INIT2,	/* during initialization */
		MMEM_IDLE,	/* init done, not in I/O */
		MMEM_READ,	/* in read operation */
		MMEM_WRITE1,	/* in write operation (read and compare) */
		MMEM_WRITE2,	/* in write operation (write) */
		MMEM_DETACH	/* detaching */
	} sc_stat;

	int		sc_npt;		/* number of partitions */
	int		sc_bsize;	/* block size */
	int		sc_wacc;	/* number of write access per block */
	int		sc_waccsz;	/* size of a write access */
	int		sc_racc;	/* number of read access per block */
	int		sc_raccsz;	/* size of a read access */

	struct mmem_pt {
		int		pt_flags;
#define MMEM_PT_OK	1	/* partition is alive */
		struct disk	pt_dk;		/* disk(9) */
		struct mmem_media_info pt_info;	/* geometry per part */

		char		pt_name[16 /* see device.h */ + 4 /* ".255" */];
	} *sc_pt;

	/* write request buffer (only one is used at a time) */
	union {
		struct mmem_request_read_data req_read;
		struct mmem_request_write_data req_write;
		struct mmem_request_get_media_info req_minfo;
	} sc_req;
#define sc_reqr	sc_req.req_read
#define sc_reqw	sc_req.req_write
#define sc_reqm	sc_req.req_minfo

	/* pending buffers */
	struct bufq_state *sc_q;

	/* current I/O access */
	struct buf	*sc_bp;
	int		sc_cnt;
	char		*sc_iobuf;
	int		sc_retry;
#define MMEM_MAXRETRY	12
};

/*
 * minor number layout (mmemdetach() depends on this layout):
 *
 * 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * |---------------------| |---------------------| |---------|
 *          unit                    part           disklabel partition
 */
#define MMEM_PART(diskunit)	((diskunit) & 0xff)
#define MMEM_UNIT(diskunit)	((diskunit) >> 8)
#define MMEM_DISKMINOR(unit, part, disklabel_partition) \
	DISKMINOR(((unit) << 8) | (part), (disklabel_partition))

static int	mmemmatch(device_t, cfdata_t, void *);
static void	mmemattach(device_t, device_t, void *);
static void	mmem_defaultlabel(struct mmem_softc *, struct mmem_pt *,
		    struct disklabel *);
static int	mmemdetach(device_t, int);
static void	mmem_intr(void *, struct maple_response *, int, int);
static void	mmem_printerror(const char *, int, int, uint32_t);
static void	mmemstart(struct mmem_softc *);
static void	mmemstart_bp(struct mmem_softc *);
static void	mmemstart_write2(struct mmem_softc *);
static void	mmemdone(struct mmem_softc *, struct mmem_pt *, int);

dev_type_open(mmemopen);
dev_type_close(mmemclose);
dev_type_read(mmemread);
dev_type_write(mmemwrite);
dev_type_ioctl(mmemioctl);
dev_type_strategy(mmemstrategy);

const struct bdevsw mmem_bdevsw = {
	mmemopen, mmemclose, mmemstrategy, mmemioctl, nodump,
	nosize, D_DISK
};

const struct cdevsw mmem_cdevsw = {
	mmemopen, mmemclose, mmemread, mmemwrite, mmemioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

CFATTACH_DECL_NEW(mmem, sizeof(struct mmem_softc),
    mmemmatch, mmemattach, mmemdetach, NULL);

struct dkdriver mmemdkdriver = { mmemstrategy };

static int
mmemmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return ma->ma_function == MAPLE_FN_MEMCARD ? MAPLE_MATCH_FUNC : 0;
}

static void
mmemattach(device_t parent, device_t self, void *aux)
{
	struct mmem_softc *sc = device_private(self);
	struct maple_attach_args *ma = aux;
	int i;
	union {
		uint32_t v;
		struct mmem_funcdef s;
	} funcdef;

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_unit = ma->ma_unit;
	sc->sc_devinfo = ma->ma_devinfo;

	funcdef.v = maple_get_function_data(ma->ma_devinfo, MAPLE_FN_MEMCARD);
	printf(": Memory card\n");
	printf("%s: %d part, %d bytes/block, ",
	    device_xname(self),
	    sc->sc_npt = funcdef.s.pt + 1,
	    sc->sc_bsize = (funcdef.s.bb + 1)  << 5);
	if ((sc->sc_wacc = funcdef.s.wa) == 0)
		printf("no write, ");
	else
		printf("%d acc/write, ", sc->sc_wacc);
	if ((sc->sc_racc = funcdef.s.ra) == 0)
		printf("no read\n");
	else
		printf("%d acc/read\n", sc->sc_racc);

	/*
	 * start init sequence
	 */
	sc->sc_stat = MMEM_INIT;
	bufq_alloc(&sc->sc_q, "disksort", BUFQ_SORT_RAWBLOCK);

	/* check consistency */
	if (sc->sc_wacc != 0) {
		sc->sc_waccsz = sc->sc_bsize / sc->sc_wacc;
		if (sc->sc_bsize != sc->sc_waccsz * sc->sc_wacc) {
			printf("%s: write access isn't equally divided\n",
			    device_xname(self));
			sc->sc_wacc = 0;	/* no write */
		} else if (sc->sc_waccsz > MMEM_MAXACCSIZE) {
			printf("%s: write access size is too large\n",
			    device_xname(self));
			sc->sc_wacc = 0;	/* no write */
		}
	}
	if (sc->sc_racc != 0) {
		sc->sc_raccsz = sc->sc_bsize / sc->sc_racc;
		if (sc->sc_bsize != sc->sc_raccsz * sc->sc_racc) {
			printf("%s: read access isn't equally divided\n",
			    device_xname(self));
			sc->sc_racc = 0;	/* no read */
		} else if (sc->sc_raccsz > MMEM_MAXACCSIZE) {
			printf("%s: read access size is too large\n",
			    device_xname(self));
			sc->sc_racc = 0;	/* no read */
		}
	}
	if (sc->sc_wacc == 0 && sc->sc_racc == 0) {
		printf("%s: device doesn't support read nor write\n",
		    device_xname(self));
		return;
	}

	/* per-part structure */
	sc->sc_pt = malloc(sizeof(struct mmem_pt) * sc->sc_npt, M_DEVBUF,
	    M_WAITOK|M_ZERO);

	for (i = 0; i < sc->sc_npt; i++) {
		sprintf(sc->sc_pt[i].pt_name, "%s.%d", device_xname(self), i);
	}

	maple_set_callback(parent, sc->sc_unit, MAPLE_FN_MEMCARD,
	    mmem_intr, sc);

	/*
	 * get capacity (start from partition 0)
	 */
	sc->sc_reqm.func_code = htobe32(MAPLE_FUNC(MAPLE_FN_MEMCARD));
	sc->sc_reqm.pt = 0;
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_MEMCARD,
	    MAPLE_COMMAND_GETMINFO, sizeof sc->sc_reqm / 4, &sc->sc_reqm, 0);
}

static int
mmemdetach(device_t self, int flags)
{
	struct mmem_softc *sc = device_private(self);
	struct buf *bp;
	int i;
	int minor_l, minor_h;

	sc->sc_stat = MMEM_DETACH;	/* just in case */

	/*
	 * kill pending I/O
	 */
	if ((bp = sc->sc_bp) != NULL) {
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	while ((bp = bufq_get(sc->sc_q)) != NULL) {
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	bufq_free(sc->sc_q);

	/*
	 * revoke vnodes
	 */
#ifdef __HAVE_OLD_DISKLABEL
 #error This code assumes DISKUNIT() is contiguous in minor number.
#endif
	minor_l = MMEM_DISKMINOR(device_unit(self), 0, 0);
	minor_h = MMEM_DISKMINOR(device_unit(self), sc->sc_npt - 1,
	    MAXPARTITIONS - 1);
	vdevgone(bdevsw_lookup_major(&mmem_bdevsw), minor_l, minor_h, VBLK);
	vdevgone(cdevsw_lookup_major(&mmem_cdevsw), minor_l, minor_h, VCHR);

	/*
	 * free per-partition structure
	 */
	if (sc->sc_pt) {
		/*
		 * detach disks
		 */
		for (i = 0; i < sc->sc_npt; i++) {
			if (sc->sc_pt[i].pt_flags & MMEM_PT_OK) {
				disk_detach(&sc->sc_pt[i].pt_dk);
				disk_destroy(&sc->sc_pt[i].pt_dk);
			}
		}
		free(sc->sc_pt, M_DEVBUF);
	}

	return 0;
}

/* fake disklabel */
static void
mmem_defaultlabel(struct mmem_softc *sc, struct mmem_pt *pt,
    struct disklabel *d)
{

	memset(d, 0, sizeof *d);

#if 0
	d->d_type = DTYPE_FLOPPY;		/* XXX? */
#endif
	strncpy(d->d_typename, sc->sc_devinfo->di_product_name,
	    sizeof d->d_typename);
	strcpy(d->d_packname, "fictitious");
	d->d_secsize = sc->sc_bsize;
	d->d_ntracks = 1;			/* XXX */
	d->d_nsectors = d->d_secpercyl = 8;	/* XXX */
	d->d_secperunit = pt->pt_info.maxblk - pt->pt_info.minblk + 1;
	d->d_ncylinders = d->d_secperunit / d->d_secpercyl;
	d->d_rpm = 1;				/* when 4 acc/write */

	d->d_npartitions = RAW_PART + 1;
	d->d_partitions[RAW_PART].p_size = d->d_secperunit;

	d->d_magic = d->d_magic2 = DISKMAGIC;
	d->d_checksum = dkcksum(d);
}

/*
 * called back from maple bus driver
 */
static void
mmem_intr(void *arg, struct maple_response *response, int sz, int flags)
{
	struct mmem_softc *sc = arg;
	struct mmem_response_read_data *r = (void *) response->data;
	struct mmem_response_media_info *rm = (void *) response->data;
	struct buf *bp;
	int part;
	struct mmem_pt *pt;
	char pbuf[9];
	int off;

	switch (sc->sc_stat) {
	case MMEM_INIT:
		/* checking part geometry */
		part = sc->sc_reqm.pt;
		pt = &sc->sc_pt[part];
		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_DATATRF:
			pt->pt_info = rm->info;
			format_bytes(pbuf, sizeof(pbuf),
			    (uint64_t)
				((pt->pt_info.maxblk - pt->pt_info.minblk + 1)
				 * sc->sc_bsize));
			printf("%s: %s, blk %d %d, inf %d, fat %d %d, dir %d %d, icon %d, data %d\n",
			    pt->pt_name,
			    pbuf,
			    pt->pt_info.maxblk, pt->pt_info.minblk,
			    pt->pt_info.infpos,
			    pt->pt_info.fatpos, pt->pt_info.fatsz,
			    pt->pt_info.dirpos, pt->pt_info.dirsz,
			    pt->pt_info.icon,
			    pt->pt_info.datasz);

			disk_init(&pt->pt_dk, pt->pt_name, &mmemdkdriver);
			disk_attach(&pt->pt_dk);

			mmem_defaultlabel(sc, pt, pt->pt_dk.dk_label);

			/* this partition is active */
			pt->pt_flags = MMEM_PT_OK;

			break;
		default:
			printf("%s: init: unexpected response %#x, sz %d\n",
			    pt->pt_name, be32toh(response->response_code), sz);
			break;
		}
		if (++part == sc->sc_npt) {
#if 1
			/*
			 * XXX Read a block and discard the contents (only to
			 * turn off the access indicator on Visual Memory).
			 */
			pt = &sc->sc_pt[0];
			sc->sc_reqr.func_code =
			    htobe32(MAPLE_FUNC(MAPLE_FN_MEMCARD));
			sc->sc_reqr.pt = 0;
			sc->sc_reqr.block = htobe16(pt->pt_info.minblk);
			sc->sc_reqr.phase = 0;
			maple_command(sc->sc_parent, sc->sc_unit,
			    MAPLE_FN_MEMCARD, MAPLE_COMMAND_BREAD,
			    sizeof sc->sc_reqr / 4, &sc->sc_reqr, 0);
			sc->sc_stat = MMEM_INIT2;
#else
			sc->sc_stat = MMEM_IDLE;	/* init done */
#endif
		} else {
			sc->sc_reqm.pt = part;
			maple_command(sc->sc_parent, sc->sc_unit,
			    MAPLE_FN_MEMCARD, MAPLE_COMMAND_GETMINFO,
			    sizeof sc->sc_reqm / 4, &sc->sc_reqm, 0);
		}
		break;

	case MMEM_INIT2:
		/* XXX just discard */
		sc->sc_stat = MMEM_IDLE;	/* init done */
		break;

	case MMEM_READ:
		bp = sc->sc_bp;

		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_DATATRF:		/* read done */
			off = sc->sc_raccsz * sc->sc_reqr.phase;
			memcpy(sc->sc_iobuf + off, r->data + off,
			    sc->sc_raccsz);

			if (++sc->sc_reqr.phase == sc->sc_racc) {
				/* all phase done */
				pt = &sc->sc_pt[sc->sc_reqr.pt];
				mmemdone(sc, pt, 0);
			} else {
				/* go next phase */
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_MEMCARD, MAPLE_COMMAND_BREAD,
				    sizeof sc->sc_reqr / 4, &sc->sc_reqr, 0);
			}
			break;
		case MAPLE_RESPONSE_FILEERR:
			mmem_printerror(sc->sc_pt[sc->sc_reqr.pt].pt_name,
			    1, bp->b_rawblkno,
			    r->func_code /* XXX */);
			mmemstart_bp(sc);		/* retry */
			break;
		default:
			printf("%s: read: unexpected response %#x %#x, sz %d\n",
			    sc->sc_pt[sc->sc_reqr.pt].pt_name,
			    be32toh(response->response_code),
			    be32toh(r->func_code), sz);
			mmemstart_bp(sc);		/* retry */
			break;
		}
		break;

	case MMEM_WRITE1:	/* read before write / verify after write */
		bp = sc->sc_bp;

		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_DATATRF:		/* read done */
			off = sc->sc_raccsz * sc->sc_reqr.phase;
			if (memcmp(r->data + off, sc->sc_iobuf + off,
			    sc->sc_raccsz)) {
				/*
				 * data differ, start writing
				 */
				mmemstart_write2(sc);
			} else if (++sc->sc_reqr.phase == sc->sc_racc) {
				/*
				 * all phase done and compared equal
				 */
				pt = &sc->sc_pt[sc->sc_reqr.pt];
				mmemdone(sc, pt, 0);
			} else {
				/* go next phase */
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_MEMCARD, MAPLE_COMMAND_BREAD,
				    sizeof sc->sc_reqr / 4, &sc->sc_reqr, 0);
			}
			break;
		case MAPLE_RESPONSE_FILEERR:
			mmem_printerror(sc->sc_pt[sc->sc_reqr.pt].pt_name,
			    1, bp->b_rawblkno,
			    r->func_code /* XXX */);
			mmemstart_write2(sc);	/* start writing */
			break;
		default:
			printf("%s: verify: unexpected response %#x %#x, sz %d\n",
			    sc->sc_pt[sc->sc_reqr.pt].pt_name,
			    be32toh(response->response_code),
			    be32toh(r->func_code), sz);
			mmemstart_write2(sc);	/* start writing */
			break;
		}
		break;

	case MMEM_WRITE2:	/* write */
		bp = sc->sc_bp;

		switch ((maple_response_t) response->response_code) {
		case MAPLE_RESPONSE_OK:			/* write done */
			if (sc->sc_reqw.phase == sc->sc_wacc) {
				/* all phase done */
				mmemstart_bp(sc);	/* start verify */
			} else if (++sc->sc_reqw.phase == sc->sc_wacc) {
				/* check error */
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_MEMCARD, MAPLE_COMMAND_GETLASTERR,
				    2 /* no data */ , &sc->sc_reqw,
				    MAPLE_FLAG_CMD_PERIODIC_TIMING);
			} else {
				/* go next phase */
				memcpy(sc->sc_reqw.data, sc->sc_iobuf +
				    sc->sc_waccsz * sc->sc_reqw.phase,
				    sc->sc_waccsz);
				maple_command(sc->sc_parent, sc->sc_unit,
				    MAPLE_FN_MEMCARD, MAPLE_COMMAND_BWRITE,
				    MMEM_SIZE_REQW(sc) / 4, &sc->sc_reqw,
				    MAPLE_FLAG_CMD_PERIODIC_TIMING);
			}
			break;
		case MAPLE_RESPONSE_FILEERR:
			mmem_printerror(sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    0, bp->b_rawblkno,
			    r->func_code /* XXX */);
			mmemstart_write2(sc);	/* retry writing */
			break;
		default:
			printf("%s: write: unexpected response %#x, %#x, sz %d\n",
			    sc->sc_pt[sc->sc_reqw.pt].pt_name,
			    be32toh(response->response_code),
			    be32toh(r->func_code), sz);
			mmemstart_write2(sc);	/* retry writing */
			break;
		}
		break;

	default:
		break;
	}
}

static void
mmem_printerror(const char *head, int rd, int blk, uint32_t code)
{

	printf("%s: error %sing blk %d:", head, rd? "read" : "writ", blk);
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
	if (code & 040)
		printf(" CRC error");
	if (code & ~077)
		printf(" Unknown error %#x", code & ~077);
	printf("\n");
}

int
mmemopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	int diskunit, unit, part, labelpart;
	struct mmem_softc *sc;
	struct mmem_pt *pt;

	diskunit = DISKUNIT(dev);
	unit = MMEM_UNIT(diskunit);
	part = MMEM_PART(diskunit);
	labelpart = DISKPART(dev);
	if ((sc = device_lookup_private(&mmem_cd, unit)) == NULL
	    || sc->sc_stat == MMEM_INIT
	    || sc->sc_stat == MMEM_INIT2
	    || part >= sc->sc_npt || (pt = &sc->sc_pt[part])->pt_flags == 0)
		return ENXIO;

	switch (devtype) {
	case S_IFCHR:
		pt->pt_dk.dk_copenmask |= (1 << labelpart);
		break;
	case S_IFBLK:
		pt->pt_dk.dk_bopenmask |= (1 << labelpart);
		break;
	}

	return 0;
}

int
mmemclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	int diskunit, unit, part, labelpart;
	struct mmem_softc *sc;
	struct mmem_pt *pt;

	diskunit = DISKUNIT(dev);
	unit = MMEM_UNIT(diskunit);
	part = MMEM_PART(diskunit);
	sc = device_lookup_private(&mmem_cd, unit);
	pt = &sc->sc_pt[part];
	labelpart = DISKPART(dev);

	switch (devtype) {
	case S_IFCHR:
		pt->pt_dk.dk_copenmask &= ~(1 << labelpart);
		break;
	case S_IFBLK:
		pt->pt_dk.dk_bopenmask &= ~(1 << labelpart);
		break;
	}

	return 0;
}

void
mmemstrategy(struct buf *bp)
{
	int diskunit, unit, part, labelpart;
	struct mmem_softc *sc;
	struct mmem_pt *pt;
	daddr_t off, nblk, cnt;

	diskunit = DISKUNIT(bp->b_dev);
	unit = MMEM_UNIT(diskunit);
	part = MMEM_PART(diskunit);
	if ((sc = device_lookup_private(&mmem_cd, unit)) == NULL
	    || sc->sc_stat == MMEM_INIT
	    || sc->sc_stat == MMEM_INIT2
	    || part >= sc->sc_npt || (pt = &sc->sc_pt[part])->pt_flags == 0)
		goto inval;

#if 0
	printf("%s: mmemstrategy: blkno %d, count %ld\n",
	    pt->pt_name, bp->b_blkno, bp->b_bcount);
#endif

	if (bp->b_flags & B_READ) {
		if (sc->sc_racc == 0)
			goto inval;		/* no read */
	} else if (sc->sc_wacc == 0) {
		bp->b_error = EROFS;		/* no write */
		goto done;
	}

	if (bp->b_blkno & ~(~(daddr_t)0 >> (DEV_BSHIFT + 1 /* sign bit */))
	    || (bp->b_bcount % sc->sc_bsize) != 0)
		goto inval;

	cnt = howmany(bp->b_bcount, sc->sc_bsize);
	if (cnt == 0)
		goto done;	/* no work */

	off = bp->b_blkno * DEV_BSIZE / sc->sc_bsize;

	/* offset to disklabel partition */
	labelpart = DISKPART(bp->b_dev);
	if (labelpart == RAW_PART) {
		nblk = pt->pt_info.maxblk - pt->pt_info.minblk + 1;
	} else {
		off +=
		    nblk = pt->pt_dk.dk_label->d_partitions[labelpart].p_offset;
		nblk += pt->pt_dk.dk_label->d_partitions[labelpart].p_size;
	}

	/* deal with the EOF condition */
	if (off + cnt > nblk) {
		if (off >= nblk) {
			if (off == nblk)
				goto done;
			goto inval;
		}
		cnt = nblk - off;
		bp->b_resid = bp->b_bcount - (cnt * sc->sc_bsize);
	}

	bp->b_rawblkno = off;

	/* queue this transfer */
	bufq_put(sc->sc_q, bp);

	if (sc->sc_stat == MMEM_IDLE)
		mmemstart(sc);

	return;

inval:	bp->b_error = EINVAL;
done:	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * start I/O operations
 */
static void
mmemstart(struct mmem_softc *sc)
{
	struct buf *bp;
	struct mmem_pt *pt;
	int s;

	if ((bp = bufq_get(sc->sc_q)) == NULL) {
		sc->sc_stat = MMEM_IDLE;
		maple_enable_unit_ping(sc->sc_parent, sc->sc_unit,
		    MAPLE_FN_MEMCARD, 1);
		return;
	}

	sc->sc_bp = bp;
	sc->sc_cnt = howmany(bp->b_bcount - bp->b_resid, sc->sc_bsize);
	KASSERT(sc->sc_cnt);
	sc->sc_iobuf = bp->b_data;
	sc->sc_retry = 0;

	pt = &sc->sc_pt[MMEM_PART(DISKUNIT(bp->b_dev))];
	s = splbio();
	disk_busy(&pt->pt_dk);
	splx(s);

	/*
	 * I/O access will fail if the removal detection (by maple driver)
	 * occurs before finishing the I/O, so disable it.
	 * We are sending commands, and the removal detection is still alive.
	 */
	maple_enable_unit_ping(sc->sc_parent, sc->sc_unit, MAPLE_FN_MEMCARD, 0);

	mmemstart_bp(sc);
}

/*
 * start/retry a specified I/O operation
 */
static void
mmemstart_bp(struct mmem_softc *sc)
{
	struct buf *bp;
	int diskunit, part;
	struct mmem_pt *pt;

	bp = sc->sc_bp;
	diskunit = DISKUNIT(bp->b_dev);
	part = MMEM_PART(diskunit);
	pt = &sc->sc_pt[part];

	/* handle retry */
	if (sc->sc_retry++ > MMEM_MAXRETRY) {
		/* retry count exceeded */
		mmemdone(sc, pt, EIO);
		return;
	}

	/*
	 * Start the first phase (phase# = 0).
	 */
	/* start read */
	sc->sc_stat = (bp->b_flags & B_READ) ? MMEM_READ : MMEM_WRITE1;
	sc->sc_reqr.func_code = htobe32(MAPLE_FUNC(MAPLE_FN_MEMCARD));
	sc->sc_reqr.pt = part;
	sc->sc_reqr.block = htobe16(bp->b_rawblkno);
	sc->sc_reqr.phase = 0;		/* first phase */
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_MEMCARD,
	    MAPLE_COMMAND_BREAD, sizeof sc->sc_reqr / 4, &sc->sc_reqr, 0);
}

static void
mmemstart_write2(struct mmem_softc *sc)
{
	struct buf *bp;
	int diskunit, part;
	struct mmem_pt *pt;

	bp = sc->sc_bp;
	diskunit = DISKUNIT(bp->b_dev);
	part = MMEM_PART(diskunit);
	pt = &sc->sc_pt[part];

	/* handle retry */
	if (sc->sc_retry++ > MMEM_MAXRETRY - 2 /* spare for verify read */) {
		/* retry count exceeded */
		mmemdone(sc, pt, EIO);
		return;
	}

	/*
	 * Start the first phase (phase# = 0).
	 */
	/* start write */
	sc->sc_stat = MMEM_WRITE2;
	sc->sc_reqw.func_code = htobe32(MAPLE_FUNC(MAPLE_FN_MEMCARD));
	sc->sc_reqw.pt = part;
	sc->sc_reqw.block = htobe16(bp->b_rawblkno);
	sc->sc_reqw.phase = 0;		/* first phase */
	memcpy(sc->sc_reqw.data, sc->sc_iobuf /* + sc->sc_waccsz * phase */,
	    sc->sc_waccsz);
	maple_command(sc->sc_parent, sc->sc_unit, MAPLE_FN_MEMCARD,
	    MAPLE_COMMAND_BWRITE, MMEM_SIZE_REQW(sc) / 4, &sc->sc_reqw,
	    MAPLE_FLAG_CMD_PERIODIC_TIMING);
}

static void
mmemdone(struct mmem_softc *sc, struct mmem_pt *pt, int err)
{
	struct buf *bp = sc->sc_bp;
	int s;
	int bcnt;

	KASSERT(bp);

	if (err) {
		bcnt = (char *)sc->sc_iobuf - (char *)bp->b_data;
		bp->b_resid = bp->b_bcount - bcnt;

		/* raise error if no block is read */
		if (bcnt == 0) {
			bp->b_error = err;
		}
		goto term_xfer;
	}

	sc->sc_iobuf += sc->sc_bsize;
	if (--sc->sc_cnt == 0) {
	term_xfer:
		/* terminate current transfer */
		sc->sc_bp = NULL;
		s = splbio();
		disk_unbusy(&pt->pt_dk,
		    (char *)sc->sc_iobuf - (char *)bp->b_data,
		    sc->sc_stat == MMEM_READ);
		biodone(bp);
		splx(s);

		/* go next transfer */
		mmemstart(sc);
	} else {
		/* go next block */
		bp->b_rawblkno++;
		sc->sc_retry = 0;
		mmemstart_bp(sc);
	}
}

int
mmemread(dev_t dev, struct uio *uio, int flags)
{

	return physio(mmemstrategy, NULL, dev, B_READ, minphys, uio);
}

int
mmemwrite(dev_t dev, struct uio *uio, int flags)
{

	return physio(mmemstrategy, NULL, dev, B_WRITE, minphys, uio);
}

int
mmemioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int diskunit, unit, part;
	struct mmem_softc *sc;
	struct mmem_pt *pt;

	diskunit = DISKUNIT(dev);
	unit = MMEM_UNIT(diskunit);
	part = MMEM_PART(diskunit);
	sc = device_lookup_private(&mmem_cd, unit);
	pt = &sc->sc_pt[part];

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)data = *pt->pt_dk.dk_label; /* XXX */
		break;

	default:
		/* generic maple ioctl */
		return maple_unit_ioctl(sc->sc_parent, sc->sc_unit, cmd, data,
		    flag, l);
	}

	return 0;
}
