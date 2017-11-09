/*	$NetBSD: mime_detach.c,v 1.9 2017/11/09 20:27:50 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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


#ifdef MIME_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: mime_detach.c,v 1.9 2017/11/09 20:27:50 christos Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "def.h"
#include "extern.h"
#ifdef USE_EDITLINE
#include "complete.h"
#endif
#ifdef MIME_SUPPORT
#include "mime.h"
#include "mime_child.h"
#include "mime_codecs.h"
#include "mime_detach.h"
#endif
#include "sig.h"


static struct {
	int overwrite;
	int batch;
	int ask;
} detach_ctl;

PUBLIC int
mime_detach_control(void)
{
	char *cp;

	detach_ctl.batch     = value(ENAME_MIME_DETACH_BATCH) != NULL;
	detach_ctl.ask       = detach_ctl.batch ? 0 : 1;
	detach_ctl.overwrite = 0;

	cp = value(ENAME_MIME_DETACH_OVERWRITE);
	if (cp == NULL || strcasecmp(cp, "no") == 0)
		detach_ctl.overwrite = 0;

	else if (*cp== '\0' || strcasecmp(cp, "yes") == 0)
		detach_ctl.overwrite = 1;

	else if (strcasecmp(cp, "ask") == 0) {
		detach_ctl.overwrite = 0;
		detach_ctl.ask       = 1;
	}
	else {
		(void)printf("invalid %s setting: %s",
		    ENAME_MIME_DETACH_OVERWRITE, cp);
		return -1;
	}
	return 0;
}

static char *
detach_get_fname(char *prompt, char *pathname)
{
	if (!detach_ctl.batch) {
		char *fname;

		fname = my_gets(&elm.filec, prompt, pathname);
		if (fname == NULL)	/* ignore this attachment */
			return NULL;
		(void)strip_WSP(fname);
		fname = skip_WSP(fname);
		if (*fname == '\0')	/* ignore this attachment */
			return NULL;
		pathname = savestr(fname);	/* save this or it gets trashed */
	}
	else if (detach_ctl.ask)
		(void)printf("%s%s\n", prompt, pathname);

	return pathname;
}

static enum {
	DETACH_OPEN_OK,
	DETACH_NEXT,
	DETACH_RENAME,
	DETACH_FAILED
}
detach_open_core(char *fname, const char *partstr)
{
	int flags;
	int fd;

	flags = (detach_ctl.overwrite ? 0 : O_EXCL) | O_CREAT | O_TRUNC | O_WRONLY;

	if ((fd = open(fname, flags | O_CLOEXEC, 0600)) != -1 &&
	    Fdopen(fd, "wef") != NULL)
		return DETACH_OPEN_OK;

	if (detach_ctl.ask && fd == -1 && errno == EEXIST) {
		char *p;
 start:
		(void)sasprintf(&p, "%-7s overwrite %s: Always/Never/once/next/rename (ANonr)[n]? ",
		    partstr, fname);
		p = my_gets(&elm.string, p, NULL);
		if (p == NULL)
			goto start;

		(void)strip_WSP(p);
		p = skip_WSP(p);

		switch (*p) {
		case 'A':	detach_ctl.overwrite = 1;
				detach_ctl.batch = 1;
				detach_ctl.ask = 0;
				/* FALLTHROUGH */
		case 'o':
			if (Fopen(fname, "wef") != NULL)
				return DETACH_OPEN_OK;
			break;

		case 'N':	detach_ctl.overwrite = 0;
				detach_ctl.batch = 1;
				detach_ctl.ask = 0;
				/* FALLTHROUGH */
		case '\0':	/* default */
		case 'n':	/* Next */
			return DETACH_NEXT;

		default:
			goto start;

		case 'r':	/* Rename */
			return DETACH_RENAME;
		}
	}
	warn("%s", fname);
	if (fd != -1)
		(void)close(fd);

	return DETACH_FAILED;
}

static char *
detach_open_target(struct mime_info *mip)
{
	char *pathname;
	char *prompt;
	const char *partstr;
	const char *subtype;

	/*
	 * XXX: If partstr == NULL, we probably shouldn't be detaching
	 * anything, but let's be liberal and try to do something with
	 * the block anyway.
	 */
	partstr = mip->mi_partstr && mip->mi_partstr[0] ? mip->mi_partstr : "0";
	subtype = mip->mi_subtype ? mip->mi_subtype : "unknown";

	/*
	 * Get the suggested target pathname.
	 */
	if (mip->mi_filename != NULL)
		(void)sasprintf(&pathname, "%s/%s", mip->mi_detachdir,
		    mip->mi_filename);
	else {
		if (mip->mi_detachall == 0)
			return NULL;

		(void)sasprintf(&pathname, "%s/msg-%s.part-%s.%s",
		    mip->mi_detachdir, mip->mi_msgstr,
		    partstr, subtype);
	}

	/*
	 * Make up the prompt
	 */
	(void)sasprintf(&prompt, "%-7s filename: ", partstr);

	/*
	 * The main loop.
	 */
	do {
		struct stat sb;
		char *fname;

		if ((fname = detach_get_fname(prompt, pathname)) == NULL)
			return NULL;
		/*
		 * Make sure we don't have the name of something other
		 * than a normal file!
		 */
		if (stat(fname, &sb) == 0 && !S_ISREG(sb.st_mode)) {
			(void)printf("not a regular file: %s", fname);
			if (!detach_ctl.ask)
				return NULL;
			continue;
		}
		switch (detach_open_core(fname, partstr)) {
		case DETACH_OPEN_OK:
			return fname;
		case DETACH_NEXT:
			return NULL;
		case DETACH_RENAME:
			detach_ctl.batch = 0;
			break;
		case DETACH_FAILED:
			break;
		}
	} while (!detach_ctl.batch);

	return NULL;
}

/*
 * The main entry point for detaching.
 */
PUBLIC FILE *
mime_detach_parts(struct mime_info *mip)
{
	mime_codec_t dec;
	char *pathname;

	if (mip->mi_ignore_body || mip->mp->m_blines == 0)
		return NULL;

	if ((dec = mime_fio_decoder(mip->mi_encoding)) == NULL &&
	    (dec = mime_fio_decoder(MIME_TRANSFER_7BIT)) == NULL)
		assert(/*CONSTCOND*/ 0); /* this should never get hit! */

	if ((pathname = detach_open_target(mip)) == NULL)
		return NULL;

	(void)printf("writing: %s\n", pathname);

	/*
	 * XXX - should we do character set conversion here (done by
	 * run_decoder()), or just run dec()?
	 */
#if 0
	mime_run_function(dec, pipe_end(mip), NULL);
#else
	run_decoder(mip, dec);
#endif
	return pipe_end(mip);
}

/*
 * Set the message part number to be used when constructing a filename
 * for detaching unnamed parts in detach_open_target().  When
 * threading, this is not a single number, hence the string value.
 */
PUBLIC void
mime_detach_msgnum(struct mime_info *mip, const char *msgstr)
{
	for (/*EMPTY*/; mip; mip = mip->mi_flink)
		mip->mi_msgstr = msgstr;
}

#endif /* MIME_SUPPORT */
