/*	$NetBSD: target.c,v 1.31.2.1 2003/08/11 19:28:59 msaitoh Exp $	*/

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
 *      This product includes software developed for the NetBSD Project by
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

/* Copyright below applies to the realpath() code */

/*
 * Copyright (c) 1989, 1991, 1993, 1995
 *      The Regents of the University of California.  All rights reserved.
 *      
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */     


#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: target.c,v 1.31.2.1 2003/08/11 19:28:59 msaitoh Exp $");
#endif

/*
 * target.c -- path-prefixing routines to access the target installation
 *  filesystems. Makes the install tools more independent of whether
 *  we're installing into a separate filesystem hierarchy mounted under /mnt,
 *  or into the currently active root mounted on /.
 */

#include <sys/param.h>			/* XXX vm_param.h always defines TRUE*/
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/stat.h>			/* stat() */
#include <sys/mount.h>			/* statfs() */

#include <fcntl.h>
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
static const char* get_rootdev (void);
static const char* mounted_rootpart (void);
int target_on_current_disk (void);
int must_mount_root (void);

static void make_prefixed_dir (const char *prefix, const char *path);
static int do_target_chdir (const char *dir, int flag);
int	target_test(unsigned int mode, const char *path);
int	target_test_dir (const char *path);	/* deprecated */
int	target_test_file (const char *path);	/* deprecated */
int	target_test_symlink (const char *path);	/* deprecated */

void backtowin(void);

void unwind_mounts (void);
int mount_with_unwind (const char *fstype, const char *from, const char *on);

/* Record a mount for later unwinding of target mounts. */
struct unwind_mount {
	struct unwind_mount *um_prev;
	char um_mountpoint[STRSIZE];
};

/* Unwind-mount stack */
struct unwind_mount *unwind_mountlist = NULL;

/*
 * Debugging options
 */
/*#define DEBUG_ROOT*/		/* turn on what-is-root? debugging. */
/*#define DEBUG_UNWIND*/	/* turn on unwind-target-mount debugging. */

/*
 * debugging helper. curses...
 */
void
backtowin()
{

	fflush(stdout);	/* curses does not leave stdout linebuffered. */
	getchar();	/* wait for user to press return */
	wrefresh(stdscr);
}

/*
 * Get name of current root device  from kernel via sysctl. 
 * On NetBSD-1.3_ALPHA, this just returns the name of a
 * device (e.g., "sd0"), not a specific partition -- like "sd0a",
 * or "sd0b" for root-in-swap.
 */
static const char *
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
target_on_current_disk()
{
	int same;

	if (strcmp(diskdev, "") == 0)	/* nothing selected yet */
		same = 1;	/* bad assumption: this is a live system */
	else
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
int
must_mount_root()
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
 * Is the root partition we're running from the same as the root 
 * which the user has selected to install/upgrade?
 * Uses global variable "diskdev" to find the selected device for
 * install/upgrade.
 * FIXME -- disklabel-editing code assumes that partition 'a' is always root.
 */
int
target_already_root()
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

	if (strcmp(diskdev, "") == 0) {
		/* no root partition was ever selected. Assume that
		 * the currently mounted one should be used */
		result = 1;
	} else {
		/* append 'a' to the partitionless target disk device name. */
		snprintf(diskdevroot, STRSIZE, "%s%c", diskdev, 'a');
		result = is_active_rootpart(diskdevroot);
	}
	return (result);
}



/*
 * ask the kernel what the current root is.
 * If it's "root_device", we should just give up now.
 * (Why won't the kernel tell us? If we booted with "n",
 * or an explicit bootpath,  the operator told *it* ....)
 *
 * Don't cache the answer in case the user suspends and remounts.
 */
