/*	$NetBSD: sdcd.c,v 1.1 2001/09/27 10:03:28 minoura Exp $	*/

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
#include <sys/disklabel.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "sdcdvar.h"
#include "iocs.h"


static int current_id = -1;
static int current_blklen, current_devsize, current_npart;
static struct boot_partinfo partitions[MAXPARTITIONS];

static int readdisklabel(int);
static int check_unit(int);

#ifdef DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	
#endif
#define alloca		__builtin_alloca

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

		error = IOCS_S_INQUIRY(100, id, inqdata);
		if (error < 0) {		/* WHY??? */
			error = ENXIO;
			goto out;
		}
		if ((inqdata->unit != 0) &&	/* direct */
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
		current_blklen = rcdata->size >> 9;
		current_devsize = rcdata->block;
	}

	{
		error = IOCS_S_READ(0, 1, id, current_blklen, buffer);
		if (error < 0) {
			error =  EIO;
			goto out;
		}
		if (strncmp((char*) buffer, "X68SCSI1", 8) != 0) {
			error = EUNLAB;
			goto out;
		}
	}

 out:
	return error;
}

static int
readdisklabel (int id)
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
	if (current_blklen > 4) {
		printf ("FATAL: Unsupported block size %d.\n",
			256 << current_blklen);
		return ERDLAB;
	}

	/* Try BSD disklabel first */
	buffer = alloca(2048);
	error = IOCS_S_READ(LABELSECTOR, 1, id, current_blklen, buffer);
	if (error < 0)
		return EIO;
	label = (void*) (buffer + LABELOFFSET);
	if (label->d_magic == DISKMAGIC &&
	    label->d_magic2 == DISKMAGIC) {
		for (i = 0; i < label->d_npartitions; i++) {
			partitions[i].start = label->d_partitions[i].p_offset;
			partitions[i].size  = label->d_partitions[i].p_size;
		}
		current_npart = label->d_npartitions;

		return 0;
	}

	/* Try Human68K-style partition table */
#if 0
	/* assumes 512byte/sec */
	error = IOCS_S_READ(DOSPARTOFF, 2, id, current_blklen, buffer);
#else
	error = IOCS_S_READ(8 >> current_blklen, 8 >> current_blklen,
			    id, current_blklen, buffer);
#endif
	if (error < 0)
		return EIO;
	parttbl = (void*) (buffer + DOSBBSECTOR);
	if (strncmp (buffer, "X68K", 4) != 0)
		return EUNLAB;
	parttbl++;
	for (current_npart = 0, i = 0;
	     current_npart < MAXPARTITIONS && i < 15 && parttbl[i].dp_size;
	     i++) {
		partitions[current_npart].start
			= parttbl[i].dp_start * 2;
		partitions[current_npart].size
			= parttbl[i].dp_size  * 2;
		if (++current_npart == RAW_PART) {
			partitions[current_npart].start = 0;
			partitions[current_npart].size = -1; /* XXX */
			current_npart++;
		}
	}
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
sd_getpartition (int id, unsigned parttop)
{
	int error, i;

	error = readdisklabel(id);
	if (error) {
		printf ("Reading disklabel: %s\n", strerror(error));
		return -1;
	}

	for (i = 0; i < current_npart; i++) {
		if (partitions[i].start == parttop)
			return i;
	}

	printf ("Could not determine the boot partition.\n");

	return -1;
}

struct sdcd_softc {
	int			sc_part;
	struct boot_partinfo	sc_partinfo;
	int			sc_blocksize;
};

int
sdopen (struct open_file *f, int id, int part)
{
	int error;
	struct sdcd_softc *sc;

	if (id < 0 || id > 7)
		return ENXIO;
	if (current_id != id) {
		error = readdisklabel(id);
		if (error)
			return error;
	}
	if (part >= current_npart)
		return ENXIO;

	sc = alloc (sizeof (struct sdcd_softc));
	sc->sc_part = part;
	sc->sc_partinfo = partitions[part];
	sc->sc_blocksize = current_blklen << 9;
	f->f_devdata = sc;
	return 0;
}

int
sdclose (struct open_file *f)
{
	free (f->f_devdata, sizeof (struct sdcd_softc));
	return 0;
}

int
sdstrategy (void *arg, int rw, daddr_t dblk, size_t size,
	    void *buf, size_t *rsize)
{
	struct sdcd_softc *sc = arg;
	u_int32_t	start = sc->sc_partinfo.start + dblk;
	size_t		nblks;
	int		error;

	if (size == 0) {
		if (rsize)
			*rsize = 0;
		return 0;
	}
	nblks = howmany (size, 256 << current_blklen);

	if (dblk & 0x1fffff == 0x1fffff && (nblks & 0xff) == nblks) {
		if (rw & F_WRITE)
			error = IOCS_S_WRITE (start, nblks, current_id,
					      current_blklen, buf);
		else
			error = IOCS_S_READ (start, nblks, current_id,
					     current_blklen, buf);
	} else {
		if (rw & F_WRITE)
			error = IOCS_S_WRITEEXT (start, nblks, current_id,
						 current_blklen, buf);
		else
			error = IOCS_S_READEXT (start, nblks, current_id,
						 current_blklen, buf);
	}
	if (error < 0)
		return EIO;

	if (rsize)
		*rsize = size;
	return 0;
}

int
cdopen (struct open_file *f, int id, int part)
{
	int error;
	struct sdcd_softc *sc;

	if (id < 0 || id > 7)
		return ENXIO;
	if (part == 0 || part == 2)
		return ENXIO;
	if (current_id != id) {
		error = check_unit(id);
		if (error)
			return error;
	}

	sc = alloc (sizeof (struct sdcd_softc));
	current_npart = 3;
	sc->sc_part = 0;
	sc->sc_partinfo.size = sc->sc_partinfo.size = current_devsize;
	sc->sc_blocksize = current_blklen << 9;
	f->f_devdata = sc;
	return 0;
}

int
cdclose (struct open_file *f)
{
	free (f->f_devdata, sizeof (struct sdcd_softc));
	return 0;
}

int
cdstrategy (void *arg, int rw, daddr_t dblk, size_t size,
	    void *buf, size_t *rsize)
{
	struct sdcd_softc *sc = arg;

	return sdstrategy (arg, rw, dblk * DEV_BSIZE / sc->sc_blocksize,
			   size, buf, rsize);
}
