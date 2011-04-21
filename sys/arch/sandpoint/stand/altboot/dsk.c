/* $NetBSD: dsk.c,v 1.4.2.3 2011/04/21 01:41:22 rmind Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

/*
 * assumptions;
 * - up to 4 IDE/SATA drives.
 * - a single (master) drive in each IDE channel.
 * - all drives are up and spinning.
 */

#include <sys/types.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>

#include <sys/disklabel.h>
#include <sys/bootblock.h>

#include <machine/bootinfo.h>
#include <machine/stdarg.h>

#include "globals.h"

/*
 * - no vtophys() translation, vaddr_t == paddr_t.
 */
#define CSR_READ_4(r)		in32rb(r)
#define CSR_WRITE_4(r,v)	out32rb(r,v)
#define CSR_READ_1(r)		*(volatile uint8_t *)(r)
#define CSR_WRITE_1(r,v)	*(volatile uint8_t *)(r)=(v)

struct dskdv {
	char *name;
	int (*match)(unsigned, void *);
	void *(*init)(unsigned, void *);
};

static struct dskdv ldskdv[] = {
	{ "pciide", pciide_match, pciide_init },
	{ "siisata", siisata_match, siisata_init },
};
static int ndskdv = sizeof(ldskdv)/sizeof(ldskdv[0]);

static int disk_scan(void *);
static int probe_drive(struct dkdev_ata *, int);
static void drive_ident(struct disk *, char *);
static char *mkident(char *, int);
static void set_xfermode(struct dkdev_ata *, int);
static void decode_dlabel(struct disk *, char *);
static int lba_read(struct disk *, int64_t, int, void *);
static void issue48(struct dvata_chan *, int64_t, int);
static void issue28(struct dvata_chan *, int64_t, int);
static struct disk *lookup_disk(int);

static struct disk ldisk[4];

int
dskdv_init(void *self)
{
	struct pcidev *pci = self;
	struct dskdv *dv;
	unsigned tag;
	int n;

	tag = pci->bdf;
	for (n = 0; n < ndskdv; n++) {
		dv = &ldskdv[n];
		if ((*dv->match)(tag, NULL) > 0)
			goto found;
	}
	return 0;
  found:
	pci->drv = (*dv->init)(tag, NULL);
	disk_scan(pci->drv);
	return 1;
}

static int
disk_scan(void *drv)
{
	struct dkdev_ata *l = drv;
	struct disk *d;
	int n, ndrive;

	ndrive = 0;
	for (n = 0; n < 4; n++) {
		if (l->presense[n] == 0)
			continue;
		if (probe_drive(l, n) == 0) {
			l->presense[n] = 0;
			continue;
		}
		d = &ldisk[ndrive];
		d->dvops = l;
		d->unittag = ndrive;
		snprintf(d->xname, sizeof(d->xname), "wd%d", d->unittag);
		set_xfermode(l, n);
		drive_ident(d, l->iobuf);
		decode_dlabel(d, l->iobuf);
		ndrive += 1;
	}
	return ndrive;
}

int
spinwait_unbusy(struct dkdev_ata *l, int n, int milli, const char **err)
{
	struct dvata_chan *chan = &l->chan[n];
	int sts;
	const char *msg;

	/*
	 * For best compatibility it is recommended to wait 400ns and
	 * read the alternate status byte four times before the status
	 * is valid.
	 */
	delay(1);
	(void)CSR_READ_1(chan->alt);
	(void)CSR_READ_1(chan->alt);
	(void)CSR_READ_1(chan->alt);
	(void)CSR_READ_1(chan->alt);

	sts = CSR_READ_1(chan->cmd + _STS);
	while (milli-- > 0
	    && sts != 0xff
	    && (sts & (ATA_STS_BUSY|ATA_STS_DRDY)) != ATA_STS_DRDY) {
		delay(1000);
		sts = CSR_READ_1(chan->cmd + _STS);
	}

	msg = NULL;
	if (sts == 0xff)
		msg = "returned 0xff";
	else if (sts & ATA_STS_ERR)
		msg = "returned ERR";
	else if (sts & ATA_STS_BUSY)
		msg = "remains BUSY";
	else if ((sts & ATA_STS_DRDY) == 0)
		msg = "no DRDY";

	if (err != NULL)
		*err = msg;
	return msg == NULL;
}

