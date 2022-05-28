/*	$NetBSD: mime_detach.h,v 1.3 2022/05/28 10:36:24 andvar Exp $	*/

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

#ifndef __MIME_DETACH_H__
#define __MIME_DETACH_H__

/*
 * The fundamental data structure shared by mime_decode.c and mime_detach.c.
 */
struct mime_info {
	struct mime_info *mi_blink;
	struct mime_info *mi_flink;

	/* sendmessage -> decoder -> filter -> pager */

	FILE *mi_fo;		/* output file handle pointing to PAGER */
	FILE *mi_pipe_end;	/* initial end of pipe */
	FILE *mi_head_end;	/* close to here at start of body */

	int mi_ignore_body;	/* suppress certain body parts */

	/* stuff specific to mime_detach */
	int mi_partnum;		/* part number displayed (if nonzero) */
	const char *mi_partstr;	/* string to actually display */
	const char *mi_msgstr;	/* message number in string form */

	/* strings extracted from MIME header fields */
	const char *mi_version;
	const char *mi_type;
	const char *mi_subtype;
	const char *mi_boundary;	/* type parameter */
	const char *mi_charset;		/* type parameter */
	const char *mi_encoding;
	const char *mi_disposition;
	const char *mi_filename;	/* from type or disposition parameter */

	struct message *mp;		/* MP for this message regarded as a part. */
	struct {
		struct mime_info *mip;	/* parent of part of multipart message */
		struct message *mp;	/* the original parent mp before being split! */
	} mi_parent;

	int mi_detachall;		/* detach unnamed parts */
	const char *mi_detachdir;	/* directory for detaching attachments */
	const char *mi_command_hook;	/* alternate command used to process this message */
};


/*
 * Routines shared by mime_decode.c and mime_detach.c
 */

/* exported from mime_detach.c */
FILE *mime_detach_parts(struct mime_info *);

/* These are exported from mime_decode.c */
FILE *pipe_end(struct mime_info *);
void run_decoder(struct mime_info *mip, void(*fn)(FILE*, FILE*, void *));

#endif /* __MIME_DETACH_H__ */
#endif /* MIME_SUPPORT */
