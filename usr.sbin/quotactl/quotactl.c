/* $NetBSD: quotactl.c,v 1.3 2011/03/24 17:05:47 bouyer Exp $ */
/*-
  * Copyright (c) 2011 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: quotactl.c,v 1.3 2011/03/24 17:05:47 bouyer Exp $");
#endif /* not lint */

/*
 * Quota report
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <prop/proplib.h>
#include <sys/quota.h>

#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <quota/quotaprop.h>

void usage(void);

#define READ_SIZE 4096

int
main(int argc, char * const argv[])
{
	int xflag = 0;
	int Dflag = 0;
	FILE *f = NULL;
	int ch;
	char *plist, *p;
	size_t plistsize;
	int status = 0, error;
	struct plistref pref;
	prop_dictionary_t qdict, cmd;
	prop_array_t cmds;
	prop_object_iterator_t cmditer;
	const char *mp, *xmlfile = NULL;

	while ((ch = getopt(argc, argv, "Dx")) != -1) {
		switch (ch) {
		case 'x':
			xflag++;
			break;
		case 'D':
			Dflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0 || argc > 2) {
		usage();
	} else if (argc == 1) {
		xmlfile = "<stdin>";
		f = stdin;
	} else {
		xmlfile = argv[1];
		f = fopen(xmlfile, "r");
		if (f == NULL)
			err(1, "%s", xmlfile);
	}
	mp = argv[0];

	plist = malloc(READ_SIZE);
	if (plist == NULL)
		err(1, "alloc buffer");
	p = plist;
	plistsize = READ_SIZE;
	while (fread(p, sizeof(char), READ_SIZE, f) == READ_SIZE) {
		plist = realloc(plist, plistsize + READ_SIZE);
		if (plist == NULL)
			err(1, "realloc buffer");
		p = plist + plistsize;
		plistsize += READ_SIZE;
	}
	if (!feof(f))
		err(1, "error reading %s", xmlfile);

	qdict = prop_dictionary_internalize(plist);
	if (qdict == NULL)
		err(1, "can't parse %s", xmlfile);
	free(plist);
	if (!prop_dictionary_send_syscall(qdict, &pref))
		err(1, "can't externalize to syscall");
	if (Dflag) {
		plist = prop_dictionary_externalize(qdict);
		fprintf(stderr, "message to kernel:\n%s\n", plist);
		free(plist);
	}
	prop_object_release(qdict);
	if (quotactl(mp,  &pref) != 0)
		err(1, "quotactl failed");

	if ((error = prop_dictionary_recv_syscall(&pref, &qdict)) != 0) {
		errx(1, "error parsing reply from kernel: %s\n",
		    strerror(error));
	}
	if (Dflag) {
		plist = prop_dictionary_externalize(qdict);
		fprintf(stderr, "reply from kernel:\n%s\n", plist);
		free(plist);
	}
	if (xflag) {
		plist = prop_dictionary_externalize(qdict);
		if (plist == NULL) {
			err(1, "can't print reply");
		}
		printf("%s\n", plist);
		exit(0);
	}
	/* parse the reply, looking for errors */
	if ((error = quota_get_cmds(qdict, &cmds)) != 0) {
		errx(1, "error parsing reply from kernel: %s\n",
		    strerror(error));
	}
	cmditer = prop_array_iterator(cmds);
	if (cmditer == NULL)
		err(1, "prop_array_iterator(cmds)");

	while ((cmd = prop_object_iterator_next(cmditer)) != NULL) {
		const char *cmdstr;
		int8_t error8;
		if (!prop_dictionary_get_cstring_nocopy(cmd, "command",
		    &cmdstr))
			err(1, "error retrieving command");
		if (!prop_dictionary_get_int8(cmd, "return", &error8))
			err(1, "error retrieving return value for command %s",
			    cmdstr);
		if (error8) {
			warnx("command %s failed: %s", cmdstr,
			    strerror(error8));
			status = 2;
		}
	}
	exit(status);
}


void
usage(void)
{
	fprintf(stderr, "usage: %s [-D] [-x] mount point [file]\n",
	    getprogname());
	exit(1);
}
