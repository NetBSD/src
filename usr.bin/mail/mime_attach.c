/*	$NetBSD: mime_attach.c,v 1.16 2013/01/04 01:54:55 christos Exp $	*/

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
__RCSID("$NetBSD: mime_attach.c,v 1.16 2013/01/04 01:54:55 christos Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <magic.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "def.h"
#include "extern.h"
#ifdef USE_EDITLINE
#include "complete.h"
#endif
#ifdef MIME_SUPPORT
#include "mime.h"
#include "mime_codecs.h"
#include "mime_child.h"
#endif
#include "glob.h"
#include "sig.h"

#if 0
/*
 * XXX - This block is for debugging only and eventually should go away.
 */
# define SHOW_ALIST(a,b) show_alist(a,b)
static void
show_alist(struct attachment *alist, struct attachment *ap)
{
	(void)printf("alist=%p ap=%p\n", alist, ap);
	for (ap = alist; ap; ap = ap->a_flink) {
		(void)printf("ap=%p ap->a_flink=%p ap->a_blink=%p ap->a_name=%s\n",
		    ap, ap->a_flink, ap->a_blink, ap->a_name ? ap->a_name : "<null>");
	}
}
#else
# define SHOW_ALIST(a,b)
#endif

#if 0
#ifndef __lint__ /* Don't lint: the public routines may not be used. */
/*
 * XXX - This block for is debugging only and eventually should go away.
 */
static void
show_name(const char *prefix, struct name *np)
{
	int i;

	i = 0;
	for (/*EMPTY*/; np; np = np->n_flink) {
		(void)printf("%s[%d]: %s\n", prefix, i, np->n_name);
		i++;
	}
}

static void fput_mime_content(FILE *fp, struct Content *Cp);

PUBLIC void
show_attach(const char *prefix, struct attachment *ap)
{
	int i;
	i = 1;
	for (/*EMPTY*/; ap; ap = ap->a_flink) {
		(void)printf("%s[%d]:\n", prefix, i);
		fput_mime_content(stdout, &ap->a_Content);
		i++;
	}
}

PUBLIC void
show_header(struct header *hp)
{
	show_name("TO", hp->h_to);
	(void)printf("SUBJECT: %s\n", hp->h_subject);
	show_name("CC", hp->h_cc);
	show_name("BCC", hp->h_bcc);
	show_name("SMOPTS", hp->h_smopts);
	show_attach("ATTACH", hp->h_attach);
}
#endif	/* __lint__ */
#endif

/***************************
 * boundary string routines
 */
static char *
getrandstring(size_t length)
{
	void *vbin;
	uint32_t *bin;
	size_t binlen;
	size_t i;
	char *b64;

	/* XXX - check this stuff again!!! */

	binlen = 3 * roundup(length, 4) / 4;	/* bytes of binary to encode base64 */
	bin = vbin = salloc(roundup(binlen, 4));
	for (i = 0; i < roundup(binlen, 4) / 4; i++)
		bin[i] = arc4random();

	b64 = salloc(roundup(length, 4));
	mime_bintob64(b64, vbin, binlen);
	b64[length] = '\0';

	return b64;
}

/*
 * Generate a boundary for MIME multipart messages.
 */
static char *
make_boundary(void)
{
#define BOUND_LEN 70	/* maximum length is 70 characters: RFC2046 sec 5.1.1 */

	char *bound;
	time_t	now;

	(void)time(&now);
	bound = salloc(BOUND_LEN);
	(void)snprintf(bound, BOUND_LEN, "=_%08lx.%s",
	    (long)now, getrandstring(BOUND_LEN - 12));
	return bound;

#undef BOUND_LEN
}

/***************************
 * Transfer coding routines
 */
/*
 * We determine the recommended transfer encoding type for a file as
 * follows:
 *
 * 1) If there is a NULL byte or a stray CR (not in a CRLF
 *    combination) in the file, play it safe and use base64.
 *
 * 2) If any high bit is set, use quoted-printable if the content type
 *    is "text" and base64 otherwise.
 *
 * 3) Otherwise:
 *    a) use quoted-printable if there are any long lines, control
 *       chars (including CR), end-of-line blank space, or a missing
 *       terminating NL.
 *    b) use 7bit in all remaining case, including an empty file.
 *
 * NOTE: This means that CRLF text (MSDOS) files will be encoded
 * quoted-printable.
 */
/*
 * RFC 821 imposes the following line length limit:
 *  The maximum total length of a text line including the
 *  <CRLF> is 1000 characters (but not counting the leading
 *  dot duplicated for transparency).
 */
#define MIME_UNENCODED_LINE_MAX	(1000 - 2)
static size_t
line_limit(void)
{
	int limit;
	const char *cp;
	limit = -1;

	if ((cp = value(ENAME_MIME_UNENC_LINE_MAX)) != NULL)
		limit = atoi(cp);

	if (limit < 0 || limit > MIME_UNENCODED_LINE_MAX)
		limit = MIME_UNENCODED_LINE_MAX;

	return (size_t)limit;
}

static inline int
is_text(const char *ctype)
{
	return ctype &&
	    strncasecmp(ctype, "text/", sizeof("text/") - 1) == 0;
}

static const char *
content_encoding_core(void *fh, const char *ctype)
{
#define MAILMSG_CLEAN	0x0
#define MAILMSG_ENDWS	0x1
#define MAILMSG_CTRLC	0x2
#define MAILMSG_8BIT	0x4
#define MAILMSG_LONGL	0x8
	int c, lastc, state;
	int ctrlchar, endwhite;
	size_t curlen, maxlen;

	state = MAILMSG_CLEAN;
	curlen = 0;
	maxlen = line_limit();
	ctrlchar = 0;
	endwhite = 0;
	lastc = EOF;
	while ((c = fgetc(fh)) != EOF) {
		curlen++;

		if (c == '\0')
			return MIME_TRANSFER_BASE64;

		if (c > 0x7f) {
			if (!is_text(ctype))
				return MIME_TRANSFER_BASE64;
			state |= MAILMSG_8BIT;
			continue;
		}
		if (c == '\n') {
			if (is_WSP(lastc))
				state |= MAILMSG_ENDWS;
			if (curlen > maxlen)
				state |= MAILMSG_LONGL;
			curlen = 0;
		}
		else if ((c < 0x20 && c != '\t') || c == 0x7f || lastc == '\r')
			state |= MAILMSG_CTRLC;
		lastc = c;
	}
	if (lastc == EOF) /* no characters read */
		return MIME_TRANSFER_7BIT;

	if (lastc != '\n' || state != MAILMSG_CLEAN)
		return MIME_TRANSFER_QUOTED;

	return MIME_TRANSFER_7BIT;
}

static const char *
content_encoding_by_name(const char *filename, const char *ctype)
{
	FILE *fp;
	const char *enc;
	fp = Fopen(filename, "re");
	if (fp == NULL) {
		warn("content_encoding_by_name: %s", filename);
		return MIME_TRANSFER_BASE64;	/* safe */
	}
	enc = content_encoding_core(fp, ctype);
	(void)Fclose(fp);
	return enc;
}

static const char *
content_encoding_by_fileno(int fd, const char *ctype)
{
	FILE *fp;
	int fd2;
	const char *encoding;
	off_t cur_pos;

	cur_pos = lseek(fd, (off_t)0, SEEK_CUR);
	if ((fd2 = dup(fd)) == -1 ||
	    (fp = Fdopen(fd2, "re")) == NULL) {
		warn("content_encoding_by_fileno");
		if (fd2 != -1)
			(void)close(fd2);
		return MIME_TRANSFER_BASE64;
	}
	encoding = content_encoding_core(fp, ctype);
	(void)Fclose(fp);
	(void)lseek(fd, cur_pos, SEEK_SET);
	return encoding;
}

static const char *
content_encoding(struct attachment *ap, const char *ctype)
{
	switch (ap->a_type) {
	case ATTACH_FNAME:
		return content_encoding_by_name(ap->a_name, ctype);
	case ATTACH_MSG:
		return "7bit";
	case ATTACH_FILENO:
		return content_encoding_by_fileno(ap->a_fileno, ctype);
	case ATTACH_INVALID:
	default:
		/* This is a coding error! */
		assert(/* CONSTCOND */ 0);
		errx(EXIT_FAILURE, "invalid attachment type: %d", ap->a_type);
		/* NOTREACHED */
	}
}

/************************
 * Content type routines
 */
/*
 * We use libmagic(3) to get the content type, except in the case of a
 * 0 or 1 byte file where libmagic gives rather useless results.
 */
static const char *
content_type_by_name(char *filename)
{
	const char *cp;
	char *cp2;
	magic_t magic;
	struct stat sb;

#ifdef BROKEN_MAGIC
	/*
	 * libmagic(3) produces annoying results on very short files.
	 * The common case is MIME encoding an empty message body.
	 * XXX - it would be better to fix libmagic(3)!
	 *
	 * Note: a 1-byte message body always consists of a newline,
	 * so size determines all there.  However, 1-byte attachments
	 * (filename != NULL) could be anything, so check those.
	 */
	if ((filename != NULL && stat(filename, &sb) == 0) ||
	    (filename == NULL && fstat(0, &sb) == 0)) {
		if (sb.st_size < 2 && S_ISREG(sb.st_mode)) {
			FILE *fp;
			int ch;

			if (sb.st_size == 0 || filename == NULL ||
			    (fp = Fopen(filename, "re")) == NULL)
				return "text/plain";

			ch = fgetc(fp);
			(void)Fclose(fp);

			return isprint(ch) || isspace(ch) ?
			    "text/plain" : "application/octet-stream";
		}
	}
#endif
	magic = magic_open(MAGIC_MIME);
	if (magic == NULL) {
		warnx("magic_open: %s", magic_error(magic));
		return NULL;
	}
	if (magic_load(magic, NULL) != 0) {
		warnx("magic_load: %s", magic_error(magic));
		return NULL;
	}
	cp = magic_file(magic, filename);
	if (cp == NULL) {
		warnx("magic_load: %s", magic_error(magic));
		return NULL;
	}
	if (filename &&
	    sasprintf(&cp2, "%s; name=\"%s\"", cp, basename(filename)) != -1)
		cp = cp2;
	else
		cp = savestr(cp);
	magic_close(magic);
	return cp;
}

static const char *
content_type_by_fileno(int fd)
{
	const char *cp;
	off_t cur_pos;
	int ofd;

	cur_pos = lseek(fd, (off_t)0, SEEK_CUR);

	ofd = dup(0);		/* save stdin */
	if (dup2(fd, 0) == -1)	/* become stdin */
		warn("dup2");

	cp = content_type_by_name(NULL);

	if (dup2(ofd, 0) == -1)	/* restore stdin */
		warn("dup2");
	(void)close(ofd);	/* close the copy */

	(void)lseek(fd, cur_pos, SEEK_SET);
	return cp;
}

static const char *
content_type(struct attachment *ap)
{
	switch (ap->a_type) {
	case ATTACH_FNAME:
		return content_type_by_name(ap->a_name);
	case ATTACH_MSG:
		/*
		 * Note: the encapusulated message header must include
		 * at least one of the "Date:", "From:", or "Subject:"
		 * fields.  See rfc2046 Sec 5.2.1.
		 * XXX - Should we really test for this?
		 */
		return "message/rfc822";
	case ATTACH_FILENO:
		return content_type_by_fileno(ap->a_fileno);
	case ATTACH_INVALID:
	default:
		/* This is a coding error! */
		assert(/* CONSTCOND */ 0);
		errx(EXIT_FAILURE, "invalid attachment type: %d", ap->a_type);
		/* NOTREACHED */
	}
}

/*************************
 * Other content routines
 */

static const char *
content_disposition(struct attachment *ap)
{
	switch (ap->a_type) {
	case ATTACH_FNAME: {
		char *disp;
		(void)sasprintf(&disp, "attachment; filename=\"%s\"",
		    basename(ap->a_name));
		return disp;
	}
	case ATTACH_MSG:
		return NULL;
	case ATTACH_FILENO:
		return "inline";

	case ATTACH_INVALID:
	default:
		/* This is a coding error! */
		assert(/* CONSTCOND */ 0);
		errx(EXIT_FAILURE, "invalid attachment type: %d", ap->a_type);
		/* NOTREACHED */
	}
}

/*ARGSUSED*/
static const char *
content_id(struct attachment *ap __unused)
{
	/* XXX - to be written. */

	return NULL;
}

static const char *
content_description(struct attachment *attach, int attach_num)
{
	if (attach_num) {
		char *description;
		(void)sasprintf(&description, "attachment %d", attach_num);
		return description;
	}
	else
		return attach->a_Content.C_description;
}

/*******************************************
 * Routines to get the MIME content strings.
 */
PUBLIC struct Content
get_mime_content(struct attachment *ap, int i)
{
	struct Content Cp;

	Cp.C_type	 = content_type(ap);
	Cp.C_encoding	 = content_encoding(ap, Cp.C_type);
	Cp.C_disposition = content_disposition(ap);
	Cp.C_id		 = content_id(ap);
	Cp.C_description = content_description(ap, i);

	return Cp;
}

/******************
 * Output routines
 */
static void
fput_mime_content(FILE *fp, struct Content *Cp)
{
	(void)fprintf(fp, MIME_HDR_TYPE ": %s\n", Cp->C_type);
	(void)fprintf(fp, MIME_HDR_ENCODING ": %s\n", Cp->C_encoding);
	if (Cp->C_disposition)
		(void)fprintf(fp, MIME_HDR_DISPOSITION ": %s\n",
		    Cp->C_disposition);
	if (Cp->C_id)
		(void)fprintf(fp, MIME_HDR_ID ": %s\n", Cp->C_id);
	if (Cp->C_description)
		(void)fprintf(fp, MIME_HDR_DESCRIPTION ": %s\n",
		    Cp->C_description);
}

static void
fput_body(FILE *fi, FILE *fo, struct Content *Cp)
{
	mime_codec_t enc;

	enc = mime_fio_encoder(Cp->C_encoding);
	if (enc == NULL)
		warnx("unknown transfer encoding type: %s\n", Cp->C_encoding);
	else
		enc(fi, fo, 0);
}

static void
fput_attachment(FILE *fo, struct attachment *ap)
{
	FILE *fi;
	struct Content *Cp = &ap->a_Content;

	fput_mime_content(fo, &ap->a_Content);
	(void)putc('\n', fo);

	switch (ap->a_type) {
	case ATTACH_FNAME:
		fi = Fopen(ap->a_name, "re");
		if (fi == NULL)
			err(EXIT_FAILURE, "Fopen: %s", ap->a_name);
		break;

	case ATTACH_FILENO:
		/*
		 * XXX - we should really dup(2) here, however we are
		 * finished with the attachment, so the Fclose() below
		 * is OK for now.  This will be changed in the future.
		 */
		fi = Fdopen(ap->a_fileno, "re");
		if (fi == NULL)
			err(EXIT_FAILURE, "Fdopen: %d", ap->a_fileno);
		break;

	case ATTACH_MSG: {
		char mailtempname[PATHSIZE];
		int fd;

		fi = NULL;	/* appease gcc */
		(void)snprintf(mailtempname, sizeof(mailtempname),
		    "%s/mail.RsXXXXXXXXXX", tmpdir);
		if ((fd = mkstemp(mailtempname)) == -1 ||
		    (fi = Fdopen(fd, "we+")) == NULL) {
			if (fd != -1)
				(void)close(fd);
			err(EXIT_FAILURE, "%s", mailtempname);
		}
		(void)rm(mailtempname);

		/*
		 * This is only used for forwarding, so use the forwardtab[].
		 *
		 * XXX - sendmessage really needs a 'flags' argument
		 * so we don't have to play games.
		 */
		ap->a_msg->m_size--;	/* XXX - remove trailing newline */
		(void)fputc('>', fi);	/* XXX - hide the headerline */
		if (sendmessage(ap->a_msg, fi, forwardtab, NULL, NULL))
			(void)fprintf(stderr, ". . . forward failed, sorry.\n");
		ap->a_msg->m_size++;

		rewind(fi);
		break;
	}
	case ATTACH_INVALID:
	default:
		/* This is a coding error! */
		assert(/* CONSTCOND */ 0);
		errx(EXIT_FAILURE, "invalid attachment type: %d", ap->a_type);
	}

	fput_body(fi, fo, Cp);
	(void)Fclose(fi);
}

/***********************************
 * Higher level attachment routines.
 */

static int
mktemp_file(FILE **nfo, FILE **nfi, const char *hint)
{
	char tempname[PATHSIZE];
	int fd, fd2;
	(void)snprintf(tempname, sizeof(tempname), "%s/%sXXXXXXXXXX",
	    tmpdir, hint);
	if ((fd = mkstemp(tempname)) == -1 ||
	    (*nfo = Fdopen(fd, "we")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", tempname);
		return -1;
	}
	(void)rm(tempname);
	if ((fd2 = dup(fd)) == -1 ||
	    (*nfi = Fdopen(fd2, "re")) == NULL) {
		warn("%s", tempname);
		(void)Fclose(*nfo);
		return -1;
	}
	return 0;
}

/*
 * Repackage the mail as a multipart MIME message.  This should always
 * be called whenever there are attachments, but might be called even
 * if there are none if we want to wrap the message in a MIME package.
 */
PUBLIC FILE *
mime_encode(FILE *fi, struct header *header)
{
	struct attachment map;	/* fake structure for the message body */
	struct attachment *attach;
	struct attachment *ap;
	FILE *nfi, *nfo;

	attach = header->h_attach;

	/*
	 * Make new phantom temporary file with read and write file
	 * handles: nfi and nfo, resp.
	 */
	if (mktemp_file(&nfo, &nfi, "mail.Rs") != 0)
		return fi;

	(void)memset(&map, 0, sizeof(map));
	map.a_type = ATTACH_FILENO;
	map.a_fileno = fileno(fi);

 	map.a_Content = get_mime_content(&map, 0);

	if (attach) {
		/* Multi-part message:
		 * Make an attachment structure for the body message
		 * and make that the first element in the attach list.
		 */
		if (fsize(fi)) {
			map.a_flink = attach;
			attach->a_blink = &map;
			attach = &map;
		}

		/* Construct our MIME boundary string - used by mime_putheader() */
		header->h_mime_boundary = make_boundary();

		(void)fprintf(nfo, "This is a multi-part message in MIME format.\n");

		for (ap = attach; ap; ap = ap->a_flink) {
			(void)fprintf(nfo, "\n--%s\n", header->h_mime_boundary);
			fput_attachment(nfo, ap);
		}

		/* the final boundary with two attached dashes */
		(void)fprintf(nfo, "\n--%s--\n", header->h_mime_boundary);
	}
	else {
		/* Single-part message (no attachments):
		 * Update header->h_Content (used by mime_putheader()).
		 * Output the body contents.
		 */
		char *encoding;

		header->h_Content = map.a_Content;

		/* check for an encoding override */
		if ((encoding = value(ENAME_MIME_ENCODE_MSG)) && *encoding)
			header->h_Content.C_encoding = encoding;

		fput_body(fi, nfo, &header->h_Content);
	}
	(void)Fclose(fi);
	(void)Fclose(nfo);
	rewind(nfi);
	return nfi;
}

static char*
check_filename(char *filename, char *canon_name)
{
	int fd;
	struct stat sb;
	char *fname = filename;

	/* We need to expand '~' if we got here from '~@'.  The shell
	 * does this otherwise.
	 */
	if (fname[0] == '~' && fname[1] == '/') {
		if (homedir && homedir[0] != '~')
			(void)easprintf(&fname, "%s/%s",
			    homedir, fname + 2);
	}
	if (realpath(fname, canon_name) == NULL) {
		warn("realpath: %s", filename);
		canon_name = NULL;
		goto done;
	}
	fd = open(canon_name, O_RDONLY, 0);
	if (fd == -1) {
		warnx("open: cannot read %s", filename);
		canon_name = NULL;
		goto done;
	}
	if (fstat(fd, &sb) == -1) {
		warn("stat: %s", canon_name);
		canon_name = NULL;
		goto do_close;
	}
	if (!S_ISREG(sb.st_mode)) {
		warnx("stat: %s is not a file", filename);
		canon_name = NULL;
	     /*	goto do_close; */
	}
 do_close:
	(void)close(fd);
 done:
	if (fname != filename)
		free(fname);

	return canon_name;
}

static struct attachment *
attach_one_file(struct attachment *ap, char *filename, int attach_num)
{
	char canon_name[MAXPATHLEN];
	struct attachment *nap;

	/*
	 * 1) check that filename is really a readable file; return NULL if not.
	 * 2) allocate an attachment structure.
	 * 3) save cananonical name for filename, so cd won't screw things later.
	 * 4) add the structure to the end of the chain.
	 * 5) return the new attachment structure.
	 */
	if (check_filename(filename, canon_name) == NULL)
		return NULL;

	nap = csalloc(1, sizeof(*nap));
	nap->a_type = ATTACH_FNAME;
	nap->a_name = savestr(canon_name);

	if (ap) {
		for (/*EMPTY*/; ap->a_flink != NULL; ap = ap->a_flink)
			continue;
		ap->a_flink = nap;
		nap->a_blink = ap;
	}

	if (attach_num)
		nap->a_Content = get_mime_content(nap, attach_num);

	return nap;
}

static char *
get_line(el_mode_t *em, const char *pr, const char *str, int i)
{
	char *prompt;
	char *line;

	/*
	 * Don't use a '\t' in the format string here as completion
	 * seems to handle it badly.
	 */
	(void)easprintf(&prompt, "#%-7d %s: ", i, pr);
	line = my_gets(em, prompt, __UNCONST(str));
	if (line != NULL) {
		(void)strip_WSP(line);	/* strip trailing whitespace */
		line = skip_WSP(line);	/* skip leading white space */
		line = savestr(line);	/* XXX - do we need this? */
	}
	else {
		line = __UNCONST("");
	}
	free(prompt);

	return line;
}

static void
sget_line(el_mode_t *em, const char *pr, const char **str, int i)
{
	char *line;
	line = get_line(em, pr, *str, i);
	if (line != NULL && strcmp(line, *str) != 0)
		*str = line;
}

static void
sget_encoding(const char **str, const char *filename, const char *ctype, int num)
{
	const char *ename;
	const char *defename;

	defename = NULL;
	ename = *str;
	for (;;) {
		ename = get_line(&elm.mime_enc, "encoding", ename, num);

		if (*ename == '\0') {
			if (defename == NULL)
				defename = content_encoding_by_name(filename, ctype);
			ename = defename;
		}
		else if (mime_fio_encoder(ename) == NULL) {
			const void *cookie;
			(void)printf("Sorry: valid encoding modes are: ");
			cookie = NULL;
			ename = mime_next_encoding_name(&cookie);
			for (;;) {
				(void)printf("%s", ename);
				ename = mime_next_encoding_name(&cookie);
				if (ename == NULL)
					break;
				(void)fputc(',', stdout);
			}
			(void)putchar('\n');
			ename = *str;
		}
		else {
			if (strcmp(ename, *str) != 0)
				*str = savestr(ename);
			break;
		}
	}
}

/*
 * Edit an attachment list.
 * Return the new attachment list.
 */
static struct attachment *
edit_attachlist(struct attachment *alist)
{
	struct attachment *ap;
	char *line;
	int attach_num;

	(void)printf("Attachments:\n");

	attach_num = 1;
	ap = alist;
	while (ap) {
		SHOW_ALIST(alist, ap);

		switch(ap->a_type) {
		case ATTACH_MSG:
			(void)printf("#%-7d message:  <not changeable>\n",
			    attach_num);
			break;
		case ATTACH_FNAME:
		case ATTACH_FILENO:
			line = get_line(&elm.filec, "filename", ap->a_name, attach_num);
			if (*line == '\0') {	/* omit this attachment */
				if (ap->a_blink) {
					struct attachment *next_ap;
					next_ap = ap->a_flink;
					ap = ap->a_blink;
					ap->a_flink = next_ap;
					if (next_ap)
						next_ap->a_blink = ap;
					else
						goto done;
				}
				else {
					alist = ap->a_flink;
					if (alist)
						alist->a_blink = NULL;
				}
			}
			else {
				char canon_name[MAXPATHLEN];
				if (strcmp(line, ap->a_name) != 0) { /* new filename */
					if (check_filename(line, canon_name) == NULL)
						continue;
					ap->a_name = savestr(canon_name);
					ap->a_Content = get_mime_content(ap, 0);
				}
				sget_line(&elm.string, "description",
				    &ap->a_Content.C_description, attach_num);
				sget_encoding(&ap->a_Content.C_encoding, ap->a_name,
				    ap->a_Content.C_type, attach_num);
			}
			break;
		case ATTACH_INVALID:
		default:
			/* This is a coding error! */
			assert(/* CONSTCOND */ 0);
			errx(EXIT_FAILURE, "invalid attachment type: %d",
			    ap->a_type);
		}

		attach_num++;
		if (alist == NULL || ap->a_flink == NULL)
			break;

		ap = ap->a_flink;
	}

	ap = alist;
	for (;;) {
		struct attachment *nap;

		SHOW_ALIST(alist, ap);

		line = get_line(&elm.filec, "filename", "", attach_num);
		if (*line == '\0')
			break;

		nap = attach_one_file(ap, line, attach_num);
		if (nap == NULL)
			continue;

		if (alist == NULL)
			alist = nap;
		ap = nap;

		sget_line(&elm.string, "description",
		    &ap->a_Content.C_description, attach_num);
		sget_encoding(&ap->a_Content.C_encoding, ap->a_name,
		    ap->a_Content.C_type, attach_num);
		attach_num++;
	}
 done:
	SHOW_ALIST(alist, ap);

	return alist;
}

/*
 * Hook used by the '~@' escape to attach files.
 */
PUBLIC struct attachment*
mime_attach_files(struct attachment * volatile attach, char *linebuf)
{
	struct attachment *ap;
	char *argv[MAXARGC];
	int argc;
	int attach_num;

	argc = getrawlist(linebuf, argv, (int)__arraycount(argv));
	attach_num = 1;
	for (ap = attach; ap && ap->a_flink; ap = ap->a_flink)
			attach_num++;

	if (argc) {
		int i;
		for (i = 0; i < argc; i++) {
			struct attachment *ap2;
			ap2 = attach_one_file(ap, argv[i], attach_num);
			if (ap2 != NULL) {
				ap = ap2;
				if (attach == NULL)
					attach = ap;
				attach_num++;
			}
		}
	}
	else {
		attach = edit_attachlist(attach);
		(void)printf("--- end attachments ---\n");
	}

	return attach;
}

/*
 * Hook called in main() to attach files registered by the '-a' flag.
 */
PUBLIC struct attachment *
mime_attach_optargs(struct name *optargs)
{
	struct attachment *attach;
	struct attachment *ap;
	struct name *np;
	char *expand_optargs;
	int attach_num;

	expand_optargs = value(ENAME_MIME_ATTACH_LIST);
	attach_num = 1;
	ap = NULL;
	attach = NULL;
	for (np = optargs; np; np = np->n_flink) {
		char *argv[MAXARGC];
		int argc;
		int i;

		if (expand_optargs != NULL)
			argc = getrawlist(np->n_name,
			    argv, (int)__arraycount(argv));
		else {
			if (np->n_name == '\0')
				argc = 0;
			else {
				argc = 1;
				argv[0] = np->n_name;
			}
			argv[argc] = NULL;/* be consistent with getrawlist() */
		}
		for (i = 0; i < argc; i++) {
			struct attachment *ap2;
			char *filename;

			if (argv[i][0] == '/')	/* an absolute path */
				(void)easprintf(&filename, "%s", argv[i]);
			else
				(void)easprintf(&filename, "%s/%s",
				    origdir, argv[i]);

			ap2 = attach_one_file(ap, filename, attach_num);
			if (ap2 != NULL) {
				ap = ap2;
				if (attach == NULL)
					attach = ap;
				attach_num++;
			}
			free(filename);
		}
	}
	return attach;
}

/*
 * Output MIME header strings as specified in the header structure.
 */
PUBLIC void
mime_putheader(FILE *fp, struct header *header)
{
	(void)fprintf(fp, MIME_HDR_VERSION ": " MIME_VERSION "\n");
	if (header->h_attach) {
		(void)fprintf(fp, MIME_HDR_TYPE ": multipart/mixed;\n");
		(void)fprintf(fp, "\tboundary=\"%s\"\n", header->h_mime_boundary);
	}
	else {
		fput_mime_content(fp, &header->h_Content);
	}
}

#endif /* MIME_SUPPORT */
