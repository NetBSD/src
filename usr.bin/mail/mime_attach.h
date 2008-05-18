/*	$NetBSD: mime_attach.h,v 1.4.6.1 2008/05/18 12:36:06 yamt Exp $	*/

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

#ifndef __MIME_ATTACH_H__
#define __MIME_ATTACH_H__

/*
 * All routines declared here are exported via mime.h
 */

struct attachment *mime_attach_optargs(struct name *);
struct attachment *mime_attach_files(struct attachment *, char *);
FILE	*mime_encode(FILE *, struct header *);
void	mime_putheader(FILE *, struct header *);
struct Content get_mime_content(struct attachment *, int);

/*
 * XXX - Debugging routines stuck in mime_attach.c ... these should go away!
 */
void	show_attach(const char *, struct attachment *);	/* for debugging only */
void	show_header(struct header *);			/* for debugging only */

#endif /* __MIME_ATTACH_H__ */
#endif /* MIME_SUPPORT */
