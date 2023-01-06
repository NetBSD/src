/*	$NetBSD: ncr5380var.h,v 1.9 2023/01/06 10:28:28 tsutsui Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NCR5380VAR_H
#define _NCR5380VAR_H

/*
 * Various debug definitions
 */
#ifdef DBG_NOSTATIC
#define	static
#endif

#ifdef DBG_SEL
#define DBG_SELPRINT(a, b)	printf(a, b)
#else
#define DBG_SELPRINT(a, b)
#endif

#ifdef DBG_PIO
#define DBG_PIOPRINT(a, b, c)	printf(a, b, c)
#else
#define DBG_PIOPRINT(a, b, c)
#endif

#ifdef DBG_INF
#define DBG_INFPRINT(a, b, c)	a(b, c)
#else
#define DBG_INFPRINT(a, b, c)
#endif

#ifdef DBG_PID
#if 0
static char *last_hit = NULL, *olast_hit = NULL;
#endif
static const char *last_hit[DBG_PID];
#define	PID(a)								\
	{								\
		int i;							\
		for (i = 0; i < DBG_PID - 1; i++)			\
			last_hit[i] = last_hit[i + 1];			\
			last_hit[DBG_PID - 1] = a;			\
	}								\
	/* olast_hit = last_hit; last_hit = a; */
#else
#define	PID(a)
#endif

#endif /* _NCR5380VAR_H */
