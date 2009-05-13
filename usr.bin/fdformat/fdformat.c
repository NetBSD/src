/*	$NetBSD: fdformat.c,v 1.15.8.1 2009/05/13 19:19:49 jym Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
 * fdformat: format a floppy diskette, using interface provided in
 * <sys/fdio.h>
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: fdformat.c,v 1.15.8.1 2009/05/13 19:19:49 jym Exp $");
#endif

#include <sys/types.h>
#include <sys/fdio.h>
#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pathnames.h"

static const char *fdb_array[2] = {_PATH_FLOPPYTAB, 0};

#define MASK_NBPS		0x0001
#define MASK_NCYL		0x0002
#define MASK_NSPT		0x0004
#define MASK_NTRK		0x0008
#define MASK_STEPSPERCYL	0x0010
#define MASK_GAPLEN		0x0020
#define MASK_FILLBYTE		0x0040
#define MASK_XFER_RATE		0x0080
#define MASK_INTERLEAVE		0x0100

#define ALLPARMS (MASK_NBPS|MASK_NCYL|MASK_NSPT|MASK_NTRK|MASK_STEPSPERCYL|MASK_GAPLEN|MASK_FILLBYTE|MASK_XFER_RATE|MASK_INTERLEAVE)

static int	confirm(int);
static void	usage(void) __dead;
static int	verify_track(int, int, int, struct fdformat_parms *, char *);

int	main(int, char **);

static int
confirm(int def)
{
	int ch;

	(void)printf(" Yes/no [%c]?", def ? 'y' : 'n');
	ch = getchar();
	switch (ch) {
	case 'y':
	case 'Y':
		return 1;
	case '\n':
		return def;
	case EOF:
	case 'n':
	case 'N':
	default:
		return 0;
	}		
}

static int
verify_track(int fd, int cyl, int trk, struct fdformat_parms *parms, char *buf)
{
	size_t tracksize;
	off_t offset;

	tracksize = parms->nbps * parms->nspt; /* bytes per track */
	offset = tracksize * (cyl * parms->ntrk + trk); /* track offset */

	if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
		(void)printf("- SEEK ERROR\n");
		return 1;
	}
	if ((size_t)read(fd, buf, tracksize) != tracksize) {
		(void)printf("- VERIFY ERROR\n");
		return 1;
	}
	return 0;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-f device] [-t type] [-n] [-B nbps] [-S nspt]\n"
	    "\t[-T ntrk] [-C ncyl] [-P stepspercyl] [-G gaplen]\n"
	    "\t[-F fillbyte] [-X xfer_rate] [-I interleave]\n", getprogname());
	exit(1);
}

