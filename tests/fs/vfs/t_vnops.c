/*	$NetBSD: t_vnops.c,v 1.7 2010/09/06 15:21:34 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/stat.h>
#include <sys/statvfs.h>

#include <atf-c.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rump.h>

#include "../common/h_fsmacros.h"
#include "../../h_macros.h"

#define USES_DIRS \
    if (FSTYPE_SYSVBFS(tc)) atf_tc_skip("dirs not supported by file system")

#define USES_SYMLINKS					\
    if (FSTYPE_SYSVBFS(tc) || FSTYPE_MSDOS(tc))		\
	atf_tc_skip("dirs not supported by file system")

static char *
md(char *buf, const char *base, const char *tail)
{

	sprintf(buf, "%s/%s", base, tail);
	return buf;
}

static void
lookup_simple(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN], final[MAXPATHLEN];
	struct stat sb1, sb2;

	strcpy(final, mountpath);
	sprintf(pb, "%s/../%s", mountpath, basename(final));
	if (rump_sys_stat(pb, &sb1) == -1)
		atf_tc_fail_errno("stat 1");

	sprintf(pb, "%s/./../%s", mountpath, basename(final));
	if (rump_sys_stat(pb, &sb2) == -1)
		atf_tc_fail_errno("stat 2");

	ATF_REQUIRE(memcmp(&sb1, &sb2, sizeof(sb1)) == 0);
}

static void
lookup_complex(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN];
	struct stat sb1, sb2;

	USES_DIRS;

	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");
	if (rump_sys_stat(pb, &sb1) == -1)
		atf_tc_fail_errno("stat 1");

	sprintf(pb, "%s/./dir/../././dir/.", mountpath);
	if (rump_sys_stat(pb, &sb2) == -1)
		atf_tc_fail_errno("stat 2");

	ATF_REQUIRE(memcmp(&sb1, &sb2, sizeof(sb1)) == 0);
}

static void
dir_simple(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN];
	struct stat sb;

	USES_DIRS;

	/* check we can create directories */
	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");
	if (rump_sys_stat(pb, &sb) == -1)
		atf_tc_fail_errno("stat new directory");

	/* check we can remove then and that it makes them unreachable */
	if (rump_sys_rmdir(pb) == -1)
		atf_tc_fail_errno("rmdir");
	if (rump_sys_stat(pb, &sb) != -1 || errno != ENOENT)
		atf_tc_fail("ENOENT expected from stat");
}

static void
dir_notempty(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN], pb2[MAXPATHLEN];
	int fd, rv;

	USES_DIRS;

	/* check we can create directories */
	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");

	sprintf(pb2, "%s/dir/file", mountpath);
	fd = rump_sys_open(pb2, O_RDWR | O_CREAT, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create file");
	rump_sys_close(fd);

	rv = rump_sys_rmdir(pb);
	if (rv != -1 || errno != ENOTEMPTY)
		atf_tc_fail("non-empty directory removed succesfully");

	if (rump_sys_unlink(pb2) == -1)
		atf_tc_fail_errno("cannot remove dir/file");

	if (rump_sys_rmdir(pb) == -1)
		atf_tc_fail_errno("remove directory");
}

static void
checkfile(const char *path, struct stat *refp)
{
	char buf[MAXPATHLEN];
	struct stat sb;
	static int n = 1;

	md(buf, path, "file");
	if (rump_sys_stat(buf, &sb) == -1)
		atf_tc_fail_errno("cannot stat file %d (%s)", n, buf);
	if (memcmp(&sb, refp, sizeof(sb)) != 0)
		atf_tc_fail("stat mismatch %d", n);
	n++;
}

