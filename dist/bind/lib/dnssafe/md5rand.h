/*	$NetBSD: md5rand.h,v 1.1.1.1 2001/05/17 20:47:13 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1994, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _MD5RAND_H_
#define _MD5RAND_H_ 1

#include "digrand.h"
#include "md5.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct A_MD5_RANDOM_CTX {
  A_DigestRandom digestRandom;                               /* "base class" */
  
  unsigned char state[3 * A_MD5_DIGEST_LEN];
  A_MD5_CTX md5Context;
} A_MD5_RANDOM_CTX;

void A_MD5RandomInit PROTO_LIST ((A_MD5_RANDOM_CTX *));
void A_MD5RandomUpdate PROTO_LIST
  ((A_MD5_RANDOM_CTX *, unsigned char *, unsigned int));
void A_MD5RandomGenerateBytes PROTO_LIST
  ((A_MD5_RANDOM_CTX *, unsigned char *, unsigned int));

#ifdef __cplusplus
}
#endif

#endif
