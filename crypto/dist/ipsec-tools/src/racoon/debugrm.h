/*	$NetBSD: debugrm.h,v 1.4.86.1 2018/04/07 04:11:47 pgoyette Exp $	*/

/* Id: debugrm.h,v 1.4 2006/04/06 14:00:06 manubsd Exp */

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

#ifndef _DEBUGRM_H
#define _DEBUGRM_H

#define DRMDUMPFILE	"/var/tmp/debugrm.dump"

#ifdef NONEED_DRM
#ifndef racoon_malloc
#define	racoon_malloc(sz)	malloc((sz))
#endif
#ifndef racoon_calloc
#define	racoon_calloc(cnt, sz)	calloc((cnt), (sz))
#endif
#ifndef racoon_realloc
#define	racoon_realloc(old, sz)	realloc((old), (sz))
#endif
#ifndef racoon_free
#define	racoon_free(p)		free((p))
#endif
#ifndef racoon_strdup
#define	racoon_strdup(p)	strdup((p))
#endif
#else /*!NONEED_DRM*/
#ifndef racoon_malloc
#define	racoon_malloc(sz)	\
	DRM_malloc(__FILE__, __LINE__, __func__, (sz))
#endif
#ifndef racoon_calloc
#define	racoon_calloc(cnt, sz)	\
	DRM_calloc(__FILE__, __LINE__, __func__, (cnt), (sz))
#endif
#ifndef racoon_realloc
#define	racoon_realloc(old, sz)	\
	DRM_realloc(__FILE__, __LINE__, __func__, (old), (sz))
#endif
#ifndef racoon_free
#define	racoon_free(p)		\
	DRM_free(__FILE__, __LINE__, __func__, (p))
#endif
#ifndef racoon_strdup
#define	racoon_strdup(p)	\
	DRM_strdup(__FILE__, __LINE__, __func__, (p))
#endif
#endif /*NONEED_DRM*/

extern void DRM_init(void);
extern void DRM_dump(void);
extern void *DRM_malloc(const char *, size_t, const char *, size_t);
extern void *DRM_calloc(const char *, size_t, const char *, size_t, size_t);
extern void *DRM_realloc(const char *, size_t, const char *, void *, size_t);
extern void DRM_free(const char *, size_t, const char *, void *);
extern char *DRM_strdup(const char *, size_t, const char *, const char *);

#ifndef NONEED_DRM
#define	vmalloc(sz)	\
	DRM_vmalloc(__FILE__, __LINE__, __func__, (sz))
#define	vdup(old)	\
	DRM_vdup(__FILE__, __LINE__, __func__, (old))
#define	vrealloc(old, sz)	\
	DRM_vrealloc(__FILE__, __LINE__, __func__, (old), (sz))
#define	vfree(p)		\
	DRM_vfree(__FILE__, __LINE__, __func__, (p))
#endif

extern void *DRM_vmalloc(const char *, size_t, const char *, size_t);
extern void *DRM_vrealloc(const char *, size_t, const char *, void *, size_t);
extern void DRM_vfree(const char *, size_t, const char *, void *);
extern void *DRM_vdup(const char *, size_t, const char *, void *);

#endif /* _DEBUGRM_H */