static void
rename_dir(const atf_tc_t *tc, const char *mp)
{
	char pb1[MAXPATHLEN], pb2[MAXPATHLEN], pb3[MAXPATHLEN];
	struct stat ref;

	if (FSTYPE_MSDOS(tc))
		atf_tc_skip("test fails in some setups, reason unknown");

	USES_DIRS;

	md(pb1, mp, "dir1");
	if (rump_sys_mkdir(pb1, 0777) == -1)
		atf_tc_fail_errno("mkdir 1");

	md(pb2, mp, "dir2");
	if (rump_sys_mkdir(pb2, 0777) == -1)
		atf_tc_fail_errno("mkdir 2");
	md(pb2, mp, "dir2/subdir");
	if (rump_sys_mkdir(pb2, 0777) == -1)
		atf_tc_fail_errno("mkdir 3");

	md(pb3, mp, "dir1/file");
	if (rump_sys_mknod(pb3, S_IFREG | 0777, -1) == -1)
		atf_tc_fail_errno("create file");
	if (rump_sys_stat(pb3, &ref) == -1)
		atf_tc_fail_errno("stat of file");

	/*
	 * First try ops which should succeed.
	 */

	/* rename within directory */
	md(pb3, mp, "dir3");
	if (rump_sys_rename(pb1, pb3) == -1)
		atf_tc_fail_errno("rename 1");
	checkfile(pb3, &ref);

	/* rename directory onto itself (two ways, should fail) */
	md(pb1, mp, "dir3/.");
	if (rump_sys_rename(pb1, pb3) != -1 || errno != EINVAL)
		atf_tc_fail_errno("rename 2");
	if (rump_sys_rename(pb3, pb1) != -1 || errno != EISDIR)
		atf_tc_fail_errno("rename 3");

	checkfile(pb3, &ref);

	/* rename father of directory into directory */
	md(pb1, mp, "dir2/dir");
	md(pb2, mp, "dir2");
	if (rump_sys_rename(pb2, pb1) != -1 || errno != EINVAL)
		atf_tc_fail_errno("rename 4");

	/* same for grandfather */
	md(pb1, mp, "dir2/subdir/dir2");
	if (rump_sys_rename(pb2, pb1) != -1 || errno != EINVAL)
		atf_tc_fail("rename 5");

	checkfile(pb3, &ref);

	/* rename directory over a non-empty directory */
	if (rump_sys_rename(pb2, pb3) != -1 || errno != ENOTEMPTY)
		atf_tc_fail("rename 6");

	/* cross-directory rename */
	md(pb1, mp, "dir3");
	md(pb2, mp, "dir2/somedir");
	if (rump_sys_rename(pb1, pb2) == -1)
		atf_tc_fail_errno("rename 7");
	checkfile(pb2, &ref);

	/* move to parent directory */
	md(pb1, mp, "dir2/somedir/../../dir3");
	if (rump_sys_rename(pb2, pb1) == -1)
		atf_tc_fail_errno("rename 8");
	md(pb1, mp, "dir2/../dir3");
	checkfile(pb1, &ref);

	/* finally, atomic cross-directory rename */
	md(pb3, mp, "dir2/subdir");
	if (rump_sys_rename(pb1, pb3) == -1)
		atf_tc_fail_errno("rename 9");
	checkfile(pb3, &ref);
}

static void
rename_dotdot(const atf_tc_t *tc, const char *mp)
{

	USES_DIRS;

	if (rump_sys_chdir(mp) == -1)
		atf_tc_fail_errno("chdir mountpoint");

	if (rump_sys_mkdir("dir1", 0777) == -1)
		atf_tc_fail_errno("mkdir 1");
	if (rump_sys_mkdir("dir2", 0777) == -1)
		atf_tc_fail_errno("mkdir 2");

	if (rump_sys_rename("dir1", "dir1/..") != -1 || errno != EINVAL)
		atf_tc_fail_errno("self-dotdot to");

	if (rump_sys_rename("dir1/..", "sometarget") != -1 || errno != EINVAL)
		atf_tc_fail_errno("self-dotdot from");
	atf_tc_expect_pass();

	if (FSTYPE_TMPFS(tc)) {
		atf_tc_expect_fail("PR kern/43617");
	}
	if (rump_sys_rename("dir1", "dir2/..") != -1 || errno != EINVAL)
		atf_tc_fail("other-dotdot");

	rump_sys_chdir("/");
}

