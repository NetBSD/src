/*	$KAME: var.h,v 1.14 2003/07/29 04:46:14 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if !defined(_VAR_H_)
#define _VAR_H_

#define MAX3(a, b, c) (a > b ? (a > c ? a : c) : (b > c ? b : c))

#define ISSET(exp, bit) (((exp) & (bit)) == (bit))

#define LALIGN(a) \
    ((a) > 0 ? ((a) &~ (sizeof(long) - 1)) : sizeof(long))

#define RNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#define ARRAYLEN(a)	(sizeof(a)/sizeof(a[0]))

#define BUFSIZE    5120

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef ENABLE_STATS
#include <sys/time.h>
#endif
#include <sys/socket.h>

/*
 * XXX use of GETNAMEINFO(x, y, NULL) is not politically correct,
 * as sizeof(NULL) would be 4, not 0.
 */
#include <sys/socket.h>
#include <netdb.h>

/* var.h is used from non-racoon code (like eaytest), so we can't use niflags */
#define NIFLAGS	(NI_NUMERICHOST | NI_NUMERICSERV)

#define GETNAMEINFO(x, y, z) \
do { \
	if (getnameinfo((x), (x)->sa_len, (y), sizeof(y), (z), sizeof(z), \
			NIFLAGS) != 0) { \
		if (y) \
			strlcpy((y), "(invalid)", sizeof(y)); \
		if (z) \
			strlcpy((z), "(invalid)", sizeof(z)); \
	} \
} while (0);

#include <sys/queue.h>
#ifndef LIST_FOREACH
#define LIST_FOREACH(elm, head, field) \
	for (elm = LIST_FIRST(head); elm; elm = LIST_NEXT(elm, field))
#endif

#include "gcmalloc.h"

#endif /*!defined(_VAR_H_)*/
