/* $NetBSD: t_mmap.c,v 1.18 2022/06/04 23:09:18 riastradh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_mmap.c,v 1.18 2022/06/04 23:09:18 riastradh Exp $");

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <pthread.h>

static long	page = 0;
static char	path[] = "mmap";
static void	map_check(void *, int);
static void	map_sighandler(int);
static void	testloan(void *, void *, char, int);

#define	BUFSIZE	(32 * 1024)	/* enough size to trigger sosend_loan */

static void
map_check(void *map, int flag)
{

	if (flag != 0) {
		ATF_REQUIRE(map == MAP_FAILED);
		return;
	}

	ATF_REQUIRE(map != MAP_FAILED);
	ATF_REQUIRE(munmap(map, page) == 0);
}

void
testloan(void *vp, void *vp2, char pat, int docheck)
{
	char buf[BUFSIZE];
	char backup[BUFSIZE];
	ssize_t nwritten;
	ssize_t nread;
	int fds[2];
	int val;

	val = BUFSIZE;

	if (docheck != 0)
		(void)memcpy(backup, vp, BUFSIZE);

	if (socketpair(AF_LOCAL, SOCK_STREAM, PF_UNSPEC, fds) != 0)
		atf_tc_fail("socketpair() failed");

	val = BUFSIZE;

	if (setsockopt(fds[1], SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) != 0)
		atf_tc_fail("setsockopt() failed, SO_RCVBUF");

	val = BUFSIZE;

	if (setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)) != 0)
		atf_tc_fail("setsockopt() failed, SO_SNDBUF");

	if (fcntl(fds[0], F_SETFL, O_NONBLOCK) != 0)
		atf_tc_fail("fcntl() failed");

	nwritten = write(fds[0], (char *)vp + page, BUFSIZE - page);

	if (nwritten == -1)
		atf_tc_fail("write() failed");

	/* Break loan. */
	(void)memset(vp2, pat, BUFSIZE);

	nread = read(fds[1], buf + page, BUFSIZE - page);

	if (nread == -1)
		atf_tc_fail("read() failed");

	if (nread != nwritten)
		atf_tc_fail("too short read");

	if (docheck != 0 && memcmp(backup, buf + page, nread) != 0)
		atf_tc_fail("data mismatch");

	ATF_REQUIRE(close(fds[0]) == 0);
	ATF_REQUIRE(close(fds[1]) == 0);
}

static void
map_sighandler(int signo)
{
	_exit(signo);
}

ATF_TC(mmap_block);
ATF_TC_HEAD(mmap_block, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) with a block device");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(mmap_block, tc)
{
	static const int mib[] = { CTL_HW, HW_DISKNAMES };
	static const unsigned int miblen = __arraycount(mib);
	char *map, *dk, *drives, dev[PATH_MAX];
	size_t len;
	int fd = -1;

	atf_tc_skip("The test case causes a panic " \
	    "(PR kern/38889, PR kern/46592)");

	ATF_REQUIRE(sysctl(mib, miblen, NULL, &len, NULL, 0) == 0);
	drives = malloc(len);
	ATF_REQUIRE(drives != NULL);
	ATF_REQUIRE(sysctl(mib, miblen, drives, &len, NULL, 0) == 0);
	for (dk = strtok(drives, " "); dk != NULL; dk = strtok(NULL, " ")) {
		if (strncmp(dk, "dk", 2) == 0)
			snprintf(dev, sizeof(dev), _PATH_DEV "%s", dk);
		else
			snprintf(dev, sizeof(dev), _PATH_DEV "%s%c", dk,
			    'a' + RAW_PART);
		fprintf(stderr, "trying: %s\n", dev);

		if ((fd = open(dev, O_RDONLY)) >= 0) {
			(void)fprintf(stderr, "using %s\n", dev);
			break;
		} else
			(void)fprintf(stderr, "%s: %s\n", dev, strerror(errno));
	}
	free(drives);

	if (fd < 0)
		atf_tc_skip("failed to find suitable block device");

	map = mmap(NULL, 4096, PROT_READ, MAP_FILE, fd, 0);
	ATF_REQUIRE_MSG(map != MAP_FAILED, "mmap: %s", strerror(errno));

	(void)fprintf(stderr, "first byte %x\n", *map);
	ATF_REQUIRE(close(fd) == 0);
	(void)fprintf(stderr, "first byte %x\n", *map);

	ATF_REQUIRE(munmap(map, 4096) == 0);
}

