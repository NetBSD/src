/*	$NetBSD: target.c,v 1.8 1997/11/09 12:47:09 jonathan Exp $	*/

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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: target.c,v 1.8 1997/11/09 12:47:09 jonathan Exp $");
#endif


/*
 * target.c -- path-prefixing routines to access the target installation
 *  filesystems. Makes  the install tools more ndependent of whether
 *  we're installing into a separate filesystem hierarchy mounted under /mnt,
 *  or into the currently active root mounted on /.
 */

#include <sys/param.h>			/* XXX vm_param.h always defines TRUE*/
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/stat.h>			/* stat() */
#include <sys/mount.h>			/* statfs() */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <curses.h>			/* defines TRUE, but checks  */
#include <errno.h>


#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"


/*
 * local  prototypes 
 */
static const char*	get_rootdev __P((void));
static const char* mounted_rootpart __P((void));
int target_on_current_disk __P((void));
int must_mount_root __P((void));

static void make_prefixed_dir __P((const char *prefix, const char *path));
const char* target_prefix __P((void));
static int do_target_chdir __P((const char *dir, int flag));
static const char* concat_paths __P((const char *prefix, const char *suffix));
int target_test(const char *test, const char *path);

void backtowin(void);

/* turn on what-is-root? debugging. */
/*#define DEBUG_ROOT*/

/*
 * debugging helper. curses...
 */
void backtowin(void)
{

	fflush(stdout);	/* curses does not leave stdout linebuffered. */

	getchar();	/* wait for user to press return */

	puts (CL);
	wrefresh(stdscr);
}

/*
 * Get name of current root device  from kernel via sysctl. 
 * On NetBSD-1.3_ALPHA, this just returns the name of a
 * device (e.g., "sd0"), not a specific partition -- like "sd0a",
 * or "sd0b" for root-in-swap.
*/
static const char*
get_rootdev()
{
	int mib[2];
	static char rootdev[STRSIZE];
	size_t varlen;

	mib[0] = CTL_KERN;
	mib[1] = KERN_ROOT_DEVICE;
	varlen = sizeof(rootdev);
	if (sysctl(mib, 2, rootdev, &varlen, NULL, 0) < 0)
		return (NULL);

#ifdef	DEBUG
	printf("get_rootdev(): sysctl returns %s\n", rootdev);
#endif
	return (rootdev);
}


/*
 * Check if current root and target are on the same 
 * device (e.g., both on "sd0") or on different devices
 * e.g., target is "sd0" and root is "le0" (nfs).
 */
int
target_on_current_disk(void)
{
	int same;
	same = (strcmp(diskdev, get_rootdev()) == 0);
	return (same);
}

/*
 * must_mount_root  -- check to see if the current root
 * partition  and the target root partition are on the same
 * device.  If they are, we need to have the root mounted read/write
 * in order to find out whether they're the same partition.
 * (this is arguably a bug in the kernel API.)
 *
 * check to see if they're
 */
int must_mount_root(void)
{
	int result;

#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
		endwin();
		printf("must_mount_root\n");
		backtowin();
#endif

	/* if they're  on different devices, we're OK. */
	if (target_on_current_disk() == 0) {
#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
		endwin();
		printf("must_mount_root: %s and %s, no?\n",
		       diskdev, get_rootdev());
		fflush(stdout);
		backtowin();
#endif
		return (0);
	}


	/* If they're on the same device, and root not mounted, yell. */
	result = (strcmp(mounted_rootpart(), "root_device") == 0);

#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
		endwin();
		printf("must_mount_root %s and root_device gives %d\n",
		       mounted_rootpart(), result);
		fflush(stdout);
		backtowin();
#endif

	return (result);
}

/*
 * Is the root pattion we're running from the same as the root 
 * which the  user has selected to install/upgrade?
 * Uses global variable "diskdev" to find the selected device for
 * install/upgrade.
 * FIXME -- disklabel-editing code assumes that partition 'a' is always root.
 */
