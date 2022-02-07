/*	$NetBSD: blkdiscard.c,v 1.1 2022/02/07 09:33:26 mrg Exp $	*/

/*
 * Copyright (c) 2019, 2020, 2022 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* from: $eterna: fdiscard-stuff.c,v 1.3 2020/12/25 23:40:19 mrg Exp $	*/

/* fdiscard(2) front-end -- TRIM support. */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

static bool secure = false;
static bool zeroout = false;
static char *zeros = NULL;

#define FDISCARD_VERSION	20220206u

static void __dead
usage(const char* msg)
{
	FILE *out = stdout;
	int rv = 0;

	if (msg) {
		out = stderr;
		rv = 1;
		fprintf(out, "%s", msg);
	}
	fprintf(out, "Usage: blkdiscard [-hnRsVvz] [-l length] "
		     "[-m chunk_bytes]\n"
		     "                  [-o first_byte] <file>\n");
	exit(rv);
}

static void __dead
version(void)
{

	printf("NetBSD blkdiscard %u\n", FDISCARD_VERSION);
	exit(0);
}

static void
write_one(int fd, off_t discard_size, off_t first_byte)
{

	if (secure)
		/* not yet */;
	else if (zeroout) {
		if (pwrite(fd, zeros, discard_size, first_byte) != discard_size)
			err(1, "pwrite");
	} else if (fdiscard(fd, first_byte, discard_size) != 0)
		err(1, "fdiscard");
}

int
main(int argc, char *argv[])
{
	off_t first_byte = 0, end_offset = 0;
	/* doing a terabyte at a time leads to ATA timeout issues */
	off_t max_per_call = 32 * 1024 * 1024;
	off_t discard_size;
	off_t size = 0;
	off_t length = 0;
	int64_t val;
	int i;
	bool verbose = false;

	/* Default /sbin/blkdiscard to be "run" */
	bool norun = strcmp(getprogname(), "blkdiscard") != 0;

	while ((i = getopt(argc, argv, "f:hl:m:no:p:RsvVz")) != -1) {
		switch (i) {
		case 'l':
			if (dehumanize_number(optarg, &val) == -1 || val < 0)
				usage("bad -s\n");
			length = val;
			break;
		case 'm':
		case 'p':
			if (dehumanize_number(optarg, &val) == -1 || val < 0)
				usage("bad -m\n");
			max_per_call = val;
			break;
		case 'f':
		case 'o':
			if (dehumanize_number(optarg, &val) == -1 || val < 0)
				usage("bad -f\n");
			first_byte = val;
			break;
		case 'n':
			norun = true;
			break;
		case 'R':
			norun = false;
			break;
		case 's':
			secure = true;
			break;
		case 'V':
			version();
		case 'v':
			verbose = true;
			break;
		case 'z':
			zeroout = true;
			break;
		case 'h':
			usage(NULL);
		default:
			usage("bad options\n");
		}
	}
	argc -= optind;
	argv += optind;

	if (secure)
		usage("blkdiscard: secure erase not yet implemnted\n");
	if (zeroout) {
		zeros = calloc(1, max_per_call);
		if (!zeros)
			errx(1, "Unable to allocate %lld bytes for zeros",
			    (long long)max_per_call);
	}
	
	if (length)
		end_offset = first_byte + length;

	if (argc != 1)
		usage("not one arg left\n");

	char *name = argv[0];
	int fd = open(name, O_RDWR);
	if (fd < 0)
		err(1, "open on %s", name);

	if (size == 0) {
		struct dkwedge_info dkw;
		if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == 0) {
			size = dkw.dkw_size * DEV_BSIZE;
			if (verbose)
				printf("%s: wedge size is %lld\n", name,
				    (long long)size);
		}
	}

	if (size == 0) {
		struct disklabel dl;
		if (ioctl(fd, DIOCGDINFO, &dl) != -1) {
			char partchar = name[strlen(name)-1];
			assert(partchar >= 'a' && partchar <= 'p');
			int part = partchar - 'a';

			size = (uint64_t)dl.d_partitions[part].p_size *
			    dl.d_secsize;
			if (verbose)
				printf("%s: disklabel size is %lld\n", name,
				    (long long)size);
		}
	}

	if (size == 0) {
		struct stat sb;
		if (fstat(fd, &sb) != -1) {
			size = sb.st_size;
			if (verbose)
				printf("%s: stat size is %lld\n", name,
				    (long long)size);
		}
	}

	if (size == 0) {
		fprintf(stderr, "unable to determine size.\n");
		exit(1);
	}

	size -= first_byte;

	if (end_offset) {
		if (end_offset > size) {
			fprintf(stderr, "end_offset %lld > size %lld\n",
			    (long long)end_offset, (long long)size);
			usage("");
		}
		size = end_offset;
	}

	if (verbose)
		printf("%sgoing to %s on %s from byte %lld for "
		       "%lld bytes, %lld at a time\n",
		       norun ? "not " : "",
		       secure ? "secure erase" :
		       zeroout ? "zero out" : "fdiscard(2)",
		       name, (long long)first_byte, (long long)size,
		       (long long)max_per_call);

	int loop = 0;
	while (size > 0) {
		if (size > max_per_call)
			discard_size = max_per_call;
		else
			discard_size = size;

		if (!norun)
			write_one(fd, discard_size, first_byte);

		first_byte += discard_size;
		size -= discard_size;
		if (verbose) {
			printf(".");
			fflush(stdout);
			if (loop++ > 100) {
				loop = 0;
				printf("\n");
			}
		}
	}
	if (loop != 0)
		printf("\n");

	if (close(fd) != 0)
		warnx("close failed: %s", strerror(errno));

	return 0;
}
