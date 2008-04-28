/*	$NetBSD: mime.h,v 1.4 2008/04/28 20:24:14 martin Exp $	*/

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

#ifndef __MIME_H__
#define __MIME_H__

/*
 * This is the public interface of the mime module visible to the rest
 * of the world.  It is the only header file that should be include in
 * a non-mime module.
 */

#include "mime_attach.h"
#include "mime_decode.h"

/* a single export from mime_codecs.c */
const char  *mime_next_encoding_name(const void **);

/* two exports from mime_detach.c */
int	mime_detach_control(void);
void	mime_detach_msgnum(struct mime_info *, const char *);

/*
 * User knobs: environment variable names used by this module.
 * See man mail(1) for a description of most of these.
 * Grep the source for the rest.
 */
#define ENAME_MIME_ENCODE_MSG		"mime-encode-message"

#define ENAME_MIME_DECODE_MSG		"mime-decode-message"
#define ENAME_MIME_DECODE_HDR		"mime-decode-header"
#define ENAME_MIME_DECODE_QUOTE		"mime-decode-quote"
#define ENAME_MIME_DECODE_INSERT	"mime-decode-insert"

#define ENAME_MIME_B64_LINE_MAX		"mime-max-base64-line-length"
#define ENAME_MIME_QP_LINE_MAX		"mime-max-quoted-line-length"
#define ENAME_MIME_UNENC_LINE_MAX	"mime-max-unencoded-line-length"

#define ENAME_MIME_CHARSET		"mime-charset"
#define ENAME_MIME_CHARSET_VERBOSE	"mime-charset-verbose"

#define ENAME_MIME_ATTACH_LIST		"mime-attach-list"

#define ENAME_MIME_DETACH_DIR		"mime-detach-dir"
#define ENAME_MIME_DETACH_BATCH		"mime-detach-batch"
#define ENAME_MIME_DETACH_OVERWRITE	"mime-detach-overwrite"

#endif /* __MIME_H__ */
#endif /* MIME_SUPPORT */
