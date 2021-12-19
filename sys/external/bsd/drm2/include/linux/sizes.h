/*	$NetBSD: sizes.h,v 1.4 2021/12/19 11:49:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_SIZES_H_
#define	_LINUX_SIZES_H_

#define	SZ_64		64u
#define	SZ_128		128u
#define	SZ_256		256u
#define	SZ_512		512u
#define	SZ_1K		1024u
#define	SZ_2K		(2u*SZ_1K)
#define	SZ_4K		(4u*SZ_1K)
#define	SZ_8K		(8u*SZ_1K)
#define	SZ_16K		(16u*SZ_1K)
#define	SZ_32K		(32u*SZ_1K)
#define	SZ_64K		(64u*SZ_1K)
#define	SZ_128K		(128u*SZ_1K)
#define	SZ_256K		(256u*SZ_1K)
#define	SZ_512K		(512u*SZ_1K)
#define	SZ_1M		(1024u*SZ_1K)
#define	SZ_2M		(2u*SZ_1M)
#define	SZ_4M		(4u*SZ_1M)
#define	SZ_8M		(8u*SZ_1M)
#define	SZ_16M		(16u*SZ_1M)
#define	SZ_32M		(32u*SZ_1M)
#define	SZ_64M		(64u*SZ_1M)
#define	SZ_128M		(128u*SZ_1M)
#define	SZ_256M		(256u*SZ_1M)
#define	SZ_512M		(512u*SZ_1M)
#define	SZ_1G		(1024u*SZ_1M)
#define	SZ_2G		(2u*SZ_1G)
#define	SZ_4G		(4ull*SZ_1G)
#define	SZ_8G		(8ull*SZ_1G)

#endif	/* _LINUX_SIZES_H_ */
