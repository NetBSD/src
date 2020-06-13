/*	$NetBSD: sun8i_crypto.h,v 1.2 2020/06/13 18:57:54 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#ifndef	_ARM_SUN8I_CRYPTO_H
#define	_ARM_SUN8I_CRYPTO_H

#include <sys/cdefs.h>

#define	SUN8I_CRYPTO_NCHAN	4

/* task_descriptor_queue common control bitmap (32bit), p. 234 */
#define	SUN8I_CRYPTO_TDQC_INTR_EN	__BIT(31)
#define	SUN8I_CRYPTO_TDQC_IV_MODE	__BIT(16) /* MD5/SHA-1/SHA-2 IV */
#define	SUN8I_CRYPTO_TDQC_IV_MODE_CONST		0 /* standard constants */
#define	SUN8I_CRYPTO_TDQC_IV_MODE_ARBITRARY	1 /* arbitrary */
#define	SUN8I_CRYPTO_TDQC_HMAC_PT_LAST	__BIT(15)
#define	SUN8I_CRYPTO_TDQC_OP_DIR	__BIT(8)
#define	SUN8I_CRYPTO_TDQC_OP_DIR_ENC		0
#define	SUN8I_CRYPTO_TDQC_OP_DIR_DEC		1
#define	SUN8I_CRYPTO_TDQC_METHOD	__BITS(6,0)
#define	SUN8I_CRYPTO_TDQC_METHOD_AES		0
#define	SUN8I_CRYPTO_TDQC_METHOD_DES		1
#define	SUN8I_CRYPTO_TDQC_METHOD_3DES		2
#define	SUN8I_CRYPTO_TDQC_METHOD_MD5		16
#define	SUN8I_CRYPTO_TDQC_METHOD_SHA1		17
#define	SUN8I_CRYPTO_TDQC_METHOD_SHA224		18
#define	SUN8I_CRYPTO_TDQC_METHOD_SHA256		19
#define	SUN8I_CRYPTO_TDQC_METHOD_HMAC_SHA1	22
#define	SUN8I_CRYPTO_TDQC_METHOD_HMAC_SHA256	23
#define	SUN8I_CRYPTO_TDQC_METHOD_RSA		32
#define	SUN8I_CRYPTO_TDQC_METHOD_TRNG		48
#define	SUN8I_CRYPTO_TDQC_METHOD_PRNG		49

/* task_descriptor_queue symmetric control (32bit), p. 235 */
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT	__BITS(23,20)
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT_SS_KEYx	0
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT_SSK	1
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT_HUK	2
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT_RSSK	3
#define	SUN8I_CRYPTO_TDQS_SKEY_SELECT_INTERNAL(n)	(8 + (n))
#define	SUN8I_CRYPTO_TDQS_AES_CTS_LAST	__BIT(16)
#define	SUN8I_CRYPTO_TDQS_OP_MODE	__BITS(11,8)
#define	SUN8I_CRYPTO_TDQS_OP_MODE_ECB		0
#define	SUN8I_CRYPTO_TDQS_OP_MODE_CBC		1
#define	SUN8I_CRYPTO_TDQS_OP_MODE_CTR		2
#define	SUN8I_CRYPTO_TDQS_OP_MODE_CTS		3
#define	SUN8I_CRYPTO_TDQS_CTR_WIDTH	__BITS(3,2)
#define	SUN8I_CRYPTO_TDQS_CTR_WIDTH_16		0
#define	SUN8I_CRYPTO_TDQS_CTR_WIDTH_32		1
#define	SUN8I_CRYPTO_TDQS_CTR_WIDTH_64		2
#define	SUN8I_CRYPTO_TDQS_CTR_WIDTH_128		3
#define	SUN8I_CRYPTO_TDQS_AES_KEYSIZE	__BITS(1,0)
#define	SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128	0
#define	SUN8I_CRYPTO_TDQS_AES_KEYSIZE_192	1
#define	SUN8I_CRYPTO_TDQS_AES_KEYSIZE_256	2

/* task_descriptor_queue asymmetric control (32bit), p. 236 */
#define	SUN8I_CRYPTO_TDQA_RSA_MODSIZE	__BITS(30,28)
#define	SUN8I_CRYPTO_TDQA_RSA_MODSIZE_512	0
#define	SUN8I_CRYPTO_TDQA_RSA_MODSIZE_1024	1
#define	SUN8I_CRYPTO_TDQA_RSA_MODSIZE_2048	2

