/*	$NetBSD: balgmeth.h,v 1.1.1.1 2001/05/17 20:47:10 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1993, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

struct B_AlgorithmInfoType;
struct B_KeyInfoType;

struct B_ALGORITHM_METHOD {
  struct B_AlgorithmInfoType *algorithmInfoType;
  int encryptFlag;
  struct B_KeyInfoType *keyInfoType;
  POINTER alga;
};

