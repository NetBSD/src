/*	$NetBSD: kifulprv.h,v 1.1.1.1 2001/05/17 20:47:13 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1990, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

int CacheFullPrivateKey PROTO_LIST
  ((B_Key *, ITEM *, ITEM *, ITEM *, ITEM *, ITEM *, ITEM *));
int GetFullPrivateKeyInfo PROTO_LIST
  ((ITEM *, ITEM *, ITEM *, ITEM *, ITEM *, ITEM *, B_Key *));