ATF_TC(mmap_err);
ATF_TC_HEAD(mmap_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test error conditions of mmap(2)");
}

ATF_TC_BODY(mmap_err, tc)
{
	void *addr = (void *)-1;
	void *map;

	errno = 0;
	map = mmap(NULL, 3, PROT_READ, MAP_FILE|MAP_PRIVATE, -1, 0);

	ATF_REQUIRE(map == MAP_FAILED);
	ATF_REQUIRE(errno == EBADF);

	errno = 0;
	map = mmap(addr, page, PROT_READ, MAP_FIXED|MAP_PRIVATE, -1, 0);

	ATF_REQUIRE(map == MAP_FAILED);
	ATF_REQUIRE_MSG(errno == EINVAL, "errno %d != EINVAL", errno);

	errno = 0;
	map = mmap(NULL, page, PROT_READ, MAP_ANON|MAP_PRIVATE, INT_MAX, 0);

	ATF_REQUIRE(map == MAP_FAILED);
	ATF_REQUIRE(errno == EINVAL);
}

ATF_TC_WITH_CLEANUP(mmap_loan);
ATF_TC_HEAD(mmap_loan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test uvm page loanout with mmap(2)");
}

ATF_TC_BODY(mmap_loan, tc)
{
	char buf[BUFSIZE];
	char *vp, *vp2;
	int fd;

	fd = open(path, O_RDWR | O_CREAT, 0600);
	ATF_REQUIRE(fd >= 0);

	(void)memset(buf, 'x', sizeof(buf));
	(void)write(fd, buf, sizeof(buf));

	vp = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_PRIVATE, fd, 0);

	ATF_REQUIRE(vp != MAP_FAILED);

	vp2 = vp;

	testloan(vp, vp2, 'A', 0);
	testloan(vp, vp2, 'B', 1);

	ATF_REQUIRE(munmap(vp, BUFSIZE) == 0);

	vp = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_SHARED, fd, 0);

	vp2 = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_SHARED, fd, 0);

	ATF_REQUIRE(vp != MAP_FAILED);
	ATF_REQUIRE(vp2 != MAP_FAILED);

	testloan(vp, vp2, 'E', 1);

	ATF_REQUIRE(munmap(vp, BUFSIZE) == 0);
	ATF_REQUIRE(munmap(vp2, BUFSIZE) == 0);
}

ATF_TC_CLEANUP(mmap_loan, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mmap_prot_1);
ATF_TC_HEAD(mmap_prot_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) protections, #1");
}

ATF_TC_BODY(mmap_prot_1, tc)
{
	void *map;
	int fd;

	/*
	 * Open a file write-only and try to
	 * map it read-only. This should fail.
	 */
	fd = open(path, O_WRONLY | O_CREAT, 0700);

	if (fd < 0)
		return;

	ATF_REQUIRE(write(fd, "XXX", 3) == 3);

	map = mmap(NULL, 3, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
	map_check(map, 1);

	map = mmap(NULL, 3, PROT_WRITE, MAP_FILE|MAP_PRIVATE, fd, 0);
	map_check(map, 0);

	ATF_REQUIRE(close(fd) == 0);
}

ATF_TC_CLEANUP(mmap_prot_1, tc)
{
	(void)unlink(path);
}

ATF_TC(mmap_prot_2);
ATF_TC_HEAD(mmap_prot_2, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) protections, #2");
}