int target_already_root()
{
	register int result;
	char diskdevroot[STRSIZE];


	/* if they're  on different devices, we're OK. */
	if (target_on_current_disk() == 0)
		return (0);

	/*
	 * otherwise, install() or upgrade() should already did
	 * have forced the user to explicitly mount the root,
	 * so we can find out via statfs().  Abort if not.
	 */

	/* append 'a' to the partitionless target disk device name. */
	snprintf(diskdevroot, STRSIZE, "%s%c", diskdev, 'a');
	result = is_active_rootpart(diskdevroot);
	return(result);
}



/*
 * ask the kernel what the current root is.
 * If it's "root_device", we should jsut give up now.
 * (Why won't the kernel tell us? If we booted with "n",
 * or an explicit bootpath,  the operator told *it* ....)
 *
 * Don't cache the answer in case the user suspnnds and remounts.
 */
static const char*
mounted_rootpart(void)
{
	struct statfs statfsbuf;
	int result;

	static char statrootstr[STRSIZE];
	bzero(&statfsbuf, sizeof(statfsbuf));
	result = statfs("/", &statfsbuf);
	if (result < 0) {
	  	endwin();
		fprintf(stderr, "Help! statfs() can't find root: %s\n",
		       strerror(errno));
		fflush(stderr);
		exit(errno);
		return(0);
	}
#if defined(DEBUG)
	endwin();
	printf("mounted_rootpart: got %s on %s\n", 
	       statfsbuf.f_mntonname, statfsbuf.f_mntfromname);
	fflush(stdout);
	backtowin();
#endif

	/*
	 * Check for unmounted root. We can't tell which partition
	 */
	strncpy(statrootstr, statfsbuf.f_mntfromname, STRSIZE);
	return(statrootstr);
}


/*
 * Is this device partition (e.g., "sd0a") mounted as root? 
 * Note difference from target_on_current_disk()!
 */
int
is_active_rootpart(const char *devpart)
{
	const char *root = 0;
	int result;

	/* check to see if the devices match? */

	/* this changes on mounts, so don't cache it. */
	root = mounted_rootpart();

	result = (strcmp(devpart, root) == 0);

#if defined(DEBUG) || defined(DEBUG_ROOT)
	endwin();
	printf("is_active_rootpart: root devpart = %s, query=%s, answer=%d\n",
	       root, devpart, result);
	fflush(stdout);
	backtowin();
#endif

	return(result);
}



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
	return(target_already_root() ? "" : "/mnt");
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
 *
 * Not static so other functions can generate target related file names.
 */
const char* 
target_expand(const char *tgtpath)
{
	return concat_paths(target_prefix(), tgtpath);
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
	run_prog_or_die("echo %s >> %s", string, target_expand(path));
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
	/* chdir returns -1 on error and sets errno. */
	if (chdir(tgt_dir) < 0) {
		error = errno;
	}
	if (error && must_succeed) {
		endwin();
		fprintf(stderr, msg_string(MSG_realdir),
		       target_prefix(), strerror(error));
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

/* Do a cp where both pathnames are  within the target filesystem. */
int cp_within_target(const char *frompath, const char *topath)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strncpy(realfrom, target_expand(frompath), STRSIZE);
	strncpy(realto, target_expand(topath), STRSIZE);

	return (run_prog("cp -p %s %s", realfrom, realto));
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
	register const char *realname =target_expand(name);

#ifdef	DEBUG
	printf("collect real name %s\n", realname);
#endif
	return collect(kind, buffer, realname);
}

/*
 * Verify a pathname already exists in the target root filesystem,
 * by running  test "testflag" on the expanded target pathname.
 */

int target_test(const char *test, const char *path)
{
	const char *realpath = target_expand(path);
	register int result;

	result = run_prog ("test %s %s", test, realpath);

#if defined(DEBUG)
	printf("target_test(%s %s) returning %d\n",
			test, realpath, result);
#endif
	return result;
}

/*
 * Verify a directory already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int target_verify_dir(const char *path)
{
 	return target_test("-d", path);
}

/*
 * Verify an ordinary file already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int target_verify_file(const char *path)
{
 	return target_test("-f", path);
}
