/*	$NetBSD: ndbm.h,v 1.11 2004/04/27 20:13:46 kleink Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ndbm.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _NDBM_H_
#define	_NDBM_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <db.h>

#if defined(_NETBSD_SOURCE)
/* Map dbm interface onto db(3). */
#define DBM_RDONLY	O_RDONLY
#endif

/* Flags to dbm_store(). */
#define DBM_INSERT      0
#define DBM_REPLACE     1

#if defined(_NETBSD_SOURCE)
/*
 * The db(3) support for ndbm(3) always appends this suffix to the
 * file name to avoid overwriting the user's original database.
 */
#define	DBM_SUFFIX	".db"
#endif

typedef struct {
	void *dptr;
	int dsize;		/* XXX */
} datum;

typedef DB DBM;
#if defined(_NETBSD_SOURCE)
#define	dbm_pagfno(a)	DBM_PAGFNO_NOT_AVAILABLE
#endif

__BEGIN_DECLS
void	 dbm_close __P((DBM *));
int	 dbm_delete __P((DBM *, datum));
datum	 dbm_fetch __P((DBM *, datum));
datum	 dbm_firstkey __P((DBM *));
datum	 dbm_nextkey __P((DBM *));
DBM	*dbm_open __P((const char *, int, mode_t));
int	 dbm_store __P((DBM *, datum, datum, int));
int	 dbm_error __P((DBM *));
int	 dbm_clearerr __P((DBM *));
#if defined(_NETBSD_SOURCE)
int	 dbm_dirfno __P((DBM *));
#endif
__END_DECLS

#endif /* !_NDBM_H_ */