#define numarg(which, maskn, op) 				\
	do {							\
		tmplong = strtol(optarg, &tmpcharp, 0); 	\
		if (*tmpcharp != '\0' || tmplong op 0) 		\
		    errx(1,					\
			"Invalid numerical argument `%s' for " 	\
			# which, optarg); 			\
		if (errno == ERANGE && (tmplong == LONG_MIN ||	\
		    tmplong == LONG_MAX)) 			\
		    err(1,					\
			"Bad numerical argument `%s' for " 	\
			# which, optarg); 			\
		parms.which = tmplong; 				\
		parmmask |= MASK_##maskn;			\
	} while (/* CONSTCOND */0)

#define getparm(structname, maskname) 				\
	do {							\
		if (cgetnum(fdbuf, # structname, &tmplong) == -1) \
			errx(1, "Parameter " # structname	\
			    " missing for type `%s'", optarg);	\
		parms.structname = tmplong; 			\
		parmmask |= MASK_ ## maskname;			\
	} while (/* CONSTCOND */0)


#define copyparm(which, mask) 					\
	if ((parmmask & MASK_##mask) == 0) 			\
		parms.which = fetchparms.which

int
main(int argc, char *argv[])
{
	char *fdbuf = NULL, *trackbuf = NULL;
	int errcnt = 0;
	int verify = 1;
	int ch;
	long tmplong;
	int tmpint;
	char *tmpcharp;
	int parmmask = 0;
	struct fdformat_parms parms, fetchparms;
	struct fdformat_cmd cmd;
	const char *filename = _PATH_FLOPPY_DEV;
	int fd;
	int trk, cyl;

	while ((ch = getopt(argc, argv, "f:t:nB:C:S:T:P:G:F:X:I:")) != -1)
		switch (ch) {
		case 't':		/* disk type */
			switch (cgetent(&fdbuf, fdb_array, optarg)) {
			case 0:
				break;
			case 1:
			case -3:
				errx(1, "tc= loop or missing entry entry in "
				     _PATH_FLOPPYTAB " for type %s", optarg);
				break;
			case -1:
				errx(1, "Unknown floppy disk type %s", optarg);
				break;
			default:
				err(1, "Problem accessing " _PATH_FLOPPYTAB);
				break;
			}

			getparm(nbps, NBPS);
			getparm(ncyl, NCYL);
			getparm(nspt, NSPT);
			getparm(ntrk, NTRK);
			getparm(stepspercyl, STEPSPERCYL);
			getparm(gaplen, GAPLEN);
			getparm(fillbyte, FILLBYTE);
			getparm(xfer_rate, XFER_RATE);
			getparm(interleave, INTERLEAVE);
			break;
		case 'f':		/* device name */
			filename = optarg;
			break;
		case 'n':		/* no verify */
			verify = 0;
			break;
		case 'B':
			numarg(nbps, NBPS, <=);
			break;
		case 'C':
			numarg(ncyl, NCYL, <=);
			break;
		case 'S':
			numarg(nspt, NSPT, <=);
			break;
		case 'T':
			numarg(ntrk, NTRK, <=);
			break;
		case 'P':
			numarg(stepspercyl, STEPSPERCYL, <=);
			break;
		case 'G':
			numarg(gaplen, GAPLEN, <=);
			break;
		case 'F':
			numarg(fillbyte, FILLBYTE, <);
			break;
		case 'X':
			numarg(xfer_rate, XFER_RATE, <=);
			break;
		case 'I':
			numarg(interleave, INTERLEAVE, <=);
			break;
		case '?':
		default:
			usage();
		}

	if (optind < argc)
		usage();

	fd = open(filename, O_RDWR);
	if (fd == -1)
		err(1, "Cannot open %s", filename);
	if (ioctl(fd, FDIOCGETFORMAT, &fetchparms) == -1) {
		if (errno == ENOTTY)
			err(1, "Device `%s' does not support floppy formatting",
			    filename);
		else
			err(1, "Cannot fetch current floppy"
			    " formatting parameters");
	}

	copyparm(nbps, NBPS);
	copyparm(ncyl, NCYL);
	copyparm(nspt, NSPT);
	copyparm(ntrk, NTRK);
	copyparm(stepspercyl, STEPSPERCYL);
	copyparm(gaplen, GAPLEN);
	copyparm(fillbyte, FILLBYTE);
	copyparm(xfer_rate, XFER_RATE);
	copyparm(interleave, INTERLEAVE);

	parms.fdformat_version = FDFORMAT_VERSION;

	tmpint = FDOPT_NORETRY|FDOPT_SILENT;
	if (ioctl(fd, FDIOCSETOPTS, &tmpint) == -1 ||
	    ioctl(fd, FDIOCSETFORMAT, &parms) == -1) {
		errx(1, "Cannot set requested formatting parameters:"
		    " %d cylinders, %d tracks, %d sectors of %d bytes",
		    parms.ncyl, parms.ntrk, parms.nspt, parms.nbps);
	}

	(void)printf("Ready to format %s with %d cylinders, %d tracks,"
	    " %d sectors of %d bytes\n(%d KB)",
	    filename, parms.ncyl, parms.ntrk, parms.nspt, parms.nbps,
	    parms.ncyl * parms.ntrk * parms.nspt * parms.nbps / 1024);
	if (!confirm(1))
		errx(1,"Formatting abandoned -- not confirmed.");
	
	if (verify) {
		trackbuf = malloc(parms.nbps * parms.nspt);
		if (trackbuf == NULL)
			warn("Cannot allocate verification buffer");
	}

	cmd.formatcmd_version = FDFORMAT_VERSION;
	for (cyl = 0; (unsigned int)cyl < parms.ncyl; cyl++) {
		cmd.cylinder = cyl;
		for (trk = 0; (unsigned int)trk < parms.ntrk; trk++) {
			cmd.head = trk;
			(void)printf("\rFormatting track %i / head %i ", cyl, trk);
			(void)fflush(stdout);
			if (ioctl(fd, FDIOCFORMAT_TRACK, &cmd) == 0) {
				if (verify)
					errcnt += verify_track(fd, cyl, trk,
					    &parms, trackbuf);
			} else if (errno == EINVAL) {
				(void)putchar('\n');
				errx(1, "Formatting botch at <%d,%d>",
				     cyl, trk);
			} else if (errno == EIO) {
				(void)printf("- IO ERROR\n");
				errcnt++;
			}
		}
	}
	(void)printf("\rFormatting %i tracks total complete.\n",
		parms.ncyl * parms.ntrk);
	if (errcnt)
		errx(1, "%d track formatting error%s",
		    errcnt, errcnt == 1 ? "" : "s");
	return 0;
}
