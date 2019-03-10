/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018, 2019 Andrew Turner
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/kcov.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <atf-c.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static int
open_kcov(void)
{
	int fd;

	fd = open("/dev/kcov", O_RDWR);
	if (fd == -1)
		atf_tc_skip("Failed to open /dev/kcov");

	return fd;
}

static int
pick_unassigned_fd(int greater_than_fd)
{
	int fd2;

	fd2 = greater_than_fd;
	do {
		++fd2;
	} while (fcntl(fd2, F_GETFL) != -1 || errno != EBADF);

	return fd2;
}

ATF_TC_WITHOUT_HEAD(kcov_dup2);
ATF_TC_BODY(kcov_dup2, tc)
{
	int fd1, fd2;
	fd1 = open_kcov();

	fd2 = pick_unassigned_fd(fd1);

	/* Test the dup2(2) trick used by syzkaller */
	ATF_REQUIRE_EQ(dup2(fd1, fd2), fd2);

	close(fd1);
	close(fd2);
}

ATF_TC_WITHOUT_HEAD(kcov_multiopen);
ATF_TC_BODY(kcov_multiopen, tc)
{
	int fd1, fd2;
	fd1 = open_kcov();

	fd2 = open("/dev/kcov", O_RDWR);
	ATF_REQUIRE(fd2 != -1);

	close(fd1);
	close(fd2);
}

ATF_TC_WITHOUT_HEAD(kcov_open_close_open);
ATF_TC_BODY(kcov_open_close_open, tc)
{
	int fd;

	fd = open_kcov();
	close(fd);
	fd = open("/dev/kcov", O_RDWR);
	ATF_REQUIRE(fd != -1);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_bufsize);
ATF_TC_BODY(kcov_bufsize, tc)
{
	int fd;
	uint64_t size;
	fd = open_kcov();

	size = 0;
	ATF_CHECK(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == -1);
	size = 2;
	ATF_CHECK(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_mmap);
ATF_TC_BODY(kcov_mmap, tc)
{
	void *data;
	int fd;
	uint64_t size = 2 * PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) == MAP_FAILED);

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0);

	ATF_REQUIRE((data = mmap(NULL, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0)) != MAP_FAILED);

	munmap(data, 2 * PAGE_SIZE);

	close(fd);
}

/* This shouldn't panic */
ATF_TC_WITHOUT_HEAD(kcov_mmap_no_munmap);
ATF_TC_BODY(kcov_mmap_no_munmap, tc)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_mmap_no_munmap_no_close);
ATF_TC_BODY(kcov_mmap_no_munmap_no_close, tc)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);
}

static sem_t sem1, sem2;

static void *
kcov_mmap_enable_thread(void *data)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;
	int mode;

	fd = open_kcov();
	*(int *)data = fd;

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);
	mode = KCOV_MODE_NONE;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);

	sem_post(&sem1);
	sem_wait(&sem2);

	return NULL;
}

ATF_TC_WITHOUT_HEAD(kcov_mmap_enable_thread_close);
ATF_TC_BODY(kcov_mmap_enable_thread_close, tc)
{
	pthread_t thread;
	int fd;

	sem_init(&sem1, 0, 0);
	sem_init(&sem2, 0, 0);
	pthread_create(&thread, NULL,
	    kcov_mmap_enable_thread, &fd);
	sem_wait(&sem1);
	close(fd);
	sem_post(&sem2);
	pthread_join(thread, NULL);
}

ATF_TC_WITHOUT_HEAD(kcov_enable);
ATF_TC_BODY(kcov_enable, tc)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;
	int mode;

	fd = open_kcov();

	mode = KCOV_MODE_NONE;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == -1);

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	/* We need to enable before disable */
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == -1);

	/* Check enabling works only with a valid trace method */
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == -1);

	/* Disable should only be called once */
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == -1);

	/* Re-enabling and changing mode should also work */
	mode = KCOV_MODE_NONE;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);
	mode = KCOV_MODE_TRACE_PC;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);
	mode = KCOV_MODE_TRACE_CMP;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_enable_no_disable);
ATF_TC_BODY(kcov_enable_no_disable, tc)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;
	int mode;

	fd = open_kcov();
	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	mode = KCOV_MODE_NONE;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_enable_no_disable_no_close);
