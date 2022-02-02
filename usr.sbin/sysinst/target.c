/*	$NetBSD: target.c,v 1.8.2.6 2022/02/02 04:25:36 msaitoh Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
__RCSID("$NetBSD: target.c,v 1.8.2.6 2022/02/02 04:25:36 msaitoh Exp $");
#endif

/*
 * target.c -- path-prefixing routines to access the target installation
 *  filesystems. Makes the install tools more independent of whether
 *  we're installing into a separate filesystem hierarchy mounted under
 * /targetroot, or into the currently active root mounted on /.
 */

#include <sys/param.h>			/* XXX vm_param.h always defines TRUE*/
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/stat.h>			/* stat() */
#include <sys/mount.h>			/* statfs() */

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <curses.h>			/* defines TRUE, but checks  */
#include <errno.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local  prototypes 
 */

static void make_prefixed_dir (const char *prefix, const char *path);
static int do_target_chdir (const char *dir, int flag);
int	target_test(unsigned int mode, const char *path);
int	target_test_dir (const char *path);	/* deprecated */
int	target_test_file (const char *path);	/* deprecated */
int	target_test_symlink (const char *path);	/* deprecated */

void unwind_mounts(void);

/* Record a mount for later unwinding of target mounts. */
struct unwind_mount {
	struct unwind_mount *um_prev;
	char um_mountpoint[4];		/* Allocated longer... */
};

/* Record a wedge for later deletion after all file systems have been unmounted */
struct umount_delwedge {
	struct umount_delwedge *next;
	char disk[DISKNAMESIZE], wedge[DISKNAMESIZE];
};
struct umount_delwedge *post_umount_dwlist = NULL;

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
#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
void
backtowin(void)
{

	fflush(stdout);	/* curses does not leave stdout linebuffered. */
	getchar();	/* wait for user to press return */
	wrefresh(stdscr);
}
#endif


/*
 * Is the root partition we're running from the same as the root 
 * which the user has selected to install/upgrade?
 * Uses global variable "pm->diskdev" to find the selected device for
 * install/upgrade.
 */
int
target_already_root(void)
{
	char dev[PATH_MAX];
	int rootpart = -1;
	static struct pm_devs *last_pm;
	static int last_res;
	part_id ptn;
	struct disk_partitions *parts, *inner;
	struct disk_part_info info;

	if (pm == NULL)
		return 1;

	if (pm == last_pm)
		return last_res;

	if (pm->cur_system)
		return 1;

	last_pm = pm;
	last_res = 0;

	parts = pm->parts;
	if (parts == NULL) {
		last_res = 0;
		return 0;
	}

	if (pm->no_part) {
		last_res = is_active_rootpart(pm->diskdev, -1);
		return last_res;
	}

	if (pm->parts->pscheme->secondary_partitions != NULL) {
		inner = pm->parts->pscheme->secondary_partitions(parts,
		    pm->ptstart, false);
		if (inner != NULL)
			parts = inner;
	}

	for (ptn = 0; ptn < parts->num_part; ptn++) {
		if (!parts->pscheme->get_part_info(parts, ptn, &info))
			continue;
		if (info.nat_type->generic_ptype != PT_root)
			continue;
		if (!is_root_part_mount(info.last_mounted))
			continue;
		if (!parts->pscheme->get_part_device(parts, ptn,
		    dev, sizeof dev, &rootpart, plain_name, false, true))
			continue;

		last_res = is_active_rootpart(dev, rootpart);
		break;
 	}

	return last_res;
}

/*
 * Could something with this "last mounted on" information be a potential
 * root partition?
 */
bool
is_root_part_mount(const char *last_mounted)
{
	if (last_mounted == NULL)
		return false;

	return strcmp(last_mounted, "/") == 0 ||
	    strcmp(last_mounted, "/targetroot") == 0 ||
	    strcmp(last_mounted, "/altroot") == 0;
}

/*
 * Is this device partition (e.g., "sd0a") mounted as root? 
 */
int
is_active_rootpart(const char *dev, int ptn)
{
	int mib[2];
	char rootdev[SSTRSIZE];
	int rootptn;
	size_t varlen;

	mib[0] = CTL_KERN;
	mib[1] = KERN_ROOT_DEVICE;
	varlen = sizeof(rootdev);
	if (sysctl(mib, 2, rootdev, &varlen, NULL, 0) < 0)
		return 1;

	if (strcmp(dev, rootdev) != 0)
		return 0;

	if (ptn < 0)
		return 1;	/* device only check, or wedge */

	mib[1] = KERN_ROOT_PARTITION;
	varlen = sizeof rootptn;
	rootptn = -1;
	if (sysctl(mib, 2, &rootptn, &varlen, NULL, 0) < 0)
		return 1;

	return ptn == rootptn;
}

/*
 * Pathname  prefixing glue to support installation either 
 * from in-ramdisk miniroots or on-disk diskimages.
 * If our root is on the target disk, the install target is mounted
 * on /targetroot and we need to prefix installed pathnames with /targetroot.
 * otherwise we are installing to the currently-active root and
 * no prefix is needed.
 */
