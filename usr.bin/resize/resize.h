/*	$NetBSD: resize.h,v 1.3 2023/04/20 22:23:53 gutteridge Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <time.h>
#include <termios.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <util.h>
#include <signal.h>
#include <libgen.h>
#include <sys/ioctl.h>

#define DFT_TERMTYPE "vt100"

#define USE_TERMINFO
#define USE_STRUCT_WINSIZE
#define USE_TERMIOS

#define GCC_NORETURN __dead
#define GCC_UNUSED __unused

#define IGNORE_RC(a) (void)(a)
#define ENVP_ARG , char **envp

#define TTYSIZE_STRUCT struct winsize
#define TTYSIZE_ROWS(ws) (ws).ws_row
#define TTYSIZE_COLS(ws) (ws).ws_col
#define SET_TTYSIZE(fd, ws) ioctl((fd), TIOCSWINSZ, &(ws))

#define x_basename(a) estrdup(basename(a))
#define x_strdup(a) estrdup(a)
#define x_getenv(a) getenv(a)
#define x_getlogin(u, p) __nothing
#define x_strindex(s, c) strstr((s), (c))

static int
x_getpwuid(uid_t uid, struct passwd *pw)
{
	struct passwd *p = getpwuid(uid);
	if (p == NULL) {
		memset(pw, 0, sizeof(*pw));
		return 1;
	}
	*pw = *p;
	return 0;
}

#define setup_winsize(ws, row, col, xpixel, ypixel) \
    (void)((ws).ws_row = row, \
	(ws).ws_col = col, \
	(ws).ws_xpixel = xpixel, \
	(ws).ws_ypixel = ypixel)

#define CharOf(a) ((unsigned char)(a))
#define OkPasswd(pw) *((pw)->pw_name)
#define TypeMallocN(t, l) emalloc(sizeof(t) * (l))
#define UIntClr(d, b)	(d) = (d) & ~(b)

#define xtermVersion() "NetBSD"
