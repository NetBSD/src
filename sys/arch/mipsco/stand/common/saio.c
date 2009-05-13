/*	$NetBSD: saio.c,v 1.9.70.1 2009/05/13 17:18:04 jym Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)rz.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/prom.h>
#include <machine/stdarg.h>

#include <sys/param.h>
#include <sys/disklabel.h>

#include "common.h"
#include "saio.h"

struct	saio_softc {
	int	sc_fd;			/* PROM file id */
	int	sc_ctlr;		/* controller number */
	int	sc_unit;		/* disk unit number */
	int	sc_part;		/* disk partition number */
	struct	disklabel sc_label;	/* disk label for this disk */
};

struct io_arg {
	int retval;
	unsigned long sectst;
	unsigned long memaddr;
	unsigned long datasz;
};

#define IOB_BUFSZ	512
#define IOB_INODESZ	316

struct device_table {
	char *dt_string;	/* device name */
	int (*dt_init) (int);	/* device init routine */
	int (*dt_open)(int);	/* device open routine */
	int (*dt_strategy)(int);	/* device strategy routine, returns cnt */
	int (*dt_close)(int);	/* device close routine */
	int (*dt_ioctl)(int);	/* device ioctl routine */
	int dt_type;		/* device "type" */
	int dt_fs;		/* file system type */
	char *dt_desc;		/* device description */
};

struct sa_iob {
	char	i_buf[IOB_BUFSZ];	/* file system or tape header */
	char	i_ino_dir[IOB_INODESZ];	/* inode or disk/tape directory */

	int	i_flgs;		/* see F_ below */
	int	i_ctlr;		/* controller board */
	int	i_unit;		/* pseudo device unit */
	int	i_part;		/* disk partition */
	char	*i_ma;		/* memory address of i/o buffer */
	int	i_cc;		/* character count of transfer */
	int32_t	i_offset;	/* seek offset in file */
	/* XXX ondisk32 */
	int32_t	i_bn;		/* 1st block # of next read */
	int	i_fstype;	/* file system type */
	int	i_errno;	/* error # return */
	unsigned int	i_devaddr;	/* csr address */
	struct device_table *i_dp;	/* pointer into device_table */
	char	*i_bufp;			/* i/o buffer for blk devs */
}__attribute__((__packed__));

extern struct sa_iob *saiob[];

int
saiostrategy(void *devdata, int rw, daddr_t bn, size_t reqcnt, void *addr, size_t *cnt)
	/* cnt:	 out: number of bytes transfered */
{
	struct saio_softc *sc = (struct saio_softc *)devdata;
	int part = sc->sc_part;
	struct partition *pp = &sc->sc_label.d_partitions[part];
	int s;
	long offset;
	struct sa_iob *iob;
	char *adr;

	offset = bn * DEV_BSIZE;
	*cnt = 0;

	/*
	 * Partial-block transfers not handled.
	 */
	if (reqcnt & (DEV_BSIZE - 1)) {
		return (EINVAL);
	}
	offset += pp->p_offset * DEV_BSIZE;

	iob = saiob[sc->sc_fd];
	iob->i_offset = offset;

#if notyet
	if (prom_lseek(sc->sc_fd, offset, 0) < 0)
		return (EIO);
#endif

	adr = (char *)addr;
	while (*cnt < reqcnt) {
		s = prom_read(sc->sc_fd, iob->i_buf, 512);
		if (s < 0) {
			return (EIO);
		}
		memcpy(adr, iob->i_buf, s);
		*cnt += s;
		adr += s;
	}
	return (0);
}

int
saioopen(struct open_file *f, ...)
{
	int ctlr, unit, part;

	struct saio_softc *sc;
	struct disklabel *lp;
	int i;
	char *msg;
	char buf[DEV_BSIZE];
	int cnt;
	static char device[] = "dksd(0,0,10)";

	va_list ap;

	va_start(ap, f);

	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);

	va_end(ap);

	device[5] = '0' + ctlr;
	device[7] = '0' + unit;

	i = prom_open(device, 0);
	if (i < 0) {
		printf("open failed\n");
		return (ENXIO);
	}

	sc = alloc(sizeof(struct saio_softc));
	memset(sc, 0, sizeof(struct saio_softc));
	f->f_devdata = (void *)sc;

	sc->sc_fd = i;
	sc->sc_ctlr = ctlr;
	sc->sc_unit = unit;
	sc->sc_part = part;

	/* try to read disk label and partition table information */
	lp = &sc->sc_label;
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[part].p_offset = 0;
	lp->d_partitions[part].p_size = 0x7fffffff;

	i = saiostrategy(sc, F_READ, (daddr_t)LABELSECTOR, DEV_BSIZE, buf, &cnt);
	if (i || cnt != DEV_BSIZE) {
		printf("%s: error reading disk label\n", device);
		goto bad;
	}

	msg = getdisklabel(buf, lp);
	if (msg) {
#ifdef LIBSA_NO_DISKLABEL_MSGS
		printf("%s: no disklabel\n", device);
#else
		printf("getlabel: %s\n", msg);
#endif
		return (0);
	}
	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0) {
	bad:
		dealloc(sc, sizeof(struct saio_softc));
		return (ENXIO);
	}
	return (0);
}

#ifndef LIBSA_NO_DEV_CLOSE
int
saioclose(struct open_file *f)
{

	prom_close(((struct saio_softc *)f->f_devdata)->sc_fd);
	dealloc(f->f_devdata, sizeof(struct saio_softc));
	f->f_devdata = (void *)0;
	return (0);
}
#endif
