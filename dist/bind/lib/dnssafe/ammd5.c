/*	$NetBSD: ammd5.c,v 1.1.1.1 2001/05/17 20:47:09 itojun Exp $	*/

/* Copyright (C) RSA Data Security, Inc. created 1990, 1996.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#include "port_before.h"
#include "global.h"
#include "algae.h"
#include "bsafe2.h"
#include "balgmeth.h"
#include "md5.h"
#include "amdigest.h"
#include "port_after.h"

static int MD5Query PROTO_LIST ((unsigned int *, POINTER));
static int MD5Init PROTO_LIST ((POINTER, POINTER, A_SURRENDER_CTX*));
static int MD5Update PROTO_LIST
  ((POINTER, unsigned char *, unsigned int, A_SURRENDER_CTX*));
static int MD5Final PROTO_LIST
  ((POINTER, unsigned char *, unsigned int *, unsigned int, A_SURRENDER_CTX*));
static int MD5GetMaxOutputLen PROTO_LIST ((POINTER, unsigned int *));

static A_DIGEST_ALGA A_MD5_DIGEST = {
  MD5Query, MD5Init, MD5Update, MD5Final, MD5GetMaxOutputLen
};

extern struct B_AlgorithmInfoType AIT_MD5;

B_ALGORITHM_METHOD AM_MD5 =
  {&AIT_MD5, 0, (struct B_KeyInfoType *)NULL_PTR, (POINTER)&A_MD5_DIGEST};

/* Returns 0.
 */
static int MD5Query (contextLen, params)
unsigned int *contextLen;
POINTER params;
{
UNUSED_ARG (params)

  *contextLen = sizeof (A_MD5_CTX);
  return (0);
}

/* Returns 0.
 */
static int MD5Init (context, params, surrenderContext)
POINTER context;
POINTER params;
A_SURRENDER_CTX *surrenderContext;
{
UNUSED_ARG (params)
UNUSED_ARG (surrenderContext)

  A_MD5Init ((A_MD5_CTX *)context);
  return (0);
}

/* Returns 0.
 */
static int MD5Update (context, input, inputLen, surrenderContext)
POINTER context;
unsigned char *input;
unsigned int inputLen;
A_SURRENDER_CTX *surrenderContext;
{
UNUSED_ARG (surrenderContext)

  A_MD5Update ((A_MD5_CTX *)context, input, inputLen);
  return (0);
}

/* Returns 0, AE_OUTPUT_LEN if maxDigestLen is too small.
 */
static int MD5Final
  (context, digest, digestLen, maxDigestLen, surrenderContext)
POINTER context;
unsigned char *digest;
unsigned int *digestLen;
unsigned int maxDigestLen;
A_SURRENDER_CTX *surrenderContext;
{
UNUSED_ARG (surrenderContext)

  if ((*digestLen = A_MD5_DIGEST_LEN) > maxDigestLen)
    return (AE_OUTPUT_LEN);

  A_MD5Final ((A_MD5_CTX *)context, digest);
  return (0);
}

static int MD5GetMaxOutputLen (context, outputLen)
POINTER context;
unsigned int *outputLen;
{
UNUSED_ARG (context)

  *outputLen = A_MD5_DIGEST_LEN;
  return(0);
}