ATF_TC_BODY(mmap_prot_2, tc)
{
	char buf[2];
	void *map;
	pid_t pid;
	int sta;

	/*
	 * Make a PROT_NONE mapping and try to access it.
	 * If we catch a SIGSEGV, all works as expected.
	 */
	map = mmap(NULL, page, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
	ATF_REQUIRE(map != MAP_FAILED);

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		ATF_REQUIRE(signal(SIGSEGV, map_sighandler) != SIG_ERR);
		ATF_REQUIRE(strlcpy(buf, map, sizeof(buf)) != 0);
	}

	(void)wait(&sta);

	ATF_REQUIRE(WIFEXITED(sta) != 0);
	ATF_REQUIRE(WEXITSTATUS(sta) == SIGSEGV);
	ATF_REQUIRE(munmap(map, page) == 0);
}

ATF_TC_WITH_CLEANUP(mmap_prot_3);
ATF_TC_HEAD(mmap_prot_3, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) protections, #3");
}

ATF_TC_BODY(mmap_prot_3, tc)
{
	char buf[2];
	int fd, sta;
	void *map;
	pid_t pid;

	/*
	 * Open a file, change the permissions
	 * to read-only, and try to map it as
	 * PROT_NONE. This should succeed, but
	 * the access should generate SIGSEGV.
	 */
	fd = open(path, O_RDWR | O_CREAT, 0700);

	if (fd < 0)
		return;

	ATF_REQUIRE(write(fd, "XXX", 3) == 3);
	ATF_REQUIRE(close(fd) == 0);
	ATF_REQUIRE(chmod(path, 0444) == 0);

	fd = open(path, O_RDONLY);
	ATF_REQUIRE(fd != -1);

	map = mmap(NULL, 3, PROT_NONE, MAP_FILE | MAP_SHARED, fd, 0);
	ATF_REQUIRE(map != MAP_FAILED);

	pid = fork();

	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		ATF_REQUIRE(signal(SIGSEGV, map_sighandler) != SIG_ERR);
		ATF_REQUIRE(strlcpy(buf, map, sizeof(buf)) != 0);
	}

	(void)wait(&sta);

	ATF_REQUIRE(WIFEXITED(sta) != 0);
	ATF_REQUIRE(WEXITSTATUS(sta) == SIGSEGV);
	ATF_REQUIRE(munmap(map, 3) == 0);
}

ATF_TC_CLEANUP(mmap_prot_3, tc)
{
	(void)unlink(path);
}

ATF_TC(mmap_reprotect_race);

ATF_TC_HEAD(mmap_reprotect_race, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test for the race condition of PR 52239");
}

const int mmap_reprotect_race_npages = 13;
const int mmap_reprotect_iterations = 1000000;

static void *
mmap_reprotect_race_thread(void *arg)
{
	int i, r;
	void *p;

	for (i = 0; i < mmap_reprotect_iterations; i++) {
		/* Get some unrelated memory */
		p = mmap(0, mmap_reprotect_race_npages * page,
			 PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
		ATF_REQUIRE(p);
		r = munmap(p, mmap_reprotect_race_npages * page);
		ATF_REQUIRE(r == 0);
	}
	return 0;
}

ATF_TC_BODY(mmap_reprotect_race, tc)
{
	pthread_t thread;
	void *p, *q;
	int i, r;

	r = pthread_create(&thread, 0, mmap_reprotect_race_thread, 0);
	ATF_REQUIRE(r == 0);

	for (i = 0; i < mmap_reprotect_iterations; i++) {
		/* Get a placeholder region */
		p = mmap(0, mmap_reprotect_race_npages * page,
			 PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
		if (p == MAP_FAILED)
			atf_tc_fail("mmap: %s", strerror(errno));

		/* Upgrade placeholder to read/write access */
		q = mmap(p, mmap_reprotect_race_npages * page,
			 PROT_READ|PROT_WRITE,
			 MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0);
		if (q == MAP_FAILED)
			atf_tc_fail("update mmap: %s", strerror(errno));
		ATF_REQUIRE(q == p);

		/* Free it */
		r = munmap(q, mmap_reprotect_race_npages * page);
		if (r != 0)
			atf_tc_fail("munmap: %s", strerror(errno));
	}
	pthread_join(thread, NULL);
}

ATF_TC_WITH_CLEANUP(mmap_truncate);
ATF_TC_HEAD(mmap_truncate, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) and ftruncate(2)");
}

ATF_TC_BODY(mmap_truncate, tc)
{
	char *map;
	long i;
	int fd;

	fd = open(path, O_RDWR | O_CREAT, 0700);

	if (fd < 0)
		return;

	/*
	 * See that ftruncate(2) works
	 * while the file is mapped.
	 */
	ATF_REQUIRE(ftruncate(fd, page) == 0);

	map = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_FILE|MAP_PRIVATE,
	     fd, 0);
	ATF_REQUIRE(map != MAP_FAILED);

	for (i = 0; i < page; i++)
		map[i] = 'x';

	ATF_REQUIRE(ftruncate(fd, 0) == 0);
	ATF_REQUIRE(ftruncate(fd, page / 8) == 0);
	ATF_REQUIRE(ftruncate(fd, page / 4) == 0);
	ATF_REQUIRE(ftruncate(fd, page / 2) == 0);
	ATF_REQUIRE(ftruncate(fd, page / 12) == 0);
	ATF_REQUIRE(ftruncate(fd, page / 64) == 0);

	(void)munmap(map, page);
	ATF_REQUIRE(close(fd) == 0);
}