static void
rename_reg_nodir(const atf_tc_t *tc, const char *mp)
{
	bool haslinks;
	struct stat sb;
	ino_t f1ino, f2ino;

	if (FSTYPE_MSDOS(tc))
		atf_tc_skip("test fails in some setups, reason unknown");

	if (rump_sys_chdir(mp) == -1)
		atf_tc_fail_errno("chdir mountpoint");

	if (FSTYPE_MSDOS(tc) || FSTYPE_SYSVBFS(tc))
		haslinks = false;
	else
		haslinks = true;

	if (rump_sys_mknod("file1", S_IFREG | 0777, -1) == -1)
		atf_tc_fail_errno("create file");
	if (rump_sys_mknod("file2", S_IFREG | 0777, -1) == -1)
		atf_tc_fail_errno("create file");

	if (rump_sys_stat("file1", &sb) == -1)
		atf_tc_fail_errno("stat");
	f1ino = sb.st_ino;

	if (haslinks) {
		if (rump_sys_link("file1", "file_link") == -1)
			atf_tc_fail_errno("link");
		if (rump_sys_stat("file_link", &sb) == -1)
			atf_tc_fail_errno("stat");
		ATF_REQUIRE_EQ(sb.st_ino, f1ino);
		ATF_REQUIRE_EQ(sb.st_nlink, 2);
	}

	if (rump_sys_stat("file2", &sb) == -1)
		atf_tc_fail_errno("stat");
	f2ino = sb.st_ino;

	if (rump_sys_rename("file1", "file3") == -1)
		atf_tc_fail_errno("rename 1");
	if (rump_sys_stat("file3", &sb) == -1)
		atf_tc_fail_errno("stat 1");
	if (haslinks) {
		ATF_REQUIRE_EQ(sb.st_ino, f1ino);
	}
	if (rump_sys_stat("file1", &sb) != -1 || errno != ENOENT)
		atf_tc_fail_errno("source 1");

	if (rump_sys_rename("file3", "file2") == -1)
		atf_tc_fail_errno("rename 2");
	if (rump_sys_stat("file2", &sb) == -1)
		atf_tc_fail_errno("stat 2");
	if (haslinks) {
		ATF_REQUIRE_EQ(sb.st_ino, f1ino);
	}

	if (rump_sys_stat("file3", &sb) != -1 || errno != ENOENT)
		atf_tc_fail_errno("source 2");

	if (haslinks) {
		if (rump_sys_rename("file2", "file_link") == -1)
			atf_tc_fail_errno("rename hardlink");
		if (rump_sys_stat("file2", &sb) != -1 || errno != ENOENT)
			atf_tc_fail_errno("source 3");
		if (rump_sys_stat("file_link", &sb) == -1)
			atf_tc_fail_errno("stat 2");
		ATF_REQUIRE_EQ(sb.st_ino, f1ino);
		ATF_REQUIRE_EQ(sb.st_nlink, 1);
	}

	rump_sys_chdir("/");
}

