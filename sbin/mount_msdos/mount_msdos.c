/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#ifndef lint
static char rcsid[] = "$Id: mount_msdos.c,v 1.5 1994/04/07 02:31:22 cgd Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *progname;

void
usage ()
{
	fprintf (stderr, "usage: %s bdev dir\n", progname);
	exit (1);
}
		
int
main (argc, argv)
int argc;
char **argv;
{
	char *dev;
	char *dir;
	struct msdosfs_args args;
	int c;
	extern char *optarg;
	extern int optind;
	int opts;

	progname = argv[0];

	opts = 0;

	while ((c = getopt (argc, argv, "F:")) != EOF) {
		switch (c) {
		case 'F':
			opts |= atoi (optarg);
			break;
		default:
			usage ();
		}
	}

	if (optind + 2 != argc)
		usage ();

	dev = argv[optind];
	dir = argv[optind + 1];

	(void)memset(&args, '\0', sizeof(args));
	args.fspec = dev;

	if (mount (MOUNT_MSDOS, dir, opts, &args) < 0) {
		perror ("mount");
		exit (1);
	}

	exit (0);
}