ATF_TC_BODY(kcov_enable_no_disable_no_close, tc)
{
	int fd;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;
	int mode;

	fd = open_kcov();
	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	mode = KCOV_MODE_NONE;
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0);
}

static void *
common_head_raw(bool fd_dup, int *fdp)
{
	void *data;
	int fd, fd2;
	uint64_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	/* Test the dup2(2) trick used by syzkaller */
	if (fd_dup) {
		fd2 = pick_unassigned_fd(fd);
		ATF_REQUIRE_EQ(dup2(fd, fd2), fd2);
		close(fd);
		fd = fd2;
	}

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0,
	    "Unable to set the kcov buffer size");

	data = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	ATF_REQUIRE_MSG(data != MAP_FAILED, "Unable to mmap the kcov buffer");

	*fdp = fd;
	return data;
}

static void *
common_head(int *fdp)
{

	return common_head_raw(false, fdp);
}

static void
common_tail(int fd, kcov_int_t *data)
{

	ATF_REQUIRE_MSG(munmap(__UNVOLATILE(data), PAGE_SIZE) == 0,
	    "Unable to unmap the kcov buffer");

	close(fd);
}

static void
kcov_basic(bool fd_dup, int mode)
{
	kcov_int_t *buf;
	int fd;

	buf = common_head_raw(fd_dup, &fd);
	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0,
	    "Unable to enable kcov ");

	KCOV_STORE(buf[0], 0);

	sleep(0); /* XXX: Is it enough for all trace types? */
	ATF_REQUIRE_MSG(KCOV_LOAD(buf[0]) != 0, "No records found");

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);
}

ATF_TC_WITHOUT_HEAD(kcov_basic_pc);
ATF_TC_BODY(kcov_basic_pc, tc)
{

	kcov_basic(false, KCOV_MODE_TRACE_PC);
}

ATF_TC_WITHOUT_HEAD(kcov_basic_cmp);
ATF_TC_BODY(kcov_basic_cmp, tc)
{

	atf_tc_skip("XXX: GCC8 needed");

	kcov_basic(false, KCOV_MODE_TRACE_CMP);
}

ATF_TC_WITHOUT_HEAD(kcov_basic_dup2_pc);
ATF_TC_BODY(kcov_basic_dup2_pc, tc)
{

	kcov_basic(true, KCOV_MODE_TRACE_PC);
}

ATF_TC_WITHOUT_HEAD(kcov_basic_dup2_cmp);
ATF_TC_BODY(kcov_basic_dup2_cmp, tc)
{

	atf_tc_skip("XXX: GCC8 needed");

	kcov_basic(true, KCOV_MODE_TRACE_CMP);
}

ATF_TC_WITHOUT_HEAD(kcov_multienable_on_the_same_thread);
ATF_TC_BODY(kcov_multienable_on_the_same_thread, tc)
{
	kcov_int_t *buf1, *buf2;
	int fd1, fd2;
	int mode;

	buf1 = common_head(&fd1);
	buf2 = common_head(&fd2);
	mode = KCOV_MODE_NONE;
	ATF_REQUIRE_MSG(ioctl(fd1, KCOV_IOC_ENABLE, &mode) == 0,
	    "Unable to enable kcov");
	ATF_REQUIRE_ERRNO(EBUSY, ioctl(fd2, KCOV_IOC_ENABLE, &mode) != 0);

	ATF_REQUIRE_MSG(ioctl(fd1, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd1, buf1);
	common_tail(fd2, buf2);
}

static void *
thread_buffer_access_test_helper(void *ptr)
{
	kcov_int_t *buf = ptr;

	/* Test mapped buffer access from a custom thread */
	KCOV_STORE(buf[0], KCOV_LOAD(buf[0]));

	return NULL;
}

ATF_TC_WITHOUT_HEAD(kcov_buffer_access_from_custom_thread);
ATF_TC_BODY(kcov_buffer_access_from_custom_thread, tc)
{
	pthread_t thread;
	kcov_int_t *buf;
	int fd;
	int mode;

	buf = common_head(&fd);

	mode = KCOV_MODE_TRACE_PC;
	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0,
	    "Unable to enable kcov ");

	pthread_create(&thread, NULL, thread_buffer_access_test_helper,
	    __UNVOLATILE(buf));
	pthread_join(thread, NULL);

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);
}

