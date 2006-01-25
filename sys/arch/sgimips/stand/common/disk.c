/*	$NetBSD: disk.c,v 1.9 2006/01/25 18:28:28 christos Exp $	*/

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
	u_long	sc_fd;			/* ARCBIOS file id */
	int	sc_part;		/* disk partition number */
	struct	disklabel sc_label;	/* disk label for this disk */
};

int
diskstrategy(void *devdata, int rw, daddr_t bn, size_t reqcnt, void *addr,
    size_t *cnt)
{
	struct disk_softc *sc = (struct disk_softc *)devdata;
	int part = sc->sc_part;
	struct partition *pp = &sc->sc_label.d_partitions[part];
	long error;
	int64_t offset;
	u_long count;

	offset = bn;

	/*
	 * Partial-block transfers not handled.
	 */
	if (reqcnt & (DEV_BSIZE - 1)) {
		*cnt = 0;
		return EINVAL;
	}

	offset += pp->p_offset;

	if (pp->p_fstype == FS_RAID)
		offset += RF_PROTECTED_SECTORS;

	/*
	 * Convert from blocks to bytes.
	 */
	offset *= DEV_BSIZE;

	error = (*ARCBIOS->Seek)(sc->sc_fd, &offset, 0);
	if (error != ARCBIOS_ESUCCESS)
		return EIO;
	error = (*ARCBIOS->Read)(sc->sc_fd, addr, reqcnt, &count);
	if (error != ARCBIOS_ESUCCESS)
		return EIO;

	*cnt = count;
	return 0;
}

int
diskopen(struct open_file *f, ...)
{
	int part;

	struct disk_softc *sc;
	struct disklabel *lp;
#ifdef arc
	char *msg, buf[DEV_BSIZE];
	size_t cnt;
	int mbrp_off, i;
#endif
	int error;
	u_long fd;
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

	if (part >= MAXPARTITIONS)
		return ENXIO;

	error = (*ARCBIOS->Open)(device, 0, &fd);
	if (error) {
		printf("diskopen: open failed, errno = %d\n", error);
		return ENXIO;
	}

	sc = alloc(sizeof(struct disk_softc));
	memset(sc, 0, sizeof(struct disk_softc));
	f->f_devdata = (void *)sc;

	sc->sc_fd = fd;
	sc->sc_part = part;

	/* try to read disk label and partition table information */
	lp = &sc->sc_label;
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[part].p_offset = 0;
	lp->d_partitions[part].p_size = 0x7fffffff;

#ifdef arc
	error = diskstrategy(sc, F_READ, (daddr_t)LABELSECTOR, DEV_BSIZE,
	    buf, &cnt);
	if (error || cnt != DEV_BSIZE) {
		printf("%s: can't read disklabel, errno = %d\n",
		    device, error);
		dealloc(sc, sizeof(struct disk_softc));
		return ENXIO;
	}
	msg = getdisklabel(buf, lp);
	if (msg) {
		/* If no label, just assume 0 and return */
		return 0;
	}

	/*
	 * On arc, we can't open whole disk, but can open each partition with
	 * OSLOADPARTITION like scsi(0)disk(0)rdisk()partition(1) etc.
	 * Thus, we don't have to add offset of the MBR partition.
	 */
	/* XXX magic: partition 2 is whole NetBSD partition */
	mbrp_off = lp->d_partitions[2].p_offset;
	for (i = 0; i < MAXPARTITIONS; i++) {
		if (lp->d_partitions[i].p_fstype != FS_UNUSED &&
		    lp->d_partitions[i].p_offset >= mbrp_off)
			lp->d_partitions[i].p_offset -= mbrp_off;
	}

	if (part >= lp->d_npartitions ||
	    lp->d_partitions[part].p_fstype == FS_UNUSED ||
	    lp->d_partitions[part].p_size == 0) {
		dealloc(sc, sizeof(struct disk_softc));
		return ENXIO;
	}
#endif
	return 0;
}

#ifndef LIBSA_NO_DEV_CLOSE
int
diskclose(struct open_file *f)
{

	(*ARCBIOS->Close)(((struct disk_softc *)(f->f_devdata))->sc_fd);
	dealloc(f->f_devdata, sizeof(struct disk_softc));
	f->f_devdata = NULL;
	return 0;
}
#endif