ATF_TC_CLEANUP(mmap_truncate, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mmap_truncate_signal);
ATF_TC_HEAD(mmap_truncate_signal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test mmap(2) ftruncate(2) causing signal");
}

ATF_TC_BODY(mmap_truncate_signal, tc)
{
	char *map;
	long i;
	int fd, sta;
	pid_t pid;

	fd = open(path, O_RDWR | O_CREAT, 0700);

	if (fd < 0)
		return;

	ATF_REQUIRE(write(fd, "foo\n", 5) == 5);

	map = mmap(NULL, page, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
	ATF_REQUIRE(map != MAP_FAILED);

	sta = 0;
	for (i = 0; i < 5; i++)
		sta += map[i];
	ATF_REQUIRE(sta == 334);

	ATF_REQUIRE(ftruncate(fd, 0) == 0);
	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		ATF_REQUIRE(signal(SIGBUS, map_sighandler) != SIG_ERR);
		ATF_REQUIRE(signal(SIGSEGV, map_sighandler) != SIG_ERR);
		sta = 0;
		for (i = 0; i < page; i++)
			sta += map[i];
		/* child never will get this far, but the compiler will
		   not know, so better use the values calculated to
		   prevent the access to be optimized out */
		ATF_REQUIRE(i == 0);
		ATF_REQUIRE(sta == 0);
		(void)munmap(map, page);
		(void)close(fd);
		return;
	}

	(void)wait(&sta);

	ATF_REQUIRE(WIFEXITED(sta) != 0);
	if (WEXITSTATUS(sta) == SIGSEGV)
		atf_tc_fail("child process got SIGSEGV instead of SIGBUS");
	ATF_REQUIRE(WEXITSTATUS(sta) == SIGBUS);
	ATF_REQUIRE(munmap(map, page) == 0);
	ATF_REQUIRE(close(fd) == 0);
}

ATF_TC_CLEANUP(mmap_truncate_signal, tc)
{
	(void)unlink(path);
}

ATF_TC(mmap_va0);
ATF_TC_HEAD(mmap_va0, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap(2) and vm.user_va0_disable");
}

ATF_TC_BODY(mmap_va0, tc)
{
	int flags = MAP_ANON | MAP_FIXED | MAP_PRIVATE;
	size_t len = sizeof(int);
	void *map;
	int val;

	/*
	 * Make an anonymous fixed mapping at zero address. If the address
	 * is restricted as noted in security(7), the syscall should fail.
	 */
	if (sysctlbyname("vm.user_va0_disable", &val, &len, NULL, 0) != 0)
		atf_tc_fail("failed to read vm.user_va0_disable");

	map = mmap(NULL, page, PROT_EXEC, flags, -1, 0);
	map_check(map, val);

	map = mmap(NULL, page, PROT_READ, flags, -1, 0);
	map_check(map, val);

	map = mmap(NULL, page, PROT_WRITE, flags, -1, 0);
	map_check(map, val);

	map = mmap(NULL, page, PROT_READ|PROT_WRITE, flags, -1, 0);
	map_check(map, val);

	map = mmap(NULL, page, PROT_EXEC|PROT_READ|PROT_WRITE, flags, -1, 0);
	map_check(map, val);
}

