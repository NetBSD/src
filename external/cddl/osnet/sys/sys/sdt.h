/*	$NetBSD: sdt.h,v 1.7.14.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/sdt.h 271802 2014-09-18 20:00:36Z smh $
 */

#ifndef _OPENSOLARIS_SYS_SDT_H_
#define	_OPENSOLARIS_SYS_SDT_H_

#include_next <sys/sdt.h>
#include <sys/dtrace.h>
 
#undef	DTRACE_PROBE
#undef	DTRACE_PROBE1
#undef	DTRACE_PROBE2
#undef	DTRACE_PROBE3
#undef	DTRACE_PROBE4
#undef	DTRACE_PROBE5
#undef	DTRACE_PROBE6
#undef	DTRACE_PROBE7

#define	DTRACE_PROBE(name)
#define	DTRACE_PROBE1(name, type1, arg1)
#define	DTRACE_PROBE2(name, type1, arg1, type2, arg2)
#define	DTRACE_PROBE3(name, type1, arg1, type2, arg2, type3, arg3)
#define	DTRACE_PROBE4(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4) 
#define	DTRACE_PROBE5(name, type1, arg1, type2, arg2, type3, arg3, \
	type4, arg4, type5, arg5)
#define	DTRACE_PROBE6(name, type1, arg1, type2, arg2, type3, arg3, \
	type4, arg4, type5, arg5, type6, arg6)
#define	DTRACE_PROBE7(name, type1, arg1, type2, arg2, type3, arg3, \
	type4, arg4, type5, arg5, type6, arg6, type7, arg7)

#ifdef KDTRACE_HOOKS
SDT_PROBE_DECLARE(sdt, , , set__error);

#define SET_ERROR(err) \
	((sdt_sdt___set__error->id ? \
	(*sdt_probe_func)(sdt_sdt___set__error->id, \
	    (uintptr_t)err, 0, 0, 0, 0) : 0), err)
#else
#define SET_ERROR(err) (err)
#endif

#endif	/* _OPENSOLARIS_SYS_SDT_H_ */
