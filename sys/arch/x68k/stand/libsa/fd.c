/*	$NetBSD: fd.c,v 1.1 2001/09/27 10:03:28 minoura Exp $	*/

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
#include <lib/libsa/stand.h>

#include "fdvar.h"
#include "iocs.h"


int
fdopen (struct open_file *f, int id, int part)
{
	int error;
	struct fd_softc *sc;
	struct fdfmt fdfmt;

	if (id < 0 || id > 3)
		return ENXIO;
	sc = alloc(sizeof (struct fd_softc));

	/* lock the medium */
	error = IOCS_B_DRVCHK((0x90 + id) << 8, 2);
	if ((error & 2) == 0)
		return ENXIO;

	/* detect the sector size */
	error = IOCS_B_RECALI((0x90 + id) << 8);
	error = fd_check_format (id, 0, &sc->fmt);
	if (error < 0) {
		IOCS_B_DRVCHK((0x90 + id) << 8, 3); /* unlock */
		free(sc, sizeof (struct fd_softc));
		return -error;
	}

	/* check the second side */
	error = fd_check_format (id, 1, &fdfmt);
	if (error == 0)		/* valid second side; set the #heads */
		sc->fmt.maxsec.H = fdfmt.maxsec.H;

	sc->unit = id;
	f->f_devdata = sc;

	return 0;
}

int
fdclose (struct open_file *f)
{
	struct fd_softc *sc = f->f_devdata;

	IOCS_B_DRVCHK((0x90 + sc->unit) << 8, 3);
	free (sc, sizeof (struct fd_softc));
	return 0;
}

int
fdstrategy (void *arg, int rw, daddr_t dblk, size_t size,
	    void *buf, size_t *rsize)
{
	struct fd_softc *sc = arg;
	int cyl, head, sect;
	int nhead, nsect;
	int error, nbytes;

	if (size == 0) {
		if (rsize)
			*rsize = 0;
		return 0;
	}
	nbytes = howmany (size, 128 << sc->fmt.minsec.N)
		* (128 << sc->fmt.minsec.N);

	nhead = sc->fmt.maxsec.H - sc->fmt.minsec.H + 1;
	nsect = sc->fmt.maxsec.R - sc->fmt.minsec.R + 1;

	sect = 	dblk % nsect + sc->fmt.minsec.R;
	head = (dblk / nsect) % nhead + sc->fmt.minsec.H;
	cyl = (dblk / nsect) / nhead + sc->fmt.minsec.C;

	error = IOCS_B_READ((sc->unit+0x90)*256 + 0x70,
			    ((sc->fmt.minsec.N << 24) |
			     (cyl << 16) |
			     (head << 8) |
			     (sect)),
			    nbytes,
			    buf);
	if (error & 0xf8ffff00) {
		nbytes = 0;
		error = EIO;
	} else
		error = 0;

	if (rsize)
		*rsize = nbytes;
	return error;
}
