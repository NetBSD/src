/*	$NetBSD: pfkeyv2.h,v 1.4 2006/09/09 16:22:08 manu Exp $	*/

#ifndef __NET_PFKEYV2_H_
#define __NET_PFKEYV2_H_ 1

#include <stdint.h>
#include <linux/pfkeyv2.h>

/* Private allocations for authentication algorithms */
#define SADB_AALG_SHA2_256		SADB_X_AALG_SHA2_256HMAC
#define SADB_X_AALG_SHA2_256		SADB_X_AALG_SHA2_256HMAC
#define SADB_AALG_SHA2_384		SADB_X_AALG_SHA2_384HMAC
#define SADB_X_AALG_SHA2_384		SADB_X_AALG_SHA2_384HMAC
#define SADB_AALG_SHA2_512		SADB_X_AALG_SHA2_512HMAC
#define SADB_X_AALG_SHA2_512		SADB_X_AALG_SHA2_512HMAC
#define SADB_AALG_RIPEMD160HMAC		SADB_X_AALG_RIPEMD160HMAC
#define SADB_X_AALG_MD5              249
#define SADB_X_AALG_SHA              250

/* private allocations - based on RFC2407/IANA assignment */
#define SADB_X_EALG_CAST128CBC	5	/* SADB_X_EALG_CASTCBC? == 6 */
#define SADB_X_EALG_RIJNDAELCBC		SADB_X_EALG_AESCBC
#define SADB_X_EALG_AES			SADB_X_EALG_AESCBC


#define SADB_X_CALG_NONE	0
#define SADB_X_CALG_OUI		1
#define SADB_X_CALG_DEFLATE	2
#define SADB_X_CALG_LZS		3
#define SADB_X_CALG_MAX		4


#define SADB_X_EXT_NONE		0x0000	/* i.e. new format. */
#define SADB_X_EXT_OLD		0x0001	/* old format. */

#define SADB_X_EXT_IV4B		0x0010	/* IV length of 4 bytes in use */
#define SADB_X_EXT_DERIV	0x0020	/* DES derived */
#define SADB_X_EXT_CYCSEQ	0x0040	/* allowing to cyclic sequence. */

	/* three of followings are exclusive flags each them */
#define SADB_X_EXT_PSEQ		0x0000	/* sequencial padding for ESP */
#define SADB_X_EXT_PRAND	0x0100	/* random padding for ESP */
#define SADB_X_EXT_PZERO	0x0200	/* zero padding for ESP */
#define SADB_X_EXT_PMASK	0x0300	/* mask for padding flag */

#define SADB_X_EXT_RAWCPI	0x0080	/* use well known CPI (IPComp) */


#define PFKEY_SOFT_LIFETIME_RATE	80

#define SADB_X_LIFETIME_ALLOCATIONS	0
#define SADB_X_LIFETIME_BYTES		1
#define SADB_X_LIFETIME_ADDTIME		2
#define SADB_X_LIFETIME_USETIME		3


#define PFKEY_ALIGN8(a) (1 + (((a) - 1) | (8 - 1)))
#define	PFKEY_EXTLEN(msg) \
	PFKEY_UNUNIT64(((struct sadb_ext *)(msg))->sadb_ext_len)
#define PFKEY_ADDR_PREFIX(ext) \
	(((struct sadb_address *)(ext))->sadb_address_prefixlen)
#define PFKEY_ADDR_PROTO(ext) \
	(((struct sadb_address *)(ext))->sadb_address_proto)
#define PFKEY_ADDR_SADDR(ext) \
	((struct sockaddr *)((caddr_t)(ext) + sizeof(struct sadb_address)))

/* in 64bits */
#define	PFKEY_UNUNIT64(a)	((a) << 3)
#define	PFKEY_UNIT64(a)		((a) >> 3)

#endif
