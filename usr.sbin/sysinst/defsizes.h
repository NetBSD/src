/*	$NetBSD: defsizes.h,v 1.3 2018/11/17 19:22:48 martin Exp $	*/

/*
 * Copyright 2018 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
 * The size likely needed to build the whole system from source, including
 * X, CTF, MKDEBUG, ...
 * This is used in some messages as a hint when partitioning.
 * Rough estimate, round high, in MB.
 */
#ifndef SYSTEM_BUILD_SIZE
#define	SYSTEM_BUILD_SIZE	15*1024
#endif


#ifndef DEFVARSIZE
#define DEFVARSIZE	32
#endif
#ifndef DEFROOTSIZE
#define DEFROOTSIZE	40
#endif
#ifndef DEFUSRSIZE
#define DEFUSRSIZE	800
#endif
#ifndef DEFSWAPSIZE
#define DEFSWAPSIZE	128
#endif
#ifndef	DEBNEEDMB
#define	DEBNEEDMB	800
#endif
