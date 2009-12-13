/*	$NetBSD: a_md5encrypt.c,v 1.1.1.1 2009/12/13 16:54:59 kardel Exp $	*/

/*
 *	digest support for NTP, MD5 and with OpenSSL more
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ntp_fp.h"
#include "ntp_string.h"
#include "ntp_stdlib.h"
#include "ntp.h"
#ifdef OPENSSL
#include "openssl/evp.h"
#else
#include "ntp_md5.h"
#endif /* OPENSSSL */

/*
 * MD5authencrypt - generate message digest
 *
 * Returns length of MAC including key ID and digest.
 */
int
MD5authencrypt(
	int	type,		/* hash algorithm */
	u_char	*key,		/* key pointer */
	u_int32 *pkt,		/* packet pointer */
	int	length		/* packet length */
	)
{
	u_char	digest[EVP_MAX_MD_SIZE];
	u_int	len;
#ifdef OPENSSL
	EVP_MD_CTX ctx;
#else
	MD5_CTX	md5;
#endif /* OPENSSL */

	/*
	 * Compute digest of key concatenated with packet. Note: the
	 * key type and digest type have been verified when the key
	 * was creaded.
	 */
#ifdef OPENSSL
	INIT_SSL();
	EVP_DigestInit(&ctx, EVP_get_digestbynid(type));
	EVP_DigestUpdate(&ctx, key, (u_int)cache_keylen);
	EVP_DigestUpdate(&ctx, (u_char *)pkt, (u_int)length);
	EVP_DigestFinal(&ctx, digest, &len);
#else /* OPENSSL */
	MD5Init(&md5);
	MD5Update(&md5, key, (u_int)cache_keylen);
	MD5Update(&md5, (u_char *)pkt, (u_int)length);
	MD5Final(digest, &md5);
	len = 16;
#endif /* OPENSSL */
	memmove((u_char *)pkt + length + 4, digest, len);
	return (len + 4);
}


/*
 * MD5authdecrypt - verify MD5 message authenticator
 *
 * Returns one if digest valid, zero if invalid.
 */
int
MD5authdecrypt(
	int	type,		/* hash algorithm */
	u_char	*key,		/* key pointer */
	u_int32	*pkt,		/* packet pointer */
	int	length,	 	/* packet length */
	int	size		/* MAC size */
	)
{
	u_char	digest[EVP_MAX_MD_SIZE];
	u_int	len;
#ifdef OPENSSL
	EVP_MD_CTX ctx;
#else
	MD5_CTX	md5;
#endif /* OPENSSL */

	/*
	 * Compute digest of key concatenated with packet. Note: the
	 * key type and digest type have been verified when the key
	 * was created.
	 */
#ifdef OPENSSL
	INIT_SSL();
	EVP_DigestInit(&ctx, EVP_get_digestbynid(type));
	EVP_DigestUpdate(&ctx, key, (u_int)cache_keylen);
	EVP_DigestUpdate(&ctx, (u_char *)pkt, (u_int)length);
	EVP_DigestFinal(&ctx, digest, &len);
#else /* OPENSSL */
	MD5Init(&md5);
	MD5Update(&md5, key, (u_int)cache_keylen);
	MD5Update(&md5, (u_char *)pkt, (u_int)length);
	MD5Final(digest, &md5);
	len = 16;
#endif /* OPENSSL */
	if ((u_int)size != len + 4) {
		msyslog(LOG_ERR,
		    "MAC decrypt: MAC length error");
		return (0);
	}
	return (!memcmp(digest, (char *)pkt + length + 4, len));
}

/*
 * Calculate the reference id from the address. If it is an IPv4
 * address, use it as is. If it is an IPv6 address, do a md5 on
 * it and use the bottom 4 bytes.
 */
u_int32
addr2refid(sockaddr_u *addr)
{
	u_char		digest[20];
	u_int32		addr_refid;
#ifdef OPENSSL
	EVP_MD_CTX	ctx;
	u_int		len;
#else
	MD5_CTX	md5;
#endif /* OPENSSL */

	if (IS_IPV4(addr))
		return (NSRCADR(addr));

#ifdef OPENSSL
	INIT_SSL();
	EVP_DigestInit(&ctx, EVP_get_digestbynid(NID_md5));
	EVP_DigestUpdate(&ctx, (u_char *)PSOCK_ADDR6(addr),
	    sizeof(struct in6_addr));
	EVP_DigestFinal(&ctx, digest, &len);
#else
	MD5Init(&md5);
	MD5Update(&md5, (u_char *)PSOCK_ADDR6(addr),
	    sizeof(struct in6_addr));
	MD5Final(digest, &md5);
#endif /* OPENSSL */
	memcpy(&addr_refid, digest, 4);
	return (addr_refid);
}
