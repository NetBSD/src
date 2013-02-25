/*	$NetBSD: mime_codecs.h,v 1.4.26.1 2013/02/25 00:30:36 tls Exp $	*/

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

#ifndef __MIME_CODECS_H__
#define __MIME_CODECS_H__

#include <iconv.h>

size_t mime_iconv(iconv_t, const char **, size_t *, char **, size_t *);
void   mime_ficonv(FILE *, FILE *, void *);

ssize_t mime_b64tobin(char *, const char *, size_t);
void    mime_bintob64(char *, const char *, size_t);

typedef void(*mime_codec_t)(FILE *, FILE *, void *);

mime_codec_t mime_fio_encoder(const char *);
mime_codec_t mime_fio_decoder(const char *);

void mime_fio_copy(FILE *, FILE *, void *);

ssize_t mime_rfc2047_decode(char, char *, size_t, const char *, size_t);

#include "mime.h"

/* This is also declared in mime.h for export to complete.c. */
const char *mime_next_encoding_name(const void **);

/*
 * valid transfer encoding names
 */
#define MIME_TRANSFER_7BIT	"7bit"
#define MIME_TRANSFER_8BIT	"8bit"
#define MIME_TRANSFER_BINARY	"binary"
#define MIME_TRANSFER_QUOTED	"quoted-printable"
#define MIME_TRANSFER_BASE64	"base64"

#endif /* __MIME_CODECS_H__ */
#endif /* MIME_SUPPORT */
