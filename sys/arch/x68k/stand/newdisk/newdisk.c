/*-
 * Copyright (c) 1999 Minoura Makoto
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
 *	This product includes software developed by Minoura Makoto.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Create the disk mark for x68k SCSI IPL.
 * It used to be a shell/awk script, but is rewritten in order to be fit with
 * the install kernel.
 *
 * Usage: /usr/mdec/newdisk [-vnfc] [-m /usr/mdec/mboot] /dev/rsd?c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>

char *prog;
char *mboot = MBOOT;
char dev[MAXPATHLEN];
char buf[4096 + 1];

const char copyright[] = "NetBSD/x68k SCSI Primary Boot. "
			 "(C) 1999 by The NetBSD Foundation, Inc.";

int verbose = 0, dry_run = 0, force = 0, check_only = 0;

volatile void usage __P((void));
int main __P((int, char *[]));

volatile void
usage(void)
{
    fprintf(stderr,
	    "Usage: %s [-v] [-n] [-f] [-c] [-m /usr/mdec/mboot] "
	    "/dev/rsdXc\n", prog);
    exit(1);
    /* NOTREACHED */
}

int
main(argc, argv)
    int argc;
    char *argv[];
{
    extern int optind;
    int ch;
    int fd;
    struct disklabel label;

    prog = argv[0];
    while ((ch = getopt(argc, argv, "vnfcm:")) != -1) {
	switch (ch) {
	case 'v':
	    verbose = 1;
	    break;
	case 'n':
	    dry_run = 1;
	    break;
	case 'f':
	    force = 1;
	    break;
	case 'c':
	    check_only = 1;
	    break;
	case 'm':
	    mboot = optarg;
	    break;
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
	usage();

    fd = opendisk(argv[0], O_RDONLY, dev, MAXPATHLEN, 0);
    if (fd < 0)
	err(1, "opening %s", dev);
    if (access(mboot, R_OK) < 0)
	err(1, "checking %s", mboot);

    if (read(fd, buf, 512) < 0)
	err(1, "reading %s", dev);
    if (strncmp(buf, "X68SCSI1", 8) == 0 &&
	!force)
	errx(1, "%s is already marked.  Use -f to overwrite the existing mark.");
    if (check_only)
	return 0;

    if (verbose)
	fprintf(stderr, "Inspecting %s... ", dev);

    if (ioctl(fd, DIOCGDINFO, &label) < 0)
	err(1, "inspecting %s", dev);
    close(fd);
    if (label.d_secsize != 512)
	errx(1, "This type of disk is not supported by NetBSD.");

    if (verbose)
	fprintf(stderr, "total number of sector is %d.\n", label.d_secperunit);

    if (verbose)
	fprintf(stderr, "Building disk mark... ");
    memset(buf, 0, 3072);
#define n label.d_secperunit
    sprintf(buf, "X68SCSI1%c%c%c%c%c%c%c%c%s",
	    2, 0,
	    (n/16777216)%256, (n/65536)%256, (n/256)%256, n%256,
	    1, 0, copyright);
#undef n
    if (verbose)
	fprintf(stderr, "done.\n");

    if (verbose)
	fprintf(stderr, "Merging %s... ", mboot);
    fd = open(mboot, O_RDONLY);
    if (fd < 0)
	err(1, "opening %s", mboot);
    if (read(fd, buf+1024, 1024) < 0)
	err(1, "reading %s", mboot);
    close(fd);
    if (verbose)
	fprintf(stderr, "done.\n");

    if (verbose)
	fprintf(stderr, "Creating an empty partition table... ");
#define n (label.d_secperunit/2)
    sprintf(buf+2048,
	    "X68K%c%c%c%c%c%c%c%c%c%c%c%c",
	    0, 0, 0, 32,
	    (n/16777215)%256, (n/65536)%256, (n/256)%256, n%256,
	    (n/16777215)%256, (n/65536)%256, (n/256)%256, n%256);
#undef n
    if (verbose)
	fprintf(stderr, "done.\n");

    if (dry_run) {
	char filename[MAXPATHLEN] = "/tmp/diskmarkXXXXX";
	fd = mkstemp(filename);
	if (fd < 0)
	    err(1, "opening %s", filename);
	if (write(fd, buf, 4096) < 0)
	    err(1, "writing %s", filename);
	close(fd);
	fprintf(stderr, "Disk mark is kept in %s.\n", filename);
    } else {
	int mode = 1;

	if (verbose)
	    fprintf(stderr, "Writing... ");
	fd = open(dev, O_WRONLY);
	if (fd < 0)
	    err(1, "opening %s", dev);
	if (ioctl(fd, DIOCWLABEL, (char *)&mode) < 0)
	    err(1, "DIOCWLABEL %s", dev);
	if (write(fd, buf, 4096) != 4096) {
	    mode = 0;
	    ioctl(fd, DIOCWLABEL, (char *)&mode);
	    err(1, "DIOCWLABEL %s", dev);
	}
	ioctl(fd, DIOCWLABEL, (char *)&mode);
	if (verbose)
	    fprintf(stderr, "done.\n");
	close(fd);
    }

    return 0;
}
