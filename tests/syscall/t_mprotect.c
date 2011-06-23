/* $NetBSD: t_mprotect.c,v 1.6.2.1 2011/06/23 14:20:41 cherry Exp $ */

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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_mprotect.c,v 1.6.2.1 2011/06/23 14:20:41 cherry Exp $");

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

static long	page = 0;
static int	pax_global = -1;
static int	pax_enabled = -1;
static char	path[] = "mmap";

static void	sighandler(int);
static bool	paxinit(void);
static bool	paxset(int, int);

static void
sighandler(int signo)
{
	_exit(signo);
}

static bool
paxinit(void)
{
	size_t len = sizeof(int);
	int rv;

	rv = sysctlbyname("security.pax.mprotect.global",
	    &pax_global, &len, NULL, 0);

	if (rv != 0)
		return false;

	rv = sysctlbyname("security.pax.mprotect.enabled",
	    &pax_enabled, &len, NULL, 0);

	if (rv != 0)
		return false;

	return paxset(1, 1);
}

static bool
paxset(int global, int enabled)
{
	size_t len = sizeof(int);
	int rv;

	rv = sysctlbyname("security.pax.mprotect.global",
	    NULL, NULL, &global, len);

	if (rv != 0)
		return false;

	rv = sysctlbyname("security.pax.mprotect.enabled",
	    NULL, NULL, &enabled, len);

	if (rv != 0)
		return false;

	return true;
}


ATF_TC_WITH_CLEANUP(mprotect_access);
ATF_TC_HEAD(mprotect_access, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test for EACCES from mprotect(2)");
}

ATF_TC_BODY(mprotect_access, tc)
{
	int prot[2] = { PROT_NONE, PROT_READ };
	void *map;
	size_t i;
	int fd;

	fd = open(path, O_RDONLY | O_CREAT);
	ATF_REQUIRE(fd >= 0);

	/*
	 * The call should fail with EACCES if we try to mark
	 * a PROT_NONE or PROT_READ file/section as PROT_WRITE.
	 */
	for (i = 0; i < __arraycount(prot); i++) {

		map = mmap(NULL, page, prot[i], MAP_SHARED, fd, 0);

		if (map == MAP_FAILED)
			continue;

		errno = 0;

		ATF_REQUIRE(mprotect(map, page, PROT_WRITE) != 0);
		ATF_REQUIRE(errno == EACCES);
		ATF_REQUIRE(munmap(map, page) == 0);
	}

	ATF_REQUIRE(close(fd) == 0);
}

ATF_TC_CLEANUP(mprotect_access, tc)
{
	(void)unlink(path);
}

ATF_TC(mprotect_err);
ATF_TC_HEAD(mprotect_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test error conditions of mprotect(2)");
}

ATF_TC_BODY(mprotect_err, tc)
{
	errno = 0;

	ATF_REQUIRE(mprotect((char *)-1, 1, PROT_READ) != 0);
	ATF_REQUIRE(errno == EINVAL);
}

ATF_TC(mprotect_pax);
ATF_TC_HEAD(mprotect_pax, tc)
{
	atf_tc_set_md_var(tc, "descr", "PaX restrictions and mprotect(2),");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(mprotect_pax, tc)
{
	const int prot[4] = { PROT_NONE, PROT_READ, PROT_WRITE };
	const char *str = NULL;
	void *map;
	size_t i;
	int rv;

	if (paxinit() != true)
		return;

	/*
	 * As noted in the original PaX documentation [1],
	 * the following restrictions should apply:
	 *
	 *   (1) creating executable anonymous mappings
	 *
	 *   (2) creating executable/writable file mappings
	 *
	 *   (3) making a non-executable mapping executable
	 *
	 *   (4) making an executable/read-only file mapping
	 *       writable except for performing relocations
	 *       on an ET_DYN ELF file (non-PIC shared library)
	 *
	 *  The following will test only the case (3).
	 *
	 * [1] http://pax.grsecurity.net/docs/mprotect.txt
	 *
	 *     (Sun Apr 3 11:06:53 EEST 2011.)
	 */
	for (i = 0; i < __arraycount(prot); i++) {

		map = mmap(NULL, page, prot[i], MAP_ANON, -1, 0);

		if (map == MAP_FAILED)
			continue;

		rv = mprotect(map, 1, prot[i] | PROT_EXEC);

		(void)munmap(map, page);

		if (rv == 0) {
			str = "non-executable mapping made executable";
			goto out;
		}
	}

out:
	if (pax_global != -1 && pax_enabled != -1)
		(void)paxset(pax_global, pax_enabled);

	if (str != NULL)
		atf_tc_fail("%s", str);
}

ATF_TC(mprotect_write);
ATF_TC_HEAD(mprotect_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mprotect(2) protections");
}

ATF_TC_BODY(mprotect_write, tc)
{
	pid_t pid;
	void *map;
	int sta;

	/*
	 * Map a page read/write, change the protection
	 * to read-only with mprotect(2), and try to write
	 * to the page. This should generate a SIGSEGV.
	 */
	map = mmap(NULL, page, PROT_WRITE|PROT_READ, MAP_ANON, -1, 0);
	ATF_REQUIRE(map != MAP_FAILED);

	ATF_REQUIRE(strlcpy(map, "XXX", 3) == 3);
	ATF_REQUIRE(mprotect(map, page, PROT_READ) == 0);

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		ATF_REQUIRE(signal(SIGSEGV, sighandler) != SIG_ERR);
		ATF_REQUIRE(strlcpy(map, "XXX", 3) == 0);
	}

	(void)wait(&sta);

	ATF_REQUIRE(WIFEXITED(sta) != 0);
	ATF_REQUIRE(WEXITSTATUS(sta) == SIGSEGV);
	ATF_REQUIRE(munmap(map, page) == 0);
}

ATF_TP_ADD_TCS(tp)
{
	page = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(page >= 0);

	ATF_TP_ADD_TC(tp, mprotect_access);
	ATF_TP_ADD_TC(tp, mprotect_err);
	ATF_TP_ADD_TC(tp, mprotect_pax);
	ATF_TP_ADD_TC(tp, mprotect_write);

	return atf_no_error();
}
