/*	$KAME: debugrm.h,v 1.3 2001/11/26 16:54:29 sakane Exp $	*/

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
#else /*!NONEED_DRM*/
#ifndef racoon_malloc
#define	racoon_malloc(sz)	\
	DRM_malloc(__FILE__, __LINE__, __FUNCTION__, (sz))
#endif
#ifndef racoon_calloc
#define	racoon_calloc(cnt, sz)	\
	DRM_calloc(__FILE__, __LINE__, __FUNCTION__, (cnt), (sz))
#endif
#ifndef racoon_realloc
#define	racoon_realloc(old, sz)	\
	DRM_realloc(__FILE__, __LINE__, __FUNCTION__, (old), (sz))
#endif
#ifndef racoon_free
#define	racoon_free(p)		\
	DRM_free(__FILE__, __LINE__, __FUNCTION__, (p))
#endif
#endif /*NONEED_DRM*/

extern void DRM_init __P((void));
extern void DRM_dump __P((void));
extern void *DRM_malloc __P((char *, int, char *, size_t));
extern void *DRM_calloc __P((char *, int, char *, size_t, size_t));
extern void *DRM_realloc __P((char *, int, char *, void *, size_t));
extern void DRM_free __P((char *, int, char *, void *));

#ifndef NONEED_DRM
#define	vmalloc(sz)	\
	DRM_vmalloc(__FILE__, __LINE__, __FUNCTION__, (sz))
#define	vdup(old)	\
	DRM_vdup(__FILE__, __LINE__, __FUNCTION__, (old))
#define	vrealloc(old, sz)	\
	DRM_vrealloc(__FILE__, __LINE__, __FUNCTION__, (old), (sz))
#define	vfree(p)		\
	DRM_vfree(__FILE__, __LINE__, __FUNCTION__, (p))
#endif

extern void *DRM_vmalloc __P((char *, int, char *, size_t));
extern void *DRM_vrealloc __P((char *, int, char *, void *, size_t));
extern void DRM_vfree __P((char *, int, char *, void *));
extern void *DRM_vdup __P((char *, int, char *, void *));
