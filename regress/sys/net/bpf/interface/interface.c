/*	$NetBSD: interface.c,v 1.1 2005/12/25 22:07:01 rpaulo Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rui Paulo.
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

#include <errno.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <net/bpf.h>
#include <net/if.h>

int
main(int argc, char *argv[])
{
	int fd,i ;
	u_int testint;
	struct bpf_dltlist testdlt;
	struct bpf_program testprog;
	struct bpf_stat teststat;
	struct bpf_version testversion;
	struct ifreq testif;
	struct timeval testtv;

	struct ioctls_lists {
		unsigned long req;
		void *argumentp;
	} ioctls[] = {
		{ BIOCGBLEN, &testint },
		{ BIOCGDLT, &testint },
		{ BIOCGDLTLIST, &testdlt },
		{ BIOCFLUSH, NULL },
		{ BIOCPROMISC, NULL },
		{ BIOCGETIF, &testif },
		{ BIOCGRTIMEOUT, &testtv },
		{ BIOCGSTATS, &teststat },
		{ BIOCIMMEDIATE, &testint },
		{ BIOCSETF, &testprog },
		{ BIOCVERSION, &testversion },
		{ BIOCGHDRCMPLT, &testint },
		{ BIOCGSEESENT, &testint },
		{ -1, NULL }
	};
	int nfailed;

	fd = open("/dev/bpf", O_RDWR, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "/dev/bpf");

	strcpy(testif.ifr_name, "lo0");
	nfailed = 0;

	/* set no filter or remove the filter */
	testprog.bf_len = 0;
	testprog.bf_insns = NULL;

	if (ioctl(fd, BIOCSETIF, &testif) < 0)
		err(EXIT_FAILURE, "lo0");

	for (i = 0; ioctls[i].req != -1; i++) 
		if (ioctl(fd, ioctls[i].req, ioctls[i].argumentp) < 0) {
		        fprintf(stderr, "0x%lx:\tFAILED (%s)\n",
			    ioctls[i].req, strerror(errno));
			switch (ioctls[i].req) {
			case BIOCGDLTLIST:
			case BIOCPROMISC:
				/* both should fail on lo0 */
				break;
			default:
				nfailed++;
			}
		} else
			printf("0x%lx:\tOK\n", ioctls[i].req);

	return nfailed;
}
