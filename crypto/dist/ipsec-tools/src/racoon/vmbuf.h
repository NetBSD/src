/*	$NetBSD: vmbuf.h,v 1.4.86.1 2018/05/21 04:35:49 pgoyette Exp $	*/

/* Id: vmbuf.h,v 1.4 2005/10/30 10:28:44 vanhu Exp */

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

#ifndef _VMBUF_H
#define _VMBUF_H

/*
 *	bp      v
 *	v       v
 *	........................
 *	        <--------------> l
 *	<----------------------> bl
 */
typedef struct _vchar_t_ {
#if notyet
	u_int32_t t;	/* type of the value */
	vchar_t *n;	/* next vchar_t buffer */
	size_t bl;	/* length of the buffer */
	caddr_t bp;	/* pointer to the buffer */
#endif
	size_t l;	/* length of the value */
	caddr_t v;	/* place holder to the pointer to the value */
} vchar_t;

#define VPTRINIT(p) \
do { \
	if (p) { \
		vfree(p); \
		(p) = NULL; \
	} \
} while(0);

#if defined(__APPLE__) && defined(__MACH__)
/* vfree is already defined in Apple's system libraries */
#define vfree	vmbuf_free
#endif

extern vchar_t *vmalloc __P((size_t));
extern vchar_t *vrealloc __P((vchar_t *, size_t));
extern void vfree __P((vchar_t *));
extern vchar_t *vdup __P((vchar_t *));

#endif /* _VMBUF_H */
