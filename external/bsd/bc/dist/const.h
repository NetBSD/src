/*	$NetBSD: const.h,v 1.1.4.2 2017/04/26 02:52:19 pgoyette Exp $ */

/*
 * Copyright (C) 1991-1994, 1997, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names Philip A. Nelson and Free Software Foundation may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/* const.h: Constants for bc. */

/* Define INT_MAX and LONG_MAX if not defined.  Assuming 32 bits... */

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif
#ifndef LONG_MAX
#define LONG_MAX 0x7FFFFFFF
#endif


/* Define constants in some reasonable size.  The next 4 constants are
   POSIX constants. */

#ifdef BC_BASE_MAX
  /* <limits.h> on a POSIX.2 system may have defined these.  Override. */
# undef BC_BASE_MAX
# undef BC_SCALE_MAX
# undef BC_STRING_MAX
# undef BC_DIM_MAX
#endif

#define BC_BASE_MAX   INT_MAX
#define BC_SCALE_MAX  INT_MAX
#define BC_STRING_MAX INT_MAX


/* Definitions for arrays. */

#define BC_DIM_MAX   16777215     /* this should be NODE_SIZE^NODE_DEPTH-1 */

#define   NODE_SIZE        64     /* Must be a power of 2. */
#define   NODE_MASK      0x3f     /* Must be NODE_SIZE-1. */
#define   NODE_SHIFT        6     /* Number of 1 bits in NODE_MASK. */
#define   NODE_DEPTH        4


/* Other BC limits defined but not part of POSIX. */

#define BC_LABEL_GROUP 64
#define BC_LABEL_LOG    6
#define BC_START_SIZE  1024	/* Initial code body size. */

/* Maximum number of variables, arrays and functions and the
   allocation increment for the dynamic arrays. */

#define MAX_STORE   32767
#define STORE_INCR     32

/* Other interesting constants. */

#define FALSE 0
#define TRUE  1

/* for use with lookup (). */
#define SIMPLE   0
#define ARRAY    1
#define FUNCT    2
#define FUNCTDEF 3

#ifdef __STDC__
#define CONST const
#define VOID  void
#else
#define CONST
#define VOID
#endif
