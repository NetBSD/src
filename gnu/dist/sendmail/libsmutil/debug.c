/* $NetBSD: debug.c,v 1.1.1.4 2003/06/01 14:01:17 atatat Exp $ */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: debug.c,v 1.1.1.4 2003/06/01 14:01:17 atatat Exp $");
#endif

/*
 * Copyright (c) 1999-2001 Sendmail, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 */

#include <sendmail.h>

SM_RCSID("@(#)Id: debug.c,v 8.9 2001/09/11 04:04:55 gshapiro Exp")

unsigned char	tTdvect[100];	/* trace vector */