static const char *
mounted_rootpart()
{
	struct statfs statfsbuf;
	int result;

	static char statrootstr[STRSIZE];
	memset(&statfsbuf, 0, sizeof(statfsbuf));
	result = statfs("/", &statfsbuf);
	if (result < 0) {
		fprintf(stderr, "Help! statfs() can't find root: %s\n",
		    strerror(errno));
		fflush(stderr);
		if (logging)
			fprintf(log, "Help! statfs() can't find root: %s\n",
			    strerror(errno));
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
	return (statrootstr);
}

/*
 * Is this device partition (e.g., "sd0a") mounted as root? 
 * Note difference from target_on_current_disk()!
 */
int
is_active_rootpart(devpart)
	const char *devpart;
{
	const char *root = 0;
	int result;
	static char devdirdevpart[STRSIZE];

	/* check to see if the devices match? */

	/* this changes on mounts, so don't cache it. */
	root = mounted_rootpart();

	/* prepend /dev. */
	/* XXX post-1.3, use strstr to strip "/dev" from input. */
	snprintf(devdirdevpart, STRSIZE, "/dev/%s", devpart);

	result = (strcmp(devdirdevpart, root) == 0);

#if defined(DEBUG) || defined(DEBUG_ROOT)
	endwin();
	printf("is_active_rootpart: activeroot = %s, query=%s, mung=%s, answer=%d\n",
	    root, devpart, devdirdevpart, result);
	fflush(stdout);
	backtowin();
#endif

	return (result);
}

/*
 * Pathname  prefixing glue to support installation either 
 * from in-ramdisk miniroots or on-disk diskimages.
 * If our root is on the target disk, the install target is mounted
 * on /mnt and we need to prefix installed pathnames with /mnt.
 * otherwise we are installing to the currently-active root and
 * no prefix is needed.
 */
const char *
target_prefix()
{
	/*
	 * XXX fetch sysctl variable for current root, and compare 
	 * to the devicename of the install target disk.
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
const char*  
concat_paths(prefix, suffix)
	const char* prefix;
	const char *suffix;
{
	static char realpath[MAXPATHLEN];

	/* absolute prefix and null suffix? */
	if (prefix[0] == '/' && suffix[0] == 0)
		return prefix;

	/* null prefix and absolute suffix? */
	if (prefix[0] == 0 && suffix[0] == '/')
		return suffix;

	/* avoid "//" */
	if (suffix[0] == '/' || suffix[0] == 0)
		snprintf(realpath, MAXPATHLEN, "%s%s", prefix, suffix);
	else
		snprintf(realpath, MAXPATHLEN, "%s/%s", prefix, suffix);
	return (realpath);
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
const char *
target_expand(tgtpath)
	const char *tgtpath;
{

	return concat_paths(target_prefix(), tgtpath);
}

/* Make a directory, with a prefix like "/mnt" or possibly just "". */
static void 
make_prefixed_dir(prefix, path)
	const char *prefix;
	const char *path;
{

	run_prog(0, NULL, "/bin/mkdir -p %s", concat_paths(prefix, path));
}

/* Make a directory with a pathname relative to the installation target. */
void
make_target_dir(path)
	const char *path;
{

	make_prefixed_dir(target_prefix(), path);
}

/* Make a directory with a pathname in the currently-mounted root. */
void
make_ramdisk_dir(path)
	const char *path;
{

	make_prefixed_dir(path, "");
}

#if 0
/* unused, will not work with new run.c */
/*
 *
 * Append |string| to the  filename |path|, where |path| is
 * relative to the root of the install target.
 * for example, 
 *    echo_to_target_file( "Newbie.NetBSD.ORG", "/etc/myname");
 * would set the default hostname at the next reboot of the installed-on disk.
 */
void
append_to_target_file(path, string)
	const char *path;
	const char *string;
{

	run_prog(RUN_FATAL, NULL, "echo %s >> %s", string, target_expand(path));
}

/*
 * As append_to_target_file, but with ftrunc semantics. 
 */
void
echo_to_target_file(path, string)
	const char *path;
	const char *string;
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

	va_start(ap, format);
	vsnprintf(lines, STRSIZE, format, ap);
	va_end(ap);

	append_to_target_file(path, lines);
}


void
trunc_target_file(path)
	const char *path;
{

	run_prog(RUN_FATAL, NULL, "cat < /dev/null > %s",  target_expand(path));
}
#endif /* if 0 */

static int
do_target_chdir(dir, must_succeed)
	const char *dir;
	int must_succeed;
{
	const char *tgt_dir;
	int error;

	error = 0;
	tgt_dir = target_expand(dir);

#ifndef DEBUG
	/* chdir returns -1 on error and sets errno. */
	if (chdir(tgt_dir) < 0)
		error = errno;
	if (logging) {
		fprintf(log, "cd to %s\n", tgt_dir);
		fflush(log);
	}
	if (scripting) {
		scripting_fprintf(NULL, "cd %s\n", tgt_dir);
		fflush(script);
	}

	if (error && must_succeed) {
		fprintf(stderr, msg_string(MSG_realdir),
		       target_prefix(), strerror(error));
		if (logging)
			fprintf(log, msg_string(MSG_realdir),
			       target_prefix(), strerror(error));
		exit(1);
	}
	return (error);
#else
	printf("target_chdir (%s)\n", tgt_dir);
	return (0);
#endif
}

void
target_chdir_or_die(dir)
	const char *dir;
{

	(void) do_target_chdir(dir, 1);
}

int
target_chdir(dir)
	const char *dir;
{

	return(do_target_chdir(dir, 0));
}

/*
 * Copy a file from the current root into the target system,
 * where the  destination pathname is relative to the target root.
 * Does not check for copy-to-self when target is  current root.
 */
int
cp_to_target(srcpath, tgt_path)
	const char *srcpath;
	const char *tgt_path;
{
	const char *realpath = target_expand(tgt_path);

	return run_prog(0, NULL, "/bin/cp %s %s", srcpath, realpath);
}

/*
 * Duplicate a file from the current root to the same pathname
 * in the target system.  Pathname must be an absolute pathname.
 * If we're running in the target, do nothing. 
 */
void
dup_file_into_target(filename)
	const char *filename;
{

	if (!target_already_root())
		cp_to_target(filename, filename);
}


/*
 * Do a mv where both pathnames are  within the target filesystem.
 */
void
mv_within_target_or_die(frompath, topath)
	const char *frompath;
	const char *topath;
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strncpy(realfrom, target_expand(frompath), STRSIZE);
	strncpy(realto, target_expand(topath), STRSIZE);

	run_prog(RUN_FATAL, NULL, "mv %s %s", realfrom, realto);
}

