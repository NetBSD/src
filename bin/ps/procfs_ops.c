/*	$NetBSD: procfs_ops.c,v 1.6 1999/10/15 19:31:25 jdolecek Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Grayson (bgrayson@netbsd.org).
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h> /* for rusage */

#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <kvm.h>

#include "ps.h"

/* Assume that no process status file will ever be larger than this. */
#define STATUS_SIZE	8192

/*
 * Handy macro for only printing a warning once.  Notice that
 * one needs to use two sets of parentheses when invoking the
 * macro:  WARNX_ONLY_ONCE(("mesgstr", arg1, arg2, ...));
 */
#define WARNX_ONLY_ONCE(x)						\
{									\
	static int firsttime = 1;					\
	if (firsttime) {						\
		firsttime = 0;						\
		warnx x ;						\
	}								\
}

static int verify_procfs_fd __P((int, const char *));
static int parsekinfo __P((const char *, KINFO *));

static int
verify_procfs_fd(fd, path)
	int     fd;
	const char *path;
{
	struct statfs procfsstat;

	/*
	 * If the fstatfs fails, die immediately.  Since we already have the
	 * FD open, any error is probably one that can't be worked around.
	 */
	if (fstatfs(fd, &procfsstat))
		err(1, "fstatfs on %s", path);

	/* Now verify that the open file is truly on a procfs filesystem. */
	if (strcmp(procfsstat.f_fstypename, MOUNT_PROCFS)) {
		warnx("%s is on a `%s' filesystem, not `procfs'", path,
		    procfsstat.f_fstypename);
		return -1;
	}
	return 0;
}

static int
parsekinfo(path, ki)
	const char *path;
	KINFO *ki;
{
	char    fullpath[MAXPATHLEN];
	int     dirfd, fd, nbytes, devmajor, devminor;
	struct timeval usertime, systime, starttime;
	char    buff[STATUS_SIZE];
	char    flagstr[256];
	struct kinfo_proc *kp = ki->ki_p;

	/*
	 * Verify that /proc/<pid> is a procfs file (and that no one has
	 * mounted anything on top of it).  If we didn't do this, an intruder
	 * could hide processes by simply mount_null'ing /tmp on top of the
	 * /proc/<pid> directory.  (And we can't just print warnings if we
	 * fail to open /proc/<pid>/status, because the process may have died
	 * since our getdents() call.)
	 */
	snprintf(fullpath, MAXPATHLEN, "/proc/%s", path);
	dirfd = open(fullpath, O_RDONLY, 0);
	if (verify_procfs_fd(dirfd, fullpath)) {
		close(dirfd);
		return -1;
	}

	/* Open /proc/<pid>/status, and parse it into the kinfo_proc. */
	snprintf(fullpath, MAXPATHLEN, "/proc/%s/status", path);
	fd = open(fullpath, O_RDONLY, 0);
	close(dirfd);
	if (fd == -1) {
		/*
		 * Don't print warning, as the process may have died since our
		 * scan of the directory entries.
		 */
		return -1;	/* Process may no longer exist. */
	}

	/*
	 * Bail out for this process attempt if it isn't on a procfs.  Some
	 * intruder could have mounted something on top of portions of /proc.
	 */
	if (verify_procfs_fd(fd, fullpath)) {
		close(fd);
		return -1;
	}

	nbytes = read(fd, buff, STATUS_SIZE - 1);
	close(fd);
	if (nbytes <= 0) {
		/*
		 * Don't print warning, as the process may have died since our
		 * scan of the directory entries.
		 */
		return -1;	/* Process may no longer exist. */
	}

	/* Make sure the buffer is terminated. */
	buff[nbytes] = '\0';

	sscanf(buff, "%s %d %d %d %d %d,%d %s %ld,%ld %ld,%ld %ld,%ld %s %d",
	    kp->kp_proc.p_comm, &kp->kp_proc.p_pid,
	    &kp->kp_eproc.e_ppid, &kp->kp_eproc.e_pgid,
	    &kp->kp_eproc.e_sid, &devmajor, &devminor,
	    flagstr, &starttime.tv_sec, &starttime.tv_usec,
	    &usertime.tv_sec, &usertime.tv_usec,
	    &systime.tv_sec, &systime.tv_usec,
	    kp->kp_eproc.e_wmesg, &kp->kp_eproc.e_ucred.cr_uid);

	kp->kp_proc.p_wmesg = kp->kp_eproc.e_wmesg;
	kp->kp_proc.p_wchan = (void *) 1;	/* XXX Set it to _something_. */
	kp->kp_eproc.e_tdev = makedev(devmajor, devminor);

	/* Put both user and sys time into rtime field.  */
	kp->kp_proc.p_rtime.tv_sec = usertime.tv_sec + systime.tv_sec;
	kp->kp_proc.p_rtime.tv_usec = usertime.tv_usec + systime.tv_usec;

	/* if starttime.[u]sec is != -1, it's in-memory process */
	if (starttime.tv_sec != -1 && starttime.tv_usec != -1) {
		kp->kp_proc.p_flag |= P_INMEM;
		ki->ki_u.u_valid = 1;
		ki->ki_u.u_start.tv_sec = starttime.tv_sec;
		ki->ki_u.u_start.tv_usec = starttime.tv_usec;
	}

	/*
	 * CPU time isn't shown unless the ki_u.u_valid flag is set.
	 * Unfortunately, we don't have access to that here.
	 */

	/* Set the flag for whether or not there is a controlling terminal. */
	if (strstr(flagstr, "ctty"))
		kp->kp_proc.p_flag |= P_CONTROLT;

	/* Set the flag for whether or not this process is session leader */
	if (strstr(flagstr, "sldr"))
		kp->kp_eproc.e_flag |= EPROC_SLEADER;

	return 0;
}

