/*	$NetBSD: aichgen.h,v 1.1.1.1 2001/05/17 20:47:09 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1993, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _AICHGEN_H_
#define _AICHGEN_H_ 1

#include "ainfotyp.h"

struct B_TypeCheck *AITChooseGenerateNewHandler PROTO_LIST
  ((B_AlgorithmInfoType *, B_Algorithm *));

#endif
