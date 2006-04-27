/*	$NetBSD: ckckp.c,v 1.3 2006/04/27 22:37:54 perseant Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

int main(int argc, char **argv)
{
	int fd, e, sno;
	char cmd[BUFSIZ], s[BUFSIZ];
	FILE *pp;

	if (argc < 5)
		errx(1, "usage: %s <fs-root> <raw-dev> <save-filename> "
		     "<work-filename>\n", argv[0]);

	fd = open(argv[1], 0, 0);
	if (fd < 0)
		err(1, argv[1]);

	fcntl(fd, LFCNWRAPGO, NULL);
	sleep(5);

	/* Loop forever calling LFCNWRAP{STOP,GO} */
	sno = 0;
	while(1) {
		printf("Waiting until fs wraps\n");
		fcntl(fd, LFCNWRAPSTOP, NULL);

		/*
		 * When the fcntl exits, the wrap is about to occur (but
		 * is waiting for the signal to go).  Call our mass-check
		 * script, and if all is well, continue.  The output
		 * of the script should end with a line that begins with a
		 * numeric code: zero for okay, nonzero for a failure.
		 */
		printf("Verifying all checkpoints from s/n %d\n", sno);
		sprintf(cmd, "./check-all %s %s %s %d", argv[2], argv[3],
			argv[4], sno);
		pp = popen(cmd, "r");
		s[0] = '\0';
		while(fgets(s, BUFSIZ, pp) != NULL)
			printf("  %s", s);
		if (s[0] == '\0') {
			printf("No checkpoints found or script exited\n");
			return 0;
		}
		sscanf(s, "%d %d", &e, &sno);
		if (e) {
			return 0;
		}
		pclose(pp);

		++sno;
		printf("Waiting until fs continues\n");
		fcntl(fd, LFCNWRAPGO, NULL);
	}

	return 0;
}
