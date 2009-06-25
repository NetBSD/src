/*	$NetBSD: iscsi-md5.h,v 1.2 2009/06/25 13:47:09 agc Exp $	*/

/*
 * This file is derived from the RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm and has been modified by Jason R. Thorpe <thorpej@NetBSD.ORG>
 * for portability and formatting.
 */

/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef _SYS_MD5_H_
#define _SYS_MD5_H_

#include <sys/types.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/* iSCSI_MD5 context. */
typedef struct iSCSI_MD5Context {
	uint32_t state[4];	/* state (ABCD) */
	uint32_t count[2];	/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64]; /* input buffer */
} iSCSI_MD5_CTX;

__BEGIN_DECLS
void	iSCSI_MD5Init(iSCSI_MD5_CTX *);
void	iSCSI_MD5Update(iSCSI_MD5_CTX *, const uint8_t *, size_t);
void	iSCSI_MD5Final(unsigned char[16], iSCSI_MD5_CTX *);
#ifndef _KERNEL
char	*iSCSI_MD5End(iSCSI_MD5_CTX *, char *);
char	*iSCSI_MD5File(const char *, char *);
char	*iSCSI_MD5Data(const uint8_t *, size_t, char *);
#endif /* _KERNEL */
__END_DECLS

#endif /* _SYS_MD5_H_ */
