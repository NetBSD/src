/*	$NetBSD: file2swp.c,v 1.1.1.1 2002/02/27 20:14:39 leo Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "libtos.h"
#include "diskio.h"
#include "ahdilbl.h"
#include "cread.h"

char		*Infile = "minifs.gz";
const char	version[] = "$Revision: 1.1.1.1 $";

extern const char	*program_name;

int		main    PROTO((int, char **));
static void	usage   PROTO((void)) NORETURN;

static void
usage()
{
	eprintf("Usage: %s [OPTIONS] DISK\n"
		"where OPTIONS are:\n"
		"\t-V         display version information\n"
		"\t-f FILE    File to copy. The FILE may be a gzipped file.\n"
		"\t           If not specified, it defaults to minifs.gz.\n"
		"\t-h         display this help and exit\n"
		"\t-o FILE    send output to FILE instead of stdout\n"
		"\t-w         wait for key press before exiting\n\n"
		"DISK is the concatenation of BUS, TARGET and LUN.\n"
		"BUS is one of `i' (IDE), `a' (ACSI) or `s' (SCSI).\n"
		"TARGET and LUN are one decimal digit each. LUN must\n"
		"not be specified for IDE devices and is optional for\n"
		"ACSI/SCSI devices (if omitted, LUN defaults to 0).\n\n"
		"Examples:  a0  refers to ACSI target 0 lun 0\n"
		"           s21 refers to SCSI target 2 lun 1\n"
		, program_name);
	xexit(EXIT_SUCCESS);
}

int
main(argc, argv)
	int		argc;
	char		**argv;
{
	extern int	optind;
	extern char	*optarg;

	disk_t		*dd;
	ptable_t	pt;
	int		rv, c, i, fd;
	u_int32_t	currblk;
	char		buf[AHDI_BSIZE];

	i = rv = 0;
	init_toslib(*argv);

	while ((c = getopt(argc, argv, "Vf:ho:w")) != -1) {
		switch (c) {
		  case 'f':
			Infile = optarg;
			break;
		  case 'o':
			redirect_output(optarg);
			break;
		  case 'w':
		  	set_wait_for_key();
			break;
		  case 'V':
			error(-1, "%s", version);
			break;
			/* NOT REACHED */
		  case 'h':
		  default:
			usage();
			/* NOT REACHED */
		}
	}
	argv += optind;

	if (!*argv) {
		error(-1, "missing DISK argument");
		usage();
		/* NOT REACHED */
	}
	dd = disk_open(*argv);
	pt.nparts = 0;
	pt.parts  = NULL;

	if (!ahdi_getparts(dd, &pt, AHDI_BBLOCK, AHDI_BBLOCK)) {
		for (i = 0; i < pt.nparts; i++) {
			if (!strncmp(pt.parts[i].id, "SWP", 3))
				break;
		}
		if (i == pt.nparts) {
			eprintf("No swap ('SWP') partition found!\n");
			xexit(1);
		}
	}
	else xexit(1);
	if ((fd = open(Infile, O_RDONLY)) < 0) {
		eprintf("Unable to open <%s>\n", Infile);
		xexit(1);
	}
	
	eprintf("Found Swap ('SWP') partition start: %d, end: %d.\n",
				pt.parts[i].start, pt.parts[i].end);
	switch(key_wait("Are you sure (y/n)? ")) {
	  case 'y':
	  case 'Y':
		currblk = pt.parts[i].start;
		while(c = read(fd, buf, sizeof(buf)) > 0) {
		    if (disk_write(dd, currblk, 1, buf) < 0) {
			eprintf("Error writing to swap partition\n");
			xexit(1);
		    }
		    if (++currblk >= pt.parts[i].end) {
			eprintf("Error: filesize exceeds swap "
							"partition size\n");
			xexit(1);
		    }
		}
		close(fd);
		eprintf("Ready\n");
		xexit(0);
		break;
	  default :
		eprintf("Aborted\n");
		break;
	}
	rv = EXIT_FAILURE;
	return(rv);
}