KINFO *
getkinfo_procfs(op, arg, cnt)
	int     op, arg;
	int    *cnt;
{
	struct stat statbuf;
	int     procdirfd, nbytes, knum = 0, maxknum = 0;
	char   *direntbuff;
	struct kinfo_proc *kp;
	KINFO	*ki;
	int     mib[4];
	size_t  len;
	struct statfs procfsstat;

	/* First, make sure that /proc is a procfs filesystem. */
	if (statfs("/proc", &procfsstat)) {
		warn("statfs on /proc failed");
		return 0;
	}
	if (strcmp(procfsstat.f_fstypename, MOUNT_PROCFS)) {
		warnx("/proc exists but does not have a procfs mounted on it.");
		return 0;
	}

	/*
	 * Try to stat /proc/1/status.  If we can't do that, then just return
	 * right away.
	 */
	if (stat("/proc/1/status", &statbuf)) {
		warn("stat of /proc/1/status");
		return 0;
	}

	/* Now, try to open /proc, and read in all process' information. */
	procdirfd = open("/proc", O_RDONLY, 0);
	if (procdirfd == -1) {
		warn("open of /proc directory");
		close(procdirfd);
		return 0;
	}
	if (verify_procfs_fd(procdirfd, "/proc")) {
		close(procdirfd);
		return 0;
	}

	direntbuff = malloc(statbuf.st_blksize);
	if (direntbuff == NULL) {
		warn("malloc() of %d bytes", statbuf.st_blksize);
		close(procdirfd);
		return 0;
	}

	/*
	 * Use sysctl to find out the total number of processes. There's still
	 * a race condition -- once we do the sysctl, someone could use a
	 * sysctl to bump it, and fork off a lot of processes.  So, to be
	 * _really_ safe, let's allocate twice as much memory.
	 */
	mib[0] = CTL_KERN;
	mib[1] = KERN_MAXPROC;
	len = sizeof(maxknum);
	if (sysctl(mib, 2, &maxknum, &len, NULL, 0) == -1)
		err(1, "sysctl to fetch maxproc");
	maxknum *= 2;		/* Double it, to be really paranoid.  */

	kp = (struct kinfo_proc *) calloc(sizeof(struct kinfo_proc)*maxknum, 1);
	ki = (KINFO *) calloc(sizeof(KINFO)*maxknum, 1);

	/* Read in a batch of entries at a time.  */
	while ((knum < maxknum) &&
	    (nbytes = getdents(procdirfd, direntbuff,
	     statbuf.st_blksize)) != 0) {
		int     i;
		struct dirent *dp;
		for (i = 0; i < nbytes; /* nothing */ ) {
			dp = (struct dirent *) & direntbuff[i];
			i += dp->d_reclen;
			if (strcmp(dp->d_name, ".") == 0)
				continue;
			if (strcmp(dp->d_name, "..") == 0)
				continue;
			if (strcmp(dp->d_name, "curproc") == 0)
				continue;
			ki[knum].ki_p = &kp[knum];
			if (parsekinfo(dp->d_name, &ki[knum]) != 0)
				continue;
			/*
			 * Now check some of the flags.  If the newest entry
			 * doesn't match the flag settings, then don't bump
			 * the pointer past it!
			 */
			switch (op) {
			case KERN_PROC_PID:
				if (kp[knum].kp_proc.p_pid == arg)
					knum++;
				break;
			case KERN_PROC_PGRP:
				if (kp[knum].kp_eproc.e_pgid == arg)
					knum++;
				break;
			case KERN_PROC_SESSION:
				if (kp[knum].kp_eproc.e_sid == arg)
					knum++;
				break;
			case KERN_PROC_TTY:
				if (kp[knum].kp_eproc.e_tdev == arg)
					knum++;
				break;
			case KERN_PROC_UID:
				if (kp[knum].kp_eproc.e_ucred.cr_uid == arg)
					knum++;
				break;
			case KERN_PROC_RUID:
				WARNX_ONLY_ONCE(("KERN_PROC_RUID flag "
					"not implemented.  Returning "
					"info for all processes."));
				knum++;
				break;
			case KERN_PROC_ALL:
				knum++;
				break;
			default:
				WARNX_ONLY_ONCE(("Bad switch case!  "
					"Returning info for "
					"all processes."));
				knum++;
				break;
			}
			if (knum > maxknum) {
				WARNX_ONLY_ONCE(("Warning:  only reporting "
					"information for first %d "
					"processes!", maxknum));
				break;
			}
		}
	}

	*cnt = knum;
	close(procdirfd);
	/* free unused memory */
	if (knum < maxknum) {
		kp = realloc(kp, sizeof(*kp) * knum);
		ki = realloc(ki, sizeof(*ki) * knum);
		for(; knum >= 0; knum--)
			ki[knum].ki_p = &kp[knum];
	}
	return ki;
}