int
perform_atareset(struct dkdev_ata *l, int n)
{
	struct dvata_chan *chan = &l->chan[n];

	CSR_WRITE_1(chan->ctl, ATA_DREQ);
	delay(10);
	CSR_WRITE_1(chan->ctl, ATA_SRST|ATA_DREQ);
	delay(10);
	CSR_WRITE_1(chan->ctl, ATA_DREQ);

	return spinwait_unbusy(l, n, 150, NULL);
}

int
satapresense(struct dkdev_ata *l, int n)
{
#define VND_CH(n) (((n&02)<<8)+((n&01)<<7))
#define VND_SC(n) (0x100+VND_CH(n))
#define VND_SS(n) (0x104+VND_CH(n))

	uint32_t sc = l->bar[5] + VND_SC(n);
	uint32_t ss = l->bar[5] + VND_SS(n);
	unsigned val;

	val = (00 << 4) | (03 << 8);	/* any speed, no pwrmgt */
	CSR_WRITE_4(sc, val | 01);	/* perform init */
	delay(50 * 1000);
	CSR_WRITE_4(sc, val);
	delay(50 * 1000);	
	val = CSR_READ_4(ss);		/* has completed */
	return ((val & 03) == 03);	/* active drive found */
}

static int
probe_drive(struct dkdev_ata *l, int n)
{
	struct dvata_chan *chan = &l->chan[n];
	uint16_t *p;
	int i;
	
	CSR_WRITE_1(chan->cmd + _CMD, ATA_CMD_IDENT);
	(void)CSR_READ_1(chan->alt);
	delay(10 * 1000);
	if (spinwait_unbusy(l, n, 1000, NULL) == 0)
		return 0;

	p = (uint16_t *)l->iobuf;
	for (i = 0; i < 512; i += 2) {
		/* need to have bswap16 */
		*p++ = iole16toh(chan->cmd + _DAT);
	}
	(void)CSR_READ_1(chan->cmd + _STS);
	return 1;
}

static void
drive_ident(struct disk *d, char *ident)
{
	uint16_t *p;
	uint64_t huge;

	p = (uint16_t *)ident;
	DPRINTF(("[49]%04x [82]%04x [83]%04x [84]%04x "
	   "[85]%04x [86]%04x [87]%04x [88]%04x\n",
	    p[49], p[82], p[83], p[84],
	    p[85], p[86], p[87], p[88]));
	huge = 0;
	printf("%s: ", d->xname);
	printf("<%s> ", mkident((char *)ident + 54, 40));
	if (p[49] & (1 << 8))
		printf("DMA ");
	if (p[49] & (1 << 9)) {
		printf("LBA ");
		huge = p[60] | (p[61] << 16);
	}
	if ((p[83] & 0xc000) == 0x4000 && (p[83] & (1 << 10))) {
		printf("LBA48 ");
		huge = p[100] | (p[101] << 16);
		huge |= (uint64_t)p[102] << 32;
		huge |= (uint64_t)p[103] << 48;
	}
	huge >>= (1 + 10);
	printf("%d MB\n", (int)huge);

	memcpy(d->ident, ident, sizeof(d->ident));
	d->nsect = huge;
	d->lba_read = lba_read;
}

static char *
mkident(char *src, int len)
{
	static char local[40];
	char *dst, *end, *last;
	
	if (len > sizeof(local))
		len = sizeof(local);
	dst = last = local;
	end = src + len - 1;

	/* reserve space for '\0' */
	if (len < 2)
		goto out;
	/* skip leading white space */
	while (*src != '\0' && src < end && *src == ' ')
		++src;
	/* copy string, omitting trailing white space */
	while (*src != '\0' && src < end) {
		*dst++ = *src;
		if (*src++ != ' ')
			last = dst;
	}
 out:
	*last = '\0';
	return local;
}

