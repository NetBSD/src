/* $NetBSD: fileext.h,v 1.3 2003/01/21 23:26:02 nathanw Exp $ */

/*-
 * Copyright (c)2001 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Citrus$
 */

/*
 * file extension
 */
struct __sfileext {
	struct	__sbuf _ub; /* ungetc buffer */
	struct wchar_io_data _wcio;	/* wide char i/o status */
#ifdef _REENTRANT
	mutex_t	_lock;	/* Lock for FLOCKFILE/FUNLOCKFILE */
#endif	
};

#define _EXT(fp) ((struct __sfileext *)(void *)((fp)->_ext._base))
#define _UB(fp) _EXT(fp)->_ub
#ifdef _REENTRANT
#define _LOCK(fp) (_EXT(fp)->_lock)
#define _FILEEXT_SETUP(f, fext) do { \
	/* LINTED */(f)->_ext._base = (unsigned char *)(fext); \
	__smutex_init(&_LOCK(f)); \
	} while (/* LINTED */ 0)
#else
#define _FILEEXT_SETUP(f, fext) /* LINTED */(f)->_ext._base = (unsigned char *)(fext)
#endif
