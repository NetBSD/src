/*	$NetBSD: target.c,v 1.2 1997/11/02 08:30:39 jonathan Exp $	*/

/*
 * Copyright 1997 Jonathan Stone
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software develooped for the NetBSD Project by
 *      Jonathan Stone.
 * 4. The name of Jonathan Stone may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JONATHAN STONE ``AS IS''
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

/* target.c -- path-prefixing routines to access the target installation
   filesystems. Makes  the install tools more ndependent of whether
   we're installing into a separate filesystem hierarchy mounted under /mnt,
   or into the currently active root mounted on /.    */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <curses.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local  prototypes 
 */
static void make_prefixed_dir __P((const char *prefix, const char *path));
const char* target_prefix __P((void));
static int do_target_chdir __P((const char *dir, int flag));


/*
 * Pathname  prefixing glue to support installation either 
 * from in-ramdisk miniroots or on-disk diskimages.
 * If our root is on the target disk, the install target is mounted
 * on /mnt and we need to prefix installed pathnames with /mnt.
 * otherwise we are installing to the currently-active root and
 * no prefix is needed.
 */

const char* target_prefix(void)
{
	/*
	 * XXX fetch sysctl variable for current root, and compare 
	 * to the devicename of the instllal target disk.
	 */
	return("/mnt");
}


/* Is the root we're running from is the root we're trying to upgrade? */
int target_already_root()
{
	return(strcmp(target_prefix(), "/mnt") == 0);
}


/* Make a directory, with a prefix like "/mnt" or possibly just "". */
static void 
make_prefixed_dir(const char *prefix, const char *path)
{
 	if (path[0] == '/' || prefix[0] == 0 || path[0] == 0)
		run_prog_or_continue("/bin/mkdir -p %s%s", prefix, path);
	else
		run_prog_or_continue("/bin/mkdir -p %s/%s", prefix, path);
}



/* Make a directory with a pathname relative to the insatllation target. */
void
make_target_dir(const char *path)
{
	make_prefixed_dir(target_prefix(), path);
}


/*  Make a directory with a pathname in the  currently-mounted root. */
void
make_ramdisk_dir(const char *path)
{
	make_prefixed_dir(path, "");
}


/*
 *
 * Append |string| to the  filename |path|, where |path| is
 * relative to the root of the install target.
 * for example, 
 *    echo_to_target_file( "Newbie.NetBSD.ORG", "/etc/myname");
 * would set the default hostname at the next reboot of the installed-on disk.
 */
void
append_to_target_file(const char *path, const char *string)
{
	run_prog_or_die("echo %s >> %s/%s", string, target_prefix(), path);
}

/*
 * As append_to_target_file, but with ftrunc semantics. 
 */
void
echo_to_target_file(const char *path, const char *string)
{
	trunc_target_file(path);
	append_to_target_file(path, string);
}

void
sprintf_to_target_file(const char *path, const char *format, ...)
{
	char lines[STRSIZE];
	va_list ap;

	trunc_target_file(path);

	va_start(format, ap);
	vsnprintf(lines, STRSIZE, format, ap);
	va_end(ap);

	append_to_target_file(path, lines);
}


void
trunc_target_file(const char *path)
{
	run_prog_or_die("cat < /dev/null > %s/%s", target_prefix(), path);
}

static int do_target_chdir(const char *dir, int must_succeed)
{
	char tgt_dir[STRSIZE];
	int error = 0;

	if (dir[0] == '/')
		snprintf(tgt_dir, STRSIZE, "%s%s", target_prefix(), dir);
	else
		snprintf(tgt_dir, STRSIZE, "%s/%s", target_prefix(), dir);


#ifndef DEBUG
	error = chdir(tgt_dir);
	if (error && must_succeed) {
		endwin();
		(void)fprintf(stderr, 
			      msg_string(MSG_realdir), target_prefix());
		exit(1);
	}
	return (error);
#else
	printf("target_chdir (%s)\n", tgt_dir);
	return (0);
#endif
}

void target_chdir_or_die(const char *dir)
{
	(void) do_target_chdir(dir, 1);
}

int target_chdir(const char *dir)
{
	return(do_target_chdir(dir, 0));
}
