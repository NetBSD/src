/*	$NetBSD: ralink_debug.h,v 1.2 2011/07/28 15:38:49 matt Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RALINK_DEBUG_H_
#define _RALINK_DEBUG_H_

/* Co-locate some debug routines to help keep the code clean.
 *  #define one or more ENABLE_RALINK_DEBUG_xxxx macros before including
 *  this file to turn on macros.  If none are defined the debug code
 *  won't be compiled in.
 */ 

/*
 * High-level debug compile flag.  If this isn't defined, this is
 *  a release build.
 */

/* Debugging options */
#ifndef ENABLE_RALINK_DEBUG_ERROR
 #define ENABLE_RALINK_DEBUG_ERROR 0
#endif
#ifndef ENABLE_RALINK_DEBUG_FUNC
 #define ENABLE_RALINK_DEBUG_FUNC  0
#endif
#ifndef ENABLE_RALINK_DEBUG_MISC
 #define ENABLE_RALINK_DEBUG_MISC  0
#endif
#ifndef ENABLE_RALINK_DEBUG_INFO
 #define ENABLE_RALINK_DEBUG_INFO  0
#endif
#ifndef ENABLE_RALINK_DEBUG_REG
 #define ENABLE_RALINK_DEBUG_REG   0
#endif
/* don't check anything in with this option.  Just use this if you want to
 * force a specific statement and the above options don't give you fine enough
 * control.
 */
#ifndef ENABLE_RALINK_DEBUG_FORCE
 #define ENABLE_RALINK_DEBUG_FORCE   0
#endif

#define RALINK_DEBUG_ERROR ((ENABLE_RALINK_DEBUG_ERROR ? 1 : 0) << 0)
#define RALINK_DEBUG_MISC  ((ENABLE_RALINK_DEBUG_MISC  ? 1 : 0) << 1)
#define RALINK_DEBUG_FUNC  ((ENABLE_RALINK_DEBUG_FUNC  ? 1 : 0) << 2)
#define RALINK_DEBUG_INFO  ((ENABLE_RALINK_DEBUG_INFO  ? 1 : 0) << 3)
#define RALINK_DEBUG_REG   ((ENABLE_RALINK_DEBUG_REG   ? 1 : 0) << 4)
#define RALINK_DEBUG_FORCE ((ENABLE_RALINK_DEBUG_FORCE ? 1 : 0) << 5)

#ifndef RALINK_DEBUG_ALL
#define RALINK_DEBUG_ALL	( RALINK_DEBUG_ERROR | RALINK_DEBUG_MISC | RALINK_DEBUG_FUNC | \
			  RALINK_DEBUG_INFO | RALINK_DEBUG_REG | RALINK_DEBUG_FORCE )
#endif

/*
 * RALINK_DEBUG_0 is used instead of:
 *
 * #if 0
 * 	RALINK_DEBUG(x, blah);
 * #endif
 *
 * in order to preserve the if'ed-out prints, without the clutter;
 * alternatively, just delete them and this macro
 */
#define RALINK_DEBUG_0(n, args...)	do { } while (0)

#ifdef CPDEBUG

#if RALINK_DEBUG_ALL > 0
#define RALINK_DEBUG(n, args...)		\
	do {				\
		if (RALINK_DEBUG_ALL & (n)) \
			printf(args);	\
	} while (0)
#else
#define RALINK_DEBUG(n, args...)   do { } while (0)
#endif 

/* helper so we don't have to retype a bunch of times */
#define RALINK_DEBUG_FUNC_ENTRY()	\
		RALINK_DEBUG(RALINK_DEBUG_FUNC, "%s() entry\n", __FUNCTION__)
#define RALINK_DEBUG_FUNC_EXIT()	\
		RALINK_DEBUG(RALINK_DEBUG_FUNC, "%s() exit\n", __FUNCTION__)

#else	/* DEBUG not defined (release build) */

#define RALINK_DEBUG(n, args...)
#define RALINK_DEBUG_FUNC_ENTRY()
#define RALINK_DEBUG_FUNC_EXIT()

#endif	/* DEBUG defined */

#endif	/* _RALINK_DEBUG_H_ */
