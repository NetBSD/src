/*	$NetBSD: warranty.c,v 1.1.2.2 2017/04/10 02:28:24 phil Exp $ */

/*
 * Copyright (C) 1991, 1992, 1993, 1994, 1997 Free Software Foundation, Inc.
 * Copyright (C) 2004, 2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Philip A. Nelson nor the name of the Free Software
 *    Foundation may not be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Warranty information ... */

#include "bcdefs.h"
#include "proto.h"

/* Print the welcome banner. */

void 
welcome(void)
{
  /* No welcome in NetBSD! */
}

/* Print out the version information. */

void
show_bc_version(void)
{
  printf("%s %s\n%s\n", PACKAGE, VERSION, BC_COPYRIGHT);
}

/* Print out the warranty information. */

void 
warranty(const char *prefix)
{
  printf ("\n%s", prefix);
  show_bc_version ();
  printf ("\n    There is no warranty!  See the NetBSD source code.\n");
}
