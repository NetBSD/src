/*	$NetBSD: ofhandlers.c,v 1.5.12.1 2014/08/20 00:05:07 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <dev/ofw/openfirmio.h>

#include "defs.h"

extern	char *path_openfirm;
extern	int eval;
extern	int verbose;

static	char err_str[BUFSIZE];

static	void of_notsupp(const struct extabent *, struct ofiocdesc *, char *);
static	void of_uint32h(const struct extabent *, struct ofiocdesc *, char *);
static	void of_uint32d(const struct extabent *, struct ofiocdesc *, char *);

/*
 * There are several known fields that I either don't know how to
 * deal with or require special treatment.
 */
static	const struct extabent ofextab[] = {
	{ "security-password",		of_notsupp },
	{ "security-mode",		of_notsupp },
	{ "oem-logo",			of_notsupp },
	{ "oem-banner",			of_notsupp },
	{ "real-base",			of_uint32h },
	{ "real-size",			of_uint32h },
	{ "load-base",			of_uint32h },
	{ "virt-base",			of_uint32h },
	{ "virt-size",			of_uint32h },
	{ "screen-#columns",		of_uint32d },
	{ "screen-#rows",		of_uint32d },
	{ "selftest-#megs",		of_uint32d },
	{ NULL,				of_notsupp },
};

#define BARF(str1, str2) {						\
	snprintf(err_str, sizeof err_str, "%s: %s", (str1), (str2));	\
	++eval;								\
	return (err_str);						\
};

void
of_action(char *keyword, char *arg)
{
	char	*cp;

	if ((cp = of_handler(keyword, arg)) != NULL)
		warnx("%s", cp);
	return;
}

char *
of_handler(char *keyword, char *arg)
{
	struct ofiocdesc ofio;
	const struct extabent *ex;
	char ofio_buf[BUFSIZE];
	int fd, optnode;

	if ((fd = open(path_openfirm, arg ? O_RDWR : O_RDONLY, 0640)) < 0)
		BARF(path_openfirm, strerror(errno));

	/* Check to see if it's a special-case keyword. */
	for (ex = ofextab; ex->ex_keyword != NULL; ++ex)
		if (strcmp(ex->ex_keyword, keyword) == 0)
			break;

	if (ioctl(fd, OFIOCGETOPTNODE, (char *)&optnode) < 0) {
		(void)close(fd);
		BARF("OFIOCGETOPTNODE", strerror(errno));
	}

	memset(&ofio_buf[0], 0, sizeof(ofio_buf));
	memset(&ofio, 0, sizeof(ofio));
	ofio.of_nodeid = optnode;
	ofio.of_name = keyword;
	ofio.of_namelen = strlen(ofio.of_name);

	if (arg) {
		if (verbose) {
			printf("old: ");

			ofio.of_buf = &ofio_buf[0];
			ofio.of_buflen = sizeof(ofio_buf);
			if (ioctl(fd, OFIOCGET, (char *)&ofio) < 0) {
				(void)close(fd);
				BARF("OFIOCGET", strerror(errno));
			}

			if (ofio.of_buflen <= 0) {
				printf("nothing available for %s\n", keyword);
				goto out;
			}

			if (ex->ex_keyword != NULL)
				(*ex->ex_handler)(ex, &ofio, NULL);
			else
				printf("%s\n", ofio.of_buf);
		}
 out:
		if (ex->ex_keyword != NULL)
			(*ex->ex_handler)(ex, &ofio, arg);
		else {
			ofio.of_buf = arg;
			ofio.of_buflen = strlen(arg);
		}

		if (ioctl(fd, OFIOCSET, (char *)&ofio) < 0) {
			(void)close(fd);
			BARF("invalid keyword", keyword);
		}

		if (verbose) {
			printf("new: ");
			if (ex->ex_keyword != NULL)
				(*ex->ex_handler)(ex, &ofio, NULL);
			else
				printf("%s\n", ofio.of_buf);
		}
	} else {
		ofio.of_buf = &ofio_buf[0];
		ofio.of_buflen = sizeof(ofio_buf);
		if (ioctl(fd, OFIOCGET, (char *)&ofio) < 0) {
			(void)close(fd);
			BARF("OFIOCGET", strerror(errno));
		}

		if (ofio.of_buflen <= 0) {
			(void)snprintf(err_str, sizeof err_str,
			    "nothing available for %s", keyword);
			return (err_str);
		}

		if (ex->ex_keyword != NULL)
			(*ex->ex_handler)(ex, &ofio, NULL);
		else
			printf("%s=%s\n", keyword, ofio.of_buf);
	}

	(void)close(fd);
	return (NULL);
}