static void
test_mmap_hint(uintptr_t hintaddr)
{
	void *hint = (void *)hintaddr;
	void *map1 = MAP_FAILED, *map2 = MAP_FAILED, *map3 = MAP_FAILED;

	map1 = mmap(hint, page, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	if (map1 == MAP_FAILED) {
		atf_tc_fail_nonfatal("mmap1 hint=%p: errno=%d", hint, errno);
		goto out;
	}
	map2 = mmap(map1, page, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	if (map2 == MAP_FAILED) {
		atf_tc_fail_nonfatal("mmap2 hint=%p map1=%p failed: errno=%d",
		    hint, map1, errno);
		goto out;
	}
	map3 = mmap(hint, page, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	if (map3 == MAP_FAILED) {
		atf_tc_fail_nonfatal("mmap3 hint=%p map1=%p failed: errno=%d",
		    hint, map1, errno);
		goto out;
	}
out:
	if (map3 != MAP_FAILED) {
		ATF_CHECK_MSG(munmap(map3, page) == 0, "munmap3 %p hint=%p",
		    map3, hint);
	}
	if (map2 != MAP_FAILED) {
		ATF_CHECK_MSG(munmap(map2, page) == 0, "munmap2 %p hint=%p",
		    map2, hint);
	}
	if (map1 != MAP_FAILED) {
		ATF_CHECK_MSG(munmap(map1, page) == 0, "munmap1 %p hint=%p",
		    map1, hint);
	}
}

ATF_TC(mmap_hint);
ATF_TC_HEAD(mmap_hint, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mmap with hints");
}
ATF_TC_BODY(mmap_hint, tc)
{
	static const int minaddress_mib[] = { CTL_VM, VM_MINADDRESS };
	static const int maxaddress_mib[] = { CTL_VM, VM_MAXADDRESS };
	long minaddress, maxaddress;
	size_t minaddresssz = sizeof(minaddress);
	size_t maxaddresssz = sizeof(maxaddress);

	ATF_REQUIRE_MSG(sysctl(minaddress_mib, __arraycount(minaddress_mib),
		&minaddress, &minaddresssz, NULL, 0) == 0,
	    "sysctl vm.minaddress: errno=%d", errno);
	ATF_REQUIRE_MSG(sysctl(maxaddress_mib, __arraycount(maxaddress_mib),
		&maxaddress, &maxaddresssz, NULL, 0) == 0,
	    "sysctl vm.maxaddress: errno=%d", errno);

	test_mmap_hint(0);
	test_mmap_hint(-1);
	test_mmap_hint(page);
	test_mmap_hint(minaddress);
	test_mmap_hint(maxaddress);
}

ATF_TP_ADD_TCS(tp)
{
	page = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(page >= 0);

	ATF_TP_ADD_TC(tp, mmap_block);
	ATF_TP_ADD_TC(tp, mmap_err);
	ATF_TP_ADD_TC(tp, mmap_loan);
	ATF_TP_ADD_TC(tp, mmap_prot_1);
	ATF_TP_ADD_TC(tp, mmap_prot_2);
	ATF_TP_ADD_TC(tp, mmap_prot_3);
	ATF_TP_ADD_TC(tp, mmap_reprotect_race);
	ATF_TP_ADD_TC(tp, mmap_truncate);
	ATF_TP_ADD_TC(tp, mmap_truncate_signal);
	ATF_TP_ADD_TC(tp, mmap_va0);
	ATF_TP_ADD_TC(tp, mmap_hint);

	return atf_no_error();
}
