/*	$NetBSD: bigpmpy.c,v 1.1.1.1 2001/05/17 20:47:10 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1987, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#include "port_before.h"
#include "global.h"
#include "bigmath.h"
#include "port_after.h"

/* a = b * c.
 */
void BigPmpy (a, b, c, n)
UINT2 *a, *b, *c;
unsigned int n;
{
  register unsigned int i;
  unsigned int cLen;
  
  BigZero (a, 2*n);
  cLen = BigLenw (c, n);
  for (i = 0; i < n; i++)
    a[cLen+i] = BigAcc (&a[i], (unsigned int)b[i], c, cLen);
}
