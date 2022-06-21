/*	$NetBSD: sdcd.c,v 1.17 2022/06/21 12:43:57 isaki Exp $	*/

/*
 * Copyright (c) 2001 MINOURA Makoto.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/disklabel.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "libx68k.h"
#include "sdcdvar.h"
#include "iocs.h"


static int current_id = -1;
static int current_blkbytes;
static int current_blkshift;
static int current_devsize, current_npart;
static struct boot_partinfo partitions[MAXPARTITIONS];

static uint human2blk(uint);
static uint human2bsd(uint);
static uint bsd2blk(uint);
static int readdisklabel(int);
static int check_unit(int);

#ifdef DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	
#endif

/*
 * Convert the number of sectors on Human68k
 * into the number of blocks on the current device.
 */
static uint
human2blk(uint n)
{
	uint blk_per_sect;

	/* Human68k uses 1024 byte/sector. */
	blk_per_sect = 4 >> current_blkshift;
	if (blk_per_sect == 0)
		blk_per_sect = 1;
	return blk_per_sect * n;
}

/*
 * Convert the number of sectors on Human68k
 * into the number of DEV_BSIZE sectors.
 */
static uint
human2bsd(uint n)
{

	return n * (1024 / DEV_BSIZE);
}

/*
 * Convert the number of DEV_BSIZE sectors
 * into the number of blocks on the current device.
 */
static uint
bsd2blk(uint n)
{

	return ((DEV_BSIZE / 256) * n) >> current_blkshift;
}

static int
check_unit(int id)
{
#define BUFFER_SIZE	8192
	int error;
	void *buffer = alloca(BUFFER_SIZE);

	if (current_id == id)
		return 0;

	current_id = -1;

	error = IOCS_S_TESTUNIT(id);
	if (error < 0) {			/* not ready */
		error = ENXIO;
		goto out;
	}

	{
		struct iocs_inquiry *inqdata = buffer;

		error = IOCS_S_INQUIRY(sizeof(*inqdata), id, inqdata);
		if (error < 0) {		/* WHY??? */
			error = ENXIO;
			goto out;
		}
		if ((inqdata->unit != 0) &&	/* direct */
		    (inqdata->unit != 5) &&	/* cdrom */
		    (inqdata->unit != 7)) {	/* optical */
			error = EUNIT;
			goto out;
		}
	}

	{
		struct iocs_readcap *rcdata = buffer;

		error = IOCS_S_READCAP(id, rcdata);
		if (error < 0) {		/* WHY??? */
			error = EUNIT;
			goto out;
		}
		current_blkbytes = rcdata->size;
		current_blkshift = fls32(current_blkbytes) - 9;
		current_devsize = rcdata->block;
	}

	{
		error = IOCS_S_READ(0, 1, id, current_blkshift, buffer);
		if (error < 0) {
			error =  EIO;
			goto out;
		}
		if (strncmp((char *)buffer, "X68SCSI1", 8) != 0) {
			error = EUNLAB;
			goto out;
		}
	}

 out:
	return error;
}

static int
readdisklabel(int id)
{
	int error, i;
	char *buffer;
	struct disklabel *label;
	struct dos_partition *parttbl;

	if (current_id == id)
		return 0;
	current_id = -1;

	error = check_unit(id);
	if (error)
		return error;
	if (current_blkbytes > 2048) {
		printf("FATAL: Unsupported block size %d.\n",
		    current_blkbytes);
		return ERDLAB;
	}

	/* Try BSD disklabel first */
	buffer = alloca(2048);
	error = IOCS_S_READ(LABELSECTOR, 1, id, current_blkshift, buffer);
	if (error < 0)
		return EIO;
	label = (void *)(buffer + LABELOFFSET);
	if (label->d_magic == DISKMAGIC &&
	    label->d_magic2 == DISKMAGIC) {
		for (i = 0; i < label->d_npartitions; i++) {
			partitions[i].start = label->d_partitions[i].p_offset;
			partitions[i].size  = label->d_partitions[i].p_size;
		}
		current_npart = label->d_npartitions;

		goto done;
	}

	/* Try Human68K-style partition table */
	error = IOCS_S_READ(human2blk(2), 1, id, current_blkshift, buffer);
	if (error < 0)
		return EIO;
	parttbl = (void *)(buffer + DOSBBSECTOR);
	if (strncmp(buffer, "X68K", 4) != 0)
		return EUNLAB;
	parttbl++;
	for (current_npart = 0, i = 0;
	     current_npart < MAXPARTITIONS && i < 15 && parttbl[i].dp_size;
	     i++) {
		partitions[current_npart].start
			= human2bsd(parttbl[i].dp_start);
		partitions[current_npart].size
			= human2bsd(parttbl[i].dp_size);
		if (++current_npart == RAW_PART) {
			partitions[current_npart].start = 0;
			partitions[current_npart].size = -1; /* XXX */
			current_npart++;
		}
	}
done:
#ifdef DEBUG
	for (i = 0; i < current_npart; i++) {
		printf ("%d: starts %d, size %d\n", i,
			partitions[i].start,
			partitions[i].size);
	}
#endif
	current_id = id;

	return 0;
}