/* Do a cp where both pathnames are  within the target filesystem. */
int cp_within_target(frompath, topath)
	const char *frompath;
	const char *topath;
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strncpy(realfrom, target_expand(frompath), STRSIZE);
	strncpy(realto, target_expand(topath), STRSIZE);

	return (run_prog(0, NULL, "cp -p %s %s", realfrom, realto));
}

/* fopen a pathname in the target. */
FILE *
target_fopen(filename, type)
	const char *filename;
	const char *type;
{

	return fopen(target_expand(filename), type);
}

/*
 * Do a mount and record the mountpoint in a list of mounts to 
 * unwind after completing or aborting a mount.
 */
int
mount_with_unwind(fstype, from, on)
	const char *fstype;
	const char *from;
	const char *on;
{
	int error;
	struct unwind_mount * m;

	m = malloc(sizeof(*m));
	if (m == 0)
		return (ENOMEM);	/* XXX */

	strncpy(m->um_mountpoint, on, STRSIZE);
	m->um_prev = unwind_mountlist;
        unwind_mountlist = m;

#ifdef DEBUG_UNWIND
	endwin();
	fprintf(stderr, "mounting %s with unwind\n", on);
	backtowin();
#endif

	error = run_prog(0, NULL, "/sbin/mount %s %s %s", fstype, from, on);
	return (error);
}

/*
 * unwind the mount stack, unmounting mounted filesystems.
 * For now, ignore any errors in unmount. 
 * (Why would we be unable to unmount?  The user has suspended
 *  us and forked shell sitting somewhere in the target root?)
 */
void
unwind_mounts()
{
	struct unwind_mount *m, *prev;
	volatile static int unwind_in_progress = 0;

	/* signal safety */
	if (unwind_in_progress)
		return;
	unwind_in_progress = 1;

	prev = NULL;
	for (m = unwind_mountlist; m;  ) {
		struct unwind_mount *prev;
#ifdef DEBUG_UNWIND
		endwin();
		fprintf(stderr, "unmounting %s\n", m->um_mountpoint);
		backtowin();
#endif
		run_prog(0, NULL, "/sbin/umount %s", m->um_mountpoint);
		prev = m->um_prev;
		free(m);
		m = prev;
	}
	unwind_mountlist = NULL;
	unwind_in_progress = 0;
}

/*
 * Do a mount onto a mountpoint in the install target.
 * NB: does not prefix mount-from, which probably breaks  nullfs mounts.
 */
int
target_mount(fstype, from, on)
	const char *fstype;
	const char *from;
	const char *on;
{
	int error;
	const char *realmount = target_expand(on);

	/* mount and record for unmounting when done.  */
	error = mount_with_unwind(fstype, from, realmount);

	return (error);
}

int
target_collect_file(kind, buffer, name)
	int kind;
	char **buffer;
	char *name;
{
	const char *realname =target_expand(name);

#ifdef	DEBUG
	printf("collect real name %s\n", realname);
#endif
	return collect(kind, buffer, realname);
}

/*
 * Verify a pathname already exists in the target root filesystem,
 * by running  test "testflag" on the expanded target pathname.
 */
int
target_test(mode, path)
	unsigned int mode;
	const char *path;
{
	const char *realpath = target_expand(path);
	register int result;

	result = !file_mode_match(realpath, mode);
	scripting_fprintf(NULL, "if [ $? != 0 ]; then echo \"%s does not exist!\"; fi\n", realpath);

#if defined(DEBUG)
	printf("target_test(%s %s) returning %d\n", test, realpath, result);
#endif
	return (result);
}