/*
 * return process arguments, possibly ones used when exec()ing
 * the process; return the array as two element array, first is
 * argv[0], second element is all the other args separated by spaces
 */
char **
procfs_getargv(kp, nchr)
	const struct kinfo_proc *kp;
	int nchr;
{
	char    fullpath[MAXPATHLEN], *buf, *name, *args, **argv;
	int fd, num;
	ssize_t len;
	size_t idx;

	/* Open /proc/<pid>/cmdline, and parse it into the argv array */
	snprintf(fullpath, MAXPATHLEN, "/proc/%d/cmdline", kp->kp_proc.p_pid);
	fd = open(fullpath, O_RDONLY, 0);
	if (fd == -1 || verify_procfs_fd(fd, fullpath)) {
		/*
		 * Don't print warning, as the process may have died since our
		 * scan of the directory entries.
		 */
		return NULL;	/* Process may no longer exist. */
	}

	buf = (char *)malloc(nchr+1);
	len = read(fd, buf, nchr);
	close(fd);
	if (len == -1) {
		warnx("procfs_getargv");
		return NULL;
	}
	
	num = 1;
	args = NULL;
	name = buf;
	/* substitute any \0's with space */
	for(idx=0; idx < len; idx++) {
		if (buf[idx] == '\0') {
			if (!args)
				args = &buf[idx+1];
			else
				buf[idx] = ' ';
			num++;
		}
	}
	buf[len] = '\0'; /* end the string */

	/* if the name is the same as the p_comm, just enclosed
	 * in parentheses, remove the parentheses */
	if (num == 1 && name[0] == '(' && name[len-1] == ')'
		&& strncmp(name+1, kp->kp_proc.p_comm, len-2) == 0)
	{
		len -= 2;
		strncpy(name, name+1, len);
		name[len] = '\0';
	}
	
	argv = (char **) malloc(3*sizeof(char *));
	argv[0] = name;
	argv[1] = args;
	argv[2] = NULL;

	return argv;
}