const char *
target_prefix(void)
{
	/*
	 * XXX fetch sysctl variable for current root, and compare 
	 * to the devicename of the install target disk.
	 */
	return(target_already_root() ? "" : targetroot_mnt);
}

/*
 * concatenate two pathnames.
 * XXX returns either input args or result in a static buffer.
 * The caller must copy if it wants to use the pathname past the 
 * next call to a target-prefixing  function, or to modify the inputs..
 * Used only  internally so this is probably safe.
 */
const char *  
concat_paths(const char *prefix, const char *suffix)
{
	static char real_path[MAXPATHLEN];

	/* absolute prefix and null suffix? */
	if (prefix[0] == '/' && suffix[0] == 0)
		return prefix;

	/* null prefix and absolute suffix? */
	if (prefix[0] == 0 && suffix[0] == '/')
		return suffix;

	/* avoid "//" */
	if (suffix[0] == '/' || suffix[0] == 0)
		snprintf(real_path, sizeof(real_path), "%s%s", prefix, suffix);
	else
		snprintf(real_path, sizeof(real_path), "%s/%s", 
		    prefix, suffix);
	return (real_path);
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
target_expand(const char *tgtpath)
{

	return concat_paths(target_prefix(), tgtpath);
}

/* Make a directory, with a prefix like "/targetroot" or possibly just "". */
static void 
make_prefixed_dir(const char *prefix, const char *path)
{

	run_program(0, "/bin/mkdir -p %s", concat_paths(prefix, path));
}

/* Make a directory with a pathname relative to the installation target. */
void
make_target_dir(const char *path)
{

	make_prefixed_dir(target_prefix(), path);
}


static int
do_target_chdir(const char *dir, int must_succeed)
{
	const char *tgt_dir;
	int error;

	error = 0;
	tgt_dir = target_expand(dir);

#ifdef DEBUG
	printf("target_chdir (%s)\n", tgt_dir);
	//return (0);
#endif
	/* chdir returns -1 on error and sets errno. */
	if (chdir(tgt_dir) < 0)
		error = errno;
	if (logfp) {
		fprintf(logfp, "cd to %s\n", tgt_dir);
		fflush(logfp);
	}
	if (script) {
		scripting_fprintf(NULL, "cd %s\n", tgt_dir);
		fflush(script);
	}

	if (error && must_succeed) {
		const char *args[] = { target_prefix(), strerror(error) };
		char *err = str_arg_subst(msg_string(MSG_realdir),
		    __arraycount(args), args);
		fprintf(stderr, "%s\n", err);
		if (logfp)
			fprintf(logfp, "%s\n", err);
		free(err);
		exit(1);
	}
	errno = error;
	return (error);
}

void
target_chdir_or_die(const char *dir)
{

	(void)do_target_chdir(dir, 1);
}

#ifdef notdef
int
target_chdir(const char *dir)
{

	return do_target_chdir(dir, 0);
}
#endif

/*
 * Copy a file from the current root into the target system,
 * where the  destination pathname is relative to the target root.
 * Does not check for copy-to-self when target is  current root.
 */
int
cp_to_target(const char *srcpath, const char *tgt_path)
{
	const char *real_path = target_expand(tgt_path);

	return run_program(0, "/bin/cp %s %s", srcpath, real_path);
}

/*
 * Duplicate a file from the current root to the same pathname
 * in the target system.  Pathname must be an absolute pathname.
 * If we're running in the target, do nothing. 
 */
void
dup_file_into_target(const char *filename)
{

	if (!target_already_root())
		cp_to_target(filename, filename);
}


/*
 * Do a mv where both pathnames are within the target filesystem.
 */
void
mv_within_target_or_die(const char *frompath, const char *topath)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strlcpy(realfrom, target_expand(frompath), sizeof realfrom);
	strlcpy(realto, target_expand(topath), sizeof realto);

	run_program(RUN_FATAL, "mv %s %s", realfrom, realto);
}

/* Do a cp where both pathnames are within the target filesystem. */
int
cp_within_target(const char *frompath, const char *topath, int optional)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strlcpy(realfrom, target_expand(frompath), sizeof realfrom);
	strlcpy(realto, target_expand(topath), sizeof realto);

	if (access(realfrom, R_OK) == -1 && optional)
		return 0;
	return (run_program(0, "cp -p %s %s", realfrom, realto));
}

/* fopen a pathname in the target. */
FILE *
target_fopen(const char *filename, const char *type)
{

	return fopen(target_expand(filename), type);
}

/*
 * Do a mount onto a mountpoint in the install target.
 * Record mountpoint so we can unmount when finished.
 * NB: does not prefix mount-from, which probably breaks nullfs mounts.
 */