static void
create_nametoolong(const atf_tc_t *tc, const char *mp)
{
	char *name;
	int fd;
	long val;
	size_t len;

	if (rump_sys_chdir(mp) == -1)
		atf_tc_fail_errno("chdir mountpoint");

	val = rump_sys_pathconf(".", _PC_NAME_MAX);
	if (val == -1)
		atf_tc_fail_errno("pathconf");

	len = val + 1;
	name = malloc(len+1);
	if (name == NULL)
		atf_tc_fail_errno("malloc");

	memset(name, 'a', len);
	*(name+len) = '\0';

	val = rump_sys_pathconf(".", _PC_NO_TRUNC);
	if (val == -1)
		atf_tc_fail_errno("pathconf");

	if (FSTYPE_MSDOS(tc))
		atf_tc_expect_fail("PR kern/43670");
	fd = rump_sys_open(name, O_RDWR|O_CREAT, 0666);
	if (val != 0 && (fd != -1 || errno != ENAMETOOLONG))
		atf_tc_fail_errno("open");

	if (val == 0 && rump_sys_close(fd) == -1)
		atf_tc_fail_errno("close");
	if (val == 0 && rump_sys_unlink(name) == -1)
		atf_tc_fail_errno("unlink");

	free(name);

	rump_sys_chdir("/");
}

static void
rename_nametoolong(const atf_tc_t *tc, const char *mp)
{
	char *name;
	int res, fd;
	long val;
	size_t len;

	if (rump_sys_chdir(mp) == -1)
		atf_tc_fail_errno("chdir mountpoint");

	val = rump_sys_pathconf(".", _PC_NAME_MAX);
	if (val == -1)
		atf_tc_fail_errno("pathconf");

	len = val + 1;
	name = malloc(len+1);
	if (name == NULL)
		atf_tc_fail_errno("malloc");

	memset(name, 'a', len);
	*(name+len) = '\0';

	fd = rump_sys_open("dummy", O_RDWR|O_CREAT, 0666);
	if (fd == -1)
		atf_tc_fail_errno("open");
	if (rump_sys_close(fd) == -1)
		atf_tc_fail_errno("close");

	val = rump_sys_pathconf(".", _PC_NO_TRUNC);
	if (val == -1)
		atf_tc_fail_errno("pathconf");

	if (FSTYPE_MSDOS(tc))
		atf_tc_expect_fail("PR kern/43670");
	res = rump_sys_rename("dummy", name);
	if (val != 0 && (res != -1 || errno != ENAMETOOLONG))
		atf_tc_fail_errno("rename");

	if (val == 0 && rump_sys_unlink(name) == -1)
		atf_tc_fail_errno("unlink");

	free(name);

	rump_sys_chdir("/");
}

static void
symlink_zerolen(const atf_tc_t *tc, const char *mp)
{

	USES_SYMLINKS;

	RL(rump_sys_chdir(mp));
	RL(rump_sys_symlink("", "afile"));
	RL(rump_sys_chdir("/"));
}

ATF_TC_FSAPPLY(lookup_simple, "simple lookup (./.. on root)");
ATF_TC_FSAPPLY(lookup_complex, "lookup of non-dot entries");
ATF_TC_FSAPPLY(dir_simple, "mkdir/rmdir");
ATF_TC_FSAPPLY(dir_notempty, "non-empty directories cannot be removed");
ATF_TC_FSAPPLY(rename_dir, "exercise various directory renaming ops");
ATF_TC_FSAPPLY(rename_dotdot, "rename dir ..");
ATF_TC_FSAPPLY(rename_reg_nodir, "rename regular files, no subdirectories");
ATF_TC_FSAPPLY(create_nametoolong, "create file with name too long");
ATF_TC_FSAPPLY(rename_nametoolong, "rename to file with name too long");
ATF_TC_FSAPPLY(symlink_zerolen, "symlink with 0-len target");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(lookup_simple);
	ATF_TP_FSAPPLY(lookup_complex);
	ATF_TP_FSAPPLY(dir_simple);
	ATF_TP_FSAPPLY(dir_notempty);
	ATF_TP_FSAPPLY(rename_dir);
	ATF_TP_FSAPPLY(rename_dotdot);
	ATF_TP_FSAPPLY(rename_reg_nodir);
	ATF_TP_FSAPPLY(create_nametoolong);
	ATF_TP_FSAPPLY(rename_nametoolong);
	ATF_TP_FSAPPLY(symlink_zerolen);

	return atf_no_error();
}
