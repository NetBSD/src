/*	$NetBSD: rz.c,v 1.26.30.1 2017/08/28 17:51:48 skrll Exp $	*/

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
#include <machine/dec_prom.h>

#include <sys/param.h>
#include <sys/disklabel.h>

#include "common.h"
#include "rz.h"

#define	RF_PROTECTED_SECTORS	64	/* XXX refer to <.../rf_optnames.h> */


struct	rz_softc {
	int	sc_fd;			/* PROM file id */
	int	sc_ctlr;		/* controller number */
	int	sc_unit;		/* disk unit number */
	int	sc_part;		/* disk partition number */
	struct	disklabel sc_label;	/* disk label for this disk */
};

int
rzstrategy(void *devdata, int rw, daddr_t bn, size_t reqcnt, void *addr, size_t *cnt)
	/* cnt:	 out: number of bytes transfered */
{
	struct rz_softc *sc = (struct rz_softc *)devdata;
	int part = sc->sc_part;
	struct partition *pp = &sc->sc_label.d_partitions[part];
	int s;
	long offset;

	offset = bn;

	/*
	 * Partial-block transfers not handled.
	 */
	if (reqcnt & (DEV_BSIZE - 1)) {
		*cnt = 0;
		return (EINVAL);
	}

	offset += pp->p_offset;

	if (pp->p_fstype == FS_RAID)
		offset += RF_PROTECTED_SECTORS;

	/*
	 * Convert from blocks to bytes.
	 */
	offset *= DEV_BSIZE;

	if (callv == &callvec) {
		/* No REX on this machine */
		if (prom_lseek(sc->sc_fd, offset, 0) < 0)
			return (EIO);
		s = prom_read(sc->sc_fd, addr, reqcnt);
	} else
		s = bootread (offset / 512, addr, reqcnt);
	if (s < 0)
		return (EIO);

	*cnt = s;
	return (0);
}

int
rzopen(struct open_file *f, ...)
{
	int ctlr, unit, part;

	struct rz_softc *sc;
	struct disklabel *lp;
	int i;
	char *msg;
	char buf[DEV_BSIZE];
	size_t cnt;
	static char device[] = "rz(0,0,0)";
	va_list ap;

	va_start(ap, f);

	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);
	if (unit >= 8 || part >= 8)
		return (ENXIO);
	device[5] = '0' + unit;
	/* NOTE: only support reads for now */
	/* Another NOTE: bootinit on the TURBOchannel doesn't look at
	   the device string - it's provided for compatibility with
	   the DS3100 PROMs.   As a consequence, it may be possible to
	   boot from some other drive with these bootblocks on the 3100,
	   but will not be possible on any TURBOchannel machine. */

	if (callv == &callvec)
		i = prom_open(device, 0);
	else
		i = bootinit (device);
	if (i < 0) {
		printf("open failed\n");
		return (ENXIO);
	}

	sc = alloc(sizeof(struct rz_softc));
	memset(sc, 0, sizeof(struct rz_softc));
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

	i = rzstrategy(sc, F_READ, (daddr_t)LABELSECTOR, DEV_BSIZE, buf, &cnt);
	if (i || cnt != DEV_BSIZE) {
		/* printf("rz%d: error reading disk label\n", unit); */
		goto bad;
	}
	msg = getdisklabel(buf, lp);
	if (msg) {
		/* If no label, just assume 0 and return */
		return (0);
	}

	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0) {
	bad:
		dealloc(sc, sizeof(struct rz_softc));
		return (ENXIO);
	}
	return (0);
}

#ifndef LIBSA_NO_DEV_CLOSE
int
rzclose(struct open_file *f)
{
	if (callv == &callvec)
		prom_close(((struct rz_softc *)f->f_devdata)->sc_fd);

	dealloc(f->f_devdata, sizeof(struct rz_softc));
	f->f_devdata = (void *)0;
	return (0);
}
#endif
