/*	$NetBSD: amencdec.h,v 1.1.1.1 2001/05/17 20:47:09 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1994, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

typedef struct {
  int (*Query) PROTO_LIST ((unsigned int *, POINTER, POINTER));
  int (*Init) PROTO_LIST ((POINTER, POINTER, POINTER, A_SURRENDER_CTX *));
  int (*Update) PROTO_LIST
    ((POINTER, unsigned char *, unsigned int *, unsigned int,
      unsigned char *, unsigned int, A_SURRENDER_CTX *));
  int (*Final) PROTO_LIST
    ((POINTER, unsigned char *, unsigned int *, unsigned int,
      A_SURRENDER_CTX *));
  int (*GetMaxOutputLen) PROTO_LIST ((POINTER, unsigned int *, unsigned int));
  int (*GetBlockLen) PROTO_LIST ((POINTER, unsigned int *));
} A_ENCRYPT_DECRYPT_ALGA;

