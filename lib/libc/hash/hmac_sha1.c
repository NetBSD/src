/*	$NetBSD: hmac_sha1.c,v 1.2 2005/02/09 21:35:46 kleink Exp $	*/

/*
 * hmac_sha1 - using HMAC from RFC 2104
 */

#include "namespace.h"
#include <sha1.h>

#define HMAC_HASH SHA1
#define HMAC_FUNC hmac_sha1
#define HMAC_KAT  hmac_kat_sha1

#define HASH_LENGTH 20
#define HASH_CTX SHA1_CTX
#define HASH_Init SHA1Init
#define HASH_Update SHA1Update
#define HASH_Final SHA1Final

#include "hmac.c"