static void
decode_dlabel(struct disk *d, char *iobuf)
{
        struct mbr_partition *mp, *bsdp;
	struct disklabel *dlp;
	struct partition *pp;
	char *dp;
	int i, first;

	bsdp = NULL;
	(*d->lba_read)(d, 0, 1, iobuf);
	if (bswap16(*(uint16_t *)(iobuf + MBR_MAGIC_OFFSET)) != MBR_MAGIC)
		goto skip;
	mp = (struct mbr_partition *)(iobuf + MBR_PART_OFFSET);
	for (i = 0; i < MBR_PART_COUNT; i++, mp++) {
		if (mp->mbrp_type == MBR_PTYPE_NETBSD) {
			bsdp = mp;
			break;
		}
	}
  skip:
	first = (bsdp) ? bswap32(bsdp->mbrp_start) : 0;
	(*d->lba_read)(d, first + LABELSECTOR, 1, iobuf);
	dp = iobuf /* + LABELOFFSET */;
	for (i = 0; i < 512 - sizeof(struct disklabel); i++, dp += 4) {
		dlp = (struct disklabel *)dp;
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC) {
			goto found;
		}
	}
	d->dlabel = NULL;
	printf("%s: no disklabel\n", d->xname);
	return;
  found:
	d->dlabel = allocaligned(sizeof(struct disklabel), 4);
	memcpy(d->dlabel, dlp, sizeof(struct disklabel));
	for (i = 0; i < dlp->d_npartitions; i += 1) {
		const char *type;
		pp = &dlp->d_partitions[i];
		type = NULL;
		switch (pp->p_fstype) {
		case FS_SWAP: /* swap */
			type = "swap";
			break;
		case FS_BSDFFS:
			type = "ffs";
			break;
		case FS_EX2FS:
			type = "ext2fs";
			break;
		}
		if (type != NULL)
			printf("%s%c: %s\n", d->xname, i + 'a', type);
	}
}

static void
set_xfermode(struct dkdev_ata *l, int n)
{
	struct dvata_chan *chan = &l->chan[n];

	CSR_WRITE_1(chan->cmd + _FEA, ATA_XFER);
	CSR_WRITE_1(chan->cmd + _NSECT, XFER_PIO0);
	CSR_WRITE_1(chan->cmd + _DEV, ATA_DEV_OBS); /* ??? */
	CSR_WRITE_1(chan->cmd + _CMD, ATA_CMD_SETF);

	spinwait_unbusy(l, n, 1000, NULL);
}

static int
lba_read(struct disk *d, int64_t bno, int bcnt, void *buf)
{
	struct dkdev_ata *l;
	struct dvata_chan *chan;
	void (*issue)(struct dvata_chan *, int64_t, int);
	int n, rdcnt, i, k;
	uint16_t *p;
	const char *err;
	int error;

	l = d->dvops;
	n = d->unittag;
	p = (uint16_t *)buf;
	chan = &l->chan[n];
	error = 0;
	for ( ; bcnt > 0; bno += rdcnt, bcnt -= rdcnt) {
		issue = (bno < (1ULL<<28)) ? issue28 : issue48;
		rdcnt = (bcnt > 255) ? 255 : bcnt;
		(*issue)(chan, bno, rdcnt);
		for (k = 0; k < rdcnt; k++) {
			if (spinwait_unbusy(l, n, 1000, &err) == 0) {
				printf("%s blk %lld %s\n", d->xname, bno, err);
				error = EIO;
				break;
			}
			for (i = 0; i < 512; i += 2) {
				/* arrives in native order */
				*p++ = *(uint16_t *)(chan->cmd + _DAT);
			}
			/* clear irq if any */
			(void)CSR_READ_1(chan->cmd + _STS);
		}
	}
	return error;
}