int
sd_getbsdpartition(int id, int humanpart)
{
	int error, i;
	char *buffer;
	struct dos_partition *parttbl;
	unsigned parttop;

	if (humanpart < 2)
		humanpart++;

	error = readdisklabel(id);
	if (error) {
		printf("Reading disklabel: %s\n", strerror(error));
		return -1;
	}
	buffer = alloca(2048);
	error = IOCS_S_READ(human2blk(2), 1, id, current_blkshift, buffer);
	if (error < 0) {
		printf("Reading partition table: %s\n", strerror(error));
		return -1;
	}
	parttbl = (void *)(buffer + DOSBBSECTOR);
	if (strncmp(buffer, "X68K", 4) != 0)
		return 0;
	parttop = human2bsd(parttbl[humanpart].dp_start);

	for (i = 0; i < current_npart; i++) {
		if (partitions[i].start == parttop)
			return i;
	}

	printf("Could not determine the boot partition.\n");

	return -1;
}

struct sdcd_softc {
	int			sc_part;
	struct boot_partinfo	sc_partinfo;
};

/* sdopen(struct open_file *f, int id, int part) */
int
sdopen(struct open_file *f, ...)
{
	int error;
	struct sdcd_softc *sc;
	int id, part;
	va_list ap;

	va_start(ap, f);
	id   = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

	if (id < 0 || id > 7)
		return ENXIO;
	if (current_id != id) {
		error = readdisklabel(id);
		if (error)
			return error;
	}
	if (part >= current_npart)
		return ENXIO;

	sc = alloc(sizeof(struct sdcd_softc));
	sc->sc_part = part;
	sc->sc_partinfo = partitions[part];
	f->f_devdata = sc;
	return 0;
}

int
sdclose(struct open_file *f)
{

	dealloc(f->f_devdata, sizeof(struct sdcd_softc));
	return 0;
}

int
sdstrategy(void *arg, int rw, daddr_t dblk, size_t size,
           void *buf, size_t *rsize)
{
	struct sdcd_softc *sc = arg;
	uint32_t	start = sc->sc_partinfo.start + dblk;
	size_t		nblks;
	int		error;

	if (size == 0) {
		if (rsize)
			*rsize = 0;
		return 0;
	}
	start = bsd2blk(start);
	nblks = howmany(size, current_blkbytes);

	if (start < 0x200000 && nblks < 256) {
		if (rw & F_WRITE)
			error = IOCS_S_WRITE(start, nblks, current_id,
			                     current_blkshift, buf);
		else
			error = IOCS_S_READ(start, nblks, current_id,
			                    current_blkshift, buf);
	} else {
		if (rw & F_WRITE)
			error = IOCS_S_WRITEEXT(start, nblks, current_id,
			                        current_blkshift, buf);
		else
			error = IOCS_S_READEXT(start, nblks, current_id,
			                       current_blkshift, buf);
	}
	if (error < 0)
		return EIO;

	if (rsize)
		*rsize = size;
	return 0;
}

/* cdopen(struct open_file *f, int id, int part) */
int
cdopen(struct open_file *f, ...)
{
	int error;
	struct sdcd_softc *sc;
	int id, part;
	va_list ap;

	va_start(ap, f);
	id   = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

	if (id < 0 || id > 7)
		return ENXIO;
	if (part != 0 && part != 2)
		return ENXIO;
	if (current_id != id) {
		error = check_unit(id);
		if (error)
			return error;
	}

	sc = alloc(sizeof(struct sdcd_softc));
	current_npart = 3;
	sc->sc_part = 0;
	sc->sc_partinfo.start = 0;
	sc->sc_partinfo.size = current_devsize;
	f->f_devdata = sc;
	current_id = id;

	return 0;
}

int
cdclose(struct open_file *f)
{

	dealloc(f->f_devdata, sizeof(struct sdcd_softc));
	return 0;
}

int
cdstrategy(void *arg, int rw, daddr_t dblk, size_t size,
           void *buf, size_t *rsize)
{
	return sdstrategy(arg, rw, dblk, size, buf, rsize);
}
