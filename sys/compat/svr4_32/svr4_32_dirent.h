/*	$NetBSD: svr4_32_dirent.h,v 1.3.38.1 2008/05/18 12:33:28 yamt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_32_DIRENT_H_
#define	_SVR4_32_DIRENT_H_

#include <compat/svr4/svr4_dirent.h>

struct svr4_32_dirent {
	svr4_32_ino_t	d_ino;
	svr4_32_off_t	d_off;
	u_short		d_reclen;
	char		d_name[SVR4_MAXNAMLEN + 1];
};
typedef netbsd32_caddr_t svr4_32_dirent_tp;

struct svr4_32_dirent64 {
	svr4_32_ino64_t	d_ino;
	svr4_32_off64_t	d_off;
	u_short		d_reclen;
	char		d_name[SVR4_MAXNAMLEN + 1];
};

#define SVR4_32_NAMEOFF(dp)       ((char *)&(dp)->d_name - (char *)dp)
#define SVR4_32_RECLEN(de,namlen) ALIGN32((SVR4_32_NAMEOFF(de) + (namlen) + 1))

#endif /* !_SVR4_32_DIRENT_H_ */
