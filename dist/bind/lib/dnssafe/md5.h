/*	$NetBSD: md5.h,v 1.1.1.1 2001/05/17 20:47:13 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1994, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _MD5_H_
#define _MD5_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#define A_MD5_DIGEST_LEN 16

typedef struct {
  UINT4 state[4];                                            /* state (ABCD) */
  UINT4 count[2];                 /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                                  /* input buffer */
} A_MD5_CTX;

void A_MD5Init PROTO_LIST ((A_MD5_CTX *));
void A_MD5Update PROTO_LIST ((A_MD5_CTX *, unsigned char *, unsigned int));
void A_MD5Final PROTO_LIST ((A_MD5_CTX *, unsigned char *));

#ifdef __cplusplus
}
#endif

#endif