int
target_mount_do(const char *opts, const char *from, const char *on)
{
	struct unwind_mount *m;
	int error;
	int len;

	len = strlen(on);
	m = malloc(sizeof *m + len);
	if (m == 0)
		return (ENOMEM);	/* XXX */

	memcpy(m->um_mountpoint, on, len + 1);

#ifdef DEBUG_UNWIND
	endwin();
	fprintf(stderr, "mounting %s with unwind\n", on);
	backtowin();
#endif

	error = run_program(0, "/sbin/mount %s %s %s%s",
			opts, from, target_prefix(), on);
	if (error) {
		free(m);
		return error;
	}
	m->um_prev = unwind_mountlist;
	unwind_mountlist = m;
	return 0;
}

/*
 * Special case - we have mounted the target / readonly
 * to peek at etc/fstab, and now want it undone.
 */
void
umount_root(void)
{

	/* verify this is the only mount */
	if (unwind_mountlist == NULL)
		return;
	if (unwind_mountlist->um_prev != NULL)
		return;

	if (run_program(0, "/sbin/umount %s", target_prefix()) != 0)
		return;

	free(unwind_mountlist);
	unwind_mountlist = NULL;
}


int
target_mount(const char *opts, const char *from, const char *on)
{
	return target_mount_do(opts, from, on);
}

static bool
delete_wedge(const char *disk, const char *wedge)
{
	struct dkwedge_info dkw;
	char diskpath[MAXPATHLEN];
	int fd, error;

	fd = opendisk(disk, O_RDWR, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return false;
	memset(&dkw, 0, sizeof(dkw));
	strlcpy(dkw.dkw_devname, wedge, sizeof(dkw.dkw_devname));
	error = ioctl(fd, DIOCDWEDGE, &dkw);
	close(fd);
	return error == 0;
}

void
register_post_umount_delwedge(const char *disk, const char *wedge)
{
	struct umount_delwedge *dw;

	if (unwind_mountlist == NULL) {
		/* we have nothing mounted, can delete it right now */
		delete_wedge(disk, wedge);
		return;
	}

	dw = calloc(1, sizeof(*dw));
	dw->next = post_umount_dwlist;
	strlcpy(dw->disk, disk, sizeof(dw->disk));
	strlcpy(dw->wedge, wedge, sizeof(dw->wedge));
	post_umount_dwlist = dw;
}

/*
 * unwind the mount stack, unmounting mounted filesystems.
 * For now, ignore any errors in unmount. 
 * (Why would we be unable to unmount?  The user has suspended
 *  us and forked shell sitting somewhere in the target root?)
 */
void
unwind_mounts(void)
{
	struct unwind_mount *m;
	struct umount_delwedge *dw;
	static volatile int unwind_in_progress = 0;

	/* signal safety */
	if (unwind_in_progress)
		return;
	unwind_in_progress = 1;

	while ((m = unwind_mountlist) != NULL) {
		unwind_mountlist = m->um_prev;
#ifdef DEBUG_UNWIND
		endwin();
		fprintf(stderr, "unmounting %s\n", m->um_mountpoint);
		backtowin();
#endif
		run_program(0, "/sbin/umount %s%s",
			target_prefix(), m->um_mountpoint);
		free(m);
	}
	while ((dw = post_umount_dwlist) != NULL) {
		post_umount_dwlist = dw->next;
		delete_wedge(dw->disk, dw->wedge);
		free(dw);
	}
	unwind_in_progress = 0;
}

int
target_collect_file(int kind, char **buffer, const char *name)
{
	const char *realname = target_expand(name);

#ifdef	DEBUG
	printf("collect real name %s\n", realname);
#endif
	return collect(kind, buffer, "%s", realname);
}

/*
 * Verify a pathname already exists in the target root filesystem,
 * by running  test "testflag" on the expanded target pathname.
 */
int
target_test(unsigned int mode, const char *path)
{
	const char *real_path = target_expand(path);
	register int result;

	result = !file_mode_match(real_path, mode);
	scripting_fprintf(NULL, "if [ $? != 0 ]; then echo \"%s does not exist!\"; fi\n", real_path);

#if defined(DEBUG)
	printf("target_test(%o, %s) returning %d\n", mode, real_path, result);
#endif
	return (result);
}

/*
 * Verify a directory already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int
target_test_dir(const char *path)
{

 	return target_test(S_IFDIR, path);
}

/*
 * Verify an ordinary file already exists in the target root 
 * filesystem. Do not create the directory if it doesn't  exist.
 * Assumes that sysinst has already mounted the target root.
 */
int
target_test_file(const char *path)
{

 	return target_test(S_IFREG, path);
}

int
target_test_symlink(const char *path)
{

 	return target_test(S_IFLNK, path);
}

int
target_file_exists_p(const char *path)
{

	return (target_test_file(path) == 0);
}

int
target_dir_exists_p(const char *path)
{

	return (target_test_dir(path) == 0);
}

int
target_symlink_exists_p(const char *path)
{

	return (target_test_symlink(path) == 0);
}

int
target_mounted(void)
{
	return (unwind_mountlist != NULL);
}