/* ARGSUSED */
static void
of_notsupp(const struct extabent *exent, struct ofiocdesc *ofiop, char *arg)
{

	warnx("property `%s' not yet supported", exent->ex_keyword);
}

static void
of_uint32h(const struct extabent *exent, struct ofiocdesc *ofiop, char *arg)
{

	printf("%s=0x%08x\n", exent->ex_keyword, *(uint32_t *)ofiop->of_buf);
}

static void
of_uint32d(const struct extabent *exent, struct ofiocdesc *ofiop, char *arg)
{

	printf("%s=%d\n", exent->ex_keyword, *(uint32_t *)ofiop->of_buf);
}

/*
 * XXX: This code is quite ugly.  You have been warned.
 * (Really!  This is the only way I could get it to work!)
 */
void
of_dump(void)
{
	struct ofiocdesc ofio1, ofio2;
	const struct extabent *ex;
	char buf1[BUFSIZE], buf2[BUFSIZE], buf3[BUFSIZE], buf4[BUFSIZE];
	int fd, optnode;

	if ((fd = open(path_openfirm, O_RDONLY, 0640)) < 0)
		err(1, "open: %s", path_openfirm);

	if (ioctl(fd, OFIOCGETOPTNODE, (char *)&optnode) < 0)
		err(1, "OFIOCGETOPTNODE");

	memset(&ofio1, 0, sizeof(ofio1));

	/* This will grab the first property name from OPIOCNEXTPROP. */
	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	ofio1.of_nodeid = ofio2.of_nodeid = optnode;

	ofio1.of_name = buf1;
	ofio1.of_buf = buf2;

	ofio2.of_name = buf3;
	ofio2.of_buf = buf4;

	/*
	 * For reference: ofio1 is for obtaining the name.  Pass the
	 * name of the last property read in of_name, and the next one
	 * will be returned in of_buf.  To get the first name, pass
	 * an empty string.  There are no more properties when an
	 * empty string is returned.
	 *
	 * ofio2 is for obtaining the value associated with that name.
	 * For some crazy reason, it seems as if we need to do all
	 * of that gratuitious zapping and copying.  *sigh*
	 */
	for (;;) {
		ofio1.of_namelen = strlen(ofio1.of_name);
		ofio1.of_buflen = sizeof(buf2);

		if (ioctl(fd, OFIOCNEXTPROP, (char *)&ofio1) < 0) {
			close(fd);
			return;
			/* err(1, "ioctl: OFIOCNEXTPROP"); */
		}

		/*
		 * The name of the property we wish to get the
		 * value for has been stored in the value field
		 * of ofio1.  If the length of the name is 0, there
		 * are no more properties left.
		 */
		strcpy(ofio2.of_name, ofio1.of_buf);	/* XXX strcpy is safe */
		ofio2.of_namelen = strlen(ofio2.of_name);

		if (ofio2.of_namelen == 0) {
			(void)close(fd);
			return;
		}

		memset(ofio2.of_buf, 0, sizeof(buf4));
		ofio2.of_buflen = sizeof(buf4);

		if (ioctl(fd, OFIOCGET, (char *)&ofio2) < 0)
			err(1, "ioctl: OFIOCGET");

		for (ex = ofextab; ex->ex_keyword != NULL; ++ex)
			if (strcmp(ex->ex_keyword, ofio2.of_name) == 0)
				break;

		if (ex->ex_keyword != NULL)
			(*ex->ex_handler)(ex, &ofio2, NULL);
		else
			printf("%s=%s\n", ofio2.of_name, ofio2.of_buf);

		/*
		 * Place the name of the last read value back into
		 * ofio1 so that we may obtain the next name.
		 */
		memset(ofio1.of_name, 0, sizeof(buf1));
		memset(ofio1.of_buf, 0, sizeof(buf2));
		strcpy(ofio1.of_name, ofio2.of_name);	/* XXX strcpy is safe */
	}
	/* NOTREACHED */
}
