/* $NetBSD: ldp_errors.h,v 1.3.2.1 2013/02/25 00:30:43 tls Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#ifndef _LDP_ERRORS_H_
#define _LDP_ERRORS_H_

#define	LDP_E_OK		0
#define	LDP_E_BAD_VERSION	1
#define	LDP_E_BAD_LENGTH	2
#define	LDP_E_MEMORY		3
#define	LDP_E_NOENT		4
#define	LDP_E_ALREADY_DONE	5
#define	LDP_E_BAD_AF		6
#define	LDP_E_BAD_FEC		7
#define	LDP_E_BAD_LABEL		8
#define	LDP_E_NO_SUCH_ROUTE	LDP_E_ROUTE_ERROR
/* 9, 10 Free */
#define	LDP_E_ROUTE_ERROR	11
#define	LDP_E_NO_BINDING	12
#define	LDP_E_TOO_MANY_LABELS	13
#define	LDP_E_INVAL		14
#define	LDP_E_TOO_MANY_FDS	15
#define	LDP_E_BAD_ID		16
#define	LDP_E_GENERIC		255

void	printtime(void);

void	debugp(const char *, ...) __printflike(1, 2);
void	fatalp(const char *, ...) __printflike(1, 2);
void	warnp(const char *, ...) __printflike(1, 2);
const char *	satos(const struct sockaddr *);

#endif	/* !_LDP_ERRORS_H_ */
