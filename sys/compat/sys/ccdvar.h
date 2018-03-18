/*	$NetBSD: ccdvar.h,v 1.1.2.2 2018/03/18 21:41:31 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
#ifndef _SYS_COMPAT_CCDVAR_H_
#define _SYS_COMPAT_CCDVAR_H_

#if defined(COMPAT_60) && !defined(_LP64)
/*
 * Old version with incorrect ccio_size
 */
struct ccd_ioctl_60 {
	char	**ccio_disks;		/* pointer to component paths */
	u_int	ccio_ndisks;		/* number of disks to concatenate */
	int	ccio_ileave;		/* interleave (DEV_BSIZE blocks) */
	int	ccio_flags;		/* see sc_flags below */
	int	ccio_unit;		/* unit number: use varies */
	size_t	ccio_size;		/* (returned) size of ccd */
};

#define CCDIOCSET_60	_IOWR('F', 16, struct ccd_ioctl_60)   /* enable ccd */
#define CCDIOCCLR_60	_IOW('F', 17, struct ccd_ioctl_60)   /* disable ccd */

#endif /* COMPAT_60 && !LP64*/

struct lwp;

extern int (*compat_ccd_ioctl_60)(dev_t, u_long, void *, int, struct lwp *,
    int (*)(dev_t, u_long, void *, int, struct lwp *));

void ccd_60_init(void);
void ccd_60_fini(void);

#endif /* _SYS_COMPAT_CCDVAR_H_ */
