/*	$NetBSD: hmac_sha1.c,v 1.2 2006/06/23 17:15:18 christos Exp $	*/

/*
 * hmac_sha1 - using HMAC from RFC 2104
 */

#include "namespace.h"
#include <sha1.h> /* XXX */

__weak_alias(hmac_sha1,_hmac_sha1)

#define HMAC_HASH SHA1
#define HMAC_FUNC hmac_sha1
#define HMAC_KAT  hmac_kat_sha1

#define HASH_LENGTH SHA1_DIGEST_LENGTH
#define HASH_CTX SHA1_CTX
#define HASH_Init SHA1Init
#define HASH_Update SHA1Update
#define HASH_Final SHA1Final

#include "../hmac.c"
