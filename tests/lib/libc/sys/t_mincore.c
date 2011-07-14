/* $NetBSD: t_mincore.c,v 1.2 2011/07/14 08:09:48 jruoho Exp $ */

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
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__RCSID("$NetBSD: t_mincore.c,v 1.2 2011/07/14 08:09:48 jruoho Exp $");

#include <sys/mman.h>
#include <sys/shm.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static long		page = 0;
static const char	path[] = "mincore";
static size_t		check_residency(void *, size_t);

static size_t
check_residency(void *addr, size_t npgs)
{
	size_t i, resident;
	char *vec;

	vec = malloc(npgs);

	ATF_REQUIRE(vec != NULL);
	ATF_REQUIRE(mincore(addr, npgs * page, vec) == 0);

	for (i = resident = 0; i < npgs; i++) {

		if (vec[i] != 0)
			resident++;

		(void)fprintf(stderr, "page 0x%p is %sresident\n",
		    (char *)addr + (i * page), vec[i] ? "" : "not ");
	}

	free(vec);

	return resident;
}

ATF_TC(mincore_err);
ATF_TC_HEAD(mincore_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from mincore(2)");
}

ATF_TC_BODY(mincore_err, tc)
{
	char *map, *vec;

	map = mmap(NULL, page, PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	vec = malloc(page);

	ATF_REQUIRE(vec != NULL);
	ATF_REQUIRE(map != MAP_FAILED);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, mincore(map, 0, vec) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(ENOMEM, mincore(0, page, vec) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, mincore(map, page, (void *)-1) == -1);

	free(vec);
	ATF_REQUIRE(munmap(map, page) == 0);
}

ATF_TC_WITH_CLEANUP(mincore_incore);
ATF_TC_HEAD(mincore_incore, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that mincore(2) works");
}

ATF_TC_BODY(mincore_incore, tc)
{
	char *buf, *vec, *map = MAP_FAILED;
	const char *str = NULL;
	const size_t n = 3;
	ssize_t tot;
	int fd, rv;
	size_t i, j;

	/*
	 * Create a temporary file, write
	 * few pages to it, and map the file.
	 */
	buf = calloc(n, page);
	vec = calloc(n, page);

	if (buf == NULL || vec == NULL)
		return;

	for (i = 0; i < (size_t)page * n; i++)
		buf[i] = 'x';

	fd = open(path, O_RDWR | O_CREAT, 0600);

	if (fd < 0) {
		str = "failed to open";
		goto out;
	}

	tot = 0;

	while (tot < page * (long)n) {

		rv = write(fd, buf, sizeof(buf));

		if (rv < 0) {
			str = "failed to write";
			goto out;
		}

		tot += rv;
	}

	map = mmap(NULL, page * n, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_PRIVATE, fd, 0);

	if (map == MAP_FAILED) {
		str = "failed to map";
		goto out;
	}

	/*
	 * Lock the mapping such that only
	 * in-core page status is returned.
	 */
	if (mlock(map, page * n) != 0) {
		str = "failed to lock";
		goto out;
	}

	if (mincore(map, page * n, vec) != 0) {
		str = "mincore failed";
		goto out;
	}

	/*
	 * Check that the in-core pages
	 * match the locked pages.
	 */
	for (i = j = 0; i < (size_t)page * n; i++) {

		if (vec[i] != 0)
			j++;
	}

	if (j != n)
		str = "mismatch of in-core pages";

out:
	free(buf);
	free(vec);

	(void)close(fd);
	(void)unlink(path);

	if (map != MAP_FAILED) {
		(void)munlock(map, page);
		(void)munmap(map, page);
	}

	if (str != NULL)
		atf_tc_fail("%s", str);
}

ATF_TC_CLEANUP(mincore_incore, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mincore_residency);
ATF_TC_HEAD(mincore_residency, tc)
{
	atf_tc_set_md_var(tc, "descr", "mmap(2) residency with mincore(2)");
}

ATF_TC_BODY(mincore_residency, tc)
{
	void *addr, *addr2, *addr3, *buf;
	size_t npgs = 0;
	struct stat st;
	ssize_t tot;
	int fd, rv;

	(void)memset(&st, 0, sizeof(struct stat));

	fd = open(path, O_RDWR | O_CREAT, 0700);
	buf = malloc(page * 5);

	ATF_REQUIRE(fd >= 0);
	ATF_REQUIRE(buf != NULL);

	tot = 0;

	while (tot < page) {

		rv = write(fd, buf, sizeof(buf));
		ATF_REQUIRE(rv >= 0);

		tot += rv;
	}

	ATF_REQUIRE(fd >= 0);
	ATF_REQUIRE(fstat(fd, &st) == 0);

	addr = mmap(NULL, (size_t)st.st_size, PROT_READ,
	    MAP_FILE | MAP_SHARED, fd, (off_t) 0);

	ATF_REQUIRE(addr != MAP_FAILED);

	(void)close(fd);

	npgs = st.st_size / page;

	if (st.st_size % page != 0)
		npgs++;

	(void)check_residency(addr, npgs);

	ATF_REQUIRE(mlock(addr, npgs * page) == 0);
	ATF_REQUIRE(munmap(addr, st.st_size) == 0);

	npgs = 128;

	addr = mmap(NULL, npgs * page, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE | MAP_WIRED, -1, (off_t)0);

	if (addr == MAP_FAILED)
		return;

	ATF_REQUIRE(check_residency(addr, npgs) == npgs);
	ATF_REQUIRE(munmap(addr, npgs * page) == 0);

	npgs = 128;

	addr = mmap(NULL, npgs * page, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE, -1, (off_t)0);

	if (addr == MAP_FAILED)
		return;

	ATF_REQUIRE(check_residency(addr, npgs) == 0);
	ATF_REQUIRE(mlockall(MCL_CURRENT|MCL_FUTURE) == 0);
	ATF_REQUIRE(check_residency(addr, npgs) == npgs);

	addr2 = mmap(NULL, npgs * page, PROT_READ, MAP_ANON, -1, (off_t)0);
	addr3 = mmap(NULL, npgs * page, PROT_NONE, MAP_ANON, -1, (off_t)0);

	if (addr2 == MAP_FAILED || addr3 == MAP_FAILED)
		return;

	ATF_REQUIRE(check_residency(addr2, npgs) == npgs);
	ATF_REQUIRE(check_residency(addr3, npgs) == 0);
	ATF_REQUIRE(mprotect(addr3, npgs * page, PROT_READ) == 0);
	ATF_REQUIRE(check_residency(addr, npgs) == npgs);
	ATF_REQUIRE(check_residency(addr2, npgs) == npgs);

	(void)munlockall();

	ATF_REQUIRE(madvise(addr2, npgs * page, MADV_FREE) == 0);
	ATF_REQUIRE(check_residency(addr2, npgs) == 0);

	(void)memset(addr, 0, npgs * page);

	ATF_REQUIRE(madvise(addr, npgs * page, MADV_FREE) == 0);
	ATF_REQUIRE(check_residency(addr, npgs) == 0);

	(void)munmap(addr, npgs * page);
	(void)munmap(addr2, npgs * page);
	(void)munmap(addr3, npgs * page);
	(void)unlink(path);
}

ATF_TC_CLEANUP(mincore_residency, tc)
{
	(void)unlink(path);
}

ATF_TC(mincore_shmseg);
ATF_TC_HEAD(mincore_shmseg, tc)
{
	atf_tc_set_md_var(tc, "descr", "residency of shared memory");
}

ATF_TC_BODY(mincore_shmseg, tc)
{
	size_t npgs = 128;
	void *addr = NULL;
	int shmid;

	shmid = shmget(IPC_PRIVATE, npgs * page,
	    IPC_CREAT | S_IRUSR | S_IWUSR);

	ATF_REQUIRE(shmid != -1);

	addr = shmat(shmid, NULL, 0);

	ATF_REQUIRE(addr != NULL);
	ATF_REQUIRE(check_residency(addr, npgs) == 0);

	(void)memset(addr, 0xff, npgs * page);

	ATF_REQUIRE(check_residency(addr, npgs) == npgs);
	ATF_REQUIRE(madvise(addr, npgs * page, MADV_FREE) == 0);

	/*
	 * NOTE!  Even though we have MADV_FREE'd the range,
	 * there is another reference (the kernel's) to the
	 * object which owns the pages.  In this case, the
	 * kernel does not simply free the pages, as haphazardly
	 * freeing pages when there are still references to
	 * an object can cause data corruption (say, the other
	 * referencer doesn't expect the pages to be freed,
	 * and is surprised by the subsequent ZFOD).
	 *
	 * Because of this, we simply report the number of
	 * pages still resident, for information only.
	 */
	npgs = check_residency(addr, npgs);

	(void)fprintf(stderr, "%zu pages still resident\n", npgs);

	ATF_REQUIRE(shmdt(addr) == 0);
	ATF_REQUIRE(shmctl(shmid, IPC_RMID, NULL) == 0);
}

ATF_TP_ADD_TCS(tp)
{

	page = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(page >= 0);

	ATF_TP_ADD_TC(tp, mincore_err);
	ATF_TP_ADD_TC(tp, mincore_incore);
	ATF_TP_ADD_TC(tp, mincore_residency);
	ATF_TP_ADD_TC(tp, mincore_shmseg);

	return atf_no_error();
}
