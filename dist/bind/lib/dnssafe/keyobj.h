/*	$NetBSD: keyobj.h,v 1.1.1.1 2001/05/17 20:47:13 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1990, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

typedef struct KeyWrap {
  B_Key key;
  char *typeTag;
  struct KeyWrap *selfCheck;
} KeyWrap;

int KeyWrapCheck PROTO_LIST ((KeyWrap *));

