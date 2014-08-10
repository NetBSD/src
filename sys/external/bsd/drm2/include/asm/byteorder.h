/*	$NetBSD: byteorder.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _ASM_BYTEORDER_H_
#define _ASM_BYTEORDER_H_

#include <sys/endian.h>

#define	cpu_to_le16	htole16
#define	cpu_to_le32	htole32
#define	cpu_to_le64	htole64
#define	cpu_to_be16	htobe16
#define	cpu_to_be32	htobe32
#define	cpu_to_be64	htobe64

#define	le16_to_cpu	le16toh
#define	le32_to_cpu	le32toh
#define	le64_to_cpu	le64toh
#define	be16_to_cpu	be16toh
#define	be32_to_cpu	be32toh
#define	be64_to_cpu	be64toh

#define	be16_to_cpup	be16dec
#define	be32_to_cpup	be32dec
#define	be64_to_cpup	be64dec
#define	le16_to_cpup	le16dec
#define	le32_to_cpup	le32dec
#define	le64_to_cpup	le64dec

#endif	/* _ASM_BYTEORDER_H_ */
