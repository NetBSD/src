/*	$NetBSD: types.h,v 1.12.8.1 2014/08/19 23:52:23 tls Exp $	*/

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
 * $FreeBSD: src/sys/compat/opensolaris/sys/types.h,v 1.3 2007/11/28 21:49:16 jb Exp $
 */

#ifndef _OPENSOLARIS_SYS_TYPES_H_
#define	_OPENSOLARIS_SYS_TYPES_H_

/*
 * This is a bag of dirty hacks to keep things compiling.
 */
#ifdef __APPLE__
#include <stdint.h>
#else
#include <sys/stdint.h>
#endif
#ifdef _NETBSD_SOURCE
#include_next <sys/types.h>
#include_next <sys/ccompile.h>
#else
#define _NETBSD_SOURCE
#include_next <sys/types.h>
#include_next <sys/ccompile.h>
#undef _NETBSD_SOURCE
#endif

#ifndef _KERNEL
#include <stdarg.h>
#else
#include <sys/stdarg.h>
#endif

#define	MAXNAMELEN	256
#define	FMNAMESZ	8

#ifdef __APPLE__
typedef int64_t longlong_t;
typedef uint64_t u_longlong_t;
typedef unsigned long vsize_t;
#endif

typedef unsigned int	size32_t;
typedef unsigned int	caddr32_t;

typedef	struct timespec	timestruc_t;
typedef u_int		uint_t;
typedef u_char		uchar_t;
typedef u_short		ushort_t;
typedef u_long		ulong_t;
typedef off_t		off64_t;
typedef id_t		taskid_t;
typedef id_t		projid_t;
typedef id_t		poolid_t;
typedef id_t		zoneid_t;
typedef id_t		ctid_t;

#define	B_FALSE	0
#define	B_TRUE	1
typedef int		boolean_t;

typedef longlong_t      hrtime_t;
typedef int32_t		t_scalar_t;
typedef uint32_t	t_uscalar_t;
typedef vsize_t		pgcnt_t;
typedef u_longlong_t	len_t;
typedef int		major_t;
typedef int		minor_t;
typedef int		o_uid_t;
typedef int		o_gid_t;
typedef struct kauth_cred cred_t;
typedef uintptr_t	pc_t;
typedef struct vm_page	page_t;
typedef	ushort_t	o_mode_t;	/* old file attribute type */
typedef	u_longlong_t	diskaddr_t;
typedef void		*zone_t;
typedef struct vfsops	vfsops_t;

#ifdef _KERNEL

typedef	short		index_t;
typedef	off_t		offset_t;
typedef	int64_t		rlim64_t;
typedef __caddr_t	caddr_t;	/* core address */

#else

typedef	longlong_t	offset_t;
typedef	u_longlong_t	u_offset_t;
typedef	uint64_t	upad64_t;
typedef	struct timespec	timespec_t;
typedef	int32_t		daddr32_t;
typedef	int32_t		time32_t;

#endif	/* !_KERNEL */

#define	MAXOFFSET_T 	0x7fffffffffffffffLL
#define	seg_rw		uio_rw
#define	S_READ		UIO_READ
#define	S_WRITE		UIO_WRITE
struct aio_req;
typedef void		*dev_info_t;

#endif	/* !_OPENSOLARIS_SYS_TYPES_H_ */