/* Crypto Engine Register List */

/* Module */
#define	SUN8I_CRYPTO_NS		0x01c15000
#define	SUN8I_CRYPTO_S		0x01c15800

/* Register */
#define	SUN8I_CRYPTO_TDQ	0x00 /* Task Descriptor Queue Address */
#define	SUN8I_CRYPTO_CTR	0x04 /* Gating Control Register */
#define	SUN8I_CRYPTO_ICR	0x08 /* Interrupt Control Register */
#define	SUN8I_CRYPTO_ISR	0x0c /* Interrupt Status Register */
#define	SUN8I_CRYPTO_TLR	0x10 /* Task Load Register */
#define	SUN8I_CRYPTO_TSR	0x14 /* Task Status Register */
#define	SUN8I_CRYPTO_ESR	0x18 /* Task Error type Register */
#define	SUN8I_CRYPTO_CSAR	0x24 /* Current Source Address Register */
#define	SUN8I_CRYPTO_CDAR	0x28 /* Current Destination Address Register */
#define	SUN8I_CRYPTO_TPR	0x2c /* Throughput Register */

#define	SUN8I_CRYPTO_CTR_RSA_CLK_EN	__BIT(3)
#define	SUN8I_CRYPTO_CTR_DIE_ID		__BITS(2,0)

#define	SUN8I_CRYPTO_ICR_INTR_EN	__BITS(3,0)
#define	SUN8I_CRYPTO_ICR_INTR_EN_CHAN(n)	(__BIT(0) << (n))

#define	SUN8I_CRYPTO_ISR_DONE		__BITS(3,0)
#define	SUN8I_CRYPTO_ISR_DONE_CHAN(n)		(__BIT(0) << (n))

#define	SUN8I_CRYPTO_TLR_LOAD		__BIT(0)

#define	SUN8I_CRYPTO_TSR_TASK		__BITS(1,0)
/* task number, 0-3 */

#define	SUN8I_CRYPTO_ESR_CHAN(n)	(__BITS(3,0) << (n))
#define	SUN8I_CRYPTO_ESR_CHAN_ALGNOTSUP		__BIT(0)
#define	SUN8I_CRYPTO_ESR_CHAN_DATALENERR	__BIT(1)
#define	SUN8I_CRYPTO_ESR_CHAN_KEYSRAMERR	__BIT(2)

#define	SUN8I_CRYPTO_MAXKEYBYTES	256	/* e.g. 2048-bit RSA */
#define	SUN8I_CRYPTO_MAXIVBYTES		64	/* e.g. SHA-512 */
#define	SUN8I_CRYPTO_MAXCTRBYTES	16	/* e.g. AES */

/*
 * Eight src segments and eight dst segments per task.  The datalen
 * field, measured either in bytes or words, is 32 bits.  Each segment
 * length is measured in words and is also 32 bits.  We never do very
 * long transfers anyway so rounding the maximum size down to 2^32 - 1
 * is not a real limitation.
 */
#define	SUN8I_CRYPTO_MAXSEGS		8
#define	SUN8I_CRYPTO_MAXSEGLEN		UINT32_MAX
#define	SUN8I_CRYPTO_MAXDATALEN		UINT32_MAX

struct sun8i_crypto_taskdesc {
	uint32_t	td_cid;		/* task channel id */
	uint32_t	td_tdqc;	/* task descriptor queue common */
	uint32_t	td_tdqs;	/* task descriptor queue symmetric */
	uint32_t	td_tdqa;	/* task descriptor queue asymmetric */
	uint32_t	td_keydesc;	/* key descriptor */
	uint32_t	td_ivdesc;	/* IV descriptor */
	uint32_t	td_ctrdesc;	/* CTR descriptor */
	uint32_t	td_datalen;	/* data length */
	struct sun8i_crypto_adrlen {
		uint32_t	adr;
		uint32_t	len;
	}		td_src[SUN8I_CRYPTO_MAXSEGS],
			td_dst[SUN8I_CRYPTO_MAXSEGS];
	uint32_t	td_nextdesc;	/* next descriptor */
	uint32_t	td_reserved[3];
} __packed;

CTASSERT(sizeof(struct sun8i_crypto_taskdesc) == 44*4);

#endif	/* _ARM_SUN8I_CRYPTO_H */
