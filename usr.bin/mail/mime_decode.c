/*	$NetBSD: mime_decode.c,v 1.14.2.1 2009/05/13 19:19:56 jym Exp $	*/

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
__RCSID("$NetBSD: mime_decode.c,v 1.14.2.1 2009/05/13 19:19:56 jym Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iconv.h>

#include "def.h"
#include "extern.h"
#ifdef USE_EDITLINE
#include "complete.h"
#endif
#ifdef MIME_SUPPORT
#include "mime.h"
#include "mime_child.h"
#include "mime_codecs.h"
#include "mime_header.h"
#include "mime_detach.h"
#endif
#include "glob.h"
#include "thread.h"

#if 0
#ifndef __lint__
/*
 * XXX - This block for debugging only and eventually should go away.
 */
static void
show_one_mime_info(FILE *fp, struct mime_info *mip)
{
#define XX(a) (a) ? (a) : "<null>"

	(void)fprintf(fp, ">> --------\n");
	(void)fprintf(fp, "mip %d:\n", mip->mi_partnum);
	(void)fprintf(fp, "** Version: %s\n",  XX(mip->mi_version));
	(void)fprintf(fp, "** type: %s\n",     XX(mip->mi_type));
	(void)fprintf(fp, "** subtype: %s\n",  XX(mip->mi_subtype));
	(void)fprintf(fp, "** boundary: %s\n", XX(mip->mi_boundary));
	(void)fprintf(fp, "** charset: %s\n",  XX(mip->mi_charset));
	(void)fprintf(fp, "** encoding: %s\n", XX(mip->mi_encoding));
	(void)fprintf(fp, "** disposition: %s\n", XX(mip->mi_disposition));
	(void)fprintf(fp, "** filename: %s\n", XX(mip->mi_filename));
	(void)fprintf(fp, "** %p: flag: 0x%x, block: %ld, offset: %d, size: %lld, lines: %ld:%ld\n",
	    mip->mp,
	    mip->mp->m_flag,
	    mip->mp->m_block, mip->mp->m_offset, mip->mp->m_size,
	    mip->mp->m_lines, mip->mp->m_blines);
	(void)fprintf(fp, "** mip: %p\n", mip);
	(void)fprintf(fp, "** mi_flink: %p\n", mip->mi_flink);
	(void)fprintf(fp, "** mi_blink: %p\n", mip->mi_blink);
	(void)fprintf(fp, "** mip %p, mp %p,  parent_mip %p, parent_mp %p\n",
	    mip, mip->mp, mip->mi_parent.mip, mip->mi_parent.mp);

	(void)fprintf(fp, "** mi_fo %p, mi_head_end %p, mi_pipe_end %p\n",
	    mip->mi_fo, mip->mi_head_end, mip->mi_pipe_end);

	(void)fprintf(fp, "** mi_ignore_body: %d\n", mip->mi_ignore_body);
	(void)fprintf(fp, "** mi_partnum: %d\n", mip->mi_partnum);
	(void)fprintf(fp, "** mi_partstr: %s\n", mip->mi_partstr);
	(void)fprintf(fp, "** mi_msgstr: %s\n", mip->mi_msgstr);

	(void)fflush(fp);

#undef XX
}

__unused
static void
show_mime_info(FILE *fp, struct mime_info *mip, struct mime_info *end_mip)
{
	for (/* EMTPY */; mip != end_mip; mip = mip->mi_flink)
		show_one_mime_info(fp, mip);

	(void)fprintf(fp, "++ =========\n");
	(void)fflush(fp);
}
#endif /* __lint__ */
#endif /* #if */


/*
 * Our interface to the file registry in popen.c
 */
PUBLIC FILE *
pipe_end(struct mime_info *mip)
{
	FILE *fp;
	fp = last_registered_file(0);	/* get last registered file or pipe */
	if (fp == NULL)
		fp = mip->mi_fo;
	return fp;
}

/*
 * Copy the first ';' delimited substring from 'src' (null terminated)
 * into 'dst', expanding quotes and removing comments (as per RFC
 * 822).  Returns a pointer in src to the next non-white character
 * following ';'.  The caller is responsible for ensuring 'dst' is
 * sufficiently large to hold the result.
 */
static char *
get_param(char *dst, char *src)
{
	char *lastq;
	char *cp;
	char *cp2;
	int nesting;

	cp2 = dst;
	lastq = dst;
	for (cp = src; *cp && *cp != ';'; cp++) {
		switch (*cp) {
		case '"':	/* start of quoted string */
			for (cp++; *cp; cp++) {
				if (*cp == '"')
					break;
				if (*cp == '\\' && cp[1] != '\0')
					++cp;
				*cp2++ = *cp;
			}
			lastq = cp2-1;
			break;
		case '(':	/* start of comment */
			nesting = 1;
			while (nesting > 0 && *++cp) {
				if (*cp == '\\' && cp[1] != '\0')
					cp++;
				if (*cp == '(')
					nesting++;
				if (*cp == ')')
					nesting--;
			}
			break;
		default:
			*cp2++ = *cp;
			break;
		}
	}
	/* remove trailing white space */
	while (cp2 > lastq && is_WSP(cp2[-1]))
		cp2--;
	*cp2 = '\0';
	if (*cp == ';')
		cp++;
	cp = skip_WSP(cp);
	return cp;
}

/*
 * Content parameter
 *    if field is NULL, return the content "specifier".
 */
static char*
cparam(const char field[], char *src, int downcase)
{
	char *cp;
	char *dst;

	if (src == NULL)
		return NULL;

	dst = salloc(strlen(src) + 1); /* large enough for any param in src */
	cp = skip_WSP(src);
	cp = get_param(dst, cp);

	if (field == NULL)
		return dst;

	while (*cp != '\0') {
		size_t len = strlen(field);
		cp = get_param(dst, cp);
		if (strncasecmp(dst, field, len) == 0 && dst[len] == '=') {
			char *cp2;
			cp2 = dst + len + 1;
			if (downcase)
				istrcpy(cp2, cp2);
			return cp2;
		}
	}
	return NULL;
}


static void
get_content(struct mime_info *mip)
{
	char *mime_disposition_field;
	char *mime_type_field;
	char *filename;
	struct message *mp;
	char *cp;

	mp = mip->mp;
	mip->mi_version  = cparam(NULL, hfield(MIME_HDR_VERSION,  mp), 0);
	mip->mi_encoding = cparam(NULL, hfield(MIME_HDR_ENCODING, mp), 1);

	mime_type_field = hfield(MIME_HDR_TYPE, mp);
	mip->mi_type = cparam(NULL, mime_type_field, 1);
	if (mip->mi_type) {
		cp = strchr(mip->mi_type, '/');
		if (cp)
			*cp++ = '\0';
		mip->mi_subtype = cp;
	}
	mip->mi_boundary = cparam("boundary", mime_type_field, 0);
	mip->mi_charset  = cparam("charset",  mime_type_field, 1);

	mime_disposition_field = hfield(MIME_HDR_DISPOSITION, mp);
	mip->mi_disposition = cparam(NULL, mime_disposition_field, 1);
	/*
	 * The type field typically has a "name" parameter for "image"
	 * and "video" types, and I assume for other types as well.
	 * We grab it, but override it if the disposition field has a
	 * filename parameter as it often does for "attachments".
	 * More careful analysis could be done, but this seems to work
	 * pretty well.
	 */
	filename = cparam("name", mime_type_field, 0);
	if ((cp = cparam("filename", mime_disposition_field, 0)) != NULL)
		filename = cp;
	if (filename) {
		filename = basename(filename);	/* avoid absolute pathnames */
		filename = savestr(filename);	/* save it! */
	}
	mip->mi_filename = filename;
}


static struct message *
salloc_message(int flag, long block, short offset)
{
	struct message *mp;
	/* use csalloc in case someone adds a field someday! */
	mp = csalloc(1, sizeof(*mp));
	mp->m_flag   = flag;
	mp->m_block  = block;
	mp->m_offset = offset;
#if 0
	mp->m_lines  = 0;
	mp->m_size   = 0;
	mp->m_blines = 0;
#endif
	return mp;
}

static struct mime_info *
insert_new_mip(struct mime_info *this_mip, struct mime_info *top_mip,
    struct message *top_mp, off_t end_pos, int partnum)
{
	struct mime_info *new_mip;

	new_mip = csalloc(1, sizeof(*new_mip));
	new_mip->mi_blink = this_mip;
	new_mip->mi_flink = this_mip->mi_flink;
	this_mip->mi_flink = new_mip;

	new_mip->mp = salloc_message(this_mip->mp->m_flag,
	    (long)blockof(end_pos), blkoffsetof(end_pos));

	new_mip->mi_parent.mip = top_mip;
	new_mip->mi_parent.mp = top_mp;
	new_mip->mi_partnum = partnum;

	return new_mip;
}

static void
split_multipart(struct mime_info *top_mip)
{
	FILE *fp;
	struct message *top_mp;
	struct message *this_mp;
	struct mime_info *this_mip;
	off_t beg_pos;
	const char *boundary;
	size_t boundary_len;
	long lines_left;	/* must be signed and same size as m_lines */
	int partnum;
	int in_header;

	top_mp = top_mip->mp;
	this_mp = salloc_message(top_mp->m_flag, top_mp->m_block, top_mp->m_offset);
	this_mip = top_mip;
	this_mip->mp = this_mp;

	partnum = 1;
/*	top_mip->mi_partnum = partnum++;  */ /* Keep the number set by the caller */
	in_header = 1;
	boundary = top_mip->mi_boundary;
	boundary_len = boundary ? strlen(boundary) : 0;

	fp = setinput(top_mp);
	beg_pos = ftello(fp);
#if 0
	warnx("beg_pos: %lld,  m_lines: %ld,  m_blines: %ld",
	    beg_pos, top_mp->m_lines, top_mp->m_blines);
#endif
	for (lines_left = top_mp->m_lines - 1; lines_left >= 0; lines_left--) {
		char *line;
		size_t line_len;

		line = fgetln(fp, &line_len);

		this_mp->m_lines++;		/* count the message lines */

		if (!in_header)
			this_mp->m_blines++;	/* count the body lines */

		if (lines_left == 0 || (
			    !in_header &&
			    line_len >= boundary_len + 2 &&
			    line[0] == '-' && line[1] == '-' &&
			    strncmp(line + 2, boundary, boundary_len) == 0)) {
			off_t cur_pos;
			off_t end_pos;

			cur_pos = ftello(fp);

			/* the boundary belongs to the next part */
			end_pos = cur_pos - line_len;
			this_mp->m_lines  -= 1;
			this_mp->m_blines -= 1;

			this_mp->m_size = end_pos - beg_pos;
#if 0
			warnx("end_pos: %lld,  m_lines: %ld,  m_blines: %ld",
			    end_pos, this_mp->m_lines, this_mp->m_blines);
#endif
			if (line[boundary_len + 2] == '-' &&
			    line[boundary_len + 3] == '-') {/* end of multipart */
				/* do a sanity check on the EOM */
				if (lines_left != 1) {
					/*
					 * XXX - this can happen!
					 * Should we display the
					 * trailing garbage or check
					 * that it is blank or just
					 * ignore it?
					 */
#if 0
					(void)printf("EOM: lines left: %ld\n", lines_left);
#endif
				}
				break;	/* XXX - stop at this point or grab the rest? */
			}
			this_mip = insert_new_mip(this_mip, top_mip, top_mp, end_pos, partnum++);
			this_mp = this_mip->mp;
			this_mp->m_lines = 1; /* already read the first line in the header! */
			beg_pos = end_pos;
			in_header = 1;
		}

		if (line_len == 1)
			in_header = 0;
	}
}

static void
split_message(struct mime_info *top_mip)
{
	struct mime_info *this_mip;
	struct message *top_mp;
	struct message *this_mp;
	FILE *fp;
	off_t beg_pos;
	long lines_left;	/* must be same size as m_lines */
	int in_header;

	top_mp = top_mip->mp;
	this_mp = salloc_message(top_mp->m_flag, top_mp->m_block, top_mp->m_offset);
	this_mip = top_mip;
	this_mip->mp = this_mp;

	in_header = 1;

	fp = setinput(top_mp);
	beg_pos = ftello(fp);

	for (lines_left = top_mp->m_lines; lines_left > 0; lines_left--) {
		size_t line_len;

		(void)fgetln(fp, &line_len);

		this_mp->m_lines++;		/* count the message lines */
		if (!in_header)
			this_mp->m_blines++;	/* count the body lines */

		if (in_header && line_len == 1) { /* end of header */
			off_t end_pos;
			end_pos = ftello(fp);
			this_mp->m_size = end_pos - beg_pos;
			this_mip = insert_new_mip(this_mip, top_mip,top_mp, end_pos, 0);
			this_mp = this_mip->mp;
			this_mp->m_lines = 1; /* we already counted one line in the header! */
			beg_pos = end_pos;
			in_header = 0;	/* never in header again */
		}
	}

	/* close the last message */
	this_mp->m_size = ftello(fp) - beg_pos;
}


static const char *
get_command_hook(struct mime_info *mip, const char *domain)
{
	char *key;
	char *cmd;

	if (mip->mi_type == NULL)
		return NULL;

	/* XXX - should we use easprintf() here?  We are probably
	 * hosed elsewhere if this fails anyway. */

	cmd = NULL;
	if (mip->mi_subtype) {
		if (asprintf(&key, "mime%s-%s-%s",
			domain,	mip->mi_type, mip->mi_subtype) == -1) {
			warn("get_command_hook: subtupe: asprintf");
			return NULL;
		}
		cmd = value(key);
		free(key);
	}
	if (cmd == NULL) {
		if (asprintf(&key, "mime%s-%s", domain, mip->mi_type) == -1) {
			warn("get_command_hook: type: asprintf");
			return NULL;
		}
		cmd = value(key);
		free(key);
	}
	return cmd;
}


static int
is_basic_alternative(struct mime_info *mip)
{
	return
	    strcasecmp(mip->mi_type, "text") == 0 &&
	    strcasecmp(mip->mi_subtype, "plain") == 0;
}

static struct mime_info *
select_alternative(struct mime_info *top_mip, struct mime_info *end_mip)
{
	struct mime_info *the_mip;	/* the chosen alternate */
	struct mime_info *this_mip;
	/*
	 * The alternates are supposed to occur in order of
	 * increasing "complexity".  So: if there is at least
	 * one alternate of type "text/plain", use the last
	 * one, otherwise default to the first alternate.
	 */
	the_mip = top_mip->mi_flink;
	for (this_mip = top_mip->mi_flink;
	     this_mip != end_mip;
	     this_mip = this_mip->mi_flink) {
		const char *cmd;

		if (this_mip->mi_type == NULL ||
		    this_mip->mi_subtype == NULL)
			continue;

		if (is_basic_alternative(this_mip))
			the_mip = this_mip;
		else if (
			(cmd = get_command_hook(this_mip, "-hook")) ||
			(cmd = get_command_hook(this_mip, "-head")) ||
			(cmd = get_command_hook(this_mip, "-body"))) {
			int flags;
			/* just get the flags. */
			flags = mime_run_command(cmd, NULL);
			if ((flags & CMD_FLAG_ALTERNATIVE) != 0)
				the_mip = this_mip;
		}
	}
	return the_mip;
}


static inline int
is_multipart(struct mime_info *mip)
{
	return mip->mi_type &&
	    strcasecmp("multipart", mip->mi_type) == 0;
}
static inline int
is_message(struct mime_info *mip)
{
	return mip->mi_type &&
	    strcasecmp("message", mip->mi_type) == 0;
}

static inline int
is_alternative(struct mime_info *mip)
{
	return mip->mi_subtype &&
	    strcasecmp("alternative", mip->mi_subtype) == 0;
}


/*
 * Take a mime_info pointer and expand it recursively into all its
 * mime parts.  Only "multipart" and "message" types recursed into;
 * they are handled separately.
 */
static struct mime_info *
expand_mip(struct mime_info *top_mip)
{
	struct mime_info *this_mip;
	struct mime_info *next_mip;

	if (top_mip->mi_partnum == 0) {
		if (top_mip->mi_blink)
			top_mip->mi_partstr = top_mip->mi_blink->mi_partstr;
	}
	else if (top_mip->mi_parent.mip) {
		const char *prefix;
		char *cp;
		prefix = top_mip->mi_parent.mip->mi_partstr;
		(void)sasprintf(&cp, "%s%s%d", prefix,
		    *prefix ? "." : "", top_mip->mi_partnum);
		top_mip->mi_partstr = cp;
	}

	next_mip = top_mip->mi_flink;

	if (is_multipart(top_mip)) {
		top_mip->mi_ignore_body = 1; /* the first body is ignored */
		split_multipart(top_mip);

		for (this_mip = top_mip->mi_flink;
		     this_mip != next_mip;
		     this_mip = this_mip->mi_flink) {
			get_content(this_mip);
		}
		if (is_alternative(top_mip)) {
			this_mip = select_alternative(top_mip, next_mip);
			this_mip->mi_partnum = 0; /* suppress partnum display */
			this_mip->mi_flink = next_mip;
			this_mip->mi_blink = top_mip;
			top_mip->mi_flink  = this_mip;
		}
		/*
		 * Recurse into each part.
		 */
		for (this_mip = top_mip->mi_flink;
		     this_mip != next_mip;
		     this_mip = expand_mip(this_mip))
			continue;
	}
	else if (is_message(top_mip)) {
		top_mip->mi_ignore_body = 1; /* the first body is ignored */
		split_message(top_mip);

		this_mip = top_mip->mi_flink;
		if (this_mip) {
			get_content(this_mip);
			/*
			 * If the one part is MIME encoded, recurse into it.
			 * XXX - Should this be conditional on subtype "rcs822"?
			 */
			if (this_mip->mi_type &&
			    this_mip->mi_version &&
			    equal(this_mip->mi_version, MIME_VERSION)) {
				this_mip->mi_partnum = 0;
				(void)expand_mip(this_mip);
			}
		}
	}
	return next_mip;
}


#if 0
static int
show_partnum(FILE *fp, struct mime_info *mip)
{
	int need_dot;
	need_dot = 0;
	if (mip->mi_parent.mip && mip->mi_parent.mip->mi_parent.mip)
		need_dot = show_partnum(fp, mip->mi_parent.mip);

	if (mip->mi_partnum) {
		(void)fprintf(fp, "%s%d", need_dot ? "." : "",  mip->mi_partnum);
		need_dot = 1;
	}
	return need_dot;
}
#endif


PUBLIC struct mime_info *
mime_decode_open(struct message *mp)
{
	struct mime_info *mip;
	struct mime_info *p;

	mip = csalloc(1, sizeof(*mip));
	mip->mp = salloc(sizeof(*mip->mp));
	*mip->mp = *mp;		/* copy this so we don't trash the master mp */

	get_content(mip);

	/* RFC 2049 - sec 2 item 1 */
	if (mip->mi_version == NULL ||
	    !equal(mip->mi_version, MIME_VERSION))
		return NULL;

	mip->mi_partstr = "";
	if (mip->mi_type)
		(void)expand_mip(mip);

	/*
	 * Get the pipe_end and propagate it down the chain.
	 */
	mip->mi_pipe_end = last_registered_file(0); /* for mime_decode_close() */
	for (p = mip->mi_flink; p; p = p->mi_flink)
		p->mi_pipe_end = mip->mi_pipe_end;

/*	show_mime_info(stderr, mip, NULL); */

	return mip;
}


PUBLIC void
mime_decode_close(struct mime_info *mip)
{
	if (mip)
		close_top_files(mip->mi_pipe_end);
}


struct prefix_line_args_s {
	const char *prefix;
	size_t prefixlen;
};

static void
prefix_line(FILE *fi, FILE *fo, void *cookie)
{
	struct prefix_line_args_s *args;
	const char *line;
	const char *prefix;
	size_t prefixlen;
	size_t length;

	args = cookie;
	prefix    = args->prefix;
	prefixlen = args->prefixlen;

	while ((line = fgetln(fi, &length)) != NULL) {
		if (length > 1)
			(void)fputs(prefix, fo);
		else
			(void)fwrite(prefix, sizeof(*prefix),
			    prefixlen, fo);
		(void)fwrite(line, sizeof(*line), length, fo);
	}
	(void)fflush(fo);
}

PUBLIC int
mime_sendmessage(struct message *mp, FILE *obuf, struct ignoretab *igntab,
    const char *prefix, struct mime_info *mip)
{
	int error;
	int detachall_flag;
	const char *detachdir;
	FILE *end_of_prefix;

	if (mip == NULL)
		return obuf ?	/* were we trying to detach? */
		    sendmessage(mp, obuf, igntab, prefix, NULL) : 0;
	/*
	 * The prefix has two meanigs which we handle here:
	 * 1) If obuf == NULL, then we are detaching to the 'prefix' directory.
	 * 2) If obuf != NULL, then the prefix is prepended to each line.
	 */
	detachdir = NULL;
	detachall_flag = igntab == detachall;
	if (obuf == NULL) {
		assert(prefix != NULL);		/* coding error! */
		if ((obuf = last_registered_file(0)) == NULL)
			obuf = stdout;
		detachdir = prefix;
		prefix = NULL;
		igntab = ignoreall;	/* always ignore the headers */
	}
	/*
	 * Set this early so pipe_end() will work!
	 */
	mip->mi_fo = obuf;

	(void)fflush(obuf);  /* Be safe and flush!  XXX - necessary? */

	/*
	 * Handle the prefix as a pipe stage so it doesn't get seen by
	 * any decoding or hooks.
	 */
	if (prefix != NULL) {
		static struct prefix_line_args_s prefix_line_args;
		const char *dp, *dp2 = NULL;
		for (dp = prefix; *dp; dp++)
			if (!is_WSP(*dp))
				dp2 = dp;
		prefix_line_args.prefixlen = dp2 == 0 ? 0 : dp2 - prefix + 1;
		prefix_line_args.prefix = prefix;
		mime_run_function(prefix_line, pipe_end(mip), (void*)&prefix_line_args);
	}

	end_of_prefix = last_registered_file(0);
	error = 0;
	for (/*EMPTY*/; mip; mip = mip->mi_flink) {
		mip->mi_fo = obuf;
		mip->mi_head_end = obuf;
		mip->mi_detachdir = detachdir;
		mip->mi_detachall = detachall_flag;
		error |= sendmessage(mip->mp, pipe_end(mip), igntab, NULL, mip);
		close_top_files(end_of_prefix);	/* don't close the prefixer! */
	}
	return error;
}


#ifdef CHARSET_SUPPORT
/**********************************************
 * higher level interface to run mime_ficonv().
 */
static void
run_mime_ficonv(struct mime_info *mip, const char *charset)
{
	FILE *fo;
	iconv_t cd;

	fo = pipe_end(mip);

	if (charset == NULL ||
	    mip->mi_charset == NULL ||
	    strcasecmp(mip->mi_charset, charset) == 0 ||
	    strcasecmp(mip->mi_charset, "unknown") == 0)
		return;

	cd = iconv_open(charset, mip->mi_charset);
	if (cd == (iconv_t)-1) {
		(void)fprintf(fo, "\t [ iconv_open failed: %s ]\n\n",
		    strerror(errno));
		(void)fflush(fo);	/* flush here or see double! */
		return;
	}

	if (mip->mi_detachdir == NULL && /* don't contaminate the detach! */
	    value(ENAME_MIME_CHARSET_VERBOSE))
		(void)fprintf(fo, "\t[ converting %s -> %s ]\n\n",
		    mip->mi_charset, charset);

	mime_run_function(mime_ficonv, fo, cd);

	(void)iconv_close(cd);
}
#endif /* CHARSET_SUPPORT */


PUBLIC void
run_decoder(struct mime_info *mip, void(*fn)(FILE*, FILE*, void *))
{
#ifdef CHARSET_SUPPORT
	char *charset;

	charset = value(ENAME_MIME_CHARSET);
	if (charset && mip->mi_type && strcasecmp(mip->mi_type, "text") == 0)
		run_mime_ficonv(mip, charset);
#endif /* CHARSET_SUPPORT */

	if (mip->mi_detachdir == NULL &&
	    fn == mime_fio_copy)/* XXX - avoid an extra unnecessary pipe stage */
		return;

	mime_run_function(fn, pipe_end(mip),
	    mip->mi_detachdir ? NULL : __UNCONST("add_lf"));
}


/*
 * Determine how to handle the display based on the type and subtype
 * fields.
 */
enum dispmode_e {
	DM_IGNORE	= 0x00,	/* silently ignore part - must be zero! */
	DM_DISPLAY,		/* decode and display the part */
	DM_UNKNOWN,		/* unknown display */
	DM_BINARY,		/* indicate binary data */
	DM_PGPSIGN,		/* OpenPGP signed part */
	DM_PGPENCR,		/* OpenPGP encrypted part */
	DM_PGPKEYS,		/* OpenPGP keys part */
	DM_SENTINEL		/* end marker; shouldn't be used */
};
#define APPLICATION_OCTET_STREAM	DM_BINARY

static enum dispmode_e
get_display_mode(struct mime_info *mip, mime_codec_t dec)
{
	struct mime_subtype_s {
		const char *st_name;
		enum dispmode_e st_dispmode;
	};
	struct mime_type_s {
		const char *mt_type;
		const struct mime_subtype_s *mt_subtype;
		enum dispmode_e mt_dispmode;	/* default if NULL subtype */
	};
	static const struct mime_subtype_s text_subtype_tbl[] = {
		{ "plain",		DM_DISPLAY },
		{ "html", 		DM_DISPLAY },	/* rfc2854 */
		{ "rfc822-headers",	DM_DISPLAY },
		{ "css",		DM_DISPLAY },	/* rfc2318 */
		{ "enriched",		DM_DISPLAY },	/* rfc1523/rfc1563/rfc1896 */
		{ "graphics",		DM_DISPLAY },	/* rfc0553 */
		{ "nroff",		DM_DISPLAY },	/* rfc4263 */
		{ "red",		DM_DISPLAY },	/* rfc4102 */
		{ NULL,			DM_DISPLAY }	/* default */
	};
	static const struct mime_subtype_s image_subtype_tbl[] = {
		{ "tiff",		DM_BINARY },	/* rfc2302/rfc3302 */
		{ "tiff-fx",		DM_BINARY },	/* rfc3250/rfc3950 */
		{ "t38",		DM_BINARY },	/* rfc3362 */
		{ NULL,			DM_BINARY }	/* default */
	};
	static const struct mime_subtype_s audio_subtype_tbl[] = {
		{ "mpeg",		DM_BINARY },	/* rfc3003 */
		{ "t38",		DM_BINARY },	/* rfc4612 */
		{ NULL,			DM_BINARY }	/* default */
	};
	static const struct mime_subtype_s video_subtype_tbl[] = {
		{ NULL,			DM_BINARY }	/* default */
	};
	static const struct mime_subtype_s application_subtype_tbl[] = {
		{ "octet-stream",	APPLICATION_OCTET_STREAM },
		{ "pgp-encrypted",      DM_PGPENCR },   /* rfc3156 */
		{ "pgp-keys",           DM_PGPKEYS },   /* rfc3156 */
		{ "pgp-signature",      DM_PGPSIGN },   /* rfc3156 */
		{ "pdf",		DM_BINARY },	/* rfc3778 */
		{ "whoispp-query",	DM_UNKNOWN },	/* rfc2957 */
		{ "whoispp-response",	DM_UNKNOWN },	/* rfc2958 */
		{ "font-tdpfr",		DM_UNKNOWN },	/* rfc3073 */
		{ "xhtml+xml",		DM_UNKNOWN },	/* rfc3236 */
		{ "ogg",		DM_UNKNOWN },	/* rfc3534 */
		{ "rdf+xml",		DM_UNKNOWN },	/* rfc3870 */
		{ "soap+xml",		DM_UNKNOWN },	/* rfc3902 */
		{ "mbox",		DM_UNKNOWN },	/* rfc4155 */
		{ "xv+xml",		DM_UNKNOWN },	/* rfc4374 */
		{ "smil",		DM_UNKNOWN },	/* rfc4536 */
		{ "smil+xml",		DM_UNKNOWN },	/* rfc4536 */
		{ "json",		DM_UNKNOWN },	/* rfc4627 */
		{ "voicexml+xml",	DM_UNKNOWN },	/* rfc4267 */
		{ "ssml+xml",		DM_UNKNOWN },	/* rfc4267 */
		{ "srgs",		DM_UNKNOWN },	/* rfc4267 */
		{ "srgs+xml",		DM_UNKNOWN },	/* rfc4267 */
		{ "ccxml+xml",		DM_UNKNOWN },	/* rfc4267 */
		{ "pls+xml.",		DM_UNKNOWN },	/* rfc4267 */
		{ NULL,			APPLICATION_OCTET_STREAM } /* default */
	};
	static const struct mime_type_s mime_type_tbl[] = {
		{ "text",	 text_subtype_tbl,		DM_DISPLAY },
		{ "image",	 image_subtype_tbl,		DM_IGNORE },
		{ "audio",	 audio_subtype_tbl,		DM_IGNORE },
		{ "video",	 video_subtype_tbl,		DM_IGNORE },
		{ "application", application_subtype_tbl,	APPLICATION_OCTET_STREAM },
		{ NULL,		 NULL,				DM_UNKNOWN }, /* default */
	};
	const struct mime_type_s *mtp;
	const struct mime_subtype_s *stp;
	const char *mi_type;
	const char *mi_subtype;

	/*
	 * Silently ignore all multipart bodies.
	 * 1) In the case of "multipart" types, this typically
	 *    contains a message for non-mime enabled mail readers.
	 * 2) In the case of "message" type, there should be no body.
	 */
	if (mip->mi_ignore_body)	/*is_multipart(mip) || is_message(mip))*/
		return DM_IGNORE;

	/*
	 * If the encoding type given but not recognized, treat block
	 * as "application/octet-stream".  rfc 2049 sec 2 part 2.
	 */
	if (mip->mi_encoding && dec == NULL)
		return APPLICATION_OCTET_STREAM;

	mi_type    = mip->mi_type;
	mi_subtype = mip->mi_type ? mip->mi_subtype : NULL;

	/*
	 * If there was no type specified, display anyway so we don't
	 * miss anything.  (The encoding type is known.)
	 */
	if (mi_type == NULL)
		return DM_DISPLAY;	/* XXX - default to something safe! */

	for (mtp = mime_type_tbl; mtp->mt_type; mtp++) {
		if (strcasecmp(mtp->mt_type, mi_type) == 0) {
			if (mi_subtype == NULL)
				return mtp->mt_dispmode;
			for (stp = mtp->mt_subtype; stp->st_name; stp++) {
				if (strcasecmp(stp->st_name, mi_subtype) == 0)
					return stp->st_dispmode;
			}
			return stp->st_dispmode;
		}
	}
	return mtp->mt_dispmode;
}


PUBLIC FILE *
mime_decode_body(struct mime_info *mip)
{
	static enum dispmode_e dispmode;
	mime_codec_t dec;
	const char *cmd;

	/* close anything left over from mime_decode_head() */
	close_top_files(mip->mi_head_end);

	/*
	 * Make sure we flush everything down the pipe so children
	 * don't see it.
	 */
	(void)fflush(pipe_end(mip));

	if (mip->mi_detachdir)	/* We are detaching!  Ignore the hooks. */
		return mime_detach_parts(mip);

	cmd = NULL;
	if (mip->mi_command_hook == NULL)
		cmd = get_command_hook(mip, "-body");

	dec = mime_fio_decoder(mip->mi_encoding);

	/*
	 * If there is a filter running, we need to send the message
	 * to it.  Otherwise, get the default display mode for this body.
	 */
	dispmode = cmd || mip->mi_command_hook ? DM_DISPLAY : get_display_mode(mip, dec);

	if (dec == NULL)	/* make sure we have a usable decoder */
		dec = mime_fio_decoder(MIME_TRANSFER_7BIT);

	if (dispmode == DM_DISPLAY) {
		int flags;
		if (cmd == NULL)
			/* just get the flags */
			flags = mime_run_command(mip->mi_command_hook, NULL);
		else
			flags = mime_run_command(cmd, pipe_end(mip));
		if ((flags & CMD_FLAG_NO_DECODE) == 0)
			run_decoder(mip, dec);
		return pipe_end(mip);
	}
	else {
		static const struct msg_tbl_s {
			enum dispmode_e dm;
			const char *msg;
		} msg_tbl[] = {
			{ DM_BINARY,	"binary content"	},
			{ DM_PGPSIGN,	"OpenPGP signature"	},
			{ DM_PGPENCR,	"OpenPGP encrypted"	},
			{ DM_PGPKEYS,	"OpenPGP keys"		},
			{ DM_UNKNOWN,	"unknown data"		},
			{ DM_IGNORE,	NULL			},
			{ DM_SENTINEL,	NULL			},
		};
		const struct msg_tbl_s *mp;

		for (mp = msg_tbl; mp->dm != DM_SENTINEL; mp++)
			if (mp->dm == dispmode)
				break;

		assert(mp->dm != DM_SENTINEL);	/* msg_tbl is short if this happens! */

		if (mp->msg)
			(void)fprintf(pipe_end(mip), "  [%s]\n\n", mp->msg);

		return NULL;
	}
}


/************************************************************************
 * Higher level header decoding interface.
 *
 * The core routines are in mime_header.c.
 */

/*
 * Decode a portion of the header field.
 *
 * linebuf	buffer to decode into.
 * bufsize	size of linebuf.
 * hdrline	full header line including header name.
 * srcstr	pointer to string to decode
 */
PUBLIC char *
mime_decode_hfield(char *linebuf, size_t bufsize, const char *hdrline, char *srcstr)
{
	hfield_decoder_t decode;
	decode = mime_hfield_decoder(hdrline);
	if (decode) {
		decode(linebuf, bufsize, srcstr);
		return linebuf;
	}
	return srcstr;
}

/*
 * Return the next header field found in the input stream.
 * Return 0 if something found, -1 otherwise.
 * For a proper header, "*colon" is set to point to the colon
 * terminating the header name.  Otherwise it is NULL.
 *
 * NOTE: unlike gethfield() in support.c this:
 * 1) preserves folding (newlines),
 * 2) reads until fgetln() gets an EOF,
 * 3) only sets *colon if there is a "proper" one.
 */
static int
get_folded_hfield(FILE *f, char *linebuf, size_t bufsize, char **colon)
{
	char *cp, *cp2;
	char *line;
	size_t len;

	if ((cp = fgetln(f, &len)) == NULL)
		return -1;
	for (cp2 = cp;
	     cp2 < cp + len && isprint((unsigned char)*cp2) &&
		 !is_WSP(*cp2) && *cp2 != ':';
	     cp2++)
		continue;
	len = MIN(bufsize - 1, len);
	bufsize -= len;
	(void)memcpy(linebuf, cp, len);
	*colon = *cp2 == ':' ? linebuf + (cp2 - cp) : NULL;
	line = linebuf + len;
	for (;;) {
		int c;
		(void)ungetc(c = getc(f), f);
		if (!is_WSP(c))
			break;

		if ((cp = fgetln(f, &len)) == NULL)
			break;
		len = MIN(bufsize - 1, len);
		bufsize -= len;
		if (len == 0)
			break;
		(void)memcpy(line, cp, len);
		line += len;
	}
	*line = 0;
	return 0;
}

static void
decode_header(FILE *fi, FILE *fo, void *cookie __unused)
{
	char linebuf[LINESIZE];
	char *colon;
#ifdef __lint__
	cookie = cookie;
#endif
	while (get_folded_hfield(fi, linebuf, sizeof(linebuf), &colon) >= 0) {
		char decbuf[LINESIZE];
		char *hdrstr;
		hdrstr = linebuf;
		if (colon)
			hdrstr = mime_decode_hfield(decbuf, sizeof(decbuf), hdrstr, hdrstr);
		(void)fprintf(fo, hdrstr);
	}
}

PUBLIC FILE *
mime_decode_header(struct mime_info *mip)
{
	int flags;
	const char *cmd;
	FILE *fo;

	fo = pipe_end(mip);

	if (mip->mi_detachdir) { /* We are detaching.  Don't run anything! */
		(void)fflush(fo);
		return pipe_end(mip);
	}

	if (mip->mi_partnum)
		(void)fprintf(fo, "----- Part %s -----\n", mip->mi_partstr);

	(void)fflush(fo);	/* Flush so the childern don't see it. */

	/*
	 * install the message hook before the head hook.
	 */
	cmd = get_command_hook(mip, "-hook");
	mip->mi_command_hook = cmd;
	if (cmd) {
		flags = mime_run_command(cmd, pipe_end(mip));
		mip->mi_head_end = last_registered_file(0);
	}
	else {
		cmd = get_command_hook(mip, "-head");
		mip->mi_head_end = last_registered_file(0);
		flags = mime_run_command(cmd, pipe_end(mip));
	}

	if (value(ENAME_MIME_DECODE_HDR) && (flags & CMD_FLAG_NO_DECODE) == 0)
		mime_run_function(decode_header, pipe_end(mip), NULL);

	return pipe_end(mip);
}

#endif /* MIME_SUPPORT */
