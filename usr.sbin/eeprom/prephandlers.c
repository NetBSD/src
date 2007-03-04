/*	$NetBSD: prephandlers.c,v 1.1.2.2 2007/03/04 14:17:07 bouyer Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour.
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
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <machine/nvram.h>

#include "defs.h"

extern char *path_prepnvram;
extern int eval;
extern int verbose;

static char err_str[BUFSIZE];

static void prep_notsupp(struct extabent *, struct pnviocdesc *, char *);

/*
 * XXX
 * This file needs a recalc checksum routine, and a routine to call a
 * write back to nvram.  The prep NVRAM is stored in RAM after boot, to
 * prevent horrific bitbanging on the NVRAM chip, we read and write into RAM
 * until a save operation is called.  At that time we write the ram-based
 * NVRAM back to the physical NVRAM.  This is a fairly expensive operation.
 */


/*
 * There are several known fields that I either don't know how to
 * deal with or require special treatment.
 */
static struct extabent prepextab[] = {
	{NULL, prep_notsupp},
};
#define BARF(str1, str2) {						\
	snprintf(err_str, sizeof err_str, "%s: %s", (str1), (str2));	\
	++eval;								\
	return (err_str);						\
};

void
prep_action(char *keyword, char *arg)
{
	char *cp;

	if ((cp = prep_handler(keyword, arg)) != NULL)
		warnx("%s", cp);
	return;
}

char *
prep_handler(char *keyword, char *arg)
{
	struct pnviocdesc nvio;
	struct extabent *ex;
	char nvio_buf[BUFSIZE];
	int fd;

	if ((fd = open(path_prepnvram, arg ? O_RDWR : O_RDONLY, 0640)) < 0)
		BARF(path_prepnvram, strerror(errno));

	/* Check to see if it's a special-case keyword. */
	for (ex = prepextab; ex->ex_keyword != NULL; ++ex)
		if (strcmp(ex->ex_keyword, keyword) == 0)
			break;

	memset(&nvio_buf[0], 0, sizeof(nvio_buf));
	memset(&nvio, 0, sizeof(nvio));
	nvio.pnv_name = keyword;
	nvio.pnv_namelen = strlen(nvio.pnv_name);

	if (arg) {
		if (verbose) {
			printf("old: ");

			nvio.pnv_buf = &nvio_buf[0];
			nvio.pnv_buflen = sizeof(nvio_buf);
			if (ioctl(fd, PNVIOCGET, (char *) &nvio) < 0)
				BARF("PNVIOCGET", strerror(errno));

			if (nvio.pnv_buflen <= 0) {
				printf("nothing available for %s\n", keyword);
				goto out;
			}
			if (ex->ex_keyword != NULL)
				(*ex->ex_handler) (ex, &nvio, NULL);
			else
				printf("%s\n", nvio.pnv_buf);
		}
out:
		if (ex->ex_keyword != NULL)
			(*ex->ex_handler) (ex, &nvio, arg);
		else {
			nvio.pnv_buf = arg;
			nvio.pnv_buflen = strlen(arg);
		}

		if (ioctl(fd, PNVIOCSET, (char *) &nvio) < 0)
			BARF("invalid keyword", keyword);

		if (verbose) {
			printf("new: ");
			if (ex->ex_keyword != NULL)
				(*ex->ex_handler) (ex, &nvio, NULL);
			else
				printf("%s\n", nvio.pnv_buf);
		}
	} else {
		nvio.pnv_buf = &nvio_buf[0];
		nvio.pnv_buflen = sizeof(nvio_buf);
		if (ioctl(fd, PNVIOCGET, (char *) &nvio) < 0)
			BARF("PNVIOCGET", strerror(errno));

		if (nvio.pnv_buflen <= 0) {
			(void) snprintf(err_str, sizeof err_str,
			    "nothing available for %s", keyword);
			return (err_str);
		}
		if (ex->ex_keyword != NULL)
			(*ex->ex_handler) (ex, &nvio, NULL);
		else
			printf("%s=%s\n", keyword, nvio.pnv_buf);
	}

	(void) close(fd);
	return (NULL);
}
/* ARGSUSED */
static void
prep_notsupp(struct extabent * exent, struct pnviocdesc * nviop, char *arg)
{

	warnx("property `%s' not yet supported", exent->ex_keyword);
}
/*
 * Derrived from op_dump().  This should dump the contents of the PReP
 * NVRAM chip.
 */

void
prep_dump()
{
	struct pnviocdesc nvio1, nvio2;
	char buf1[BUFSIZE], buf2[BUFSIZE], buf3[BUFSIZE], buf4[BUFSIZE];
	int fd, optnode, nrofvars;

	if ((fd = open(path_prepnvram, O_RDONLY, 0640)) < 0)
		err(1, "open: %s", path_prepnvram);

	memset(&nvio1, 0, sizeof(nvio1));

	if (ioctl(fd, PNVIOCGETNUMGE, (char *) &nvio1) < 0)
		err(1, "PNVIOCGETNUMGE");

	nrofvars = nvio1.pnv_num;
	if (nrofvars < 1)
		err(1, "PNVIOCGETNUMGE");

	memset(&nvio1, 0, sizeof(nvio1));
	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));
	memset(buf3, 0, sizeof(buf3));
	memset(buf4, 0, sizeof(buf4));
	nvio1.pnv_name = NULL;
	nvio1.pnv_buf = buf2;
	nvio2.pnv_name = buf3;
	nvio2.pnv_buf = buf4;
	for (optnode = 0; optnode < nrofvars; optnode++) {
		nvio1.pnv_buflen = sizeof(buf2);

		if (ioctl(fd, PNVIOCGETNEXTNAME, (char *) &nvio1) < 0)
			err(1, "PNVIOCGETNEXTNAME");

		if (optnode == 0)
			nvio1.pnv_name = buf1;

		/* the property name is now stored in nvio1.pnv_buf */
		strcpy(nvio2.pnv_name, nvio1.pnv_buf);	/* safe */
		nvio2.pnv_namelen = strlen(nvio2.pnv_name);

		if (nvio2.pnv_namelen == 0) {
			(void) close(fd);
			return;
		}
		memset(nvio2.pnv_buf, 0, sizeof(buf4));
		nvio2.pnv_buflen = sizeof(buf4);

		if (ioctl(fd, PNVIOCGET, (char *) &nvio2) < 0)
			err(1, "PNVIOCGET");

		printf("%s=%s\n", nvio1.pnv_buf, nvio2.pnv_buf);

		/* now clean out the buffers for the next loop */

		memset(nvio1.pnv_name, 0, sizeof(buf1));
		memset(nvio1.pnv_buf, 0, sizeof(buf2));
		strcpy(nvio1.pnv_name, nvio2.pnv_name);	/* safe */
		nvio1.pnv_namelen = strlen(nvio1.pnv_name);
	}
	close(fd);
}
