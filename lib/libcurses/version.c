/*	$NetBSD: version.c,v 1.4 2021/09/06 02:50:43 rin Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: version.c,v 1.4 2021/09/06 02:50:43 rin Exp $");
#endif

#include "curses.h"

#ifndef CURSES_VERSION
/*
 * Bikeshed about what the version should be, if any:
 * https://mail-index.netbsd.org/tech-userlevel/2019/09/02/msg012101.html
 * This is the end result and should at least provide some amusement :)
 */
#define	CURSES_VERSION	"believe in unicorns"
#endif

#ifdef CURSES_VERSION
/*
 * Any version given should be braced to give some indication it's not
 * really a version recognised by NetBSD.
 * It should also have some product branding to indicate from whence
 * if came. For example, if FrobozzCo packaged it:
 * CFLAGS+=	-DCURSES_VERSION="\"FrobozzCo 1.2.3\""
 */
#define	_CURSES_VERSION	" (" CURSES_VERSION ")"
#else
#define	_CURSES_VERSION
#endif

const char *
curses_version(void)
{

	return "NetBSD-Curses" _CURSES_VERSION;
}
