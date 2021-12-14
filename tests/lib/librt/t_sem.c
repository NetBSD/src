/* $NetBSD: t_sem.c,v 1.6 2021/12/14 16:25:11 wiz Exp $ */

/*
 * Copyright (c) 2008, 2010, 2019 The NetBSD Foundation, Inc.
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

/*
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008, 2010, 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_sem.c,v 1.6 2021/12/14 16:25:11 wiz Exp $");

#include <sys/mman.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include <atf-c.h>

#define NCHILDREN 10

ATF_TC_WITH_CLEANUP(basic);
ATF_TC_HEAD(basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks basic functionality of POSIX "
	    "semaphores");
}
ATF_TC_BODY(basic, tc)
{
	int val;
	sem_t *sem_b;

	if (sysconf(_SC_SEMAPHORES) == -1)
		atf_tc_skip("POSIX semaphores not supported");

	sem_b = sem_open("/sem_b", O_CREAT | O_EXCL, 0644, 0);
	ATF_REQUIRE(sem_b != SEM_FAILED);

	ATF_REQUIRE_EQ(sem_getvalue(sem_b, &val), 0);
	ATF_REQUIRE_EQ(val, 0);

	ATF_REQUIRE_EQ(sem_post(sem_b), 0);
	ATF_REQUIRE_EQ(sem_getvalue(sem_b, &val), 0);
	ATF_REQUIRE_EQ(val, 1);

	ATF_REQUIRE_EQ(sem_wait(sem_b), 0);
	ATF_REQUIRE_EQ(sem_trywait(sem_b), -1);
	ATF_REQUIRE_EQ(errno, EAGAIN);
	ATF_REQUIRE_EQ(sem_post(sem_b), 0);
	ATF_REQUIRE_EQ(sem_trywait(sem_b), 0);
	ATF_REQUIRE_EQ(sem_post(sem_b), 0);
	ATF_REQUIRE_EQ(sem_wait(sem_b), 0);
	ATF_REQUIRE_EQ(sem_post(sem_b), 0);

	ATF_REQUIRE_EQ(sem_close(sem_b), 0);
	ATF_REQUIRE_EQ(sem_unlink("/sem_b"), 0);
}
ATF_TC_CLEANUP(basic, tc)
{
	(void)sem_unlink("/sem_b");
}

ATF_TC_WITH_CLEANUP(child);
ATF_TC_HEAD(child, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks using semaphores to synchronize "
	    "parent with multiple child processes");
}
ATF_TC_BODY(child, tc)
{
	pid_t children[NCHILDREN];
	unsigned i, j;
	sem_t *sem_a;
	int status;

	pid_t pid;

	if (sysconf(_SC_SEMAPHORES) == -1)
		atf_tc_skip("POSIX semaphores not supported");

	sem_a = sem_open("/sem_a", O_CREAT | O_EXCL, 0644, 0);
	ATF_REQUIRE(sem_a != SEM_FAILED);

	for (j = 1; j <= 2; j++) {
		for (i = 0; i < NCHILDREN; i++) {
			switch ((pid = fork())) {
			case -1:
				atf_tc_fail("fork() returned -1");
			case 0:
				printf("PID %d waiting for semaphore...\n",
				    getpid());
				ATF_REQUIRE_MSG(sem_wait(sem_a) == 0,
				    "sem_wait failed; iteration %d", j);
				printf("PID %d got semaphore\n", getpid());
				_exit(0);
			default:
				children[i] = pid;
				break;
			}
		}

		for (i = 0; i < NCHILDREN; i++) {
			sleep(1);
			printf("main loop %d: posting...\n", j);
			ATF_REQUIRE_EQ(sem_post(sem_a), 0);
		}

		for (i = 0; i < NCHILDREN; i++) {
			ATF_REQUIRE_EQ(waitpid(children[i], &status, 0), children[i]);
			ATF_REQUIRE(WIFEXITED(status));
			ATF_REQUIRE_EQ(WEXITSTATUS(status), 0);
		}
	}

	ATF_REQUIRE_EQ(sem_close(sem_a), 0);
	ATF_REQUIRE_EQ(sem_unlink("/sem_a"), 0);
}
ATF_TC_CLEANUP(child, tc)
{
	(void)sem_unlink("/sem_a");
}

ATF_TC_WITH_CLEANUP(pshared);
ATF_TC_HEAD(pshared, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks using pshared unnamed "
	    "semaphores to synchronize a master with multiple slave processes");
}

struct shared_region {
	sem_t	the_sem;
};

static struct shared_region *
get_shared_region(int o_flags)
{

	int fd = shm_open("/shm_semtest_a", o_flags, 0644);
	ATF_REQUIRE(fd != -1);

	ATF_REQUIRE_EQ(ftruncate(fd, sizeof(struct shared_region)), 0);

	void *rv = mmap(NULL, sizeof(struct shared_region),
	    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	ATF_REQUIRE(rv != MAP_FAILED);

	(void)close(fd);

	return rv;
}

static void
put_shared_region(struct shared_region *r)
{
	ATF_REQUIRE_EQ(munmap(r, sizeof(struct shared_region)), 0);
}

ATF_TC_BODY(pshared, tc)
{
	struct shared_region *master_region, *slave_region;

	if (sysconf(_SC_SEMAPHORES) == -1)
		atf_tc_skip("POSIX semaphores not supported");

	/*
	 * Create a shared memory region to contain the pshared
	 * semaphore, create the semaphore there, and then detach
	 * from the shared memory region to ensure that our child
	 * processes will be getting at it from scratch.
	 */
	master_region = get_shared_region(O_RDWR | O_CREAT | O_EXCL);
	ATF_REQUIRE(sem_init(&master_region->the_sem, 1, 0) == 0);
	put_shared_region(master_region);

	/*
	 * Now execute a test that's essentially equivalent to the
	 * "child" test above, but using the pshared semaphore in the
	 * shared memory region.
	 */
	
	pid_t pid, children[NCHILDREN];
	unsigned i, j;
	int status;

	for (j = 1; j <= 2; j++) {
		for (i = 0; i < NCHILDREN; i++) {
			switch ((pid = fork())) {
			case -1:
				atf_tc_fail("fork() returned -1");
			case 0:
				slave_region = get_shared_region(O_RDWR);
				printf("PID %d waiting for semaphore...\n",
				    getpid());
				ATF_REQUIRE_MSG(sem_wait(&slave_region->the_sem)
				    == 0,
				    "sem_wait failed; iteration %d", j);
				printf("PID %d got semaphore\n", getpid());
				_exit(0);
			default:
				children[i] = pid;
				break;
			}
		}

		master_region = get_shared_region(O_RDWR);

		for (i = 0; i < NCHILDREN; i++) {
			sleep(1);
			printf("main loop %d: posting...\n", j);
			ATF_REQUIRE_EQ(sem_post(&master_region->the_sem), 0);
		}

		put_shared_region(master_region);

		for (i = 0; i < NCHILDREN; i++) {
			ATF_REQUIRE_EQ(waitpid(children[i], &status, 0), children[i]);
			ATF_REQUIRE(WIFEXITED(status));
			ATF_REQUIRE_EQ(WEXITSTATUS(status), 0);
		}
	}

	master_region = get_shared_region(O_RDWR);
	ATF_REQUIRE_EQ(sem_destroy(&master_region->the_sem), 0);
	put_shared_region(master_region);

	ATF_REQUIRE_EQ(shm_unlink("/shm_semtest_a"), 0);
}
ATF_TC_CLEANUP(pshared, tc)
{
	/*
	 * The kernel will g/c the pshared semaphore when the process that
	 * created it exits, so no need to include that in the cleanup here.
	 */
	(void)shm_unlink("/shm_semtest_a");
}

