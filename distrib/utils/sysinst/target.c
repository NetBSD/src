/*	$NetBSD: target.c,v 1.4 1997/11/03 02:38:52 jonathan Exp $	*/

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
static const char* concat_paths __P((const char *prefix, const char *suffix));
static const char * target_expand __P((const char *pathname));

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

/*
 * concatenate two pathnames.
 * XXX returns either input args or result in a static buffer.
 * The caller must copy if it wants to use the pathname past the 
 * next call to a target-prefixing  function, or to modify the inputs..
 * Used only  internally so this is probably safe.
 */
static const char* 
concat_paths(const char* prefix, const char *suffix)
{
	static char realpath[STRSIZE];

	/* absolute prefix and null suffix? */
	if (prefix[0] == '/' && suffix[0] == 0)
		return prefix;

	/* null prefix and absolute suffix? */
	if (prefix[0] == 0 && suffix[0] == '/')
		return suffix;

	/* avoid "//" */
	if (suffix[0] == '/' || suffix[0] == 0)
		snprintf(realpath, STRSIZE, "%s%s", prefix, suffix);
	else
		snprintf(realpath, STRSIZE, "%s/%s", prefix, suffix);
	return realpath;
}



/*
 * Do target prefix expansion on a pathname.
 * XXX uses concat_paths and so returns result in a static buffer.
 * The caller must copy if it wants to use the pathname past the 
 * next call to a target-prefixing  function, or to modify the inputs..
 * Used only  internally so this is probably safe.
 */
static const char* 
target_expand(const char *tgtpath)
{
	return concat_paths(target_prefix(), tgtpath);
}


/* Is the root we're running from is the root we're trying to upgrade? */
int target_already_root()
{
	/* FIXME */
	return(strcmp(target_prefix(), "/mnt") == 0);
}

/* Is this partname (e.g., "sd0a") mounted as root? */
int is_active_rootpart(const char *partname)
{
	/* FIXME -- compare to kernel's sysctl string with current root. */
	return(0);
}


/* Make a directory, with a prefix like "/mnt" or possibly just "". */
static void 
make_prefixed_dir(const char *prefix, const char *path)
{
	run_prog_or_continue("/bin/mkdir -p %s", concat_paths(prefix, path));
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
	run_prog_or_die("echo %s >> %s", target_expand(path));
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
	run_prog_or_die("cat < /dev/null > %s",  target_expand(path));
}

static int do_target_chdir(const char *dir, int must_succeed)
{
	const char *tgt_dir;
	int error = 0;

	tgt_dir = target_expand(dir);

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

/*
 * Duplicate a file from the current root to the same pathname
 *  in the target system.  Pathname must be an absolute pathname.
 * If we're running in the target, do nothing. 
 */
void dup_file_into_target(const char *filename)
{
	if (!target_already_root()) {
		const char *realpath = target_expand(filename);
		run_prog ("/bin/cp %s %s", filename, realpath);
	}
}


/* Do a mv where both pathnames are  within the target filesystem. */
void mv_within_target_or_die(const char *frompath, const char *topath)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strncpy(realfrom, target_expand(frompath), STRSIZE);
	strncpy(realto, target_expand(topath), STRSIZE);

	run_prog_or_die("mv %s %s", realfrom, realto);
}

/* fopen a pathname in the target. */
FILE*	target_fopen (const char *filename, const char *type)
{
	return fopen(target_expand(filename), type);
}

/*
 * Do a mount onto a moutpoint in the install target.
 * NB: does not prefix mount-from, which probably breaks  nullfs mounts.
 */
int target_mount(const char *fstype, const char *from, const char *on)
{
	int error;
	const char *realmount = target_expand(on);

	error = run_prog("/sbin/mount %s %s %s", fstype, from, realmount);
	return (error);
}

int	target_collect_file(int kind, char **buffer, char *name)
{
	return collect(kind, buffer, target_expand(name));
}
