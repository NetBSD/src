/*	$NetBSD: hmac_md5.c,v 1.2 2005/02/09 21:35:46 kleink Exp $	*/

/*
 * hmac_md5 - using HMAC from RFC 2104
 */

#include "namespace.h"
#include <sys/types.h>
#include <md5.h>

#define HMAC_HASH MD5
#define HMAC_FUNC hmac_md5
#define HMAC_KAT  hmac_kat_md5

#define HASH_LENGTH 16
#define HASH_CTX MD5_CTX
#define HASH_Init MD5Init
#define HASH_Update MD5Update
#define HASH_Final MD5Final

#include "hmac.c"