static void *
thread_test_helper(void *ptr)
{
	volatile int i;

	/* It does not matter what operation is in action. */
	for (i = 0; i < 1000; i++) {
		if (getpid() == 0)
			break;
	}

	return NULL;
}

ATF_TC_WITHOUT_HEAD(kcov_thread);
ATF_TC_BODY(kcov_thread, tc)
{
	pthread_t thread;
	kcov_int_t *buf;
	int fd;
	int mode;
	volatile int i;

	buf = common_head(&fd);

	mode = KCOV_MODE_TRACE_PC;
	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0,
	    "Unable to enable kcov ");

	/* The thread does something, does not matter what exactly. */
	pthread_create(&thread, NULL, thread_test_helper, __UNVOLATILE(buf));

	KCOV_STORE(buf[0], 0);
	for (i = 0; i < 10000; i++)
		continue;
	ATF_REQUIRE_EQ_MSG(KCOV_LOAD(buf[0]), 0,
	    "Records changed in blocked thread");

	pthread_join(thread, NULL);

	ATF_REQUIRE_EQ_MSG(ioctl(fd, KCOV_IOC_DISABLE), 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);
}

static void *
multiple_threads_helper(void *ptr __unused)
{
	kcov_int_t *buf;
	int fd;
	int mode;

	buf = common_head(&fd);
	mode = KCOV_MODE_TRACE_PC;
	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE, &mode) == 0,
	    "Unable to enable kcov ");

	KCOV_STORE(buf[0], 0);

	sleep(0);
	ATF_REQUIRE_MSG(KCOV_LOAD(buf[0]) != 0, "No records found");

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);

	return NULL;
}

static void
kcov_multiple_threads(size_t N)
{
	pthread_t thread[32];
	size_t i;

	ATF_REQUIRE(__arraycount(thread) >= N);

	for (i = 0; i < N; i++)
		pthread_create(&thread[i], NULL, multiple_threads_helper, NULL);

	for (i = 0; i < N; i++)
		pthread_join(thread[i], NULL);
}

#define KCOV_MULTIPLE_THREADS(n)		\
ATF_TC_WITHOUT_HEAD(kcov_multiple_threads##n);	\
ATF_TC_BODY(kcov_multiple_threads##n, tc)	\
{						\
						\
	kcov_multiple_threads(n);		\
}

KCOV_MULTIPLE_THREADS(2)
KCOV_MULTIPLE_THREADS(4)
KCOV_MULTIPLE_THREADS(8)
KCOV_MULTIPLE_THREADS(16)
KCOV_MULTIPLE_THREADS(32)

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, kcov_dup2);
	ATF_TP_ADD_TC(tp, kcov_multiopen);
	ATF_TP_ADD_TC(tp, kcov_open_close_open);
	ATF_TP_ADD_TC(tp, kcov_bufsize);
	ATF_TP_ADD_TC(tp, kcov_mmap);
	ATF_TP_ADD_TC(tp, kcov_mmap_no_munmap);
	ATF_TP_ADD_TC(tp, kcov_mmap_no_munmap_no_close);
	ATF_TP_ADD_TC(tp, kcov_enable);
	ATF_TP_ADD_TC(tp, kcov_enable_no_disable);
	ATF_TP_ADD_TC(tp, kcov_enable_no_disable_no_close);
	ATF_TP_ADD_TC(tp, kcov_mmap_enable_thread_close);
	ATF_TP_ADD_TC(tp, kcov_basic_pc);
	ATF_TP_ADD_TC(tp, kcov_basic_cmp);
	ATF_TP_ADD_TC(tp, kcov_basic_dup2_pc);
	ATF_TP_ADD_TC(tp, kcov_basic_dup2_cmp);
	ATF_TP_ADD_TC(tp, kcov_multienable_on_the_same_thread);
	ATF_TP_ADD_TC(tp, kcov_buffer_access_from_custom_thread);
	ATF_TP_ADD_TC(tp, kcov_thread);
	ATF_TP_ADD_TC(tp, kcov_multiple_threads2);
	ATF_TP_ADD_TC(tp, kcov_multiple_threads4);
	ATF_TP_ADD_TC(tp, kcov_multiple_threads8);
	ATF_TP_ADD_TC(tp, kcov_multiple_threads16);
	ATF_TP_ADD_TC(tp, kcov_multiple_threads32);
	return atf_no_error();
}