static void
issue48(struct dvata_chan *chan, int64_t bno, int nblk)
{

	CSR_WRITE_1(chan->cmd + _NSECT, 0); /* always less than 256 */
	CSR_WRITE_1(chan->cmd + _LBAL, (bno >> 24) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAM, (bno >> 32) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAH, (bno >> 40) & 0xff);
	CSR_WRITE_1(chan->cmd + _NSECT, nblk);
	CSR_WRITE_1(chan->cmd + _LBAL, (bno >>  0) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAM, (bno >>  8) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAH, (bno >> 16) & 0xff);
	CSR_WRITE_1(chan->cmd + _DEV, ATA_DEV_LBA);
	CSR_WRITE_1(chan->cmd + _CMD, ATA_CMD_READ_EXT);
}

static void
issue28(struct dvata_chan *chan, int64_t bno, int nblk)
{

	CSR_WRITE_1(chan->cmd + _NSECT, nblk);
	CSR_WRITE_1(chan->cmd + _LBAL, (bno >>  0) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAM, (bno >>  8) & 0xff);
	CSR_WRITE_1(chan->cmd + _LBAH, (bno >> 16) & 0xff);
	CSR_WRITE_1(chan->cmd + _DEV, ((bno >> 24) & 0xf) | ATA_DEV_LBA);
	CSR_WRITE_1(chan->cmd + _CMD, ATA_CMD_READ);
}

static struct disk *
lookup_disk(int unit)
{

	return &ldisk[unit];
}

int
dsk_open(struct open_file *f, ...)
{
	va_list ap;
	int unit, part;
	const char *name;
	struct disk *d;
	struct disklabel *dlp;
	struct fs_ops *fs;
	int error;
	extern struct btinfo_bootpath bi_path;
	extern struct btinfo_rootdevice bi_rdev;
	extern struct fs_ops fs_ffsv2, fs_ffsv1;

	va_start(ap, f);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	name = va_arg(ap, const char *);
	va_end(ap);

	if ((d = lookup_disk(unit)) == NULL)
		return ENXIO;
	f->f_devdata = d;
	if ((dlp = d->dlabel) == NULL || part >= dlp->d_npartitions)
		return ENXIO;
	d->part = part;

	snprintf(bi_path.bootpath, sizeof(bi_path.bootpath), name);
	if (dlp->d_partitions[part].p_fstype == FS_BSDFFS) {
		if ((error = ffsv2_open(name, f)) == 0) {
			fs = &fs_ffsv2;
			goto found;
		}
		if (error == EINVAL && (error = ffsv1_open(name, f)) == 0) {
			fs = &fs_ffsv1;
			goto found;
		}
		return error;
	}
	return ENXIO;
  found:
	d->fsops = fs;
	f->f_devdata = d;

	/* build btinfo to identify disk device */
	snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), "wd");
	bi_rdev.cookie = d->unittag; /* disk unit number */
	return 0;
}

int
dsk_close(struct open_file *f)
{
	struct disk *d = f->f_devdata;
	struct fs_ops *fs = d->fsops;

	(*fs->close)(f);
	d->fsops = NULL;
	f->f_devdata = NULL;
	return 0;
}

int
dsk_strategy(void *devdata, int rw, daddr_t dblk, size_t size,
	void *p, size_t *rsize)
{
	struct disk *d = devdata;
	struct disklabel *dlp;
	int64_t bno;

	if (size == 0)
		return 0;
	if (rw != F_READ)
		return EOPNOTSUPP;

	bno = dblk;
	if ((dlp = d->dlabel) != NULL)
		bno += dlp->d_partitions[d->part].p_offset;
	(*d->lba_read)(d, bno, size / 512, p);
	if (rsize != NULL)
		*rsize = size;
	return 0;
}

struct fs_ops *
dsk_fsops(struct open_file *f)
{
	struct disk *d = f->f_devdata;

	return d->fsops;
}
