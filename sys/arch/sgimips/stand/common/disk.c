/*	$NetBSD: disk.c,v 1.3 2003/08/07 16:29:27 agc Exp $	*/

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
 *	@(#)disk.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>
#include <machine/stdarg.h>

#include <sys/param.h>
#include <sys/disklabel.h>

#include <dev/arcbios/arcbios.h>

#include "common.h"
#include "disk.h"

#define	RF_PROTECTED_SECTORS	64	/* XXX refer to <.../rf_optnames.h> */

extern const struct arcbios_fv *ARCBIOS;

struct	disk_softc {
	int	sc_fd;			/* PROM file id */
	int	sc_part;		/* disk partition number */
	struct	disklabel sc_label;	/* disk label for this disk */
};

int
diskstrategy(devdata, rw, bn, reqcnt, addr, cnt)
	void *devdata;
	int rw;
	daddr_t bn;
	size_t reqcnt;
	void *addr;
	size_t *cnt;	/* out: number of bytes transfered */
{
	struct disk_softc *sc = (struct disk_softc *)devdata;
	int part = sc->sc_part;
	struct partition *pp = &sc->sc_label.d_partitions[part];
	int s;
	int64_t offset;
	int count;

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

	s = ARCBIOS->Seek(sc->sc_fd, &offset, 0);
	s = ARCBIOS->Read(sc->sc_fd, addr, reqcnt, &count);

	if (s < 0)
		return (EIO);

	*cnt = count;
	return (0);
}

int
diskopen(struct open_file *f, ...)
{
	int ctlr, unit, part;

	struct disk_softc *sc;
	struct disklabel *lp;
#ifdef arc
	char *msg, buf[DEV_BSIZE];
#endif
	int i, cnt;
	char *device;
	va_list ap;

	va_start(ap, f);

	device = va_arg(ap, char *);

	/*
	 * For NetBSD/sgimips, since we use the SGI partition map directly,
	 * we fake an in-core NetBSD disklabel with offset of 0.
	 *
	 * For NetBSD/arc, there is a MBR partition map on the disk, which we
	 * then expect to find a NetBSD disklabel within the MBR partition.
	 * We require that the kernel be located in first partition in the
	 * NetBSD disklabel, because we have not other way to represent the
	 * root partition.
	 */
	part = 0;

	if (part >= 16)
		return (ENXIO);

	if (ARCBIOS->Open(device, 0, &i)) {
		printf("open failed\n");
		return (ENXIO);
	}

	sc = alloc(sizeof(struct disk_softc));
	memset(sc, 0, sizeof(struct disk_softc));
	f->f_devdata = (void *)sc;

	sc->sc_fd = i;
	sc->sc_part = part;

	/* try to read disk label and partition table information */
	lp = &sc->sc_label;
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[part].p_offset = 0;
	lp->d_partitions[part].p_size = 0x7fffffff;

#ifdef arc
	i = diskstrategy(sc, F_READ, (daddr_t)LABELSECTOR, DEV_BSIZE,
	    buf, &cnt);
	if (i || cnt != DEV_BSIZE) {
		printf("%s: error reading disk label\n", device);
		free(sc, sizeof(struct disk_softc));
		return (ENXIO);
	}
	msg = getdisklabel(buf, lp);
	if (msg) {
		/* If no label, just assume 0 and return */
		return (0);
	}
#endif

	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0) {
		free(sc, sizeof(struct disk_softc));
		return (ENXIO);
	}
	return (0);
}

#ifndef LIBSA_NO_DEV_CLOSE
int
diskclose(f)
	struct open_file *f;
{
	ARCBIOS->Close(((struct disk_softc *)(f->f_devdata))->sc_fd);
	free(f->f_devdata, sizeof(struct disk_softc));
	f->f_devdata = (void *)0;
	return (0);
}
#endif
