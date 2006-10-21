/*	$NetBSD: mime_attach.c,v 1.1 2006/10/21 21:37:21 christos Exp $	*/

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

#ifdef MIME_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: mime_attach.c,v 1.1 2006/10/21 21:37:21 christos Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <magic.h>
#include <setjmp.h>
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


#if 0
#ifndef __lint__
/*
 * XXX - This block for debugging only and eventually should go away.
 */
static void
show_name(const char *prefix, struct name *np)
{
	int i;

	i = 0;
	for (/* EMPTY */; np; np = np->n_flink) {
		(void)printf("%s[%d]: %s\n", prefix, i, np->n_name);
		i++;
	}
}

PUBLIC void
show_attach(const char *prefix, struct attachment *ap)
{
	int i;
	i = 1;
	for (/* EMPTY */; ap; ap = ap->a_flink) {
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

static inline
int is_text(const char *ctype)
{
	return ctype &&
	    strncasecmp(ctype, "text/", sizeof("text/") - 1) == 0;
}

static const char *
content_encoding_core(void *fh, const char *ctype)
{
	int c, lastc;
	int ctrlchar, endwhite;
	size_t curlen, maxlen;
	
	curlen = 0;
	maxlen = 0;
	ctrlchar = 0;
	endwhite = 0;
	lastc = EOF;
	while ((c = fgetc(fh)) != EOF) {
		curlen++;
		
		if (c == '\0' || (lastc == '\r' && c != '\n'))
			return MIME_TRANSFER_BASE64;

		if (c > 0x7f) {
			if (is_text(ctype))
				return MIME_TRANSFER_QUOTED;
			else
				return MIME_TRANSFER_BASE64;
		}
		if (c == '\n') {
			if (isblank((unsigned char)lastc))
				endwhite = 1;
			if (curlen > maxlen)
				maxlen = curlen;
			curlen = 0;
		}
		else if ((c < 0x20 && c != '\t') || c == 0x7f)
			ctrlchar = 1;
		
		lastc = c;
	}
	if (lastc == EOF) /* no characters read */
		return MIME_TRANSFER_7BIT;

	if (lastc != '\n' || ctrlchar || endwhite || maxlen > line_limit())
		return MIME_TRANSFER_QUOTED;

	return MIME_TRANSFER_7BIT;
}

static const char *
content_encoding_by_name(const char *filename, const char *ctype)
{
	FILE *fp;
	const char *enc;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		warn("content_encoding_by_name: %s", filename);
		return MIME_TRANSFER_BASE64;	/* safe */
	}
	enc = content_encoding_core(fp, ctype);
	(void)fclose(fp);
	return enc;
}

static const char *
content_encoding_by_fileno(int fd, const char *ctype)
{
	FILE *fp;
	const char *encoding;
	off_t cur_pos;

	cur_pos = lseek(fd, (off_t)0, SEEK_CUR);
	fp = fdopen(fd, "r");
	if (fp == NULL) {
		warn("content_encoding_by_fileno");
		return MIME_TRANSFER_BASE64;
	}
	encoding = content_encoding_core(fp, ctype);
	(void)lseek(fd, cur_pos, SEEK_SET);
	return encoding;
}

static const char *
content_encoding(struct attachment *attach, const char *ctype)
{
	switch (attach->a_type) {
	case ATTACH_FNAME:
		return content_encoding_by_name(attach->a_name, ctype);
	case ATTACH_MSG:
		errx(EXIT_FAILURE, "msgno not supported yet\n");
		/* NOTREACHED */
	case ATTACH_FILENO:
		return content_encoding_by_fileno(attach->a_fileno, ctype);
	default:
		errx(EXIT_FAILURE, "invalid attach type: %d\n", attach->a_type);
	}
	/* NOTREACHED */
}



/************************
 * Content type routines
 */
/*
 * We use libmagic(3) to get the content type, except in the case of a
 * 0 or 1 byte file, which we declare "text/plain".
 */
static const char *
content_type_by_name(const char *filename)
{
	const char *cp;
	magic_t magic;
	struct stat sb;

	/*
	 * libmagic produces annoying results on very short files.
	 * Note: a 1-byte file always consists of a newline, so size
	 * determines all here.
	 */
	if ((filename != NULL && stat(filename, &sb) == 0) ||
	    (filename == NULL && fstat(0, &sb) == 0))
		if (sb.st_size < 2 && S_ISREG(sb.st_mode))
			return "text/plain";
		       
	magic = magic_open(MAGIC_MIME);
	if (magic == NULL) {
		warn("magic_open: %s", magic_error(magic));
		return NULL;
	}
	if (magic_load(magic, NULL) != 0) {
		warn("magic_load: %s", magic_error(magic));
		return NULL;
	}
	cp = magic_file(magic, filename);
	if (cp == NULL) {
		warn("magic_load: %s", magic_error(magic));
		return NULL;
	}
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
content_type(struct attachment *attach)
{
	switch (attach->a_type) {
	case ATTACH_FNAME:
		return content_type_by_name(attach->a_name);
	case ATTACH_MSG:
		return "message/rfc822";
	case ATTACH_FILENO:
		return content_type_by_fileno(attach->a_fileno);
	default:
		/* This is a coding error! */
		assert(/* CONSTCOND */ 0);
		return NULL;
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
		char *buf;
		char *disp;
		(void)easprintf(&buf, "attachment; filename=\"%s\"", basename(ap->a_name));
		disp = savestr(buf);
		free(buf);
		return disp;
	}
	case ATTACH_MSG:
	case ATTACH_FILENO:
		return "inline";
		
	default:
		return NULL;
	}
}

/*ARGSUSED*/ 
static const char *
content_id(struct attachment *attach __unused)
{
	/* XXX - to be written. */
	
	return NULL;
}

static const char *
content_description(struct attachment *attach, int attach_num)
{
	if (attach_num) {
		char *description;
		char *cp;
		(void)easprintf(&cp, "attachment %d", attach_num);
		description = savestr(cp);
		free(cp);
		return description;
	}
	else
		return attach->a_Content.C_description;
}

/*******************************************
 * Routines to get the MIME content strings.
 */
static struct Content
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

/*
 * A hook to complete the content part of the attachment struct.  This
 * is used when a file is attached by the '-a' to complete the process
 * after the flags have been processed.
 */
PUBLIC void
mime_attach_content(struct attachment *ap)
{
	int i;
	i = 1;

	for (/* EMPTY */; ap; ap = ap->a_flink)
		ap->a_Content = get_mime_content(ap, i++);
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
		fi = fopen(ap->a_name, "r");
		if (fi == NULL)
			err(EXIT_FAILURE, "fopen: %s", ap->a_name);
		break;

	case ATTACH_FILENO:
		fi = fdopen(ap->a_fileno, "r");
		if (fi == NULL)
			err(EXIT_FAILURE, "fdopen: %d", ap->a_fileno);
		break;

	case ATTACH_MSG:
	default:
		errx(EXIT_FAILURE, "unsupported attachment type");
	}

	fput_body(fi, fo, Cp);

	if (ap->a_type == ATTACH_FNAME)
		(void)fclose(fi);
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
	    (*nfo = Fdopen(fd, "w")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", tempname);
		return -1;
	}
	(void)rm(tempname);
	if ((fd2 = dup(fd)) == -1 ||
	    (*nfi = Fdopen(fd2, "r")) == NULL) {
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
	return(nfi);
}

static char*
check_filename(char *filename, char *canon_name)
{
	int fd;
	struct stat sb;
	char *fname = filename;
	/*
	 * 1) check that filename is really a file.
	 * 2) check that filename is readable.
	 * 3) allocate an attachment structure.
	 * 4) save cananonical name for filename, so cd won't screw things later.
	 * 5) add the structure to the end of the chain.
	 */

	if (fname[0] == '~' && fname[1] == '/') {
		char *home = getenv("HOME");
		/*
		 * XXX - we could use the global 'homedir' here if we
		 * move tinit() before getopt() in main.c
		 */
		if (home && home[0] != '~')
			(void)easprintf(&fname, "%s/%s", home, fname + 2);
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
attach_one_file(struct attachment *attach, char *filename, int attach_num)
{
	char canon_name[MAXPATHLEN];
	struct attachment *ap, *nap;

	if (check_filename(filename, canon_name) == NULL)
		return NULL;
	
	nap = csalloc(1, sizeof(*nap));
	nap->a_type = ATTACH_FNAME;
	nap->a_name = savestr(canon_name);

	if (attach == NULL)
		attach = nap;
	else {		
		for (ap = attach; ap->a_flink != NULL; ap = ap->a_flink)
			continue;
		ap->a_flink = nap;
		nap->a_blink = ap;
	}

	if (attach_num)
		nap->a_Content = get_mime_content(nap, attach_num);

	return attach;
}


static jmp_buf intjmp;
/*ARGSUSED*/
static void
sigint(int signum __unused)
{
	siglongjmp(intjmp, 1);
}

static char *
get_line(el_mode_t *em, const char *pr, const char *str, int i)
{
	sig_t saveint;
	char *cp;
	char *line;
	char *prompt;

	saveint = signal(SIGINT, sigint);
	if (sigsetjmp(intjmp, 1)) {
		(void)signal(SIGINT, saveint);
		(void)putc('\n', stdout);
		return __UNCONST("");
	}

	/* Don't use a '\t' in the format string here as completion
	 * seems to handle it badly. */
	(void)easprintf(&prompt, "#%-8d%s: ", i, pr);
	line = my_gets(em, prompt, __UNCONST(str));
	/* LINTED */
	line = line ? savestr(line) : __UNCONST("");
	free(prompt);

	(void)signal(SIGINT, saveint);

	/* strip trailing white space */
	for (cp = line + strlen(line) - 1;
	     cp >= line && isblank((unsigned char)*cp); cp--)
		*cp = '\0';

	/* skip leading white space */
	cp = skip_white(line);

	return cp;
}

static void
sget_line(el_mode_t *em, const char *pr, const char **str, int i)
{
	char *line;
	line = get_line(em, pr, *str, i);
	if (strcmp(line, *str) != 0)
		*str = savestr(line);
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
			ename = *str;
		}
		else {
			if (strcmp(ename, *str) != 0)
				*str = savestr(ename);
			break;
		}
	}
}

static struct attachment *
edit_attachments(struct attachment *attach)
{
	char canon_name[MAXPATHLEN];
	struct attachment *ap;
	char *line;
	int i;

	(void)printf("Attachments:\n");

	i = 1;
	ap = attach;
	while (ap) {
		line = get_line(&elm.filec, "filename", ap->a_name, i);
		if (*line == '\0') {	/* omit this attachment */
			if (ap->a_blink)
				ap->a_blink->a_flink = ap->a_flink;
			else
				attach = ap->a_flink;
		}
		else {
			if (strcmp(line, ap->a_name) != 0) { /* new filename */
				if (check_filename(line, canon_name) == NULL)
					continue;
				ap->a_name = savestr(canon_name);
				ap->a_Content = get_mime_content(ap, 0);
			}
			sget_line(&elm.string, "description", &ap->a_Content.C_description, i);
			sget_encoding(&ap->a_Content.C_encoding, ap->a_name, ap->a_Content.C_type, i);
		}
		i++;
		if (ap->a_flink == NULL)
			break;

		ap = ap->a_flink;
	}

	do {
		struct attachment *nap;

		line = get_line(&elm.filec, "filename", "", i);
		if (*line == '\0')
			break;

		nap = attach_one_file(ap, line, i);
		if (nap == NULL)
			continue;

		if (ap)
			ap = ap->a_flink;
		else
			ap = attach = nap;

		sget_line(&elm.string, "description", &ap->a_Content.C_description, i);
		sget_encoding(&ap->a_Content.C_encoding, ap->a_name, ap->a_Content.C_type, i);
		i++;

	} while (ap);

	return attach;
}

/*
 * Hook used by the '-a' flag and '~@' escape to attach files.
 */
PUBLIC struct attachment*
mime_attach_files(struct attachment *attach, char *linebuf, int with_content)
{
	struct attachment *ap;
	char *argv[MAXARGC];
	int argc;
	int attach_num;

	argc = getrawlist(linebuf, argv, sizeof(argv)/sizeof(*argv));

	attach_num = attach ? 1 : 0;
	for (ap = attach; ap && ap->a_flink; ap = ap->a_flink)
			attach_num++;

	if (argc) {
		int i;
		for (i = 0; i < argc; i++) {
			attach_num++;
			ap = attach_one_file(ap, argv[i], with_content ? attach_num : 0);
			if (attach == NULL)
				attach = ap;
		}
	}
	else {
		attach = edit_attachments(attach);
		(void)printf("--- end attachments ---\n");
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
