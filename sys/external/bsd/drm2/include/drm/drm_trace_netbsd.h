/*	$NetBSD: drm_trace_netbsd.h,v 1.1 2018/08/27 15:23:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_DRM_DRM_TRACE_NETBSD_H_
#define	_DRM_DRM_TRACE_NETBSD_H_

#include <sys/sdt.h>

#ifdef CREATE_TRACE_POINTS
#define	DEFINE_TRACE0(m,p,n)		SDT_PROBE_DEFINE0(sdt,m,p,n)
#define	DEFINE_TRACE1(m,p,n,a)		SDT_PROBE_DEFINE1(sdt,m,p,n,a)
#define	DEFINE_TRACE2(m,p,n,a,b)	SDT_PROBE_DEFINE2(sdt,m,p,n,a,b)
#define	DEFINE_TRACE3(m,p,n,a,b,c)	SDT_PROBE_DEFINE3(sdt,m,p,n,a,b,c)
#define	DEFINE_TRACE4(m,p,n,a,b,c,d)	SDT_PROBE_DEFINE4(sdt,m,p,n,a,b,c,d)
#define	DEFINE_TRACE5(m,p,n,a,b,c,d,e)	SDT_PROBE_DEFINE5(sdt,m,p,n,a,b,c,d,e)
#define	DEFINE_TRACE6(m,p,n,a,b,c,d,e,f)				      \
	SDT_PROBE_DEFINE6(sdt,m,p,n,a,b,c,d,e,f)
#define	DEFINE_TRACE7(m,p,n,a,b,c,d,e,f,g)				      \
	SDT_PROBE_DEFINE7(sdt,m,p,n,a,b,c,d,e,f,g)
#else
#define	DEFINE_TRACE0(m,p,n)		SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE1(m,p,n,a)		SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE2(m,p,n,a,b)	SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE3(m,p,n,a,b,c)	SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE4(m,p,n,a,b,c,d)	SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE5(m,p,n,a,b,c,d,e)	SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE6(m,p,n,a,b,c,d,e,f)				      \
	SDT_PROBE_DECLARE(sdt,m,p,n)
#define	DEFINE_TRACE7(m,p,n,a,b,c,d,e,f,g)				      \
	SDT_PROBE_DECLARE(sdt,m,p,n)
#endif

#define	TRACE0(m,p,n)			SDT_PROBE0(sdt,m,p,n)
#define	TRACE1(m,p,n,a)			SDT_PROBE1(sdt,m,p,n,a)
#define	TRACE2(m,p,n,a,b)		SDT_PROBE2(sdt,m,p,n,a,b)
#define	TRACE3(m,p,n,a,b,c)		SDT_PROBE3(sdt,m,p,n,a,b,c)
#define	TRACE4(m,p,n,a,b,c,d)		SDT_PROBE4(sdt,m,p,n,a,b,c,d)
#define	TRACE5(m,p,n,a,b,c,d,e)		SDT_PROBE5(sdt,m,p,n,a,b,c,d,e)
#define	TRACE6(m,p,n,a,b,c,d,e,f)	SDT_PROBE6(sdt,m,p,n,a,b,c,d,e,f)
#define	TRACE7(m,p,n,a,b,c,d,e,f,g)	SDT_PROBE7(sdt,m,p,n,a,b,c,d,e,f,g)

#endif	/* _LINUX_TRACEPOINT_H_ */
