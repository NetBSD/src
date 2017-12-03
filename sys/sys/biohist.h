/*	$NetBSD: biohist.h,v 1.2.18.2 2017/12/03 11:39:20 jdolecek Exp $ */

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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

#ifndef _KERN_BIOHIST_H_
#define _KERN_BIOHIST_H_

#ifdef _KERNEL_OPT
#include "opt_biohist.h"
#endif

/*
 * Make BIOHIST_PRINT force on KERNHIST_PRINT for at least BIOHIST_* usage.
 */
#if defined(BIOHIST_PRINT) && !defined(KERNHIST_PRINT)
#define KERNHIST_PRINT 1
#endif

#include <sys/kernhist.h>

#if defined(BIOHIST)

#define BIOHIST_DECL(NAME)		KERNHIST_DECL(NAME)
#define BIOHIST_DEFINE(NAME)		KERNHIST_DEFINE(NAME)
#define BIOHIST_INIT(NAME,N)		KERNHIST_INIT(NAME,N)
#define BIOHIST_INIT_STATIC(NAME,BUF)	KERNHIST_INIT_STATIC(NAME,BUF)
#define BIOHIST_INITIALIZER(NAME,BUF)	KERNHIST_INITIALIZER(NAME,BUF)
#define BIOHIST_LINK_STATIC(NAME)	KERNHIST_LINK_STATIC(NAME)
#define BIOHIST_LOG(NAME,FMT,A,B,C,D)	KERNHIST_LOG(NAME,FMT,A,B,C,D)
#define BIOHIST_CALLED(NAME)		KERNHIST_CALLED(NAME)
#define BIOHIST_CALLARGS(NAME,FMT,A,B,C,D)	\
					KERNHIST_CALLARGS(NAME,FMT,A,B,C,D)
#define BIOHIST_FUNC(FNAME)		KERNHIST_FUNC(FNAME)

#ifndef BIOHIST_SIZE
#define BIOHIST_SIZE 500
#endif  /* BIOHIST_SIZE */

#else

#define BIOHIST_DECL(NAME)
#define BIOHIST_DEFINE(NAME)
#define BIOHIST_INIT(NAME,N)
#define BIOHIST_INIT_STATIC(NAME,BUF)
#define BIOHIST_INITIALIZER(NAME,BUF)
#define BIOHIST_LINK_STATIC(NAME)
#define BIOHIST_LOG(NAME,FMT,A,B,C,D)
#define BIOHIST_CALLED(NAME)
#define BIOHIST_CALLARGS(NAME,FMT,A,B,C,D)
#define BIOHIST_FUNC(FNAME)

#endif	/* BIOHIST */

BIOHIST_DECL(biohist);

#endif	/* _KERN_BIOHIST_H_ */