/*
 * Verify a directory already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int
target_test_dir(path)
	const char *path;
{

 	return target_test(S_IFDIR, path);
}

/*
 * Verify an ordinary file already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int
target_test_file(path)
	const char *path;
{

 	return target_test(S_IFREG, path);
}

int
target_test_symlink(path)
	const char *path;
{

 	return target_test(S_IFLNK, path);
}

int target_file_exists_p(path)
	const char *path;
{

	return (target_test_file(path) == 0);
}

int target_dir_exists_p(path)
	const char *path;
{

	return (target_test_dir(path) == 0);
}

int target_symlink_exists_p(path)
	const char *path;
{

	return (target_test_symlink(path) == 0);
}

/*
 * XXXX had to include this to deal with symlinks in some places. When the target
 * disk is mounted under /mnt, absolute symlinks on it don't work right. This
 * function will resolve them using the mountpoint as prefix. Copied verbatim
 * from libc, with added prefix handling.
 *
 * char *realpath(const char *path, char resolved_path[MAXPATHLEN]);
 *
 * Find the real name of path, by removing all ".", ".." and symlink
 * components.  Returns (resolved) on success, or (NULL) on failure,
 * in which case the path which caused trouble is left in (resolved).
 */
char *
target_realpath(const char *path, char *resolved)
{
	struct stat sb;
	int fd, n, rootd, serrno, nlnk = 0;
	char *p, *q, wbuf[MAXPATHLEN];

	/* Save the starting point. */
	if ((fd = open(".", O_RDONLY)) < 0) {
		(void)strcpy(resolved, ".");
		return (NULL);
	}

	/*
	 * Find the dirname and basename from the path to be resolved.
	 * Change directory to the dirname component.
	 * lstat the basename part.
	 *     if it is a symlink, read in the value and loop.
	 *     if it is a directory, then change to that directory.
	 * get the current directory name and append the basename.
	 */
	if (target_prefix() != NULL && strcmp(target_prefix(), "") != 0)
		snprintf(resolved, MAXPATHLEN, "%s/%s", target_prefix(), path);
	else {
		(void)strncpy(resolved, path, MAXPATHLEN - 1);
		resolved[MAXPATHLEN - 1] = '\0';
	}
loop:
	q = strrchr(resolved, '/');
	if (q != NULL) {
		p = q + 1;
		if (q == resolved)
			q = "/";
		else {
			do {
				--q;
			} while (q > resolved && *q == '/');
			q[1] = '\0';
			q = resolved;
		}
		if (chdir(q) < 0)
			goto err1;
	} else
		p = resolved;

	/* Deal with the last component. */
	if (lstat(p, &sb) == 0) {
		if (S_ISLNK(sb.st_mode)) {
			if (nlnk++ >= MAXSYMLINKS) {
				errno = ELOOP;
				goto err1;
			}
			n = readlink(p, wbuf, MAXPATHLEN);
			if (n < 0)
				goto err1;
			wbuf[n] = '\0';
			if (wbuf[0] == '/')
				snprintf(resolved, MAXPATHLEN, "%s%s", target_prefix(),
				    wbuf);
			else
				strcpy(resolved, wbuf);
			goto loop;
		}
		if (S_ISDIR(sb.st_mode)) {
			if (chdir(p) < 0)
				goto err1;
			p = "";
		}
	}

	/*
	 * Save the last component name and get the full pathname of
	 * the current directory.
	 */
	(void)strncpy(wbuf, p, (sizeof(wbuf) - 1));

	/*
	 * Call the internal internal version of getcwd which
	 * does a physical search rather than using the $PWD short-cut
	 */
	if (getcwd(resolved, MAXPATHLEN) == 0)
		goto err1;

	/*
	 * Join the two strings together, ensuring that the right thing
	 * happens if the last component is empty, or the dirname is root.
	 */
	if (resolved[0] == '/' && resolved[1] == '\0')
		rootd = 1;
	else
		rootd = 0;

	if (*wbuf) {
		if (strlen(resolved) + strlen(wbuf) + (rootd ? 0 : 1) + 1 >
		    MAXPATHLEN) {
			errno = ENAMETOOLONG;
			goto err1;
		}
		if (rootd == 0)
			(void)strcat(resolved, "/"); /* XXX: strcat is safe */
		(void)strcat(resolved, wbuf);	/* XXX: strcat is safe */
	}

	/* Go back to where we came from. */
	if (fchdir(fd) < 0) {
		serrno = errno;
		goto err2;
	}

	/* It's okay if the close fails, what's an fd more or less? */
	(void)close(fd);
	return (resolved);

err1:	serrno = errno;
	(void)fchdir(fd);
err2:	(void)close(fd);
	errno = serrno;
	return (NULL);
}
