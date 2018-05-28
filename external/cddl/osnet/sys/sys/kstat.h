/*	$NetBSD: kstat.h,v 1.4 2018/05/28 21:05:10 chs Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/kstat.h 315896 2017-03-24 09:37:00Z mav $
 */

#ifndef _OPENSOLARIS_SYS_KSTAT_H_
#define	_OPENSOLARIS_SYS_KSTAT_H_

#include <sys/sysctl.h>

#define	KSTAT_TYPE_NAMED	1

#define	KSTAT_FLAG_VIRTUAL	0x01

#define	KSTAT_READ	0
#define	KSTAT_WRITE	1

typedef struct kstat {
	void	*ks_data;
	u_int	 ks_ndata;
#ifdef _KERNEL
	struct sysctllog *ks_clog;
	const struct sysctlnode *ks_node;
#endif
	int		(*ks_update)(struct kstat *, int); /* dynamic update */
	void		*ks_private;	/* arbitrary provider-private data */
} kstat_t;

typedef struct kstat_named {
#define	KSTAT_STRLEN	31
	char	name[KSTAT_STRLEN];
#define	KSTAT_DATA_CHAR		0
#define	KSTAT_DATA_INT32	1
#define	KSTAT_DATA_UINT32	2
#define	KSTAT_DATA_INT64	3
#define	KSTAT_DATA_UINT64	4
	uchar_t	data_type;
#define	KSTAT_DESCLEN		128
	char	desc[KSTAT_DESCLEN];
	union {
		uint64_t	ui64;
	} value;
} kstat_named_t;

kstat_t *kstat_create(char *module, int instance, char *name, char *class,
    uchar_t type, ulong_t ndata, uchar_t flags);
void kstat_install(kstat_t *ksp);
void kstat_delete(kstat_t *ksp);
void kstat_set_string(char *, const char *);
void kstat_named_init(kstat_named_t *, const char *, uchar_t);

#endif	/* _OPENSOLARIS_SYS_KSTAT_H_ */