ATF_TC_WITH_CLEANUP(invalid_ops);
ATF_TC_HEAD(invalid_ops, tc)
{
	atf_tc_set_md_var(tc, "descr", "Validates behavior when calling "
	    "bad operations for the semaphore type");
}
ATF_TC_BODY(invalid_ops, tc)
{
	sem_t *sem;
	sem_t the_sem;

	sem = sem_open("/sem_c", O_CREAT | O_EXCL, 0644, 0);
	ATF_REQUIRE(sem != SEM_FAILED);
	ATF_REQUIRE(sem_destroy(sem) == -1 && errno == EINVAL);
	ATF_REQUIRE_EQ(sem_close(sem), 0);

	ATF_REQUIRE_EQ(sem_init(&the_sem, 0, 0), 0);
	ATF_REQUIRE(sem_close(&the_sem) == -1 && errno == EINVAL);
	ATF_REQUIRE_EQ(sem_destroy(&the_sem), 0);
}
ATF_TC_CLEANUP(invalid_ops, tc)
{
	(void)sem_unlink("/sem_c");
}

ATF_TC_WITH_CLEANUP(sem_open_address);
ATF_TC_HEAD(sem_open_address, tc)
{
	atf_tc_set_md_var(tc, "descr", "Validate that multiple sem_open calls "
	    "return the same address");
}
ATF_TC_BODY(sem_open_address, tc)
{
	sem_t *sem, *sem2, *sem3;
	atf_tc_expect_fail("kern/56549: consecutive sem_open() do not return the same address");
	sem = sem_open("/sem_d", O_CREAT | O_EXCL, 0777, 0);
	ATF_REQUIRE(sem != SEM_FAILED);
	sem2 = sem_open("/sem_d", O_CREAT | O_EXCL, 0777, 0);
	ATF_REQUIRE(sem2 == SEM_FAILED && errno == EEXIST);
	sem3 = sem_open("/sem_d", 0);
	ATF_REQUIRE(sem3 != SEM_FAILED);
	ATF_REQUIRE(sem == sem3);
	ATF_REQUIRE_EQ(sem_close(sem3), 0);
	ATF_REQUIRE_EQ(sem_close(sem), 0);
	ATF_REQUIRE_EQ(sem_unlink("/sem_d"), 0);
}
ATF_TC_CLEANUP(sem_open_address, tc)
{
	(void)sem_unlink("/sem_d");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, basic);
	ATF_TP_ADD_TC(tp, child);
	ATF_TP_ADD_TC(tp, pshared);
	ATF_TP_ADD_TC(tp, invalid_ops);
	ATF_TP_ADD_TC(tp, sem_open_address);

	return atf_no_error();
}
