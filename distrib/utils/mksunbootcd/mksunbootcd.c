/*	$NetBSD: mksunbootcd.c,v 1.2 1999/02/12 15:04:01 kleink Exp $	*/

/*
 * Copyright (c) 1998 Ignatios Souvatzis
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Ignatios Souvatzis
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * add bootable Sun partitions to a CD-ROM image
 */

#include <sys/types.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <dev/sun/disklabel.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * How we work:
 * 
 * a) get length of cd image
 * b) if there is already a disk image, complain (later: allow to add or 
 *	overwrite old ufs images)
 * c) get length of miniroots
 * d) round all the lengthes to some convenient size, set that as cylinder
 *    size. (Sun uses 640 sector (320 k) cylinders)
 * e) create label
 * f) seek to start of cd image
 * g) write label
 * h) foreach ufs image
 * h1) seek to its start
 * h2) copy it it over
 */

void usage __P((void));
int main __P((int, char *[]));

#define Dprintf(x) if (debug) printf x
#define Vprintf(x) if (verbose) printf x

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern int optreset;

	struct sun_disklabel sdl;
	int ch, debug, verbose;
	int cylsz;
	u_short *sp, sum;
	int pfd[8], npart;	/* cd filesystem descriptor; partition	*/
#define cdfd pfd[0]		/* fs descriptors & info		*/
	int psz[8], totalsz;
	int pstart[8];
	struct stat sbs[8];
	char buf[8192];
	int i, j, nothing, rc, rc2;

	/* code starts here */
	verbose = debug = 0;
	cylsz = 640;	/* in 512-sector units */

	while ((ch = getopt(argc, argv, "vdc:")) != -1)
		switch(ch) {
		case 'c':
			cylsz = atoi(optarg);
			if (cylsz % 4)
				errx(1, "cylinder size must be multiple of 4");
			break;
		case 'd':
			debug = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
	}

	argc -= optind;
	argv += optind;

	npart = argc;
	if (npart < 1)
		usage();

	if (npart > 8)
		npart = 8;

	Vprintf(("%d partitions.\n", npart ));
	for (i = 0; i < npart; i++) {
		pfd[i] = open(argv[i], i ? O_RDONLY : O_RDWR, 0666);
		if (pfd[i] == -1)
			err(2, argv[i]);

		rc = fstat(pfd[i], &sbs[i]);
		if (rc)
			err(2, "can't get %s status", argv[i]);

		Dprintf(("%-30s: size %10d", argv[i], (int)sbs[i].st_size));

		psz[i] = (sbs[i].st_size + 512 * cylsz - 1) / (512 * cylsz);
		psz[i] *= cylsz;
		
		Dprintf((" rounded to %5ds\n", psz[i]));
	}

	pstart[0] = 0;
	Dprintf(("\n%-30s: start %10dc\n", argv[0], pstart[0]));
	
	for (i = 1; i < npart; i++) {
		for (nothing = 1, j = 1; j < i; j++) {
			if (sbs[i].st_ino == sbs[j].st_ino &&
			    sbs[i].st_dev == sbs[j].st_dev) {	/* same as me */
				pstart[i] = pstart[j];
				nothing = 0;
				break;
			}
		}
		if (nothing)
			pstart[i] = pstart[i-1] + psz[i-1] / cylsz;
		Dprintf(("%-30s: start %10dc\n", argv[i], pstart[i]));
	}
	totalsz = pstart[npart-1] + psz[npart-1] / cylsz;

	/* initialize label template */
	(void)memset(&sdl, 0, sizeof(sdl));
	sdl.sl_magic = htons(SUN_DKMAGIC);
	sdl.sl_rpm = htons(300);
	sdl.sl_nsectors = htons(cylsz);
	sdl.sl_ntracks = htons(1);
	sdl.sl_pcylinders = sdl.sl_ncylinders = htons(totalsz);
	sdl.sl_acylinders = htons(0);
	sdl.sl_sparespercyl = htons(0);
	sdl.sl_interleave = htons(0);

	for (i = 0; i < npart; i++) {
		sdl.sl_part[i].sdkp_cyloffset = htonl(pstart[i]);
		sdl.sl_part[i].sdkp_nsectors = htonl(psz[i]);
	}

	/* insert geometry */
	
	/* insert partition info */
	/* compute checksum */
	sp = (u_short *)&sdl;
	sum = 0;
	while (sp < (u_short *)((&sdl)+1) - 1) {
		sum ^= *sp++;	/* XXX no need to ntohs/htons for XOR */
	}
	sdl.sl_cksum = sum;
	Dprintf(("label size is %d\n", sizeof(sdl)));
	Dprintf(("cksum computed is 0x%04x\n", ntohs(sum)));

	/* copy partition data to cd image */
	for (i = 1; i < npart; i++) {
		rc = lseek(pfd[0],
		    (off_t)pstart[i] * (off_t)cylsz * (off_t)512, SEEK_SET);

		if (rc < 0)
			err(2, "positioning cd image");
		
		while ((rc = read(pfd[i], buf, sizeof(buf))) > 0) {
			rc2 = write(pfd[0], buf, rc);
			if (rc2 < 0)
				err(2, "writing");
			if (rc2 < rc)
				errx(2, "short write (partition data)");
		}

		if (rc < 0)
			err(2, "reading");
	}

	/* ensure even size in case we rounded the partition size earlier */
	rc = ftruncate(pfd[0], (off_t) cylsz * (off_t)totalsz * (off_t)512);
	if (rc < 0)
		err(2, "truncating cd image");

	/*
	 * Write label now. Partition data isn't valid before the partitions 
	 * are written!
	 */

	rc = lseek(pfd[0], 0, SEEK_SET);
	if (rc)
		err(2, "rewinding cd image");

	rc = write(pfd[0], (void *)&sdl, sizeof(sdl));
	if (rc < 0)
		err(2, "writing label");

	if (rc < sizeof(sdl))
		errx(2, "short write on label write");

	exit(0);
}

void
usage() 
{

	errx(1, "Usage: mksunbootcd [-v] [-c cylsize] cdimage part1 part2...");
}
